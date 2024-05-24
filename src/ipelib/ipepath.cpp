// --------------------------------------------------------------------
// The path object
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

#include "ipepath.h"
#include "ipepainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Path
  \ingroup obj
  \brief The path object (polylines, polygons, and generalizations).

  This object represents any vector graphics.  The geometry is
  contained in a Shape.

  The filling algorithm is the <b>even-odd rule</b> of PDF: To
  determine whether a point lies inside the filled shape, draw a ray
  from that point in any direction, and count the number of path
  segments that cross the ray.  If this number is odd, the point is
  inside; if even, the point is outside. (Path objects can also render
  using the <b>winding fill rule</b> by setting the fillRule
  attribute.  This isn't really supported by the Ipe user interface,
  which doesn't show the orientation of paths.)

  If the path consists of a single line segment and is filled only,
  then it is not drawn at all.  This can be used to draw arrow heads
  without bodies.  The fill color is used to draw the arrows in this
  case.
 */

//! Construct from XML data.
Path *Path::create(const XmlAttributes &attr, String data)
{
  std::unique_ptr<Path> self(new Path(attr));
  if (!self->iShape.load(data))
    return nullptr;
  self->makeArrowData();
  return self.release();
}

//! Create empty path with attributes taken from XML
Path::Path(const XmlAttributes &attr)
  : Object(attr)
{
  bool stroked = false;
  bool filled = false;
  iStroke = Attribute::BLACK();
  iFill = Attribute::WHITE();

  String str;
  if (attr.has("stroke", str)) {
    iStroke = Attribute::makeColor(str, Attribute::BLACK());
    stroked = true;
  }
  if (attr.has("fill", str)) {
    iFill = Attribute::makeColor(str, Attribute::WHITE());
    filled = true;
  }

  if (!stroked && !filled) {
    stroked = true;
    iStroke= Attribute::BLACK();
  }

  iDashStyle = Attribute::makeDashStyle(attr["dash"]);

  iPen = Attribute::makeScalar(attr["pen"], Attribute::NORMAL());

  if (attr.has("opacity", str))
    iOpacity = Attribute(true, str);
  else
    iOpacity = Attribute::OPAQUE();

  if (attr.has("stroke-opacity", str))
    iStrokeOpacity = Attribute(true, str);
  else
    iStrokeOpacity = iOpacity;

  iGradient = Attribute::NORMAL();
  iTiling = Attribute::NORMAL();

  if (attr.has("gradient", str))
    iGradient = Attribute(true, str);
  else if (attr.has("tiling", str))
    iTiling = Attribute(true, str);

  iPathMode = (stroked ? (filled ? EStrokedAndFilled : EStrokedOnly) :
	       EFilledOnly);

  iLineCap = EDefaultCap;
  iLineJoin = EDefaultJoin;
  iFillRule = EDefaultRule;
  if (attr.has("cap", str))
    iLineCap = TLineCap(Lex(str).getInt() + 1);
  if (attr.has("join", str))
    iLineJoin = TLineJoin(Lex(str).getInt() + 1);
  if (attr.has("fillrule", str)) {
    if (str == "eofill")
      iFillRule = EEvenOddRule;
    else if (str == "wind")
      iFillRule = EWindRule;
  }

  iHasFArrow = false;
  iHasRArrow = false;
  iFArrowShape = iRArrowShape = Attribute::ARROW_NORMAL();
  iFArrowSize = iRArrowSize = Attribute::NORMAL();
  iFArrowIsM = false;
  iRArrowIsM = false;

  if (attr.has("arrow", str)) {
    iHasFArrow = true;
    int i = str.find("/");
    if (i >= 0) {
      iFArrowShape = Attribute(true, String("arrow/") + str.left(i) + "(spx)");
      iFArrowSize = Attribute::makeScalar(str.substr(i+1), Attribute::NORMAL());
      iFArrowIsM = iFArrowShape.isMidArrow();
    } else
      iFArrowSize = Attribute::makeScalar(str, Attribute::NORMAL());
  }

  if (attr.has("rarrow", str)) {
    iHasRArrow = true;
    int i = str.find("/");
    if (i >= 0) {
      iRArrowShape = Attribute(true, String("arrow/") + str.left(i) + "(spx)");
      iRArrowSize = Attribute::makeScalar(str.substr(i+1), Attribute::NORMAL());
      iRArrowIsM = iRArrowShape.isMidArrow();
    } else
      iRArrowSize = Attribute::makeScalar(str, Attribute::NORMAL());
  }
}

void Path::init(const AllAttributes &attr, bool withArrows)
{
  iPathMode = attr.iPathMode;
  iStroke = attr.iStroke;
  iFill = attr.iFill;
  iDashStyle = attr.iDashStyle;
  iPen = attr.iPen;
  iOpacity = attr.iOpacity;
  iStrokeOpacity = attr.iStrokeOpacity;
  iTiling = attr.iTiling;
  if (iTiling.isNormal())
    iGradient = attr.iGradient;
  else
    iGradient = Attribute::NORMAL();
  iLineCap = attr.iLineCap;
  iLineJoin = attr.iLineJoin;
  iFillRule = attr.iFillRule;
  iHasFArrow = false;
  iHasRArrow = false;
  iFArrowShape = iRArrowShape = Attribute::ARROW_NORMAL();
  iFArrowSize = iRArrowSize = Attribute::NORMAL();
  iFArrowIsM = iRArrowIsM = false;
  if (withArrows) {
    iFArrowShape = attr.iFArrowShape;
    iRArrowShape = attr.iRArrowShape;
    iFArrowSize = attr.iFArrowSize;
    iRArrowSize = attr.iRArrowSize;
    iHasFArrow = attr.iFArrow;
    iHasRArrow = attr.iRArrow;
    iFArrowIsM = iFArrowShape.isMidArrow();
    iRArrowIsM = iRArrowShape.isMidArrow();
  }
}

//! Create for given shape.
Path::Path(const AllAttributes &attr, const Shape &shape, bool withArrows)
  : Object(attr), iShape(shape)
{
  init(attr, withArrows);
  makeArrowData();
}

//! Return a clone (constant-time).
Object *Path::clone() const
{
  return new Path(*this);
}

//! Compute the arrow information.
void Path::makeArrowData()
{
  if (iShape.countSubPaths() == 0 || iShape.countSubPaths() > 1 || iShape.subPath(0)->closed()) {
    iFArrowOk = false;
    iRArrowOk = false;
    iMArrowOk = false;
  } else {
    const Curve *curve = iShape.subPath(0)->asCurve();

    CurveSegment seg = curve->segment(0);
    iRArrowOk = true;
    iRArrowPos = seg.cp(0);
    iRArrowArc = 0;
    if (seg.type() == CurveSegment::EArc) {
      iRArrowArc = 1;
      Angle alpha = (seg.matrix().inverse() * seg.cp(0)).angle();
      Linear m = seg.matrix().linear();
      iRArrowDir = (m * Vector(Angle(alpha - IpeHalfPi))).angle();
    } else {
      if (seg.cp(1) == seg.cp(0))
	iRArrowOk = false;
      else
	iRArrowDir = (iRArrowPos - seg.cp(1)).angle();
    }

    seg = curve->segment(-1);
    iFArrowOk = true;
    iFArrowPos = seg.last();
    iFArrowArc = 0;
    if (seg.type() == CurveSegment::EArc) {
      iFArrowArc = 1;
      Angle alpha = (seg.matrix().inverse() * seg.cp(1)).angle();
      Linear m = seg.matrix().linear();
      iFArrowDir = (m * Vector(Angle(alpha + IpeHalfPi))).angle();
    } else {
      Vector p = seg.cp(seg.countCP() - 2);
      if (p == seg.last())
	iFArrowOk = false;
      else
	iFArrowDir = (iFArrowPos - p).angle();
    }

    iMArrowOk = false;
    Matrix m = matrix();
    double len = 0.0;
    for (int i = 0; i < curve->countSegments(); ++i) {
      seg = curve->segment(i);
      if (seg.type() != CurveSegment::ESegment)
	return;
      len += (m * seg.cp(0) - m * seg.cp(1)).len();
    }
    double mid = len / 2.0;
    len = 0.0;
    for (int i = 0; i < curve->countSegments(); ++i) {
      seg = curve->segment(i);
      double d = (m * seg.cp(0) - m * seg.cp(1)).len();
      if (len < mid && mid <= len + d) {
	// found the position
	iMArrowOk = true;
	double lambda = (mid - len) / d;
	iMArrowPos = seg.cp(0) + lambda * (seg.cp(1) - seg.cp(0));
	iMArrowDir = (seg.cp(1) - seg.cp(0)).angle();
	return;
      }
      len += d;
    }
  }
}

//! Return pointer to this object.
Path *Path::asPath()
{
  return this;
}

Object::Type Path::type() const
{
  return EPath;
}

//! Call visitPath of visitor.
void Path::accept(Visitor &visitor) const
{
  visitor.visitPath(this);
}

void Path::saveAsXml(Stream &stream, String layer) const
{
  bool stroked = (iPathMode <= EStrokedAndFilled);
  bool filled = (iPathMode >= EStrokedAndFilled);
  stream << "<path";
  saveAttributesAsXml(stream, layer);
  if (stroked)
    stream << " stroke=\"" << iStroke.string() << "\"";
  if (filled)
    stream << " fill=\"" << iFill.string() << "\"";
  if (stroked && !iDashStyle.isNormal())
    stream << " dash=\"" << iDashStyle.string() << "\"";
  if ((stroked || (iHasFArrow && iFArrowOk) || (iHasRArrow && iRArrowOk))
      && !iPen.isNormal())
    stream << " pen=\"" << iPen.string() << "\"";
  if (stroked && iLineCap != EDefaultCap)
    stream << " cap=\"" << iLineCap - 1 << "\"";
  if (stroked && iLineJoin != EDefaultJoin)
    stream << " join=\"" << iLineJoin - 1 << "\"";
  if (filled && iFillRule == EWindRule)
    stream << " fillrule=\"wind\"";
  else if (filled && iFillRule == EEvenOddRule)
    stream << " fillrule=\"eofill\"";
  if (iHasFArrow && iFArrowOk) {
    String s = iFArrowShape.string();
    stream << " arrow=\"" << s.substr(6, s.size() - 11)
	   << "/" << iFArrowSize.string() << "\"";
  }
  if (iHasRArrow && iRArrowOk) {
    String s = iRArrowShape.string();
    stream << " rarrow=\"" << s.substr(6, s.size() - 11)
	   << "/" << iRArrowSize.string() << "\"";
  }
  if (iOpacity != Attribute::OPAQUE())
    stream << " opacity=\"" << iOpacity.string() << "\"";
  if (iStrokeOpacity != iOpacity)
    stream << " stroke-opacity=\"" << iStrokeOpacity.string() << "\"";
  if (filled && !iTiling.isNormal())
    stream << " tiling=\"" << iTiling.string() << "\"";
  if (filled && !iGradient.isNormal())
    stream << " gradient=\"" << iGradient.string() << "\"";
  stream << ">\n";
  iShape.save(stream);
  stream << "</path>\n";
}

/*! Draw an arrow of \a size with tip at \a pos directed
  in direction \a angle. */
void Path::drawArrow(Painter &painter, Vector pos, Angle angle,
		     Attribute shape, Attribute size, double radius)
{
  const Symbol *symbol = painter.cascade()->findSymbol(shape);
  if (symbol) {
    double s = painter.cascade()->find(EArrowSize, size).number().toDouble();
    Color color = painter.stroke();
    // Fixed opaq = painter.opacity();

    painter.push();
    painter.pushMatrix();
    painter.translate(pos);
    painter.transform(Linear(angle));
    painter.untransform(ETransformationsRigidMotions);

    bool cw = (radius < 0);
    if (cw)
      radius = -radius;
    bool pointy = (shape == Attribute::ARROW_PTARC() ||
		   shape == Attribute::ARROW_FPTARC());

    if (shape.isArcArrow() && (radius > s)) {
      Angle delta = s / radius;
      Angle alpha = atan(1.0/3.0);
      Arc arc1;
      Arc arc2;
      Arc arc3;
      if (cw) {
	arc1 = Arc(Matrix(radius, 0, 0, radius, 0, -radius),
		   IpeHalfPi, IpeHalfPi + delta);
	arc2 = Arc(Matrix(radius, 0, 0, -radius, 0, -radius),
		   -IpeHalfPi - delta, -IpeHalfPi);
	arc3 = Arc(Matrix(radius, 0, 0, radius, 0, -radius),
		   IpeHalfPi, IpeHalfPi + 0.8 * delta);
      } else {
	arc1 = Arc(Matrix(radius, 0, 0, radius, 0, radius),
		   -IpeHalfPi - delta, -IpeHalfPi);
	arc2 = Arc(Matrix(radius, 0, 0, -radius, 0, radius),
		   IpeHalfPi, IpeHalfPi + delta);
	arc3 = Arc(Matrix(radius, 0, 0, radius, 0, radius),
		   -IpeHalfPi - 0.8 * delta, -IpeHalfPi);
      }
      arc1 = Linear(alpha) * arc1;
      arc2 = Linear(-alpha) * arc2;
      painter.setStroke(Attribute(color));
      if (shape == Attribute::ARROW_FARC() ||
	  shape == Attribute::ARROW_FPTARC())
	painter.setFill(Attribute(Color(1000, 1000, 1000)));
      else
	painter.setFill(Attribute(color));
      // painter.setOpacity(Attribute(opaq));
      painter.newPath();
      painter.moveTo(arc1.beginp());
      painter.drawArc(arc1);
      if (cw) {
	if (pointy)
	  painter.lineTo(arc3.endp());
	painter.lineTo(arc2.beginp());
	painter.drawArc(arc2);
      } else {
	painter.drawArc(arc2);
	if (pointy)
	  painter.lineTo(arc3.beginp());
      }
      painter.closePath();
      painter.drawPath(EStrokedAndFilled);
    } else {
      Matrix m(s, 0, 0, s, 0, 0);
      painter.transform(m);
      painter.setSymStroke(Attribute(color));
      painter.setSymFill(Attribute(color));
      painter.setSymPen(Attribute(painter.pen()));
      symbol->iObject->draw(painter);
    }
    painter.popMatrix();
    painter.pop();
  }
}

void Path::setShape(const Shape &shape)
{
  iShape = shape;
  makeArrowData();
}

void Path::draw(Painter &painter) const
{
  painter.push();
  if (iPathMode <= EStrokedAndFilled) {
    painter.setStroke(iStroke);
    painter.setDashStyle(iDashStyle);
    painter.setPen(iPen);
    painter.setLineCap(lineCap());
    painter.setLineJoin(lineJoin());
  }
  if (iPathMode >= EStrokedAndFilled) {
    painter.setFill(iFill);
    painter.setFillRule(fillRule());
    painter.setTiling(iTiling);
    painter.setGradient(iGradient);
  }
  painter.setOpacity(iOpacity);
  painter.setStrokeOpacity(iStrokeOpacity);
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  if (!iShape.isSegment() || iPathMode != EFilledOnly) {
    painter.newPath();
    iShape.draw(painter);
    painter.drawPath(iPathMode);
  }
  if (iPathMode == EStrokedAndFilled && !iGradient.isNormal()) {
    // need to stroke separately
    painter.newPath();
    iShape.draw(painter);
    painter.drawPath(EStrokedOnly);
  }
  if ((iHasFArrow && iFArrowOk) || (iHasRArrow && iRArrowOk)) {
    // Draw arrows
    if (iPathMode == EFilledOnly) {
      painter.setStroke(iFill);
      painter.setPen(iPen);
      painter.setLineCap(lineCap());
      painter.setLineJoin(lineJoin());
    }
    if (iHasFArrow && iFArrowOk) {
      double r = 0.0;
      if (iFArrowArc && iFArrowShape.isArcArrow()) {
	CurveSegment seg = iShape.subPath(0)->asCurve()->segment(-1);
	Vector center = painter.matrix() * seg.matrix().translation();
	r = (center - painter.matrix() * iFArrowPos).len();
	if ((painter.matrix().linear() * seg.matrix().linear()).determinant()<0)
	  r = -r;
      }
      if (iFArrowIsM && iMArrowOk)
	drawArrow(painter, iMArrowPos, iMArrowDir, iFArrowShape, iFArrowSize, r);
      else
	drawArrow(painter, iFArrowPos, iFArrowDir, iFArrowShape, iFArrowSize, r);
    }
    if (iHasRArrow && iRArrowOk) {
      double r = 0.0;
      if (iRArrowArc && iRArrowShape.isArcArrow()) {
	CurveSegment seg = iShape.subPath(0)->asCurve()->segment(0);
	Vector center = painter.matrix() * seg.matrix().translation();
	r = (center - painter.matrix() * iRArrowPos).len();
	if ((painter.matrix().linear() * seg.matrix().linear()).determinant()>0)
	  r = -r;
      }
      if (iRArrowIsM && iMArrowOk)
	drawArrow(painter, iMArrowPos, iMArrowDir + IPE_PI, iRArrowShape, iRArrowSize, r);
      else
	drawArrow(painter, iRArrowPos, iRArrowDir, iRArrowShape, iRArrowSize, r);
    }
  }
  painter.popMatrix();
  painter.pop();
}

void Path::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  painter.newPath();
  iShape.draw(painter);
  painter.drawPath(EStrokedOnly);
  painter.popMatrix();
}

void Path::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  iShape.addToBBox(box, m * matrix(), cp);
}

double Path::distance(const Vector &v, const Matrix &m, double bound) const
{
  return iShape.distance(v, m * matrix(), bound);
}

void Path::snapVtx(const Vector &mouse, const Matrix &m,
		   Vector &pos, double &bound) const
{
  iShape.snapVtx(mouse, m * matrix(), pos, bound, false);
}

void Path::snapCtl(const Vector &mouse, const Matrix &m,
		   Vector &pos, double &bound) const
{
  iShape.snapVtx(mouse, m * matrix(), pos, bound, true);
}

void Path::snapBnd(const Vector &mouse, const Matrix &m,
		   Vector &pos, double &bound) const
{
  iShape.snapBnd(mouse, m * matrix(), pos, bound);
}

void Path::setMatrix(const Matrix &matrix)
{
  Object::setMatrix(matrix);
  makeArrowData();
}

//! Set whether object will be stroked and filled.
void Path::setPathMode(TPathMode pm)
{
  iPathMode = pm;
}

//! Set stroke color.
void Path::setStroke(Attribute stroke)
{
  iStroke = stroke;
}

//! Set fill color.
void Path::setFill(Attribute fill)
{
  iFill = fill;
}

//! Set tiling pattern of the object.
/*! Resets gradient fill. */
void Path::setTiling(Attribute til)
{
  iTiling = til;
  iGradient = Attribute::NORMAL();
}

//! Set gradient fill of the object.
/*! Resets tiling pattern. */
void Path::setGradient(Attribute grad)
{
  iGradient = grad;
  iTiling = Attribute::NORMAL();
}

//! Set opacity of the object.
void Path::setOpacity(Attribute opaq)
{
  iOpacity = opaq;
}

//! Set stroke opacity of the object.
void Path::setStrokeOpacity(Attribute opaq)
{
  iStrokeOpacity = opaq;
}

//! Set pen.
void Path::setPen(Attribute pen)
{
  iPen = pen;
}

//! Set dash style.
void Path::setDashStyle(Attribute dash)
{
  iDashStyle = dash;
}

//! Set forward arrow.
void Path::setArrow(bool arrow, Attribute shape, Attribute size)
{
  iHasFArrow = arrow;
  iFArrowShape = shape;
  iFArrowSize = size;
  iFArrowIsM = iFArrowShape.isMidArrow();
}

//! Set backward arrow (if the object can take it).
void Path::setRarrow(bool arrow, Attribute shape, Attribute size)
{
  iHasRArrow = arrow;
  iRArrowShape = shape;
  iRArrowSize = size;
  iRArrowIsM = iRArrowShape.isMidArrow();
}

//! Set line cap style.
void Path::setLineCap(TLineCap s)
{
  iLineCap = s;
}

//! Set line join style.
void Path::setLineJoin(TLineJoin s)
{
  iLineJoin = s;
}

//! Set fill rule.
void Path::setFillRule(TFillRule s)
{
  iFillRule = s;
}

void Path::checkStyle(const Cascade *sheet, AttributeSeq &seq) const
{
  checkSymbol(EColor, iStroke, sheet, seq);
  checkSymbol(EColor, iFill, sheet, seq);
  checkSymbol(EDashStyle, iDashStyle, sheet, seq);
  checkSymbol(EPen, iPen, sheet, seq);
  checkSymbol(EArrowSize, iFArrowSize, sheet, seq);
  checkSymbol(EArrowSize, iRArrowSize, sheet, seq);
  checkSymbol(ESymbol, iFArrowShape, sheet, seq);
  checkSymbol(ESymbol, iRArrowShape, sheet, seq);
  checkSymbol(EOpacity, iOpacity, sheet, seq);
  checkSymbol(EOpacity, iStrokeOpacity, sheet, seq);
  if (!iTiling.isNormal())
    checkSymbol(ETiling, iTiling, sheet, seq);
  if (!iGradient.isNormal())
    checkSymbol(EGradient, iGradient, sheet, seq);
}

bool Path::setAttribute(Property prop, Attribute value)
{
  switch (prop) {
  case EPropPathMode:
    if (value.pathMode() != pathMode()) {
      setPathMode(value.pathMode());
      return true;
    }
    break;
  case EPropStrokeColor:
    if (value != stroke()) {
      setStroke(value);
      return true;
    }
    break;
  case EPropFillColor:
    if (value != fill()) {
      setFill(value);
      return true;
    }
    break;
  case EPropPen:
    if (value != pen()) {
      setPen(value);
      return true;
    }
    break;
  case EPropDashStyle:
    if (value != dashStyle()) {
      setDashStyle(value);
      return true;
    }
    break;
  case EPropTiling:
    if (value != tiling()) {
      setTiling(value);
      return true;
    }
    break;
  case EPropGradient:
    if (value != gradient()) {
      setGradient(value);
      return true;
    }
    break;
  case EPropOpacity:
    if (value != opacity()) {
      setOpacity(value);
      return true;
    }
    break;
  case EPropStrokeOpacity:
    if (value != strokeOpacity()) {
      setStrokeOpacity(value);
      return true;
    }
    break;
  case EPropFArrow:
    if (value.boolean() != iHasFArrow) {
      iHasFArrow = value.boolean();
      return true;
    }
    break;
  case EPropRArrow:
    if (value.boolean() != iHasRArrow) {
      iHasRArrow = value.boolean();
      return true;
    }
    break;
  case EPropFArrowSize:
    if (value != iFArrowSize) {
      iFArrowSize= value;
      return true;
    }
    break;
  case EPropRArrowSize:
    if (value != iRArrowSize) {
      iRArrowSize = value;
      return true;
    }
    break;
  case EPropFArrowShape:
    if (value != iFArrowShape) {
      iFArrowShape = value;
      iFArrowIsM = iFArrowShape.isMidArrow();
      return true;
    }
    break;
  case EPropRArrowShape:
    if (value != iRArrowShape) {
      iRArrowShape = value;
      iRArrowIsM = iRArrowShape.isMidArrow();
      return true;
    }
    break;
  case EPropLineJoin:
    assert(value.isEnum());
    if (value.lineJoin() != iLineJoin) {
      iLineJoin = value.lineJoin();
      return true;
    }
    break;
  case EPropLineCap:
    assert(value.isEnum());
    if (value.lineCap() != iLineCap) {
      iLineCap = value.lineCap();
      return true;
    }
    break;
  case EPropFillRule:
    assert(value.isEnum());
    if (value.fillRule() != iFillRule) {
      iFillRule = value.fillRule();
      return true;
    }
    break;
  default:
    return Object::setAttribute(prop, value);
  }
  return false;
}

Attribute Path::getAttribute(Property prop) const noexcept
{
  switch (prop) {
  case EPropPathMode:
    return Attribute(iPathMode);
  case EPropStrokeColor:
    return stroke();
  case EPropFillColor:
    return fill();
  case EPropPen:
    return pen();
  case EPropDashStyle:
    return dashStyle();
  case EPropOpacity:
    return opacity();
  case EPropStrokeOpacity:
    return strokeOpacity();
  case EPropTiling:
    return tiling();
  case EPropGradient:
    return gradient();
  case EPropFArrow:
    return Attribute::Boolean(iHasFArrow);
  case EPropRArrow:
    return Attribute::Boolean(iHasRArrow);
  case EPropFArrowSize:
    return iFArrowSize;
  case EPropRArrowSize:
    return iRArrowSize;
  case EPropFArrowShape:
    return iFArrowShape;
  case EPropRArrowShape:
    return iRArrowShape;
  case EPropLineJoin:
    return Attribute(iLineJoin);
  case EPropLineCap:
    return Attribute(iLineCap);
  case EPropFillRule:
    return Attribute(iFillRule);
  default:
    return Object::getAttribute(prop);
  }
}

// --------------------------------------------------------------------
