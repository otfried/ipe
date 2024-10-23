// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for GTK
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

#ifndef APPUI_GTK_H
#define APPUI_GTK_H

#include "appui.h"

using namespace ipe;

// --------------------------------------------------------------------

class AppUi : public AppUiBase {
public:
  AppUi(lua_State *L0, int model);
  ~AppUi();

  virtual void action(String name);
  virtual void setLayers(const Page *page, int view);

  virtual void setZoom(double zoom);
  virtual void setActionsEnabled(bool mode);
  virtual void setNumbers(String vno, bool vm, String pno, bool pm);
  virtual void setNotes(String notes);

  virtual WINID windowId();
  virtual void closeWindow();
  virtual bool actionState(const char *name);
  virtual void setActionState(const char *name, bool value);
  virtual void setWindowCaption(bool mod, const char *s);
  virtual void explain(const char *s, int t);
  virtual void showWindow(int width, int height, int x, int y, const Color & pathViewColor);
  virtual void setFullScreen(int mode);

  virtual void setBookmarks(int no, const String *s);
  virtual void setToolVisible(int m, bool vis);
  virtual int pageSorter(lua_State *L, Document *doc, int pno,
			 int width, int height, int thumbWidth);

  virtual int clipboard(lua_State *L);
  virtual int setClipboard(lua_State *L);

  virtual void setRecentFileMenu(const std::vector<String> & names) override;

  virtual bool waitDialog(const char *cmd, const char *label) override;

private:
  int actionId(const char *name) const;
  virtual void addRootMenu(int id, const char *name);
  void addItem(GtkMenuShell *shell, const char *title, const char *name);
  virtual void addItem(int id, const char *title, const char *name);
  virtual void startSubMenu(int id, const char *name, int tag);
  virtual void addSubItem(const char *title, const char *name);
  virtual MENUHANDLE endSubMenu();
  virtual void setMouseIndicator(const char *s);
  virtual void setSnapIndicator(const char *s);
  virtual void addCombo(int sel, String s);
  virtual void resetCombos();
  virtual void addComboColors(AttributeSeq &sym, AttributeSeq &abs);
  virtual void setComboCurrent(int sel, int idx);
  virtual void setCheckMark(String name, Attribute a);
  virtual void setPathView(const AllAttributes &all, Cascade *sheet);
  virtual void setButtonColor(int sel, Color color);

  static void menuitem_cb(GtkWidget *item, gpointer data);
private:
  struct SAction {
    String name;
    GtkWidget *menuItem;
  };
  std::vector<SAction> iActions;
  GtkWidget *iWindow;
  GtkWidget *iRootMenu[ENumMenu];
  GtkWidget *iSubMenu[ENumMenu];
  GtkWidget *iStatusBar;
  int iStatusBarContextid;
  GtkWidget *iMousePosition;
  GtkWidget *iResolution;
  GtkAccelGroup *iAccelGroup;
};

// --------------------------------------------------------------------
#endif
