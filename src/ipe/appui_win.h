// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for Win32
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

#ifndef APPUI_WIN_H
#define APPUI_WIN_H

#include "appui.h"

class PathView;
class LayerList;

using namespace ipe;

// --------------------------------------------------------------------

class AppUi : public AppUiBase {
public:
  static void init(HINSTANCE hInstance, int nCmdShow);
  static bool isDrawing(HWND target);

  AppUi(lua_State *L0, int model);
  ~AppUi();

  virtual int dpi() const override;

  virtual void action(String name) override;
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

  virtual int actionInfo(lua_State *L) const override;
  virtual int pageSorter(lua_State *L, Document *doc, int pno,
			 int width, int height, int thumbWidth) override;

  virtual int clipboard(lua_State *L) override;
  virtual int setClipboard(lua_State *L) override;

  virtual void setRecentFileMenu(const std::vector<String> & names) override;

  void toggleFullscreen();

private:
  void initUi();
  void setButtonIcons();
  void createFont();
  void createColorIcons();
  void layoutChildren(bool resizeRebar);
  void handleDpiChange(HWND hwnd, WPARAM wParam, LPARAM lParam);

  HBITMAP loadIcon(String action, int w, int h, int r0, int g0, int b0);
  int loadIcon(String action, HIMAGELIST il, int scale);
  HBITMAP loadButtonIcon(String action, int scale);

  void createAction(String name, String tooltip, bool canWhileDrawing = false);
  int findAction(const char *name) const;
  int actionId(const char *name) const;
  void addItem(HMENU menu, const char *title, const char *name);
  virtual void addRootMenu(int id, const char *name) override;
  virtual void addItem(int id, const char *title, const char *name) override;
  virtual void startSubMenu(int id, const char *name, int tag) override;
  virtual void addSubItem(const char *title, const char *name) override;
  virtual MENUHANDLE endSubMenu() override;
  virtual void setSnapIndicator(const char *s) override;
  virtual void setMouseIndicator(const char *s) override;
  virtual void addCombo(int sel, String s) override;
  virtual void resetCombos() override;
  virtual void addComboColors(AttributeSeq &sym, AttributeSeq &abs) override;
  virtual void setComboCurrent(int sel, int idx) override;
  virtual void setCheckMark(String name, Attribute a) override;
  virtual void setPathView(const AllAttributes &all, Cascade *sheet) override;
  virtual void setButtonColor(int sel, Color color) override;

  void populateTextStyleMenu();
  void populateLayerMenus();
  void populateSizeMenus();
  void populateSizeMenu(HMENU h, int sel, int base);
  void cmd(int id, int notification);
  void setCheckMark(String name, String value);
  void aboutIpe();
  void closeRequested();
  int iconId(const char *name) const;
  HWND createToolBar(HINSTANCE hInst);
  void addTButton(HWND tb, const char *name = nullptr, int flags = 0);
  void setTooltip(HWND h, String tip, bool isComboBoxEx = false);
  void toggleVisibility(String action, HWND h);
  void setLeftDockVisibility(bool vis);
  HWND createButton(HINSTANCE hInst, int id,
		    int flags = BS_BITMAP|BS_PUSHBUTTON);

  inline int uiscale(int x) { return iDpi * iUiScale * x / 9600; }

private:
  static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				  WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK enumThreadWndProc(HWND hwnd, LPARAM lParam);
  static const wchar_t className[];
private:
  struct SAction {
    String name;
    String tooltip;
    int icon;
    bool alwaysOn;
  };
  std::vector<SAction> iActions;
  HMENU hMenuBar;
  HMENU hRootMenu[ENumMenu];
  HIMAGELIST hIcons;
  HIMAGELIST hColorIcons;
  std::vector<Color> iColorIcons;
  HFONT hFont;

  HWND hwnd;
  int iDpi; // dpi of monitor currently hosting the window

  HWND hwndCanvas;

  HWND hTip;
  HWND hStatusBar;
  HWND hSnapTools;
  HWND hEditTools;
  HWND hObjectTools;
  int iToolButtonCount, iSnapButtons, iEditButtons, iObjectButtons;

  HWND hRebar;
  HWND hNotes;
  HWND hBookmarks;

  HWND hProperties;
  HWND hLayerGroup;
  HWND hNotesGroup;
  HWND hBookmarksGroup;

  HWND hButton[EUiOpacity]; // EUiDashStyle and EUiMarkShape are not used
  HWND hSelector[EUiView];
  HWND hViewNumber;
  HWND hPageNumber;
  HWND hViewMarked;
  HWND hPageMarked;
  PathView *iPathView;
  HWND hLayers;
  bool iSettingLayers;
  std::vector<String> iLayerNames;
  std::vector<String> iRecentFiles;
  // for fullscreen mode
  bool iFullScreen;
  bool iWasMaximized;
  RECT iWindowRect;
  LONG iWindowStyle;
  LONG iWindowExStyle;
};

// --------------------------------------------------------------------
#endif
