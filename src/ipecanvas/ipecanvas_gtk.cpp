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

void Canvas::invalidate(int, int, int, int) { gtk_widget_queue_draw(iWindow); }

// --------------------------------------------------------------------

static int convertModifiers(GdkModifierType state) {
    int mod = 0;
    if (state & GDK_SHIFT_MASK) mod |= CanvasBase::EShift;
    if (state & GDK_CONTROL_MASK) mod |= CanvasBase::EControl;
    if (state & GDK_ALT_MASK) mod |= CanvasBase::EAlt;
    if (state & GDK_SUPER_MASK) mod |= CanvasBase::EMeta;
    return mod;
}

static int convertMouseButton(guint gbutton) {
    switch (gbutton) {
    case 1: // left
    default: return 1;
    case 2: // middle
	return 4;
    case 3: // right
	return 2;
    case 8: // extra 1
	return 8;
    case 9: // extra 2
	return 16;
    }
}

void Canvas::buttonHandler(double x, double y, GtkGestureClick * gesture, int nPress,
			   bool down) {
    guint gbutton = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    GdkModifierType state =
	gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
    // ipeDebug("Canvas::button %d %g %g %d %d %d", gbutton, x, y, down, nPress, state);
    int button = convertMouseButton(gbutton);
    if (button == 1 && nPress == 2)
	// left double click
	button = 0x81;
    iGlobalPos = Vector(x, y);
    computeFifi(x, y);
    int mod = convertModifiers(state) | iAdditionalModifiers;
    if (iTool)
	iTool->mouseButton(button | mod, down);
    else if (down && iObserver)
	iObserver->canvasObserverMouseAction(button | mod);
}

gboolean Canvas::keyHandler(guint keyval, guint keycode, GdkModifierType state) {
    String key = gdk_keyval_name(keyval);
    ipeDebug("Key pressed: %s (keyval: %u, keycode: %u)", key.z(), keyval, keycode);

    // TODO: add key translation
    // need at least Escape -> \027
    // space -> 0x20
    // BackSpace -> \8
    // Delete -> \127
    // linestool : s a y
    // TODO; remove delete_key from prefs, simply interpret both!

    if (iTool && iTool->key(key, convertModifiers(state) | iAdditionalModifiers))
	return GDK_EVENT_STOP; // Event handled
    else
	return GDK_EVENT_PROPAGATE; // Pass unhandled keys up to parent widgets
}

void Canvas::motionHandler(double x, double y) {
    // ipeDebug("Canvas::mouseMove %g %g", x, y);
    computeFifi(x, y);
    if (iTool) iTool->mouseMove();
    if (iObserver) iObserver->canvasObserverPositionChanged();
}

void Canvas::scrollHandler(double dx, double dy, GdkModifierType state) {
    int kind = (state & GDK_CONTROL_MASK) ? 2 : 0;
    // ipeDebug("Canvas::wheel %g %g", dx, dy);
    if (iObserver) {
	if (state & GDK_SHIFT_MASK)
	    iObserver->canvasObserverWheelMoved(15 * dy, 15 * dx, kind);
	else
	    iObserver->canvasObserverWheelMoved(15 * dx, -15 * dy, kind);
    }
}

void Canvas::exposeHandler(cairo_t * cr, int width, int height) {
    iWidth = width;
    iHeight = height;
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

void Canvas::expose_cb(GtkDrawingArea *, cairo_t * cr, int width, int height,
		       Canvas * canvas) {
    canvas->exposeHandler(cr, width, height);
}

void Canvas::pressed_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			Canvas * canvas) {
    canvas->buttonHandler(x, y, gesture, nPress, true);
}

void Canvas::released_cb(GtkGestureClick * gesture, int nPress, double x, double y,
			 Canvas * canvas) {
    canvas->buttonHandler(x, y, gesture, nPress, false);
}

void Canvas::motion_cb(GtkEventControllerMotion *, double x, double y, Canvas * canvas) {
    canvas->motionHandler(x, y);
}

gboolean Canvas::scroll_cb(GtkEventControllerScroll * controller, double dx, double dy,
			   Canvas * canvas) {
    GdkModifierType state =
	gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(controller));
    canvas->scrollHandler(dx, dy, state);
    return TRUE;
}

gboolean Canvas::keypress_cb(GtkEventControllerKey * controller, guint keyval,
			     guint keycode, GdkModifierType state, Canvas * canvas) {
    return canvas->keyHandler(keyval, keycode, state);
}

void Canvas::setCursor(TCursor cursor, double w, Color * color) {
    // TODO
}

// --------------------------------------------------------------------

Canvas::Canvas(GtkWidget * /* parent */) {
    iWindow = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(iWindow), 600);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(iWindow), 400);
    gtk_widget_set_size_request(iWindow, 600, 400);
    gtk_widget_set_hexpand(iWindow, TRUE);
    gtk_widget_set_vexpand(iWindow, TRUE);
    gtk_widget_set_can_focus(iWindow, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(iWindow),
				   GtkDrawingAreaDrawFunc(expose_cb), this, nullptr);

    GtkGesture * click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
    g_signal_connect(click, "pressed", G_CALLBACK(pressed_cb), this);
    g_signal_connect(click, "released", G_CALLBACK(released_cb), this);
    gtk_widget_add_controller(iWindow, GTK_EVENT_CONTROLLER(click));

    GtkEventController * motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "motion", G_CALLBACK(motion_cb), this);
    gtk_widget_add_controller(iWindow, motion);

    GtkEventController * scroll =
	gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
    g_signal_connect(scroll, "scroll", G_CALLBACK(scroll_cb), this);
    gtk_widget_add_controller(iWindow, scroll);

    gtk_widget_set_focusable(iWindow, TRUE);

    // grab focus when clicked
    g_signal_connect_swapped(click, "pressed", G_CALLBACK(gtk_widget_grab_focus),
			     iWindow);

    GtkEventController * key = gtk_event_controller_key_new();
    // make sure we get keys before the global accelerators are recognized
    gtk_event_controller_set_propagation_phase(key, GTK_PHASE_CAPTURE);
    g_signal_connect(key, "key-pressed", G_CALLBACK(keypress_cb), this);
    gtk_widget_add_controller(iWindow, key);
}

Canvas::~Canvas() {}

// --------------------------------------------------------------------
