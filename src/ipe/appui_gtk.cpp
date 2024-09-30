// --------------------------------------------------------------------
// AppUi  for GTK
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

#include "appui_gtk.h"
#include "ipecanvas_gtk.h"
// #include "controls_qt.h"

#include "ipelua.h"

// for version info only
#include "ipefonts.h"
#include <zlib.h>

#include <cstdio>
#include <cstdlib>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static String change_mnemonic(const char *s)
{
  int n = strlen(s);
  String r;
  int i = 0;
  while (i < n) {
    if (s[i] == '&') {
      if (i+1 < n && s[i+1] == '&') {
	r += '&';
	++i; // extra
      } else
	r += '_';
    } else
      r += s[i];
    ++i;
  }
  return r;
}

void AppUi::addRootMenu(int id, const char *name)
{
  iRootMenu[id] = gtk_menu_item_new_with_mnemonic(change_mnemonic(name).z());
  iSubMenu[id] = gtk_menu_new();
}

void AppUi::menuitem_cb(GtkWidget *item, gpointer data)
{
  AppUi *ui = (AppUi *) g_object_get_data(G_OBJECT(item), "ipe-appui");
  gint idx = GPOINTER_TO_INT(data);
  ui->action(ui->iActions[idx].name);
}

void AppUi::addItem(GtkMenuShell *shell, const char *title, const char *name)
{
  if (!title) {
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(shell, item);
    gtk_widget_show(item);
    return;
  }
  bool checkable =
    (shell == GTK_MENU_SHELL(iSubMenu[EModeMenu]) ||
     String(name).find('|') >= 0);
  if (name[0] == '*') {
    checkable = true;
    name = name + 1;
  }

  GtkWidget *item = checkable ?
    gtk_check_menu_item_new_with_mnemonic(change_mnemonic(title).z()) :
    gtk_menu_item_new_with_mnemonic(change_mnemonic(title).z());
  SAction s;
  s.name = String(name);
  s.menuItem = item;
  iActions.push_back(s);
  g_object_set_data(G_OBJECT(item), "ipe-appui", this);
  gtk_menu_shell_append(shell, item);
  g_signal_connect(item, "activate", G_CALLBACK(menuitem_cb),
		   GINT_TO_POINTER(iActions.size() - 1));
  gtk_widget_show(item);
}

void AppUi::addItem(int id, const char *title, const char *name)
{
  GtkMenuShell *shell = GTK_MENU_SHELL(iSubMenu[id]);
  addItem(shell, title, name);
}

static GtkWidget *submenu = nullptr;
static GtkWidget *submenuitem = nullptr;
static int submenuId = 0;

void AppUi::startSubMenu(int id, const char *name, int tag)
{
  submenuId = id;
  submenu = gtk_menu_new();
  submenuitem = gtk_menu_item_new_with_mnemonic(change_mnemonic(name).z());
}

void AppUi::addSubItem(const char *title, const char *name)
{
  GtkMenuShell *shell = GTK_MENU_SHELL(submenu);
  addItem(shell, title, name);
}

MENUHANDLE AppUi::endSubMenu()
{
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenuitem), submenu);
  gtk_widget_show(submenuitem);
  GtkMenuShell *menu = GTK_MENU_SHELL(iSubMenu[submenuId]);
  gtk_menu_shell_append(menu, submenuitem);
  return GTK_MENU(submenu);
}

// --------------------------------------------------------------------

AppUi::AppUi(lua_State *L0, int model)
  : AppUiBase(L0, model)
{
  iWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(iWindow, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

  iAccelGroup = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(iWindow), iAccelGroup);

  buildMenus();

  GtkWidget *menu_bar = gtk_menu_bar_new();

  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(iWindow), vbox);
  gtk_widget_show(vbox);

  gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
  gtk_widget_show(menu_bar);

  for (int i = 0; i < ENumMenu; ++i) {
    gtk_widget_show(iRootMenu[i]);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(iRootMenu[i]), iSubMenu[i]);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), iRootMenu[i]);
  }

  Canvas *canvas = new Canvas(iWindow);
  gtk_box_pack_start(GTK_BOX(vbox), canvas->window(), TRUE, TRUE, 0);
  gtk_widget_show(canvas->window());
  iCanvas = canvas;
  iCanvas->setObserver(this);

  iStatusBar = gtk_statusbar_new();
  iStatusBarContextid = gtk_statusbar_get_context_id(GTK_STATUSBAR(iStatusBar),
						     "explain");
  iMousePosition = gtk_label_new(nullptr);
  iResolution = gtk_label_new(nullptr);
  GtkBox *sb =
    GTK_BOX(gtk_statusbar_get_message_area(GTK_STATUSBAR(iStatusBar)));
  gtk_box_pack_end(sb, iResolution, FALSE, FALSE, 0);
  gtk_box_pack_end(sb, iMousePosition, FALSE, FALSE, 0);
  gtk_widget_show(iResolution);
  gtk_widget_show(iMousePosition);
  gtk_box_pack_end(GTK_BOX(vbox), iStatusBar, FALSE, FALSE, 0);
  gtk_widget_show(iStatusBar);
}

AppUi::~AppUi()
{
  ipeDebug("AppUi C++ destructor");
  // TODO: destroy iWindow or not?
}

// --------------------------------------------------------------------

void AppUi::setRecentFileMenu(const std::vector<String> & names)
{
  // TODO
}

void AppUi::resetCombos()
{
  // TODO
}

void AppUi::addCombo(int sel, String s)
{
  // TODO
}

void AppUi::addComboColors(AttributeSeq &sym, AttributeSeq &abs)
{
  // TODO
}

void AppUi::setComboCurrent(int sel, int idx)
{
  // TODO
}

void AppUi::setButtonColor(int sel, Color color)
{
  // TODO
}

void AppUi::setLayers(const Page *page, int view)
{
  // TODO
}

void AppUi::setZoom(double zoom)
{
  char s[32];
  sprintf(s, "(%dppi)", int(72.0 * zoom));
  iCanvas->setZoom(zoom);
  gtk_label_set_text(GTK_LABEL(iResolution), s);
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
  // TODO
}

void AppUi::setCheckMark(String name, Attribute a)
{
}

void AppUi::setActionsEnabled(bool mode)
{
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
}

void AppUi::setNotes(String notes)
{
}

// only used for snapXXX and grid_size
bool AppUi::actionState(const char *name)
{
  int idx = actionId(name);
  return (idx >= 0) ?
    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(iActions[idx].menuItem))
    : false;
}

// only used for snapXXX and grid_size
void AppUi::setActionState(const char *name, bool value)
{
  int idx = actionId(name);
  if (idx >= 0)
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(iActions[idx].menuItem),
				   value);
}

void AppUi::setBookmarks(int no, const String *s)
{
  // TODO
}

void AppUi::setToolVisible(int m, bool vis)
{
  // TODO
}

// --------------------------------------------------------------------

int AppUi::actionId(const char *name) const
{
  for (int i = 0; i < int(iActions.size()); ++i) {
    if (iActions[i].name == name)
      return i;
  }
  return -1;
}

WINID AppUi::windowId()
{
  return iWindow;
}

void AppUi::closeWindow()
{
  delete this;
}

void AppUi::explain(const char *s, int t)
{
  gtk_statusbar_push(GTK_STATUSBAR(iStatusBar), iStatusBarContextid, s);
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  // TODO: What is mod for?
  gtk_window_set_title(GTK_WINDOW(iWindow), s);
}

void AppUi::setMouseIndicator(const char *s)
{
  gtk_label_set_text(GTK_LABEL(iMousePosition), s);
}

void AppUi::setSnapIndicator(const char *s)
{
  // TODO
}

void AppUi::showWindow(int width, int height, int x, int y, const Color & pathViewColor)
{
  gtk_widget_set_size_request(GTK_WIDGET(iWindow), width, height);
  gtk_widget_show(iWindow);
}

void AppUi::setFullScreen(int mode)
{
  // TODO
}

int AppUi::setClipboard(lua_State *L)
{
  const char *data = luaL_checkstring(L, 1);
  GtkClipboard *b = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text(b, data, -1);
  return 0;
}

int AppUi::clipboard(lua_State *L)
{
  // TODO IMPLEMENT IMAGE
  GtkClipboard *b = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  char *data = gtk_clipboard_wait_for_text(b);
  if (data) {
    lua_pushstring(L, data);
    g_free(data);
    return 1;
  }
  return 0;
}

// --------------------------------------------------------------------

void AppUi::action(String name)
{
  luaAction(name);
}

int AppUi::pageSorter(lua_State *L, Document *doc, int pno,
		      int width, int height, int thumbWidth)
{
  // TODO
  return 0;
}

bool AppUi::waitDialog(const char *cmd, const char *label)
{
  // TODO
  return true;
}

// --------------------------------------------------------------------

AppUiBase *createAppUi(lua_State *L0, int model)
{
  return new AppUi(L0, model);
}

// --------------------------------------------------------------------
