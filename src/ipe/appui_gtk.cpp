// --------------------------------------------------------------------
// AppUi  for GTK
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

#include "appui_gtk.h"
#include "controls_gtk.h"
#include "ipecanvas_gtk.h"

#include "ipelua.h"

#include "ipeattributes.h"
#include "ipethumbs.h"

// for version info only
#include "ipefonts.h"
#include <zlib.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------
// dynamic (lazily populated) submenus
enum {
    EDynSelectLayer,
    EDynMoveLayer,
    EDynTextStyle,
    EDynLabelStyle,
    EDynGridSize,
    EDynAngleSize
};

// --------------------------------------------------------------------

static String change_mnemonic(const char * s) {
    int n = strlen(s);
    String r;
    int i = 0;
    while (i < n) {
	if (s[i] == '&') {
	    if (i + 1 < n && s[i + 1] == '&') {
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

// Parse a Qt-style shortcut string ("Ctrl+Shift+X") into a GDK keyval + mods.
static bool parse_accelerator(String s, guint & keyval, GdkModifierType & mods) {
    mods = (GdkModifierType)0;
    keyval = 0;
    String rest = s;
    for (;;) {
	int i = rest.find('+');
	if (i < 0) break;
	String tok = rest.left(i);
	rest = rest.substr(i + 1);
	if (tok == "Ctrl" || tok == "Control")
	    mods = (GdkModifierType)(mods | GDK_CONTROL_MASK);
	else if (tok == "Shift")
	    mods = (GdkModifierType)(mods | GDK_SHIFT_MASK);
	else if (tok == "Alt")
	    mods = (GdkModifierType)(mods | GDK_MOD1_MASK);
	else if (tok == "Meta" || tok == "Command")
	    mods = (GdkModifierType)(mods | GDK_META_MASK);
    }
    if (rest.empty()) return false;
    keyval = gdk_keyval_from_name(rest.z());
    if (keyval == 0 && rest.size() == 1) keyval = gdk_unicode_to_keyval((guchar)rest[0]);
    return keyval != 0;
}

// --------------------------------------------------------------------

GdkPixbuf * AppUi::prefsPixbuf(String name, int size) {
    char keybuf[64];
    sprintf(keybuf, "%d:", size);
    String key = String(keybuf) + name;
    auto it = iIconCache.find(key);
    if (it != iIconCache.end()) return it->second;

    GdkPixbuf * pixbuf = nullptr;
    if (name == "ipe") {
	String fname = Platform::folder(FolderIcons, "icon_128x128.png");
	if (Platform::fileExists(fname))
	    pixbuf =
		gdk_pixbuf_new_from_file_at_scale(fname.z(), size, size, TRUE, nullptr);
    }
    if (!pixbuf) {
	int pno = ipeIcon(name);
	if (pno >= 0) {
	    GdkRGBA fg;
	    gtk_style_context_get_color(gtk_widget_get_style_context(iWindow),
					GTK_STATE_FLAG_NORMAL, &fg);
	    bool dark = (0.299 * fg.red + 0.587 * fg.green + 0.114 * fg.blue) > 0.5;
	    if (!ipeIcons) {
		// force AppUiBase::ipeIcon() to load the icon document
	    }
	    Document * doc = dark ? ipeIconsDark.get() : ipeIcons.get();
	    if (doc) {
		Thumbnail thumbs(doc, size);
		thumbs.setNoCrop(true);
		Buffer b = thumbs.render(doc->page(pno), 0);
		pixbuf = pixbufFromArgb32((const uint8_t *)b.data(), size,
					  thumbs.height(), size * 4);
	    }
	}
    }
    if (pixbuf) iIconCache[key] = pixbuf;
    return pixbuf;
}

void AppUi::setButtonIcon(GtkWidget * button, String name, int size) {
    GdkPixbuf * pixbuf = prefsPixbuf(name, size);
    if (!pixbuf) return;
    GtkWidget * image = gtk_image_new_from_pixbuf(pixbuf);
    gtk_widget_show(image);
    if (GTK_IS_TOOL_BUTTON(button))
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(button), image);
    else
	gtk_button_set_image(GTK_BUTTON(button), image);
}

void AppUi::setButtonColorIcon(GtkWidget * button, Color color, int size) {
    GdkPixbuf * pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, size, size);
    guint32 pixel = (guint32(color.iRed.toDouble() * 255) << 24)
		    | (guint32(color.iGreen.toDouble() * 255) << 16)
		    | (guint32(color.iBlue.toDouble() * 255) << 8) | 0xff;
    gdk_pixbuf_fill(pixbuf, pixel);
    GtkWidget * image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_widget_show(image);
    gtk_button_set_image(GTK_BUTTON(button), image);
}

void AppUi::setActionAccelerator(GtkWidget * item, const char * name) {
    lua_getglobal(L, "shortcuts");
    lua_getfield(L, -1, name);
    if (lua_isstring(L, -1)) {
	String s = lua_tolstring(L, -1, nullptr);
	guint keyval;
	GdkModifierType mods;
	if (parse_accelerator(s, keyval, mods))
	    gtk_widget_add_accelerator(item, "activate", iAccelGroup, keyval, mods,
				       GTK_ACCEL_VISIBLE);
    }
    lua_pop(L, 2); // shortcuts, name
}

String AppUi::menuLabel(int idx) const {
    const char * s = gtk_menu_item_get_label(GTK_MENU_ITEM(iActions[idx].menuItem));
    return s ? String(s) : iActions[idx].name;
}

// --------------------------------------------------------------------

void AppUi::addRootMenu(int id, const char * name) {
    iRootMenu[id] = gtk_menu_item_new_with_mnemonic(change_mnemonic(name).z());
    iSubMenu[id] = gtk_menu_new();
}

void AppUi::syncAndTrigger(int idx, bool active, GtkWidget * source) {
    SAction & a = iActions[idx];
    if (a.menuItem && a.menuItem != source && GTK_IS_CHECK_MENU_ITEM(a.menuItem)) {
	g_signal_handlers_block_by_func(a.menuItem, (gpointer)menuitem_cb,
					GINT_TO_POINTER(idx));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(a.menuItem), active);
	g_signal_handlers_unblock_by_func(a.menuItem, (gpointer)menuitem_cb,
					  GINT_TO_POINTER(idx));
    }
    if (a.toolItem && a.toolItem != source && GTK_IS_TOGGLE_TOOL_BUTTON(a.toolItem)) {
	g_signal_handlers_block_by_func(a.toolItem, (gpointer)toolitem_cb,
					GINT_TO_POINTER(idx));
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(a.toolItem), active);
	g_signal_handlers_unblock_by_func(a.toolItem, (gpointer)toolitem_cb,
					  GINT_TO_POINTER(idx));
    }
}

void AppUi::menuitem_cb(GtkWidget * item, gpointer data) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(item), "ipe-appui");
    gint idx = GPOINTER_TO_INT(data);
    bool active = GTK_IS_CHECK_MENU_ITEM(item)
		  && gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
    ui->syncAndTrigger(idx, active, item);
    ui->action(ui->iActions[idx].name);
}

void AppUi::toolitem_cb(GtkWidget * item, gpointer data) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(item), "ipe-appui");
    gint idx = GPOINTER_TO_INT(data);
    bool active = GTK_IS_TOGGLE_TOOL_BUTTON(item)
		  && gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(item));
    ui->syncAndTrigger(idx, active, item);
    ui->action(ui->iActions[idx].name);
}

int AppUi::addItem(GtkMenuShell * shell, const char * title, const char * name) {
    if (!title) {
	GtkWidget * item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(shell, item);
	gtk_widget_show(item);
	return -1;
    }
    if (name[0] == '@') name = name + 1; // "can be used while drawing" -- ignored here
    bool checkable =
	(shell == GTK_MENU_SHELL(iSubMenu[EModeMenu]) || String(name).find('|') >= 0);
    if (name[0] == '*') {
	checkable = true;
	name = name + 1;
    }

    GtkWidget * item =
	checkable ? gtk_check_menu_item_new_with_mnemonic(change_mnemonic(title).z())
		  : gtk_menu_item_new_with_mnemonic(change_mnemonic(title).z());
    SAction s;
    s.name = String(name);
    s.menuItem = item;
    iActions.push_back(s);
    int idx = int(iActions.size()) - 1;
    g_object_set_data(G_OBJECT(item), "ipe-appui", this);
    gtk_menu_shell_append(shell, item);
    g_signal_connect(item, "activate", G_CALLBACK(menuitem_cb), GINT_TO_POINTER(idx));
    setActionAccelerator(item, name);
    gtk_widget_show(item);
    return idx;
}

void AppUi::addItem(int id, const char * title, const char * name) {
    GtkMenuShell * shell = GTK_MENU_SHELL(iSubMenu[id]);
    int idx = addItem(shell, title, name);
    if (id == EModeMenu && idx >= 0)
	addToolButton(iObjectTools, iActions[idx].name.z(), title);
}

static GtkWidget * submenu = nullptr;
static GtkWidget * submenuitem = nullptr;
static int submenuId = 0;

void AppUi::startSubMenu(int id, const char * name, int tag) {
    submenuId = id;
    submenu = gtk_menu_new();
    submenuitem = gtk_menu_item_new_with_mnemonic(change_mnemonic(name).z());
    g_object_set_data(G_OBJECT(submenu), "ipe-appui", this);
    if (tag != 0) {
	int kind = -1;
	switch (tag) {
	case ESubmenuGridSize: kind = EDynGridSize; break;
	case ESubmenuAngleSize: kind = EDynAngleSize; break;
	case ESubmenuTextStyle: kind = EDynTextStyle; break;
	case ESubmenuLabelStyle: kind = EDynLabelStyle; break;
	case ESubmenuSelectLayer: kind = EDynSelectLayer; break;
	case ESubmenuMoveLayer: kind = EDynMoveLayer; break;
	default: break;
	}
	if (kind >= 0)
	    g_signal_connect(submenu, "show", G_CALLBACK(populate_menu_cb),
			     GINT_TO_POINTER(kind));
    }
}

void AppUi::addSubItem(const char * title, const char * name) {
    GtkMenuShell * shell = GTK_MENU_SHELL(submenu);
    addItem(shell, title, name);
}

MENUHANDLE AppUi::endSubMenu() {
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenuitem), submenu);
    gtk_widget_show(submenuitem);
    GtkMenuShell * menu = GTK_MENU_SHELL(iSubMenu[submenuId]);
    gtk_menu_shell_append(menu, submenuitem);
    return GTK_MENU(submenu);
}

void AppUi::populate_menu_cb(GtkWidget * menu, gpointer data) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(menu), "ipe-appui");
    ui->populateDynamicMenu(menu, GPOINTER_TO_INT(data));
}

void AppUi::dynamic_menu_item_cb(GtkWidget * item, gpointer data) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(item), "ipe-appui");
    int kind = GPOINTER_TO_INT(data);
    const char * value = (const char *)g_object_get_data(G_OBJECT(item), "ipe-value");
    switch (kind) {
    case EDynSelectLayer: ui->action(String("selectinlayer-") + value); break;
    case EDynMoveLayer: ui->action(String("movetolayer-") + value); break;
    case EDynTextStyle: ui->action(String("textstyle|") + value); break;
    case EDynLabelStyle: ui->action(String("labelstyle|") + value); break;
    case EDynGridSize: ui->action(String("gridsize|") + value); break;
    case EDynAngleSize: ui->action(String("anglesize|") + value); break;
    default: break;
    }
}

static void clear_menu(GtkWidget * menu) {
    gtk_container_foreach(
	GTK_CONTAINER(menu), [](GtkWidget * w, gpointer) { gtk_widget_destroy(w); },
	nullptr);
}

void AppUi::populateDynamicMenu(GtkWidget * menu, int kind) {
    clear_menu(menu);
    auto addEntry = [&](const String & text, bool current) {
	GtkWidget * item = current ? gtk_check_menu_item_new_with_label(text.z())
				   : gtk_menu_item_new_with_label(text.z());
	if (current && GTK_IS_CHECK_MENU_ITEM(item))
	    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
	g_object_set_data(G_OBJECT(item), "ipe-appui", this);
	g_object_set_data_full(G_OBJECT(item), "ipe-value", g_strdup(text.z()), g_free);
	g_signal_connect(item, "activate", G_CALLBACK(dynamic_menu_item_cb),
			 GINT_TO_POINTER(kind));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
    };

    switch (kind) {
    case EDynSelectLayer:
    case EDynMoveLayer:
	for (auto & name : iLayerList.layers()) addEntry(name, false);
	break;
    case EDynTextStyle:
	if (iCascade) {
	    AttributeSeq seq;
	    iCascade->allNames(ETextStyle, seq);
	    for (auto & a : seq)
		addEntry(a.string(), a.string() == iAll.iTextStyle.string());
	}
	break;
    case EDynLabelStyle:
	if (iCascade) {
	    AttributeSeq seq;
	    iCascade->allNames(ELabelStyle, seq);
	    for (auto & a : seq)
		addEntry(a.string(), a.string() == iAll.iLabelStyle.string());
	}
	break;
    case EDynGridSize:
	for (auto & s : iComboContents[EUiGridSize])
	    addEntry(s, s == selectorCurrentText(EUiGridSize));
	break;
    case EDynAngleSize:
	for (auto & s : iComboContents[EUiAngleSize])
	    addEntry(s, s == selectorCurrentText(EUiAngleSize));
	break;
    default: break;
    }
}

// --------------------------------------------------------------------

GtkWidget * AppUi::addToolButton(GtkWidget * toolbar, const char * name,
				 const char * title) {
    int idx = actionId(name);
    if (idx < 0) return nullptr;
    bool checkable = GTK_IS_CHECK_MENU_ITEM(iActions[idx].menuItem);
    GtkToolItem * item =
	checkable ? gtk_toggle_tool_button_new() : gtk_tool_button_new(nullptr, nullptr);
    setButtonIcon(GTK_WIDGET(item), name, 22);
    String tip = title ? String(title) : menuLabel(idx);
    gtk_widget_set_tooltip_text(GTK_WIDGET(item), tip.z());
    if (checkable)
	gtk_toggle_tool_button_set_active(
	    GTK_TOGGLE_TOOL_BUTTON(item),
	    gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(iActions[idx].menuItem)));
    g_object_set_data(G_OBJECT(item), "ipe-appui", this);
    g_signal_connect(item, "clicked", G_CALLBACK(toolitem_cb), GINT_TO_POINTER(idx));
    iActions[idx].toolItem = GTK_WIDGET(item);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_show(GTK_WIDGET(item));
    return GTK_WIDGET(item);
}

void AppUi::addSnap(const char * name) { addToolButton(iSnapTools, name); }

void AppUi::addEdit(const char * name) { addToolButton(iEditTools, name); }

void AppUi::shift_key_cb(GtkWidget * b, gpointer data) {
    AppUi * ui = (AppUi *)data;
    if (ui->iCanvas) {
	int mod = 0;
	if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(b)))
	    mod |= CanvasBase::EShift;
	ui->iCanvas->setAdditionalModifiers(mod);
    }
}

void AppUi::abort_cb(GtkWidget * b, gpointer data) { ((AppUi *)data)->action("stop"); }

void AppUi::absolute_button_cb(GtkWidget * b, gpointer data) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(b), "ipe-appui");
    ui->absoluteButton(GPOINTER_TO_INT(data));
}

void AppUi::absoluteButton(int id) { luaAbsoluteButton(selectorNames[id]); }

// --------------------------------------------------------------------

static void set_widget_css(GtkWidget * w, const char * css) {
    GtkCssProvider * provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, nullptr);
    gtk_style_context_add_provider(gtk_widget_get_style_context(w),
				   GTK_STYLE_PROVIDER(provider),
				   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

String AppUi::selectorCurrentText(int sel) const {
    GtkTreeIter it;
    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(iSelector[sel]), &it))
	return String();
    GtkTreeModel * model = gtk_combo_box_get_model(GTK_COMBO_BOX(iSelector[sel]));
    gchar * text = nullptr;
    gtk_tree_model_get(model, &it, 1, &text, -1);
    String result = text ? String(text) : String();
    g_free(text);
    return result;
}

void AppUi::combo_changed_cb(GtkComboBox * combo, gpointer data) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(combo), "ipe-appui");
    ui->comboSelector(GPOINTER_TO_INT(data));
}

void AppUi::comboSelector(int id) {
    luaSelector(String(selectorNames[id]), selectorCurrentText(id));
}

static GtkWidget * new_icon_combo() {
    GtkListStore * store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    GtkWidget * combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    GtkCellRenderer * pixRenderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), pixRenderer, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), pixRenderer, "pixbuf", 0);
    GtkCellRenderer * textRenderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), textRenderer, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), textRenderer, "text", 1);
    return combo;
}

void AppUi::resetCombos() {
    for (int i = 0; i < EUiView; ++i) {
	g_signal_handlers_block_by_func(iSelector[i], (gpointer)combo_changed_cb,
					GINT_TO_POINTER(i));
	GtkListStore * store =
	    GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(iSelector[i])));
	gtk_list_store_clear(store);
	g_signal_handlers_unblock_by_func(iSelector[i], (gpointer)combo_changed_cb,
					  GINT_TO_POINTER(i));
    }
}

void AppUi::addCombo(int sel, String s) {
    GtkListStore * store =
	GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(iSelector[sel])));
    GtkTreeIter it;
    gtk_list_store_append(store, &it);
    gtk_list_store_set(store, &it, 0, nullptr, 1, s.z(), -1);
    if (sel == EUiVariant) {
	if (iComboContents[EUiVariant].size() <= 1)
	    gtk_widget_hide(iVariantTools);
	else
	    gtk_widget_show(iVariantTools);
    }
}

void AppUi::addComboColors(AttributeSeq & sym, AttributeSeq & abs) {
    GtkListStore * strokeStore =
	GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(iSelector[EUiStroke])));
    GtkListStore * fillStore =
	GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(iSelector[EUiFill])));
    GtkTreeIter it;
    gtk_list_store_append(strokeStore, &it);
    gtk_list_store_set(strokeStore, &it, 0, nullptr, 1, IPEABSOLUTE, -1);
    gtk_list_store_append(fillStore, &it);
    gtk_list_store_set(fillStore, &it, 0, nullptr, 1, IPEABSOLUTE, -1);
    iComboContents[EUiStroke].push_back(IPEABSOLUTE);
    iComboContents[EUiFill].push_back(IPEABSOLUTE);
    for (uint i = 0; i < sym.size(); ++i) {
	Color color = abs[i].color();
	String s = sym[i].string();
	GdkPixbuf * icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 16, 16);
	guint32 pixel = (guint32(color.iRed.toDouble() * 255) << 24)
			| (guint32(color.iGreen.toDouble() * 255) << 16)
			| (guint32(color.iBlue.toDouble() * 255) << 8) | 0xff;
	gdk_pixbuf_fill(icon, pixel);

	gtk_list_store_append(strokeStore, &it);
	gtk_list_store_set(strokeStore, &it, 0, icon, 1, s.z(), -1);
	gtk_list_store_append(fillStore, &it);
	gtk_list_store_set(fillStore, &it, 0, icon, 1, s.z(), -1);
	g_object_unref(icon);

	iComboContents[EUiStroke].push_back(s);
	iComboContents[EUiFill].push_back(s);
    }
}

void AppUi::setComboCurrent(int sel, int idx) {
    g_signal_handlers_block_by_func(iSelector[sel], (gpointer)combo_changed_cb,
				    GINT_TO_POINTER(sel));
    gtk_combo_box_set_active(GTK_COMBO_BOX(iSelector[sel]), idx);
    g_signal_handlers_unblock_by_func(iSelector[sel], (gpointer)combo_changed_cb,
				      GINT_TO_POINTER(sel));
}

void AppUi::setButtonColor(int sel, Color color) {
    if (iButton[sel]) setButtonColorIcon(iButton[sel], color, 16);
}

void AppUi::setPathView(const AllAttributes & all, Cascade * sheet) {
    iPathView.set(all, sheet);
}

void AppUi::setCheckMark(String name, Attribute a) {
    String sa = name + "|";
    int na = sa.size();
    String sb = sa + a.string();
    for (auto & act : iActions) {
	if (act.name.left(na) == sa && GTK_IS_CHECK_MENU_ITEM(act.menuItem)) {
	    bool active = (act.name == sb);
	    g_signal_handlers_block_by_func(act.menuItem, (gpointer)menuitem_cb,
					    GINT_TO_POINTER(actionId(act.name.z())));
	    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(act.menuItem), active);
	    g_signal_handlers_unblock_by_func(act.menuItem, (gpointer)menuitem_cb,
					      GINT_TO_POINTER(actionId(act.name.z())));
	}
    }
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm) {
    if (vno.empty()) {
	gtk_widget_hide(iViewNumber);
	gtk_widget_hide(iViewMarked);
    } else {
	gtk_button_set_label(GTK_BUTTON(iViewNumber), vno.z());
	gtk_widget_show(iViewNumber);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iViewMarked), vm);
	gtk_widget_show(iViewMarked);
    }
    if (pno.empty()) {
	gtk_widget_hide(iPageNumber);
	gtk_widget_hide(iPageMarked);
    } else {
	gtk_button_set_label(GTK_BUTTON(iPageNumber), pno.z());
	gtk_widget_show(iPageNumber);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iPageMarked), pm);
	gtk_widget_show(iPageMarked);
    }
}

void AppUi::setNotes(String notes) {
    GtkTextBuffer * buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(iPageNotes));
    gtk_text_buffer_set_text(buf, notes.z(), -1);
}

void AppUi::setLayers(const Page * page, int view) { iLayerList.set(page, view); }

void AppUi::bookmark_row_activated_cb(GtkListBox * box, GtkListBoxRow * row,
				      gpointer data) {
    AppUi * ui = (AppUi *)data;
    ui->bookmarkSelected(gtk_list_box_row_get_index(row));
}

void AppUi::bookmarkSelected(int index) { luaBookmarkSelected(index); }

void AppUi::setBookmarks(int no, const String * s) {
    clear_menu(iBookmarks);
    for (int i = 0; i < no; ++i) {
	GtkWidget * label = gtk_label_new(s[i].z());
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	if (s[i][0] == ' ') set_widget_css(label, "label { color: blue; }");
	gtk_container_add(GTK_CONTAINER(iBookmarks), label);
	gtk_widget_show(label);
    }
}

void AppUi::toggleToolVisible(int m) { /* unused, kept for symmetry with Qt */ }

void AppUi::setToolVisible(int m, bool vis) {
    GtkWidget * tool = nullptr;
    switch (m) {
    case 0: tool = iPropertiesTools; break;
    case 1: tool = iBookmarkTools; break;
    case 2: tool = iNotesTools; break;
    case 3: tool = iLayerTools; break;
    default: break;
    }
    if (!tool) return;
    if (vis)
	gtk_widget_show(tool);
    else
	gtk_widget_hide(tool);
}

void AppUi::setZoom(double zoom) {
    char s[32];
    sprintf(s, "(%dppi)", int(72.0 * zoom));
    iCanvas->setZoom(zoom);
    gtk_label_set_text(GTK_LABEL(iResolution), s);
}

// --------------------------------------------------------------------

void AppUi::setActionsEnabled(bool mode) {
    for (int id : {EFileMenu, EEditMenu, EModeMenu, EPropertiesMenu, ELayerMenu,
		   EViewMenu, EPageMenu, EIpeletMenu})
	gtk_widget_set_sensitive(iRootMenu[id], mode);
    gtk_widget_set_sensitive(iPropertiesTools, mode);
    gtk_widget_set_sensitive(iLayerTools, mode);
    gtk_widget_set_sensitive(iBookmarkTools, mode);
    gtk_widget_set_sensitive(iObjectTools, mode);
}

// --------------------------------------------------------------------

int AppUi::actionId(const char * name) const {
    for (int i = 0; i < int(iActions.size()); ++i) {
	if (iActions[i].name == name) return i;
    }
    return -1;
}

// only used for snapXXX, grid_visible, pretty_display, viewmarked, pagemarked
bool AppUi::actionState(const char * name) {
    if (!strcmp(name, "viewmarked"))
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iViewMarked));
    if (!strcmp(name, "pagemarked"))
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iPageMarked));
    int idx = actionId(name);
    return (idx >= 0 && GTK_IS_CHECK_MENU_ITEM(iActions[idx].menuItem))
	       ? gtk_check_menu_item_get_active(
		     GTK_CHECK_MENU_ITEM(iActions[idx].menuItem))
	       : false;
}

void AppUi::setActionState(const char * name, bool value) {
    int idx = actionId(name);
    if (idx < 0) return;
    syncAndTrigger(idx, value, nullptr); // only syncs the widgets, does not call action()
}

// --------------------------------------------------------------------

static const char * const aboutText =
    "<span size='x-large'>Ipe %d.%d.%d</span>\n\n"
    "Copyright (c) 1993-%d Otfried Cheong\n\n"
    "The extensible drawing editor Ipe creates figures in PDF format, "
    "using LaTeX to format the text in the figures.\n\n"
    "Ipe is released under the GNU Public License.\n\n"
    "See http://ipe.otfried.org for further information.";

void AppUi::aboutIpe() {
    std::vector<char> buf(strlen(aboutText) + 100);
    sprintf(buf.data(), aboutText, IPELIB_VERSION / 10000, (IPELIB_VERSION / 100) % 100,
	    IPELIB_VERSION % 100, COPYRIGHT_YEAR);
    GtkWidget * dialog = gtk_message_dialog_new(
	GTK_WINDOW(iWindow), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, nullptr);
    gtk_window_set_title(GTK_WINDOW(dialog), "About Ipe");
    gtk_window_set_icon_name(GTK_WINDOW(dialog), "ipe");
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), buf.data());
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void AppUi::action(String name) {
    if (name == "fullscreen") {
	GdkWindowState state = gdk_window_get_state(gtk_widget_get_window(iWindow));
	if (state & GDK_WINDOW_STATE_FULLSCREEN)
	    gtk_window_unfullscreen(GTK_WINDOW(iWindow));
	else
	    gtk_window_fullscreen(GTK_WINDOW(iWindow));
    } else if (name == "about") {
	aboutIpe();
    } else {
	if (name.left(5) == "mode_")
	    gtk_image_set_from_pixbuf(GTK_IMAGE(iModeIndicator), prefsPixbuf(name, 22));
	luaAction(name);
    }
}

// --------------------------------------------------------------------

void AppUi::showPathStylePopup(int x, int y) { luaShowPathStylePopup(Vector(x, y)); }

void AppUi::showLayerBoxPopup(int x, int y, String layer) {
    luaShowLayerBoxPopup(Vector(x, y), layer);
}

void AppUi::layerAction(String name, String layer) { luaLayerAction(name, layer); }

void AppUi::recentFileSelected(String name) { luaRecentFileSelected(name); }

void AppUi::recent_file_cb(GtkWidget * item, gpointer) {
    AppUi * ui = (AppUi *)g_object_get_data(G_OBJECT(item), "ipe-appui");
    const char * fn = (const char *)g_object_get_data(G_OBJECT(item), "ipe-filename");
    ui->recentFileSelected(fn);
}

void AppUi::setRecentFileMenu(const std::vector<String> & names) {
    clear_menu(GTK_WIDGET(iRecentFileMenu));
    for (auto & name : names) {
	GtkWidget * item = gtk_menu_item_new_with_label(name.z());
	g_object_set_data(G_OBJECT(item), "ipe-appui", this);
	g_object_set_data_full(G_OBJECT(item), "ipe-filename", g_strdup(name.z()),
			       g_free);
	g_signal_connect(item, "activate", G_CALLBACK(recent_file_cb), nullptr);
	gtk_menu_shell_append(GTK_MENU_SHELL(iRecentFileMenu), item);
	gtk_widget_show(item);
    }
}

// --------------------------------------------------------------------

int AppUi::pageSorter(lua_State * L, Document * doc, int pno, int width, int height,
		      int thumbWidth) {
    GtkWidget * dialog =
	gtk_dialog_new_with_buttons(pno >= 0 ? "Ipe View Sorter" : "Ipe Page Sorter",
				    GTK_WINDOW(iWindow), GTK_DIALOG_MODAL, "_Cancel",
				    GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_OK, nullptr);
    gtk_window_set_default_size(GTK_WINDOW(dialog), width, height);

    PageSorter sorter(doc, pno, thumbWidth);
    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), sorter.window(), TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_OK) {
	gtk_widget_destroy(dialog);
	return 0;
    }

    int n = sorter.count();
    lua_createtable(L, n, 0);
    for (int i = 1; i <= n; ++i) {
	lua_pushinteger(L, sorter.pageAt(i - 1) + 1);
	lua_rawseti(L, -2, i);
    }
    int m = int(sorter.iMarks.size());
    lua_createtable(L, m, 0);
    for (int i = 1; i <= m; ++i) {
	lua_pushboolean(L, sorter.iMarks[i - 1]);
	lua_rawseti(L, -2, i);
    }
    gtk_widget_destroy(dialog);
    return 2;
}

// --------------------------------------------------------------------

WINID AppUi::windowId() { return iWindow; }

gboolean AppUi::delete_event_cb(GtkWidget * w, GdkEvent * ev, gpointer data) {
    ((AppUi *)data)->closeWindow();
    return TRUE; // do not let GTK destroy the window itself
}

void AppUi::closeWindow() {
    lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
    lua_getfield(L, -1, "okay_close");
    bool okay = lua_toboolean(L, -1);
    lua_pop(L, 2);
    if (okay)
	delete this;
    else
	wrapCall("closeEvent", 0);
}

void AppUi::explain(const char * s, int t) {
    gtk_statusbar_pop(GTK_STATUSBAR(iStatusBar), iStatusBarContextid);
    gtk_statusbar_push(GTK_STATUSBAR(iStatusBar), iStatusBarContextid, s);
}

void AppUi::setWindowCaption(bool modified, const char * caption, const char * filename) {
    gtk_window_set_title(GTK_WINDOW(iWindow), caption);
}

void AppUi::setMouseIndicator(const char * s) {
    gtk_label_set_text(GTK_LABEL(iMousePosition), s);
}

void AppUi::setSnapIndicator(const char * s) {
    gtk_label_set_text(GTK_LABEL(iSnapIndicator), s);
}

void AppUi::showWindow(int width, int height, int x, int y, const Color & pathViewColor) {
    iPathView.setColor(pathViewColor);
    if (width > 0 && height > 0) gtk_window_resize(GTK_WINDOW(iWindow), width, height);
    if (x >= 0 && y >= 0) gtk_window_move(GTK_WINDOW(iWindow), x, y);
    gtk_widget_show(iWindow);
}

void AppUi::setFullScreen(int mode) {
    switch (mode) {
    case 1: gtk_window_maximize(GTK_WINDOW(iWindow)); break;
    case 2: gtk_window_fullscreen(GTK_WINDOW(iWindow)); break;
    default:
	gtk_window_unmaximize(GTK_WINDOW(iWindow));
	gtk_window_unfullscreen(GTK_WINDOW(iWindow));
	break;
    }
}

int AppUi::setClipboard(lua_State * L) {
    const char * data = luaL_checkstring(L, 2);
    GtkClipboard * b = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(b, data, -1);
    return 0;
}

int AppUi::clipboard(lua_State * L) {
    // bitmap clipboard access is not implemented for GTK
    GtkClipboard * b = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    char * data = gtk_clipboard_wait_for_text(b);
    if (data) {
	lua_pushstring(L, data);
	g_free(data);
	return 1;
    }
    return 0;
}

// --------------------------------------------------------------------

namespace {
struct WaitCtx {
    AppUi * self;
    GtkWidget * dialog;
    bool shown = false;
    bool completed = false;
};
} // namespace

static void waitdialog_child_watch_cb(GPid pid, gint status, gpointer data) {
    WaitCtx * ctx = (WaitCtx *)data;
    g_spawn_close_pid(pid);
    ctx->completed = true;
    if (ctx->shown) {
	AppUi * self = ctx->self;
	gtk_widget_destroy(ctx->dialog);
	delete ctx;
	self->resumeLua();
    }
}

bool AppUi::waitDialog(const char * cmd, const char * label) {
    GPid pid;
    GError * error = nullptr;
    char shell[] = "/bin/sh";
    char opt[] = "-c";
    std::vector<char> cmdbuf(cmd, cmd + strlen(cmd) + 1);
    char * argv[] = {shell, opt, cmdbuf.data(), nullptr};
    bool ok = g_spawn_async(nullptr, argv, nullptr, G_SPAWN_DO_NOT_REAP_CHILD, nullptr,
			    nullptr, &pid, &error);
    if (!ok) {
	ipeDebug("waitDialog: g_spawn_async failed: %s", error->message);
	g_error_free(error);
	return true;
    }

    WaitCtx * ctx = new WaitCtx();
    ctx->self = this;

    GtkWidget * dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Ipe: waiting");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(iWindow));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);
    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget * l = gtk_label_new(label);
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);
    gtk_box_pack_start(GTK_BOX(content), l, TRUE, TRUE, 0);
    gtk_widget_show_all(content);
    ctx->dialog = dialog;

    g_child_watch_add(pid, waitdialog_child_watch_cb, ctx);

    for (int i = 0; i < 30 && !ctx->completed; ++i) {
	g_usleep(10000);
	while (gtk_events_pending()) gtk_main_iteration();
    }

    if (ctx->completed) {
	gtk_widget_destroy(dialog);
	delete ctx;
	return true;
    }
    ctx->shown = true;
    gtk_widget_show(dialog);
    return false;
}

// --------------------------------------------------------------------

static GtkWidget * make_framed(const char * title, GtkWidget * child) {
    GtkWidget * frame = gtk_frame_new(title);
    gtk_container_add(GTK_CONTAINER(frame), child);
    return frame;
}

AppUi::AppUi(lua_State * L0, int model)
    : AppUiBase(L0, model)
    , iLayerList()
    , iPathView(iUiScale) {
    iWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(iWindow, "delete-event", G_CALLBACK(delete_event_cb), this);

    iAccelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(iWindow), iAccelGroup);

    iSnapTools = gtk_toolbar_new();
    iVariantTools = gtk_toolbar_new();
    iEditTools = gtk_toolbar_new();
    iObjectTools = gtk_toolbar_new();
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(iSnapTools), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(iVariantTools), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(iEditTools), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(iObjectTools), GTK_ICON_SIZE_SMALL_TOOLBAR);

    buildMenus();

    GtkWidget * menu_bar = gtk_menu_bar_new();
    for (int i = 0; i < ENumMenu; ++i) {
	gtk_widget_show(iRootMenu[i]);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(iRootMenu[i]), iSubMenu[i]);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), iRootMenu[i]);
    }

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(iWindow), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    gtk_widget_show(menu_bar);

    GtkWidget * row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(row1), iSnapTools, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row1), iVariantTools, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row1), iEditTools, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), row1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), iObjectTools, FALSE, FALSE, 0);

    iShiftKey = GTK_WIDGET(gtk_toggle_tool_button_new());
    setButtonIcon(iShiftKey, "shift_key", 22);
    gtk_widget_set_tooltip_text(iShiftKey, "Shift key");
    g_signal_connect(iShiftKey, "clicked", G_CALLBACK(shift_key_cb), this);
    gtk_toolbar_insert(GTK_TOOLBAR(iEditTools), GTK_TOOL_ITEM(iShiftKey), -1);
    gtk_widget_show(iShiftKey);

    iAbortButton = GTK_WIDGET(gtk_tool_button_new(nullptr, nullptr));
    setButtonIcon(iAbortButton, "stop", 22);
    gtk_widget_set_tooltip_text(iAbortButton, "Stop current operation");
    g_signal_connect(iAbortButton, "clicked", G_CALLBACK(abort_cb), this);
    gtk_toolbar_insert(GTK_TOOLBAR(iEditTools), GTK_TOOL_ITEM(iAbortButton), -1);
    gtk_widget_show(iAbortButton);

    addSnap("snapvtx");
    addSnap("snapctl");
    addSnap("snapbd");
    addSnap("snapint");
    addSnap("snapgrid");
    iSelector[EUiGridSize] = new_icon_combo();
    GtkToolItem * gridItem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(gridItem), iSelector[EUiGridSize]);
    gtk_toolbar_insert(GTK_TOOLBAR(iSnapTools), gridItem, -1);
    addSnap("snapangle");
    iSelector[EUiAngleSize] = new_icon_combo();
    GtkToolItem * angleItem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(angleItem), iSelector[EUiAngleSize]);
    gtk_toolbar_insert(GTK_TOOLBAR(iSnapTools), angleItem, -1);
    addSnap("snapcustom");
    addSnap("snapauto");

    iSelector[EUiVariant] = new_icon_combo();
    GtkToolItem * variantItem = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(variantItem), iSelector[EUiVariant]);
    gtk_toolbar_insert(GTK_TOOLBAR(iVariantTools), variantItem, -1);

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
    addEdit("grid_visible");

    for (int i = 0; i < EUiView; ++i) {
	if (i != EUiGridSize && i != EUiAngleSize && i != EUiVariant)
	    iSelector[i] = new_icon_combo();
	g_object_set_data(G_OBJECT(iSelector[i]), "ipe-appui", this);
	g_signal_connect(iSelector[i], "changed", G_CALLBACK(combo_changed_cb),
			 GINT_TO_POINTER(i));
    }

    // properties grid
    GtkWidget * grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 1);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 2);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 2);

    iButton[EUiDashStyle] = nullptr;
    iButton[EUiMarkShape] = nullptr;
    for (int i = 0; i <= EUiSymbolSize; ++i) {
	if (i == EUiDashStyle || i == EUiMarkShape) continue;
	iButton[i] = gtk_button_new();
	g_object_set_data(G_OBJECT(iButton[i]), "ipe-appui", this);
	g_signal_connect(iButton[i], "clicked", G_CALLBACK(absolute_button_cb),
			 GINT_TO_POINTER(i));
	int row = (i >= EUiTextSize) ? i + 1 : i;
	int rowspan = (i == EUiPen || i == EUiSymbolSize) ? 2 : 1;
	gtk_grid_attach(GTK_GRID(grid), iButton[i], 0, row, 1, rowspan);
    }
    setButtonColorIcon(iButton[EUiStroke], Color(1000, 0, 0), 16);
    setButtonColorIcon(iButton[EUiFill], Color(1000, 1000, 0), 16);
    setButtonIcon(iButton[EUiPen], "pen", 16);
    setButtonIcon(iButton[EUiTextSize], "mode_label", 16);
    setButtonIcon(iButton[EUiSymbolSize], "mode_marks", 16);
    gtk_widget_set_tooltip_text(iButton[EUiStroke], "Absolute stroke color");
    gtk_widget_set_tooltip_text(iButton[EUiFill], "Absolute fill color");
    gtk_widget_set_tooltip_text(iButton[EUiPen], "Absolute pen width");
    gtk_widget_set_tooltip_text(iButton[EUiTextSize], "Absolute text size");
    gtk_widget_set_tooltip_text(iButton[EUiSymbolSize], "Absolute symbol size");

    for (int i = 0; i < EUiGridSize; ++i) {
	int row = (i == EUiOpacity) ? i + 1 : ((i >= EUiTextSize) ? i + 1 : i);
	int col = (i == EUiOpacity) ? 0 : 1;
	int colspan = (i == EUiOpacity) ? 2 : 1;
	gtk_grid_attach(GTK_GRID(grid), iSelector[i], col, row, colspan, 1);
    }
    gtk_widget_set_tooltip_text(iSelector[EUiStroke], "Symbolic stroke color");
    gtk_widget_set_tooltip_text(iSelector[EUiFill], "Symbolic fill color");
    gtk_widget_set_tooltip_text(iSelector[EUiPen], "Symbolic pen width");
    gtk_widget_set_tooltip_text(iSelector[EUiTextSize], "Symbolic text size");
    gtk_widget_set_tooltip_text(iSelector[EUiMarkShape], "Mark shape");
    gtk_widget_set_tooltip_text(iSelector[EUiSymbolSize], "Symbolic symbol size");
    gtk_widget_set_tooltip_text(iSelector[EUiDashStyle], "Dash style");
    gtk_widget_set_tooltip_text(iSelector[EUiOpacity], "Opacity");
    gtk_widget_set_tooltip_text(iSelector[EUiGridSize], "Grid size");
    gtk_widget_set_tooltip_text(iSelector[EUiAngleSize], "Angle for angular snap");
    gtk_widget_set_tooltip_text(iSelector[EUiVariant],
				"Variant to display and to use for new text");

    iModeIndicator = gtk_image_new();
    gtk_grid_attach(GTK_GRID(grid), iModeIndicator, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), iPathView.window(), 1, 4, 1, 1);
    iPathView.activated = [this](String s) { action(s); };
    iPathView.showPathStylePopup = [this](int x, int y) { showPathStylePopup(x, y); };

    GtkWidget * hol = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    iViewMarked = gtk_check_button_new();
    iPageMarked = gtk_check_button_new();
    iViewNumber = gtk_button_new_with_label("View 1/1");
    iPageNumber = gtk_button_new_with_label("Page 1/1");
    gtk_widget_set_tooltip_text(iViewNumber, "Current view number");
    gtk_widget_set_tooltip_text(iPageNumber, "Current page number");
    g_object_set_data(G_OBJECT(iViewNumber), "ipe-appui", this);
    g_object_set_data(G_OBJECT(iPageNumber), "ipe-appui", this);
    g_object_set_data(G_OBJECT(iViewMarked), "ipe-appui", this);
    g_object_set_data(G_OBJECT(iPageMarked), "ipe-appui", this);
    g_signal_connect(iViewNumber, "clicked", G_CALLBACK(absolute_button_cb),
		     GINT_TO_POINTER(EUiView));
    g_signal_connect(iPageNumber, "clicked", G_CALLBACK(absolute_button_cb),
		     GINT_TO_POINTER(EUiPage));
    g_signal_connect(iViewMarked, "clicked", G_CALLBACK(absolute_button_cb),
		     GINT_TO_POINTER(EUiViewMarked));
    g_signal_connect(iPageMarked, "clicked", G_CALLBACK(absolute_button_cb),
		     GINT_TO_POINTER(EUiPageMarked));
    gtk_box_pack_start(GTK_BOX(hol), iViewMarked, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hol), iViewNumber, FALSE, FALSE, 0);
    GtkWidget * spacer = gtk_label_new(nullptr);
    gtk_box_pack_start(GTK_BOX(hol), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hol), iPageMarked, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hol), iPageNumber, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid), hol, 0, EUiOpacity + 2, 2, 1);

    iPropertiesTools = make_framed("Properties", grid);

    iLayerTools = make_framed("Layers", iLayerList.window());
    iLayerList.activated = [this](String a, String b) { layerAction(a, b); };
    iLayerList.showLayerBoxPopup = [this](int x, int y, String l) {
	showLayerBoxPopup(x, y, l);
    };

    iBookmarks = gtk_list_box_new();
    g_signal_connect(iBookmarks, "row-activated", G_CALLBACK(bookmark_row_activated_cb),
		     this);
    GtkWidget * bookmarkScroller = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(bookmarkScroller),
				   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(bookmarkScroller), iBookmarks);
    iBookmarkTools = make_framed("Bookmarks", bookmarkScroller);

    iPageNotes = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(iPageNotes), FALSE);
    GtkWidget * notesScroller = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(notesScroller), iPageNotes);
    iNotesTools = make_framed("Notes", notesScroller);

    GtkWidget * leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(leftBox), iPropertiesTools, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), iLayerTools, TRUE, TRUE, 0);

    GtkWidget * rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(rightBox), iBookmarkTools, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), iNotesTools, TRUE, TRUE, 0);

    Canvas * canvas = new Canvas(iWindow);
    iCanvas = canvas;

    GtkWidget * innerPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack1(GTK_PANED(innerPaned), canvas->window(), TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(innerPaned), rightBox, FALSE, TRUE);

    GtkWidget * outerPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack1(GTK_PANED(outerPaned), leftBox, FALSE, TRUE);
    gtk_paned_pack2(GTK_PANED(outerPaned), innerPaned, TRUE, TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), outerPaned, TRUE, TRUE, 0);

    iStatusBar = gtk_statusbar_new();
    iStatusBarContextid =
	gtk_statusbar_get_context_id(GTK_STATUSBAR(iStatusBar), "explain");
    iMousePosition = gtk_label_new(nullptr);
    iSnapIndicator = gtk_label_new(nullptr);
    iResolution = gtk_label_new(nullptr);
    GtkWidget * sb = gtk_statusbar_get_message_area(GTK_STATUSBAR(iStatusBar));
    gtk_box_pack_end(GTK_BOX(sb), iResolution, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(sb), iMousePosition, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(sb), iSnapIndicator, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(vbox), iStatusBar, FALSE, FALSE, 0);

    gtk_widget_show_all(vbox);
    gtk_widget_hide(iVariantTools); // shown again once addCombo(EUiVariant,...) is called

    iCanvas->setObserver(this);
}

AppUi::~AppUi() { ipeDebug("AppUi C++ destructor"); }

// --------------------------------------------------------------------

AppUiBase * createAppUi(lua_State * L0, int model) { return new AppUi(L0, model); }

// --------------------------------------------------------------------
