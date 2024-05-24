// --------------------------------------------------------------------
// AppUi for QT
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "appui_qt.h"
#include "ipecanvas_qt.h"
#include "controls_qt.h"

#include "ipelua.h"

#include "ipethumbs.h"

#include <cstdio>
#include <cstdlib>

#include <QMenuBar>
#include <QKeySequence>
#include <QCloseEvent>
#include <QStatusBar>
#include <QGridLayout>
#include <QButtonGroup>
#include <QMessageBox>
#include <QSignalMapper>
#include <QToolTip>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QMimeData>
#include <QWindow>
#include <QScreen>

#include <sys/types.h>
#include <sys/stat.h>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

inline QSize adapt_size(const QSize &size, int factor)
{
  return QSize(factor * size.width() / 100, factor * size.height() / 100);
}

QIcon AppUi::prefsIcon(String name)
{
  if (name == "ipe") {
    String fname = ipeIconDirectory() + "icon_128x128.png";
    if (Platform::fileExists(fname))
      return QIcon(QIpe(fname));
  }

  String svgdir = Platform::latexDirectory() + "/icons/";
  if (!Platform::fileExists(svgdir) && mkdir(svgdir.z(), 0700) != 0)
    return QIcon();

  if (name == "ipe") {
    String png = svgdir + "icon_128x128.png";
    if (!Platform::fileExists(png)) {
      String fname = ipeIconDirectory() + "ipe_logo.ipe";
      std::unique_ptr<Document> doc(Document::loadWithErrorReport(fname.z()));
      if (!doc || doc->runLatex(fname) != Document::ErrNone)
	return QIcon();
      Thumbnail thumbs(doc.get(), 128);
      thumbs.setNoCrop(true);
      thumbs.saveRender(Thumbnail::EPNG, png.z(), doc->page(0), 0, 4.47);
    }
    return QIcon(QIpe(png));
  }

  String svgname = svgdir + name + ".svg";
  int pno = ipeIcon(name);
  if (pno >= 0) {
    bool dark = QGuiApplication::palette().text().color().value() > 128;
    Document *doc = dark ? ipeIconsDark.get() : ipeIcons.get();
    Thumbnail thumbs(doc, 22);
    thumbs.setNoCrop(true);
    thumbs.saveRender(Thumbnail::ESVG, svgname.z(), doc->page(pno), 0, 1.0);
    return QIcon(QIpe(svgname));
  } else
    return QIcon();
}

QPixmap AppUi::prefsPixmap(String name)
{
  QIcon icon = prefsIcon(name);
  int size = uiscale(24);
  return icon.pixmap(QSize(size, size));
}

QIcon AppUi::prefsColorIcon(Color color)
{
  int size = uiscale(16);
  QPixmap pixmap(size, size);
  pixmap.fill(QIpe(color));
  return QIcon(pixmap);
}

// --------------------------------------------------------------------

QAction *AppUi::findAction(const char *name) const
{
  std::map<String, QAction*>::const_iterator it = iActions.find(name);
  if (it != iActions.end())
    return it->second;
  else
    return nullptr;
}

void AppUi::addItem(QMenu *m, const QString &title, const char *name)
{
  if (name[0] == '@') {
    // canUseWhileDrawing = true;
    name = name + 1;
  }
  bool checkable = (m == iMenu[EModeMenu]) ||
    (String(name).find('|') >= 0);
  if (name[0] == '*') {
    checkable = true;
    name = name + 1;
  }
  QAction *a = new QAction(title, this);
  a->setIconVisibleInMenu(false);
  if (checkable)
    a->setCheckable(true);
  lua_getglobal(L, "shortcuts");
  lua_getfield(L, -1, name);
  if (lua_isstring(L, -1)) {
    QString s = lua_tostring(L, -1);
    a->setShortcut(QKeySequence(s));
    QString tt = title + " [" + s + "]";
    a->setToolTip(tt);
  }
  a->setIcon(prefsIcon(name));
  lua_pop(L, 2);
  m->addAction(a);
  if (m == iMenu[EModeMenu]) {
    a->setActionGroup(iModeActionGroup);
    iObjectTools->addAction(a);
  }
  connect(a, SIGNAL(triggered()), iActionMap, SLOT(map()));
  iActionMap->setMapping(a, name);
  iActions[name] = a;
}

void AppUi::addSnap(const char *name)
{
  QAction *a = findAction(name);
  assert(a);
  a->setCheckable(true);
  iSnapTools->addAction(a);
}

void AppUi::addEdit(const char *name)
{
  QAction *a = findAction(name);
  assert(a);
  iEditTools->addAction(a);
}

void AppUi::addRootMenu(int id, const char *name)
{
  iMenu[id] = menuBar()->addMenu(name);
}

// if title and name == 0, add separator (default)
void AppUi::addItem(int id, const char *title, const char *name)
{
  if (title == nullptr)
    iMenu[id]->addSeparator();
  else
    addItem(iMenu[id], QString::fromUtf8(title), name);
}

static QMenu *submenu = nullptr;
static int submenuId = 0;

void AppUi::startSubMenu(int id, const char *name, int tag)
{
  submenuId = id;
  submenu = new QMenu(name);
}

void AppUi::addSubItem(const char *title, const char *name)
{
  addItem(submenu, title, name);
}

MENUHANDLE AppUi::endSubMenu()
{
  iMenu[submenuId]->addMenu(submenu);
  return submenu;
}

Qt::DockWidgetArea getDockSide(lua_State *L, const char *name,
			       Qt::DockWidgetArea deflt)
{
  Qt::DockWidgetArea r = deflt;
  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "tools_placement");
  if (lua_istable(L, -1)) {
    lua_getfield(L, -1, name);
    const char *s = lua_tolstring(L, -1, nullptr);
    if (!strcmp(s, "left"))
      r = Qt::LeftDockWidgetArea;
    else if (!strcmp(s, "right"))
      r = Qt::RightDockWidgetArea;
    lua_pop(L, 1); // left or right
  }
  lua_pop(L, 2); // prefs, tools_placement
  return r;
}

static void set_toolbar_size(QToolBar *bar, int factor)
{
  QSize size = bar->iconSize();
  QSize size1 = adapt_size(size, factor);
  bar->setIconSize(size1);
  /*
  ipeDebug("toolbar %s has size %d x %d, changed to %d x %d",
	   bar->windowTitle().toUtf8().constData(), size.width(), size.height(),
	   size1.width(), size1.height());
  */
}

// --------------------------------------------------------------------

AppUi::AppUi(lua_State *L0, int model, Qt::WindowFlags f)
  : QMainWindow(nullptr, f), AppUiBase(L0, model)
{
  qApp->setWindowIcon(prefsIcon("ipe"));
  QMainWindow::setAttribute(Qt::WA_DeleteOnClose);
  setDockOptions(AnimatedDocks); // do not allow stacking properties and layers

  Canvas *canvas = new Canvas(this);
  iCanvas = canvas;
  setCentralWidget(canvas);

  iSnapTools = addToolBar("Snap");
  iEditTools = addToolBar("Edit");
  addToolBarBreak();
  iObjectTools = addToolBar("Objects");

  set_toolbar_size(iEditTools, iToolbarScale);
  set_toolbar_size(iSnapTools, iToolbarScale);
  set_toolbar_size(iObjectTools, iToolbarScale);

  iActionMap = new QSignalMapper(this);
  connect(iActionMap, SIGNAL(mappedString(const QString &)),
	  SLOT(qAction(const QString &)));

  iModeActionGroup = new QActionGroup(this);
  iModeActionGroup->setExclusive(true);
  QActionGroup *cg = new QActionGroup(this);
  cg->setExclusive(true);
  QActionGroup *cs = new QActionGroup(this);
  cs->setExclusive(true);

  buildMenus();

  findAction("coordinates|points")->setActionGroup(cg);
  findAction("coordinates|mm")->setActionGroup(cg);
  findAction("coordinates|m")->setActionGroup(cg);
  findAction("coordinates|inch")->setActionGroup(cg);

  for (uint i = 0; i < iScalings.size(); ++i) {
    char action[32];
    sprintf(action, "scaling|%d", iScalings[i]);
    findAction(action)->setActionGroup(cs);
  }

  connect(iRecentFileMenu, SIGNAL(triggered(QAction *)),
	  SLOT(recentFileAction(QAction *)));
  connect(iSelectLayerMenu, SIGNAL(triggered(QAction *)),
	  SLOT(selectLayerAction(QAction *)));
  connect(iMoveToLayerMenu, SIGNAL(triggered(QAction *)),
	  SLOT(moveToLayerAction(QAction *)));
  connect(iTextStyleMenu, SIGNAL(triggered(QAction *)),
	  SLOT(textStyleAction(QAction *)));
  connect(iLabelStyleMenu, SIGNAL(triggered(QAction *)),
	  SLOT(labelStyleAction(QAction *)));
  connect(iGridSizeMenu, SIGNAL(triggered(QAction *)),
	  SLOT(gridSizeAction(QAction *)));
  connect(iAngleSizeMenu, SIGNAL(triggered(QAction *)),
	  SLOT(angleSizeAction(QAction *)));

  QSignalMapper *comboMap = new QSignalMapper(this);

  iSelector[EUiGridSize] = new QComboBox();
  iSelector[EUiAngleSize] = new QComboBox();
  connect(iSelector[EUiGridSize], SIGNAL(activated(int)),
	  comboMap, SLOT(map()));
  connect(iSelector[EUiAngleSize], SIGNAL(activated(int)),
	  comboMap, SLOT(map()));
  comboMap->setMapping(iSelector[EUiGridSize], EUiGridSize);
  comboMap->setMapping(iSelector[EUiAngleSize], EUiAngleSize);

  addSnap("snapvtx");
  addSnap("snapctl");
  addSnap("snapbd");
  addSnap("snapint");
  addSnap("snapgrid");
  iSnapTools->addWidget(iSelector[EUiGridSize]);
  addSnap("snapangle");
  iSnapTools->addWidget(iSelector[EUiAngleSize]);
  addSnap("snapcustom");
  addSnap("snapauto");

  addEdit("copy");
  addEdit("cut");
  addEdit("paste");
  addEdit("delete");
  addEdit("undo");
  addEdit("redo");
  addEdit("zoom_in");
  addEdit("zoom_out");
  addEdit("fit_objects");
  addEdit("fit_page");
  addEdit("fit_width");
  addEdit("keyboard");
  iShiftKey = new QAction("shift_key", this);
  iShiftKey->setCheckable(true);
  iShiftKey->setIcon(prefsIcon("shift_key"));
  iEditTools->addAction(iShiftKey);
  iEditTools->addAction(findAction("grid_visible"));
  iAbortButton = new QAction("stop", this);
  iAbortButton->setIcon(prefsIcon("stop"));
  iEditTools->addAction(iAbortButton);
  connect(iShiftKey, SIGNAL(triggered()), SLOT(toolbarModifiersChanged()));
  connect(iAbortButton, SIGNAL(triggered()), SLOT(abortDrawing()));

  iPropertiesTools = new QDockWidget("Properties", this);
  addDockWidget(getDockSide(L, "properties", Qt::LeftDockWidgetArea),
		iPropertiesTools);
  iPropertiesTools->setAllowedAreas(Qt::LeftDockWidgetArea|
				    Qt::RightDockWidgetArea);

  iBookmarkTools = new QDockWidget("Bookmarks", this);
  addDockWidget(getDockSide(L, "bookmarks", Qt::RightDockWidgetArea),
		iBookmarkTools);
  iBookmarkTools->setAllowedAreas(Qt::LeftDockWidgetArea|
				  Qt::RightDockWidgetArea);
  iMenu[EPageMenu]->addAction(iBookmarkTools->toggleViewAction());

  iNotesTools = new QDockWidget("Notes", this);
  addDockWidget(getDockSide(L, "notes", Qt::RightDockWidgetArea),
		iNotesTools);
  iNotesTools->setAllowedAreas(Qt::LeftDockWidgetArea|
			       Qt::RightDockWidgetArea);
  iMenu[EPageMenu]->addAction(iNotesTools->toggleViewAction());

  iLayerTools = new QDockWidget("Layers", this);
  addDockWidget(getDockSide(L, "layers", Qt::LeftDockWidgetArea),
		iLayerTools);
  iLayerTools->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);

  // object names are used for saving toolbar state
  iSnapTools->setObjectName(QLatin1String("SnapTools"));
  iObjectTools->setObjectName(QLatin1String("ObjectTools"));
  iPropertiesTools->setObjectName(QLatin1String("PropertiesTools"));
  iLayerTools->setObjectName(QLatin1String("LayerTools"));
  iNotesTools->setObjectName(QLatin1String("NotesTools"));
  iBookmarkTools->setObjectName(QLatin1String("BookmarkTools"));

  QWidget *wg = new QFrame();
  QGridLayout *lo = new QGridLayout();
  wg->setLayout(lo);
  int m = uiscale(2);
  lo->setSpacing(1);
  lo->setContentsMargins(m, m, m, m); // l t r b
  // wg->setMaximumWidth(150);
  lo->setSizeConstraint(QLayout::SetFixedSize);
  QButtonGroup *bg = new QButtonGroup(wg);
  bg->setExclusive(false);
  connect(bg, SIGNAL(idClicked(int)), this, SLOT(absoluteButton(int)));
  iButton[EUiMarkShape] = nullptr;   // no such buttons
  iButton[EUiDashStyle] = nullptr;
  /* enum { EUiStroke, EUiFill, EUiPen, EUiDashStyle,
     EUiTextSize, EUiMarkShape, EUiSymbolSize, EUiOpacity ... }
     layout:
     0: stroke
     1: fill
     2: pen       \   pen button
     3: dashstyle /
     4: pathview
     5: textsize
     6: markshape   \ symbolsize button
     7: symbolsize  /
     8: opacity
   */
  for (int i = 0; i <= EUiOpacity; ++i) {
    if (i != EUiDashStyle && i != EUiMarkShape && i != EUiOpacity) {
      iButton[i] = new QToolButton();
      iButton[i]->setFocusPolicy(Qt::NoFocus);
      iButton[i]->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding,
					    QSizePolicy::ToolButton));
      bg->addButton(iButton[i], i);
      if (i == EUiPen)
	lo->addWidget(iButton[i], i, 0, 2, 1);
      else if (i == EUiSymbolSize)
	lo->addWidget(iButton[i], i, 0, 2, 1);
      else
	lo->addWidget(iButton[i], i >= EUiTextSize ? i+1 : i, 0);
      iButton[i]->setIconSize(adapt_size(iButton[i]->iconSize(), iUiScale));
    }
    iSelector[i] = new QComboBox();
    if (i == EUiOpacity)
      lo->addWidget(iSelector[i], i+1, 0, 1, 2);
    else
      lo->addWidget(iSelector[i], i >= EUiTextSize ? i+1 : i, 1);
    connect(iSelector[i], SIGNAL(activated(int)), comboMap, SLOT(map()));
    comboMap->setMapping(iSelector[i], i);
  }
  iButton[EUiStroke]->setIcon(prefsColorIcon(Color(1000, 0, 0)));
  iButton[EUiFill]->setIcon(prefsColorIcon(Color(1000, 1000, 0)));
  iButton[EUiPen]->setIcon(prefsIcon("pen"));
  iButton[EUiTextSize]->setIcon(prefsIcon("mode_label"));
  iButton[EUiSymbolSize]->setIcon(prefsIcon("mode_marks"));

  iButton[EUiStroke]->setToolTip("Absolute stroke color");
  iButton[EUiFill]->setToolTip("Absolute fill color");
  iButton[EUiPen]->setToolTip("Absolute pen width");
  iButton[EUiTextSize]->setToolTip("Absolute text size");
  iButton[EUiSymbolSize]->setToolTip("Absolute symbol size");

  iSelector[EUiStroke]->setToolTip("Symbolic stroke color");
  iSelector[EUiFill]->setToolTip("Symbolic fill color");
  iSelector[EUiPen]->setToolTip("Symbolic pen width");
  iSelector[EUiTextSize]->setToolTip("Symbolic text size");
  iSelector[EUiMarkShape]->setToolTip("Mark shape");
  iSelector[EUiSymbolSize]->setToolTip("Symbolic symbol size");
  iSelector[EUiDashStyle]->setToolTip("Dash style");
  iSelector[EUiOpacity]->setToolTip("Opacity");

  iSelector[EUiGridSize]->setToolTip("Grid size");
  iSelector[EUiAngleSize]->setToolTip("Angle for angular snap");

  connect(comboMap, SIGNAL(mappedInt(int)), this, SLOT(comboSelector(int)));

  iPathView = new PathView(uiscale(100));
  connect(iPathView, SIGNAL(activated(String)), SLOT(action(String)));
  connect(iPathView, SIGNAL(showPathStylePopup(Vector)),
	  SLOT(showPathStylePopup(Vector)));
  lo->addWidget(iPathView, 4, 1);
  iPropertiesTools->setWidget(wg);

  iModeIndicator = new QLabel();
  iModeIndicator->setPixmap(prefsPixmap("mode_select"));
  lo->addWidget(iModeIndicator, 4, 0);

  QHBoxLayout *hol = new QHBoxLayout();
  iViewNumber = new QToolButton();
  iPageNumber = new QToolButton();
  iViewNumber->setFocusPolicy(Qt::NoFocus);
  iPageNumber->setFocusPolicy(Qt::NoFocus);
  iViewNumber->setText("View 1/1");
  iViewNumber->setToolTip("Current view number");
  iPageNumber->setText("Page 1/1");
  iPageNumber->setToolTip("Current page number");
  iViewMarked = new QCheckBox();
  iPageMarked = new QCheckBox();
  iViewMarked->setFocusPolicy(Qt::NoFocus);
  iPageMarked->setFocusPolicy(Qt::NoFocus);
  bg->addButton(iViewNumber, EUiView);
  bg->addButton(iPageNumber, EUiPage);
  bg->addButton(iViewMarked, EUiViewMarked);
  bg->addButton(iPageMarked, EUiPageMarked);
  hol->setSpacing(0);
  hol->addWidget(iViewMarked);
  hol->addWidget(iViewNumber);
  hol->addStretch(1);
  hol->addWidget(iPageMarked);
  hol->addWidget(iPageNumber);
  lo->addLayout(hol, EUiOpacity + 2, 0, 1, -1);

  iPageNotes = new QTextEdit();
  iPageNotes->setAcceptRichText(false);
  iPageNotes->setReadOnly(true);
  iPageNotes->setFocusPolicy(Qt::NoFocus);
  iNotesTools->setWidget(iPageNotes);

  iBookmarks = new QListWidget();
  iBookmarks->setToolTip("Bookmarks of this document");
  iBookmarks->setFocusPolicy(Qt::NoFocus);
  connect(iBookmarks, SIGNAL(itemActivated(QListWidgetItem *)),
	  this, SLOT(bookmarkSelected(QListWidgetItem *)));
  iBookmarkTools->setWidget(iBookmarks);

  iLayerList = new LayerBox();
  iLayerList->setToolTip("Layers of this page");
  iLayerTools->setWidget(iLayerList);
  connect(iLayerList, SIGNAL(activated(String, String)),
	  this, SLOT(layerAction(String, String)));
  connect(iLayerList, SIGNAL(showLayerBoxPopup(Vector, String)),
	  SLOT(showLayerBoxPopup(Vector, String)));

  connect(iSelectLayerMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowSelectLayerMenu()));
  connect(iMoveToLayerMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowMoveToLayerMenu()));
  connect(iTextStyleMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowTextStyleMenu()));
  connect(iLabelStyleMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowLabelStyleMenu()));
  connect(iGridSizeMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowGridSizeMenu()));
  connect(iAngleSizeMenu, SIGNAL(aboutToShow()),
	  this, SLOT(aboutToShowAngleSizeMenu()));

  iSnapIndicator = new QLabel(statusBar());
  statusBar()->addPermanentWidget(iSnapIndicator, 0);
  QFont font = iSnapIndicator->font();
  font.setFamily("Monospace");
  iSnapIndicator->setFont(font);

  iMouse = new QLabel(statusBar());
  findAction("coordinates|points")->setChecked(true);
  findAction("scaling|1")->setChecked(true);
  statusBar()->addPermanentWidget(iMouse, 0);
  iMouse->setFont(font);

  iResolution = new QLabel(statusBar());
  statusBar()->addPermanentWidget(iResolution, 0);

  iCanvas->setObserver(this);
}

AppUi::~AppUi()
{
  ipeDebug("AppUi C++ destructor");
}

// --------------------------------------------------------------------

void AppUi::setRecentFileMenu(const std::vector<String> & names)
{
  iRecentFileMenu->clear();
  for (const auto & name : names) {
    QAction *a = new QAction(QIpe(name), iRecentFileMenu);
    iRecentFileMenu->addAction(a);
  }
}

void AppUi::recentFileAction(QAction *a)
{
  luaRecentFileSelected(IpeQ(a->text()));
}

static void populateLayerMenu(LayerBox *layerList, QMenu *menu)
{
  menu->clear();
  for (int i = 0; i < layerList->count(); ++i) {
    LayerItem *item = dynamic_cast<LayerItem *>(layerList->item(i));
    if (item) {
      QAction *a = new QAction(QIpe(item->ipeLayerName), menu);
      menu->addAction(a);
    }
  }

}

void AppUi::aboutToShowSelectLayerMenu()
{
  populateLayerMenu(iLayerList, iSelectLayerMenu);
}

void AppUi::selectLayerAction(QAction *a)
{
  action(String("selectinlayer-") + IpeQ(a->text()));
}

void AppUi::aboutToShowMoveToLayerMenu()
{
  populateLayerMenu(iLayerList, iMoveToLayerMenu);
}

void AppUi::moveToLayerAction(QAction *a)
{
  action(String("movetolayer-") + IpeQ(a->text()));
}

void AppUi::aboutToShowStyleMenu(Kind kind, MENUHANDLE menu, String current)
{
  AttributeSeq seq;
  iCascade->allNames(kind, seq);
  menu->clear();
  for (uint i = 0; i < seq.size(); ++i) {
    String s = seq[i].string();
    QAction *a = new QAction(QIpe(s), menu);
    a->setCheckable(true);
    if (s == current)
      a->setChecked(true);
    menu->addAction(a);
  }
}

void AppUi::aboutToShowTextStyleMenu()
{
  aboutToShowStyleMenu(ETextStyle, iTextStyleMenu, iAll.iTextStyle.string());
}

void AppUi::aboutToShowLabelStyleMenu()
{
  aboutToShowStyleMenu(ELabelStyle, iLabelStyleMenu, iAll.iLabelStyle.string());
}

void AppUi::aboutToShowGridSizeMenu()
{
  iGridSizeMenu->clear();
  for (auto &name : iComboContents[EUiGridSize]) {
    QString s = QIpe(name);
    QAction *a = new QAction(s, iGridSizeMenu);
    a->setCheckable(true);
    if (s == iSelector[EUiGridSize]->currentText())
      a->setChecked(true);
    iGridSizeMenu->addAction(a);
  }
}

void AppUi::aboutToShowAngleSizeMenu()
{
  iAngleSizeMenu->clear();
  for (auto &name : iComboContents[EUiAngleSize]) {
    QString s = QIpe(name);
    QAction *a = new QAction(s, iAngleSizeMenu);
    a->setCheckable(true);
    if (s == iSelector[EUiAngleSize]->currentText())
      a->setChecked(true);
    iAngleSizeMenu->addAction(a);
  }
}

void AppUi::textStyleAction(QAction *a)
{
  action(String("textstyle|") + IpeQ(a->text()));
}

void AppUi::labelStyleAction(QAction *a)
{
  action(String("labelstyle|") + IpeQ(a->text()));
}

void AppUi::gridSizeAction(QAction *a)
{
  action(String("gridsize|") + IpeQ(a->text()));
}

void AppUi::angleSizeAction(QAction *a)
{
  action(String("anglesize|") + IpeQ(a->text()));
}

void AppUi::toolbarModifiersChanged()
{
  if (iCanvas) {
    int mod = 0;
    if (iShiftKey->isChecked())
      mod |= CanvasBase::EShift;
    iCanvas->setAdditionalModifiers(mod);
  }
}

void AppUi::abortDrawing()
{
  action("stop");
}

// --------------------------------------------------------------------

void AppUi::resetCombos()
{
  for (int i = 0; i < EUiView; ++i)
    iSelector[i]->clear();
}

void AppUi::addComboColors(AttributeSeq &sym, AttributeSeq &abs)
{
  iSelector[EUiStroke]->addItem(IPEABSOLUTE);
  iSelector[EUiFill]->addItem(IPEABSOLUTE);
  iComboContents[EUiStroke].push_back(IPEABSOLUTE);
  iComboContents[EUiFill].push_back(IPEABSOLUTE);
  for (uint i = 0; i < sym.size(); ++i) {
    Color color = abs[i].color();
    String s = sym[i].string();
    QIcon icon = prefsColorIcon(color);
    iSelector[EUiStroke]->addItem(icon, QIpe(s));
    iSelector[EUiFill]->addItem(icon, QIpe(s));
    iComboContents[EUiStroke].push_back(s);
    iComboContents[EUiFill].push_back(s);
  }
}

void AppUi::addCombo(int sel, String s)
{
  iSelector[sel]->addItem(QIpe(s));
}

void AppUi::setComboCurrent(int sel, int idx)
{
  iSelector[sel]->setCurrentIndex(idx);
}

void AppUi::setButtonColor(int sel, Color color)
{
  iButton[sel]->setIcon(prefsColorIcon(color));
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
  iPathView->set(all, sheet);
}

void AppUi::setCheckMark(String name, Attribute a)
{
  String sa = name + "|";
  int na = sa.size();
  String sb = sa + a.string();
  for (std::map<String, QAction *>::iterator it = iActions.begin();
       it != iActions.end(); ++it) {
    if (it->first.left(na) == sa)
      it->second->setChecked(it->first == sb);
  }
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
  if (vno.empty()) {
    iViewNumber->hide();
    iViewMarked->hide();
  } else {
    iViewNumber->setText(QIpe(vno));
    iViewNumber->show();
    iViewMarked->setCheckState(vm ? Qt::Checked : Qt::Unchecked);
    iViewMarked->show();
  }
  if (pno.empty()) {
    iPageNumber->hide();
    iPageMarked->hide();
  } else {
    iPageNumber->show();
    iPageMarked->show();
    iPageNumber->setText(QIpe(pno));
    iPageMarked->setCheckState(pm ? Qt::Checked : Qt::Unchecked);
  }
}

void AppUi::setNotes(String notes)
{
  iPageNotes->setPlainText(QIpe(notes));
}

void AppUi::setLayers(const Page *page, int view)
{
  iLayerList->set(page, view);
}

void AppUi::setBookmarks(int no, const String *s)
{
  iBookmarks->clear();
  for (int i = 0; i < no; ++i) {
    QListWidgetItem *item = new QListWidgetItem(QIpe(s[i]));
    if (s[i][0] == ' ')
      item->setForeground(Qt::blue);
    iBookmarks->addItem(item);
  }
}

void AppUi::bookmarkSelected(QListWidgetItem *item)
{
  int index = iBookmarks->row(item);
  luaBookmarkSelected(index);
}

void AppUi::setToolVisible(int m, bool vis)
{
  QWidget *tool = nullptr;
  switch (m) {
  case 0: tool = iPropertiesTools; break;
  case 1: tool = iBookmarkTools; break;
  case 2: tool = iNotesTools; break;
  case 3: tool = iLayerTools; break;
  default: break;
  }
  if (vis)
    tool->show();
  else
    tool->hide();
}

void AppUi::setZoom(double zoom)
{
  QString s = QString("(%1ppi)").arg(int(72.0 * zoom));
  iResolution->setText(s);
  iCanvas->setZoom(zoom);
}

// --------------------------------------------------------------------

static void enableActions(QMenu *menu, bool mode)
{
  menu->setEnabled(mode);
  QListIterator<QAction *> it(menu->actions());
  while (it.hasNext())
    it.next()->setEnabled(mode);
}

void AppUi::setActionsEnabled(bool mode)
{
  enableActions(iMenu[EFileMenu], mode);
  enableActions(iMenu[EEditMenu], mode);
  enableActions(iMenu[EModeMenu], mode);
  enableActions(iMenu[EPropertiesMenu], mode);
  enableActions(iMenu[ELayerMenu], mode);
  enableActions(iMenu[EViewMenu], mode);
  enableActions(iMenu[EPageMenu], mode);
  enableActions(iMenu[EIpeletMenu], mode);

  iModeActionGroup->setEnabled(mode);
  iPropertiesTools->setEnabled(mode);
  iLayerTools->setEnabled(mode);
  iBookmarkTools->setEnabled(mode);
}

// --------------------------------------------------------------------

//! Window has been closed
void AppUi::closeEvent(QCloseEvent* ce)
{
  // calls model
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "closeEvent");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  lua_call(L, 1, 1);
  bool result = lua_toboolean(L, -1);
  if (result)
    ce->accept();
  else
    ce->ignore();
}

// --------------------------------------------------------------------

// Determine if action is checked
// Used for snapXXX, grid_visible, viewmarked, and pagemarked
bool AppUi::actionState(const char *name)
{
  if (!strcmp(name, "viewmarked"))
    return (iViewMarked->checkState() == Qt::Checked);
  if (!strcmp(name, "pagemarked"))
    return (iPageMarked->checkState() == Qt::Checked);

  QAction *a = findAction(name);
  if (a)
    return a->isChecked();
  else
    return 0;
}

// Check/uncheck an action
// Used for snapXXX, grid_visible, to initialize mode_select
void AppUi::setActionState(const char *name, bool value)
{
  QAction *a = findAction(name);
  if (a) a->setChecked(value);
}

void AppUi::absoluteButton(int id)
{
  luaAbsoluteButton(selectorNames[id]);
}

void AppUi::selector(int id, String value)
{
  luaSelector(String(selectorNames[id]), value);
}

void AppUi::comboSelector(int id)
{
  luaSelector(String(selectorNames[id]), IpeQ(iSelector[id]->currentText()));
}

// --------------------------------------------------------------------

static const char * const aboutText =
  "<qt><h1>Ipe %d.%d.%d</h1>"
  "<p>Copyright (c) 1993-%d Otfried Cheong</p>"
  "<p>The extensible drawing editor Ipe creates figures in PDF format, "
  "using LaTeX to format the text in the figures.</p>"
  "<p>Ipe is released under the GNU Public License.</p>"
  "<p>See the <a href=\"http://ipe.otfried.org\">Ipe homepage</a>"
  " for further information.</p>"
  "<p>If you are an Ipe fan and want to show others, have a look at the "
  "<a href=\"https://www.shirtee.com/en/store/ipe\">Ipe T-shirts</a>.</p>"
  "<h3>Platinum and gold sponsors</h3>"
  "<ul><li>Hee-Kap Ahn</li>"
  "<li>Günter Rote</li>"
  "<li>SCALGO</li>"
  "<li>Martin Ziegler</li></ul>"
  "<p>If you enjoy Ipe, feel free to treat the author on a cup of coffee at "
  "<a href=\"https://ko-fi.com/ipe7author\">Ko-fi</a>.</p>"
  "<p>You can also become a member of the exclusive community of "
  "<a href=\"http://patreon.com/otfried\">Ipe patrons</a>. "
  "For the price of a cup of coffee per month you can make a meaningful contribution "
  "to the continuing development of Ipe.</p>"
  "</qt>";

void AppUi::aboutIpe()
{
  std::vector<char> buf(strlen(aboutText) + 100);
  sprintf(buf.data(), aboutText,
	  IPELIB_VERSION / 10000,
	  (IPELIB_VERSION / 100) % 100,
	  IPELIB_VERSION % 100,
	  COPYRIGHT_YEAR);

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("About Ipe");
  msgBox.setWindowIcon(prefsIcon("ipe"));
  msgBox.setInformativeText(buf.data());
  msgBox.setIconPixmap(prefsPixmap("ipe"));
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

void AppUi::qAction(const QString &name)
{
  action(IpeQ(name));
}

void AppUi::action(String name)
{
  if (name == "fullscreen") {
    setWindowState(windowState() ^ Qt::WindowFullScreen);
  } else if (name == "about") {
    aboutIpe();
  } else {
    if (name.left(5) == "mode_")
      iModeIndicator->setPixmap(prefsPixmap(name));
    luaAction(name);
  }
}

// --------------------------------------------------------------------

int AppUi::pageSorter(lua_State *L, Document *doc, int pno,
		      int width, int height, int thumbWidth)
{
  QDialog *d = new QDialog();
  if (pno >= 0)
    d->setWindowTitle("Ipe View Sorter");
  else
    d->setWindowTitle("Ipe Page Sorter");

  QLayout *lo = new QVBoxLayout;
  PageSorter *p = new PageSorter(doc, pno, thumbWidth);
  QDialogButtonBox *buttonBox =
    new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  lo->addWidget(p);
  lo->addWidget(buttonBox);
  d->setLayout(lo);

  d->connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
  d->connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));

  d->resize(width, height);

  if (d->exec() == QDialog::Rejected) {
    delete d;
    return 0;
  }

  lua_createtable(L, p->count(), 0);
  for (int i = 1; i <= p->count(); ++i) {
    lua_pushinteger(L, p->pageAt(i-1) + 1);
    lua_rawseti(L, -2, i);
  }
  int m = p->iMarks.size();
  lua_createtable(L, m, 0);
  for (int i = 1; i <= m; ++i) {
    lua_pushboolean(L, p->iMarks[i-1]);
    lua_rawseti(L, -2, i);
  }
  delete d;
  return 2;
}

// --------------------------------------------------------------------

void AppUi::showPathStylePopup(Vector v)
{
  luaShowPathStylePopup(v);
}

void AppUi::showLayerBoxPopup(Vector v, String layer)
{
  luaShowLayerBoxPopup(v, layer);
}

void AppUi::layerAction(String name, String layer)
{
  luaLayerAction(name, layer);
}

// --------------------------------------------------------------------

WINID AppUi::windowId()
{
  return this;
}

void AppUi::closeWindow()
{
  close();
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  setWindowModified(mod);
  setWindowTitle(QString::fromUtf8(s));
}

void AppUi::setMouseIndicator(const char *s)
{
  iMouse->setText(s);
}

void AppUi::setSnapIndicator(const char *s)
{
  iSnapIndicator->setText(s);
}

void AppUi::explain(const char *s, int t)
{
  statusBar()->showMessage(QString::fromUtf8(s), t);
}

void AppUi::showWindow(int width, int height, int x, int y, const Color & pathViewColor)
{
  iPathView->setColor(pathViewColor);
  if (width > 0 && height > 0)
    resize(width, height);
  if (x >= 0 && y >= 0)
    move(x, y);
  show();
}

void AppUi::setFullScreen(int mode)
{
  Qt::WindowStates state = windowState() & ~(Qt::WindowFullScreen | Qt::WindowMaximized);
  switch (mode) {
  case 1:
    state |= Qt::WindowMaximized;
    break;
  case 2:
    state |= Qt::WindowFullScreen;
    break;
  default:
    break;
  }
  setWindowState(state);
}

int AppUi::setClipboard(lua_State *L)
{
  QString data = QString::fromUtf8(luaL_checkstring(L, 2));
  QClipboard *cb = QApplication::clipboard();
  cb->setText(data);
  return 0;
}

static int getImage(lua_State *L, QImage &img)
{
  QImage im1 = img.convertToFormat(QImage::Format_ARGB32);
  int w = im1.width();
  int h = im1.height();
  Buffer data(w * h * sizeof(uint32_t));
  uint32_t *d = (uint32_t *) data.data();
  for (int y = 0; y < h; ++y) {
    uint32_t *p = (uint32_t *) im1.scanLine(y);
    for (int x = 0; x < w; ++x) {
      *d++ = *p++;
    }
  }

  Bitmap bitmap(w, h, Bitmap::ENative, data);
  Rect r(Vector::ZERO, Vector(w, h));
  Image *im = new Image(r, bitmap);
  push_object(L, im);
  return 1;
}

int AppUi::clipboard(lua_State *L)
{
  bool allow_bitmap = lua_toboolean(L, 2);
  const QClipboard *cb = QApplication::clipboard();
  if (allow_bitmap) {
    const QMimeData *md = cb->mimeData();
    if (md->hasUrls()) {
      auto urls = md->urls();
      if (urls.size() == 1 && urls[0].isLocalFile()) {
	if (readImage(L, IpeQ(urls[0].toLocalFile())))
	  return 1;
      }
    }
    QImage img = cb->image();
    if (!img.isNull()) {
      return getImage(L, img);
    }
  }
  QString data = cb->text();
  lua_pushstring(L, data.toUtf8());
  return 1;
}

AppUiBase *createAppUi(lua_State *L0, int model)
{
  return new AppUi(L0, model);
}

// --------------------------------------------------------------------
