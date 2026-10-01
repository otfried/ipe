// -*- C++ -*-
// --------------------------------------------------------------------
// Appui for GTK
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

#ifndef APPUI_GTK_H
#define APPUI_GTK_H

#include "appui.h"
#include "controls_gtk.h"

#include <map>

using namespace ipe;

// the single GtkApplication instance, created in main_gtk.cpp
extern GtkApplication * ipeApp;

// --------------------------------------------------------------------

class AppUi : public AppUiBase {
public:
    AppUi(lua_State * L0, int model);
    ~AppUi();

    virtual void action(String name);
    virtual void setLayers(const Page * page, int view);

    virtual void setZoom(double zoom);
    virtual void setActionsEnabled(bool mode);
    virtual void setNumbers(String vno, bool vm, String pno, bool pm);
    virtual void setNotes(String notes);

    virtual WINID windowId();
    virtual void closeWindow();
    virtual bool actionState(const char * name);
    virtual void setActionState(const char * name, bool value);
    virtual void setWindowCaption(bool mod, const char * caption, const char * fn);
    virtual void explain(const char * s, int t);
    virtual void showWindow(int width, int height, const Color & pathViewColor);
    virtual void setFullScreen(int mode);

    virtual void setBookmarks(int no, const String * s);
    virtual void setToolVisible(int m, bool vis);
    virtual int pageSorter(lua_State * L, Document * doc, int pno, int width, int height,
			   int thumbWidth);

    virtual int clipboard(lua_State * L);
    virtual int setClipboard(lua_State * L);

    virtual void setRecentFileMenu(const std::vector<String> & names) override;

    virtual bool waitDialog(lua_State * co, const char * cmd,
			    const char * label) override;

    // used by free-function helpers that build GtkDropDown widgets
    static void setup_combo_item_cb(GtkListItemFactory *, GtkListItem * item, gpointer);
    static void bind_combo_item_cb(GtkListItemFactory *, GtkListItem * item, gpointer);

private:
    // -------------------- menu / action building --------------------
    virtual void addRootMenu(int id, const char * name);
    virtual void addItem(int id, const char * title, const char * name);
    virtual void startSubMenu(int id, const char * name, int tag);
    virtual void addSubItem(const char * title, const char * name);
    virtual MENUHANDLE endSubMenu();
    virtual void setCheckMark(String name, Attribute a);

    struct SAction {
	String name;  // full ipe action name
	String title; // human-readable label, for toolbar tooltips
	GSimpleAction * action = nullptr;
	GtkWidget * toolButton = nullptr; // set if a toolbar button mirrors this action
    };
    int actionId(const char * name) const;
    // create a plain or boolean-stateful action for a single menu item
    void addAction(const char * title, const char * name, GMenu * section);
    // dispatch to addAction() or to a shared radio group, based on the
    // "@"/"*"/"prefix|value" naming conventions used throughout buildMenus()
    void addMenuEntry(GMenu * section, const char * title, const char * name,
		      bool isModeGroup);
    // create/reuse the shared radio (string-stateful) action for a group
    // of items sharing a "prefix|value" naming convention (or the mode
    // menu, whose members use "bare" full names as the target directly)
    GSimpleAction * radioAction(const String & prefix, bool bare);
    void setActionAccelerator(const String & detailedName, const char * shortcutName);
    String opaqueName();
    void flushSection(int id);

    static void action_activated_cb(GSimpleAction * action, GVariant * parameter,
				    gpointer data);
    static void radio_activated_cb(GSimpleAction * action, GVariant * parameter,
				   gpointer data);
    static void layer_menu_item_cb(GSimpleAction * action, GVariant * parameter,
				   gpointer data);
    void populateDynamicMenu(int kind);

private:
    // -------------------- toolbars --------------------
    GtkWidget * addToolButton(GtkWidget * toolbar, const char * name,
			      const char * title = nullptr);
    void addSnap(const char * name);
    void addEdit(const char * name);
    static void toolbutton_clicked_cb(GtkWidget * b, gpointer data);
    static void shift_key_cb(GtkWidget * b, gpointer data);
    static void abort_cb(GtkWidget * b, gpointer data);

private:
    // -------------------- combo boxes (GtkDropDown) --------------------
    virtual void addCombo(int sel, String s);
    virtual void resetCombos();
    virtual void addComboColors(AttributeSeq & sym, AttributeSeq & abs);
    virtual void setComboCurrent(int sel, int idx);
    String selectorCurrentText(int sel) const;
    void absoluteButton(int id);
    void comboSelector(int id);
    static void combo_changed_cb(GObject * dropdown, GParamSpec *, gpointer data);
    static void absolute_button_cb(GtkWidget * b, gpointer data);

private:
    // -------------------- icons --------------------
    GdkPixbuf * prefsPixbuf(String name, int size);
    void setButtonIcon(GtkWidget * button, String name, int size,
		       const char * cssName = nullptr);
    void setButtonColorIcon(GtkWidget * button, Color color, int size);
    virtual void setButtonColor(int sel, Color color);
    virtual void setPathView(const AllAttributes & all, Cascade * sheet);

private:
    // -------------------- misc UI callbacks --------------------
    void aboutIpe();
    void showPathStylePopup(GtkWidget * relativeTo, int x, int y);
    void showLayerBoxPopup(GtkWidget * relativeTo, int x, int y, String layer);
    void layerAction(String name, String layer);
    void bookmarkSelected(int index);
    void recentFileSelected(String name);
    virtual void setMouseIndicator(const char * s);
    virtual void setSnapIndicator(const char * s);

    static void bookmark_row_activated_cb(GtkListBox * box, GtkListBoxRow * row,
					  gpointer data);
    static void recent_file_cb(GSimpleAction *, GVariant *, gpointer data);
    static gboolean close_request_cb(GtkWindow * w, gpointer data);

private:
    GtkWidget * iWindow;

    // one GMenu per top-level menu, plus the section currently being filled
    // (GMenu has no separator concept, only nested "sections")
    GMenu * iSubMenu[ENumMenu];
    GMenu * iCurrentSection[ENumMenu];
    String iRootMenuTitle[ENumMenu];

    std::vector<SAction> iActions;                  // plain + boolean-stateful actions
    std::map<String, GSimpleAction *> iRadioGroups; // keyed by "prefix" (or "mode")
    GSimpleAction * iSelectLayerAction;
    GSimpleAction * iMoveLayerAction;
    GSimpleAction * iRecentFileAction;
    int iNextActionId;

    // transient state while building a submenu (startSubMenu/addSubItem/endSubMenu)
    GMenu * iSubMenuBuilding;
    String iSubMenuBuildingTitle;
    int iSubMenuParentId;

    GtkWidget * iStatusBar;
    GtkWidget * iMousePosition;
    GtkWidget * iSnapIndicator;
    GtkWidget * iResolution;

    GtkWidget * iSnapTools;
    GtkWidget * iVariantTools;
    GtkWidget * iEditTools;
    GtkWidget * iObjectTools;

    GtkWidget * iPropertiesTools;
    GtkWidget * iLayerTools;
    GtkWidget * iBookmarkTools;
    GtkWidget * iNotesTools;

    GtkWidget * iButton[EUiOpacity]; // color/pen/textsize/symbolsize absolute buttons
    GtkWidget * iSelector[EUiView];  // GtkDropDown

    GtkWidget * iViewNumber;
    GtkWidget * iPageNumber;
    GtkWidget * iViewMarked;
    GtkWidget * iPageMarked;

    GtkWidget * iShiftKey; // GtkToggleButton
    GtkWidget * iAbortButton;

    GtkWidget * iModeIndicator; // GtkImage

    GtkWidget * iBookmarks; // GtkListBox
    LayerBox iLayerList;
    PathView iPathView;
    GtkWidget * iPageNotes; // GtkTextView

    std::map<String, GdkPixbuf *> iIconCache;
};

// --------------------------------------------------------------------
#endif
