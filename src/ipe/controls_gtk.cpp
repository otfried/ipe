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

// Apply a small CSS snippet scoped to a single widget (used instead of the
// deprecated gtk_widget_modify_bg/fg + GdkColor API).
static void set_widget_css(GtkWidget * w, const char * css) {
    GtkCssProvider * provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, nullptr);
    gtk_style_context_add_provider(gtk_widget_get_style_context(w),
				   GTK_STYLE_PROVIDER(provider),
				   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

LayerBox::LayerBox() {
    iInSet = false;
    iList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(iList), GTK_SELECTION_NONE);
    iScroller = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(iScroller), GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(iScroller), iList);
    gtk_widget_show(iList);
}

LayerBox::~LayerBox() {
    // nothing: widgets are owned by their parent
}

std::vector<String> LayerBox::layers() const {
    std::vector<String> result;
    for (auto & r : iRows) result.push_back(r.layer);
    return result;
}

void LayerBox::check_toggled_cb(GtkToggleButton * b, gpointer data) {
    LayerBox * self = (LayerBox *)g_object_get_data(G_OBJECT(b), "ipe-layerbox");
    if (self->iInSet) return;
    String name = (const char *)g_object_get_data(G_OBJECT(b), "ipe-layername");
    if (self->activated)
	self->activated(gtk_toggle_button_get_active(b) ? "selecton" : "selectoff", name);
}

gboolean LayerBox::button_press_cb(GtkWidget * w, GdkEventButton * ev, gpointer data) {
    LayerBox * self = (LayerBox *)data;
    String name = (const char *)g_object_get_data(G_OBJECT(w), "ipe-layername");
    if (ev->button == 1) {
	if (self->activated) self->activated("active", name);
    } else if (ev->button == 3) {
	if (self->showLayerBoxPopup)
	    self->showLayerBoxPopup(int(ev->x_root), int(ev->y_root), name);
	return TRUE;
    }
    return FALSE;
}

void LayerBox::set(const Page * page, int view) {
    std::vector<int> objCounts;
    page->objectsPerLayer(objCounts);
    iInSet = true;

    gtk_container_foreach(
	GTK_CONTAINER(iList), [](GtkWidget * w, gpointer) { gtk_widget_destroy(w); },
	nullptr);
    iRows.clear();

    for (int i = 0; i < page->countLayers(); ++i) {
	String name = page->layer(i);
	char buf[32];
	sprintf(buf, " (%d)", objCounts[i]);
	String text = name + buf;

	GtkWidget * row = gtk_event_box_new();
	GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_container_add(GTK_CONTAINER(row), hbox);

	GtkWidget * check = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), page->visible(view, i));
	GtkWidget * label = gtk_label_new(text.z());
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);

	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 2);

	if (page->layer(i) == page->active(view)) {
	    set_widget_css(row, "box { background-color: #3465a4; }");
	    set_widget_css(label, "label { color: #ffffff; }");
	} else if (page->isLocked(i)) {
	    set_widget_css(row, "box { background-color: #ffdcdc; }");
	}

	switch (page->snapping(i)) {
	case Page::SnapMode::Never:
	    set_widget_css(label, "label { color: #5080ff; }");
	    break;
	case Page::SnapMode::Always:
	    set_widget_css(label, "label { color: #00a000; }");
	    break;
	default: break;
	}

	g_object_set_data_full(G_OBJECT(check), "ipe-layername", g_strdup(name.z()),
			       g_free);
	g_object_set_data(G_OBJECT(check), "ipe-layerbox", this);
	g_signal_connect(check, "toggled", G_CALLBACK(check_toggled_cb), nullptr);

	g_object_set_data_full(G_OBJECT(row), "ipe-layername", g_strdup(name.z()),
			       g_free);
	gtk_widget_add_events(row, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(row, "button-press-event", G_CALLBACK(button_press_cb), this);

	gtk_container_add(GTK_CONTAINER(iList), row);
	gtk_widget_show_all(row);

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
    gtk_widget_add_events(iDrawingArea, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    gtk_widget_set_has_tooltip(iDrawingArea, TRUE);
    g_signal_connect(iDrawingArea, "draw", G_CALLBACK(draw_cb), this);
    g_signal_connect(iDrawingArea, "button-press-event", G_CALLBACK(button_press_cb),
		     this);
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

gboolean PathView::draw_cb(GtkWidget * widget, cairo_t * cr, gpointer data) {
    PathView * self = (PathView *)data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    self->draw(cr, alloc.width, alloc.height);
    return TRUE;
}

gboolean PathView::button_press_cb(GtkWidget * widget, GdkEventButton * ev,
				   gpointer data) {
    PathView * self = (PathView *)data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int w = alloc.width;
    if (ev->button == 1) {
	if (ev->x < w * 3 / 10) {
	    if (self->activated)
		self->activated(self->iAll.iRArrow ? "rarrow|false" : "rarrow|true");
	} else if (ev->x > w * 4 / 10 && ev->x < w * 72 / 100) {
	    if (self->activated)
		self->activated(self->iAll.iFArrow ? "farrow|false" : "farrow|true");
	} else if (ev->x > w * 78 / 100) {
	    const char * s = nullptr;
	    switch (self->iAll.iPathMode) {
	    case EStrokedOnly: s = "pathmode|strokedfilled"; break;
	    case EStrokedAndFilled: s = "pathmode|filled"; break;
	    case EFilledOnly: s = "pathmode|stroked"; break;
	    }
	    if (self->activated) self->activated(s);
	}
    } else if (ev->button == 3) {
	if (self->showPathStylePopup)
	    self->showPathStylePopup(int(ev->x_root), int(ev->y_root));
    }
    return TRUE;
}

gboolean PathView::query_tooltip_cb(GtkWidget * widget, gint x, gint y,
				    gboolean keyboard_mode, GtkTooltip * tooltip,
				    gpointer data) {
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int w = alloc.width;
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

enum { ColIcon, ColText, ColPage, ColMarked, ColNum };

PageSorter::PageSorter(Document * doc, int pno, int width)
    : iDoc(doc) {
    iStore = gtk_list_store_new(ColNum, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT,
				G_TYPE_BOOLEAN);

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
	    GtkTreeIter it;
	    gtk_list_store_append(iStore, &it);
	    gtk_list_store_set(iStore, &it, ColIcon, pixbuf, ColText, s.z(), ColPage, i,
			       ColMarked, iMarks.back(), -1);
	    g_object_unref(pixbuf);
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
	    GtkTreeIter it;
	    gtk_list_store_append(iStore, &it);
	    gtk_list_store_set(iStore, &it, ColIcon, pixbuf, ColText, s.z(), ColPage, i,
			       ColMarked, iMarks.back(), -1);
	    g_object_unref(pixbuf);
	}
    }

    iIconView = gtk_icon_view_new_with_model(GTK_TREE_MODEL(iStore));
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iIconView), ColIcon);
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(iIconView), ColText);
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(iIconView), GTK_SELECTION_MULTIPLE);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(iIconView), width + 20);

    gtk_widget_add_events(iIconView, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(iIconView, "button-press-event", G_CALLBACK(button_press_cb), this);

    iScroller = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(iScroller), GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(iScroller), iIconView);
    gtk_widget_show_all(iScroller);
}

PageSorter::~PageSorter() { g_object_unref(iStore); }

int PageSorter::count() const {
    return gtk_tree_model_iter_n_children(GTK_TREE_MODEL(iStore), nullptr);
}

int PageSorter::pageAt(int r) const {
    GtkTreeIter it;
    GtkTreePath * path = gtk_tree_path_new_from_indices(r, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(iStore), &it, path);
    gtk_tree_path_free(path);
    int page;
    gtk_tree_model_get(GTK_TREE_MODEL(iStore), &it, ColPage, &page, -1);
    return page;
}

void PageSorter::deletePages() {
    GList * items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(iIconView));
    // remove starting from the highest index so indices stay valid
    std::vector<GtkTreePath *> paths;
    for (GList * l = items; l; l = l->next) paths.push_back((GtkTreePath *)l->data);
    std::sort(paths.begin(), paths.end(), [](GtkTreePath * a, GtkTreePath * b) {
	return gtk_tree_path_compare(a, b) > 0;
    });
    for (auto path : paths) {
	GtkTreeIter it;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(iStore), &it, path);
	gtk_list_store_remove(iStore, &it);
	gtk_tree_path_free(path);
    }
    g_list_free(items);
}

void PageSorter::markPages(bool mark) {
    GList * items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(iIconView));
    for (GList * l = items; l; l = l->next) {
	GtkTreeIter it;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(iStore), &it, (GtkTreePath *)l->data);
	int page;
	gtk_tree_model_get(GTK_TREE_MODEL(iStore), &it, ColPage, &page, -1);
	iMarks[page] = mark;
	gtk_list_store_set(iStore, &it, ColMarked, mark, -1);
	gtk_tree_path_free((GtkTreePath *)l->data);
    }
    g_list_free(items);
}

static void menu_delete_cb(GtkMenuItem *, gpointer data) {
    ((PageSorter *)data)->deletePages();
}

static void menu_mark_cb(GtkMenuItem *, gpointer data) {
    ((PageSorter *)data)->markPages(true);
}

static void menu_unmark_cb(GtkMenuItem *, gpointer data) {
    ((PageSorter *)data)->markPages(false);
}

void PageSorter::showContextMenu(int x, int y) {
    GtkWidget * menu = gtk_menu_new();
    GtkWidget * item_delete = gtk_menu_item_new_with_mnemonic("_Delete");
    GtkWidget * item_mark = gtk_menu_item_new_with_mnemonic("_Mark");
    GtkWidget * item_unmark = gtk_menu_item_new_with_mnemonic("_Unmark");
    g_signal_connect(item_delete, "activate", G_CALLBACK(menu_delete_cb), this);
    g_signal_connect(item_mark, "activate", G_CALLBACK(menu_mark_cb), this);
    g_signal_connect(item_unmark, "activate", G_CALLBACK(menu_unmark_cb), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_delete);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_mark);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_unmark);
    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
}

gboolean PageSorter::button_press_cb(GtkWidget * w, GdkEventButton * ev, gpointer data) {
    PageSorter * self = (PageSorter *)data;
    if (ev->button == 3) {
	self->showContextMenu(int(ev->x_root), int(ev->y_root));
	return TRUE;
    }
    return FALSE;
}

// --------------------------------------------------------------------
