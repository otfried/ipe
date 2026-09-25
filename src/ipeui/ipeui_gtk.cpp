// --------------------------------------------------------------------
// Lua bindings for GTK dialogs
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

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

// does the same as change_mnemonic in appui_gtk.cpp,
// but let's keep this library self-contained.
std::string gtkMnemonic(const std::string & text) {
    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
	if (text[i] == '&') {
	    if (i + 1 < text.size() && text[i + 1] == '&') {
		result.push_back('&');
		++i;
	    } else {
		result.push_back('_');
	    }
	} else {
	    result.push_back(text[i]);
	}
    }

    return result;
}

// --------------------------------------------------------------------

class PDialog : public Dialog {
public:
    PDialog(lua_State * L0, WINID parent, const char * caption, const char * language);
    virtual ~PDialog();

    virtual void setMapped(lua_State * L, int idx);
    virtual Result buildAndRun(int w, int h);
    virtual void retrieveValues();
    virtual void enableItem(int idx, bool value);
    virtual void acceptDialog(lua_State * L);
    virtual int takeDown(lua_State * L);

private:
    static void itemResponse(GtkWidget * item, PDialog * dlg);
    static void response_cb(GtkDialog * dialog, int response, PDialog * dlg);
    static void button_response_cb(GtkButton * button, PDialog * dlg);
    static gboolean escape_response_cb(GtkEventControllerKey * controller, guint keyval,
				       guint keycode, GdkModifierType state,
				       PDialog * dlg);

private:
    std::vector<GtkWidget *> iWidgets;
    int threadRef;
};

PDialog::PDialog(lua_State * L0, WINID parent, const char * caption,
		 const char * language)
    : Dialog(L0, parent, caption, language) {
    //
}

PDialog::~PDialog() {
    //
}

void PDialog::button_response_cb(GtkButton * button, PDialog * dlg) {
    int response = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "response"));
    gtk_dialog_response(GTK_DIALOG(dlg->hDialog), response);
}

gboolean PDialog::escape_response_cb(GtkEventControllerKey *, guint keyval, guint,
				     GdkModifierType, PDialog * dlg) {
    if (keyval != GDK_KEY_Escape) return FALSE;
    dlg->retrieveValues();
    if (dlg->iIgnoreEscapeField >= 0
	&& dlg->iElements[dlg->iIgnoreEscapeField].text == dlg->iIgnoreEscapeText)
	gtk_dialog_response(GTK_DIALOG(dlg->hDialog), GTK_RESPONSE_DELETE_EVENT);
    return TRUE;
}

static void list_item_setup(GtkListItemFactory *, GtkListItem * item, gpointer) {
    GtkWidget * label = gtk_label_new(nullptr);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_halign(label, GTK_ALIGN_FILL);
    gtk_list_item_set_child(item, label);
}

static void list_item_bind(GtkListItemFactory *, GtkListItem * item, gpointer) {
    GtkStringObject * object = GTK_STRING_OBJECT(gtk_list_item_get_item(item));
    gtk_label_set_text(GTK_LABEL(gtk_list_item_get_child(item)),
		       gtk_string_object_get_string(object));
}

void PDialog::acceptDialog(lua_State * L) {
    int accept = lua_toboolean(L, 2);
    retrieveValues();
    gtk_dialog_response(GTK_DIALOG(hDialog),
			accept ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_REJECT);
}

void PDialog::itemResponse(GtkWidget * item, PDialog * dlg) {
    for (int i = 0; i < int(dlg->iWidgets.size()); ++i) {
	if (dlg->iWidgets[i] == item) {
	    dlg->callLua(dlg->iElements[i].lua_method);
	    return;
	}
    }
}

void PDialog::setMapped(lua_State * L, int idx) {
    SElement & m = iElements[idx];
    GtkWidget * w = iWidgets[idx];
    switch (m.type) {
    case ELabel: gtk_label_set_text(GTK_LABEL(w), m.text.c_str()); break;
    case ECheckBox: gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), m.value); break;
    case ETextEdit:
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)),
				 m.text.c_str(), -1);
	break;
    case EInput: gtk_editable_set_text(GTK_EDITABLE(w), m.text.c_str()); break;
    case EList: {
	GtkSingleSelection * s =
	    GTK_SINGLE_SELECTION(gtk_list_view_get_model(GTK_LIST_VIEW(w)));
	if (lua_istable(L, 3)) {
	    GtkStringList * strings = gtk_string_list_new(nullptr);
	    for (const auto & item : m.items)
		gtk_string_list_append(strings, item.c_str());
	    gtk_single_selection_set_model(s, G_LIST_MODEL(strings));
	}
	gtk_single_selection_set_selected(s, m.value);
    } break;
    case ECombo:
	if (lua_istable(L, 3)) {
	    GtkStringList * strings = gtk_string_list_new(nullptr);
	    for (const auto & item : m.items)
		gtk_string_list_append(strings, item.c_str());
	    gtk_drop_down_set_model(GTK_DROP_DOWN(w), G_LIST_MODEL(strings));
	}
	gtk_drop_down_set_selected(GTK_DROP_DOWN(w), m.value);
	break;
    default: break; // EButton
    }
}

static String getTextEdit(GtkWidget * w) {
    GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
    gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);
    gchar * s = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
    return String(s);
}

void PDialog::retrieveValues() {
    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	GtkWidget * w = iWidgets[i];
	switch (m.type) {
	case EInput: m.text = String(gtk_editable_get_text(GTK_EDITABLE(w))); break;
	case ETextEdit: m.text = getTextEdit(w); break;
	case EList: {
	    GtkSelectionModel * s = gtk_list_view_get_model(GTK_LIST_VIEW(w));
	    for (size_t k = 0; k < m.items.size(); ++k) {
		if (gtk_selection_model_is_selected(s, k)) m.value = k;
	    }
	} break;
	case ECombo: m.value = gtk_drop_down_get_selected(GTK_DROP_DOWN(w)); break;
	case ECheckBox: m.value = gtk_check_button_get_active(GTK_CHECK_BUTTON(w)); break;
	default: break; // label and button - nothing to do
	}
    }
}

void PDialog::enableItem(int idx, bool value) {
    gtk_widget_set_sensitive(iWidgets[idx], value);
}

static GtkWidget * addScrollBar(GtkWidget * w) {
    GtkWidget * ww = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ww), GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(ww), w);
    return ww;
}

Dialog::Result PDialog::buildAndRun(int w, int h) {
    hDialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(hDialog), iCaption.c_str());
    gtk_window_set_modal(GTK_WINDOW(hDialog), TRUE);
    if (iParent) gtk_window_set_transient_for(GTK_WINDOW(hDialog), GTK_WINDOW(iParent));
    if (w > 0 && h > 0) gtk_window_set_default_size(GTK_WINDOW(hDialog), w, h);
    g_signal_connect(hDialog, "response", G_CALLBACK(response_cb), this);
    if (iIgnoreEscapeField >= 0) {
	GtkEventController * key = gtk_event_controller_key_new();
	g_signal_connect(key, "key-pressed", G_CALLBACK(escape_response_cb), this);
	gtk_widget_add_controller(hDialog, key);
    }

    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(hDialog));
    GtkWidget * grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_start(grid, 12);
    gtk_widget_set_margin_end(grid, 12);
    gtk_widget_set_margin_top(grid, 12);
    gtk_widget_set_margin_bottom(grid, 12);
    gtk_box_append(GTK_BOX(content), grid);
    GtkWidget * action_area = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(action_area, GTK_ALIGN_END);
    gtk_widget_set_margin_start(action_area, 12);
    gtk_widget_set_margin_end(action_area, 12);
    gtk_box_append(GTK_BOX(content), action_area);

    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	GtkWidget * widget = nullptr;
	GtkWidget * placed = nullptr;
	bool hexpand = false;
	bool vexpand = false;
	if (m.row < 0) {
	    widget = gtk_button_new_with_mnemonic(gtkMnemonic(m.text).c_str());
	    gtk_box_append(GTK_BOX(action_area), widget);
	    if (m.flags & EAccept) {
		g_object_set_data(G_OBJECT(widget), "response",
				  GINT_TO_POINTER(GTK_RESPONSE_ACCEPT));
		g_signal_connect(widget, "clicked", G_CALLBACK(button_response_cb), this);
	    } else if (m.flags & EReject) {
		g_object_set_data(G_OBJECT(widget), "response",
				  GINT_TO_POINTER(GTK_RESPONSE_REJECT));
		g_signal_connect(widget, "clicked", G_CALLBACK(button_response_cb), this);
	    } else if (m.lua_method) {
		g_signal_connect(widget, "clicked", G_CALLBACK(itemResponse), this);
	    }
	} else {
	    switch (m.type) {
	    case ELabel:
		widget = gtk_label_new(m.text.c_str());
		gtk_label_set_xalign(GTK_LABEL(widget), 0.0);
		break;
	    case EButton:
		widget = gtk_button_new_with_mnemonic(gtkMnemonic(m.text).c_str());
		if (m.lua_method)
		    g_signal_connect(widget, "clicked", G_CALLBACK(itemResponse), this);
		break;
	    case ECheckBox:
		widget = gtk_check_button_new_with_mnemonic(gtkMnemonic(m.text).c_str());
		gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), m.value);
		if (m.lua_method)
		    g_signal_connect(widget, "toggled", G_CALLBACK(itemResponse), this);
		break;
	    case EInput:
		widget = gtk_entry_new();
		gtk_widget_set_hexpand(widget, TRUE);
		hexpand = true;
		break;
	    case ETextEdit:
		widget = gtk_text_view_new();
		gtk_text_view_set_editable(GTK_TEXT_VIEW(widget), !(m.flags & EReadOnly));
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)),
					 m.text.c_str(), -1);
		placed = addScrollBar(widget);
		hexpand = vexpand = true;
		break;
	    case ECombo: {
		GtkStringList * strings = gtk_string_list_new(nullptr);
		for (const auto & item : m.items)
		    gtk_string_list_append(strings, item.c_str());
		widget = gtk_drop_down_new(G_LIST_MODEL(strings), nullptr);
		gtk_drop_down_set_selected(GTK_DROP_DOWN(widget), m.value);
		gtk_widget_set_valign(widget, GTK_ALIGN_START);
		hexpand = true;
	    } break;
	    case EList: {
		GtkStringList * strings = gtk_string_list_new(nullptr);
		for (const auto & item : m.items)
		    gtk_string_list_append(strings, item.c_str());
		GtkSingleSelection * selection =
		    gtk_single_selection_new(G_LIST_MODEL(strings));
		gtk_single_selection_set_selected(selection, m.value);
		GtkListItemFactory * factory = gtk_signal_list_item_factory_new();
		g_signal_connect(factory, "setup", G_CALLBACK(list_item_setup), nullptr);
		g_signal_connect(factory, "bind", G_CALLBACK(list_item_bind), nullptr);
		widget = gtk_list_view_new(GTK_SELECTION_MODEL(selection),
					   GTK_LIST_ITEM_FACTORY(factory));
		placed = addScrollBar(widget);
		hexpand = vexpand = true;
	    } break;
	    default: break;
	    }
	}
	if (m.row >= 0) {
	    if (!placed) placed = widget;
	    gtk_widget_set_hexpand(placed, hexpand);
	    gtk_widget_set_vexpand(placed, vexpand);
	    gtk_grid_attach(GTK_GRID(grid), placed, m.col, m.row, m.colspan, m.rowspan);
	}
	if (widget && (m.flags & EDisabled)) gtk_widget_set_sensitive(widget, FALSE);
	iWidgets.push_back(widget);
	fprintf(stderr, "%d: %ld of type %d\n", i, iWidgets.size(), m.type);
    }
    // save current thread
    lua_pushthread(L);
    threadRef = luaL_ref(L, LUA_REGISTRYINDEX);

    gtk_window_present(GTK_WINDOW(hDialog));
    return Result::MODAL;
}

void PDialog::response_cb(GtkDialog * dialog, int response, PDialog * dlg) {
    lua_pushnumber(dlg->L, response);
    int nresults = 1;
    lua_resume(dlg->L, nullptr, nresults, &nresults);
    luaL_unref(dlg->L, LUA_REGISTRYINDEX, dlg->threadRef);
}

int PDialog::takeDown(lua_State * L) {
    int result = luaL_checkinteger(L, 2);
    release(L); // release references to Lua objects

    retrieveValues();

    fprintf(stderr, "destroying window\n");
    gtk_window_close(GTK_WINDOW(hDialog));
    hDialog = nullptr;

    lua_pushboolean(L, result == 1);
    return 1;
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State * L) {
    WINID parent = check_winid(L, 1);
    const char * s = luaL_checkstring(L, 2);
    const char * language = "";
    if (lua_isstring(L, 3)) language = luaL_checkstring(L, 3);

    Dialog ** dlg = (Dialog **)lua_newuserdata(L, sizeof(Dialog *));
    *dlg = nullptr;
    luaL_getmetatable(L, "Ipe.dialog");
    lua_setmetatable(L, -2);
    *dlg = new PDialog(L, parent, s, language);
    return 1;
}

// --------------------------------------------------------------------

class PMenu : public Menu {
public:
    PMenu(WINID parent)
	: iParent(parent)
	, iMenu(g_menu_new())
	, iActions(g_simple_action_group_new()) {}
    ~PMenu() override {
	g_object_unref(iMenu);
	g_object_unref(iActions);
    }
    int add(lua_State * L) override;
    int execute(lua_State * L) override;

private:
    static void activate_cb(GSimpleAction * action, GVariant *, PMenu * menu);
    WINID iParent;
    GMenu * iMenu;
    GSimpleActionGroup * iActions;
    struct Item {
	std::string name;
	std::string itemName;
    };
    std::vector<Item> iItems;
    int iNextAction = 0;
    int iSelected = -1;
    GMainLoop * iLoop = nullptr;
};

void PMenu::activate_cb(GSimpleAction * action, GVariant *, PMenu * menu) {
    menu->iSelected = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "index"));
    if (menu->iLoop) g_main_loop_quit(menu->iLoop);
}

int PMenu::add(lua_State * L) {
    const char * name = luaL_checkstring(L, 2);
    const char * title = luaL_checkstring(L, 3);
    auto addItem = [&](GMenu * menu, const char * label, const char * itemName,
		       bool checkable, bool active) {
	char action_name[32];
	sprintf(action_name, "item%d", iNextAction++);
	GSimpleAction * action =
	    checkable ? g_simple_action_new_stateful(action_name, nullptr,
						     g_variant_new_boolean(active))
		      : g_simple_action_new(action_name, nullptr);
	int index = iItems.size();
	g_object_set_data(G_OBJECT(action), "index", GINT_TO_POINTER(index));
	g_signal_connect(action, "activate", G_CALLBACK(activate_cb), this);
	g_action_map_add_action(G_ACTION_MAP(iActions), G_ACTION(action));
	iItems.push_back({name, itemName ? itemName : ""});
	char detailed[40];
	sprintf(detailed, "menu.%s", action_name);
	GMenuItem * menuItem = g_menu_item_new(label, detailed);
	g_menu_append_item(menu, menuItem);
	g_object_unref(menuItem);
    };

    if (lua_gettop(L) == 3) {
	addItem(iMenu, title, nullptr, false, false);
	return 0;
    }

    luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
    bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
    bool hastable = !hasmap && !lua_isnoneornil(L, 5);
    bool hascolor = !lua_isnoneornil(L, 6) && lua_isfunction(L, 6);
    bool hascheck = !hascolor && !lua_isnoneornil(L, 6);
    if (hastable)
	luaL_argcheck(L, lua_istable(L, 5), 5, "argument is not a function or table");
    const char * current = nullptr;
    if (hascheck) {
	luaL_argcheck(L, lua_isstring(L, 6), 6, "argument is not a function or string");
	current = luaL_checkstring(L, 6);
    }

    GMenu * submenu = g_menu_new();
    int count = lua_rawlen(L, 4);
    for (int i = 1; i <= count; ++i) {
	lua_rawgeti(L, 4, i);
	luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
	const char * itemName = lua_tostring(L, -1);
	if (hastable) {
	    lua_rawgeti(L, 5, i);
	    luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
	} else if (hasmap) {
	    lua_pushvalue(L, 5);
	    lua_pushinteger(L, i);
	    lua_pushvalue(L, -3);
	    lua_call(L, 2, 1);
	    luaL_argcheck(L, lua_isstring(L, -1), 5, "function does not return string");
	} else {
	    lua_pushvalue(L, -1);
	}
	const char * label = lua_tostring(L, -1);
	// Color callbacks are intentionally ignored for the GTK-4 menu.
	addItem(submenu, label, itemName, hascheck,
		hascheck && !g_strcmp0(itemName, current));
	lua_pop(L, 2);
    }
    GMenuItem * parentItem = g_menu_item_new_submenu(title, G_MENU_MODEL(submenu));
    g_menu_append_item(iMenu, parentItem);
    g_object_unref(parentItem);
    g_object_unref(submenu);
    return 0;
}

int PMenu::execute(lua_State * L) {
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    GtkWidget * popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(iMenu));
    gtk_widget_insert_action_group(popover, "menu", G_ACTION_GROUP(iActions));
    gtk_widget_set_parent(popover, iParent);
    GdkRectangle rect = {x, y, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
    iSelected = -1;
    iLoop = g_main_loop_new(nullptr, FALSE);
    gtk_popover_popup(GTK_POPOVER(popover));
    g_main_loop_run(iLoop);
    g_main_loop_unref(iLoop);
    iLoop = nullptr;
    gtk_widget_unparent(popover);
    if (iSelected < 0) return 0;
    lua_pushstring(L, iItems[iSelected].name.c_str());
    lua_pushstring(L, iItems[iSelected].itemName.c_str());
    return 2;
}

// GtkImageMenuItem is gone in GTK3; build a plain menu item whose child is a
// small box with a color swatch and a label instead.
#if GTK_MAJOR_VERSION < 4
static GtkWidget * colorMenuItem(double r, double g, double b, const char * text) {
    GdkPixbuf * pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 16, 16);
    guint32 pixel = (guint32(r * 255) << 24) | (guint32(g * 255) << 16)
		    | (guint32(b * 255) << 8) | 0xff;
    gdk_pixbuf_fill(pixbuf, pixel);
    GtkWidget * image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    GtkWidget * label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

    GtkWidget * item = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(item), hbox);
    gtk_widget_show_all(hbox);
    return item;
}

int PMenu::add(lua_State * L) {
    const char * name = luaL_checkstring(L, 2);
    const char * title = luaL_checkstring(L, 3);
    if (lua_gettop(L) == 3) {
	GtkWidget * w = gtk_menu_item_new_with_label(title);
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
	    luaL_argcheck(L, lua_istable(L, 5), 5, "argument is not a function or table");
	const char * current = nullptr;
	if (hascheck) {
	    luaL_argcheck(L, lua_isstring(L, 6), 6,
			  "argument is not a function or string");
	    current = luaL_checkstring(L, 6);

	    GtkWidget * sm = gtk_menu_new();

	    int no = lua_rawlen(L, 4);
	    for (int i = 1; i <= no; ++i) {
		lua_rawgeti(L, 4, i);
		luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
		const char * item = lua_tostring(L, -1);
		if (hastable) {
		    lua_rawgeti(L, 5, i);
		    luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
		} else if (hasmap) {
		    lua_pushvalue(L, 5);  // function
		    lua_pushnumber(L, i); // index
		    lua_pushvalue(L, -3); // name
		    lua_call(L, 2, 1);    // function returns label
		    luaL_argcheck(L, lua_isstring(L, -1), 5,
				  "function does not return string");
		} else
		    lua_pushvalue(L, -1);

		const char * text = lua_tostring(L, -1);

		GtkWidget * w;
		if (hascolor) {
		    lua_pushvalue(L, 6);  // function
		    lua_pushnumber(L, i); // index
		    lua_pushvalue(L, -3); // name
		    lua_call(L, 2, 3);    // function returns red, green, blue
		    double red = luaL_checknumber(L, -3);
		    double green = luaL_checknumber(L, -2);
		    double blue = luaL_checknumber(L, -1);
		    lua_pop(L, 3); // pop result
		    w = colorMenuItem(red, green, blue, text);
		} else if (hascheck) {
		    w = gtk_check_menu_item_new_with_label(text);
		    if (!g_strcmp0(item, current))
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), true);
		} else {
		    w = gtk_menu_item_new_with_label(text);
		}
		gtk_menu_shell_append(GTK_MENU_SHELL(sm), w);
		g_signal_connect(w, "activate", G_CALLBACK(itemResponse), this);
		gtk_widget_show(w);
		Item mitem;
		mitem.name = g_strdup(name);
		mitem.itemName = g_strdup(item);
		mitem.itemIndex = i;
		mitem.widget = w;
		items.push_back(mitem);

		lua_pop(L, 2); // item, text
	    }
	    GtkWidget * sme = gtk_menu_item_new_with_label(title);
	    gtk_widget_show(sme);
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sme), sm);
	    gtk_menu_shell_append(GTK_MENU_SHELL(iMenu), sme);
	    gtk_widget_show(sme);
	}
	return 0;
    }
#endif

    // --------------------------------------------------------------------

    static int menu_constructor(lua_State * L) {
	GtkWidget * parent = check_winid(L, 1);
	Menu ** m = (Menu **)lua_newuserdata(L, sizeof(Menu *));
	*m = nullptr;
	luaL_getmetatable(L, "Ipe.menu");
	lua_setmetatable(L, -2);
	*m = new PMenu(parent);
	return 1;
    }

    // --------------------------------------------------------------------

    struct LuaAsyncContext {
	lua_State * lua;
	int threadRef;
    };

    struct FileDialogContext : LuaAsyncContext {
	bool save;
    };

    static void ipeui_fileDialog_response(GObject * source, GAsyncResult * result,
					  gpointer data) {
	auto * context = static_cast<FileDialogContext *>(data);
	GError * error = nullptr;
	GFile * file =
	    context->save
		? gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &error)
		: gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
	if (file) {
	    char * path = g_file_get_path(file);
	    lua_pushstring(context->lua, path);
	    g_free(path);
	    g_object_unref(file);
	}
	if (error) g_error_free(error);
	int nresults = file ? 1 : 0;
	lua_resume(context->lua, nullptr, nresults, &nresults);
	luaL_unref(context->lua, LUA_REGISTRYINDEX, context->threadRef);
	g_object_unref(source);
	delete context;
    }

    static void ipeui_getColor_response(GObject * source, GAsyncResult * result,
					gpointer data) {
	auto * context = static_cast<LuaAsyncContext *>(data);
	GError * error = nullptr;
	GdkRGBA * color =
	    gtk_color_dialog_choose_rgba_finish(GTK_COLOR_DIALOG(source), result, &error);
	int nresults = 0;
	if (color) {
	    lua_pushnumber(context->lua, color->red);
	    lua_pushnumber(context->lua, color->green);
	    lua_pushnumber(context->lua, color->blue);
	    nresults = 3;
	    g_free(color);
	}
	if (error) g_error_free(error);
	lua_resume(context->lua, nullptr, nresults, &nresults);
	luaL_unref(context->lua, LUA_REGISTRYINDEX, context->threadRef);
	g_object_unref(source);
	delete context;
    }

    static int ipeui_getColorAsync(lua_State * L) {
	GtkWindow * parent = GTK_WINDOW(check_winid(L, 1));
	const char * title = luaL_checkstring(L, 2);
	GdkRGBA color = {float(luaL_checknumber(L, 3)), float(luaL_checknumber(L, 4)),
			 float(luaL_checknumber(L, 5)), 1.0f};
	GtkColorDialog * dialog = gtk_color_dialog_new();
	gtk_color_dialog_set_title(dialog, title);
	gtk_color_dialog_set_with_alpha(dialog, FALSE);
	lua_pushthread(L);
	auto * context = new LuaAsyncContext{L, luaL_ref(L, LUA_REGISTRYINDEX)};
	gtk_color_dialog_choose_rgba(dialog, parent, &color, nullptr,
				     ipeui_getColor_response, context);
	return 0;
    }

    static int ipeui_fileDialogAsync(lua_State * L) {
	GtkWindow * parent = GTK_WINDOW(check_winid(L, 1));
	static const char * const typenames[] = {"open", "save", nullptr};
	int type = luaL_checkoption(L, 2, nullptr, typenames);
	const char * caption = luaL_checkstring(L, 3);
	if (!lua_isnoneornil(L, 4)) luaL_checktype(L, 4, LUA_TTABLE);
	const char * dir = lua_isnoneornil(L, 5) ? nullptr : luaL_checkstring(L, 5);
	const char * name = lua_isnoneornil(L, 6) ? nullptr : luaL_checkstring(L, 6);

	GtkFileDialog * dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, caption);
	if (dir) {
	    GFile * folder = g_file_new_for_path(dir);
	    gtk_file_dialog_set_initial_folder(dialog, folder);
	    g_object_unref(folder);
	}
	if (name) gtk_file_dialog_set_initial_name(dialog, name);
	lua_pushthread(L);
	auto * context =
	    new FileDialogContext{{L, luaL_ref(L, LUA_REGISTRYINDEX)}, type != 0};
	if (type == 0)
	    gtk_file_dialog_open(dialog, parent, nullptr, ipeui_fileDialog_response,
				 context);
	else
	    gtk_file_dialog_save(dialog, parent, nullptr, ipeui_fileDialog_response,
				 context);
	return 0;
    }

    struct AlertContext {
	lua_State * lua;
	int threadRef;
	int buttons;
    };

    static void alert_response(GObject * source, GAsyncResult * result, gpointer data) {
	auto * context = static_cast<AlertContext *>(data);
	GtkAlertDialog * dialog = GTK_ALERT_DIALOG(source);
	GError * error = nullptr;
	int response = gtk_alert_dialog_choose_finish(dialog, result, &error);
	int buttons = context->buttons;
	int value = -1;
	if (!error) {
	    int positive = (buttons == 0) ? 0 : ((buttons == 2 || buttons == 4) ? 2 : 1);
	    if (response == positive)
		value = 1;
	    else if (response > 0)
		value = 0;
	}
	(void)value;
	if (error) g_error_free(error);
	g_object_unref(dialog);

	lua_pushinteger(context->lua, value);
	int nresults = 0;
	lua_resume(context->lua, nullptr, 1, &nresults);
	luaL_unref(context->lua, LUA_REGISTRYINDEX, context->threadRef);
	delete context;
    }

    static int ipeui_messageBoxAsync(lua_State * L) {
	GtkWindow * parent = GTK_WINDOW(check_winid(L, 1));
	static const char * const options[] = {"none",     "warning",  "information",
					       "question", "critical", nullptr};
	luaL_checkoption(L, 2, "none", options); // not used in GTK
	const char * text = luaL_checkstring(L, 3);
	const char * details = nullptr;
	if (!lua_isnoneornil(L, 4)) details = luaL_checkstring(L, 4);
	int buttons = 0;
	if (lua_isnumber(L, 5))
	    buttons = (int)luaL_checkinteger(L, 5);
	else if (!lua_isnoneornil(L, 5)) {
	    static const char * const buttontype[] = {
		"ok",   "okcancel", "yesnocancel", "discardcancel", "savediscardcancel",
		nullptr};
	    buttons = luaL_checkoption(L, 5, nullptr, buttontype);
	}
	luaL_argcheck(L, 0 <= buttons && buttons <= 4, 5, "invalid button type");

	static const char * const ok[] = {"_OK", nullptr};
	static const char * const okcancel[] = {"_Cancel", "_OK", nullptr};
	static const char * const yesnocancel[] = {"_Cancel", "_No", "_Yes", nullptr};
	static const char * const discardcancel[] = {"_Cancel", "_Discard", nullptr};
	static const char * const savediscardcancel[] = {"_Cancel", "_Discard", "_Save",
							 nullptr};
	static const char * const * const buttonsets[] = {
	    ok, okcancel, yesnocancel, discardcancel, savediscardcancel};

	GtkAlertDialog * dialog = gtk_alert_dialog_new("%s", text);
	gtk_alert_dialog_set_modal(dialog, TRUE);
	gtk_alert_dialog_set_buttons(dialog, buttonsets[buttons]);
	gtk_alert_dialog_set_cancel_button(dialog, 0);
	gtk_alert_dialog_set_default_button(dialog, buttons == 0                     ? 0
						    : (buttons == 2 || buttons == 4) ? 2
										     : 1);
	if (details) gtk_alert_dialog_set_detail(dialog, details);

	lua_pushthread(L);
	auto * context = new AlertContext{L, luaL_ref(L, LUA_REGISTRYINDEX), buttons};

	gtk_alert_dialog_choose(dialog, parent, nullptr, alert_response, context);
	return 0;
    }

    // --------------------------------------------------------------------

    class PTimer : public Timer {
    public:
	PTimer(lua_State * L0, int lua_object, const char * method);
	virtual ~PTimer();

	virtual int setInterval(lua_State * L);
	virtual int active(lua_State * L);
	virtual int start(lua_State * L);
	virtual int stop(lua_State * L);

    private:
	gboolean elapsed();
	static gboolean timerCallback(gpointer data);

    private:
	guint iTimer;
	guint iInterval;
    };

    gboolean PTimer::timerCallback(gpointer data) {
	PTimer * t = (PTimer *)data;
	return t->elapsed();
    }

    PTimer::PTimer(lua_State * L0, int lua_object, const char * method)
	: Timer(L0, lua_object, method) {
	iTimer = 0;
	iInterval = 0;
    }

    PTimer::~PTimer() {
	if (iTimer != 0) g_source_remove(iTimer);
    }

    gboolean PTimer::elapsed() {
	callLua();
	if (iSingleShot) {
	    iTimer = 0;
	    return FALSE;
	} else
	    return TRUE;
    }

    // does not update interval on running timer
    int PTimer::setInterval(lua_State * L) {
	int t = (int)luaL_checkinteger(L, 2);
	iInterval = t;
	return 0;
    }

    int PTimer::active(lua_State * L) {
	lua_pushboolean(L, (iTimer != 0));
	return 1;
    }

    int PTimer::start(lua_State * L) {
	if (iTimer == 0) {
	    if (iInterval > 3000)
		iTimer = g_timeout_add_seconds(iInterval / 1000,
					       GSourceFunc(timerCallback), this);
	    else
		iTimer = g_timeout_add(iInterval, GSourceFunc(timerCallback), this);
	}
	return 0;
    }

    int PTimer::stop(lua_State * L) {
	if (iTimer != 0) {
	    g_source_remove(iTimer);
	    iTimer = 0;
	}
	return 0;
    }

    // --------------------------------------------------------------------

    static int timer_constructor(lua_State * L) {
	luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
	const char * method = luaL_checkstring(L, 2);

	Timer ** t = (Timer **)lua_newuserdata(L, sizeof(Timer *));
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

    static int ipeui_currentDateTime(lua_State * L) {
	time_t t = time(NULL);
	struct tm * tmp = localtime(&t);
	if (tmp == NULL) return 0;

	char buf[16];
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", tmp);
	lua_pushstring(L, buf);
	return 1;
    }

    // --------------------------------------------------------------------

    static const struct luaL_Reg ipeui_functions[] = {
	{"Dialog", dialog_constructor},
	{"Menu", menu_constructor},
	{"Timer", timer_constructor},
	{"getColorAsync", ipeui_getColorAsync},
	{"fileDialogAsync", ipeui_fileDialogAsync},
	{"messageBoxAsync", ipeui_messageBoxAsync},
	{"currentDateTime", ipeui_currentDateTime},
	{nullptr, nullptr},
    };

    // --------------------------------------------------------------------

    void addMethod(lua_State * L, const char * name, const char * luacode) {
	int ok = luaL_loadstring(L, luacode);
	if (ok != LUA_OK) luaL_error(L, "cannot prepare function");
	lua_call(L, 0, 1);
	lua_setfield(L, -2, name);
    }

    int luaopen_ipeui(lua_State * L) {
	luaL_newlib(L, ipeui_functions);
	addMethod(L, "messageBox",
		  "return function (...) ipeui.messageBoxAsync(...)"
		  "return coroutine.yield() end");
	addMethod(L, "fileDialog",
		  "return function (...) ipeui.fileDialogAsync(...)"
		  "return coroutine.yield(), 1 end");
	addMethod(L, "getColor",
		  "return function (...) ipeui.getColorAsync(...)"
		  "return coroutine.yield() end");
	lua_setglobal(L, "ipeui");
	luaopen_ipeui_common(L);
	return 0;
    }

    // --------------------------------------------------------------------
