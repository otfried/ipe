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

using namespace ipe;
using namespace emscripten;

// --------------------------------------------------------------------

namespace ipe {
  
class JsPainter : public Painter {
public:
  JsPainter(const Cascade *sheet, emscripten::val context, double dpr);
  virtual ~JsPainter();

  void setPen(int r, int g, int b);
  String colorString(int r, int g, int b);

  void drawLine(const Vector & p1, const Vector & p2);
  void drawPath(const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4);

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
  double iDpr;
};

} // namespace ipe

JsPainter::JsPainter(const Cascade *sheet, emscripten::val context, double dpr)
  : Painter(sheet), iCtx{context}, iDpr{dpr}
{
  // adjust for display's pixel ratio
  transform(Linear(iDpr, 0, 0, iDpr));
}

JsPainter::~JsPainter()
{
  // nothing
}

String JsPainter::colorString(int r, int g, int b)
{
  String result;
  StringStream ss{result};
  ss << "#";
  ss.putHexByte(r);
  ss.putHexByte(g);
  ss.putHexByte(b);
  return result;
}

void JsPainter::setPen(int r, int g, int b)
{
  String rgb = colorString(r, g, b);
  iCtx.set("strokeStyle", std::string(rgb.z()));
  iCtx.set("lineWidth", iDpr);
}

void JsPainter::drawLine(const Vector & v1, const Vector & v2)
{
  iCtx.call<void>("beginPath");
  iCtx.call<void>("moveTo", v1.x * iDpr, v1.y * iDpr);
  iCtx.call<void>("lineTo", v2.x * iDpr, v2.y * iDpr);
  iCtx.call<void>("stroke");
}

void JsPainter::drawPath(const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4)
{
  iCtx.call<void>("beginPath");
  iCtx.call<void>("moveTo", v1.x * iDpr, v1.y * iDpr);
  iCtx.call<void>("lineTo", v2.x * iDpr, v2.y * iDpr);
  iCtx.call<void>("lineTo", v3.x * iDpr, v3.y * iDpr);
  iCtx.call<void>("lineTo", v4.x * iDpr, v4.y * iDpr);
  iCtx.call<void>("closePath");
  iCtx.call<void>("stroke");
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
  auto colorString1 = [this](Color color) {
    return colorString(int(color.iRed.internal() * 255 / 1000),
		       int(color.iGreen.internal() * 255 / 1000),
		       int(color.iBlue.internal() * 255 / 1000));
  };
  if (mode >= EStrokedAndFilled) {
    String fill1 = colorString1(fill());
    iCtx.set("fillStyle", std::string(fill1.z()));
    iCtx.call<void>("fill");
  }
  if (mode <= EStrokedAndFilled) {
    String stroke1 = colorString1(stroke());
    iCtx.set("strokeStyle", std::string(stroke1.z()));
    iCtx.set("lineWidth", iDpr * pen().toDouble());
    iCtx.call<void>("stroke");
  }
}

// --------------------------------------------------------------------

EM_JS(void, addIpeCanvasJS, (), {
function ipeBlitSurface(ctx, buffer1, w, h) {
  const buffer2 = new Uint8ClampedArray(buffer1.buffer, buffer1.byteOffset, buffer1.byteLength);
  const img = new ImageData(buffer2, w, h);
  ctx.putImageData(img, 0, 0);
}

Module['ipeBlitSurface'] = ipeBlitSurface;
  });

//! Construct a new canvas.
Canvas::Canvas(val bottomCanvas, val topCanvas)
{
  addIpeCanvasJS();
  iBottomCanvas = bottomCanvas;
  iTopCanvas = topCanvas;

  updateSize();
  iNeedPaint = false;

  val options = val::object();
  options.set("alpha", false);
  iBottomCtx = iBottomCanvas.call<val>("getContext", val("2d"), options);
  iTopCtx = iTopCanvas.call<val>("getContext", val("2d"));
  ipeDebug("Canvas has size: %g x %g (%g x %g)",
	   iWidth, iHeight, iBWidth, iBHeight);
}

void Canvas::updateSize()
{
  iDpr = val::global("window")["devicePixelRatio"].as<double>();
  iBWidth = iBottomCanvas["width"].as<double>();
  iBHeight = iBottomCanvas["height"].as<double>();
  iWidth = iBWidth / iDpr;
  iHeight = iBHeight / iDpr;
}

void Canvas::setCursor(TCursor cursor, double w, Color *color)
{
  // not implemented
}

// --------------------------------------------------------------------

static void repaint(void * arg)
{
  Canvas * canvas = (Canvas *) arg;
  canvas->paint();
}

void Canvas::invalidate()
{
  iNeedPaint = true;
  emscripten_async_call(repaint, this, 0);
}

void Canvas::invalidate(int x, int y, int w, int h)
{
  invalidate();
}

// --------------------------------------------------------------------

static int convertModifiers(val ev)
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
  iGlobalPos = Vector(ev["clientX"].as<double>(), ev["clientY"].as<double>());
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
      iObserver->canvasObserverWheelMoved(p.x / 8.0, -p.y / 8.0, kind);
  }
}

bool Canvas::keyPressEvent(emscripten::val ev)
{
  if (iTool) {
    int mod = convertModifiers(ev);
    String key{ev["key"].as<std::string>()};
    if (key == "Escape")
      key = "\x1b";
    else if (key == "Delete" || key == "Backspace")
      key = "\x08";
    else if ((mod & EControl) && key.size() == 1 && 'a' <= key[0] && key[0] <= 'z') {
      const char ctrlKey = key[0] & 0x1f;
      key = ""; key += ctrlKey;
    }
    iTool->key(key, mod | iAdditionalModifiers);
    return true;
  } else
    return false;
}

static void draw_plus(const Vector &p, JsPainter &q)
{
  q.drawLine(p - Vector(8, 0), p + Vector(8, 0));
  q.drawLine(p - Vector(0, 8), p + Vector(0, 8));
}

static void draw_rhombus(const Vector &p, JsPainter &q)
{
  q.drawPath(p - Vector(8, 0),
	     p + Vector(0, 8),
	     p + Vector(8, 0),
	     p + Vector(0, -8));
}

static void draw_square(const Vector &p, JsPainter &q)
{
  q.drawPath(p + Vector(-7, -7),
	     p + Vector(7, -7),
	     p + Vector(7, 7),
	     p + Vector(-7, 7));
}

static void draw_x(const Vector &p, JsPainter &q)
{
  q.drawLine(p - Vector(5.6, 5.6),
	     p + Vector(5.6, 5.6));
  q.drawLine(p - Vector(5.6, -5.6),
	     p + Vector(5.6, -5.6));
}

static void draw_star(const Vector &p, JsPainter &q)
{
  q.drawLine(p - Vector(8, 0), p + Vector(8, 0));
  q.drawLine(p + Vector(-4, 7), p + Vector(4, -7));
  q.drawLine(p + Vector(-4, -7), p + Vector(4, 7));
}

void Canvas::drawFifi(JsPainter &q)
{
  Vector p = userToDev(iMousePos);
  switch (iFifiMode) {
  case Snap::ESnapNone:
    // don't draw at all
    break;
  case Snap::ESnapVtx:
    q.setPen(255, 0, 0);
    draw_rhombus(p, q);
    break;
  case Snap::ESnapCtl:
    q.setPen(255, 0, 0);
    draw_square(p, q);
    break;
    break;
  case Snap::ESnapBd:
    q.setPen(255, 0, 0);
    draw_plus(p, q);
    break;
  case Snap::ESnapInt:
    q.setPen(255, 0, 0);
    draw_x(p, q);
    break;
  case Snap::ESnapGrid:
    q.setPen(0, 128, 0);
    draw_plus(p, q);
    break;
  case Snap::ESnapAngle:
  case Snap::ESnapAuto:
  default:
    q.setPen(255, 0, 0);
    draw_star(p, q);
    break;
  }
  iOldFifi = p;
}

void Canvas::paint()
{
  iNeedPaint = false;

  if (refreshSurface()) {
    // adjust from ARGB to RGBA
    uint8_t * p = cairo_image_surface_get_data(iSurface);
    const uint32_t * source = (uint32_t *) p;
    int nPixels = cairo_image_surface_get_width(iSurface) * cairo_image_surface_get_height(iSurface);
    const uint32_t * fin = source + nPixels;
    while (source != fin) {
      uint32_t bits = *source++;
      p[0] = (bits & 0x00ff0000) >> 16;
      p[1] = (bits & 0x0000ff00) >> 8;
      p[2] = (bits & 0x000000ff);
      p[3] = 0xff;
      p += 4;
    }
    // not sure if this is needed, we will repaint completely anyway
    cairo_surface_mark_dirty(iSurface);
    val buffer1 = val(typed_memory_view(nPixels * 4, cairo_image_surface_get_data(iSurface)));
    val::module_property("ipeBlitSurface")(iBottomCtx, buffer1, iBWidth, iBHeight);
  }

  iTopCtx.call<void>("clearRect", 0, 0, iBWidth, iBHeight);

  JsPainter qp(iCascade, iTopCtx, iDpr);
  if (iFifiVisible)
    drawFifi(qp);
  if (iPage) {
    qp.transform(canvasTfm());
    qp.pushMatrix();
    drawTool(qp);
    qp.popMatrix();
  }
}

// --------------------------------------------------------------------
