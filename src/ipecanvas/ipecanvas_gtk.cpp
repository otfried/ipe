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

#include "ipecanvas_gtk.h"
#include "ipecairopainter.h"
#include "ipetool.h"

#include <cairo.h>

using namespace ipe;

// --------------------------------------------------------------------

void Canvas::invalidate() { gtk_widget_queue_draw(iWindow); }

void Canvas::invalidate(int x, int y, int w, int h) {
    gtk_widget_queue_draw_area(iWindow, x, y, w, h);
}

// --------------------------------------------------------------------

void Canvas::buttonHandler(GdkEventButton * ev) {
    ipeDebug("Canvas::button %d %d %g %g", ev->button, ev->type, ev->x, ev->y);
    iGlobalPos = Vector(ev->x_root, ev->y_root);
    computeFifi(ev->x, ev->y);
    // TODO: int mod = getModifiers() | iAdditionalModifiers;
    int mod = iAdditionalModifiers;
    bool down = (ev->type == GDK_BUTTON_PRESS);
    if (iTool)
	iTool->mouseButton(ev->button | mod, down);
    else if (down && iObserver)
	iObserver->canvasObserverMouseAction(ev->button | mod);
}

void Canvas::motionHandler(GdkEventMotion * event) {
    // ipeDebug("Canvas::mouseMove %g %g", event->x, event->y);
    computeFifi(event->x, event->y);
    if (iTool) iTool->mouseMove();
    if (iObserver) iObserver->canvasObserverPositionChanged();
}

void Canvas::scrollHandler(GdkEventScroll * event) {
    int zDelta = (event->direction == GDK_SCROLL_UP) ? 120 : -120;
    int kind = (event->state & GDK_CONTROL_MASK) ? 2 : 0;
    ipeDebug("Canvas::wheel %d", zDelta);
    if (iObserver) {
	if (event->state & GDK_SHIFT_MASK)
	    iObserver->canvasObserverWheelMoved(zDelta, 0.0, kind);
	else
	    iObserver->canvasObserverWheelMoved(0.0, zDelta, kind);
    }
}

void Canvas::exposeHandler(cairo_t * cr) {
    iWidth = gtk_widget_get_allocated_width(iWindow);
    iHeight = gtk_widget_get_allocated_height(iWindow);
    int scale = gtk_widget_get_scale_factor(iWindow);
    iBWidth = iWidth * scale;
    iBHeight = iHeight * scale;
    // ipeDebug("Canvas::exposeHandler %gx%g (%gx%g)", iWidth, iHeight, iBWidth,
    // iBHeight);

    refreshSurface();

    cairo_save(cr);
    cairo_scale(cr, 1.0 / scale, 1.0 / scale);
    cairo_set_source_surface(cr, iSurface, 0.0, 0.0);
    cairo_paint(cr);
    cairo_restore(cr);

    if (iFifiVisible) drawFifi(cr);

    if (iPage) {
	CairoPainter cp(iCascade, iFonts.get(), cr, iZoom, false, false);
	cp.transform(canvasTfm());
	cp.pushMatrix();
	drawTool(cp);
	cp.popMatrix();
    }
}

// --------------------------------------------------------------------

gboolean Canvas::button_cb(GtkWidget * widget, GdkEvent * event, Canvas * canvas) {
    canvas->buttonHandler((GdkEventButton *)event);
    return TRUE;
}

gboolean Canvas::expose_cb(GtkWidget * widget, cairo_t * cr, Canvas * canvas) {
    canvas->exposeHandler(cr);
    return TRUE;
}

gboolean Canvas::motion_cb(GtkWidget * widget, GdkEvent * event, Canvas * canvas) {
    canvas->motionHandler((GdkEventMotion *)event);
    return TRUE;
}

gboolean Canvas::scroll_cb(GtkWidget * widget, GdkEvent * event, Canvas * canvas) {
    canvas->scrollHandler((GdkEventScroll *)event);
    return TRUE;
}

void Canvas::setCursor(TCursor cursor, double w, Color * color) {
    // TODO
}

// --------------------------------------------------------------------

Canvas::Canvas(GtkWidget * parent) {
    iWindow = gtk_drawing_area_new();
    gtk_widget_add_events(iWindow, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
				       | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
    gtk_widget_set_size_request(iWindow, 600, 400);
    gtk_widget_set_hexpand(iWindow, TRUE);
    gtk_widget_set_vexpand(iWindow, TRUE);
    gtk_widget_set_can_focus(iWindow, TRUE);
    g_signal_connect(G_OBJECT(iWindow), "button-release-event", G_CALLBACK(button_cb),
		     this);
    g_signal_connect(G_OBJECT(iWindow), "button-press-event", G_CALLBACK(button_cb),
		     this);
    g_signal_connect(G_OBJECT(iWindow), "draw", G_CALLBACK(expose_cb), this);
    g_signal_connect(G_OBJECT(iWindow), "motion-notify-event", G_CALLBACK(motion_cb),
		     this);
    g_signal_connect(G_OBJECT(iWindow), "scroll-event", G_CALLBACK(scroll_cb), this);
}

Canvas::~Canvas() {
    // do I need to delete the GTK Window?  It is owned by its parent.
}

// --------------------------------------------------------------------
