// --------------------------------------------------------------------
// ipe::Canvas for HTML, Javascript, and webassembly
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

#include "ipecanvas_js.h"
#include "ipepainter.h"
#include "ipetool.h"

#include <cairo.h>

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace ipe;
using namespace emscripten;

// --------------------------------------------------------------------

//! Construct a new canvas.
Canvas::Canvas(val canvas, double dpr)
{
  iCanvas = canvas;
  iBWidth = iCanvas["width"].as<double>();
  iBHeight = iCanvas["height"].as<double>();

  iWidth = iBWidth / dpr;
  iHeight = iBHeight / dpr;

  iCtx = iCanvas.call<val>("getContext", val("2d"));
  ipeDebug("Canvas has size: %g x %g (%g x %g)",
	   iWidth, iHeight, iBWidth, iBHeight);
}

void Canvas::setCursor(TCursor cursor, double w, Color *color)
{
  // not implemented
}

// --------------------------------------------------------------------

void Canvas::invalidate()
{
  paint();
}

void Canvas::invalidate(int x, int y, int w, int h)
{
  paint();
}

// --------------------------------------------------------------------

/*
static int convertModifiers(int qmod)
{
  int mod = 0;
  if (qmod & Qt::ShiftModifier)
    mod |= CanvasBase::EShift;
  if (qmod & Qt::ControlModifier)
    mod |= CanvasBase::EControl;
  if (qmod & Qt::AltModifier)
    mod |= CanvasBase::EAlt;
  if (qmod & Qt::MetaModifier)
    mod |= CanvasBase::EMeta;
  return mod;
}
*/

#if 0
void Canvas::keyPressEvent(QKeyEvent *ev)
{
  if (iTool &&
      iTool->key(IpeQ(ev->text()),
		 (convertModifiers(ev->modifiers()) | iAdditionalModifiers)))
    ev->accept();
  else
    ev->ignore();
}

void Canvas::mouseButton(QMouseEvent *ev, int button, bool press)
{
  iGlobalPos = Vector(ev->globalPosition().x(), ev->globalPosition().y());
  computeFifi(ev->position().x(), ev->position().y());
  int mod = convertModifiers(ev->modifiers()) | iAdditionalModifiers;
  if (iTool)
    iTool->mouseButton(button | mod, press);
  else if (press && iObserver)
    iObserver->canvasObserverMouseAction(button | mod);
}

void Canvas::mousePressEvent(QMouseEvent *ev)
{
  mouseButton(ev, ev->button(), true);
}

void Canvas::mouseReleaseEvent(QMouseEvent *ev)
{
  mouseButton(ev, ev->button(), false);
}

void Canvas::mouseMoveEvent(QMouseEvent *ev)
{
  computeFifi(ev->position().x(), ev->position().y());
  if (iTool)
    iTool->mouseMove();
  if (iObserver)
    iObserver->canvasObserverPositionChanged();
}

void Canvas::wheelEvent(QWheelEvent *ev)
{
  QPoint p = ev->angleDelta();
  int kind = (ev->modifiers() & Qt::ControlModifier) ? 2 : 0;
  if (iObserver) {
    if (ev->modifiers() & Qt::ShiftModifier)
      // switch x and y
      iObserver->canvasObserverWheelMoved(p.y() / 8.0, p.x() / 8.0, kind);
    else
      iObserver->canvasObserverWheelMoved(p.x() / 8.0, p.y() / 8.0, kind);
  }
  ev->accept();
}

static void draw_plus(const Vector &p, QPainter &q)
{
  q.drawLine(QPt(p - Vector(8, 0)), QPt(p + Vector(8, 0)));
  q.drawLine(QPt(p - Vector(0, 8)), QPt(p + Vector(0, 8)));
}

static void draw_rhombus(const Vector &p, QPainter &q)
{
 QPainterPath path;
 path.moveTo(QPt(p - Vector(8, 0)));
 path.lineTo(QPt(p + Vector(0, 8)));
 path.lineTo(QPt(p + Vector(8, 0)));
 path.lineTo(QPt(p + Vector(0, -8)));
 path.closeSubpath();
 q.drawPath(path);
}

static void draw_square(const Vector &p, QPainter &q)
{
 QPainterPath path;
 path.moveTo(QPt(p + Vector(-7, -7)));
 path.lineTo(QPt(p + Vector(7, -7)));
 path.lineTo(QPt(p + Vector(7, 7)));
 path.lineTo(QPt(p + Vector(-7, 7)));
 path.closeSubpath();
 q.drawPath(path);
}

static void draw_x(const Vector &p, QPainter &q)
{
  q.drawLine(QPt(p - Vector(5.6, 5.6)),
	     QPt(p + Vector(5.6, 5.6)));
  q.drawLine(QPt(p - Vector(5.6, -5.6)),
	     QPt(p + Vector(5.6, -5.6)));
}

static void draw_star(const Vector &p, QPainter &q)
{
  q.drawLine(QPt(p - Vector(8, 0)), QPt(p + Vector(8, 0)));
  q.drawLine(QPt(p + Vector(-4, 7)), QPt(p + Vector(4, -7)));
  q.drawLine(QPt(p + Vector(-4, -7)), QPt(p + Vector(4, 7)));
}

void Canvas::drawFifi(QPainter &q)
{
  Vector p = userToDev(iMousePos);
  switch (iFifiMode) {
  case Snap::ESnapNone:
    // don't draw at all
    break;
  case Snap::ESnapVtx:
    q.setPen(QColor(255, 0, 0, 255));
    draw_rhombus(p, q);
    break;
  case Snap::ESnapCtl:
    q.setPen(QColor(255, 0, 0, 255));
    draw_square(p, q);
    break;
    break;
  case Snap::ESnapBd:
    q.setPen(QColor(255, 0, 0, 255));
    draw_plus(p, q);
    break;
  case Snap::ESnapInt:
    q.setPen(QColor(255, 0, 0, 255));
    draw_x(p, q);
    break;
  case Snap::ESnapGrid:
    q.setPen(QColor(0, 128, 0, 255));
    draw_plus(p, q);
    break;
  case Snap::ESnapAngle:
  case Snap::ESnapAuto:
  default:
    q.setPen(QColor(255, 0, 0, 255));
    draw_star(p, q);
    break;
  }
  iOldFifi = p;
}
#endif

void Canvas::paint()
{
  refreshSurface();

  // TODO: maybe call a single JS function to do all of this at once?
  val buffer1 = val(typed_memory_view(iBWidth * iBHeight * 4, cairo_image_surface_get_data(iSurface)));
  val Uint8ClampedArray = val::global("Uint8ClampedArray");
  val buffer2 = Uint8ClampedArray.new_(buffer1["buffer"],
				       buffer1["byteOffset"],
				       buffer1["byteLength"]);
  val ImageData = val::global("ImageData");
  val img = ImageData.new_(buffer2, iBWidth, iBHeight);
  iCtx.call<void>("putImageData", img, 0, 0);
  /*
  if (iFifiVisible)
    drawFifi(qPainter);
  if (iPage) {
    IpeQtPainter qp(iCascade, &qPainter);
    qp.transform(canvasTfm());
    qp.pushMatrix();
    drawTool(qp);
    qp.popMatrix();
  }
  */
}

namespace {
  void setPage(Canvas * canvas, Page *page, int pno, int view, Cascade *sheet) {
    return canvas->setPage(page, pno, view, sheet);
  }
  void updateCanvas(Canvas * canvas) {
    return canvas->update();
  }
}

// --------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(ipecanvas) {
  emscripten::class_<Canvas>("Canvas")
    .constructor<val, double>()
    .function("update", &updateCanvas, emscripten::allow_raw_pointers())
    .property("width", &Canvas::canvasWidth)
    .property("height", &Canvas::canvasHeight)
    .function("setPage", &setPage, emscripten::allow_raw_pointers())
    .property("pan", &Canvas::pan, &Canvas::setPan)
    .property("zoom", &Canvas::zoom, &Canvas::setZoom);
}

// --------------------------------------------------------------------
