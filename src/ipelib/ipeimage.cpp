// --------------------------------------------------------------------
// The image object.
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

#include "ipeimage.h"
#include "ipepainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Image
  \ingroup obj
  \brief The image object.
*/

//! Create a new image
Image::Image(const Rect &rect, Bitmap bitmap)
  : Object()
{
  iRect = rect;
  iBitmap = bitmap;
  iOpacity = Attribute::OPAQUE();
  assert(!iBitmap.isNull());
}

//! Create from XML stream.
Image::Image(const XmlAttributes &attr, String data)
  : Object(attr)
{
  init(attr);
  iBitmap = Bitmap(attr, data);
}

//! Create from XML stream with given bitmap.
Image::Image(const XmlAttributes &attr, Bitmap bitmap)
  : Object(attr), iBitmap(bitmap)
{
  init(attr);
}

void Image::init(const XmlAttributes &attr)
{
  String str;
  if (attr.has("opacity", str))
    iOpacity = Attribute(true, str);
  else
    iOpacity = Attribute::OPAQUE();

  // parse rect
  Lex st(attr["rect"]);
  Vector v;
  st >> v.x >> v.y;
  iRect.addPoint(v);
  st >> v.x >> v.y;
  iRect.addPoint(v);
}

//! Clone object
Object *Image::clone() const
{
  return new Image(*this);
}

//! Return pointer to this object.
Image *Image::asImage()
{
  return this;
}

Object::Type Image::type() const
{
  return EImage;
}

//! Call VisitImage of visitor.
void Image::accept(Visitor &visitor) const
{
  visitor.visitImage(this);
}

//! Save image in XML stream.
void Image::saveAsXml(Stream &stream, String layer) const
{
  stream << "<image";
  saveAttributesAsXml(stream, layer);
  stream << " rect=\"" << rect() << "\"";
  if (iOpacity != Attribute::OPAQUE())
    stream << " opacity=\"" << iOpacity.string() << "\"";
  stream << " bitmap=\"" << iBitmap.objNum() << "\"";
  stream << "/>\n";
}

//! Draw image.
void Image::draw(Painter &painter) const
{
  Matrix m(iRect.width(), 0, 0, iRect.height(),
	   iRect.bottomLeft().x, iRect.bottomLeft().y);
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  painter.transform(m);
  painter.push();
  painter.setOpacity(iOpacity);
  painter.drawBitmap(iBitmap);
  painter.pop();
  painter.popMatrix();
}

void Image::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  painter.newPath();
  painter.rect(iRect);
  painter.drawPath(EStrokedOnly);
  painter.popMatrix();
}

double Image::distance(const Vector &v, const Matrix &m, double bound) const
{
  Matrix m1 = effectiveMatrix(m);
  Vector u[5];
  u[0] = m1 * iRect.bottomLeft();
  u[1] = m1 * iRect.bottomRight();
  u[2] = m1 * iRect.topRight();
  u[3] = m1 * iRect.topLeft();
  u[4] = u[0];
  Rect box;
  for (int i = 0; i < 4; ++i)
    box.addPoint(u[i]);
  if (box.certainClearance(v, bound))
    return bound;
  double d = bound;
  double d1;
  for (int i = 0; i < 4; ++i) {
    if ((d1 = Segment(u[i], u[i+1]).distance(v, d)) < d)
      d = d1;
  }
  return d;
}

void Image::addToBBox(Rect &box, const Matrix &m, bool) const
{
  Matrix m1 = effectiveMatrix(m);
  box.addPoint(m1 * iRect.bottomLeft());
  box.addPoint(m1 * iRect.bottomRight());
  box.addPoint(m1 * iRect.topRight());
  box.addPoint(m1 * iRect.topLeft());
}

void Image::snapCtl(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  Matrix m1 = effectiveMatrix(m);
  (m1 * iRect.bottomLeft()).snap(mouse, pos, bound);
  (m1 * iRect.bottomRight()).snap(mouse, pos, bound);
  (m1 * iRect.topRight()).snap(mouse, pos, bound);
  (m1 * iRect.topLeft()).snap(mouse, pos, bound);
}

//! Set opacity of the object.
void Image::setOpacity(Attribute opaq)
{
  iOpacity = opaq;
}

bool Image::setAttribute(Property prop, Attribute value)
{
  switch (prop) {
  case EPropOpacity:
    if (value != opacity()) {
      setOpacity(value);
      return true;
    }
    break;
  default:
    return Object::setAttribute(prop, value);
  }
  return false;
}

Attribute Image::getAttribute(Property prop) const noexcept
{
  switch (prop) {
  case EPropOpacity:
    return opacity();
  default:
    return Object::getAttribute(prop);
  }
}

// --------------------------------------------------------------------
