// --------------------------------------------------------------------
// Shapes
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

#include "ipeshape.h"
#include "ipepainter.h"

using namespace ipe;

// --------------------------------------------------------------------

inline bool snapVertex(const Vector &mouse, const Vector &v,
		       Vector &pos, double &bound)
{
  return v.snap(mouse, pos, bound);
}

inline void snapBezier(const Vector &mouse, const Bezier &bez,
		       Vector &pos, double &bound)
{
  double t;
  (void) bez.snap(mouse, t, pos, bound);
}

// --------------------------------------------------------------------

/*! \class ipe::CurveSegment
  \ingroup geo
  \brief A segment on an SubPath.

  A segment is either an elliptic arc, a straight segment, or a spline
  curve, depending on its type().  This is a lightweight object,
  created on the fly by Curve::segment().  There is no public
  constructor, so the only way to create such an object is through
  that method.

  The type() is one of the following:

  - \c ESegment: the segment has two control points, and represents a
    line segment.

  - \c ESpline: a B-spline curve with n control points.  The first and
    last control point's knot value is repeated three times, so the
    curve begins and ends in these points. A spline with 4 control
    points is a single Bezier curve with those control points.  A
    spline with 3 control points is defined to be a quadratic Bezier
    curve with those control points.

  - \c ECardinalSpline: a cardinal spline curve with n control points.
    The curve passes through the control points in the given order.
    The tension is currently fixed at 0.5.

  - \c ESpiroSpline: a clothoid spline curve with n control points,
    computed using Raph Levien's spiro code (through libspiro).

  - \c EOldSpline: an incorrectly defined B-spline, used by Ipe for
    many years.  Supported for compatibility.

  - \c EArc: an elliptic arc, with begin and end point.  The
    supporting ellipse is defined by the matrix(), it is the image
    under the affine transformation matrix() of the unit circle.
    matrix() is such that its inverse transforms both start and end
    position to points (nearly) on the unit circle. The arc is the
    image of the positively (counter-clockwise) directed arc from the
    pre-image of the start position to the pre-image of the end
    position.  Whether this is a positively or negatively oriented arc
    in user space depends on the matrix.

*/

//! Create a segment.
/*! Matrix \a m defaults to null, for all segments but arcs. */
CurveSegment::CurveSegment(const Curve *curve, int index)
  : iCurve(curve), index(index)
{
  if (type() == ESpiroSpline) {
    iNumCP = iCurve->iSeg[index].iLastCP - iCurve->iSeg[index].iBezier;
  } else {
    int cpbg = (index > 0) ? iCurve->iSeg[index-1].iLastCP : 0;
    iNumCP = iCurve->iSeg[index].iLastCP - cpbg + 1;
  }
}

//! Return segment as Arc.
/*! Panics if segment is not an arc. */
Arc CurveSegment::arc() const
{
  assert(type() == EArc);
  return Arc(matrix(), cp(0), cp(1));
}

//! Tension (if Type() == ECardinalSpline)
/*! Panics if segment is not a cardinal spline. */
float CurveSegment::tension() const
{
  assert(type() == ECardinalSpline);
  return iCurve->iSeg[index].iTension;
}

//! Convert spline curve to a sequence of Bezier splines.
void CurveSegment::beziers(std::vector<Bezier> &bez) const
{
  switch (type()) {
  case EOldSpline:
    Bezier::oldSpline(countCP(), cps(), bez);
    break;
  case ESpline:
    Bezier::spline(countCP(), cps(), bez);
    break;
  case ECardinalSpline:
    Bezier::cardinalSpline(countCP(), cps(), tension(), bez);
    break;
  case ESpiroSpline: {
    // use pre-computed Bezier curve
    int cpbg = (index > 0) ? iCurve->iSeg[index-1].iLastCP : 0;
    const Vector *cp = iCurve->iCP.data() + cpbg;
    const Vector *fin = iCurve->iCP.data() + iCurve->iSeg[index].iBezier - 1;
    while (cp < fin) {
      bez.push_back(Bezier(cp[0], cp[1], cp[2], cp[3]));
      cp += 3;
    }
    break; }
  default:
    break;
  }
}

//! Draw the segment.
/*! Current position of the \a painter is already on first control
  point. */
void CurveSegment::draw(Painter &painter) const
{
  switch (type()) {
  case ESegment:
    painter.lineTo(cp(1));
    break;
  case EOldSpline:
  case ESpline:
  case ECardinalSpline:
  case ESpiroSpline: {
    std::vector<Bezier> bez;
    beziers(bez);
    for (const auto & b : bez)
      painter.curveTo(b);
    break; }
  case EArc:
    painter.drawArc(arc());
    break;
  }
}

//! Add segment to bounding box.
/*! Does not assume that first control point has already been added.

  If \a cpf is true, then control points of splines, Bezier curves,
  and the center of arcs are included in the bbox (so that snapping
  can find them).  Otherwise, a tight bounding box for the geometric
  object itself is computed.
*/
void CurveSegment::addToBBox(Rect &box, const Matrix &m, bool cpf) const
{
  switch (type()) {
  case ESegment:
    box.addPoint(m * cp(0));
    box.addPoint(m * cp(1));
    break;
  case EArc:
    box.addRect((m * arc()).bbox());
    if (cpf)
      box.addPoint((m * matrix()).translation());
    break;
  case ESpline:
  case EOldSpline:
  case ECardinalSpline:
  case ESpiroSpline:
    if (cpf) {
      for (int i = 0; i < countCP(); ++i)
	box.addPoint(m * cp(i));
    } else {
      std::vector<Bezier> bez;
      beziers(bez);
      for (const auto & b : bez)
	box.addRect((m * b).bbox());
    }
    break;
  }
}

//! Return distance to the segment.
double CurveSegment::distance(const Vector &v, const Matrix &m,
			      double bound) const
{
  switch (type()) {
  case ESegment:
    return Segment(m * cp(0), m * cp(1)).distance(v, bound);
  case EArc:
    return (m * arc()).distance(v, bound);
  case EOldSpline:
  case ESpline:
  case ECardinalSpline:
  case ESpiroSpline: {
    std::vector<Bezier> bez;
    beziers(bez);
    double d = bound;
    double d1;
    for (const auto & b : bez) {
      if ((d1 = (m * b).distance(v, d)) < d)
	d = d1;
    }
    return d; }
  default: // make compiler happy
    return bound;
  }
}

//! Snap to vertex of the segment.
/*! The method assumes that the first control point has already been
  tested. */
void CurveSegment::snapVtx(const Vector &mouse, const Matrix &m,
			   Vector &pos, double &bound, bool ctl) const
{
  switch (type()) {
  case ESegment:
    if (ctl)
      // segment midpoint
      snapVertex(mouse, m * (0.5 * (cp(0) + cp(1))), pos, bound);
    else
      snapVertex(mouse, m * cp(1), pos, bound);
    break;
  case EArc:
    // snap to center and endpoints
    if (ctl)
      // center
      snapVertex(mouse, (m * matrix()).translation(), pos, bound);
    else
      snapVertex(mouse, m * cp(1), pos, bound);
    break;
  case ESpline:
  case ECardinalSpline:
  case ESpiroSpline:
  case EOldSpline:
    // real end point is cp(countCP() - 1)
    // snap to all control points
    if (ctl) {
      for (int i = 1; i < countCP() - 1; ++i)
	snapVertex(mouse, m * cp(i), pos, bound);
    } else
      snapVertex(mouse, m * cp(countCP() - 1), pos, bound);
    break;
  }
}

void CurveSegment::snapBnd(const Vector &mouse, const Matrix &m,
			   Vector &pos, double &bound) const
{
  switch (type()) {
  case ESegment:
    Segment(m * cp(0), m * cp(1)).snap(mouse, pos, bound);
    break;
  case EArc: {
    Arc a = m * arc();
    Vector pos1;
    Angle angle;
    double d1 = a.distance(mouse, bound, pos1, angle);
    if (d1 < bound) {
      bound = d1;
      pos = pos1;
    }
    break; }
  case ESpline:
  case EOldSpline:
  case ECardinalSpline:
  case ESpiroSpline: {
    std::vector<Bezier> bez;
    beziers(bez);
    for (const auto & b : bez)
      snapBezier(mouse, m * b, pos, bound);
    break; }
  }
}

// --------------------------------------------------------------------

/*! \class ipe::Curve
  \ingroup geo
  \brief Subpath consisting of a sequence of CurveSegment's.
*/

//! Create an empty, open subpath
Curve::Curve()
{
  iClosed = false;
}

//! Append a straight segment to the subpath.
void Curve::appendSegment(const Vector &v0, const Vector &v1)
{
  if (iSeg.empty())
    iCP.push_back(v0);
  assert(v0 == iCP.back());
  iCP.push_back(v1);
  Seg seg;
  seg.iType = CurveSegment::ESegment;
  seg.iLastCP = iCP.size() - 1;
  seg.iMatrix = iM.size() - 1;
  iSeg.push_back(seg);
}

//! Append elliptic arc to the subpath.
void Curve::appendArc(const Matrix &m, const Vector &v0, const Vector &v1)
{
  if (iSeg.empty())
    iCP.push_back(v0);
  assert(v0 == iCP.back());
  iCP.push_back(v1);
  iM.push_back(m);
  Seg seg;
  seg.iType = CurveSegment::EArc;
  seg.iLastCP = iCP.size() - 1;
  seg.iMatrix = iM.size() - 1;
  iSeg.push_back(seg);
}

//! Append spline curve.
void Curve::appendSpline(const std::vector<Vector> &v, CurveSegment::Type type)
{
  assert(type == CurveSegment::ESpline ||
	 type == CurveSegment::ECardinalSpline ||
	 type == CurveSegment::EOldSpline);
  if (iSeg.empty())
    iCP.push_back(v[0]);
  assert(v[0] == iCP.back());
  for (int i = 1; i < size(v); ++i)
    iCP.push_back(v[i]);
  Seg seg;
  seg.iType = type;
  seg.iLastCP = iCP.size() - 1;
  seg.iMatrix = 0;
  iSeg.push_back(seg);
}

//! Append a cardinal spline curve.
void Curve::appendCardinalSpline(const std::vector<Vector> &v, float tension)
{
  appendSpline(v, CurveSegment::ECardinalSpline);
  iSeg.back().iTension = tension;
}

//! Append a spiro spline curve.
void Curve::appendSpiroSpline(const std::vector<Vector> &v)
{
  if (iSeg.empty())
    iCP.push_back(v[0]);
  assert(v[0] == iCP.back());
  // compute Bezier representation
  std::vector<Bezier> bez;
  Bezier::spiroSpline(v.size(), v.data(), bez);
  // save Bezier cps
  for (const auto & b : bez) {
    iCP.push_back(b.iV[1]);
    iCP.push_back(b.iV[2]);
    iCP.push_back(b.iV[3]);
  }
  int bezIndex = iCP.size() - 1;
  // now save spiro control points, including first one
  for (int i = 0; i < size(v); ++i)
    iCP.push_back(v[i]);
  Seg seg;
  seg.iType = CurveSegment::ESpiroSpline;
  seg.iLastCP = iCP.size() - 1;
  seg.iBezier = bezIndex;
  iSeg.push_back(seg);
}

//! Append a spiro spline curve with precomputed Bezier control points.
void Curve::appendSpiroSplinePrecomputed(const std::vector<Vector> &v, int sep)
{
  if (iSeg.empty())
    iCP.push_back(v[0]);
  assert(v[0] == iCP.back());
  // add Bezier representation
  for (int i = 1; i < sep; ++i)
    iCP.push_back(v[i]);
  // need to insert final target and origin point
  iCP.push_back(v.back());
  int bezIndex = iCP.size() - 1;
  iCP.push_back(v.front());
  // now save actual spiro control points, including first one
  for (int i = sep; i < size(v); ++i)
    iCP.push_back(v[i]);
  Seg seg;
  seg.iType = CurveSegment::ESpiroSpline;
  seg.iLastCP = iCP.size() - 1;
  seg.iBezier = bezIndex;
  iSeg.push_back(seg);
}

//! Set whether subpath is closed or not.
/*! Must be called after all segments have been added to the path. */
void Curve::setClosed(bool closed)
{
  assert(!iSeg.empty() && !iClosed);
  iClosed = closed;
  if (iClosed)
    appendSegment(iCP.back(), iCP.front());
}

SubPath::Type Curve::type() const
{
  return ECurve;
}

const Curve *Curve::asCurve() const
{
  return this;
}

//! Return segment.
/*! If \a i is negative, elements from the end are returned.  If
  i == countSegments(), the closing segment of a closed path is returned.
*/
CurveSegment Curve::segment(int i) const
{
  if (i < 0)
    i += iSeg.size();
  return CurveSegment(this, i);
}

void Curve::save(Stream &stream) const
{
  // moveto first control point
  stream << iCP[0] << " m\n";
  int vtx = 1; // next control point
  int mat = 0;
  std::vector<Seg>::const_iterator fin = iClosed ? iSeg.end() - 1 : iSeg.end();
  for (std::vector<Seg>::const_iterator it = iSeg.begin(); it != fin; ++it) {
    switch (it->iType) {
    case CurveSegment::ESegment:
      assert(vtx == it->iLastCP);
      stream << iCP[vtx++] << " l\n";
      break;
    case CurveSegment::EArc:
      assert(vtx == it->iLastCP && mat == it->iMatrix);
      stream << iM[mat++] << " " << iCP[vtx++] << " a\n";
      break;
    case CurveSegment::EOldSpline:
      while (vtx < it->iLastCP)
	stream << iCP[vtx++] << "\n";
      stream << iCP[vtx++] << " s\n";
      break;
    case CurveSegment::ESpline:
      while (vtx < it->iLastCP)
	stream << iCP[vtx++] << "\n";
      stream << iCP[vtx++] << " c\n";
      break;
    case CurveSegment::ECardinalSpline:
      while (vtx < it->iLastCP)
	stream << iCP[vtx++] << "\n";
      stream << iCP[vtx++] << " " << it->iTension << " C\n";
      break;
    case CurveSegment::ESpiroSpline: {
      while (vtx < it->iBezier - 1)
	stream << iCP[vtx++] << "\n";
      stream << iCP[it->iBezier - 1] << " *\n";
      vtx = it->iBezier + 2; // skip repeated last and first cp
      while (vtx < it->iLastCP)
	stream << iCP[vtx++] << "\n";
      stream << iCP[vtx++] << " L\n"; // in honor of Raph Levien
      break; }
    }
  }
  if (closed())
    stream << "h\n";
}

void Curve::draw(Painter &painter) const
{
  painter.moveTo(iCP[0]);
  for (int i = 0; i < countSegments(); ++i)
    segment(i).draw(painter);
  if (closed())
    painter.closePath();
}

void Curve::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  for (int i = 0; i < countSegments(); ++i)
    segment(i).addToBBox(box, m, cp);
}

double Curve::distance(const Vector &v, const Matrix &m,
		       double bound) const
{
  double d = bound;
  for (int i = 0; i < countSegmentsClosing(); ++i) {
    double d1 = segment(i).distance(v, m, d);
    if (d1 < d)
      d = d1;
  }
  return d;
}

void Curve::snapVtx(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound, bool ctl) const
{
  if (!ctl)
    snapVertex(mouse, m * segment(0).cp(0), pos, bound);
  else if (closed())
    // midpoint of closing segment
    closingSegment().snapVtx(mouse, m, pos, bound, ctl);
  for (int i = 0; i < countSegments(); ++i)
    segment(i).snapVtx(mouse, m, pos, bound, ctl);
}

void Curve::snapBnd(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  snapVertex(mouse, m * segment(0).cp(0), pos, bound);
  for (int i = 0; i < countSegmentsClosing(); ++i)
    segment(i).snapBnd(mouse, m, pos, bound);
}

//! Returns the closing segment of a closed path.
/*! This method panics if the Curve is not closed. */
CurveSegment Curve::closingSegment() const
{
  assert(iClosed);
  return CurveSegment(this, iSeg.size() - 1);
}

// --------------------------------------------------------------------

/*! \class ipe::SubPath
  \ingroup geo
  \brief A subpath of a Path.

  A subpath is either open, or closed.  There are two special kinds of
  closed subpaths, namely ellipses and closed B-splines.
*/

//! Implementation of pure virtual destructor.
SubPath::~SubPath()
{
  // nothing
}

//! Is this subpath closed?
/*! Default implementation returns \c true. */
bool SubPath::closed() const
{
  return true;
}

//! Return this object as an Ellipse, or nullptr if it's not an ellipse.
const Ellipse *SubPath::asEllipse() const
{
  return nullptr;
}

//! Return this object as an ClosedSpline, or nullptr if it's not a closed spline.
const ClosedSpline *SubPath::asClosedSpline() const
{
  return nullptr;
}

//! Return this object as an Curve, or else nullptr.
const Curve *SubPath::asCurve() const
{
  return nullptr;
}

// --------------------------------------------------------------------

/*! \class ipe::Ellipse
  \ingroup geo
  \brief An ellipse subpath
*/

Ellipse::Ellipse(const Matrix &m)
  : iM(m)
{
  // nothing
}

SubPath::Type Ellipse::type() const
{
  return EEllipse;
}

const Ellipse *Ellipse::asEllipse() const
{
  return this;
}

void Ellipse::save(Stream &stream) const
{
  stream << matrix() << " e\n";
}

void Ellipse::draw(Painter &painter) const
{
  painter.drawArc(Arc(iM));
}

void Ellipse::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  box.addRect(Arc(m * iM).bbox());
}

double Ellipse::distance(const Vector &v, const Matrix &m,
			 double bound) const
{
  Arc arc(m * iM);
  return arc.distance(v, bound);
}

//! snaps to center of ellipse.
void Ellipse::snapVtx(const Vector &mouse, const Matrix &m,
		      Vector &pos, double &bound, bool ctl) const
{
  if (ctl)
    snapVertex(mouse, (m * iM).translation(), pos, bound);
}

void Ellipse::snapBnd(const Vector &mouse, const Matrix &m,
		      Vector &pos, double &bound) const
{
  Arc arc(m * iM);
  Vector pos1;
  Angle angle;
  double d1 = arc.distance(mouse, bound, pos1, angle);
  if (d1 < bound) {
    bound = d1;
    pos = pos1;
  }
}

// --------------------------------------------------------------------

/*! \class ipe::ClosedSpline
  \ingroup geo
  \brief A closed B-spline curve.
*/

ClosedSpline::ClosedSpline(const std::vector<Vector> &v)
{
  assert(v.size() >= 3);
  std::copy(v.begin(), v.end(), std::back_inserter(iCP));
}

SubPath::Type ClosedSpline::type() const
{
  return EClosedSpline;
}

const ClosedSpline *ClosedSpline::asClosedSpline() const
{
  return this;
}

void ClosedSpline::save(Stream &stream) const
{
  for (int i = 0; i < size(iCP) - 1; ++i)
    stream << iCP[i] << "\n";
  stream << iCP.back() << " u\n";
}

void ClosedSpline::draw(Painter &painter) const
{
  std::vector<Bezier> bez;
  beziers(bez);
  painter.moveTo(bez.front().iV[0]);
  for (const auto & b : bez)
    painter.curveTo(b);
  painter.closePath();
}

void ClosedSpline::addToBBox(Rect &box, const Matrix &m, bool cpf) const
{
  if (cpf) {
    for (const auto & cp : iCP)
      box.addPoint(m * cp);
  } else {
    std::vector<Bezier> bez;
    beziers(bez);
    for (const auto & b : bez)
      box.addRect((m * b).bbox());
  }
}

double ClosedSpline::distance(const Vector &v, const Matrix &m,
			      double bound) const
{
  std::vector<Bezier> bez;
  beziers(bez);
  double d = bound;
  double d1;
  for (const auto & b : bez) {
    if ((d1 = (m * b).distance(v, d)) < d)
      d = d1;
  }
  return d;
}

void ClosedSpline::beziers(std::vector<Bezier> &bez) const
{
  Bezier::closedSpline(iCP.size(), &iCP.front(), bez);
}

void ClosedSpline::snapVtx(const Vector &mouse, const Matrix &m,
			   Vector &pos, double &bound, bool ctl) const
{
  if (ctl) {
    // snap to control points
    for (const auto & cp : iCP)
      snapVertex(mouse, m * cp, pos, bound);
  }
}

void ClosedSpline::snapBnd(const Vector &mouse, const Matrix &m,
			   Vector &pos, double &bound) const
{
  std::vector<Bezier> bez;
  beziers(bez);
  for (const auto & b : bez)
    snapBezier(mouse, m * b, pos, bound);
}

// --------------------------------------------------------------------

/*! \class ipe::Shape
  \ingroup geo
  \brief A geometric shape, consisting of several (open or closed) subpaths.

  This class represents vector graphics geometry following the PDF
  "path", but is actually a bit more complicated since we add new
  subtypes: arcs, parabolas, uniform B-splines (in PDF, all of these
  are converted to cubic Bezier splines).

  A Shape consists of a set of subpaths (SubPath), each of which
  is either open or closed, and which are rendered by stroking and
  filling as a whole. The distinction between open and closed is
  meaningful for stroking only, for filling any open subpath is
  implicitely closed.  Stroking a set of subpaths is identical to
  stroking them individually.  This is not true for filling: using
  several subpaths, one can construct objects with holes, and more
  complicated pattern.

  A subpath is either an Ellipse (a complete, closed ellipse), a
  ClosedSpline (a closed uniform B-spline curve), or a Curve.  A curve
  consists of a sequence of segments.  Segments are either straight, a
  quadratic Bezier spline, a cubic Bezier spline, an elliptic arc, or
  a uniform cubic B-spline.

  Shape is implemented using reference counting and can be copied and
  passed by value efficiently.  The only mutator methods are
  appendSubPath() and load(), which can only be called during
  construction of the Shape (that is, before its implementation has
  been shared).
*/

//! Construct an empty shape (zero subpaths).
Shape::Shape()
{
  iImp = new Imp;
  iImp->iRefCount = 1;
}

//! Copy constructor (constant time).
Shape::Shape(const Shape &rhs)
{
  iImp = rhs.iImp;
  iImp->iRefCount++;
}

//! Destructor (takes care of reference counting).
Shape::~Shape()
{
  if (iImp->iRefCount == 1)
    delete iImp;
  else
    iImp->iRefCount--;
}

//! Assignment operator (constant-time).
Shape &Shape::operator=(const Shape &rhs)
{
  if (this != &rhs) {
    if (iImp->iRefCount == 1)
      delete iImp;
    else
      iImp->iRefCount--;
    iImp = rhs.iImp;
    iImp->iRefCount++;
  }
  return *this;
}

// --------------------------------------------------------------------

//! Convenience function: create a rectangle shape.
Shape::Shape(const Rect &rect)
{
  iImp = new Imp;
  iImp->iRefCount = 1;

  Curve *sp = new Curve;
  sp->appendSegment(rect.bottomLeft(), rect.bottomRight());
  sp->appendSegment(rect.bottomRight(), rect.topRight());
  sp->appendSegment(rect.topRight(), rect.topLeft());
  sp->setClosed(true);
  appendSubPath(sp);
}

//! Convenience function: create a single line segment.
Shape::Shape(const Segment &seg)
{
  iImp = new Imp;
  iImp->iRefCount = 1;

  Curve *sp = new Curve;
  sp->appendSegment(seg.iP, seg.iQ);
  appendSubPath(sp);
}

//! Convenience function: create circle with \a center and \a radius.
Shape::Shape(const Vector &center, double radius)
{
  iImp = new Imp;
  iImp->iRefCount = 1;

  appendSubPath(new Ellipse(Matrix(radius, 0.0, 0.0, radius,
				   center.x, center.y)));
}

//! Convenience function: create circular arc.
/*! If \a alpha1 is larger than \a alpha0, the arc is oriented positively,
  otherwise negatively. */
Shape::Shape(const Vector &center, double radius,
	     double alpha0, double alpha1)
{
  iImp = new Imp;
  iImp->iRefCount = 1;

  Matrix m = Matrix(radius, 0, 0, radius, center.x, center.y);
  Vector v0 = m * Vector(Angle(alpha0));
  Vector v1 = m * Vector(Angle(alpha1));
  if (alpha1 < alpha0)
    // negative orientation
    m = m * Linear(1, 0, 0, -1);
  Curve *sp = new Curve;
  sp->appendArc(m, v0, v1);
  appendSubPath(sp);
}

// --------------------------------------------------------------------

//! Is this Shape a single straight segment?
bool Shape::isSegment() const
{
  if (countSubPaths() != 1)
    return false;
  const SubPath *p = subPath(0);
  if (p->type() != SubPath::ECurve || p->closed())
    return false;
  const Curve *c = p->asCurve();
  if (c->countSegments() != 1 ||
      c->segment(0).type() != CurveSegment::ESegment)
    return false;
  return true;
}

//! Add shape (transformed by \a m) to \a box.
void Shape::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  for (int i = 0; i < countSubPaths(); ++i)
    subPath(i)->addToBBox(box, m, cp);
}

double Shape::distance(const Vector &v, const Matrix &m, double bound) const
{
  double d = bound;
  for (int i = 0; i < countSubPaths(); ++i) {
    double d1 = subPath(i)->distance(v, m, d);
    if (d1 < d)
      d = d1;
  }
  return d;
}

void Shape::snapVtx(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound, bool ctl) const
{
  for (int i = 0; i < countSubPaths(); ++i)
    subPath(i)->snapVtx(mouse, m, pos, bound, ctl);
}

void Shape::snapBnd(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  for (int i = 0; i < countSubPaths(); ++i)
    subPath(i)->snapBnd(mouse, m, pos, bound);
}

//! Append a SubPath to shape.
/*! The Shape will take ownership of the subpath.  This method can
  only be used during construction of the Shape.  It will panic if the
  implementation has been shared.
*/
void Shape::appendSubPath(SubPath *sp)
{
  assert(iImp->iRefCount == 1);
  iImp->iSubPaths.push_back(sp);
}

//! Draw the Shape as a path to \a painter.
/*! Does not call newPath() on \a painter. */
void Shape::draw(Painter &painter) const
{
  for (int i = 0; i < countSubPaths(); ++i)
    subPath(i)->draw(painter);
}

Shape::Imp::~Imp()
{
  // delete the subpaths
  for (SubPathSeq::iterator it = iSubPaths.begin();
       it != iSubPaths.end(); ++it) {
    delete *it;
    *it = nullptr;
  }
}

static Vector getVector(std::vector<double> &args)
{
  Vector v;
  v.x = args[0];
  v.y = args[1];
  args.erase(args.begin(), args.begin() + 2);
  return v;
}

static Matrix getMatrix(std::vector<double> &args)
{
  Matrix m;
  for (int i = 0; i < 6; ++i)
    m.a[i] = args[i];
  args.erase(args.begin(), args.begin() + 6);
  return m;
}

//! Save Shape onto XML stream.
void Shape::save(Stream &stream) const
{
  for (int i = 0; i < countSubPaths(); ++i)
    subPath(i)->save(stream);
}

//! Create a Shape from XML data.
/*! Appends subpaths from XML data to the current Shape.  Returns
  false if the path syntax is incorrect (the Shape will be in an
  inconsistent state and must be discarded).

  This method can only be used during construction of the Shape.  It
  will panic if the implementation has been shared. */
bool Shape::load(String data)
{
  assert(iImp->iRefCount == 1);
  Lex stream(data);
  String word;
  String type;
  Curve *sp = nullptr;
  Vector org;
  int mid = -1;
  std::vector<double> args;
  do {
    if (stream.token() == "h") { // closing path
      if (!sp)
	return false;
      stream.nextToken(); // eat token
      sp->setClosed(true);
      sp = nullptr;
      mid = -1;
    } else if (stream.token() == "m") {
      if (args.size() != 2)
	return false;
      stream.nextToken(); // eat token
      // begin new subpath
      sp = new Curve;
      appendSubPath(sp);
      org = getVector(args);
      mid = -1;
    } else if (stream.token() == "l") {
      if (!sp || args.size() != 2)
	return false;
      stream.nextToken(); // eat token
      while (!args.empty()) {
	Vector v = getVector(args);
	sp->appendSegment(org, v);
	org = v;
      }
      mid = -1;
    } else if (stream.token() == "a") {
      if (!sp || args.size() != 8)
	return false;
      stream.nextToken();
      Matrix m = getMatrix(args);
      if (m.determinant() == 0)
	return false; // don't accept zero-radius arc
      Vector v1 = getVector(args);
      sp->appendArc(m, org, v1);
      org = v1;
      mid = -1;
    } else if (stream.token() == "s" || stream.token() == "q"
	       || stream.token() == "c" || stream.token() == "C"
	       || stream.token() == "L") {
      size_t parity = (stream.token() == "C") ? 1 : 0;
      if (!sp || args.size() < 2 || (args.size() % 2 != parity))
	return false;
      String typeToken = stream.token();
      stream.nextToken();
      std::vector<Vector> v;
      v.push_back(org);
      while (args.size() >= 2)
	v.push_back(getVector(args));
      if (typeToken == "s")
	sp->appendOldSpline(v);
      else if (typeToken == "C") {
	sp->appendCardinalSpline(v, float(args.back()));
	args.pop_back();  // remove tension
      } else if (typeToken == "L") {
	if (mid >= 0) {
	  if ((mid % 2) != 0)  // wrong parity
	    return false;
	  mid = (mid / 2);     // count Vectors
	  if (mid < 2 || (mid % 3) != 2)  // * marker in wrong position
	    return false;
	  sp->appendSpiroSplinePrecomputed(v, mid + 1);
	} else
	  sp->appendSpiroSpline(v);
      } else
	sp->appendSpline(v);
      org = v.back();
      mid = -1;
    } else if (stream.token() == "e") {
      if (args.size() != 6)
	return false;
      stream.nextToken();
      sp = nullptr;
      mid = -1;
      Ellipse *e = new Ellipse(getMatrix(args));
      appendSubPath(e);
    } else if (stream.token() == "u") {
      if (args.size() < 6 || (args.size() % 2 != 0))
	return false;
      stream.nextToken();
      sp = nullptr;
      mid = -1;
      std::vector<Vector> v;
      while (!args.empty())
	v.push_back(getVector(args));
      ClosedSpline *e = new ClosedSpline(v);
      appendSubPath(e);
    } else if (stream.token() == "*") {
      // remember position in args
      mid = args.size();
      stream.nextToken();
    } else { // must be a number
      double num;
      stream >> num;
      args.push_back(num);
    }
    stream.skipWhitespace();
  } while (!stream.eos());
  // we allow the last subpath to be empty (a single trailing "m" operator)
  int sbn = countSubPaths();
  if (sbn > 0 && subPath(sbn-1)->asCurve() && subPath(sbn-1)->asCurve()->countSegments() == 0)
    iImp->iSubPaths.pop_back();
  // sanity checks
  for (int i = 0; i < countSubPaths(); ++i) {
    if (subPath(i)->asCurve() && subPath(i)->asCurve()->countSegments() == 0)
      return false;
  }
  return true;
}

// --------------------------------------------------------------------
