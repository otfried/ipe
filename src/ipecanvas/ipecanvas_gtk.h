// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Canvas for GTK
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

#ifndef IPECANVAS_GTK_H
#define IPECANVAS_GTK_H
// --------------------------------------------------------------------

#include "ipecanvas.h"

#include <gtk/gtk.h>

namespace ipe {

class Canvas : public CanvasBase {
public:
    Canvas(GtkWidget * parent);
    ~Canvas();

    GtkWidget * window() const { return iWindow; }

private:
    virtual void setCursor(TCursor cursor, double w = 1.0, Color * color = nullptr);

    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);

    void exposeHandler(cairo_t * cr, int width, int height);
    void buttonHandler(double x, double y, GtkGestureClick * gesture, int nPress,
		       bool down);
    void motionHandler(double x, double y);
    void scrollHandler(double dx, double dy, GdkModifierType state);
    gboolean keyHandler(guint keyval, guint keycode, GdkModifierType state);

    static void expose_cb(GtkDrawingArea * area, cairo_t * cr, int width, int height,
			  Canvas * canvas);
    static void pressed_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			   Canvas * canvas);
    static void released_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			    Canvas * canvas);
    static void motion_cb(GtkEventControllerMotion * controller, double x, double y,
			  Canvas * canvas);
    static gboolean scroll_cb(GtkEventControllerScroll * controller, double dx, double dy,
			      Canvas * canvas);
    static gboolean keypress_cb(GtkEventControllerKey * controller, guint keyval,
				guint keycode, GdkModifierType state, Canvas * canvas);

private:
    GtkWidget * iWindow;
};

} // namespace ipe

// --------------------------------------------------------------------
#endif
