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

#include "appui_js.h"
#include "ipecanvas_js.h"

#include "ipelua.h"

// #include "ipethumbs.h"

// #include <cstdio>
// #include <cstdlib>

// #include <sys/types.h>
// #include <sys/stat.h>

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static bool build_menus = true;

AppUi::AppUi(lua_State *L0, int model, Canvas *canvas)
  : AppUiBase(L0, model)
{
  iCanvas = canvas;
  buildMenus();
  build_menus = false; // all Windows share the same main menu
}

AppUi::~AppUi()
{
  ipeDebug("AppUi C++ destructor");
}

// --------------------------------------------------------------------

void AppUi::addRootMenu(int id, const char *name)
{
  ipeDebug("addRootMenu %d %s", id, name);
}

void AppUi::addItem(int id, const char *title, const char *name)
{
  ipeDebug("addItem %d %s", id, name);
}

static MENUHANDLE submenu = nullptr;

void AppUi::startSubMenu(int id, const char *name, int tag)
{
}

void AppUi::addSubItem(const char *title, const char *name)
{
}

MENUHANDLE AppUi::endSubMenu()
{
  return submenu;
}

// ------------------------------------------------------------------------

void AppUi::setRecentFileMenu(const std::vector<String> & names)
{
}

// --------------------------------------------------------------------

void AppUi::resetCombos()
{
  // for (int i = 0; i < EUiView; ++i)
  // iSelector[i]->clear();
}

void AppUi::addComboColors(AttributeSeq &sym, AttributeSeq &abs)
{
}

void AppUi::addCombo(int sel, String s)
{
}

void AppUi::setComboCurrent(int sel, int idx)
{
}

void AppUi::setButtonColor(int sel, Color color)
{
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
}

void AppUi::setCheckMark(String name, Attribute a)
{
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
}

void AppUi::setNotes(String notes)
{
}

void AppUi::setLayers(const Page *page, int view)
{
}

void AppUi::setBookmarks(int no, const String *s)
{
}

void AppUi::setToolVisible(int m, bool vis)
{
}

void AppUi::setZoom(double zoom)
{
}

// --------------------------------------------------------------------

void AppUi::setActionsEnabled(bool mode)
{
}

// --------------------------------------------------------------------

// Determine if action is checked
// Used for snapXXX, grid_visible, viewmarked, and pagemarked
bool AppUi::actionState(const char *name)
{
  return false;
}

// Check/uncheck an action
// Used for snapXXX, grid_visible, to initialize mode_select
void AppUi::setActionState(const char *name, bool value)
{
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

  /*
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("About Ipe");
  msgBox.setWindowIcon(prefsIcon("ipe"));
  msgBox.setInformativeText(buf.data());
  msgBox.setIconPixmap(prefsPixmap("ipe"));
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
  */
}

// --------------------------------------------------------------------

void AppUi::action(String name)
{
  /*
  if (name == "fullscreen") {
    setWindowState(windowState() ^ Qt::WindowFullScreen);
  } else 
  */
  if (name == "about") {
    aboutIpe();
  } else {
    // if (name.left(5) == "mode_")
    // iModeIndicator->setPixmap(prefsPixmap(name));
    luaAction(name);
  }
}

// --------------------------------------------------------------------

int AppUi::pageSorter(lua_State *L, Document *doc, int pno,
		      int width, int height, int thumbWidth)
{
  return 0;
}

// --------------------------------------------------------------------

WINID AppUi::windowId()
{
  return this;
}

void AppUi::closeWindow()
{
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  // setWindowModified(mod);
  // setWindowTitle(QString::fromUtf8(s));
}

void AppUi::setMouseIndicator(const char *s)
{
  // iMouse->setText(s);
}

void AppUi::setSnapIndicator(const char *s)
{
  // iSnapIndicator->setText(s);
}

void AppUi::explain(const char *s, int t)
{
  // statusBar()->showMessage(QString::fromUtf8(s), t);
}

void AppUi::showWindow(int width, int height, int x, int y, const Color & pathViewColor)
{
  /*
  iPathView->setColor(pathViewColor);
  if (width > 0 && height > 0)
    resize(width, height);
  if (x >= 0 && y >= 0)
    move(x, y);
  show();
  */
}

void AppUi::setFullScreen(int mode)
{
}

int AppUi::setClipboard(lua_State *L)
{
  return 0;
}

int AppUi::clipboard(lua_State *L)
{
  return 0;
}

// --------------------------------------------------------------------

int CanvasBase::selectPageOrView(Document *doc, int page, int startIndex,
				 int pageWidth, int width, int height)
{
  // TODO
  return -1;
}

// ------------------------------------------------------------------------

