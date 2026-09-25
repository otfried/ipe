// -*- C++ -*-
// --------------------------------------------------------------------
// Special widgets for GTK
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

#ifndef CONTROLS_GTK_H
#define CONTROLS_GTK_H

#include "ipelib.h"

#include <functional>
#include <gtk/gtk.h>

using namespace ipe;

// --------------------------------------------------------------------

// Convert a Cairo ARGB32 image buffer (as returned by Thumbnail::render)
// into a GdkPixbuf.  Transfers ownership of the returned pixbuf to the
// caller (g_object_unref when done).
GdkPixbuf * pixbufFromArgb32(const uint8_t * data, int w, int h, int stride);

// Wrap a GdkPixbuf as a GdkTexture, the paintable GtkImage now wants.
GdkTexture * textureFromPixbuf(GdkPixbuf * pixbuf);

// Position a popover so that it points at (x, y) in the coordinate space
// of "relativeTo", regardless of which widget the popover is parented to.
void popupPointingAt(GtkWidget * popover, GtkWidget * parent, GtkWidget * relativeTo,
		     int x, int y);

// --------------------------------------------------------------------

// Displays the layers of a page, one row per layer, with a checkbox
// to toggle visibility.  Clicking on the name makes the layer active.
// Right-click brings up a context menu (through a callback).
class LayerBox {
public:
    LayerBox();
    ~LayerBox();

    GtkWidget * window() const { return iScroller; }

    void set(const Page * page, int view);
    std::vector<String> layers() const;

    // called with ("selecton"/"selectoff"/"active", layername)
    std::function<void(String, String)> activated;
    // called with (widget, x, y, layername), (x, y) relative to widget
    std::function<void(GtkWidget *, int, int, String)> showLayerBoxPopup;

private:
    struct SRow {
	String layer;
	GtkWidget * check;
	GtkWidget * label;
	GtkWidget * row;
    };

    static void check_toggled_cb(GtkCheckButton * b, gpointer data);
    static void label_click_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			       gpointer data);
    static void row_popup_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			     gpointer data);

private:
    GtkWidget * iScroller;
    GtkWidget * iList; // GtkListBox
    std::vector<SRow> iRows;
    bool iInSet;
};

// --------------------------------------------------------------------

// Shows the current path style (stroke/fill/pen/dash/arrows) as a
// small drawing, similar to the Qt PathView widget.
class PathView {
public:
    PathView(int uiScale);
    ~PathView();

    GtkWidget * window() const { return iDrawingArea; }

    void setColor(const Color & color);
    void set(const AllAttributes & all, Cascade * sheet);

    // called with e.g. "farrow|true", "pathmode|filled", ...
    std::function<void(String)> activated;
    // called with (widget, x, y), (x, y) relative to widget
    std::function<void(GtkWidget *, int, int)> showPathStylePopup;

private:
    static void draw_cb(GtkDrawingArea * area, cairo_t * cr, int width, int height,
			gpointer data);
    static void click_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			 gpointer data);
    static void popup_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			 gpointer data);
    static gboolean query_tooltip_cb(GtkWidget * widget, gint x, gint y,
				     gboolean keyboard_mode, GtkTooltip * tooltip,
				     gpointer data);
    void draw(cairo_t * cc, int w, int h);

private:
    GtkWidget * iDrawingArea;
    int iUiScale;
    Cascade * iCascade;
    AllAttributes iAll;
    Color iColor;
};

// --------------------------------------------------------------------

// A grid of thumbnails for sorting/marking/deleting pages or views,
// similar to the Qt PageSorter widget.  GtkIconView is gone in GTK4, so
// this is built from GtkGridView + GListModel.
class PageSorter {
public:
    PageSorter(Document * doc, int pno, int width);
    ~PageSorter();

    GtkWidget * window() const { return iScroller; }

    int count() const;
    int pageAt(int r) const;
    void deletePages();
    void markPages(bool mark);

    std::vector<bool> iMarks;

private:
    static void popup_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			 gpointer data);
    static void setup_cb(GtkListItemFactory *, GtkListItem * item, gpointer data);
    static void bind_cb(GtkListItemFactory *, GtkListItem * item, gpointer data);
    void showContextMenu(int x, int y);
    void appendItem(GdkPixbuf * pixbuf, const String & text, int page, bool marked);

private:
    Document * iDoc;
    GtkWidget * iScroller;
    GtkWidget * iGridView;
    GListStore * iStore;
    GtkSelectionModel * iSelection;
};

// --------------------------------------------------------------------
#endif
