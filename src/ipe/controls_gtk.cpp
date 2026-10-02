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

#include "controls_gtk.h"

#include "ipecairopainter.h"
#include "ipethumbs.h"

#include <algorithm>
#include <cstdio>

// --------------------------------------------------------------------

// Cairo image surfaces use CAIRO_FORMAT_ARGB32/RGB24, i.e. a native-endian
// 32-bit value 0xAARRGGBB.  On the little-endian platforms Ipe supports,
// this means the bytes in memory are B, G, R, A.  GdkPixbuf wants R, G, B,
// A, so we need to swap the red and blue channels.
GdkPixbuf * pixbufFromArgb32(const uint8_t * data, int w, int h, int stride) {
    GdkPixbuf * pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    int dstride = gdk_pixbuf_get_rowstride(pixbuf);
    uint8_t * dst = gdk_pixbuf_get_pixels(pixbuf);
    for (int y = 0; y < h; ++y) {
	const uint8_t * s = data + y * stride;
	uint8_t * d = dst + y * dstride;
	for (int x = 0; x < w; ++x) {
	    d[0] = s[2]; // R
	    d[1] = s[1]; // G
	    d[2] = s[0]; // B
	    d[3] = s[3]; // A
	    s += 4;
	    d += 4;
	}
    }
    return pixbuf;
}

// --------------------------------------------------------------------

GdkTexture * textureFromPixbuf(GdkPixbuf * pixbuf) {
    return gdk_texture_new_for_pixbuf(pixbuf);
}

static gboolean idle_unparent_cb(gpointer data) {
    gtk_widget_unparent(GTK_WIDGET(data));
    return G_SOURCE_REMOVE;
}

// GtkPopoverMenu closes (emitting "closed") *before* a clicked row's GAction
// "activate" signal fires - GTK defers that to an idle callback. Unparenting
// synchronously here would tear the popover down before that idle runs, so
// the action would never fire; queue our own idle after GTK's instead.
static void on_popover_closed(GtkPopover * popover, gpointer) {
    g_idle_add(idle_unparent_cb, popover);
}

void popupPointingAt(GtkWidget * popover, GtkWidget * parent, GtkWidget * relativeTo,
		     int x, int y) {
    gtk_widget_set_parent(popover, parent);
    g_signal_connect(popover, "closed", G_CALLBACK(on_popover_closed), nullptr);
    int px = x, py = y;
    if (relativeTo != parent) {
	graphene_point_t src = GRAPHENE_POINT_INIT(float(x), float(y));
	graphene_point_t dst;
	if (!gtk_widget_compute_point(relativeTo, parent, &src, &dst)) dst = src;
	px = int(dst.x);
	py = int(dst.y);
    }
    GdkRectangle rect = {px, py, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
    gtk_popover_popup(GTK_POPOVER(popover));
}

// --------------------------------------------------------------------

// Named CSS classes for LayerBox row/label coloring, loaded once into the
// default display's stylesheet (a non-deprecated, display-wide provider).
static void ensureLayerBoxCss() {
    static bool loaded = false;
    if (loaded) return;
    loaded = true;
    GtkCssProvider * provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
				      ".layer-row-active { background-color: #3465a4; }"
				      ".layer-row-locked { background-color: #ffdcdc; }"
				      ".layer-label-active { color: #ffffff; }"
				      ".layer-label-snap-never { color: #5080ff; }"
				      ".layer-label-snap-always { color: #00a000; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
					       GTK_STYLE_PROVIDER(provider),
					       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

LayerBox::LayerBox() {
    ensureLayerBoxCss();
    iInSet = false;
    iList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(iList), GTK_SELECTION_NONE);
    iScroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(iScroller), GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(iScroller), iList);
}

LayerBox::~LayerBox() {
    // nothing: widgets are owned by their parent
}

std::vector<String> LayerBox::layers() const {
    std::vector<String> result;
    for (auto & r : iRows) result.push_back(r.layer);
    return result;
}

void LayerBox::check_toggled_cb(GtkCheckButton * b, gpointer data) {
    LayerBox * self = (LayerBox *)data;
    if (self->iInSet) return;
    String name = (const char *)g_object_get_data(G_OBJECT(b), "ipe-layername");
    if (self->activated)
	self->activated(gtk_check_button_get_active(b) ? "selecton" : "selectoff", name);
}

void LayerBox::label_click_cb(GtkGestureClick * gesture, int, double, double,
			      gpointer data) {
    LayerBox * self = (LayerBox *)data;
    GtkWidget * label = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    String name = (const char *)g_object_get_data(G_OBJECT(label), "ipe-layername");
    if (self->activated) self->activated("active", name);
}

void LayerBox::row_popup_cb(GtkGestureClick * gesture, int, double x, double y,
			    gpointer data) {
    LayerBox * self = (LayerBox *)data;
    GtkWidget * row = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    String name = (const char *)g_object_get_data(G_OBJECT(row), "ipe-layername");
    if (self->showLayerBoxPopup) self->showLayerBoxPopup(row, int(x), int(y), name);
}

void LayerBox::set(const Page * page, int view) {
    std::vector<int> objCounts;
    page->objectsPerLayer(objCounts);
    iInSet = true;

    // GtkListBox keeps its own row bookkeeping, so rows must be removed
    // through gtk_list_box_remove rather than raw gtk_widget_unparent.
    GtkWidget * stale;
    while ((stale = gtk_widget_get_first_child(iList)) != nullptr)
	gtk_list_box_remove(GTK_LIST_BOX(iList), stale);
    iRows.clear();

    for (int i = 0; i < page->countLayers(); ++i) {
	String name = page->layer(i);
	char buf[32];
	sprintf(buf, " (%d)", objCounts[i]);
	String text = name + buf;

	GtkWidget * row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

	GtkWidget * check = gtk_check_button_new();
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check), page->visible(view, i));
	GtkWidget * label = gtk_label_new(text.z());
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_widget_set_hexpand(label, TRUE);

	gtk_box_append(GTK_BOX(row), check);
	gtk_box_append(GTK_BOX(row), label);

	if (page->layer(i) == page->active(view)) {
	    gtk_widget_add_css_class(row, "layer-row-active");
	    gtk_widget_add_css_class(label, "layer-label-active");
	} else if (page->isLocked(i)) {
	    gtk_widget_add_css_class(row, "layer-row-locked");
	}

	switch (page->snapping(i)) {
	case Page::SnapMode::Never:
	    gtk_widget_add_css_class(label, "layer-label-snap-never");
	    break;
	case Page::SnapMode::Always:
	    gtk_widget_add_css_class(label, "layer-label-snap-always");
	    break;
	default: break;
	}

	g_object_set_data_full(G_OBJECT(check), "ipe-layername", g_strdup(name.z()),
			       g_free);
	g_signal_connect(check, "toggled", G_CALLBACK(check_toggled_cb), this);

	g_object_set_data_full(G_OBJECT(label), "ipe-layername", g_strdup(name.z()),
			       g_free);
	GtkGesture * labelClick = gtk_gesture_click_new();
	g_signal_connect(labelClick, "pressed", G_CALLBACK(label_click_cb), this);
	gtk_widget_add_controller(label, GTK_EVENT_CONTROLLER(labelClick));

	g_object_set_data_full(G_OBJECT(row), "ipe-layername", g_strdup(name.z()),
			       g_free);
	GtkGesture * rowPopup = gtk_gesture_click_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(rowPopup), GDK_BUTTON_SECONDARY);
	g_signal_connect(rowPopup, "pressed", G_CALLBACK(row_popup_cb), this);
	gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(rowPopup));

	gtk_list_box_append(GTK_LIST_BOX(iList), row);

	SRow r;
	r.layer = name;
	r.check = check;
	r.label = label;
	r.row = row;
	iRows.push_back(r);
    }
    iInSet = false;
}

// --------------------------------------------------------------------

PathView::PathView(int uiScale)
    : iUiScale(uiScale)
    , iCascade(nullptr) {
    iDrawingArea = gtk_drawing_area_new();
    int w = 120 * uiScale / 100;
    int h = 40 * uiScale / 100;
    gtk_widget_set_size_request(iDrawingArea, w, h);
    gtk_widget_set_has_tooltip(iDrawingArea, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(iDrawingArea), draw_cb, this,
				   nullptr);

    GtkGesture * click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(click_cb), this);
    gtk_widget_add_controller(iDrawingArea, GTK_EVENT_CONTROLLER(click));

    GtkGesture * popup = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(popup), GDK_BUTTON_SECONDARY);
    g_signal_connect(popup, "pressed", G_CALLBACK(popup_cb), this);
    gtk_widget_add_controller(iDrawingArea, GTK_EVENT_CONTROLLER(popup));

    g_signal_connect(iDrawingArea, "query-tooltip", G_CALLBACK(query_tooltip_cb), this);
}

PathView::~PathView() {}

void PathView::setColor(const Color & color) {
    iColor = color;
    gtk_widget_queue_draw(iDrawingArea);
}

void PathView::set(const AllAttributes & all, Cascade * sheet) {
    iCascade = sheet;
    iAll = all;
    gtk_widget_queue_draw(iDrawingArea);
}

void PathView::draw(cairo_t * cc, int w, int h) {
    cairo_set_source_rgb(cc, iColor.iRed.toDouble(), iColor.iGreen.toDouble(),
			 iColor.iBlue.toDouble());
    cairo_rectangle(cc, 0, 0, w, h);
    cairo_fill(cc);

    if (!iCascade) return;

    cairo_save(cc);
    cairo_translate(cc, 0, h);
    double zoom = w / 70.0;
    cairo_scale(cc, zoom, -zoom);
    Vector v0 = (1.0 / zoom) * Vector(0.1 * w, 0.5 * h);
    Vector v1 = (1.0 / zoom) * Vector(0.7 * w, 0.5 * h);
    Vector u1 = (1.0 / zoom) * Vector(0.88 * w, 0.8 * h);
    Vector u2 = (1.0 / zoom) * Vector(0.80 * w, 0.5 * h);
    Vector u3 = (1.0 / zoom) * Vector(0.88 * w, 0.2 * h);
    Vector u4 = (1.0 / zoom) * Vector(0.96 * w, 0.5 * h);
    Vector mid = 0.5 * (v0 + v1);
    Vector vf = iAll.iFArrowShape.isMidArrow() ? mid : v1;
    Vector vr = iAll.iRArrowShape.isMidArrow() ? mid : v0;

    CairoPainter painter(iCascade, nullptr, cc, 3.0, false, false);
    painter.setPen(iAll.iPen);
    painter.setDashStyle(iAll.iDashStyle);
    painter.setStroke(iAll.iStroke);
    painter.setFill(iAll.iFill);
    painter.pushMatrix();
    painter.newPath();
    painter.moveTo(v0);
    painter.lineTo(v1);
    painter.drawPath(EStrokedOnly);
    if (iAll.iFArrow)
	Path::drawArrow(painter, vf, Angle(0), iAll.iFArrowShape, iAll.iFArrowSize,
			100.0);
    if (iAll.iRArrow)
	Path::drawArrow(painter, vr, Angle(IpePi), iAll.iRArrowShape, iAll.iRArrowSize,
			100.0);
    painter.setDashStyle(Attribute::NORMAL());
    painter.setTiling(iAll.iTiling);
    painter.newPath();
    painter.moveTo(u1);
    painter.lineTo(u2);
    painter.lineTo(u3);
    painter.lineTo(u4);
    painter.closePath();
    painter.drawPath(iAll.iPathMode);
    painter.popMatrix();
    cairo_restore(cc);
}

void PathView::draw_cb(GtkDrawingArea *, cairo_t * cr, int width, int height,
		       gpointer data) {
    ((PathView *)data)->draw(cr, width, height);
}

void PathView::click_cb(GtkGestureClick * gesture, int, double x, double y,
			gpointer data) {
    PathView * self = (PathView *)data;
    GtkWidget * widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    int w = gtk_widget_get_width(widget);
    if (x < w * 3 / 10) {
	if (self->activated)
	    self->activated(self->iAll.iRArrow ? "rarrow|false" : "rarrow|true");
    } else if (x > w * 4 / 10 && x < w * 72 / 100) {
	if (self->activated)
	    self->activated(self->iAll.iFArrow ? "farrow|false" : "farrow|true");
    } else if (x > w * 78 / 100) {
	const char * s = nullptr;
	switch (self->iAll.iPathMode) {
	case EStrokedOnly: s = "pathmode|strokedfilled"; break;
	case EStrokedAndFilled: s = "pathmode|filled"; break;
	case EFilledOnly: s = "pathmode|stroked"; break;
	}
	if (self->activated) self->activated(s);
    }
}

void PathView::popup_cb(GtkGestureClick * gesture, int, double x, double y,
			gpointer data) {
    PathView * self = (PathView *)data;
    GtkWidget * widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    if (self->showPathStylePopup) self->showPathStylePopup(widget, int(x), int(y));
}

gboolean PathView::query_tooltip_cb(GtkWidget * widget, gint x, gint y,
				    gboolean keyboard_mode, GtkTooltip * tooltip,
				    gpointer data) {
    int w = gtk_widget_get_width(widget);
    const char * tip = nullptr;
    if (x < w * 3 / 10)
	tip = "Toggle reverse arrow";
    else if (x > w * 4 / 10 && x < w * 72 / 100)
	tip = "Toggle forward arrow";
    else if (x > w * 78 / 100)
	tip = "Toggle stroked/stroked & filled/filled";
    if (tip) {
	gtk_tooltip_set_text(tooltip, tip);
	return TRUE;
    }
    return FALSE;
}

// --------------------------------------------------------------------

// PageSorter items are plain GObjects carrying their data as qdata, to
// avoid the boilerplate of defining a full custom GType.
static GObject * ps_item_new(GdkPixbuf * pixbuf, const String & text, int page,
			     bool marked) {
    GObject * obj = (GObject *)g_object_new(G_TYPE_OBJECT, nullptr);
    g_object_set_data_full(obj, "ipe-pixbuf", pixbuf, g_object_unref);
    g_object_set_data_full(obj, "ipe-text", g_strdup(text.z()), g_free);
    g_object_set_data(obj, "ipe-page", GINT_TO_POINTER(page));
    g_object_set_data(obj, "ipe-marked", GINT_TO_POINTER(marked ? 1 : 0));
    return obj;
}

void PageSorter::appendItem(GdkPixbuf * pixbuf, const String & text, int page,
			    bool marked) {
    GObject * item = ps_item_new(pixbuf, text, page, marked);
    g_list_store_append(iStore, item);
    g_object_unref(item);
}

PageSorter::PageSorter(Document * doc, int pno, int width)
    : iDoc(doc)
    , iThumbWidth(width) {
    iStore = g_list_store_new(G_TYPE_OBJECT);

    Thumbnail r(iDoc, width);

    if (pno >= 0) {
	const Page * p = doc->page(pno);
	for (int i = 0; i < p->countViews(); ++i) {
	    Buffer b = r.render(p, i);
	    GdkPixbuf * pixbuf =
		pixbufFromArgb32((const uint8_t *)b.data(), width, r.height(), width * 4);
	    String t = p->viewName(i);
	    char buf[32];
	    String s;
	    if (t.empty()) {
		sprintf(buf, "View %d", i + 1);
		s = buf;
	    } else {
		sprintf(buf, "%d: ", i + 1);
		s = String(buf) + t;
	    }
	    iMarks.push_back(p->markedView(i));
	    appendItem(pixbuf, s, i, iMarks.back());
	}
    } else {
	Attribute variant = doc->properties().iVariant;
	for (int i = 0; i < doc->countPages(); ++i) {
	    Page * p = doc->page(i);
	    Buffer b = r.render(p, p->countViews() - 1);
	    GdkPixbuf * pixbuf =
		pixbufFromArgb32((const uint8_t *)b.data(), width, r.height(), width * 4);
	    String t = p->title(variant);
	    char buf[32];
	    String s;
	    if (t.empty()) {
		sprintf(buf, "Page %d", i + 1);
		s = buf;
	    } else {
		sprintf(buf, "%d: ", i + 1);
		s = String(buf) + t;
	    }
	    iMarks.push_back(p->marked());
	    appendItem(pixbuf, s, i, iMarks.back());
	}
    }

    iSelection = GTK_SELECTION_MODEL(gtk_multi_selection_new(G_LIST_MODEL(iStore)));

    GtkListItemFactory * factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), this);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_cb), this);

    iGridView = gtk_grid_view_new(iSelection, factory);
    gtk_grid_view_set_max_columns(GTK_GRID_VIEW(iGridView), 100);

    GtkGesture * popup = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(popup), GDK_BUTTON_SECONDARY);
    g_signal_connect(popup, "pressed", G_CALLBACK(popup_cb), this);
    gtk_widget_add_controller(iGridView, GTK_EVENT_CONTROLLER(popup));

    iScroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(iScroller), GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(iScroller), iGridView);
    // without this, the box packing it into the dialog gives it only its
    // natural (tiny) size, instead of filling and following dialog resizes
    gtk_widget_set_hexpand(iScroller, TRUE);
    gtk_widget_set_vexpand(iScroller, TRUE);
}

PageSorter::~PageSorter() {
    // iSelection owns iStore; iGridView (owned by iScroller) owns iSelection
}

void PageSorter::setup_cb(GtkListItemFactory *, GtkListItem * item, gpointer data) {
    GtkWidget * box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget * image = gtk_image_new();
    GtkWidget * label = gtk_label_new(nullptr);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(box), image);
    gtk_box_append(GTK_BOX(box), label);
    g_object_set_data(G_OBJECT(box), "ipe-image", image);
    g_object_set_data(G_OBJECT(box), "ipe-label", label);
    g_object_set_data(G_OBJECT(box), "ipe-sorter", data);

    // drag source: lets the user pick up a page and drop it elsewhere to
    // reorder, similar to the HTML/JS version's Sortable-based drag
    GtkDragSource * dragSource = gtk_drag_source_new();
    gtk_drag_source_set_actions(dragSource, GDK_ACTION_MOVE);
    g_signal_connect(dragSource, "prepare", G_CALLBACK(drag_prepare_cb), nullptr);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(dragSource));

    GtkDropTarget * dropTarget = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
    g_signal_connect(dropTarget, "drop", G_CALLBACK(drop_cb), nullptr);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(dropTarget));

    gtk_list_item_set_child(item, box);
}

void PageSorter::bind_cb(GtkListItemFactory *, GtkListItem * item, gpointer data) {
    PageSorter * self = (PageSorter *)data;
    GObject * obj = G_OBJECT(gtk_list_item_get_item(item));
    GtkWidget * box = gtk_list_item_get_child(item);
    GtkWidget * image = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "ipe-image"));
    GtkWidget * label = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "ipe-label"));
    GdkPixbuf * pixbuf = GDK_PIXBUF(g_object_get_data(obj, "ipe-pixbuf"));
    const char * text = (const char *)g_object_get_data(obj, "ipe-text");
    bool marked = GPOINTER_TO_INT(g_object_get_data(obj, "ipe-marked"));
    // GtkListView doesn't reliably re-bind a row just because its position
    // shifted (only when the bound item itself changes), so caching the
    // position here would go stale after a reorder; cache the stable page
    // number instead, and look the position up fresh when needed
    g_object_set_data(G_OBJECT(box), "ipe-page", g_object_get_data(obj, "ipe-page"));
    GdkTexture * texture = textureFromPixbuf(pixbuf);
    gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
    g_object_unref(texture);
    // GtkImage doesn't reliably use the paintable's own size as its natural
    // size, so force the intended thumbnail width explicitly
    gtk_image_set_pixel_size(GTK_IMAGE(image), self->iThumbWidth);
    gtk_label_set_text(GTK_LABEL(label), marked ? (String("\u2713 ") + text).z() : text);
}

int PageSorter::count() const { return g_list_model_get_n_items(G_LIST_MODEL(iStore)); }

int PageSorter::pageAt(int r) const {
    GObject * obj = G_OBJECT(g_list_model_get_item(G_LIST_MODEL(iStore), r));
    int page = GPOINTER_TO_INT(g_object_get_data(obj, "ipe-page"));
    g_object_unref(obj);
    return page;
}

guint PageSorter::positionOfPage(int page) const {
    guint n = g_list_model_get_n_items(G_LIST_MODEL(iStore));
    for (guint i = 0; i < n; ++i) {
	GObject * obj = G_OBJECT(g_list_model_get_item(G_LIST_MODEL(iStore), i));
	int p = GPOINTER_TO_INT(g_object_get_data(obj, "ipe-page"));
	g_object_unref(obj);
	if (p == page) return i;
    }
    return 0; // shouldn't happen
}

void PageSorter::deletePages() {
    std::vector<guint> selected;
    guint n = g_list_model_get_n_items(G_LIST_MODEL(iStore));
    for (guint i = 0; i < n; ++i)
	if (gtk_selection_model_is_selected(iSelection, i)) selected.push_back(i);
    // remove starting from the highest index so indices stay valid
    std::sort(selected.begin(), selected.end(), std::greater<guint>());
    for (guint i : selected) g_list_store_remove(iStore, i);
}

void PageSorter::markPages(bool mark) {
    guint n = g_list_model_get_n_items(G_LIST_MODEL(iStore));
    for (guint i = 0; i < n; ++i) {
	if (!gtk_selection_model_is_selected(iSelection, i)) continue;
	GObject * obj = G_OBJECT(g_list_model_get_item(G_LIST_MODEL(iStore), i));
	int page = GPOINTER_TO_INT(g_object_get_data(obj, "ipe-page"));
	GdkPixbuf * pixbuf = GDK_PIXBUF(g_object_get_data(obj, "ipe-pixbuf"));
	String text = (const char *)g_object_get_data(obj, "ipe-text");
	iMarks[page] = mark;
	// GtkGridView skips rebinding a row when the spliced-in object is the
	// same pointer as before, so splice in a genuinely new item instead
	// of mutating this one in place
	GObject * newObj = ps_item_new(GDK_PIXBUF(g_object_ref(pixbuf)), text, page, mark);
	gpointer item = newObj;
	g_list_store_splice(iStore, i, 1, &item, 1);
	g_object_unref(newObj);
	g_object_unref(obj);
    }
}

void PageSorter::moveItems(std::vector<guint> positions, guint dst) {
    if (positions.empty()) return;
    std::sort(positions.begin(), positions.end());
    // dropping a selection onto one of its own members is a no-op
    if (std::binary_search(positions.begin(), positions.end(), dst)) return;

    guint before = 0;
    for (guint p : positions)
	if (p < dst) ++before;

    // remove from highest to lowest index so earlier indices stay valid,
    // then reverse to restore the original (ascending) relative order
    std::vector<GObject *> objs;
    for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
	objs.push_back(G_OBJECT(g_list_model_get_item(G_LIST_MODEL(iStore), *it)));
	g_list_store_remove(iStore, *it);
    }
    std::reverse(objs.begin(), objs.end());

    guint insertAt = dst - before;
    for (GObject * obj : objs) {
	g_list_store_insert(iStore, insertAt++, obj);
	g_object_unref(obj);
    }
}

GdkContentProvider * PageSorter::drag_prepare_cb(GtkDragSource * source, double, double,
						 gpointer) {
    GtkWidget * box = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(source));
    PageSorter * self = (PageSorter *)g_object_get_data(G_OBJECT(box), "ipe-sorter");
    int page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "ipe-page"));
    guint pos = self->positionOfPage(page);

    // if the dragged row is part of a multi-selection, take the whole
    // (ordered) selection along; otherwise just this one row
    GString * s = g_string_new(nullptr);
    if (gtk_selection_model_is_selected(self->iSelection, pos)) {
	guint n = g_list_model_get_n_items(G_LIST_MODEL(self->iStore));
	for (guint i = 0; i < n; ++i) {
	    if (!gtk_selection_model_is_selected(self->iSelection, i)) continue;
	    if (s->len) g_string_append_c(s, ',');
	    g_string_append_printf(s, "%u", i);
	}
    } else {
	g_string_append_printf(s, "%u", pos);
    }
    GdkContentProvider * provider = gdk_content_provider_new_typed(G_TYPE_STRING, s->str);
    g_string_free(s, TRUE);
    return provider;
}

gboolean PageSorter::drop_cb(GtkDropTarget * target, const GValue * value, double,
			    double, gpointer) {
    GtkWidget * box = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
    PageSorter * self = (PageSorter *)g_object_get_data(G_OBJECT(box), "ipe-sorter");
    int page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "ipe-page"));
    guint dstPos = self->positionOfPage(page);

    std::vector<guint> positions;
    gchar ** parts = g_strsplit(g_value_get_string(value), ",", -1);
    for (int i = 0; parts[i]; ++i)
	positions.push_back((guint)g_ascii_strtoull(parts[i], nullptr, 10));
    g_strfreev(parts);

    self->moveItems(positions, dstPos);
    return TRUE;
}

static void action_delete_cb(GSimpleAction *, GVariant *, gpointer data) {
    ((PageSorter *)data)->deletePages();
}

static void action_mark_cb(GSimpleAction *, GVariant *, gpointer data) {
    ((PageSorter *)data)->markPages(true);
}

static void action_unmark_cb(GSimpleAction *, GVariant *, gpointer data) {
    ((PageSorter *)data)->markPages(false);
}

void PageSorter::showContextMenu(int x, int y) {
    GSimpleActionGroup * group = g_simple_action_group_new();
    static const GActionEntry entries[] = {
	{"delete", action_delete_cb, nullptr, nullptr, nullptr, {0, 0, 0}},
	{"mark", action_mark_cb, nullptr, nullptr, nullptr, {0, 0, 0}},
	{"unmark", action_unmark_cb, nullptr, nullptr, nullptr, {0, 0, 0}},
    };
    g_action_map_add_action_entries(G_ACTION_MAP(group), entries, G_N_ELEMENTS(entries),
				    this);

    GMenu * menu = g_menu_new();
    g_menu_append(menu, "Delete", "sorter.delete");
    g_menu_append(menu, "Mark", "sorter.mark");
    g_menu_append(menu, "Unmark", "sorter.unmark");

    GtkWidget * popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
    // insert directly on the popover: it isn't a descendant of iGridView
    // until popupPointingAt() parents it, so actions wouldn't be found yet
    gtk_widget_insert_action_group(popover, "sorter", G_ACTION_GROUP(group));
    g_object_unref(menu);
    g_object_unref(group);
    popupPointingAt(popover, iGridView, iGridView, x, y);
}

void PageSorter::popup_cb(GtkGestureClick *, int, double x, double y, gpointer data) {
    ((PageSorter *)data)->showContextMenu(int(x), int(y));
}

// --------------------------------------------------------------------
