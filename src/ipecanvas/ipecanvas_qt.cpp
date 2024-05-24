// --------------------------------------------------------------------
// ipe::Canvas for Qt
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

#include "ipecanvas_qt.h"
#include "ipepainter.h"
#include "ipetool.h"

#include <cairo.h>

#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPointingDevice>

using namespace ipe;

// --------------------------------------------------------------------

class IpeQtPainter : public Painter {
public:
  IpeQtPainter(const Cascade *sheet, QPainter *painter);
  virtual ~IpeQtPainter();

protected:
  virtual void doNewPath();
  virtual void doMoveTo(const Vector &v);
  virtual void doLineTo(const Vector &v);
  virtual void doCurveTo(const Vector &v1, const Vector &v2,
			 const Vector &v3);
  virtual void doClosePath();

  virtual void doDrawPath(TPathMode mode);

private:
  QPainter *iQP;
  QPainterPath iPP;
};

IpeQtPainter::IpeQtPainter(const Cascade *sheet, QPainter *painter)
  : Painter(sheet)
{
  iQP = painter;
}

IpeQtPainter::~IpeQtPainter()
{
  // nothing
}

void IpeQtPainter::doNewPath()
{
  iPP = QPainterPath();
}

void IpeQtPainter::doMoveTo(const Vector &v)
{
  iPP.moveTo(QPt(v));
}

void IpeQtPainter::doLineTo(const Vector &v)
{
  iPP.lineTo(QPt(v));
}

void IpeQtPainter::doCurveTo(const Vector &v1, const Vector &v2,
	       const Vector &v3)
{
  iPP.cubicTo(QPt(v1), QPt(v2), QPt(v3));
}

void IpeQtPainter::doClosePath()
{
  iPP.closeSubpath();
}

void IpeQtPainter::doDrawPath(TPathMode mode)
{
  if (mode >= EStrokedAndFilled) {
    QBrush qbrush(QIpe(fill()));
    iQP->fillPath(iPP, qbrush);
  }
  if (mode <= EStrokedAndFilled) {
    QPen qpen(QIpe(stroke()));
    qpen.setWidthF(pen().toDouble());
    iQP->strokePath(iPP, qpen);
  }
}

// --------------------------------------------------------------------

//! Construct a new canvas.
Canvas::Canvas(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
{
  setAttribute(Qt::WA_OpaquePaintEvent);
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);
}

QSize Canvas::sizeHint() const
{
  return QSize(640, 480);
}

void Canvas::setCursor(TCursor cursor, double w, Color *color)
{
  switch (cursor) {
  case EStandardCursor:
  default:
    QWidget::unsetCursor();
    break;
  case EHandCursor:
    QWidget::setCursor(QCursor(Qt::PointingHandCursor));
    break;
  case ECrossCursor:
    QWidget::setCursor(QCursor(Qt::CrossCursor));
    break;
  case EDotCursor:
    {
      QPixmap p(32, 32);
      p.fill(QColor(255, 255, 255, 0));
      QPainter painter(&p);
      double s = 0.5 * w * zoom();
      if (s < 1.0)
	s = 1.0;
      if (s > 10.0)
	s = 10.0;
      int r = 255 * color->iRed.internal() / 1000;
      int g = 255 * color->iGreen.internal() / 1000;
      int b = 255 * color->iBlue.internal() / 1000;
      painter.setBrush(QColor(r, g, b));
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(QRectF(16.0 - s, 16.0 - s, 2 * s, 2 * s));
      painter.end();
      QWidget::setCursor(QCursor(p));
    }
    break;
  }
}

// --------------------------------------------------------------------

void Canvas::invalidate()
{
  QWidget::update();
}

void Canvas::invalidate(int x, int y, int w, int h)
{
  QWidget::update(QRect(x, y, w, h));
}

// --------------------------------------------------------------------

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

void Canvas::mouseDoubleClickEvent(QMouseEvent *ev)
{
  mouseButton(ev, 0x81, true);
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

void Canvas::tabletEvent(QTabletEvent *ev)
{
  static bool pressed = false;
  Vector globalPos(ev->globalPosition().x(), ev->globalPosition().y());
  QPointF hiPos = ev->position();

  switch (ev->type()) {
  case QEvent::TabletPress:
    if (pressed) {
      ev->ignore();
      pressed = false;
      return;
    }
    ipeDebug("TabletPress: %d %d %ld", ev->button(), ev->pointerType(), ev->timestamp());
    pressed = true;
    ev->accept();
    iGlobalPos = globalPos;
    computeFifi(hiPos.x(), hiPos.y());
    if (ev->pointerType() == QPointingDevice::PointerType::Eraser) {
      if (iObserver)
	iObserver->canvasObserverMouseAction(Qt::XButton1 |
					     iAdditionalModifiers);
    } else if (iTool)
      iTool->mouseButton(ev->button() | iAdditionalModifiers, true);
    else if (iObserver)
      iObserver->canvasObserverMouseAction(ev->button() |
					   iAdditionalModifiers);
    break;
  case QEvent::TabletMove:
    if (ev->pressure() > 0.01) {
      ev->accept();
      computeFifi(hiPos.x(), hiPos.y());
      if (iTool)
	iTool->mouseMove();
      if (iObserver)
	iObserver->canvasObserverPositionChanged();
      break;
    }
    // else fall through and consider it a release event
  case QEvent::TabletRelease:
    pressed = false;
    ev->accept();
    iGlobalPos = globalPos;
    computeFifi(hiPos.x(), hiPos.y());
    if (iTool)
      iTool->mouseButton(Qt::LeftButton, false);
    break;
  default:
    ipeDebug("Unknown tablet event");
    break;
  }
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

void Canvas::paintEvent(QPaintEvent * ev)
{
  iWidth = width();
  iHeight = height();

  double dpr = devicePixelRatio();
  iBWidth = iWidth * dpr;
  iBHeight = iHeight * dpr;

  refreshSurface();

  QPainter qPainter;
  qPainter.begin(this);
  QRect r = ev->rect();
  QRect source(r.left() * dpr, r.top() * dpr, r.width() * dpr, r.height() * dpr);
  QImage bits(cairo_image_surface_get_data(iSurface),
	      iBWidth, iBHeight, QImage::Format_RGB32);
  qPainter.drawImage(r, bits, source);
  if (iFifiVisible)
    drawFifi(qPainter);
  if (iPage) {
    IpeQtPainter qp(iCascade, &qPainter);
    qp.transform(canvasTfm());
    qp.pushMatrix();
    drawTool(qp);
    qp.popMatrix();
  }
  qPainter.end();
}

// --------------------------------------------------------------------
