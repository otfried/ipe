// -*- objc -*-
// --------------------------------------------------------------------
// Appui for Cocoa
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

#ifndef APPUI_COCOA_H
#define APPUI_COCOA_H

#include "appui.h"

using namespace ipe;

// --------------------------------------------------------------------

@class IpeCanvasView;
@class IpeWindowDelegate;
@class IpeAction;
@class IpePathView;
@class IpeLayerView;
@class IpeBookmarksView;

class AppUi : public AppUiBase {
public:
    AppUi(lua_State * L0, int model);
    ~AppUi();

    virtual void action(String name) override;
    virtual void setLayers(const Page * page, int view) override;

    virtual void setZoom(double zoom) override;
    virtual void setActionsEnabled(bool mode) override;
    virtual void setNumbers(String vno, bool vm, String pno, bool pm) override;
    virtual void setNotes(String notes) override;

    virtual WINID windowId() override;
    virtual void closeWindow() override;
    virtual bool actionState(const char * name) override;
    virtual void setActionState(const char * name, bool value) override;
    virtual void setWindowCaption(bool mod, const char * caption,
				  const char * fn) override;
    virtual void explain(const char * s, int t) override;
    virtual void showWindow(int width, int height, int x, int y,
			    const Color & pathViewColor) override;
    virtual void setFullScreen(int mode) override;
    virtual void setRecentFileMenu(const std::vector<String> & names) override;

    virtual void setBookmarks(int no, const String * s) override;
    virtual void setToolVisible(int m, bool vis) override;
    virtual int pageSorter(lua_State * L, Document * doc, int pno, int width, int height,
			   int thumbWidth) override;

    virtual int clipboard(lua_State * L) override;
    virtual int setClipboard(lua_State * L) override;

    IpeAction * findAction(NSString * name) const;
    bool actionsEnabled() const { return iActionsEnabled; }
    BOOL isModified();
    BOOL closeRequested();
    void absoluteButton(int sel);
    void snapButton(int sel);
    void selectorChanged(int sel);
    void layerMenu(NSPoint p, NSString * layer);
    void layerAction(NSString * actionName, NSString * layer);
    void layerToggle(NSString * layer);
    BOOL validateMenuItem(NSMenuItem * item, NSString * name);
    void toggleSnapbarShown();
    void togglePropertiesShown();
    void fillDynamicSubmenu(NSMenuItem * item);
    NSImage * loadIcon(String action, bool touchBar = false, int size = 22);
    bool waitDialog(const char * cmd, const char * label) override;

private:
    void createAction(String name, String tooltip, bool canWhileDrawing, bool toggles);
    void addItem(NSMenu * menu, const char * title, const char * name);
    void setCheckMark(String name, String value);
    void makePropertiesTool();
    void makeSnapBar();
    NSImage * createIcon(int pno, int size, bool touchBar);

    virtual void addRootMenu(int id, const char * name) override;
    virtual void addItem(int id, const char * title, const char * name) override;
    virtual void startSubMenu(int id, const char * name, int tag) override;
    virtual void addSubItem(const char * title, const char * name) override;
    virtual MENUHANDLE endSubMenu() override;
    virtual void setSnapIndicator(const char * s) override;
    virtual void setMouseIndicator(const char * s) override;
    virtual void addCombo(int sel, String s) override;
    virtual void resetCombos() override;
    virtual void addComboColors(AttributeSeq & sym, AttributeSeq & abs) override;
    virtual void setComboCurrent(int sel, int idx) override;
    virtual void setCheckMark(String name, Attribute a) override;
    virtual void setPathView(const AllAttributes & all, Cascade * sheet) override;
    virtual void setButtonColor(int sel, Color color) override;

private:
    static const int NUMSNAPBUTTONS = 9;

    NSMutableDictionary<NSString *, IpeAction *> * iActions;
    bool iActionsEnabled;
    bool iInUiUpdate;
    NSWindow * iWindow;
    IpeWindowDelegate * iDelegate;
    NSView * iContent;
    NSBox * iPropertiesBox;
    NSBox * iLayerBox;
    IpeCanvasView * iView;
    NSTimer * iIndicatorTimer;
    NSTextField * iStatus;
    NSTextField * iSnapIndicator;
    NSTextField * iMouseIndicator;
    NSTextField * iZoomIndicator;
    IpePathView * iPathView;
    IpeLayerView * iLayerView;
    std::vector<String> iLayerNames;
    std::vector<String> iRecentFiles;

    NSButton * iButton[EUiOpacity]; // EUiDashStyle and EUiMarkShape are not used
    NSPopUpButton * iSelector[EUiView];

    NSImageView * iModeIndicator;
    NSButton * iViewNumber;
    NSButton * iPageNumber;
    NSButton * iViewMarked;
    NSButton * iPageMarked;

    NSView * iSnapBar;
    NSButton * iSnapButton[NUMSNAPBUTTONS];
    NSLayoutConstraint * iViewToTop;
    NSLayoutConstraint * iViewToSnapBar;
    NSLayoutConstraint * iViewToProperties;
    NSLayoutConstraint * iViewToLayers;
    NSLayoutConstraint * iViewToLeft;

    NSPanel * iNotesPanel;
    NSTextView * iNotesField;
    NSPanel * iBookmarksPanel;
    IpeBookmarksView * iBookmarksView;
};

// --------------------------------------------------------------------
#endif
