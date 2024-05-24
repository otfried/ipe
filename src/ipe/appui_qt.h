// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for QT
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

#ifndef APPUI_QT_H
#define APPUI_QT_H

#include "appui.h"

#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QCheckBox>
#include <QToolBar>
#include <QDockWidget>
#include <QActionGroup>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QTextEdit>

using namespace ipe;

// --------------------------------------------------------------------

class PathView;
class LayerBox;
class QSignalMapper;

// --------------------------------------------------------------------

class AppUi : public QMainWindow, public AppUiBase {
  Q_OBJECT

public:
  AppUi(lua_State *L0, int model, Qt::WindowFlags f=Qt::Widget);
  ~AppUi();

  QAction *findAction(const char *name) const;

  virtual void setLayers(const Page *page, int view) override;

  virtual void setZoom(double zoom) override;
  virtual void setActionsEnabled(bool mode) override;
  virtual void setNumbers(String vno, bool vm, String pno, bool pm) override;
  virtual void setNotes(String notes) override;

  virtual WINID windowId() override;
  virtual void closeWindow() override;
  virtual bool actionState(const char *name) override;
  virtual void setActionState(const char *name, bool value) override;
  virtual void setWindowCaption(bool mod, const char *s) override;
  virtual void explain(const char *s, int t) override;
  virtual void showWindow(int width, int height, int x, int y, const Color & pathViewColor) override;
  virtual void setFullScreen(int mode) override;
  virtual void setBookmarks(int no, const String *s) override;
  virtual void setToolVisible(int m, bool vis) override;
  virtual int pageSorter(lua_State *L, Document *doc, int pno,
			 int width, int height, int thumbWidth) override;
  virtual int clipboard(lua_State *L) override;
  virtual int setClipboard(lua_State *L) override;

  virtual void setRecentFileMenu(const std::vector<String> & names) override;

public slots:
  void action(String name) override;
  void qAction(const QString &name);
  void selectLayerAction(QAction *a);
  void moveToLayerAction(QAction *a);
  void textStyleAction(QAction *a);
  void labelStyleAction(QAction *a);
  void gridSizeAction(QAction *a);
  void angleSizeAction(QAction *a);

  void layerAction(String name, String layer);
  void toolbarModifiersChanged();
  void abortDrawing();
  void aboutIpe();

  void absoluteButton(int id);
  void selector(int id, String value);
  void comboSelector(int id);

  void bookmarkSelected(QListWidgetItem *item);
  void recentFileAction(QAction *a);

  void aboutToShowSelectLayerMenu();
  void aboutToShowMoveToLayerMenu();
  void aboutToShowTextStyleMenu();
  void aboutToShowLabelStyleMenu();
  void aboutToShowGridSizeMenu();
  void aboutToShowAngleSizeMenu();

  void showPathStylePopup(Vector v);
  void showLayerBoxPopup(Vector v, String layer);

private:
  void addItem(QMenu *m, const QString &title, const char *name);
  void addItem(int m, const QString &title, const char *name);
  void addSnap(const char *name);
  void addEdit(const char *name);
  QIcon prefsIcon(String name);
  QIcon prefsColorIcon(Color color);
  QPixmap prefsPixmap(String name);
  void aboutToShowStyleMenu(Kind kind, MENUHANDLE menu, String current);

  inline int uiscale(int x) { return iUiScale * x / 100; }

private:
  virtual void addRootMenu(int id, const char *name) override;
  virtual void addItem(int id, const char *title, const char *name) override;
  virtual void startSubMenu(int id, const char *name, int tag) override;
  virtual void addSubItem(const char *title, const char *name) override;
  virtual MENUHANDLE endSubMenu() override;
  virtual void setMouseIndicator(const char *s) override;
  virtual void setSnapIndicator(const char *s) override;
  virtual void addCombo(int sel, String s) override;
  virtual void resetCombos() override;
  virtual void addComboColors(AttributeSeq &sym, AttributeSeq &abs) override;
  virtual void setComboCurrent(int sel, int idx) override;
  virtual void setCheckMark(String name, Attribute a) override;
  virtual void setPathView(const AllAttributes &all, Cascade *sheet) override;
  virtual void setButtonColor(int sel, Color color) override;

protected:
  void closeEvent(QCloseEvent *ev) override;

private:
  PathView *iPathView;

  QMenu *iMenu[ENumMenu];

  QToolButton *iButton[EUiOpacity]; // EUiDashStyle and EUiMarkShape are not used
  QComboBox *iSelector[EUiView];

  QToolButton *iViewNumber;
  QToolButton *iPageNumber;

  QCheckBox *iViewMarked;
  QCheckBox *iPageMarked;

  QToolBar *iSnapTools;
  QToolBar *iEditTools;
  QToolBar *iObjectTools;

  QDockWidget *iPropertiesTools;
  QDockWidget *iLayerTools;
  QDockWidget *iBookmarkTools;
  QDockWidget *iNotesTools;

  QActionGroup *iModeActionGroup;

  QAction *iShiftKey;
  QAction *iAbortButton;

  QListWidget *iBookmarks;
  LayerBox *iLayerList;
  QTextEdit *iPageNotes;

  QLabel *iModeIndicator;
  QLabel *iSnapIndicator;
  QLabel *iMouse;
  QLabel *iResolution;

  QSignalMapper *iActionMap;
  std::map<String, QAction *> iActions;
};

// --------------------------------------------------------------------
#endif
