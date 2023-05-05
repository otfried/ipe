// --------------------------------------------------------------------
// ipe::PdfView for Qt
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

#include "ipepdfview_qt.h"

#include <cairo.h>

#include <QPainter>
#include <QPaintEvent>

using namespace ipe;

// --------------------------------------------------------------------

PdfView::PdfView(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
{
  setAttribute(Qt::WA_OpaquePaintEvent);
}

QSize PdfView::sizeHint() const
{
  return QSize(640, 480);
}

// --------------------------------------------------------------------

void PdfView::invalidate()
{
  QWidget::update();
}

void PdfView::invalidate(int x, int y, int w, int h)
{
  QWidget::update(QRect(x, y, w, h));
}

// --------------------------------------------------------------------

void PdfView::paintEvent(QPaintEvent * ev)
{
  if (iBWidth != width() || iBHeight != height()) {
    iBWidth = iWidth = width();
    iBHeight = iHeight = height();
    emit sizeChanged();
  }

  refreshSurface();

  QPainter qPainter;
  qPainter.begin(this);
  QRect r = ev->rect();
  QRect source(r.left(), r.top(), r.width(), r.height());
  QImage bits(cairo_image_surface_get_data(iSurface),
	      iWidth, iHeight, QImage::Format_RGB32);
  qPainter.drawImage(r, bits, source);
  qPainter.end();
}

void PdfView::mousePressEvent(QMouseEvent *ev)
{
  Vector v = devToUser(Vector(ev->position().x(), ev->position().y()));
  emit mouseButton(ev->button() != Qt::LeftButton ? 1 : 0, v);
}

// --------------------------------------------------------------------
