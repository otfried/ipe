// --------------------------------------------------------------------
// Lua bindings for GTK dialogs
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

#include "ipeui_common.h"
using String = std::string;

// --------------------------------------------------------------------

class PDialog : public Dialog {
public:
  PDialog(lua_State *L0, WINID parent, const char *caption, const char *language);
  virtual ~PDialog();

  virtual void setMapped(lua_State *L, int idx);
  virtual bool buildAndRun(int w, int h);
  virtual void retrieveValues();
  virtual void enableItem(int idx, bool value);
  virtual void acceptDialog(lua_State *L);
private:
  static void setListBoxRow(GtkTreeView *w, int row);
  static GtkWidget *createListBox(const SElement &m);
  static void fillListStore(GtkListStore *store, const SElement &m);
  static void itemResponse(GtkWidget *item, PDialog *dlg);

private:
  std::vector<GtkWidget *> iWidgets;
};

PDialog::PDialog(lua_State *L0, WINID parent, const char *caption, const char *language)
  : Dialog(L0, parent, caption, language)
{
  //
}

PDialog::~PDialog()
{
  //
}

void PDialog::acceptDialog(lua_State *L)
{
  int accept = lua_toboolean(L, 2);
  (void) accept; // TODO
  // QDialog::done(accept);
}

void PDialog::itemResponse(GtkWidget *item, PDialog *dlg)
{
  for (int i = 0; i < int(dlg->iWidgets.size()); ++i) {
    if (dlg->iWidgets[i] == item) {
      dlg->callLua(dlg->iElements[i].lua_method);
      return;
    }
  }
}

void PDialog::setMapped(lua_State *L, int idx)
{
  SElement &m = iElements[idx];
  GtkWidget *w = iWidgets[idx];
  switch (m.type) {
  case ELabel:
    gtk_label_set_text(GTK_LABEL(w), m.text.c_str());
    break;
  case ECheckBox:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), m.value);
    break;
  case ETextEdit:
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)),
			     m.text.c_str(), -1);
    break;
  case EInput:
    gtk_entry_set_text(GTK_ENTRY(w), m.text.c_str());
    break;
  case EList:
    if (lua_istable(L, 3)) {
      GtkTreeModel *mod = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
      fillListStore(GTK_LIST_STORE(mod), m);
    }
    setListBoxRow(GTK_TREE_VIEW(w), m.value);
    break;
  case ECombo:
    if (lua_istable(L, 3)) {
      GtkTreeModel *mod = gtk_combo_box_get_model(GTK_COMBO_BOX(w));
      fillListStore(GTK_LIST_STORE(mod), m);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(w), m.value);
    break;
  default:
    break;  // EButton
  }
}

static String getTextEdit(GtkWidget *w)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
  GtkTextIter start;
  GtkTextIter end;
  gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
  gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);
  gchar *s = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
  return String(s);
}

void PDialog::retrieveValues()
{
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    GtkWidget *w = iWidgets[i];
    switch (m.type) {
    case EInput:
      m.text = String(gtk_entry_get_text(GTK_ENTRY(w)));
      break;
    case ETextEdit:
      m.text = getTextEdit(w);
      break;
    case EList:
      {
	GtkTreeSelection *s = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(s, &model, &iter)) {
	  gint *path =
	    gtk_tree_path_get_indices(gtk_tree_model_get_path(model, &iter));
	  m.value = path[0];
	} else
	  m.value = 0;
      }
      break;
    case ECombo:
      m.value = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
      break;
    case ECheckBox:
      m.value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
      break;
    default:
      break;  // label and button - nothing to do
    }
  }
}

void PDialog::enableItem(int idx, bool value)
{
  gtk_widget_set_sensitive(iWidgets[idx], value);
}

void PDialog::setListBoxRow(GtkTreeView *w, int row)
{
  GtkTreeSelection *s = gtk_tree_view_get_selection(w);
  GtkTreePath *path = gtk_tree_path_new_from_indices(row, -1);
  gtk_tree_selection_select_path(s, path);
  gtk_tree_path_free(path);
}

void PDialog::fillListStore(GtkListStore *store, const SElement &m)
{
  GtkTreeIter iter;
  gtk_list_store_clear(store);
  for (int k = 0; k < int(m.items.size()); ++k) {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, m.items[k].c_str(), -1);
  }
}

GtkWidget *PDialog::createListBox(const SElement &m)
{
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
  fillListStore(store, m);
  GtkWidget *w = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(G_OBJECT(store));
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  // g_object_set(G_OBJECT(renderer), "foreground", "red", NULL);
  GtkTreeViewColumn *column =
    gtk_tree_view_column_new_with_attributes("Title", renderer,
					     "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(w), column);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w), false);
  GtkTreeSelection *s = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  gtk_tree_selection_set_mode(s, GTK_SELECTION_SINGLE);
  setListBoxRow(GTK_TREE_VIEW(w), m.value);
  return w;
}

static GtkWidget *addScrollBar(GtkWidget *w)
{
  GtkWidget *ww = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(ww),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ww),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(ww), w);
  return ww;
}

static void ctrlEnterResponse(GtkWidget *, GtkDialog *dlg)
{
  gtk_dialog_response(dlg, GTK_RESPONSE_ACCEPT);
}

static void escapeResponse(GtkWidget *, GtkDialog *dlg)
{
  // catching escape, doing nothing
}

bool PDialog::buildAndRun(int w, int h)
{
  hDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(hDialog), iCaption.c_str());

  GtkAccelGroup *accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(hDialog), accel_group);
  guint accel_key;
  GdkModifierType accel_mods;
  gtk_accelerator_parse("<Control>Return", &accel_key, &accel_mods);
  gtk_accel_group_connect(accel_group, accel_key, accel_mods, GtkAccelFlags(0),
			  g_cclosure_new(G_CALLBACK(ctrlEnterResponse),
					 hDialog, NULL));
  if (iIgnoreEscapeField >= 0) {
    gtk_accelerator_parse("Escape", &accel_key, &accel_mods);
    gtk_accel_group_connect(accel_group, accel_key, accel_mods,
			    GtkAccelFlags(0),
			    g_cclosure_new(G_CALLBACK(escapeResponse),
					   hDialog, NULL));
  }

  if (w > 0 && h > 0)
    gtk_window_set_default_size(GTK_WINDOW(hDialog), w, h);

  GtkWidget *ca = gtk_dialog_get_content_area(GTK_DIALOG(hDialog));
  GtkWidget *grid = gtk_table_new(iNoRows, iNoCols, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(grid), 8);
  gtk_table_set_col_spacings(GTK_TABLE(grid), 8);
  gtk_table_set_homogeneous(GTK_TABLE(grid), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
  gtk_box_pack_start(GTK_BOX(ca), grid, TRUE, TRUE, 0);
  gtk_widget_show(grid);

  GtkWidget *aa = gtk_dialog_get_action_area(GTK_DIALOG(hDialog));

  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    GtkWidget *w = nullptr;
    GtkWidget *ww = nullptr; // for alignment
    int xOptions = 0; // GTK_EXPAND|GTK_SHRINK|GTK_FILL
    int yOptions = 0; // GTK_EXPAND|GTK_SHRINK|GTK_FILL
    if (m.row < 0) {
      if (m.flags & EAccept) {
	w = gtk_dialog_add_button(GTK_DIALOG(hDialog), m.text.c_str(),
				  GTK_RESPONSE_ACCEPT);
	gtk_widget_set_can_default(w, TRUE);
	gtk_widget_grab_default(w);
      } else if (m.flags & EReject)
	w = gtk_dialog_add_button(GTK_DIALOG(hDialog), m.text.c_str(),
				  GTK_RESPONSE_ACCEPT);
      else {
	w = gtk_button_new_with_label(m.text.c_str());
	gtk_box_pack_start(GTK_BOX(aa), w, FALSE, FALSE, 0);
	gtk_widget_show(w);
	if (m.lua_method)
	  g_signal_connect(w, "clicked", G_CALLBACK(itemResponse), this);
      }
    } else {
      switch (m.type) {
      case ELabel:
	ww = gtk_alignment_new(0.0, 0.5, 0.0, 1.0);
	w = gtk_label_new(m.text.c_str());
	gtk_container_add(GTK_CONTAINER(ww), w);
	xOptions |= GTK_FILL; // left align in cell
	break;
      case EButton:
	w = gtk_button_new_with_label(m.text.c_str());
	break;
      case ECheckBox:
	w = gtk_check_button_new_with_label(m.text.c_str());
	if (m.lua_method)
	  g_signal_connect(w, "toggled", G_CALLBACK(itemResponse), this);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), m.value);
	break;
      case EInput:
	w = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
	xOptions |= GTK_FILL;
	break;
      case ETextEdit:
	w = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(w), !(m.flags & EReadOnly));
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)),
				 m.text.c_str(), -1);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w), GTK_WRAP_WORD);
	ww = addScrollBar(w);
	xOptions |= GTK_FILL;
	yOptions |= GTK_FILL;
	break;
      case ECombo:
	{
	  ww = gtk_alignment_new(0.5, 0.0, 1.0, 0.0);
	  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
	  fillListStore(store, m);
	  w = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(w), renderer, TRUE);
	  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(w), renderer,
					"text", 0);
	  gtk_container_add(GTK_CONTAINER(ww), w);
	  gtk_combo_box_set_active(GTK_COMBO_BOX(w), m.value);
	  xOptions |= GTK_FILL;
	  yOptions |= GTK_FILL;  // align at top
	}
	break;
      case EList:
	w = createListBox(m);
	ww = addScrollBar(w);
	xOptions |= GTK_FILL;
	yOptions |= GTK_FILL;
	break;
      default:
	break;
      }
      for (int r = m.row; r < m.row + m.rowspan; ++r)
	if (iRowStretch[r] > 0)
	  yOptions |= GTK_EXPAND;
      for (int c = m.col; c < m.col + m.colspan; ++c)
	if (iColStretch[c] > 0)
	  xOptions |= GTK_EXPAND;
      if (ww == nullptr)
	ww = w;
      if (ww != nullptr) {
	gtk_table_attach(GTK_TABLE(grid), ww,
			 m.col, m.col+m.colspan, m.row, m.row+m.rowspan,
			 GtkAttachOptions(xOptions),
			 GtkAttachOptions(yOptions), 0, 0);
	gtk_widget_show(ww);
	gtk_widget_show(w);
      }
    }
    if (m.flags & EDisabled)
      gtk_widget_set_sensitive(w, false);
    iWidgets.push_back(w);
  }
  gint result = gtk_dialog_run(GTK_DIALOG(hDialog));
  retrieveValues(); // for future reference
  gtk_widget_destroy(hDialog);
  hDialog = NULL;
  return (result == GTK_RESPONSE_ACCEPT);
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State *L)
{
  WINID parent = check_winid(L, 1);
  const char *s = luaL_checkstring(L, 2);
  const char *language = "";
  if (lua_isstring(L, 3))
    language = luaL_checkstring(L, 3);

  Dialog **dlg = (Dialog **) lua_newuserdata(L, sizeof(Dialog *));
  *dlg = nullptr;
  luaL_getmetatable(L, "Ipe.dialog");
  lua_setmetatable(L, -2);
  *dlg = new PDialog(L, parent, s, language);
  return 1;
}

// --------------------------------------------------------------------

class PMenu : public Menu {
public:
  PMenu(WINID parent);
  virtual ~PMenu();
  virtual int add(lua_State *L);
  virtual int execute(lua_State *L);
private:
  static void itemResponse(GtkWidget *item, PMenu *menu);
  static void deactivateResponse(GtkMenuShell *, PMenu *);
  static void positionResponse(GtkMenu *, gint *x, gint *y,
			       gboolean *push_in, PMenu *menu);
private:
  GtkWidget *iMenu;
  struct Item {
    gchar *name;
    gchar *itemName;
    int itemIndex;
    GtkWidget *widget;
  };
  std::vector<Item> items;
  int iSelectedItem;
  int iPopupX;
  int iPopupY;
};

void PMenu::itemResponse(GtkWidget *item, PMenu *menu)
{
  for (int i = 0; i < int(menu->items.size()); ++i) {
    if (menu->items[i].widget == item) {
      menu->iSelectedItem = i;
      return;
    }
  }
}

void PMenu::deactivateResponse(GtkMenuShell *, PMenu *)
{
  gtk_main_quit(); // drop out of nested loop
}

// NOT USED: Better to just let GTK use the cursor position
void PMenu::positionResponse(GtkMenu *, gint *x, gint *y,
			     gboolean *push_in, PMenu *menu)
{
  *x = menu->iPopupX;
  *y = menu->iPopupY;
  *push_in = TRUE;
}

PMenu::PMenu(WINID parent)
{
  iMenu = gtk_menu_new();
  g_object_ref_sink(iMenu);
  g_signal_connect(iMenu, "deactivate",
		   G_CALLBACK(deactivateResponse), this);
}

PMenu::~PMenu()
{
  for (int i = 0; i < int(items.size()); ++i) {
    g_free(items[i].name);
    g_free(items[i].itemName);
  }
  g_object_unref(iMenu);
}

int PMenu::execute(lua_State *L)
{
  iPopupX = (int)luaL_checkinteger(L, 2);
  iPopupY = (int)luaL_checkinteger(L, 3);
  iSelectedItem = -1;
  gtk_menu_popup(GTK_MENU(iMenu), NULL, NULL,
		 // GtkMenuPositionFunc(positionResponse), this,
		 NULL, NULL,
		 0,    // initiated by button release
		 gtk_get_current_event_time());

  // nested main loop
  gtk_main();

  if (0 <= iSelectedItem && iSelectedItem < int(items.size())) {
    lua_pushstring(L, items[iSelectedItem].name);
    lua_pushinteger(L, items[iSelectedItem].itemIndex);
    if (items[iSelectedItem].itemName)
      lua_pushstring(L, items[iSelectedItem].itemName);
    else
      lua_pushstring(L, "");
    return 3;
  } else
    return 0;
}

static GtkWidget *colorIcon(double red, double green, double blue)
{
  GtkWidget *w = gtk_drawing_area_new();
  gtk_widget_set_size_request(w, 13, 13);
  GdkColor color;
  color.red = int(red * 65535.0);
  color.green = int(green * 65535.0);
  color.blue = int(blue * 65535.0);
  gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &color);
  g_object_ref_sink(w);
  return w;
}

int PMenu::add(lua_State *L)
{
  const char *name = luaL_checkstring(L, 2);
  const char *title = luaL_checkstring(L, 3);
  if (lua_gettop(L) == 3) {
    GtkWidget *w = gtk_menu_item_new_with_label(title);
    gtk_menu_shell_append(GTK_MENU_SHELL(iMenu), w);
    g_signal_connect(w, "activate", G_CALLBACK(itemResponse), this);
    gtk_widget_show(w);
    Item item;
    item.name = g_strdup(name);
    item.itemName = nullptr;
    item.itemIndex = 0;
    item.widget = w;
    items.push_back(item);
  } else {
    luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
    bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
    bool hastable = !hasmap && !lua_isnoneornil(L, 5);
    bool hascolor = !lua_isnoneornil(L, 6) && lua_isfunction(L, 6);
    bool hascheck = !hascolor && !lua_isnoneornil(L, 6);
    if (hastable)
      luaL_argcheck(L, lua_istable(L, 5), 5,
		    "argument is not a function or table");
    const char *current = nullptr;
    if (hascheck) {
      luaL_argcheck(L, lua_isstring(L, 6), 6,
		    "argument is not a function or string");
      current = luaL_checkstring(L, 6);
    }

    GtkWidget *sm = gtk_menu_new();

    int no = lua_rawlen(L, 4);
    for (int i = 1; i <= no; ++i) {
      lua_rawgeti(L, 4, i);
      luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
      const char *item = lua_tostring(L, -1);
      if (hastable) {
	lua_rawgeti(L, 5, i);
	luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
      } else if (hasmap) {
	lua_pushvalue(L, 5);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -3);  // name
	lua_call(L, 2, 1);     // function returns label
	luaL_argcheck(L, lua_isstring(L, -1), 5,
		      "function does not return string");
      } else
	lua_pushvalue(L, -1);

      const char *text = lua_tostring(L, -1);

      GtkWidget *w = nullptr;
      if (hascheck)
	w = gtk_check_menu_item_new_with_label(text);
      else if (hascolor)
	w = gtk_image_menu_item_new_with_label(text);
      else
	w = gtk_menu_item_new_with_label(text);
      if (hascheck && !g_strcmp0(item, current))
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), true);
      gtk_menu_shell_append(GTK_MENU_SHELL(sm), w);
      g_signal_connect(w, "activate", G_CALLBACK(itemResponse), this);
      gtk_widget_show(w);
      Item mitem;
      mitem.name = g_strdup(name);
      mitem.itemName = g_strdup(item);
      mitem.itemIndex = i;
      mitem.widget = w;
      items.push_back(mitem);

      if (hascolor) {
	gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(w),
						  true);
	lua_pushvalue(L, 6);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -4);  // name
	lua_call(L, 2, 3);     // function returns red, green, blue
	double red = luaL_checknumber(L, -3);
	double green = luaL_checknumber(L, -2);
	double blue = luaL_checknumber(L, -1);
	lua_pop(L, 3);         // pop result
	GtkWidget *im = colorIcon(red, green, blue);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(w), im);
	g_object_unref(im);
      }
      lua_pop(L, 2); // item, text
    }
    GtkWidget *sme = gtk_menu_item_new_with_label(title);
    gtk_widget_show(sme);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sme), sm);
    gtk_menu_shell_append(GTK_MENU_SHELL(iMenu), sme);
    gtk_widget_show(sme);
  }
  return 0;
}

// --------------------------------------------------------------------

static int menu_constructor(lua_State *L)
{
  GtkWidget *parent = check_winid(L, 1);
  Menu **m = (Menu **) lua_newuserdata(L, sizeof(Menu *));
  *m = nullptr;
  luaL_getmetatable(L, "Ipe.menu");
  lua_setmetatable(L, -2);
  *m = new PMenu(parent);
  return 1;
}

// --------------------------------------------------------------------

static int ipeui_getColor(lua_State *L)
{
  check_winid(L, 1);
  const char *title = luaL_checkstring(L, 2);
  double r = luaL_checknumber(L, 3);
  double g = luaL_checknumber(L, 4);
  double b = luaL_checknumber(L, 5);

  GdkColor color;
  color.red = int(r * 65535);
  color.green = int(g * 65535);
  color.blue = int(b * 65535);

  GtkWidget *dlg = gtk_color_selection_dialog_new(title);
  GtkColorSelection *sel =
    GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection
			(GTK_COLOR_SELECTION_DIALOG(dlg)));
  gtk_color_selection_set_current_color(sel, &color);
  int result = gtk_dialog_run(GTK_DIALOG(dlg));
  if (result == GTK_RESPONSE_OK) {
    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(sel), &color);
    gtk_widget_destroy(dlg);
    lua_pushnumber(L, color.red / 65535.0);
    lua_pushnumber(L, color.green / 65535.0);
    lua_pushnumber(L, color.blue / 65535.0);
    return 3;
  }
  gtk_widget_destroy(dlg);
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_fileDialog(lua_State *L)
{
  static const char * const typenames[] = { "open", "save", nullptr };
  GtkWindow *parent = GTK_WINDOW(check_winid(L, 1));
  int type = luaL_checkoption(L, 2, nullptr, typenames);
  const char *caption = luaL_checkstring(L, 3);
  // GTK dialog uses no filters: args 4 and 7 are not used
  const char *dir = nullptr;
  if (!lua_isnoneornil(L, 5))
    dir = luaL_checkstring(L, 5);
  const char *name = nullptr;
  if (!lua_isnoneornil(L, 6))
    name = luaL_checkstring(L, 6);

  GtkWidget *dlg =
    gtk_file_chooser_dialog_new(caption, parent,
				(type ?
				 GTK_FILE_CHOOSER_ACTION_SAVE :
				 GTK_FILE_CHOOSER_ACTION_OPEN),
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				NULL);
  if (dir)
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), dir);
  if (name)
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), name);

  int result = gtk_dialog_run(GTK_DIALOG(dlg));

  if (result == GTK_RESPONSE_ACCEPT) {
    char *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    lua_pushstring(L, fn);
    lua_pushinteger(L, 1); // name filters not used
    g_free(fn);
    gtk_widget_destroy(dlg);
    return 2;
  }
  gtk_widget_destroy(dlg);
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_messageBox(lua_State *L)
{
  static const char * const options[] =  {
    "none", "warning", "information", "question", "critical", nullptr };
  static const char * const buttontype[] = {
    "ok", "okcancel", "yesnocancel", "discardcancel",
    "savediscardcancel", nullptr };

  GtkWidget *parent = check_winid(L, 1);
  int type = luaL_checkoption(L, 2, "none", options);
  const char *text = luaL_checkstring(L, 3);
  const char *details = nullptr;
  if (!lua_isnoneornil(L, 4))
    details = luaL_checkstring(L, 4);
  int buttons = 0;
  if (lua_isnumber(L, 5))
    buttons = (int)luaL_checkinteger(L, 5);
  else if (!lua_isnoneornil(L, 5))
    buttons = luaL_checkoption(L, 5, nullptr, buttontype);

  GtkMessageType t = GTK_MESSAGE_OTHER;
  switch (type) {
  case 0: t = GTK_MESSAGE_OTHER; break;
  case 1: t = GTK_MESSAGE_WARNING; break;
  case 2: t = GTK_MESSAGE_INFO; break;
  case 3: t = GTK_MESSAGE_QUESTION; break;
  case 4: t = GTK_MESSAGE_ERROR; break;
  default: break;
  }

  GtkWidget *dlg =
    gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL,
			   t, GTK_BUTTONS_NONE, "%s", text);
  if (details)
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg),
					     "%s", details);
  switch (buttons) {
  case 0: // "ok"
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   GTK_STOCK_OK, GTK_RESPONSE_OK,
			   NULL);
    break;
  case 1: // "okcancel"
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			   GTK_STOCK_OK, GTK_RESPONSE_OK,
			   NULL);
    break;
  case 2: // "yesnocancel"
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			   GTK_STOCK_NO, GTK_RESPONSE_NO,
			   GTK_STOCK_YES, GTK_RESPONSE_YES,
			   NULL);
    break;
  case 3: // "discardcancel"
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			   GTK_STOCK_DISCARD, GTK_RESPONSE_NO,
			   NULL);
    break;
  case 4: // "savediscardcancel"
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			   GTK_STOCK_DISCARD, GTK_RESPONSE_NO,
			   GTK_STOCK_SAVE, GTK_RESPONSE_YES,
			   NULL);
  default:
    break;
  }

  int result = gtk_dialog_run(GTK_DIALOG(dlg));

  switch (result) {
  case GTK_RESPONSE_YES:
  case GTK_RESPONSE_OK:
    lua_pushnumber(L, 1);
    break;
  case GTK_RESPONSE_NO:
    lua_pushnumber(L, 0);
    break;
  case GTK_RESPONSE_CANCEL:
  default:
    lua_pushnumber(L, -1);
    break;
  }
  gtk_widget_destroy(dlg);
  return 1;
}

// --------------------------------------------------------------------

class PTimer : public Timer {
public:
  PTimer(lua_State *L0, int lua_object, const char *method);
  virtual ~PTimer();

  virtual int setInterval(lua_State *L);
  virtual int active(lua_State *L);
  virtual int start(lua_State *L);
  virtual int stop(lua_State *L);

private:
  gboolean elapsed();
  static gboolean timerCallback(gpointer data);

private:
  guint iTimer;
  guint iInterval;
};

gboolean PTimer::timerCallback(gpointer data)
{
  PTimer *t = (PTimer *) data;
  return t->elapsed();
}

PTimer::PTimer(lua_State *L0, int lua_object, const char *method)
  : Timer(L0, lua_object, method)
{
  iTimer = 0;
  iInterval = 0;
}

PTimer::~PTimer()
{
  if (iTimer != 0)
    g_source_remove(iTimer);
}

gboolean PTimer::elapsed()
{
  callLua();
  if (iSingleShot) {
    iTimer = 0;
    return FALSE;
  } else
    return TRUE;
}

// does not update interval on running timer
int PTimer::setInterval(lua_State *L)
{
  int t = (int)luaL_checkinteger(L, 2);
  iInterval = t;
  return 0;
}

int PTimer::active(lua_State *L)
{
  lua_pushboolean(L, (iTimer != 0));
  return 1;
}

int PTimer::start(lua_State *L)
{
  if (iTimer == 0) {
    if (iInterval > 3000)
      iTimer = g_timeout_add_seconds(iInterval / 1000,
				     GSourceFunc(timerCallback), this);
    else
      iTimer = g_timeout_add(iInterval, GSourceFunc(timerCallback), this);
  }
  return 0;
}

int PTimer::stop(lua_State *L)
{
  if (iTimer != 0) {
    g_source_remove(iTimer);
    iTimer = 0;
  }
  return 0;
}

// --------------------------------------------------------------------

static int timer_constructor(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  const char *method = luaL_checkstring(L, 2);

  Timer **t = (Timer **) lua_newuserdata(L, sizeof(Timer *));
  *t = nullptr;
  luaL_getmetatable(L, "Ipe.timer");
  lua_setmetatable(L, -2);

  // create a table with weak reference to Lua object
  lua_createtable(L, 1, 1);
  lua_pushliteral(L, "v");
  lua_setfield(L, -2, "__mode");
  lua_pushvalue(L, -1);
  lua_setmetatable(L, -2);
  lua_pushvalue(L, 1);
  lua_rawseti(L, -2, 1);
  int lua_object = luaL_ref(L, LUA_REGISTRYINDEX);
  *t = new PTimer(L, lua_object, method);
  return 1;
}

// --------------------------------------------------------------------

static int ipeui_wait(lua_State *L)
{
  luaL_error(L, "'waitDialog' is not yet implemented.");
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State *L)
{
  time_t t = time(NULL);
  struct tm *tmp = localtime(&t);
  if (tmp == NULL)
    return 0;

  char buf[16];
  strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", tmp);
  lua_pushstring(L, buf);
  return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
  { "Dialog", dialog_constructor },
  { "Menu", menu_constructor },
  { "Timer", timer_constructor },
  { "getColor", ipeui_getColor },
  { "fileDialog", ipeui_fileDialog },
  { "messageBox", ipeui_messageBox },
  { "waitDialog", ipeui_wait },
  { "currentDateTime", ipeui_currentDateTime },
  { "downloadFileIfIpeWeb", ipeui_downloadFileIfIpeWeb },
  { nullptr, nullptr },
};

// --------------------------------------------------------------------

int luaopen_ipeui(lua_State *L)
{
  luaL_newlib(L, ipeui_functions);
  lua_setglobal(L, "ipeui");
  luaopen_ipeui_common(L);
  return 0;
}

// --------------------------------------------------------------------
