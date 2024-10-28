// --------------------------------------------------------------------
// ipe::Canvas
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

#include "ipecanvas.h"
#include "ipetool.h"

#include "ipecairopainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \defgroup canvas Ipe canvas
  \brief A widget (control) that displays an Ipe document page.

  This module contains the classes needed to display and edit Ipe
  objects using the selected toolkit.

  These classes are not in Ipelib, but in a separate library
  libipecanvas.
*/

// --------------------------------------------------------------------

CanvasObserver::~CanvasObserver() { /* nothing */ }
void CanvasObserver::canvasObserverWheelMoved(double xDegrees, double yDegrees,
					      int kind) { /* nothing */ }
void CanvasObserver::canvasObserverMouseAction(int button) { /* nothing */ }
void CanvasObserver::canvasObserverPositionChanged() { /* nothing */ }
void CanvasObserver::canvasObserverToolChanged(bool hasTool) { /* nothing */ }
void CanvasObserver::canvasObserverSizeChanged() { /* nothing */ }

/*! \class ipe::Canvas
  \ingroup canvas
  \brief A widget (control) that displays an Ipe document page.
*/

//! Construct a new canvas.
CanvasBase::CanvasBase() {
    iObserver = nullptr;
    iTool = nullptr;
    iPage = nullptr;
    iCascade = nullptr;
    iSurface = nullptr;
    iPan = Vector::ZERO;
    iZoom = 1.0;
    iDimmed = false;
    iWidth = 0; // not yet known (canvas is not yet mapped)
    iHeight = 0;
    iBWidth = 0;
    iBHeight = 0;
    iRepaintObjects = false;
    iResources = nullptr;
    iAutoSnap = false;
    iFifiVisible = false;
    iFifiMode = Snap::ESnapNone;
    iSelectionVisible = true;

    iType3Font = false;

    isInkMode = false;
    iAdditionalModifiers = 0;

    iStyle.paperColor = Color(1000, 1000, 1000);
    iStyle.primarySelectionColor = Color(1000, 0, 0);
    iStyle.secondarySelectionColor = Color(1000, 0, 1000);
    iStyle.selectionSurroundColor = Color(1000, 1000, 0);
    iStyle.gridLineColor = Color(300, 300, 300);
    iStyle.primarySelectionWidth = 3.0;
    iStyle.secondarySelectionWidth = 2.0;
    iStyle.selectionSurroundWidth = 6.0;
    iStyle.pretty = false;
    iStyle.classicGrid = false;
    iStyle.thinLine = 0.2;
    iStyle.thickLine = 0.9;
    iStyle.thinStep = 1;
    iStyle.thickStep = 4;
    iStyle.paperClip = false;
    iStyle.numberPages = false;

    iSnap.iSnap = 0;
    iSnap.iGridVisible = false;
    iSnap.iGridSize = 8;
    iSnap.iAngleSize = IPE_PI / 6.0;
    iSnap.iSnapDistance = 10;
    iSnap.iWithAxes = false;
    iSnap.iOrigin = Vector::ZERO;
    iSnap.iDir = 0;
}

//! destructor.
CanvasBase::~CanvasBase() {
    if (iSurface) {
	ipeDebug("Surface has %d references left",
		 cairo_surface_get_reference_count(iSurface));
	cairo_surface_finish(iSurface);
	cairo_surface_destroy(iSurface);
    }
    delete iTool;
    ipeDebug("CanvasBase::~CanvasBase");
}

// --------------------------------------------------------------------

//! set information about Latex fonts (from ipe::Document)
void CanvasBase::setResources(const PdfResources * resources) {
    iFonts.reset();
    iResources = resources;
    iFonts = std::make_unique<Fonts>(resources);
}

bool CanvasBase::type3Font() {
    bool result = iType3Font;
    iType3Font = false;
    return result;
}

// --------------------------------------------------------------------

//! Set the page to be displayed.
/*! Doesn't take ownership of any argument.
  The page number \a pno is only needed if page numbering is turned on.
*/
void CanvasBase::setPage(const Page * page, int pno, int view, const Cascade * sheet) {
    iPage = page;
    iPageNumber = pno;
    iView = view;
    iCascade = sheet;
}

//! Set style of canvas drawing.
/*! Includes paper color, pretty text, and grid. */
void CanvasBase::setCanvasStyle(const Style & style) { iStyle = style; }

//! Set current pan position.
/*! The pan position is the user coordinate that is displayed at
  the very center of the canvas. */
void CanvasBase::setPan(const Vector & v) { iPan = v; }

//! Set current zoom factor.
/*! The zoom factor maps user coordinates to screen pixel coordinates. */
void CanvasBase::setZoom(double zoom) { iZoom = zoom; }

//! Set the snapping information.
void CanvasBase::setSnap(const Snap & s) { iSnap = s; }

//! Dim whole canvas, except for the Tool.
/*! This mode will be reset when the Tool finishes. */
void CanvasBase::setDimmed(bool dimmed) { iDimmed = dimmed; }

//! Set additional modifiers.
/*! These modifier bits are passed to the Tool when a key is pressed
  or a drawing action is performed in addition to the actual
  keyboard modifiers. */
void CanvasBase::setAdditionalModifiers(int mod) { iAdditionalModifiers = mod; }

//! Enable automatic angular snapping with this origin.
void CanvasBase::setAutoOrigin(const Vector & v) {
    iAutoOrigin = v;
    iAutoSnap = true;
}

//! Convert canvas (device) coordinates to user coordinates.
Vector CanvasBase::devToUser(const Vector & arg) const {
    Vector v = arg - center();
    v.x /= iZoom;
    v.y /= -iZoom;
    v += iPan;
    return v;
}

//! Convert user coordinates to canvas (device) coordinates.
Vector CanvasBase::userToDev(const Vector & arg) const {
    Vector v = arg - iPan;
    v.x *= iZoom;
    v.y *= -iZoom;
    v += center();
    return v;
}

//! Matrix mapping user coordinates to canvas coordinates
Matrix CanvasBase::canvasTfm() const {
    return Matrix(center()) * Linear(iZoom, 0, 0, -iZoom) * Matrix(-iPan);
}

// --------------------------------------------------------------------

void CanvasBase::drawAxes(cairo_t * cc) {
    double alpha = 0.0;
    double ep = (iWidth + iHeight) / iZoom;

    cairo_save(cc);
    cairo_set_source_rgb(cc, 0.0, 1.0, 0.0);
    cairo_set_line_width(cc, 2.0 / iZoom);
    while (alpha < IpeTwoPi) {
	double beta = iSnap.iDir + alpha;
	cairo_move_to(cc, iSnap.iOrigin.x, iSnap.iOrigin.y);
	Vector dir(beta);
	cairo_rel_line_to(cc, ep * dir.x, ep * dir.y);
	if (alpha == 0.0) {
	    cairo_stroke(cc);
	    cairo_set_line_width(cc, 1.0 / iZoom);
	}
	alpha += iSnap.iAngleSize;
    }
    cairo_stroke(cc);
    cairo_restore(cc);
}

void CanvasBase::drawGrid(cairo_t * cc) {
    int step = iSnap.iGridSize * iStyle.thinStep;
    double pixstep = step * iZoom;
    if (pixstep < 3.0) return;

    // Rect paper = iCascade->findLayout()->paper();
    // Vector ll = paper.bottomLeft();
    // Vector ur = paper.topRight();
    Vector ll = Vector::ZERO;
    Vector ur = iCascade->findLayout()->iFrameSize;

    int left = step * int(ll.x / step);
    if (left < ll.x) ++left;
    int bottom = step * int(ll.y / step);
    if (bottom < ll.y) ++bottom;

    // only draw lines that intersect canvas
    Vector screenUL = devToUser(Vector::ZERO);
    Vector screenLR = devToUser(Vector(iWidth, iHeight));

    cairo_save(cc);
    cairo_set_source_rgb(cc, iStyle.gridLineColor.iRed.toDouble(),
			 iStyle.gridLineColor.iGreen.toDouble(),
			 iStyle.gridLineColor.iBlue.toDouble());

    if (iStyle.classicGrid) {
	double lw = iStyle.thinLine / iZoom;
	cairo_set_line_width(cc, lw);
	for (int y = bottom; y < ur.y; y += step) {
	    if (screenLR.y <= y && y <= screenUL.y) {
		for (int x = left; x < ur.x; x += step) {
		    if (screenUL.x <= x && x <= screenLR.x) {
			cairo_move_to(cc, x, y - 0.5 * lw);
			cairo_line_to(cc, x, y + 0.5 * lw);
			cairo_stroke(cc);
		    }
		}
	    }
	}
    } else {
	double thinLine = iStyle.thinLine / iZoom;
	double thickLine = iStyle.thickLine / iZoom;
	int thickStep = iStyle.thickStep * step;

	// draw horizontal lines
	for (int y = bottom; y < ur.y; y += step) {
	    if (screenLR.y <= y && y <= screenUL.y) {
		cairo_set_line_width(cc, (y % thickStep) ? thinLine : thickLine);
		cairo_move_to(cc, ll.x, y);
		cairo_line_to(cc, ur.x, y);
		cairo_stroke(cc);
	    }
	}

	// draw vertical lines
	for (int x = left; x < ur.x; x += step) {
	    if (screenUL.x <= x && x <= screenLR.x) {
		cairo_set_line_width(cc, (x % thickStep) ? thinLine : thickLine);
		cairo_move_to(cc, x, ll.y);
		cairo_line_to(cc, x, ur.y);
		cairo_stroke(cc);
	    }
	}
    }

    cairo_restore(cc);
}

void CanvasBase::drawPaper(cairo_t * cc) {
    const Layout * l = iCascade->findLayout();
    cairo_rectangle(cc, -l->iOrigin.x, -l->iOrigin.y, l->iPaperSize.x, l->iPaperSize.y);
    cairo_set_source_rgb(cc, iStyle.paperColor.iRed.toDouble(),
			 iStyle.paperColor.iGreen.toDouble(),
			 iStyle.paperColor.iBlue.toDouble());
    cairo_fill(cc);
}

void CanvasBase::drawFrame(cairo_t * cc) {
    const Layout * l = iCascade->findLayout();
    cairo_set_source_rgb(cc, 0.5, 0.5, 0.5);
    cairo_save(cc);
    double dashes[2] = {3.0 / iZoom, 7.0 / iZoom};
    cairo_set_dash(cc, dashes, 2, 0.0);
    cairo_set_line_width(cc, 2.5 / iZoom);
    cairo_move_to(cc, 0.0, 0.0);
    cairo_line_to(cc, 0.0, l->iFrameSize.y);
    cairo_line_to(cc, l->iFrameSize.x, l->iFrameSize.y);
    cairo_line_to(cc, l->iFrameSize.x, 0);
    cairo_close_path(cc);
    cairo_stroke(cc);
    cairo_restore(cc);
}

void CanvasBase::drawObjects(cairo_t * cc) {
    if (!iPage) return;

    if (iStyle.paperClip) {
	const Layout * l = iCascade->findLayout();
	cairo_rectangle(cc, -l->iOrigin.x, -l->iOrigin.y, l->iPaperSize.x,
			l->iPaperSize.y);
	cairo_clip(cc);
    }

    CairoPainter painter(iCascade, iFonts.get(), cc, iZoom, iStyle.pretty, false);
    painter.setDimmed(iDimmed);
    const auto viewMap = iPage->viewMap(iView, iCascade);
    painter.setAttributeMap(&viewMap);
    std::vector<Matrix> layerMatrices = iPage->layerMatrices(iView);
    painter.pushMatrix();

    const Symbol * background = iCascade->findSymbol(iPage->backgroundSymbol(iCascade));
    if (background && iPage->findLayer("BACKGROUND") < 0)
	background->iObject->draw(painter);

    if (iResources && iStyle.numberPages) {
	const Text * pn = iResources->pageNumber(iPageNumber, iView);
	if (pn) pn->draw(painter);
    }

    const Text * title = iPage->titleText();
    if (title) title->draw(painter);

    for (int i = 0; i < iPage->count(); ++i) {
	if (iPage->objectVisible(iView, i)) {
	    painter.pushMatrix();
	    painter.transform(layerMatrices[iPage->layerOf(i)]);
	    iPage->object(i)->draw(painter);
	    painter.popMatrix();
	}
    }
    painter.popMatrix();
    if (painter.type3Font()) iType3Font = true;
}

// --------------------------------------------------------------------

static void draw_plus(const Vector & p, cairo_t * cr) {
    cairo_move_to(cr, p.x - 8, p.y);
    cairo_line_to(cr, p.x + 8, p.y);
    cairo_move_to(cr, p.x, p.y - 8);
    cairo_line_to(cr, p.x, p.y + 8);
    cairo_stroke(cr);
}

static void draw_rhombus(const Vector & p, cairo_t * cr) {
    cairo_move_to(cr, p.x - 8, p.y);
    cairo_line_to(cr, p.x, p.y + 8);
    cairo_line_to(cr, p.x + 8, p.y);
    cairo_line_to(cr, p.x, p.y - 8);
    cairo_close_path(cr);
    cairo_stroke(cr);
}

static void draw_square(const Vector & p, cairo_t * cr) {
    cairo_move_to(cr, p.x - 7, p.y - 7);
    cairo_line_to(cr, p.x + 7, p.y - 7);
    cairo_line_to(cr, p.x + 7, p.y + 7);
    cairo_line_to(cr, p.x - 7, p.y + 7);
    cairo_close_path(cr);
    cairo_stroke(cr);
}

static void draw_x(const Vector & p, cairo_t * cr) {
    cairo_move_to(cr, p.x - 5.6, p.y - 5.6);
    cairo_line_to(cr, p.x + 5.6, p.y + 5.6);
    cairo_move_to(cr, p.x - 5.6, p.y + 5.6);
    cairo_line_to(cr, p.x + 5.6, p.y - 5.6);
    cairo_stroke(cr);
}

static void draw_star(const Vector & p, cairo_t * cr) {
    cairo_move_to(cr, p.x - 8, p.y);
    cairo_line_to(cr, p.x + 8, p.y);
    cairo_move_to(cr, p.x - 4, p.y + 7);
    cairo_line_to(cr, p.x + 4, p.y - 7);
    cairo_move_to(cr, p.x - 4, p.y - 7);
    cairo_line_to(cr, p.x + 4, p.y + 7);
    cairo_stroke(cr);
}

void CanvasBase::drawFifi(cairo_t * cr) {
    Vector p = userToDev(iMousePos);
    switch (iFifiMode) {
    case Snap::ESnapNone:
	// don't draw at all
	break;
    case Snap::ESnapVtx:
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	draw_rhombus(p, cr);
	break;
    case Snap::ESnapCtl:
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	draw_square(p, cr);
	break;
    case Snap::ESnapBd:
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	draw_plus(p, cr);
	break;
    case Snap::ESnapInt:
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	draw_x(p, cr);
	break;
    case Snap::ESnapGrid:
	cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
	draw_plus(p, cr);
	break;
    case Snap::ESnapAngle:
    case Snap::ESnapAuto:
    default:
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
	draw_star(p, cr);
	break;
    }
    iOldFifi = p;
}

// --------------------------------------------------------------------

//! Draw the current canvas tool.
/*! If no tool is set, it draws the selected objects. */
void CanvasBase::drawTool(Painter & painter) {
    if (iTool) {
	iTool->draw(painter);
    } else if (iSelectionVisible) {
	for (int i = 0; i < iPage->count(); ++i) {
	    if (iPage->objectVisible(iView, i)) {
		if (iPage->select(i) == EPrimarySelected) {
		    painter.setStroke(Attribute(iStyle.selectionSurroundColor));
		    painter.setPen(Attribute(Fixed(iStyle.selectionSurroundWidth)));
		    iPage->object(i)->drawSimple(painter);
		    painter.setStroke(Attribute(iStyle.primarySelectionColor));
		    painter.setPen(Attribute(Fixed(iStyle.primarySelectionWidth)));
		    iPage->object(i)->drawSimple(painter);
		} else if (iPage->select(i) == ESecondarySelected) {
		    painter.setStroke(Attribute(iStyle.selectionSurroundColor));
		    painter.setPen(Attribute(Fixed(iStyle.selectionSurroundWidth)));
		    iPage->object(i)->drawSimple(painter);
		    painter.setStroke(Attribute(iStyle.secondarySelectionColor));
		    painter.setPen(Attribute(Fixed(iStyle.secondarySelectionWidth)));
		    iPage->object(i)->drawSimple(painter);
		}
	    }
	}
    }
}

//! Set an observer.
/*! Use 0 to delete current observer. */
void CanvasBase::setObserver(CanvasObserver * observer) { iObserver = observer; }

//! Set a new tool.
/*! Calls canvasObserverToolChanged(). */
void CanvasBase::setTool(Tool * tool) {
    assert(tool);
    iTool = tool;
    updateTool();
    if (iObserver) iObserver->canvasObserverToolChanged(true);
}

// Current tool has done its job.
/* Tool is deleted, canvas fully updated, and cursor reset.
   Calls canvasObserverToolChanged(). */
void CanvasBase::finishTool() {
    delete iTool;
    iTool = nullptr;
    iDimmed = false;
    iAutoSnap = false;
    update();
    if (iSelectionVisible) setCursor(EStandardCursor);
    if (iObserver) iObserver->canvasObserverToolChanged(false);
}

// --------------------------------------------------------------------

void CanvasBase::snapToPaperAndFrame() {
    double snapDist = iSnap.iSnapDistance / iZoom;
    double d = snapDist;
    Vector fifi = iMousePos;
    const Layout * layout = iCascade->findLayout();
    Rect paper = layout->paper();
    Rect frame(Vector::ZERO, layout->iFrameSize);

    // vertices
    if (iSnap.iSnap & Snap::ESnapVtx) {
	paper.bottomLeft().snap(iMousePos, fifi, d);
	paper.topRight().snap(iMousePos, fifi, d);
	paper.topLeft().snap(iMousePos, fifi, d);
	paper.bottomRight().snap(iMousePos, fifi, d);
	frame.bottomLeft().snap(iMousePos, fifi, d);
	frame.topRight().snap(iMousePos, fifi, d);
	frame.topLeft().snap(iMousePos, fifi, d);
	frame.bottomRight().snap(iMousePos, fifi, d);
    }

    // Return if snapping has occurred
    if (d < snapDist) {
	iMousePos = fifi;
	iFifiMode = Snap::ESnapVtx;
	return;
    }

    // boundary
    if (iSnap.iSnap & Snap::ESnapBd) {
	Segment(paper.bottomLeft(), paper.bottomRight()).snap(iMousePos, fifi, d);
	Segment(paper.bottomRight(), paper.topRight()).snap(iMousePos, fifi, d);
	Segment(paper.topRight(), paper.topLeft()).snap(iMousePos, fifi, d);
	Segment(paper.topLeft(), paper.bottomLeft()).snap(iMousePos, fifi, d);
	Segment(frame.bottomLeft(), frame.bottomRight()).snap(iMousePos, fifi, d);
	Segment(frame.bottomRight(), frame.topRight()).snap(iMousePos, fifi, d);
	Segment(frame.topRight(), frame.topLeft()).snap(iMousePos, fifi, d);
	Segment(frame.topLeft(), frame.bottomLeft()).snap(iMousePos, fifi, d);
    }

    if (d < snapDist) {
	iMousePos = fifi;
	iFifiMode = Snap::ESnapBd;
    }
}

// --------------------------------------------------------------------

//! Set whether Fifi should be shown.
/*! Fifi will only be shown if a snapping mode is active. */
void CanvasBase::setFifiVisible(bool visible) {
    iFifiVisible = visible;
    if (!visible) updateTool(); // when making visible, wait for position update
}

//! Set whether selection should be shown when there is no tool.
void CanvasBase::setSelectionVisible(bool visible) {
    iSelectionVisible = visible;
    updateTool();
}

//! Return snapped mouse position without angular snapping.
Vector CanvasBase::simpleSnapPos() const {
    Vector pos = iUnsnappedMousePos;
    iSnap.simpleSnap(pos, iPage, iView, iSnap.iSnapDistance / iZoom);
    return pos;
}

// --------------------------------------------------------------------

//! Mark for update with redrawing of objects.
void CanvasBase::update() {
    iRepaintObjects = true;
    invalidate();
}

//! Mark for update with redrawing of tool only.
void CanvasBase::updateTool() { invalidate(); }

// --------------------------------------------------------------------

/*! Stores the mouse position in iUnsnappedMousePos, computes Fifi if
  snapping is enabled, and stores snapped position in iMousePos. */
void CanvasBase::computeFifi(double x, double y) {
    iUnsnappedMousePos = devToUser(Vector(x, y));
    iMousePos = iUnsnappedMousePos;

    if (!iPage) return;

    int mask = iAutoSnap ? 0 : Snap::ESnapAuto;
    if (iSnap.iSnap & ~mask) {
	iFifiMode = iSnap.snap(iMousePos, iPage, iView, iSnap.iSnapDistance / iZoom,
			       iTool, (iAutoSnap ? &iAutoOrigin : nullptr));
	if (iFifiMode == Snap::ESnapNone) snapToPaperAndFrame();

	// convert fifi coordinates back into device space
	Vector fifi = userToDev(iMousePos);
	if (iFifiVisible && fifi != iOldFifi) {
	    invalidate(int(iOldFifi.x - 10), int(iOldFifi.y - 10), 21, 21);
	    invalidate(int(fifi.x - 10), int(fifi.y - 10), 21, 21);
	}
    } else if (iFifiVisible) {
	// remove old fifi
	invalidate(int(iOldFifi.x - 10), int(iOldFifi.y - 10), 21, 21);
	iFifiVisible = false;
    }
}

// --------------------------------------------------------------------

bool CanvasBase::refreshSurface() {
    if (!iSurface || iBWidth != cairo_image_surface_get_width(iSurface)
	|| iBHeight != cairo_image_surface_get_height(iSurface)) {
	// size has changed
	ipeDebug("size has changed to %g x %g (%g x %g)", iWidth, iHeight, iBWidth,
		 iBHeight);
	if (iSurface) cairo_surface_destroy(iSurface);
	iSurface = nullptr;
	iRepaintObjects = true;
	// give Ipe a chance to set pan and zoom according to new size
	if (iObserver) iObserver->canvasObserverSizeChanged();
    }
    if (iRepaintObjects) {
	iRepaintObjects = false;
	if (!iSurface)
	    iSurface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, iBWidth, iBHeight);
	cairo_t * cc = cairo_create(iSurface);
	// background
	cairo_set_source_rgb(cc, 0.4, 0.4, 0.4);
	cairo_rectangle(cc, 0, 0, iBWidth, iBHeight);
	cairo_fill(cc);

	cairo_translate(cc, 0.5 * iBWidth, 0.5 * iBHeight);
	cairo_scale(cc, iBWidth / iWidth, iBHeight / iHeight);
	cairo_scale(cc, iZoom, -iZoom);
	cairo_translate(cc, -iPan.x, -iPan.y);

	if (iPage) {
	    drawPaper(cc);
	    if (!iStyle.pretty) drawFrame(cc);
	    if (iSnap.iGridVisible) drawGrid(cc);
	    drawObjects(cc);
	    if (iSnap.iWithAxes) drawAxes(cc);
	}
	cairo_surface_flush(iSurface);
	cairo_destroy(cc);
	return true;
    } else
	return false;
}

// --------------------------------------------------------------------
