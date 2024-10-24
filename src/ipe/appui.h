// -*- C++ -*-
// --------------------------------------------------------------------
// Appui
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

#ifndef APPUI_H
#define APPUI_H

#include "ipecanvas.h"

// avoid including Lua headers here
typedef struct lua_State lua_State;

#ifdef IPEUI_GTK
#include <gtk/gtk.h>
typedef GtkWidget *WINID;
typedef GtkMenu *MENUHANDLE;
#endif
#ifdef IPEUI_WIN32
#include <windows.h>
#include <commctrl.h>
typedef HWND WINID;
typedef HMENU MENUHANDLE;
#endif
#ifdef IPEUI_QT
#include <QWidget>
#include <QMenu>
typedef QWidget *WINID;
typedef QMenu *MENUHANDLE;
#endif
#ifdef IPEUI_COCOA
#include <Cocoa/Cocoa.h>
typedef NSWindow *WINID;
typedef NSMenu *MENUHANDLE;
#endif
#ifdef IPEUI_JS
typedef void *WINID;
typedef int MENUHANDLE;
#endif

using namespace ipe;

constexpr int COPYRIGHT_YEAR = 2024;

#define IPEABSOLUTE "<absolute>"

class AppUiBase;
extern WINID check_winid(lua_State *L, int i);
extern void push_winid(lua_State *L, WINID win);
extern String ipeIconDirectory();
extern Buffer ipeIcon(String action, int size);
extern AppUiBase *createAppUi(lua_State *L0, int model);

// --------------------------------------------------------------------

class AppUiBase : public CanvasObserver {
public:
  enum { EFileMenu, EEditMenu, EPropertiesMenu, ESnapMenu,
	 EModeMenu, EZoomMenu, ELayerMenu, EViewMenu,
	 EPageMenu, EIpeletMenu, EHelpMenu, ENumMenu };

  // change list selectorNames if enum changes
  enum { EUiStroke, EUiFill, EUiPen, EUiDashStyle,
	 EUiTextSize, EUiMarkShape, EUiSymbolSize, EUiOpacity,
	 EUiGridSize, EUiAngleSize,
	 EUiView, EUiPage, EUiViewMarked, EUiPageMarked };

  // tags for submenus
  // change list submenuNames if enum changes
  enum { ESubmenuGridSize = 1000, ESubmenuAngleSize,
	 ESubmenuTextStyle, ESubmenuLabelStyle,
	 ESubmenuSelectLayer, ESubmenuMoveLayer,
	 ESubmenuRecentFiles,
	 ESubmenuFin };

public:
  virtual ~AppUiBase();

  CanvasBase *canvas() { return iCanvas; }

  virtual void setupSymbolicNames(const Cascade *sheet);
  void setGridAngleSize(Attribute abs_grid, Attribute abs_angle);
  void setAttributes(const AllAttributes &all, Cascade *sheet);

  void wrapCall(String method, int nArgs);
  void luaShowPathStylePopup(Vector v);
  void luaBookmarkSelected(int index);
  void luaRecentFileSelected(String name);
  void resumeLua();
  void luaAbsoluteButton(const char *s);
  void luaSelector(String name, String value);
  void luaLayerAction(String name, String layer);
  void luaShowLayerBoxPopup(Vector v, String layer);
  void luaLayerOrderChanged(std::vector<String> & order);

  inline void setInkMode(bool ink) { isInkMode = ink; }
  static int readImage(lua_State *L, String fn);
  int model() const noexcept { return iModel; }

public:  // What platforms must implement:
  virtual WINID windowId() = 0;
  virtual int dpi() const;
  virtual void closeWindow() = 0;
  virtual bool actionState(const char *name) = 0;
  virtual void setActionState(const char *name, bool value) = 0;
  virtual void setNumbers(String vno, bool vm, String pno, bool pm) = 0;
  virtual void setLayers(const Page *page, int view) = 0;
  virtual void setZoom(double zoom) = 0;
  virtual void setWindowCaption(bool mod, const char *caption, const char *fn) = 0;
  virtual void setNotes(String notes) = 0;
  virtual void explain(const char *s, int t) = 0;
  virtual void showWindow(int width, int height, int x, int y, const Color & pathViewColor) = 0;
  virtual void setFullScreen(int mode) = 0;
  virtual void action(String name) = 0;
  virtual void setActionsEnabled(bool mode) = 0;
  virtual void setMouseIndicator(const char *s) = 0;
  virtual void setSnapIndicator(const char *s) = 0;
  virtual void setBookmarks(int no, const String *s) = 0;
  virtual void setToolVisible(int m, bool vis) = 0;
  virtual int pageSorter(lua_State *L, Document *doc, int pno,
			 int width, int height, int thumbWidth) = 0;
  virtual int clipboard(lua_State *L) = 0;
  virtual int setClipboard(lua_State *L) = 0;
  // Only used on Windows to compute shortcuts:
  virtual int actionInfo(lua_State *L) const;

  virtual void setRecentFileMenu(const std::vector<String> & names) = 0;

  virtual bool waitDialog(const char *cmd, const char *label) = 0; 

protected:   // What platforms must implement:
  virtual void addRootMenu(int id, const char *name) = 0;
  // if title == 0, add separator
  virtual void addItem(int id, const char *title = nullptr, const char *name = nullptr) = 0;
  virtual void startSubMenu(int id, const char *name, int tag = 0) = 0;
  virtual void addSubItem(const char *title, const char *name) = 0;
  virtual MENUHANDLE endSubMenu() = 0;
  virtual void addCombo(int sel, String s) = 0;
  virtual void resetCombos() = 0;
  virtual void addComboColors(AttributeSeq &sym, AttributeSeq &abs) = 0;
  virtual void setComboCurrent(int sel, int idx) = 0;
  virtual void setPathView(const AllAttributes &all, Cascade *sheet) = 0;
  virtual void setCheckMark(String name, Attribute a) = 0;
  virtual void setButtonColor(int sel, Color color) = 0;

protected:
  virtual void canvasObserverWheelMoved(double xDegrees, double yDegrees,
					int kind);
  virtual void canvasObserverMouseAction(int button);
  virtual void canvasObserverPositionChanged();
  virtual void canvasObserverToolChanged(bool hasTool);
  virtual void canvasObserverSizeChanged();

protected:
  AppUiBase(lua_State *L0, int model);
  static const char * const selectorNames[];

  void luaAction(String name);

  void buildMenus();
  void showInCombo(const Cascade *sheet, Kind kind,
		   int sel, const char *deflt = nullptr);
  void showMarksInCombo(const Cascade *sheet);
  void setAttribute(int sel, Attribute a);

  int ipeIcon(String action);

protected:
  lua_State *L;
  int iModel;  // reference to Lua model

  MENUHANDLE iRecentFileMenu;
  MENUHANDLE iSelectLayerMenu;
  MENUHANDLE iMoveToLayerMenu;
  MENUHANDLE iTextStyleMenu;
  MENUHANDLE iLabelStyleMenu;
  MENUHANDLE iGridSizeMenu;
  MENUHANDLE iAngleSizeMenu;

  Cascade *iCascade;
  AllAttributes iAll; // current settings in UI
  std::vector<String> iComboContents[EUiView];

  CanvasBase *iCanvas;

  int iWidthNotesBookmarks;
  std::vector<int> iScalings;
  String iCoordinatesFormat;
  int iMouseIn;
  double iMouseFactor;
  int iUiScale;
  int iToolbarScale;
  int iUiGap;
  bool isMiniEdit;
  bool iLeftDockFloats;
  bool isInkMode;

  std::unique_ptr<Document> ipeIcons;
  std::unique_ptr<Document> ipeIconsDark;
};

// --------------------------------------------------------------------
#endif
