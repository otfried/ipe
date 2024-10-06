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

class JsPainter : public Painter {
public:
  JsPainter(const Cascade *sheet, emscripten::val context);
  virtual ~JsPainter();

protected:
  virtual void doNewPath();
  virtual void doMoveTo(const Vector &v);
  virtual void doLineTo(const Vector &v);
  virtual void doCurveTo(const Vector &v1, const Vector &v2,
			 const Vector &v3);
  virtual void doClosePath();

  virtual void doDrawPath(TPathMode mode);

private:
  emscripten::val iCtx;
};

JsPainter::JsPainter(const Cascade *sheet, emscripten::val context)
  : Painter(sheet), iCtx{context}
{
  // nothing
}

JsPainter::~JsPainter()
{
  // nothing
}

void JsPainter::doNewPath()
{
  iCtx.call<void>("beginPath");
}

void JsPainter::doMoveTo(const Vector &v)
{
  iCtx.call<void>("moveTo", v.x, v.y);
}

void JsPainter::doLineTo(const Vector &v)
{
  iCtx.call<void>("lineTo", v.x, v.y);
}

void JsPainter::doCurveTo(const Vector &v1, const Vector &v2, const Vector &v3)
{
  iCtx.call<void>("bezierCurveTo", v1.x, v1.y, v2.x, v2.y, v3.x, v3.y);
}

void JsPainter::doClosePath()
{
  iCtx.call<void>("closePath");
}

void JsPainter::doDrawPath(TPathMode mode)
{
  auto colorString = [](Color color) {
    String result;
    StringStream ss{result};
    ss << "#";
    ss.putHexByte(int(color.iRed.internal() * 255 / 1000));
    ss.putHexByte(int(color.iGreen.internal() * 255 / 1000));
    ss.putHexByte(int(color.iBlue.internal() * 255 / 1000));
    return result;
  };
  if (mode >= EStrokedAndFilled) {
    String fill1 = colorString(fill());
    iCtx.set("fillStyle", std::string(fill1.z()));
    iCtx.call<void>("fill");
  }
  if (mode <= EStrokedAndFilled) {
    String stroke1 = colorString(stroke());
    iCtx.set("strokeStyle", std::string(stroke1.z()));
    iCtx.call<void>("stroke");
  }
}

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

static int convertModifiers(emscripten::val ev)
{
  int mod = 0;
  if (ev["shiftKey"].as<bool>())
    mod |= CanvasBase::EShift;
  if (ev["ctrlKey"].as<bool>())
    mod |= CanvasBase::EControl;
  if (ev["altKey"].as<bool>())
    mod |= CanvasBase::EAlt;
  if (ev["metaKey"].as<bool>())
    mod |= CanvasBase::EMeta;
  return mod;
}

void Canvas::mouseButtonEvent(emscripten::val ev, int button, bool press)
{
  iGlobalPos = Vector(ev["screenX"].as<double>(), ev["screenY"].as<double>());
  computeFifi(ev["offsetX"].as<double>(), ev["offsetY"].as<double>());
  int mod = convertModifiers(ev) | iAdditionalModifiers;
  // ipeDebug("mouseButton %g, %g %d %d %d", iUnsnappedMousePos.x, iUnsnappedMousePos.y, press, button, mod);
  if (iTool)
    iTool->mouseButton(button | mod, press);
  else if (press && iObserver)
    iObserver->canvasObserverMouseAction(button | mod);
}

void Canvas::mouseMoveEvent(emscripten::val ev)
{
  computeFifi(ev["offsetX"].as<double>(), ev["offsetY"].as<double>());
  // ipeDebug("mouseMove %g, %g", iUnsnappedMousePos.x, iUnsnappedMousePos.y);
  if (iTool)
    iTool->mouseMove();
  if (iObserver)
    iObserver->canvasObserverPositionChanged();
}

void Canvas::wheelEvent(emscripten::val ev)
{
  Vector p{ev["deltaX"].as<double>(), ev["deltaY"].as<double>()};
  int mod = convertModifiers(ev);
  // ipeDebug("wheel %g, %g %d", p.x, p.y, mod);
  int kind = (mod & CanvasBase::EControl) ? 2 : 0;
  if (iObserver) {
    if (mod & CanvasBase::EShift)
      // switch x and y
      iObserver->canvasObserverWheelMoved(p.y / 8.0, p.x / 8.0, kind);
    else
      iObserver->canvasObserverWheelMoved(p.x / 8.0, p.y / 8.0, kind);
  }
}

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

// TODO: make two separate canvasses that are overlaid,
// use the bottom one for the cairo surface and the top one for the tool,
// saving a lot of redrawing

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
  // if (iFifiVisible)
  // drawFifi(qPainter);
  if (iPage) {
    JsPainter qp(iCascade, iCtx);
    qp.transform(Linear(2, 0, 0, 2));
    qp.transform(canvasTfm());
    qp.pushMatrix();
    drawTool(qp);
    qp.popMatrix();
  }
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
    .property("zoom", &Canvas::zoom, &Canvas::setZoom)
    .function("mouseButtonEvent", &Canvas::mouseButtonEvent)
    .function("mouseMoveEvent", &Canvas::mouseMoveEvent)
    .function("wheelEvent", &Canvas::wheelEvent);
}

// --------------------------------------------------------------------
