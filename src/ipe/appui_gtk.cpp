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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace ipe;
using namespace ipelua;

extern void resumeLuaThread(lua_State * T, int nArgs);

// --------------------------------------------------------------------
// dynamic (rebuilt-on-change) submenus
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

// Some shortcuts.lua entries use Qt-style key names that don't match GDK's
// X11-derived keysym names.
static const char * gdkKeyName(const String & name) {
    static const std::map<String, const char *> special = {
	{"pgup", "Page_Up"},        {"pgdown", "Page_Down"}, {"delete", "Delete"},
	{"backspace", "BackSpace"}, {"return", "Return"},    {"enter", "Return"},
	{"escape", "Escape"},       {"esc", "Escape"},       {"tab", "Tab"},
	{"space", "space"},         {"insert", "Insert"},    {"ins", "Insert"},
    };
    String lower;
    for (int i = 0; i < name.size(); ++i) lower += char(tolower((unsigned char)name[i]));
    auto it = special.find(lower);
    return it != special.end() ? it->second : name.z();
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
	    mods = (GdkModifierType)(mods | GDK_ALT_MASK);
	else if (tok == "Meta" || tok == "Command")
	    mods = (GdkModifierType)(mods | GDK_META_MASK);
    }
    if (rest.empty()) return false;
    keyval = gdk_keyval_from_name(gdkKeyName(rest));
    if ((keyval == 0 || keyval == GDK_KEY_VoidSymbol) && rest.size() == 1)
	keyval = gdk_unicode_to_keyval((guchar)rest[0]);
    return keyval != 0 && keyval != GDK_KEY_VoidSymbol;
}

static GtkWidget * makeFramed(const char * title, GtkWidget * child) {
    gtk_widget_set_margin_start(child, 4);
    gtk_widget_set_margin_end(child, 4);
    gtk_widget_set_margin_top(child, 4);
    gtk_widget_set_margin_bottom(child, 4);
    GtkWidget * frame = gtk_frame_new(title);
    gtk_frame_set_child(GTK_FRAME(frame), child);
    return frame;
}

static void about_response_cb(GtkDialog * d, int, gpointer) {
    gtk_window_destroy(GTK_WINDOW(d));
}

// --------------------------------------------------------------------
// icons

GdkPixbuf * AppUi::prefsPixbuf(String name, int size) {
    // render at the monitor's actual scale factor so icons stay crisp on HiDPI
    int scale = gtk_widget_get_scale_factor(iWindow);
    if (scale < 1) scale = 1;
    int renderSize = size * scale;
    char keybuf[64];
    sprintf(keybuf, "%d:", renderSize);
    String key = String(keybuf) + name;
    auto it = iIconCache.find(key);
    if (it != iIconCache.end()) return it->second;

    GdkPixbuf * pixbuf = nullptr;
    if (name == "ipe") {
	String fname = Platform::folder(FolderIcons, "icon_128x128.png");
	if (Platform::fileExists(fname))
	    pixbuf = gdk_pixbuf_new_from_file_at_scale(fname.z(), renderSize, renderSize,
						       TRUE, nullptr);
    }
    if (!pixbuf) {
	int pno = ipeIcon(name);
	if (pno >= 0) {
	    GdkRGBA fg;
	    gtk_widget_get_color(iWindow, &fg);
	    bool dark = (0.299 * fg.red + 0.587 * fg.green + 0.114 * fg.blue) > 0.5;
	    Document * doc = dark ? ipeIconsDark.get() : ipeIcons.get();
	    if (doc) {
		Thumbnail thumbs(doc, renderSize);
		thumbs.setTransparent(true);
		thumbs.setNoCrop(true);
		Buffer b = thumbs.render(doc->page(pno), 0);
		pixbuf = pixbufFromArgb32((const uint8_t *)b.data(), renderSize,
					  thumbs.height(), renderSize * 4);
	    }
	}
    }
    if (pixbuf) iIconCache[key] = pixbuf;
    return pixbuf;
}

void AppUi::setButtonIcon(GtkWidget * button, String name, int size,
			  const char * cssName) {
    GdkPixbuf * pixbuf = prefsPixbuf(name, size);
    if (!pixbuf) return;
    GdkTexture * texture = textureFromPixbuf(pixbuf);
    GtkWidget * image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
    gtk_image_set_pixel_size(GTK_IMAGE(image), size);
    g_object_unref(texture);
    gtk_button_set_child(GTK_BUTTON(button), image);
    if (cssName) {
	gtk_widget_add_css_class(button, cssName);
    } else if (name.hasPrefix("snap")) {
	gtk_widget_add_css_class(button, "snap");
    } else if (name.hasPrefix("mode_")) {
	gtk_widget_add_css_class(button, "mode");
    } else
	gtk_widget_add_css_class(button, "action");
}

void AppUi::setButtonColorIcon(GtkWidget * button, Color color, int size) {
    GdkPixbuf * pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, size, size);
    guint32 pixel = (guint32(color.iRed.toDouble() * 255) << 24)
		    | (guint32(color.iGreen.toDouble() * 255) << 16)
		    | (guint32(color.iBlue.toDouble() * 255) << 8) | 0xff;
    gdk_pixbuf_fill(pixbuf, pixel);
    GdkTexture * texture = textureFromPixbuf(pixbuf);
    g_object_unref(pixbuf);
    GtkWidget * image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
    gtk_image_set_pixel_size(GTK_IMAGE(image), size);
    g_object_unref(texture);
    gtk_button_set_child(GTK_BUTTON(button), image);
    gtk_widget_add_css_class(button, "color");
}

void AppUi::setButtonColor(int sel, Color color) {
    if (iButton[sel]) setButtonColorIcon(iButton[sel], color, 16);
}

void AppUi::setPathView(const AllAttributes & all, Cascade * sheet) {
    iPathView.set(all, sheet);
}

// --------------------------------------------------------------------
// menu / action building

String AppUi::opaqueName() {
    char buf[16];
    sprintf(buf, "act%d", iNextActionId++);
    return buf;
}

int AppUi::actionId(const char * name) const {
    for (int i = 0; i < int(iActions.size()); ++i)
	if (iActions[i].name == name) return i;
    return -1;
}

void AppUi::setActionAccelerator(const String & detailedName, const char * name) {
    lua_getglobal(L, "shortcuts");
    lua_getfield(L, -1, name);
    if (lua_isstring(L, -1)) {
	String s = lua_tolstring(L, -1, nullptr);
	guint keyval;
	GdkModifierType mods;
	if (parse_accelerator(s, keyval, mods)) {
	    char * accelName = gtk_accelerator_name(keyval, mods);
	    const char * accels[2] = {accelName, nullptr};
	    gtk_application_set_accels_for_action(ipeApp, detailedName.z(), accels);
	    g_free(accelName);
	}
    }
    lua_pop(L, 2); // shortcuts, name
}

void AppUi::action_activated_cb(GSimpleAction * action, GVariant *, gpointer data) {
    AppUi * self = (AppUi *)data;
    int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "ipe-index"));
    if (g_action_get_state_type(G_ACTION(action))) {
	GVariant * state = g_action_get_state(G_ACTION(action));
	bool cur = g_variant_get_boolean(state);
	g_variant_unref(state);
	g_simple_action_set_state(action, g_variant_new_boolean(!cur));
	if (self->iActions[idx].toolButton)
	    gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(self->iActions[idx].toolButton), !cur);
    }
    self->action(self->iActions[idx].name);
}

void AppUi::radio_activated_cb(GSimpleAction * action, GVariant * parameter,
			       gpointer data) {
    AppUi * self = (AppUi *)data;
    const char * target = g_variant_get_string(parameter, nullptr);
    g_simple_action_set_state(action, parameter);
    bool bare = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "ipe-bare"));
    const char * prefix = (const char *)g_object_get_data(G_OBJECT(action), "ipe-prefix");
    String ipeName = bare ? String(target) : (String(prefix) + "|" + target);
    self->action(ipeName);
}

void AppUi::layer_menu_item_cb(GSimpleAction * action, GVariant * parameter,
			       gpointer data) {
    AppUi * self = (AppUi *)data;
    const char * prefix = (const char *)g_object_get_data(G_OBJECT(action), "ipe-prefix");
    const char * layer = g_variant_get_string(parameter, nullptr);
    self->action(String(prefix) + layer);
}

GSimpleAction * AppUi::radioAction(const String & prefix, bool bare) {
    auto it = iRadioGroups.find(prefix);
    if (it != iRadioGroups.end()) return it->second;
    String actName = opaqueName();
    GSimpleAction * action = g_simple_action_new_stateful(
	actName.z(), G_VARIANT_TYPE_STRING, g_variant_new_string(""));
    g_object_set_data_full(G_OBJECT(action), "ipe-prefix", g_strdup(prefix.z()), g_free);
    g_object_set_data(G_OBJECT(action), "ipe-bare", GINT_TO_POINTER(bare ? 1 : 0));
    g_signal_connect(action, "activate", G_CALLBACK(radio_activated_cb), this);
    g_action_map_add_action(G_ACTION_MAP(iWindow), G_ACTION(action));
    iRadioGroups[prefix] = action;
    return action;
}

void AppUi::addAction(const char * title, const char * name, GMenu * section) {
    bool checkable = false;
    if (name[0] == '*') {
	checkable = true;
	name = name + 1;
    }
    String actName = opaqueName();
    GSimpleAction * action =
	checkable ? g_simple_action_new_stateful(actName.z(), nullptr,
						 g_variant_new_boolean(false))
		  : g_simple_action_new(actName.z(), nullptr);
    g_signal_connect(action, "activate", G_CALLBACK(action_activated_cb), this);
    g_action_map_add_action(G_ACTION_MAP(iWindow), G_ACTION(action));

    SAction s;
    s.name = name;
    s.title = change_mnemonic(title);
    s.action = action;
    iActions.push_back(s);
    int idx = int(iActions.size()) - 1;
    g_object_set_data(G_OBJECT(action), "ipe-index", GINT_TO_POINTER(idx));

    String detailed = String("win.") + actName;
    GMenuItem * item = g_menu_item_new(s.title.z(), detailed.z());
    g_menu_append_item(section, item);
    g_object_unref(item);

    setActionAccelerator(detailed, name);
}

void AppUi::addMenuEntry(GMenu * section, const char * title, const char * name,
			 bool isModeGroup) {
    if (name[0] == '@') name = name + 1;
    int bar = String(name).find('|');
    if (isModeGroup || bar >= 0) {
	String prefix = isModeGroup ? String("mode") : String(name).left(bar);
	String target = isModeGroup ? String(name) : String(name).substr(bar + 1);
	GSimpleAction * group = radioAction(prefix, isModeGroup);
	String actName = String("win.") + g_action_get_name(G_ACTION(group));
	GMenuItem * item = g_menu_item_new(change_mnemonic(title).z(), nullptr);
	g_menu_item_set_action_and_target_value(item, actName.z(),
						g_variant_new_string(target.z()));
	g_menu_append_item(section, item);
	g_object_unref(item);
	setActionAccelerator(actName + "::" + target, name);
    } else {
	addAction(title, name, section);
    }
}

void AppUi::addRootMenu(int id, const char * name) {
    iRootMenuTitle[id] = name;
    iSubMenu[id] = g_menu_new();
    iCurrentSection[id] = g_menu_new();
}

void AppUi::flushSection(int id) {
    if (g_menu_model_get_n_items(G_MENU_MODEL(iCurrentSection[id])) > 0)
	g_menu_append_section(iSubMenu[id], nullptr, G_MENU_MODEL(iCurrentSection[id]));
    g_object_unref(iCurrentSection[id]);
    iCurrentSection[id] = nullptr;
}

void AppUi::addItem(int id, const char * title, const char * name) {
    if (!title) {
	if (g_menu_model_get_n_items(G_MENU_MODEL(iCurrentSection[id])) > 0) {
	    g_menu_append_section(iSubMenu[id], nullptr,
				  G_MENU_MODEL(iCurrentSection[id]));
	    g_object_unref(iCurrentSection[id]);
	    iCurrentSection[id] = g_menu_new();
	}
	return;
    }
    addMenuEntry(iCurrentSection[id], title, name, id == EModeMenu);
    if (id == EModeMenu) addToolButton(iObjectTools, name, title);
}

void AppUi::startSubMenu(int id, const char * name, int) {
    iSubMenuParentId = id;
    iSubMenuBuildingTitle = name;
    iSubMenuBuilding = g_menu_new();
}

void AppUi::addSubItem(const char * title, const char * name) {
    if (!title) return; // no separators appear inside submenus
    addMenuEntry(iSubMenuBuilding, title, name, false);
}

MENUHANDLE AppUi::endSubMenu() {
    GMenuItem * item = g_menu_item_new_submenu(
	change_mnemonic(iSubMenuBuildingTitle.z()).z(), G_MENU_MODEL(iSubMenuBuilding));
    g_menu_append_item(iCurrentSection[iSubMenuParentId], item);
    g_object_unref(item);
    return iSubMenuBuilding;
}

void AppUi::populateDynamicMenu(int kind) {
    GMenu * menu = nullptr;
    switch (kind) {
    case EDynGridSize: menu = iGridSizeMenu; break;
    case EDynAngleSize: menu = iAngleSizeMenu; break;
    case EDynTextStyle: menu = iTextStyleMenu; break;
    case EDynLabelStyle: menu = iLabelStyleMenu; break;
    case EDynSelectLayer: menu = iSelectLayerMenu; break;
    case EDynMoveLayer: menu = iMoveToLayerMenu; break;
    default: break;
    }
    if (!menu) return;
    g_menu_remove_all(menu);

    if (kind == EDynSelectLayer || kind == EDynMoveLayer) {
	GSimpleAction * action =
	    (kind == EDynSelectLayer) ? iSelectLayerAction : iMoveLayerAction;
	String actName = String("win.") + g_action_get_name(G_ACTION(action));
	for (auto & layer : iLayerList.layers()) {
	    GMenuItem * item = g_menu_item_new(layer.z(), nullptr);
	    g_menu_item_set_action_and_target_value(item, actName.z(),
						    g_variant_new_string(layer.z()));
	    g_menu_append_item(menu, item);
	    g_object_unref(item);
	}
	return;
    }

    String prefix;
    std::vector<String> values;
    String current;
    switch (kind) {
    case EDynGridSize:
	prefix = "gridsize";
	values = iComboContents[EUiGridSize];
	current = selectorCurrentText(EUiGridSize);
	break;
    case EDynAngleSize:
	prefix = "anglesize";
	values = iComboContents[EUiAngleSize];
	current = selectorCurrentText(EUiAngleSize);
	break;
    case EDynTextStyle:
	prefix = "textstyle";
	if (iCascade) {
	    AttributeSeq seq;
	    iCascade->allNames(ETextStyle, seq);
	    for (auto & a : seq) values.push_back(a.string());
	}
	current = iAll.iTextStyle.string();
	break;
    case EDynLabelStyle:
	prefix = "labelstyle";
	if (iCascade) {
	    AttributeSeq seq;
	    iCascade->allNames(ELabelStyle, seq);
	    for (auto & a : seq) values.push_back(a.string());
	}
	current = iAll.iLabelStyle.string();
	break;
    default: break;
    }
    GSimpleAction * group = radioAction(prefix, false);
    String actName = String("win.") + g_action_get_name(G_ACTION(group));
    for (auto & v : values) {
	GMenuItem * item = g_menu_item_new(v.z(), nullptr);
	g_menu_item_set_action_and_target_value(item, actName.z(),
						g_variant_new_string(v.z()));
	g_menu_append_item(menu, item);
	g_object_unref(item);
    }
    g_simple_action_set_state(group, g_variant_new_string(current.z()));
}

void AppUi::setCheckMark(String name, Attribute a) {
    if (name == "textstyle") {
	populateDynamicMenu(EDynTextStyle);
	return;
    }
    if (name == "labelstyle") {
	populateDynamicMenu(EDynLabelStyle);
	return;
    }
    auto it = iRadioGroups.find(name);
    if (it != iRadioGroups.end())
	g_simple_action_set_state(it->second, g_variant_new_string(a.string().z()));
}

void AppUi::recent_file_cb(GSimpleAction *, GVariant * parameter, gpointer data) {
    AppUi * self = (AppUi *)data;
    self->recentFileSelected(g_variant_get_string(parameter, nullptr));
}

void AppUi::recentFileSelected(String name) { luaRecentFileSelected(name); }

void AppUi::setRecentFileMenu(const std::vector<String> & names) {
    g_menu_remove_all(iRecentFileMenu);
    String actName = String("win.") + g_action_get_name(G_ACTION(iRecentFileAction));
    for (auto & name : names) {
	GMenuItem * item = g_menu_item_new(name.z(), nullptr);
	g_menu_item_set_action_and_target_value(item, actName.z(),
						g_variant_new_string(name.z()));
	g_menu_append_item(iRecentFileMenu, item);
	g_object_unref(item);
    }
}

// only used for snapXXX, grid_visible, pretty_display, viewmarked, pagemarked
bool AppUi::actionState(const char * name) {
    if (!strcmp(name, "viewmarked"))
	return gtk_check_button_get_active(GTK_CHECK_BUTTON(iViewMarked));
    if (!strcmp(name, "pagemarked"))
	return gtk_check_button_get_active(GTK_CHECK_BUTTON(iPageMarked));
    int idx = actionId(name);
    if (idx < 0) return false;
    GVariant * state = g_action_get_state(G_ACTION(iActions[idx].action));
    if (!state) return false;
    bool v = g_variant_get_boolean(state);
    g_variant_unref(state);
    return v;
}

void AppUi::setActionState(const char * name, bool value) {
    int idx = actionId(name);
    if (idx < 0) return;
    g_simple_action_set_state(iActions[idx].action, g_variant_new_boolean(value));
    if (iActions[idx].toolButton)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iActions[idx].toolButton), value);
}

void AppUi::setActionsEnabled(bool mode) {
    for (auto & a : iActions) g_simple_action_set_enabled(a.action, mode);
    for (auto & kv : iRadioGroups) g_simple_action_set_enabled(kv.second, mode);
    gtk_widget_set_sensitive(iPropertiesTools, mode);
    gtk_widget_set_sensitive(iLayerTools, mode);
    gtk_widget_set_sensitive(iBookmarkTools, mode);
    gtk_widget_set_sensitive(iObjectTools, mode);
}

// --------------------------------------------------------------------
// toolbars

GtkWidget * AppUi::addToolButton(GtkWidget * toolbar, const char * name,
				 const char * title) {
    int idx = actionId(name);
    bool checkable =
	idx >= 0 && g_action_get_state_type(G_ACTION(iActions[idx].action)) != nullptr;
    GtkWidget * btn = checkable ? gtk_toggle_button_new() : gtk_button_new();
    setButtonIcon(btn, name, 22);
    String tip =
	title ? change_mnemonic(title) : (idx >= 0 ? iActions[idx].title : String(name));
    gtk_widget_set_tooltip_text(btn, tip.z());
    if (checkable) {
	GVariant * state = g_action_get_state(G_ACTION(iActions[idx].action));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn),
				     g_variant_get_boolean(state));
	g_variant_unref(state);
    }
    if (idx >= 0) iActions[idx].toolButton = btn;
    g_object_set_data_full(G_OBJECT(btn), "ipe-name", g_strdup(name), g_free);
    g_signal_connect(btn, "clicked", G_CALLBACK(toolbutton_clicked_cb), this);
    gtk_box_append(GTK_BOX(toolbar), btn);
    return btn;
}

void AppUi::toolbutton_clicked_cb(GtkWidget * b, gpointer data) {
    AppUi * self = (AppUi *)data;
    const char * name = (const char *)g_object_get_data(G_OBJECT(b), "ipe-name");
    if (GTK_IS_TOGGLE_BUTTON(b)) {
	int idx = self->actionId(name);
	bool active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
	if (idx >= 0)
	    g_simple_action_set_state(self->iActions[idx].action,
				      g_variant_new_boolean(active));
    }
    self->action(name);
}

void AppUi::addSnap(const char * name) { addToolButton(iSnapTools, name); }

void AppUi::addEdit(const char * name) { addToolButton(iEditTools, name); }

void AppUi::shift_key_cb(GtkWidget * b, gpointer data) {
    AppUi * self = (AppUi *)data;
    if (self->iCanvas) {
	int mod =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b)) ? CanvasBase::EShift : 0;
	self->iCanvas->setAdditionalModifiers(mod);
    }
}

void AppUi::abort_cb(GtkWidget *, gpointer data) { ((AppUi *)data)->action("stop"); }

// --------------------------------------------------------------------
// combo boxes (GtkDropDown)

static GtkWidget * newTextDropDown(const char * cssClass) {
    GtkStringList * list = gtk_string_list_new(nullptr);
    GtkWidget * w = gtk_drop_down_new(G_LIST_MODEL(list), nullptr);
    gtk_widget_add_css_class(w, cssClass);
    return w;
}

void AppUi::setup_combo_item_cb(GtkListItemFactory *, GtkListItem * item, gpointer) {
    GtkWidget * box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget * image = gtk_image_new();
    GtkWidget * label = gtk_label_new(nullptr);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_append(GTK_BOX(box), image);
    gtk_box_append(GTK_BOX(box), label);
    g_object_set_data(G_OBJECT(box), "ipe-image", image);
    g_object_set_data(G_OBJECT(box), "ipe-label", label);
    gtk_list_item_set_child(item, box);
}

void AppUi::bind_combo_item_cb(GtkListItemFactory *, GtkListItem * item, gpointer) {
    GObject * obj = G_OBJECT(gtk_list_item_get_item(item));
    GtkWidget * box = gtk_list_item_get_child(item);
    GtkWidget * image = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "ipe-image"));
    GtkWidget * label = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "ipe-label"));
    GdkPixbuf * pixbuf = GDK_PIXBUF(g_object_get_data(obj, "ipe-pixbuf"));
    const char * text = (const char *)g_object_get_data(obj, "ipe-text");
    if (pixbuf) {
	GdkTexture * texture = textureFromPixbuf(pixbuf);
	gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
	g_object_unref(texture);
	gtk_widget_set_visible(image, TRUE);
    } else {
	gtk_widget_set_visible(image, FALSE);
    }
    gtk_label_set_text(GTK_LABEL(label), text);
}

static GtkWidget * newColorDropDown() {
    GListStore * store = g_list_store_new(G_TYPE_OBJECT);
    GtkWidget * dd = gtk_drop_down_new(G_LIST_MODEL(store), nullptr);
    GtkListItemFactory * factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(AppUi::setup_combo_item_cb), nullptr);
    g_signal_connect(factory, "bind", G_CALLBACK(AppUi::bind_combo_item_cb), nullptr);
    gtk_drop_down_set_factory(GTK_DROP_DOWN(dd), factory);
    g_object_unref(factory);
    gtk_widget_add_css_class(dd, "color");
    return dd;
}

static void appendColorItem(GListStore * store, GdkPixbuf * pixbuf, const String & text) {
    GObject * obj = (GObject *)g_object_new(G_TYPE_OBJECT, nullptr);
    if (pixbuf) g_object_set_data_full(obj, "ipe-pixbuf", pixbuf, g_object_unref);
    g_object_set_data_full(obj, "ipe-text", g_strdup(text.z()), g_free);
    g_list_store_append(store, obj);
    g_object_unref(obj);
}

static bool isColorSelector(int sel) {
    return sel == AppUiBase::EUiStroke || sel == AppUiBase::EUiFill;
}

String AppUi::selectorCurrentText(int sel) const {
    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(iSelector[sel]));
    if (idx == GTK_INVALID_LIST_POSITION) return String();
    GListModel * model =
	G_LIST_MODEL(gtk_drop_down_get_model(GTK_DROP_DOWN(iSelector[sel])));
    if (isColorSelector(sel)) {
	GObject * obj = G_OBJECT(g_list_model_get_item(model, idx));
	String text = (const char *)g_object_get_data(obj, "ipe-text");
	g_object_unref(obj);
	return text;
    }
    return gtk_string_list_get_string(GTK_STRING_LIST(model), idx);
}

void AppUi::combo_changed_cb(GObject * dropdown, GParamSpec *, gpointer data) {
    AppUi * self = (AppUi *)data;
    int sel = GPOINTER_TO_INT(g_object_get_data(dropdown, "ipe-sel"));
    self->comboSelector(sel);
}

void AppUi::comboSelector(int id) {
    luaSelector(String(selectorNames[id]), selectorCurrentText(id));
}

void AppUi::absoluteButton(int id) { luaAbsoluteButton(selectorNames[id]); }

void AppUi::absolute_button_cb(GtkWidget * b, gpointer data) {
    AppUi * self = (AppUi *)data;
    int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "ipe-id"));
    self->absoluteButton(id);
}

void AppUi::resetCombos() {
    for (int i = 0; i < EUiView; ++i) {
	g_signal_handlers_block_by_func(iSelector[i], (gpointer)combo_changed_cb, this);
	GListModel * model =
	    G_LIST_MODEL(gtk_drop_down_get_model(GTK_DROP_DOWN(iSelector[i])));
	guint n = g_list_model_get_n_items(model);
	if (isColorSelector(i))
	    g_list_store_remove_all(G_LIST_STORE(model));
	else
	    gtk_string_list_splice(GTK_STRING_LIST(model), 0, n, nullptr);
	g_signal_handlers_unblock_by_func(iSelector[i], (gpointer)combo_changed_cb, this);
    }
}

void AppUi::addCombo(int sel, String s) {
    g_signal_handlers_block_by_func(iSelector[sel], (gpointer)combo_changed_cb, this);
    GtkStringList * list =
	GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(iSelector[sel])));
    gtk_string_list_append(list, s.z());
    g_signal_handlers_unblock_by_func(iSelector[sel], (gpointer)combo_changed_cb, this);
    if (sel == EUiVariant)
	gtk_widget_set_visible(iVariantTools, iComboContents[EUiVariant].size() > 1);
    else if (sel == EUiGridSize)
	populateDynamicMenu(EDynGridSize);
    else if (sel == EUiAngleSize)
	populateDynamicMenu(EDynAngleSize);
}

void AppUi::addComboColors(AttributeSeq & sym, AttributeSeq & abs) {
    g_signal_handlers_block_by_func(iSelector[EUiStroke], (gpointer)combo_changed_cb,
				    this);
    g_signal_handlers_block_by_func(iSelector[EUiFill], (gpointer)combo_changed_cb, this);
    GListStore * strokeStore =
	G_LIST_STORE(gtk_drop_down_get_model(GTK_DROP_DOWN(iSelector[EUiStroke])));
    GListStore * fillStore =
	G_LIST_STORE(gtk_drop_down_get_model(GTK_DROP_DOWN(iSelector[EUiFill])));
    appendColorItem(strokeStore, nullptr, IPEABSOLUTE);
    appendColorItem(fillStore, nullptr, IPEABSOLUTE);
    iComboContents[EUiStroke].push_back(IPEABSOLUTE);
    iComboContents[EUiFill].push_back(IPEABSOLUTE);
    for (uint i = 0; i < sym.size(); ++i) {
	Color color = abs[i].color();
	String s = sym[i].string();
	guint32 pixel = (guint32(color.iRed.toDouble() * 255) << 24)
			| (guint32(color.iGreen.toDouble() * 255) << 16)
			| (guint32(color.iBlue.toDouble() * 255) << 8) | 0xff;
	GdkPixbuf * icon1 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 16, 16);
	gdk_pixbuf_fill(icon1, pixel);
	GdkPixbuf * icon2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 16, 16);
	gdk_pixbuf_fill(icon2, pixel);
	appendColorItem(strokeStore, icon1, s);
	appendColorItem(fillStore, icon2, s);
	iComboContents[EUiStroke].push_back(s);
	iComboContents[EUiFill].push_back(s);
    }
    g_signal_handlers_unblock_by_func(iSelector[EUiStroke], (gpointer)combo_changed_cb,
				      this);
    g_signal_handlers_unblock_by_func(iSelector[EUiFill], (gpointer)combo_changed_cb,
				      this);
}

void AppUi::setComboCurrent(int sel, int idx) {
    g_signal_handlers_block_by_func(iSelector[sel], (gpointer)combo_changed_cb, this);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(iSelector[sel]), idx);
    g_signal_handlers_unblock_by_func(iSelector[sel], (gpointer)combo_changed_cb, this);
    if (sel == EUiGridSize)
	populateDynamicMenu(EDynGridSize);
    else if (sel == EUiAngleSize)
	populateDynamicMenu(EDynAngleSize);
}

// --------------------------------------------------------------------

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm) {
    if (vno.empty()) {
	gtk_widget_set_visible(iViewNumber, FALSE);
	gtk_widget_set_visible(iViewMarked, FALSE);
    } else {
	gtk_button_set_label(GTK_BUTTON(iViewNumber), vno.z());
	gtk_widget_set_visible(iViewNumber, TRUE);
	g_signal_handlers_block_by_func(iViewMarked, (gpointer)absolute_button_cb, this);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(iViewMarked), vm);
	g_signal_handlers_unblock_by_func(iViewMarked, (gpointer)absolute_button_cb,
					  this);
	gtk_widget_set_visible(iViewMarked, TRUE);
    }
    if (pno.empty()) {
	gtk_widget_set_visible(iPageNumber, FALSE);
	gtk_widget_set_visible(iPageMarked, FALSE);
    } else {
	gtk_button_set_label(GTK_BUTTON(iPageNumber), pno.z());
	gtk_widget_set_visible(iPageNumber, TRUE);
	g_signal_handlers_block_by_func(iPageMarked, (gpointer)absolute_button_cb, this);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(iPageMarked), pm);
	g_signal_handlers_unblock_by_func(iPageMarked, (gpointer)absolute_button_cb,
					  this);
	gtk_widget_set_visible(iPageMarked, TRUE);
    }
}

void AppUi::setNotes(String notes) {
    GtkTextBuffer * buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(iPageNotes));
    gtk_text_buffer_set_text(buf, notes.z(), -1);
}

void AppUi::setLayers(const Page * page, int view) {
    iLayerList.set(page, view);
    populateDynamicMenu(EDynSelectLayer);
    populateDynamicMenu(EDynMoveLayer);
}

void AppUi::bookmark_row_activated_cb(GtkListBox *, GtkListBoxRow * row, gpointer data) {
    ((AppUi *)data)->bookmarkSelected(gtk_list_box_row_get_index(row));
}

void AppUi::bookmarkSelected(int index) { luaBookmarkSelected(index); }

void AppUi::setBookmarks(int no, const String * s) {
    GtkWidget * child;
    while ((child = gtk_widget_get_first_child(iBookmarks)) != nullptr)
	gtk_list_box_remove(GTK_LIST_BOX(iBookmarks), child);
    for (int i = 0; i < no; ++i) {
	GtkWidget * label = gtk_label_new(s[i].z());
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	if (s[i][0] == ' ') gtk_widget_add_css_class(label, "bookmark-marked");
	gtk_list_box_append(GTK_LIST_BOX(iBookmarks), label);
    }
}

void AppUi::setToolVisible(int m, bool vis) {
    GtkWidget * tool = nullptr;
    switch (m) {
    case 0: tool = iPropertiesTools; break;
    case 1: tool = iBookmarkTools; break;
    case 2: tool = iNotesTools; break;
    case 3: tool = iLayerTools; break;
    default: break;
    }
    if (tool) gtk_widget_set_visible(tool, vis);
}

void AppUi::setZoom(double zoom) {
    char s[32];
    sprintf(s, "(%dppi)", int(72.0 * zoom));
    iCanvas->setZoom(zoom);
    gtk_label_set_text(GTK_LABEL(iResolution), s);
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
    // avoid GtkMessageDialog (deprecated GTK4 shim): its icon area appears to
    // reflow after the first frame, which is what was causing the jump
    GtkWidget * dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "About Ipe");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(iWindow));

    GtkWidget * label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(label), buf.data());
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_margin_start(label, 20);
    gtk_widget_set_margin_end(label, 20);
    gtk_widget_set_margin_top(label, 20);
    gtk_widget_set_margin_bottom(label, 20);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "_OK", GTK_RESPONSE_OK);

    g_signal_connect(dialog, "response", G_CALLBACK(about_response_cb), nullptr);
    gtk_window_present(GTK_WINDOW(dialog));
}

void AppUi::action(String name) {
    if (name == "fullscreen") {
	if (gtk_window_is_fullscreen(GTK_WINDOW(iWindow)))
	    gtk_window_unfullscreen(GTK_WINDOW(iWindow));
	else
	    gtk_window_fullscreen(GTK_WINDOW(iWindow));
    } else if (name == "about") {
	aboutIpe();
    } else {
	if (name.left(5) == "mode_") {
	    GdkTexture * texture = textureFromPixbuf(prefsPixbuf(name, 22));
	    gtk_image_set_from_paintable(GTK_IMAGE(iModeIndicator),
					 GDK_PAINTABLE(texture));
	    gtk_image_set_pixel_size(GTK_IMAGE(iModeIndicator), 22);
	    g_object_unref(texture);
	}
	luaAction(name);
    }
}

// --------------------------------------------------------------------

void AppUi::showPathStylePopup(GtkWidget * /* relativeTo */, int x, int y) {
    // Lua builds a menu and calls back into ipeui, which positions
    // itself using windowId() as parent - the exact click point is
    // close enough without translation
    luaShowPathStylePopup(Vector(x, y));
}

void AppUi::showLayerBoxPopup(GtkWidget * relativeTo, int x, int y, String layer) {
    luaShowLayerBoxPopup(Vector(x, y), layer);
    (void)relativeTo;
}

void AppUi::layerAction(String name, String layer) { luaLayerAction(name, layer); }

// --------------------------------------------------------------------

namespace {
struct AsyncCtx {
    lua_State * L;
    int threadRef;
};

AsyncCtx * newAsyncCtx(lua_State * L) {
    AsyncCtx * ctx = new AsyncCtx();
    ctx->L = L;
    lua_pushthread(L);
    ctx->threadRef = luaL_ref(L, LUA_REGISTRYINDEX);
    return ctx;
}

void resumeAndFree(AsyncCtx * ctx, int nresults) {
    lua_resume(ctx->L, nullptr, nresults, &nresults);
    luaL_unref(ctx->L, LUA_REGISTRYINDEX, ctx->threadRef);
    delete ctx;
}

struct PageSorterCtx : AsyncCtx {
    GtkWidget * dialog;
    PageSorter * sorter;
};

void pagesorter_response_cb(GtkDialog *, int response, gpointer data) {
    PageSorterCtx * ctx = (PageSorterCtx *)data;
    lua_State * L = ctx->L;
    int nresults = 0;
    if (response == GTK_RESPONSE_OK) {
	int n = ctx->sorter->count();
	lua_createtable(L, n, 0);
	for (int i = 1; i <= n; ++i) {
	    lua_pushinteger(L, ctx->sorter->pageAt(i - 1) + 1);
	    lua_rawseti(L, -2, i);
	}
	int m = int(ctx->sorter->iMarks.size());
	lua_createtable(L, m, 0);
	for (int i = 1; i <= m; ++i) {
	    lua_pushboolean(L, ctx->sorter->iMarks[i - 1]);
	    lua_rawseti(L, -2, i);
	}
	nresults = 2;
    }
    GtkWidget * dialog = ctx->dialog;
    PageSorter * sorter = ctx->sorter;
    resumeAndFree(ctx, nresults);
    delete sorter;
    gtk_window_destroy(GTK_WINDOW(dialog));
}
} // namespace

int AppUi::pageSorter(lua_State * L, Document * doc, int pno, int width, int height,
		      int thumbWidth) {
    GtkWidget * dialog =
	gtk_dialog_new_with_buttons(pno >= 0 ? "Ipe View Sorter" : "Ipe Page Sorter",
				    GTK_WINDOW(iWindow), GTK_DIALOG_MODAL, "_Cancel",
				    GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_OK, nullptr);
    gtk_window_set_default_size(GTK_WINDOW(dialog), width, height);

    PageSorterCtx * ctx = new PageSorterCtx();
    ctx->L = L;
    lua_pushthread(L);
    ctx->threadRef = luaL_ref(L, LUA_REGISTRYINDEX);
    ctx->dialog = dialog;
    ctx->sorter = new PageSorter(doc, pno, thumbWidth);

    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_append(GTK_BOX(content), ctx->sorter->window());

    g_signal_connect(dialog, "response", G_CALLBACK(pagesorter_response_cb), ctx);
    gtk_window_present(GTK_WINDOW(dialog));
    return 0;
}

// --------------------------------------------------------------------

WINID AppUi::windowId() { return iWindow; }

gboolean AppUi::close_request_cb(GtkWindow *, gpointer data) {
    ((AppUi *)data)->closeWindow();
    return TRUE; // we handle closing ourselves (possibly deferred)
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

void AppUi::explain(const char * s, int) { gtk_label_set_text(GTK_LABEL(iStatusBar), s); }

void AppUi::setWindowCaption(bool, const char * caption, const char *) {
    gtk_window_set_title(GTK_WINDOW(iWindow), caption);
}

void AppUi::setMouseIndicator(const char * s) {
    gtk_label_set_text(GTK_LABEL(iMousePosition), s);
}

void AppUi::setSnapIndicator(const char * s) {
    gtk_label_set_text(GTK_LABEL(iSnapIndicator), s);
}

void AppUi::showWindow(int width, int height, int, int, const Color & pathViewColor) {
    iPathView.setColor(pathViewColor);
    // window positioning (x, y) is not possible on GTK4/Wayland - dropped
    if (width > 0 && height > 0)
	gtk_window_set_default_size(GTK_WINDOW(iWindow), width, height);
    gtk_window_present(GTK_WINDOW(iWindow));
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
    GdkClipboard * cb = gtk_widget_get_clipboard(iWindow);
    gdk_clipboard_set_text(cb, data);
    return 0;
}

namespace {
void clipboard_read_cb(GObject * source, GAsyncResult * result, gpointer data) {
    AsyncCtx * ctx = (AsyncCtx *)data;
    GError * error = nullptr;
    char * text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, &error);
    int nresults = 1;
    if (text) {
	lua_pushstring(ctx->L, text);
	g_free(text);
    } else {
	if (error) g_error_free(error);
	lua_pushnil(ctx->L);
    }
    resumeAndFree(ctx, nresults);
}
} // namespace

int AppUi::clipboard(lua_State * L) {
    // bitmap clipboard access is not implemented for GTK
    GdkClipboard * cb = gtk_widget_get_clipboard(iWindow);
    gdk_clipboard_read_text_async(cb, nullptr, clipboard_read_cb, newAsyncCtx(L));
    return 0;
}

// --------------------------------------------------------------------

namespace {
struct WaitCtx {
    lua_State * thread;
    GtkWidget * dialog;
    GtkWidget * appWindow;
    bool shown = false;
    bool completed = false;
};

void waitdialog_child_watch_cb(GPid pid, gint, gpointer data) {
    WaitCtx * ctx = (WaitCtx *)data;
    g_spawn_close_pid(pid);
    ctx->completed = true;
    if (ctx->shown) {
	lua_State * co = ctx->thread;
	gtk_window_destroy(GTK_WINDOW(ctx->dialog));
	gtk_widget_set_sensitive(GTK_WIDGET(ctx->appWindow), TRUE);
	delete ctx;
	resumeLuaThread(co, 0);
	// TODO: unref thread
    }
}
} // namespace

bool AppUi::waitDialog(lua_State * co, const char * cmd, const char * label) {
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
    ctx->thread = co;
    ctx->appWindow = iWindow;
    // TODO: ref it

    GtkWidget * dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Ipe: waiting");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(iWindow));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);
    GtkWidget * l = gtk_label_new(label);
    gtk_widget_set_margin_start(l, 12);
    gtk_widget_set_margin_end(l, 12);
    gtk_widget_set_margin_top(l, 12);
    gtk_widget_set_margin_bottom(l, 12);
    gtk_window_set_child(GTK_WINDOW(dialog), l);
    ctx->dialog = dialog;

    g_child_watch_add(pid, waitdialog_child_watch_cb, ctx);

    // block input to the main window: the dialog is not shown yet, so it would
    // not otherwise stop the user from triggering another action (such as
    // editing the same text object) while the Latex conversion is in progress
    gtk_widget_set_sensitive(iWindow, FALSE);
    for (int i = 0; i < 30 && !ctx->completed; ++i) {
	g_usleep(10000);
	while (g_main_context_pending(nullptr)) g_main_context_iteration(nullptr, FALSE);
    }

    if (ctx->completed) {
	gtk_window_destroy(GTK_WINDOW(dialog));
	gtk_widget_set_sensitive(iWindow, TRUE);
	delete ctx;
	return true;
    }
    ctx->shown = true;
    gtk_window_present(GTK_WINDOW(dialog));
    return false;
}

// --------------------------------------------------------------------

AppUi::AppUi(lua_State * L0, int model)
    : AppUiBase(L0, model)
    , iNextActionId(0)
    , iLayerList()
    , iPathView(iUiScale) {
    iWindow = gtk_application_window_new(ipeApp);
    g_signal_connect(iWindow, "close-request", G_CALLBACK(close_request_cb), this);

    iSnapTools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    iVariantTools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    iEditTools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    iObjectTools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    iSelectLayerAction = g_simple_action_new("selectlayer", G_VARIANT_TYPE_STRING);
    g_object_set_data_full(G_OBJECT(iSelectLayerAction), "ipe-prefix",
			   g_strdup("selectinlayer-"), g_free);
    g_signal_connect(iSelectLayerAction, "activate", G_CALLBACK(layer_menu_item_cb),
		     this);
    g_action_map_add_action(G_ACTION_MAP(iWindow), G_ACTION(iSelectLayerAction));

    iMoveLayerAction = g_simple_action_new("movelayer", G_VARIANT_TYPE_STRING);
    g_object_set_data_full(G_OBJECT(iMoveLayerAction), "ipe-prefix",
			   g_strdup("movetolayer-"), g_free);
    g_signal_connect(iMoveLayerAction, "activate", G_CALLBACK(layer_menu_item_cb), this);
    g_action_map_add_action(G_ACTION_MAP(iWindow), G_ACTION(iMoveLayerAction));

    iRecentFileAction = g_simple_action_new("recentfile", G_VARIANT_TYPE_STRING);
    g_signal_connect(iRecentFileAction, "activate", G_CALLBACK(recent_file_cb), this);
    g_action_map_add_action(G_ACTION_MAP(iWindow), G_ACTION(iRecentFileAction));

    lua_getglobal(L, "prefs");
    lua_getfield(L, -1, "visual_css");
    if (lua_isstring(L, -1)) {
	GtkCssProvider * cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_string(cssProvider, lua_tolstring(L, -1, nullptr));
	gtk_style_context_add_provider_for_display(
	    gdk_display_get_default(), GTK_STYLE_PROVIDER(cssProvider),
	    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(cssProvider);
    }
    lua_pop(L, 2);

    buildMenus();
    for (int i = 0; i < ENumMenu; ++i) flushSection(i);

    GMenu * menuBarModel = g_menu_new();
    for (int i = 0; i < ENumMenu; ++i)
	g_menu_append_submenu(menuBarModel, change_mnemonic(iRootMenuTitle[i].z()).z(),
			      G_MENU_MODEL(iSubMenu[i]));
    GtkWidget * menuBar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menuBarModel));
    g_object_unref(menuBarModel);

    GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(iWindow), vbox);
    gtk_box_append(GTK_BOX(vbox), menuBar);

    /*
    // The following was meant to better deal with the case where the window
    // is not wide enough for the toolbar, but it doesn't work very well:
    // a flow box wraps the snap/variant/edit/mode toolbars onto more lines
    // instead of clipping them when the window is too narrow; each toolbar
    // keeps its natural size; leftover space stays at the end of the line
    // rather than being distributed between the toolbars
    GtkWidget * row1 = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(row1), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(row1), FALSE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(row1), 4);
    // gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(row1), 16);
    gtk_widget_set_margin_start(row1, 4);
    gtk_widget_set_margin_end(row1, 4);
    gtk_widget_set_margin_top(row1, 4);
    gtk_widget_set_margin_bottom(row1, 4);
    for (GtkWidget * toolbar : {iSnapTools, iVariantTools, iEditTools, iObjectTools}) {
	// gtk_widget_set_hexpand(toolbar, FALSE);
	// gtk_widget_set_halign(toolbar, GTK_ALIGN_START);
	gtk_flow_box_append(GTK_FLOW_BOX(row1), toolbar);
    }
    gtk_box_append(GTK_BOX(vbox), row1);
    */
    GtkWidget * row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_box_append(GTK_BOX(row1), iSnapTools);
    gtk_box_append(GTK_BOX(row1), iVariantTools);
    gtk_box_append(GTK_BOX(row1), iEditTools);
    gtk_widget_set_margin_start(row1, 4);
    gtk_widget_set_margin_end(row1, 4);
    gtk_widget_set_margin_bottom(row1, 4);
    gtk_box_append(GTK_BOX(vbox), row1);

    GtkWidget * row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(row2), iObjectTools);
    gtk_widget_set_margin_start(row2, 4);
    gtk_widget_set_margin_end(row2, 4);
    gtk_widget_set_margin_bottom(row2, 4);
    gtk_box_append(GTK_BOX(vbox), row2);

    addSnap("snapvtx");
    addSnap("snapctl");
    addSnap("snapbd");
    addSnap("snapint");
    addSnap("snapgrid");
    iSelector[EUiGridSize] = newTextDropDown("bar");
    gtk_box_append(GTK_BOX(iSnapTools), iSelector[EUiGridSize]);
    addSnap("snapangle");
    iSelector[EUiAngleSize] = newTextDropDown("bar");
    gtk_box_append(GTK_BOX(iSnapTools), iSelector[EUiAngleSize]);
    addSnap("snapcustom");
    addSnap("snapauto");

    iSelector[EUiVariant] = newTextDropDown("bar");
    gtk_box_append(GTK_BOX(iVariantTools), iSelector[EUiVariant]);

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

    iShiftKey = gtk_toggle_button_new();
    setButtonIcon(iShiftKey, "shift_key", 22);
    gtk_widget_set_tooltip_text(iShiftKey, "Shift key");
    g_signal_connect(iShiftKey, "clicked", G_CALLBACK(shift_key_cb), this);
    gtk_box_append(GTK_BOX(iEditTools), iShiftKey);

    iAbortButton = gtk_button_new();
    setButtonIcon(iAbortButton, "stop", 22);
    gtk_widget_set_tooltip_text(iAbortButton, "Stop current operation");
    g_signal_connect(iAbortButton, "clicked", G_CALLBACK(abort_cb), this);
    gtk_box_append(GTK_BOX(iEditTools), iAbortButton);

    for (int i = 0; i < EUiView; ++i) {
	if (i != EUiGridSize && i != EUiAngleSize && i != EUiVariant)
	    iSelector[i] =
		isColorSelector(i) ? newColorDropDown() : newTextDropDown("properties");
	g_object_set_data(G_OBJECT(iSelector[i]), "ipe-sel", GINT_TO_POINTER(i));
	g_signal_connect(iSelector[i], "notify::selected", G_CALLBACK(combo_changed_cb),
			 this);
    }

    GtkWidget * grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 1);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 2);
    gtk_widget_set_margin_start(grid, 2);
    gtk_widget_set_margin_end(grid, 2);
    gtk_widget_set_margin_top(grid, 2);
    gtk_widget_set_margin_bottom(grid, 2);

    iButton[EUiDashStyle] = nullptr;
    iButton[EUiMarkShape] = nullptr;
    for (int i = 0; i <= EUiSymbolSize; ++i) {
	if (i == EUiDashStyle || i == EUiMarkShape) continue;
	iButton[i] = gtk_button_new();
	gtk_widget_set_hexpand(iButton[i], FALSE);
	g_object_set_data(G_OBJECT(iButton[i]), "ipe-id", GINT_TO_POINTER(i));
	g_signal_connect(iButton[i], "clicked", G_CALLBACK(absolute_button_cb), this);
	int row = (i == EUiTextSize) ? i + 1 : i;
	int rowspan = (i == EUiPen || i == EUiSymbolSize) ? 2 : 1;
	gtk_grid_attach(GTK_GRID(grid), iButton[i], 0, row, 1, rowspan);
    }
    setButtonColorIcon(iButton[EUiStroke], Color(1000, 0, 0), 16);
    setButtonColorIcon(iButton[EUiFill], Color(1000, 1000, 0), 16);
    setButtonIcon(iButton[EUiPen], "pen", 16, "absolute");
    setButtonIcon(iButton[EUiTextSize], "mode_label", 16, "absolute");
    setButtonIcon(iButton[EUiSymbolSize], "mode_marks", 16, "absolute");
    gtk_widget_set_tooltip_text(iButton[EUiStroke], "Absolute stroke color");
    gtk_widget_set_tooltip_text(iButton[EUiFill], "Absolute fill color");
    gtk_widget_set_tooltip_text(iButton[EUiPen], "Absolute pen width");
    gtk_widget_set_tooltip_text(iButton[EUiTextSize], "Absolute text size");
    gtk_widget_set_tooltip_text(iButton[EUiSymbolSize], "Absolute symbol size");

    for (int i = 0; i < EUiGridSize; ++i) {
	int row = (i == EUiOpacity) ? i + 1 : ((i >= EUiTextSize) ? i + 1 : i);
	int col = (i == EUiOpacity) ? 0 : 1;
	int colspan = (i == EUiOpacity) ? 2 : 1;
	gtk_widget_set_hexpand(iSelector[i], TRUE);
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
    gtk_widget_set_hexpand(iModeIndicator, FALSE);
    gtk_grid_attach(GTK_GRID(grid), iModeIndicator, 0, 4, 1, 1);
    gtk_widget_set_hexpand(iPathView.window(), TRUE);
    gtk_grid_attach(GTK_GRID(grid), iPathView.window(), 1, 4, 1, 1);
    iPathView.activated = [this](String s) { action(s); };
    iPathView.showPathStylePopup = [this](GtkWidget * w, int x, int y) {
	showPathStylePopup(w, x, y);
    };

    GtkWidget * hol = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    iViewMarked = gtk_check_button_new();
    iPageMarked = gtk_check_button_new();
    iViewNumber = gtk_button_new_with_label("View 1/1");
    iPageNumber = gtk_button_new_with_label("Page 1/1");
    gtk_widget_set_tooltip_text(iViewNumber, "Current view number");
    gtk_widget_set_tooltip_text(iPageNumber, "Current page number");
    g_object_set_data(G_OBJECT(iViewNumber), "ipe-id", GINT_TO_POINTER(int(EUiView)));
    g_object_set_data(G_OBJECT(iPageNumber), "ipe-id", GINT_TO_POINTER(int(EUiPage)));
    g_object_set_data(G_OBJECT(iViewMarked), "ipe-id",
		      GINT_TO_POINTER(int(EUiViewMarked)));
    g_object_set_data(G_OBJECT(iPageMarked), "ipe-id",
		      GINT_TO_POINTER(int(EUiPageMarked)));
    g_signal_connect(iViewNumber, "clicked", G_CALLBACK(absolute_button_cb), this);
    g_signal_connect(iPageNumber, "clicked", G_CALLBACK(absolute_button_cb), this);
    g_signal_connect(iViewMarked, "toggled", G_CALLBACK(absolute_button_cb), this);
    g_signal_connect(iPageMarked, "toggled", G_CALLBACK(absolute_button_cb), this);
    gtk_box_append(GTK_BOX(hol), iViewMarked);
    gtk_box_append(GTK_BOX(hol), iViewNumber);
    GtkWidget * spacer = gtk_label_new(nullptr);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(hol), spacer);
    gtk_box_append(GTK_BOX(hol), iPageMarked);
    gtk_box_append(GTK_BOX(hol), iPageNumber);
    gtk_grid_attach(GTK_GRID(grid), hol, 0, EUiOpacity + 2, 2, 1);

    iPropertiesTools = makeFramed("Properties", grid);

    iLayerTools = makeFramed("Layers", iLayerList.window());
    iLayerList.activated = [this](String a, String b) { layerAction(a, b); };
    iLayerList.showLayerBoxPopup = [this](GtkWidget * w, int x, int y, String l) {
	showLayerBoxPopup(w, x, y, l);
    };

    iBookmarks = gtk_list_box_new();
    g_signal_connect(iBookmarks, "row-activated", G_CALLBACK(bookmark_row_activated_cb),
		     this);
    GtkWidget * bookmarkScroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(bookmarkScroller),
				   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(bookmarkScroller), iBookmarks);
    iBookmarkTools = makeFramed("Bookmarks", bookmarkScroller);

    iPageNotes = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(iPageNotes), FALSE);
    GtkWidget * notesScroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notesScroller), iPageNotes);
    iNotesTools = makeFramed("Notes", notesScroller);

    GtkWidget * leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(leftBox, 4);
    gtk_widget_set_margin_end(leftBox, 4);
    gtk_box_append(GTK_BOX(leftBox), iPropertiesTools);
    gtk_box_append(GTK_BOX(leftBox), iLayerTools);
    gtk_widget_set_vexpand(iLayerTools, TRUE);

    GtkWidget * rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(rightBox, 4);
    gtk_widget_set_margin_end(rightBox, 4);
    gtk_box_append(GTK_BOX(rightBox), iBookmarkTools);
    gtk_box_append(GTK_BOX(rightBox), iNotesTools);
    gtk_widget_set_vexpand(iBookmarkTools, TRUE);
    gtk_widget_set_vexpand(iNotesTools, TRUE);

    Canvas * canvas = new Canvas(iWindow);
    iCanvas = canvas;
    gtk_widget_set_hexpand(canvas->window(), TRUE);
    gtk_widget_set_vexpand(canvas->window(), TRUE);

    GtkWidget * innerPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(innerPaned), canvas->window());
    gtk_paned_set_resize_start_child(GTK_PANED(innerPaned), TRUE);
    gtk_paned_set_end_child(GTK_PANED(innerPaned), rightBox);
    gtk_paned_set_resize_end_child(GTK_PANED(innerPaned), FALSE);

    GtkWidget * outerPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(outerPaned), leftBox);
    gtk_paned_set_resize_start_child(GTK_PANED(outerPaned), FALSE);
    gtk_paned_set_end_child(GTK_PANED(outerPaned), innerPaned);
    gtk_paned_set_resize_end_child(GTK_PANED(outerPaned), TRUE);
    gtk_widget_set_vexpand(outerPaned, TRUE);

    gtk_box_append(GTK_BOX(vbox), outerPaned);

    GtkWidget * statusBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(statusBox, 4);
    gtk_widget_set_margin_end(statusBox, 4);
    gtk_widget_set_margin_top(statusBox, 4);
    gtk_widget_set_margin_bottom(statusBox, 4);
    iStatusBar = gtk_label_new(nullptr);
    gtk_label_set_xalign(GTK_LABEL(iStatusBar), 0.0);
    gtk_widget_set_hexpand(iStatusBar, TRUE);
    iMousePosition = gtk_label_new(nullptr);
    iSnapIndicator = gtk_label_new(nullptr);
    iResolution = gtk_label_new(nullptr);
    gtk_box_append(GTK_BOX(statusBox), iStatusBar);
    gtk_box_append(GTK_BOX(statusBox), iSnapIndicator);
    gtk_box_append(GTK_BOX(statusBox), iMousePosition);
    gtk_box_append(GTK_BOX(statusBox), iResolution);
    gtk_box_append(GTK_BOX(vbox), statusBox);

    gtk_widget_set_visible(iVariantTools, FALSE);
    // shown once addCombo(EUiVariant,...) runs

    iCanvas->setObserver(this);
}

AppUi::~AppUi() {
    ipeDebug("AppUi C++ destructor");
    gtk_window_destroy(GTK_WINDOW(iWindow));
}

// --------------------------------------------------------------------

AppUiBase * createAppUi(lua_State * L0, int model) { return new AppUi(L0, model); }

// --------------------------------------------------------------------
