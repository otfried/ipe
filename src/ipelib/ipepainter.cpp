// -*- C++ -*-
// --------------------------------------------------------------------
// Ipe drawing interface
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

#include "ipepainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Painter
 * \ingroup base
 * \brief Interface for drawing.

 Painter-derived classes are used for drawing to the screen and for
 generating PDF and Postscript output.

 The Painter maintains a stack of graphics states, which includes
 stroke and fill color, line width, dash style, miter limit, line cap
 and line join.  It also maintains a separate stack of transformation
 matrices.  The Painter class takes care of maintaining the stacks,
 and setting of the attributes in the current graphics state.

 Setting an attribute with a symbolic value is resolved immediately
 using the stylesheet Cascade attached to the Painter, so calling the
 stroke() or fill() methods of Painter will return the current
 absolute color.

 It's okay to set symbolic attributes that the stylesheet does not
 define - they are set to a default absolute value (black, solid,
 etc.).

 The painter is either in "general" or in "path construction" mode.
 The newPath() member starts path construction mode. In this mode,
 only the path construction operators (moveTo, lineTo, curveTo, rect,
 drawArc, closePath), the transformation operators (transform,
 untransform, translate), and the matrix stack operators (pushMatrix,
 popMatrix) are admissible.  The path is drawn using drawPath, this
 ends path construction mode.  Path construction operators cannot be
 used in general mode.

 The graphics state for a path must be set before starting path
 construction mode, that is, before calling newPath().

 Derived classes need to implement the doXXX functions for drawing
 paths, images, and texts.  The transformation matrix has already been
 applied to the coordinates passed to the doXXX functions.
*/

//! Constructor takes a (cascaded) style sheet, which is not owned.
/*! The initial graphics state contains all default attributes. */
Painter::Painter(const Cascade *style)
{
  iCascade = style;
  State state;
  state.iStroke = Color(0,0,0);
  state.iFill = Color(1000, 1000, 1000);
  state.iPen = iCascade->find(EPen, Attribute::NORMAL()).number();
  state.iDashStyle = "[]0"; // solid
  state.iLineCap = style->lineCap();
  state.iLineJoin = style->lineJoin();
  state.iFillRule = style->fillRule();
  state.iSymStroke = Color(0, 0, 0);
  state.iSymFill = Color(1000, 1000, 1000);
  state.iSymPen = Fixed(1);
  state.iOpacity = Fixed(1);
  state.iStrokeOpacity = Fixed(1);
  state.iTiling = Attribute::NORMAL();
  state.iGradient = Attribute::NORMAL();
  iState.push_back(state);
  iMatrix.push_back(Matrix()); // identity
  iAttributeMap = nullptr;
  iInPath = 0;
}

//! Virtual destructor.
Painter::~Painter()
{
  // nothing
}

//! Set a new attribute map.
void Painter::setAttributeMap(const AttributeMap *map)
{
  iAttributeMap = map;
}

//! Lookup a symbolic attribute
/*! Uses first the attribute map and then the stylesheet. */
Attribute Painter::lookup(Kind kind, Attribute sym) const
{
  if (iAttributeMap && sym.isSymbolic())
    return iCascade->find(kind, iAttributeMap->map(kind, sym));
  else
    return iCascade->find(kind,sym);
}

//! Concatenate a matrix to current transformation matrix.
void Painter::transform(const Matrix &m)
{
  iMatrix.back() = matrix() * m;
}

//! Reset transformation to original one, but with different origin/direction.
/*! This changes the current transformation matrix to the one set
  before the first push operation, but maintaining the current origin.
  Only the operations allowed in \a allowed are applied.
*/
void Painter::untransform(TTransformations trans)
{
  if (trans == ETransformationsAffine)
    return;
  Matrix m = matrix();
  Vector org = m.translation();
  Vector dx = Vector(m.a[0], m.a[1]);
  Linear m1(iMatrix.front().linear());
  if (trans == ETransformationsRigidMotions) {
    // compute what direction is transformed to dx by original matrix
    Angle alpha = (m1.inverse() * dx).angle();
    // ensure that (1,0) is rotated into this orientation
    m1 = m1 * Linear(alpha);
  }
  iMatrix.back() = Matrix(m1, org);
}

//! Concatenate a translation to current transformation matrix.
void Painter::translate(const Vector &v)
{
  Matrix m;
  m.a[4] = v.x;
  m.a[5] = v.y;
  iMatrix.back() = matrix() * m;
}

//! Enter path construction mode.
void Painter::newPath()
{
  assert(!iInPath);
  iInPath = iState.size(); // save current nesting level
  doNewPath();
}

//! Start a new subpath.
void Painter::moveTo(const Vector &v)
{
  assert(iInPath > 0);
  doMoveTo(matrix() * v);
}

//! Add line segment to current subpath.
void Painter::lineTo(const Vector &v)
{
  assert(iInPath > 0);
  doLineTo(matrix() * v);
}

//! Add a Bezier segment to current subpath.
void Painter::curveTo(const Vector &v1, const Vector &v2, const Vector &v3)
{
  assert(iInPath > 0);
  doCurveTo(matrix() * v1, matrix() * v2, matrix() * v3);
}

//! Add an elliptic arc to current path.
/*! Assumes the current point is \a arc.beginp(). */
void Painter::drawArc(const Arc &arc)
{
  assert(iInPath > 0);
  doDrawArc(arc);
}

//! Add a rectangle subpath to the path.
/*! This is implemented in terms of moveTo() and lineTo(). */
void Painter::rect(const Rect &re)
{
  moveTo(re.bottomLeft());
  lineTo(re.bottomRight());
  lineTo(re.topRight());
  lineTo(re.topLeft());
  closePath();
}

//! Close the current subpath.
void Painter::closePath()
{
  assert(iInPath > 0);
  doClosePath();
}

//! Save current graphics state.
/*! Cannot be called in path construction mode. */
void Painter::push()
{
  assert(!iInPath);
  State state = iState.back();
  iState.push_back(state);
  doPush();
}

//! Restore previous graphics state.
/*! Cannot be called in path construction mode. */
void Painter::pop()
{
  assert(!iInPath);
  iState.pop_back();
  doPop();
}

//! Save current transformation matrix.
void Painter::pushMatrix()
{
  iMatrix.push_back(matrix());
}

//! Restore previous transformation matrix.
void Painter::popMatrix()
{
  iMatrix.pop_back();
}

//! Fill and/or stroke a path.
/*! As in PDF, a "path" can consist of several subpaths.  Whether it
  is filled or stroked depends on \a mode.  */
void Painter::drawPath(TPathMode mode)
{
  assert(iInPath > 0);
  doDrawPath(mode);
  iInPath = 0;
}

//! Render a bitmap.
/*! Assumes the transformation matrix has been set up to map the unit
  square to the image area on the paper.
*/
void Painter::drawBitmap(Bitmap bitmap)
{
  assert(!iInPath);
  doDrawBitmap(bitmap);
}

//! Render a text object.
/*! Stroke color is already set, and the origin is the lower-left
  corner of the text box (not the reference point!). */
void Painter::drawText(const Text *text)
{
  assert(!iInPath);
  doDrawText(text);
}

//! Render a symbol.
/*! The current coordinate system is already the symbol coordinate
    system. If the symbol is parameterized, then sym-stroke, sym-fill,
    and sym-pen are already set. */
void Painter::drawSymbol(Attribute symbol)
{
  assert(!iInPath);
  doDrawSymbol(symbol);
}

//! Add current path as clip path.
void Painter::addClipPath()
{
  assert(iInPath > 0);
  doAddClipPath();
  iInPath = 0;
}

// --------------------------------------------------------------------

//! Set stroke color, resolving symbolic color and "sym-x" colors
void Painter::setStroke(Attribute color)
{
  assert(!iInPath);
  if (color == Attribute::SYM_STROKE())
    iState.back().iStroke = iState.back().iSymStroke;
  else if (color == Attribute::SYM_FILL())
    iState.back().iStroke = iState.back().iSymFill;
  else
    iState.back().iStroke = lookup(EColor, color).color();
}

//! Set fill color, resolving symbolic color.
void Painter::setFill(Attribute color)
{
  assert(!iInPath);
  if (color == Attribute::SYM_STROKE())
    iState.back().iFill = iState.back().iSymStroke;
  else if (color == Attribute::SYM_FILL())
    iState.back().iFill = iState.back().iSymFill;
  else
    iState.back().iFill = lookup(EColor, color).color();
}

//! Set pen, resolving symbolic value.
void Painter::setPen(Attribute pen)
{
  assert(!iInPath);
  if (pen == Attribute::SYM_PEN())
    iState.back().iPen = iState.back().iSymPen;
  else
    iState.back().iPen = lookup(EPen, pen).number();
}

//! Set dash style, resolving symbolic value.
void Painter::setDashStyle(Attribute dash)
{
  assert(!iInPath);
  iState.back().iDashStyle = lookup(EDashStyle, dash).string();
}

//! Return dashstyle as a double sequence.
void Painter::dashStyle(std::vector<double> &dashes, double &offset) const
{
  dashes.clear();
  offset = 0.0;
  String s = dashStyle();

  int i = s.find("[");
  int j = s.find("]");
  if (i < 0 || j < 0)
    return;

  Lex lex(s.substr(i+1, j - i - 1));
  while (!lex.eos())
    dashes.push_back(lex.getDouble());
  offset = Lex(s.substr(j+1)).getDouble();
}

//! Set line cap.
/*! If \a cap is EDefaultCap, the current setting remains unchanged. */
void Painter::setLineCap(TLineCap cap)
{
  assert(!iInPath);
  if (cap != EDefaultCap)
    iState.back().iLineCap = cap;
}

//! Set line join.
/*! If \a join is EDefaultJoin, the current setting remains unchanged. */
void Painter::setLineJoin(TLineJoin join)
{
  assert(!iInPath);
  if (join != EDefaultJoin)
    iState.back().iLineJoin = join;
}

//! Set fill rule (wind or even-odd).
/*! If the rule is EDefaultRule, the current setting remains unchanged. */
void Painter::setFillRule(TFillRule rule)
{
  assert(!iInPath);
  if (rule != EDefaultRule)
    iState.back().iFillRule = rule;
}

//! Set opacity.
void Painter::setOpacity(Attribute opaq)
{
  assert(!iInPath);
  iState.back().iOpacity = lookup(EOpacity, opaq).number();
}

//! Set stroke opacity.
void Painter::setStrokeOpacity(Attribute opaq)
{
  assert(!iInPath);
  iState.back().iStrokeOpacity = lookup(EOpacity, opaq).number();
}

//! Set tiling pattern.
/*! If \a tiling is not \c normal, resets the gradient pattern. */
void Painter::setTiling(Attribute tiling)
{
  assert(!iInPath);
  iState.back().iTiling = tiling;
  if (!tiling.isNormal())
    iState.back().iGradient = Attribute::NORMAL();
}

//! Set gradient fill.
/*! If \a grad is not \c normal, resets the tiling pattern. */
void Painter::setGradient(Attribute grad)
{
  assert(!iInPath);
  iState.back().iGradient = grad;
  if (!grad.isNormal())
    iState.back().iTiling = Attribute::NORMAL();
}

//! Set symbol stroke color, resolving symbolic color.
void Painter::setSymStroke(Attribute color)
{
  assert(!iInPath);
  if (color == Attribute::SYM_STROKE())
    iState.back().iSymStroke = (++iState.rbegin())->iSymStroke;
  else if (color == Attribute::SYM_FILL())
    iState.back().iSymStroke = (++iState.rbegin())->iSymFill;
  else
    iState.back().iSymStroke = lookup(EColor, color).color();
}

//! Set symbol fill color, resolving symbolic color.
void Painter::setSymFill(Attribute color)
{
  assert(!iInPath);
  if (color == Attribute::SYM_STROKE())
    iState.back().iSymFill = (++iState.rbegin())->iSymStroke;
  else if (color == Attribute::SYM_FILL())
    iState.back().iSymFill = (++iState.rbegin())->iSymFill;
  else
    iState.back().iSymFill = lookup(EColor, color).color();
}

//! Set symbol pen, resolving symbolic pen.
void Painter::setSymPen(Attribute pen)
{
  assert(!iInPath);
  if (pen == Attribute::SYM_PEN())
    iState.back().iSymPen = (++iState.rbegin())->iSymPen;
  else
    iState.back().iSymPen = lookup(EPen, pen).number();
}

//! Set full graphics state at once.
void Painter::setState(const State &state)
{
  iState.back() = state;
}

// --------------------------------------------------------------------

// Coordinate for bezier approximation for quarter circle.
const double BETA = 0.55228474983079334;
const double PI15 = IpePi + IpeHalfPi;

//! Draw an arc of the unit circle of length \a alpha.
/*! PDF does not have an "arc" or "circle" primitive, so to draw an
  arc, circle, or ellipse, Ipe has to translate it into a sequence of
  Bezier curves.

  The approximation is based on the following: The unit circle arc
  from (1,0) to (cos a, sin a) be approximated by a Bezier spline with
  control points (1, 0), (1, beta) and their mirror images along the
  line with slope a/2, where
  beta = 4.0 * (1.0 - cos(a/2)) / (3 * sin(a/2))

  Ipe draws circles by drawing four Bezier curves for the quadrants,
  and arcs by patching together quarter circle approximations with a
  piece computed from the formula above.

  \a alpha is normalized to [0, 2 pi], and applied starting from the
  point (1,0).

  The function generates a sequence of Bezier splines as calls to
  curveTo.  It is assumed that the caller has already executed a
  moveTo to the beginning of the arc at (1,0).

  This function may modify the transformation matrix.
*/
void Painter::drawArcAsBezier(double alpha)
{
  // Vector p0(1.0, 0.0);
  Vector p1(1.0, BETA);
  Vector p2(BETA, 1.0);
  Vector p3(0.0, 1.0);
  Vector q1(-BETA, 1.0);
  Vector q2(-1.0, BETA);
  Vector q3(-1.0, 0.0);

  double begAngle = 0.0;
  if (alpha > IpeHalfPi) {
    curveTo(p1, p2, p3);
    begAngle = IpeHalfPi;
  }
  if (alpha > IpePi) {
    curveTo(q1, q2, q3);
    begAngle = IpePi;
  }
  if (alpha > PI15) {
    curveTo(-p1, -p2, -p3);
    begAngle = PI15;
  }
  if (alpha >= IpeTwoPi) {
    curveTo(-q1, -q2, -q3);
  } else {
    alpha -= begAngle;
    double alpha2 = alpha / 2.0;
    double divi = 3.0 * sin(alpha2);
    if (divi == 0.0)
      return;  // alpha2 is close to zero
    double beta = 4.0 * (1.0 - cos(alpha2)) / divi;
    Linear m = Linear(Angle(begAngle));

    Vector pp1(1.0, beta);
    Vector pp2 = Linear(Angle(alpha)) * Vector(1.0, -beta);
    Vector pp3 = Vector(Angle(alpha));

    curveTo(m * pp1, m * pp2, m * pp3);
  }
}

// --------------------------------------------------------------------

//! Perform graphics state push on output medium.
void Painter::doPush()
{
  // nothing
}

//! Perform graphics state pop on output medium.
void Painter::doPop()
{
  // nothing
}

//! Perform new path operator.
void Painter::doNewPath()
{
  // nothing
}

//! Perform moveto operator.
/*! The transformation matrix has already been applied. */
void Painter::doMoveTo(const Vector &)
{
  // nothing
}

//! Perform lineto operator.
/*! The transformation matrix has already been applied. */
void Painter::doLineTo(const Vector &)
{
  // nothing
}

//! Perform curveto operator.
/*! The transformation matrix has already been applied. */
void Painter::doCurveTo(const Vector &, const Vector &, const Vector &)
{
  // nothing
}

//! Draw an elliptic arc.
/*! The default implementations calls drawArcAsBezier().  The
  transformation matrix has not yet been applied to \a arc. */
void Painter::doDrawArc(const Arc &arc)
{
  pushMatrix();
  transform(arc.iM);
  if (arc.isEllipse()) {
    moveTo(Vector(1,0));
    drawArcAsBezier(IpeTwoPi);
  } else {
    transform(Linear(arc.iAlpha));
    double alpha = Angle(arc.iBeta - arc.iAlpha).normalize(0.0);
    drawArcAsBezier(alpha);
  }
  popMatrix();
}

//! Perform closepath operator.
void Painter::doClosePath()
{
  // nothing
}

//! Actually draw the path.
void Painter::doDrawPath(TPathMode)
{
  // nothing
}

//! Draw a bitmap.
void Painter::doDrawBitmap(Bitmap)
{
  // nothing
}

//! Draw a text object.
void Painter::doDrawText(const Text *)
{
  // nothing
}

//! Draw a symbol.
/*! The default implementation calls the draw method of the
    object. Only PDF drawing overrides this to reuse a PDF XForm. */
void Painter::doDrawSymbol(Attribute symbol)
{
  const Symbol *sym = iAttributeMap ?
    cascade()->findSymbol(iAttributeMap->map(ESymbol, symbol)) :
    cascade()->findSymbol(symbol);
  if (sym)
    sym->iObject->draw(*this);
}

//! Add a clip path
void Painter::doAddClipPath()
{
  // nothing
}
// --------------------------------------------------------------------
