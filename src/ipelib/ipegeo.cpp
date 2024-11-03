// --------------------------------------------------------------------
// Ipe geometry primitives
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

/*! \defgroup geo Ipe Geometry
  \brief Geometric primitives for Ipe.

  The IpeGeo module provides a few classes for constant-size geometric
  primitives, such as vector, axis-aligned rectangles, lines, rays,
  line segments, etc.

*/

#ifdef IPE_GSL
#include <gsl/gsl_min.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_poly.h>
#include <gsl/gsl_vector.h>
#endif

// #define GSL_OUTPUT 1

const double BEZIER_INTERSECT_PRECISION = 1.0;

#include "ipegeo.h"

using namespace ipe;

inline double sq(double x) { return x * x; }

// --------------------------------------------------------------------

extern "C" { // old version of the header does not have cplusplus wrapper
#include "bezctx.h"
#include "spiroentrypoints.h"
}

// --------------------------------------------------------------------

/*! \class ipe::Angle
  \ingroup geo
  \brief A double that's an angle.

  An Angle is really nothing more than a double.  Having a separate
  type is sometimes useful, for instance in the Vector constructor,
  and this class serves as the right place for a few utility
  functions.  It also makes it clear whether a value is in radians or
  in degrees.
*/

double Angle::degrees() const { return (iAlpha / IpePi * 180.0); }

//! Normalize the value to the range lowlimit .. lowlimit + 2 pi.
/*! This Angle object is modified, a copy is returned. */
Angle Angle::normalize(double lowlimit) {
    while (iAlpha >= lowlimit + IpeTwoPi) iAlpha -= IpeTwoPi;
    while (iAlpha < lowlimit) iAlpha += IpeTwoPi;
    return *this;
}

/*! When considering the positively oriented circle arc from angle \a
  small to \a large, does it cover this angle?
*/
bool Angle::liesBetween(Angle small, Angle large) const {
    large.normalize(iAlpha);
    small.normalize(large.iAlpha - IpeTwoPi);
    return (iAlpha >= small.iAlpha);
}

// --------------------------------------------------------------------

/*! \class ipe::Vector
  \ingroup geo
  \brief Two-dimensional vector.

  Unlike some other libraries, I don't make a difference between
  points and vectors.
*/

//! Construct a unit vector with this direction.
Vector::Vector(Angle alpha) {
    x = cos(alpha);
    y = sin(alpha);
}

//! Return angle of the vector (with positive x-direction).
/*! The returned angle lies between -pi and +pi.
  Returns zero for the zero vector. */
Angle Vector::angle() const {
    if (x == 0.0 && y == 0.0)
	return Angle(0.0);
    else
	return Angle(atan2(y, x));
}

//! The origin (zero vector).
Vector Vector::ZERO = Vector(0.0, 0.0);

double Vector::len() const { return sqrt(sqLen()); }

//! Return this vector normalized (with length one).
/*! Normalizing the zero vector returns the vector (1,0). */
Vector Vector::normalized() const {
    double len = sqLen();
    if (len == 1.0) return *this;
    if (len == 0.0) return Vector(1, 0);
    return (1.0 / sqrt(len)) * (*this);
}

//! Return this vector turned 90 degrees to the left.
Vector Vector::orthogonal() const { return Vector(-y, x); }

/*! Normalizes this vector into \a unit and returns length.
  If this is the zero vector, \a unit is set to (1,0). */
double Vector::factorize(Vector & unit) const {
    double len = sqLen();
    if (len == 0.0) {
	unit = Vector(1, 0);
	return len;
    }
    if (len == 1.0) {
	unit = *this;
	return len;
    }
    len = sqrt(len);
    unit = (1.0 / len) * (*this);
    return len;
}

//! Snap to nearby vertex.
/*! If distance between \a mouse and this vector is less than \a
  bound, set \a pos to this vector and \a bound to the distance, and
  return \c true. */
bool Vector::snap(const Vector & mouse, Vector & pos, double & bound) const {
    double d = (mouse - *this).len();
    if (d < bound) {
	pos = *this;
	bound = d;
	return true;
    }
    return false;
}

//! Output operator for Vector.
Stream & ipe::operator<<(Stream & stream, const Vector & rhs) {
    return stream << rhs.x << " " << rhs.y;
}

// --------------------------------------------------------------------

/*! \class ipe::Rect
  \ingroup geo
  \brief Axis-parallel rectangle (which can be empty)
*/

//! Create rectangle containing points \a c1 and \a c2.
Rect::Rect(const Vector & c1, const Vector & c2)
    : iMin(1, 0)
    , iMax(-1, 0) {
    addPoint(c1);
    addPoint(c2);
}

//! Does (closed) rectangle contain the point?
bool Rect::contains(const Vector & rhs) const {
    // this correctly handles empty this
    return (iMin.x <= rhs.x && rhs.x <= iMax.x && iMin.y <= rhs.y && rhs.y <= iMax.y);
}

//! Does rectangle contain other rectangle?
bool Rect::contains(const Rect & rhs) const {
    if (rhs.isEmpty()) return true;
    if (isEmpty()) return false;
    return (iMin.x <= rhs.iMin.x && rhs.iMax.x <= iMax.x && iMin.y <= rhs.iMin.y
	    && rhs.iMax.y <= iMax.y);
}

//! Does rectangle intersect other rectangle?
bool Rect::intersects(const Rect & rhs) const {
    if (isEmpty() || rhs.isEmpty()) return false;
    return (iMin.x <= rhs.iMax.x && rhs.iMin.x <= iMax.x && iMin.y <= rhs.iMax.y
	    && rhs.iMin.y <= iMax.y);
}

//! Enlarge rectangle to contain point.
void Rect::addPoint(const Vector & rhs) {
    if (isEmpty()) {
	iMin = rhs;
	iMax = rhs;
    } else {
	if (rhs.x > iMax.x)
	    iMax.x = rhs.x;
	else if (rhs.x < iMin.x)
	    iMin.x = rhs.x;
	if (rhs.y > iMax.y)
	    iMax.y = rhs.y;
	else if (rhs.y < iMin.y)
	    iMin.y = rhs.y;
    }
}

//! Enlarge rectangle to contain rhs rectangle.
/*! Does nothing if \a rhs is empty. */
void Rect::addRect(const Rect & rhs) {
    if (isEmpty()) {
	iMin = rhs.iMin;
	iMax = rhs.iMax;
    } else if (!rhs.isEmpty()) {
	if (rhs.iMax.x > iMax.x) iMax.x = rhs.iMax.x;
	if (rhs.iMin.x < iMin.x) iMin.x = rhs.iMin.x;
	if (rhs.iMax.y > iMax.y) iMax.y = rhs.iMax.y;
	if (rhs.iMin.y < iMin.y) iMin.y = rhs.iMin.y;
    }
}

//! Clip rectangle to fit inside \a cbox.
/*! Does nothing if either rectangle is empty. */
void Rect::clipTo(const Rect & cbox) {
    if (isEmpty() || cbox.isEmpty()) return;
    if (!intersects(cbox)) { // make box empty
	iMin = Vector(1, 0);
	iMax = Vector(-1, 0);
    } else {
	if (iMin.x < cbox.iMin.x) iMin.x = cbox.iMin.x;
	if (iMin.y < cbox.iMin.y) iMin.y = cbox.iMin.y;
	if (iMax.x > cbox.iMax.x) iMax.x = cbox.iMax.x;
	if (iMax.y > cbox.iMax.y) iMax.y = cbox.iMax.y;
    }
}

/*! Returns false if the distance between the box and v is smaller
  than \a bound.  Often returns true if their distance is larger than
  \a bound.
*/
bool Rect::certainClearance(const Vector & v, double bound) const {
    return ((iMin.x - v.x) >= bound || (v.x - iMax.x) >= bound || (iMin.y - v.y) >= bound
	    || (v.y - iMax.y) >= bound);
}

//! Output operator for Rect.
Stream & ipe::operator<<(Stream & stream, const Rect & rhs) {
    return stream << rhs.bottomLeft() << " " << rhs.topRight();
}

// --------------------------------------------------------------------

/*! \class ipe::Line
  \ingroup geo
  \brief A directed line.
*/

//! Construct a line from \a p with direction \a dir.
/*! Asserts unit length of \a dir. */
Line::Line(const Vector & p, const Vector & dir) {
    assert(sq(dir.sqLen() - 1.0) < 1e-10);
    iP = p;
    iDir = dir;
}

//! Construct a line through two points.
Line Line::through(const Vector & p, const Vector & q) {
    assert(q != p);
    return Line(p, (q - p).normalized());
}

//! Result is > 0, = 0, < 0 if point lies to the left, on, to the right.
double Line::side(const Vector & p) const { return dot(normal(), p - iP); }

//! Returns distance between line and \a v.
double Line::distance(const Vector & v) const {
    Vector diff = v - iP;
    return (diff - dot(diff, iDir) * iDir).len();
}

inline double cross(const Vector & v1, const Vector & v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

static bool line_intersection(double & lambda, const Line & l, const Line & m) {
    double denom = cross(m.dir(), l.dir());
    if (denom == 0.0) return false;
    lambda = cross(l.iP - m.iP, m.dir()) / denom;
    return true;
}

//! Does this line intersect \a line?  If so, computes intersection point.
bool Line::intersects(const Line & line, Vector & pt) {
    double lambda;
    if (line_intersection(lambda, *this, line)) {
	pt = iP + lambda * iDir;
	return true;
    }
    return false;
}

//! Orthogonally project point \a v onto the line.
Vector Line::project(const Vector & v) const {
    double dx = dot(iDir, v - iP);
    return iP + dx * iDir;
}

// --------------------------------------------------------------------

/*! \class ipe::Segment
  \ingroup geo
  \brief A directed line segment.
*/

/*! Returns distance between segment and point \a v,
  but may just return \a bound when its larger than \a bound. */
double Segment::distance(const Vector & v, double bound) const {
    if (Rect(iP, iQ).certainClearance(v, bound)) return bound;
    return distance(v);
}

/*! Returns distance between segment and point \a v */
double Segment::distance(const Vector & v) const {
    Vector dir = iQ - iP;
    Vector udir;
    double len = dir.factorize(udir);
    double dx = dot(udir, v - iP);
    if (dx <= 0) return (v - iP).len();
    if (dx >= len) return (v - iQ).len();
    return (v - (iP + dx * udir)).len();
}

/*! Project point \a v orthogonally on segment.
  Returns false if the point falls outside the segment. */
bool Segment::project(const Vector & v, Vector & projection) const {
    Vector dir = iQ - iP;
    Vector udir;
    double len = dir.factorize(udir);
    double dx = dot(udir, v - iP);
    if (dx <= 0 || dx >= len) return false;
    projection = iP + dx * udir;
    return true;
}

//! Compute intersection point. Return \c false if segs don't intersect.
bool Segment::intersects(const Segment & seg, Vector & pt) const {
    if (iP == iQ || seg.iP == seg.iQ) return false;
    if (!Rect(iP, iQ).intersects(Rect(seg.iP, seg.iQ))) return false;
    if (!line().intersects(seg.line(), pt)) return false;
    // have intersection point, check whether it's on both segments.
    Vector dir = iQ - iP;
    Vector dir1 = seg.iQ - seg.iP;
    return (dot(pt - iP, dir) >= 0 && dot(pt - iQ, dir) <= 0
	    && dot(pt - seg.iP, dir1) >= 0 && dot(pt - seg.iQ, dir1) <= 0);
}

//! Compute intersection point. Return \c false if no intersection.
bool Segment::intersects(const Line & l, Vector & pt) const {
    if (!line().intersects(l, pt)) return false;
    // have intersection point, check whether it's on the segment
    Vector dir = iQ - iP;
    return (dot(pt - iP, dir) >= 0 && dot(pt - iQ, dir) <= 0);
}

//! Snap mouse position to this segment.
/*! If distance between \a mouse and the segment is less than
  \a bound, then set \a pos to the point on the segment,
  \a bound to the distance, and return true. */
bool Segment::snap(const Vector & mouse, Vector & pos, double & bound) const {
    if (Rect(iP, iQ).certainClearance(mouse, bound)) return false;
    Vector v;
    if (project(mouse, v)) {
	double d = (mouse - v).len();
	if (d < bound) {
	    pos = v;
	    bound = d;
	    return true;
	}
	return false;
    } else
	return iQ.snap(mouse, pos, bound);
}

// --------------------------------------------------------------------

/*! \class ipe::Linear
  \ingroup geo
  \brief Linear transformation in the plane (2x2 matrix).
*/

//! Create matrix representing a rotation by angle.
Linear::Linear(Angle angle) {
    a[0] = cos(angle);
    a[1] = sin(angle);
    a[2] = -a[1];
    a[3] = a[0];
}

//! Parse string.
Linear::Linear(String str) {
    Lex lex(str);
    lex >> a[0] >> a[1] >> a[2] >> a[3];
}

//! Return inverse.
Linear Linear::inverse() const {
    double t = determinant();
    assert(t != 0);
    t = 1.0 / t;
    return Linear(a[3] * t, -a[1] * t, -a[2] * t, a[0] * t);
}

//! Output operator for Linear.
Stream & ipe::operator<<(Stream & stream, const Linear & rhs) {
    return stream << rhs.a[0] << " " << rhs.a[1] << " " << rhs.a[2] << " " << rhs.a[3];
}

// --------------------------------------------------------------------

/*! \class ipe::Matrix
  \ingroup geo
  \brief Homogeneous transformation in the plane.
*/

//! Parse string.
Matrix::Matrix(String str) {
    Lex lex(str);
    lex >> a[0] >> a[1] >> a[2] >> a[3] >> a[4] >> a[5];
}

//! Return inverse.
Matrix Matrix::inverse() const {
    double t = determinant();
    assert(t != 0);
    t = 1.0 / t;
    return Matrix(a[3] * t, -a[1] * t, -a[2] * t, a[0] * t,
		  (a[2] * a[5] - a[3] * a[4]) * t, -(a[0] * a[5] - a[1] * a[4]) * t);
}

//! Output operator for Matrix.
Stream & ipe::operator<<(Stream & stream, const Matrix & rhs) {
    return stream << rhs.a[0] << " " << rhs.a[1] << " " << rhs.a[2] << " " << rhs.a[3]
		  << " " << rhs.a[4] << " " << rhs.a[5];
}

// --------------------------------------------------------------------

/*! \class ipe::Bezier
  \ingroup geo
  \brief A cubic Bezier spline.

*/

inline Vector midpoint(const Vector & p, const Vector & q) { return 0.5 * (p + q); }

inline Vector thirdpoint(const Vector & p, const Vector & q) {
    return (1.0 / 3.0) * ((2 * p) + q);
}

//! Return point on curve with parameter \a t (from 0.0 to 1.0).
Vector Bezier::point(double t) const {
    double t1 = 1.0 - t;
    return t1 * t1 * t1 * iV[0] + 3 * t * t1 * t1 * iV[1] + 3 * t * t * t1 * iV[2]
	   + t * t * t * iV[3];
}

//! Return tangent direction of curve at parameter \a t (from 0.0 to 1.0).
/*! The returned vector is not normalized. */
Vector Bezier::tangent(double t) const {
    double tt = 1.0 - t;
    Vector p = tt * iV[0] + t * iV[1];
    Vector q = tt * iV[1] + t * iV[2];
    Vector r = tt * iV[2] + t * iV[3];
    p = tt * p + t * q;
    q = tt * q + t * r;
    r = tt * p + t * q;
    return r - p;
}

/*! Returns true if the Bezier curve is nearly identical to the line
  segment iV[0]..iV[3]. */
bool Bezier::straight(double precision) const {
    if (iV[0] == iV[3]) {
	return ((iV[1] - iV[0]).len() < precision && (iV[2] - iV[0]).len() < precision);
    } else {
	Line l = Line::through(iV[0], iV[3]);
	double d1 = l.distance(iV[1]);
	double d2 = l.distance(iV[2]);
	return (d1 < precision && d2 < precision);
    }
}

//! Subdivide this Bezier curve in the middle.
void Bezier::subdivide(Bezier & l, Bezier & r) const {
    Vector h;
    l.iV[0] = iV[0];
    l.iV[1] = 0.5 * (iV[0] + iV[1]);
    h = 0.5 * (iV[1] + iV[2]);
    l.iV[2] = 0.5 * (l.iV[1] + h);
    r.iV[2] = 0.5 * (iV[2] + iV[3]);
    r.iV[1] = 0.5 * (h + r.iV[2]);
    r.iV[0] = 0.5 * (l.iV[2] + r.iV[1]);
    l.iV[3] = r.iV[0];
    r.iV[3] = iV[3];
}

//! Approximate by a polygonal chain.
/*! \a result must be empty when calling this. */
void Bezier::approximate(double precision, std::vector<Vector> & result) const {
    if (straight(precision)) {
	result.push_back(iV[3]);
    } else {
	Bezier l, r;
	subdivide(l, r);
	l.approximate(precision, result);
	r.approximate(precision, result);
    }
}

//! Convert a quadratic Bezier-spline to a cubic one.
/*! The quadratic Bezier-spline with control points p0, p1, p2
  is identical to the cubic Bezier-spline with control points
  q0 = p0, q1 = (2p1 + p0)/3, q2 = (2p1 + p2)/3, q3 = p2. */
Bezier Bezier::quadBezier(const Vector & p0, const Vector & p1, const Vector & p2) {
    Vector q1 = thirdpoint(p1, p0);
    Vector q2 = thirdpoint(p1, p2);
    return Bezier(p0, q1, q2, p2);
}

//! Convert an old-style Ipe B-spline to a series of Bezier splines.
/*! For some reason lost in the mist of time, this was the definition
  of splines in Ipe for many years.  It doesn't use knots.  The first
  and last control point are simply given multiplicity 3.

  Bezier splines are appended to \a result.
*/
void Bezier::oldSpline(int n, const Vector * v, std::vector<Bezier> & result) {
    Vector p0, p1, p2, p3, q0, q1, q2, q3;
    // First segment (p1 = p2 = p0 => q1 = q2 = q0 = p0)
    p0 = v[0];
    p3 = v[1];
    q3 = midpoint(thirdpoint(p0, p3), p0);
    result.push_back(Bezier(p0, p0, p0, q3));
    if (n > 2) {
	// Second segment
	p1 = v[0];
	p2 = v[1];
	p3 = v[2];
	q0 = q3; // from previous
	q1 = thirdpoint(p1, p2);
	q2 = thirdpoint(p2, p1);
	q3 = midpoint(thirdpoint(p2, p3), q2);
	result.push_back(Bezier(q0, q1, q2, q3));
	// create n - 3 segments
	for (int i = 0; i < n - 3; ++i) {
	    p0 = v[i];
	    p1 = v[i + 1];
	    p2 = v[i + 2];
	    p3 = v[i + 3];
	    q0 = q3; // from previous
	    // q0 = midpoint(thirdpoint(p1, p0), q1); // the real formula
	    q1 = thirdpoint(p1, p2);
	    q2 = thirdpoint(p2, p1);
	    q3 = midpoint(thirdpoint(p2, p3), q2);
	    result.push_back(Bezier(q0, q1, q2, q3));
	}
    }
    // Second to last segment
    p1 = v[n - 2];
    p2 = v[n - 1];
    p3 = v[n - 1];
    q0 = q3; // from previous
    q1 = thirdpoint(p1, p2);
    q2 = thirdpoint(p2, p1);
    q3 = midpoint(p3, q2);
    result.push_back(Bezier(q0, q1, q2, q3));
    // Last segment (p1 = p2 = p3 => q1 = q2 = q3 = p3)
    result.push_back(Bezier(q3, p3, p3, p3));
}

//! Convert a clamped uniform B-spline to a series of Bezier splines.
/*! See Thomas Sederberg, Computer-Aided Geometric Design, Chapter 6.

  In polar coordinates, a control point is defined by three knots, so
  n control points need n + 2 knots.  To clamp the spline to the first
  and last control point, the first and last knot are repeated three
  times. This leads to k knot intervals and the knot sequence
  [0, 0, 0, 1, 2, 3, 4, 5, 6, ..., k-2, k-1, k, k, k]
  There are k + 5 = n + 2 knots in this sequence, so k = n-3 is the
  number of knot intervals and therefore the number of output Bezier
  curves.

  If n = 4, the knot sequence is [0, 0, 0, 1, 1, 1] and the result is
  simply the Bezier curve on the four control points.

  When n in {2, 3}, returns a single Bezier curve that is a segment or
  quadratic Bezier spline.  This is different from the behaviour of
  the "old" Ipe splines.

  Bezier splines are appended to \a result.
*/
void Bezier::spline(int n, const Vector * v, std::vector<Bezier> & result) {
    if (n == 2) {
	result.push_back(Bezier(v[0], v[0], v[1], v[1]));
    } else if (n == 3) {
	result.push_back(quadBezier(v[0], v[1], v[2]));
    } else if (n == 4) {
	result.push_back(Bezier(v[0], v[1], v[2], v[3]));
    } else if (n == 5) {
	// Given are [0,0,0], [0,0,1], [0,1,2], [1,2,2], [2,2,2]
	// Interval 0-1: [0,0,0], [0,0,1], [0,1,1], [1,1,1]
	Vector q0 = v[0];                // [0,0,0]
	Vector q1 = v[1];                // [0,0,1]
	Vector q2 = midpoint(q1, v[2]);  // [0,1,1] = 1/2 [0,1,0] + 1/2 [0,1,2]
	Vector r = midpoint(v[2], v[3]); // [1,1,2] = 1/3 [0,1,2] + 1/2 [2,1,2]
	Vector q3 = midpoint(q2, r);     // [1,1,1] = 1/2 [1,0,1] + 1/2 [1,2,1]
	result.push_back(Bezier(q0, q1, q2, q3));
	// Interval 1-2: [1,1,1], [1,1,2], [1,2,2], [2,2,2]
	result.push_back(Bezier(q3, r, v[3], v[4]));
    } else {
	int k = n - 3;
	// Interval 0-1: [0,0,0], [0,0,1], [0,1,1], [1,1,1]
	Vector q0 = v[0];                  // [0,0,0]
	Vector q1 = v[1];                  // [0,0,1]
	Vector q2 = midpoint(q1, v[2]);    // [0,1,1] = 1/2 [0,1,0] + 1/2 [0,1,2]
	Vector r = thirdpoint(v[2], v[3]); // [1,2,1] = 2/3 [0,1,2] + 1/3 [3,1,2]
	Vector q3 = midpoint(q2, r);       // [1,1,1] = 1/2 [1,0,1] + 1/2 [1,2,1]
	result.push_back(Bezier(q0, q1, q2, q3));
	for (int i = 1; i < k - 2; ++i) {
	    // Interval i-i+1: [i,i,i], [i,i,i+1], [i,i+1,i+1], [i+1,i+1,i+1]
	    q0 = q3; // [i,i,i]
	    q1 = r;  // [i,i,i+1]
	    // [i,i+1,i+1] = 1/2 [i,i+1,i] + 1/2 [i, i+1, i+2]
	    q2 = midpoint(q1, v[i + 2]);
	    // [i+1,i+1,i+2] = 2/3 [i,i+1,i+2] + [i+3,i+1,i+2]
	    r = thirdpoint(v[i + 2], v[i + 3]);
	    // [i+1,i+1,i+1] = 1/2 [i+1,i+1,i] + 1/2 [i+1,i+1,i+2]
	    q3 = midpoint(q2, r);
	    result.push_back(Bezier(q0, q1, q2, q3));
	}
	// Interval (k-2)-(k-1):
	// [k-2,k-2,k-2], [k-2,k-2,k-1], [k-2,k-1,k-1], [k-1,k-1,k-1]
	q0 = q3;
	q1 = r; // [k-2,k-2,k-1]
	// [k-2,k-1,k-1] = 1/2 [k-2,k-1,k-2] + 1/2 [k-2,k-1,k]
	q2 = midpoint(q1, v[k]);
	// [k-1,k-1,k] =  1/2 [k-2,k-1,k] + 1/2 [k,k-1,k]
	r = midpoint(v[k], v[k + 1]);
	// [k-1,k-1,k-1] = 1/2 [k-1,k-1,k-2] + 1/2 [k-1,k-1,k]
	q3 = midpoint(q2, r);
	result.push_back(Bezier(q0, q1, q2, q3));
	// Interval (k-1)-k: [k-1,k-1,k-1], [k-1,k-1,k], [k-1,k,k], [k,k,k]
	q0 = q3;
	q1 = r;
	q2 = v[n - 2]; // [k-1,k,k]
	q3 = v[n - 1]; // [k,k,k]
	result.push_back(Bezier(q0, q1, q2, q3));
    }
}

//! Convert a closed uniform cubic B-spline to a series of Bezier splines.
/*! Bezier splines are appended to \a result. */
void Bezier::closedSpline(int n, const Vector * v, std::vector<Bezier> & result) {
    for (int i = 0; i < n; ++i) {
	Vector p0 = v[i % n];           // [0, 1, 2]
	Vector p1 = v[(i + 1) % n];     // [1, 2, 3]
	Vector p2 = v[(i + 2) % n];     // [2, 3, 4]
	Vector p3 = v[(i + 3) % n];     // [3, 4, 5]
	Vector r = thirdpoint(p1, p0);  // [1, 2, 2]
	Vector u = thirdpoint(p2, p3);  // [3, 3, 4]
	Vector q1 = thirdpoint(p1, p2); // [2, 2, 3]
	Vector q2 = thirdpoint(p2, p1); // [2, 3, 3]
	Vector q0 = midpoint(r, q1);    // [2, 2, 2]
	Vector q3 = midpoint(u, q2);    // [3, 3, 3]
	result.push_back(Bezier(q0, q1, q2, q3));
    }
}

static Bezier cardinal(Vector prev, Vector p, Vector q, Vector next, double tension) {
    Vector t1 = q - prev; // tangent in p
    Vector t2 = next - p; // tangent in q
    Vector cp1 = p + (tension / 3.0) * t1;
    Vector cp2 = q - (tension / 3.0) * t2;
    return Bezier(p, cp1, cp2, q);
}

//! Convert a cardinal spline to a series of Bezier splines.
/*! Bezier splines are appended to \a result.
 */
void Bezier::cardinalSpline(int n, const Vector * v, double tension,
			    std::vector<Bezier> & result) {
    if (n == 2) {
	result.push_back(Bezier(v[0], v[0], v[1], v[1]));
    } else {
	result.push_back(cardinal(v[0], v[0], v[1], v[2], tension));
	for (int i = 1; i < n - 2; ++i)
	    result.push_back(cardinal(v[i - 1], v[i], v[i + 1], v[i + 2], tension));
	result.push_back(cardinal(v[n - 3], v[n - 2], v[n - 1], v[n - 1], tension));
    }
}

// ------------------------------------------------------------------------------------------
// These functions are called by spiro to generate Bezier output

struct BezierContext {
    struct _bezctx base;
    Vector pos;
    std::vector<Bezier> & bez;

    BezierContext(const Vector & pos, std::vector<Bezier> & bez);
};

extern "C" void bezctx_ipe_moveto(bezctx * bc0, double x, double y, int is_open) {
    // ipeDebug("bezctx_moveto(%g,%g)_%d",x,y,is_open);
    BezierContext * bc = (BezierContext *)bc0;
    bc->pos = Vector(x, y);
}

extern "C" void bezctx_ipe_lineto(bezctx * bc0, double x, double y) {
    // ipeDebug("bezctx_lineto(%g,%g)",x,y);
    BezierContext * bc = (BezierContext *)bc0;
    Vector pos{x, y};
    bc->bez.push_back(Bezier(bc->pos, bc->pos, pos, pos));
    bc->pos = pos;
}

// most likely this will not be called with our settings
extern "C" void bezctx_ipe_quadto(bezctx * bc0, double x1, double y1, double x2,
				  double y2) {
    // ipeDebug("bezctx_quadto(%g,%g, %g,%g)",x1,y1,x2,y2);
    BezierContext * bc = (BezierContext *)bc0;
    Vector cp1{x1, y1}, cp2{x2, y2};
    bc->bez.push_back(Bezier::quadBezier(bc->pos, cp1, cp2));
    bc->pos = cp2;
}

extern "C" void bezctx_ipe_curveto(bezctx * bc0, double x1, double y1, double x2,
				   double y2, double x3, double y3) {
    // ipeDebug("bezctx_curveto(%g,%g, %g,%g, %g,%g)",x1,y1,x2,y2,x3,y3);
    BezierContext * bc = (BezierContext *)bc0;
    Vector cp1{x1, y1}, cp2{x2, y2}, cp3{x3, y3};
    bc->bez.push_back(Bezier(bc->pos, cp1, cp2, cp3));
    bc->pos = cp3;
}

// apparently this is called to indicate when the curve reaches each knot
extern "C" void bezctx_ipe_mark_knot(bezctx * bc, int knot_idx) {
    // ipeDebug("bezctx_mark_knot: %d",knot_idx);
}

BezierContext::BezierContext(const Vector & pos, std::vector<Bezier> & bez)
    : pos{pos}
    , bez{bez} {
    base.moveto = bezctx_ipe_moveto;
    base.lineto = bezctx_ipe_lineto;
    base.quadto = bezctx_ipe_quadto;
    base.curveto = bezctx_ipe_curveto;
    base.mark_knot = bezctx_ipe_mark_knot;
}

// ------------------------------------------------------------------------------------------

void Bezier::spiroSpline(int n, const Vector * v, std::vector<Bezier> & result) {
    std::vector<spiro_cp> spiroCp;
    for (int i = 0; i < n; ++i) { spiroCp.push_back({v[i].x, v[i].y, SPIRO_G2}); }
    if (n > 2 && v[n - 2] == v[n - 1]) spiroCp.pop_back();
    if (spiroCp.size() < 3) {
	result.push_back(Bezier(v[0], v[0], v[1], v[1]));
	return;
    }

    BezierContext context{v[0], result};
    if (
#ifdef SPIRO_CUBIC_TO_BEZIER
	// newer libspiro1 versions >= 20190731
	SpiroCPsToBezier2(spiroCp.data(), spiroCp.size(), SPIRO_CUBIC_TO_BEZIER, false,
			  (bezctx *)&context)
#else
	// older libspiro0 versions <= 20150131
	// curve calculations affected by cp x,y points relative to [0,0]
	// moving the spiroSpline might appear different if moved around.
	SpiroCPsToBezier0(spiroCp.data(), spiroCp.size(), false, (bezctx *)&context)
#endif
	!= 1) {
	// spiro failed to resolve, use another spline instead
	Bezier::spline(n, v, result);
    }
}

//! Return distance to Bezier spline.
/*! But may just return \a bound if actual distance is larger.  The
  Bezier spline is approximated to a precision of 1.0, and the
  distance to the approximation is returned.
 */
double Bezier::distance(const Vector & v, double bound) {
    Rect box;
    box.addPoint(iV[0]);
    box.addPoint(iV[1]);
    box.addPoint(iV[2]);
    box.addPoint(iV[3]);
    if (box.certainClearance(v, bound)) return bound;
    std::vector<Vector> approx;
    approximate(1.0, approx);
    Vector cur = iV[0];
    double d = bound;
    double d1;
    for (std::vector<Vector>::const_iterator it = approx.begin(); it != approx.end();
	 ++it) {
	if ((d1 = Segment(cur, *it).distance(v, d)) < d) d = d1;
	cur = *it;
    }
    return d;
}

//! Return a tight bounding box (accurate to within 0.5).
Rect Bezier::bbox() const {
    Rect box(iV[0]);
    std::vector<Vector> approx;
    approximate(0.5, approx);
    for (std::vector<Vector>::const_iterator it = approx.begin(); it != approx.end();
	 ++it) {
	box.addPoint(*it);
    }
    return Rect(box.bottomLeft() - Vector(0.5, 0.5), box.topRight() + Vector(0.5, 0.5));
}

Vector Arc::midpoint() const {
    if ((iBeta - iAlpha > IpeTwoPi - 1e-10)
	|| (iAlpha - 1e-10 < iBeta && iBeta < iAlpha)) {
	// Arc approximates a full ellipse
	return iM * Vector(iAlpha + IpePi);
    }

    double delta = Angle(iBeta - iAlpha).normalize(0.0);

    if (sq(delta) < 1e-20) {
	// Arc approximately zero-length, return the starting point
	return iM * Vector(iAlpha);
    }

    return iM * Vector(iAlpha + delta / 2);
}

//! Find (approximately) nearest point on Bezier spline.
/*! Find point on spline nearest to \a v, but only if
  it is closer than \a bound.
  If a point is found, sets \a t to the parameter value and
  \a pos to the actual point, and returns true. */
bool Bezier::snap(const Vector & v, double & t, Vector & pos, double & bound) const {
    Rect box(iV[0], iV[1]);
    box.addPoint(iV[2]);
    box.addPoint(iV[3]);
    if (box.certainClearance(v, bound)) return false;

    // handle straight ends of B-splines
    if (iV[0] != iV[1] && iV[1] == iV[2] && iV[2] == iV[3]) {
	Vector prj;
	double d;
	if (Segment(iV[0], iV[3]).project(v, prj) && (d = (v - prj).len()) < bound) {
	    bound = d;
	    pos = prj;
	    t = 1.0 - pow((pos - iV[3]).len() / (iV[0] - iV[3]).len(), 1.0 / 3.0);
	    return true;
	} // endpoints handled by code below
    }
    if (iV[0] == iV[1] && iV[1] == iV[2] && iV[2] != iV[3]) {
	Vector prj;
	double d;
	if (Segment(iV[3], iV[0]).project(v, prj) && (d = (v - prj).len()) < bound) {
	    bound = d;
	    pos = prj;
	    t = 1.0 - pow((pos - iV[0]).len() / (iV[3] - iV[0]).len(), 1.0 / 3.0);
	    return true;
	} // endpoints handled by code below
    }

    if (straight(1.0)) {
	Vector prj;
	if (iV[0] != iV[3] && Segment(iV[0], iV[3]).project(v, prj)) {
	    double t1 = (prj - iV[0]).len() / (iV[3] - iV[0]).len();
	    Vector u = point(t1);
	    double d = (v - u).len();
	    if (d < bound) {
		t = t1;
		bound = d;
		pos = u;
		return true;
	    } else
		return false;
	} else {
	    bool v0 = iV[0].snap(v, pos, bound);
	    bool v1 = iV[3].snap(v, pos, bound);
	    if (v0) t = 0.0;
	    if (v1) t = 1.0;
	    return v0 || v1;
	}
    } else {
	Bezier l, r;
	subdivide(l, r);
	bool p1 = l.snap(v, t, pos, bound);
	bool p2 = r.snap(v, t, pos, bound);
	if (p1 || p2) t = 0.5 * t;
	if (p2) t = t + 0.5;
	return p1 || p2;
    }
}

// --------------------------------------------------------------------

#ifdef IPE_GSL

// Use Gnu Scientific Library to compute more precise intersection points
// https://www.gnu.org/software/gsl/doc/html/multimin.html

// Distance between points on two Bezier splines
static double bezierDistance(const gsl_vector * v, void * params) {
    Bezier * p = (Bezier *)params;
    double t1 = gsl_vector_get(v, 0);
    double t2 = gsl_vector_get(v, 1);
    return (p[0].point(t1) - p[1].point(t2)).sqLen();
}

/* This uses a minimizer that doesn't need gradients.
   Our function is polynomial, so one could work out the derivatives -
   but this already works very well... */
static Vector nearbyBezierIntersection(const Vector & q, const Bezier & a,
				       const Bezier & b) {
    Bezier par[2] = {a, b};

    gsl_multimin_function minFunc;
    minFunc.n = 2;
    minFunc.f = bezierDistance;
    minFunc.params = par;

    // Starting point x
    gsl_vector * x = gsl_vector_alloc(2);
    gsl_vector_set(x, 0, (q - a.iV[0]).len() / (a.iV[3] - a.iV[0]).len());
    gsl_vector_set(x, 1, (q - b.iV[0]).len() / (b.iV[3] - b.iV[0]).len());

    // Initial step size
    gsl_vector * ss = gsl_vector_alloc(2);
    gsl_vector_set_all(ss, 0.1);

    const gsl_multimin_fminimizer_type * T = gsl_multimin_fminimizer_nmsimplex;
    gsl_multimin_fminimizer * s = gsl_multimin_fminimizer_alloc(T, 2);
    int status = gsl_multimin_fminimizer_set(s, &minFunc, x, ss);

    if (status == GSL_SUCCESS) {
	size_t iter = 0;
	do {
	    iter++;
	    status = gsl_multimin_fminimizer_iterate(s);
	    if (status) break;
	    double size = gsl_multimin_fminimizer_size(s);
	    status = gsl_multimin_test_size(size, 1e-4);
#ifdef GSL_OUTPUT
	    if (status == GSL_SUCCESS) ipeDebug("Minimum obtained");
	    ipeDebug("minimize: %3ld t1=%.9f t2=%.9f sqDist=%.3e size=%.3e", iter,
		     gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1), s->fval, size);
#endif
	} while (status == GSL_CONTINUE && iter < 100);
    }
    Vector result = (status == GSL_SUCCESS) ? a.point(gsl_vector_get(s->x, 0)) : q;
    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free(s);
    return result;
}

struct BezierLineParams {
    Bezier b;
    Line l;
};

// distance of a point on the Bezier spline from the line
static double bezierLineDistance(double x, void * params) {
    BezierLineParams * p = (BezierLineParams *)params;
    return sq(p->l.distance(p->b.point(x)));
}

static Vector nearbyBezierLineIntersection(const Vector & q, const Bezier & a,
					   const Line & l) {
    BezierLineParams params{a, l};

    gsl_function minFunc;
    minFunc.function = &bezierLineDistance;
    minFunc.params = &params;

    const gsl_min_fminimizer_type * T = gsl_min_fminimizer_brent;
    gsl_min_fminimizer * s = gsl_min_fminimizer_alloc(T);

    double t = (q - a.iV[0]).len() / (a.iV[3] - a.iV[0]).len();
    int status = gsl_min_fminimizer_set(s, &minFunc, t, 0.0, 1.0);
    if (status == GSL_SUCCESS) {
	size_t iter = 0;
	do {
	    iter++;
	    status = gsl_min_fminimizer_iterate(s);
	    if (status) break;
	    double x0 = gsl_min_fminimizer_x_lower(s);
	    double x1 = gsl_min_fminimizer_x_upper(s);
	    status = gsl_min_test_interval(x0, x1, 1e-4, 0.0);
#ifdef GSL_OUTPUT
	    if (status == GSL_SUCCESS) ipeDebug("Minimum obtained");
	    ipeDebug("minimize: %3ld [%.7f, %.7f] %.7f", iter, x0, x1,
		     gsl_min_fminimizer_x_minimum(s));
#endif
	} while (status == GSL_CONTINUE && iter < 100);
    }

    Vector result =
	(status == GSL_SUCCESS) ? a.point(gsl_min_fminimizer_x_minimum(s)) : q;
    gsl_min_fminimizer_free(s);
    return result;
}

struct BezierEllipseParams {
    Bezier b;
    Matrix m;
};

// distance of a point on the Bezier spline from an ellipse (in ellipse space)
static double bezierEllipseDistance(double x, void * params) {
    BezierEllipseParams * p = (BezierEllipseParams *)params;
    return sq((p->m * p->b.point(x)).len() - 1.0);
}

static Vector nearbyBezierEllipseIntersection(const Vector & q, const Bezier & a,
					      const Matrix & m) {
    BezierEllipseParams params{a, m.inverse()};

    gsl_function minFunc;
    minFunc.function = &bezierEllipseDistance;
    minFunc.params = &params;

    const gsl_min_fminimizer_type * T = gsl_min_fminimizer_brent;
    gsl_min_fminimizer * s = gsl_min_fminimizer_alloc(T);

    double t = (q - a.iV[0]).len() / (a.iV[3] - a.iV[0]).len();
    int status = gsl_min_fminimizer_set(s, &minFunc, t, 0.0, 1.0);
    if (status == GSL_SUCCESS) {
	size_t iter = 0;
	do {
	    iter++;
	    ipeDebug("About it iterate");
	    status = gsl_min_fminimizer_iterate(s);
	    if (status) break;
	    double x0 = gsl_min_fminimizer_x_lower(s);
	    double x1 = gsl_min_fminimizer_x_upper(s);
	    status = gsl_min_test_interval(x0, x1, 1e-4, 0.0);
#ifdef GSL_OUTPUT
	    if (status == GSL_SUCCESS) ipeDebug("Minimum obtained");
	    ipeDebug("minimize: %3ld [%.7f, %.7f] %.7f", iter, x0, x1,
		     gsl_min_fminimizer_x_minimum(s));
#endif
	} while (status == GSL_CONTINUE && iter < 100);
    }

    Vector result =
	(status == GSL_SUCCESS) ? a.point(gsl_min_fminimizer_x_minimum(s)) : q;
    gsl_min_fminimizer_free(s);
    return result;
}
#endif

// --------------------------------------------------------------------

/* Determines intersection point(s) of two cubic Bezier-Splines.
 * The found intersection points are stored in the vector intersections.
 */

static void intersectBeziers(std::vector<Vector> & intersections, const Bezier & a,
			     const Bezier & b) {
    /* Recursive approximation procedure to find intersections:
     * If the bounding boxes of two Beziers overlap, both are subdivided,
     * each one into two partial Beziers.
     * In the next recursion steps, it is checked if the bounding boxes
     * of the partial Beziers overlap. If they do, they are subdivided
     * again and so on, until a special precision is achieved:
     * Then the Beziers are converted to Segments and checked for intersection.
     */
    Rect abox(a.iV[0], a.iV[1]);
    abox.addPoint(a.iV[2]);
    abox.addPoint(a.iV[3]);
    Rect bbox(b.iV[0], b.iV[1]);
    bbox.addPoint(b.iV[2]);
    bbox.addPoint(b.iV[3]);

    if (!abox.intersects(bbox)) return;

    if (a.straight(BEZIER_INTERSECT_PRECISION)
	&& b.straight(BEZIER_INTERSECT_PRECISION)) {
	Segment as = Segment(a.iV[0], a.iV[3]);
	Segment bs = Segment(b.iV[0], b.iV[3]);
	Vector p;
	if (as.intersects(bs, p))
#ifdef IPE_GSL
	    intersections.push_back(nearbyBezierIntersection(p, a, b));
#else
	    intersections.push_back(p);
#endif
    } else {
	Bezier leftA, rightA, leftB, rightB;
	a.subdivide(leftA, rightA);
	b.subdivide(leftB, rightB);

	intersectBeziers(intersections, leftA, leftB);
	intersectBeziers(intersections, rightA, leftB);
	intersectBeziers(intersections, leftA, rightB);
	intersectBeziers(intersections, rightA, rightB);
    }
}

//! Compute intersection points of Bezier with Line.
void Bezier::intersect(const Line & l, std::vector<Vector> & result) const {
    double sgn = l.side(iV[0]);
    if (sgn < 0 && l.side(iV[1]) < 0 && l.side(iV[2]) < 0 && l.side(iV[3]) < 0) return;
    if (sgn > 0 && l.side(iV[1]) > 0 && l.side(iV[2]) > 0 && l.side(iV[3]) > 0) return;

    if (straight(BEZIER_INTERSECT_PRECISION)) {
	Vector p;
	if (Segment(iV[0], iV[3]).intersects(l, p)) {
#ifdef IPE_GSL
	    result.push_back(nearbyBezierLineIntersection(p, *this, l));
#else
	    result.push_back(p);
#endif
	}
    } else {
	Bezier leftA, rightA;
	subdivide(leftA, rightA);
	leftA.intersect(l, result);
	rightA.intersect(l, result);
    }
}

//! Compute intersection points of Bezier with Segment.
void Bezier::intersect(const Segment & s, std::vector<Vector> & result) const {
    // convert Segment to Bezier and use Bezier-Bezier-intersection
    // this works well since the segment is immediately "straight"
    intersectBeziers(result, *this, Bezier(s.iQ, s.iQ, s.iP, s.iP));
}

//! Compute intersection points of Bezier with Bezier.
void Bezier::intersect(const Bezier & b, std::vector<Vector> & result) const {
    intersectBeziers(result, *this, b);
}

// --------------------------------------------------------------------

/*! \class ipe::Arc
  \ingroup geo
  \brief An arc of an ellipse.

  The ellipse is represented using the matrix that transforms the unit
  circle x^2 + y^2 = 1 to the desired ellipse.  The arc coordinate
  system is the coordinate system of this unit circle.

  A full ellipse is described by iAlpha = 0, iBeta = IpeTwoPi.

  An elliptic arc is the image of the circular arc from iAlpha to
  iBeta (in increasing angle in arc coordinate system).
*/

//! Construct arc for ellipse defined by m, from begp to endp.
/*! This assumes that \a m has been correctly computed such that \a
  begb and \a endp already lie on the ellipse. */
Arc::Arc(const Matrix & m, const Vector & begp, const Vector & endp) {
    iM = m;
    Matrix inv = m.inverse();
    iAlpha = (inv * begp).angle();
    iBeta = (inv * endp).angle();
}

//! This doesn't really compute the distance, but a reasonable approximation.
double Arc::distance(const Vector & v, double bound) const {
    Vector pos;
    Angle angle;
    return distance(v, bound, pos, angle);
}

/*! Like distance(), but sets pos to point on arc and angle to its angle
  in arc coordinates.
  \a angle and \a pos are not modified if result is larger than bound.
*/
double Arc::distance(const Vector & v, double bound, Vector & pos, Angle & angle) const {
    Matrix inv1 = iM.inverse();
    Vector v1 = inv1 * v;
    Vector pos1 = iM * v1.normalized();
    double d = (v - pos1).len();

    if (isEllipse()) {
	if (d < bound) {
	    bound = d;
	    pos = pos1;
	    angle = v1.angle();
	}
    } else {
	// elliptic arc
	if (d < bound && v1.angle().liesBetween(iAlpha, iBeta)) {
	    bound = d;
	    pos = pos1;
	    angle = v1.angle();
	}
	pos1 = iM * Vector(iAlpha);
	d = (v - pos1).len();
	if (d < bound) {
	    bound = d;
	    pos = pos1;
	    angle = iAlpha;
	}
	pos1 = iM * Vector(iBeta);
	d = (v - pos1).len();
	if (d < bound) {
	    bound = d;
	    pos = pos1;
	    angle = iBeta;
	}
    }

    return bound;
}

//! Return a tight bounding box.
Rect Arc::bbox() const {
    Rect box;

    // add begin and end point
    box.addPoint(iM * Vector(iAlpha));
    box.addPoint(iM * Vector(iBeta));

    Linear inv = iM.linear().inverse();
    bool ell = isEllipse();

    Angle alpha = (inv * Vector(0, 1)).angle() - IpeHalfPi;
    if (ell || alpha.liesBetween(iAlpha, iBeta)) box.addPoint(iM * Vector(alpha));
    alpha = (inv * Vector(0, -1)).angle() - IpeHalfPi;
    if (ell || alpha.liesBetween(iAlpha, iBeta)) box.addPoint(iM * Vector(alpha));
    alpha = (inv * Vector(1, 0)).angle() - IpeHalfPi;
    if (ell || alpha.liesBetween(iAlpha, iBeta)) box.addPoint(iM * Vector(alpha));
    alpha = (inv * Vector(-1, 0)).angle() - IpeHalfPi;
    if (ell || alpha.liesBetween(iAlpha, iBeta)) box.addPoint(iM * Vector(alpha));
    return box;
}

// --------------------------------------------------------------------

//! Compute intersection points of Arc with Line.
void Arc::intersect(const Line & l, std::vector<Vector> & result) const {
    Matrix m = iM.inverse();
    Vector p = m * l.iP;
    Vector d = (m.linear() * l.dir()).normalized();
    // solve quadratic equation
    double b = 2 * dot(p, d);
    double c = dot(p, p) - 1.0;
    double D = b * b - 4 * c;
    if (D < 0) return;
    double sD = (b < 0) ? -sqrt(D) : sqrt(D);
    double t1 = -0.5 * (b + sD);
    Vector v = p + t1 * d;
    if (v.angle().liesBetween(iAlpha, iBeta)) result.push_back(iM * v);
    if (D > 0) {
	v = p + (c / t1) * d;
	if (v.angle().liesBetween(iAlpha, iBeta)) result.push_back(iM * v);
    }
}

//! Compute intersection points of Arc with Segment.
void Arc::intersect(const Segment & s, std::vector<Vector> & result) const {
    std::vector<Vector> pt;
    intersect(s.line(), pt);
    Vector dir = s.iQ - s.iP;
    for (int i = 0; i < size(pt); ++i) {
	// check whether it's on the segment
	Vector v = pt[i];
	if (dot(v - s.iP, dir) >= 0 && dot(v - s.iQ, dir) <= 0) result.push_back(v);
    }
}

#ifdef IPE_GSL
/* Computes intersection points between a unit SEMI-circle and the
 * ellipse defined by the inverse of m.
 *
 * We write x = 2t/(1+t^2) and y = (1-t^2)/(1+t^2) for -1 <= t <= 1
 * This parameterizes the points (x, y) on the upper unit semicircle.
 * We map with respect to m to obtain (x1, y1), and set x1^2 + y1^2 - 1 = 0.
 *
 * If m = [ a, b, c, d, e, f ], the point m * (x, y) is ax + cy + e, bx + dy + f,
 * so we set (ax + cy +e)^2 + (bx + dy + f)^2 - 1 = 0, where x, y are as above.
 * aaxx + ccyy + ee + 2axcy + 2axe + 2cye + bbxx + ddyy + ff + 2bxdy + 2bxf + 2dyf - 1 = 0
 * or (aa + bb)xx + (cc + dd)yy + (2ac + 2bd)xy + (2ae + 2bf)x + (2ce + 2df)y + (ee + ff -
 * 1) = 0
 *
 * Substituting x and y using t and multiplying with (1+tt)^2, this gives
 * (aa + bb)4tt + (cc + dd)(1-tt)(1-tt) + (2ac + 2bd)2t(1-tt) +
 * (2ae + 2bf)2t(1+tt) + (2ce + 2df)(1-tt)(1+tt) + (ee + ff - 1)(1+tt)(1+tt) = 0
 *
 * Collecting terms of t:
 *  t^4: cc + dd - (2ce + 2df) + (ee + ff - 1)
 *  t^3: -2(2ac + 2bd) + 2(2ae + 2bf)
 *  t^2: 4(aa + bb) - 2(cc + dd) + 2(ee + ff - 1)
 *  t  : 2(2ac + 2bd) + 2(2ae + 2bf)
 *  1  : (cc + dd) + (2ce + 2df) + (ee + ff - 1)
 *
 * We return points on the unit circle.
 */
static void ellipseIntersections(const Matrix & m, double ysign,
				 std::vector<Vector> & pts) {
    double a = m.a[0];
    double b = m.a[1];
    double c = ysign * m.a[2];
    double d = ysign * m.a[3];
    double e = m.a[4];
    double f = m.a[5];

    double t4 = c * c + d * d - (2 * c * e + 2 * d * f) + (e * e + f * f - 1);
    double t3 = -2 * (2 * a * c + 2 * b * d) + 2 * (2 * a * e + 2 * b * f);
    double t2 = 4 * (a * a + b * b) - 2 * (c * c + d * d) + 2 * (e * e + f * f - 1);
    double t1 = 2 * (2 * a * c + 2 * b * d) + 2 * (2 * a * e + 2 * b * f);
    double t0 = (c * c + d * d) + (2 * c * e + 2 * d * f) + (e * e + f * f - 1);

    // coefficients of P(x) =  t4 * t^4 + ... t1 * t + t0
    double coeff[5] = {t0, t1, t2, t3, t4};

    int deg = 4;
    while (deg > 0 && coeff[deg] == 0.0) --deg;
    if (!deg) // either identical zero or never zero
	return;

    gsl_poly_complex_workspace * w = gsl_poly_complex_workspace_alloc(deg + 1);

    double z[8];
    int status = gsl_poly_complex_solve(coeff, deg + 1, w, z);

    gsl_poly_complex_workspace_free(w);

    if (status != GSL_SUCCESS) return; // report no intersections

    for (int i = 0; i < deg; i++) {
#ifdef GSL_OUTPUT
	ipeDebug("z%d = %+.18f %+.18f", i, z[2 * i], z[2 * i + 1]);
#endif
	if (fabs(z[2 * i + 1]) < 1e-12) {
	    // we take this as a real root
	    double t = z[2 * i];
	    if (-1.0 - 1e-6 <= t && t <= 1.0 + 1e-6) {
		double tt = t * t;
		pts.push_back(Vector(2 * t / (1 + tt), ysign * (1 - tt) / (1 + tt)));
	    }
	}
    }
}

//! Compute intersection points of Arc with Arc.
void Arc::intersect(const Arc & a, std::vector<Vector> & result) const {
    if (!bbox().intersects(a.bbox())) return;

    std::vector<Vector> pts;
    Matrix m = iM.inverse() * a.iM;
    ellipseIntersections(m, +1.0, pts); // upper semi-circle
    ellipseIntersections(m, -1.0, pts); // lower semi-circle

    for (int i = 0; i < size(pts); ++i) {
	Vector & p = pts[i];
	if (p.angle().liesBetween(a.iAlpha, a.iBeta)) {
	    // if (1,0) or (-1,0) is an intersection point, it will be
	    // returned by both calls, filter out
	    int j = 0;
	    while (j < i && (pts[j] - p).sqLen() > 1e-12) ++j;
	    if (j == i) {
		Vector q = m * p;
		if (q.angle().liesBetween(iAlpha, iBeta)) result.push_back(a.iM * p);
	    }
	}
    }
}

#else

//! Compute intersection points of Arc with Arc.
void Arc::intersect(const Arc & a, std::vector<Vector> & result) const {
    /* Recursive approximation procedure to find intersections:
     * If the bounding boxes of two Arcs overlap, both are subdivided,
     * each one into two partial Arcs.
     * In the next recursion steps, it is checked if the bounding boxes
     * of the partial Arcs overlap. If they do, they are subdivided
     * again and so on, until a special precision is achieved.
     */
    const double PRECISION = 0.05; // 0.05 is about ~2.8647 degrees

    if (!bbox().intersects(a.bbox())) return;

    if (straight(PRECISION) && a.straight(PRECISION)) {
	intersect(Segment(a.beginp(), a.endp()), result);
    } else {
	Arc al, ar;
	subdivide(al, ar);
	Arc bl, br;
	a.subdivide(bl, br);

	al.intersect(bl, result);
	al.intersect(br, result);
	ar.intersect(bl, result);
	ar.intersect(br, result);
    }
}
#endif

//! Compute intersection points of Arc with Bezier.
void Arc::intersect(const Bezier & b, std::vector<Vector> & result) const {
    /* Recursive approximation procedure to find intersections: If the
     * bounding boxes of the Bezier and the Arc overlap, both are
     * subdivided, the Bezier into two Beziers and the Arc into two
     * Arcs.  In the next recursion steps, it is checked if a bounding
     * box of a partial Bezier overlap one of a partial Arc. If they
     * overlap, they are subdivided again and so on, until a special
     * precision is achieved.
     */

    const double PRECISION = 0.05; // 0.05 is about ~2.8647 degrees

    Rect bboxB(b.iV[0], b.iV[1]);
    bboxB.addPoint(b.iV[2]);
    bboxB.addPoint(b.iV[3]);
    if (!bbox().intersects(bboxB)) return;

    if (b.straight(BEZIER_INTERSECT_PRECISION)) {
#ifdef IPE_GSL
	std::vector<Vector> pts;
	intersect(Segment(b.iV[0], b.iV[3]), pts);
	for (const auto & q : pts) {
	    result.push_back(nearbyBezierEllipseIntersection(q, b, iM));
	}
#else
	intersect(Segment(b.iV[0], b.iV[3]), result);
#endif
    } else {
	// is it really useful to divide the arc?
	// the hope is to achieve emptiness of intersection more quickly
	if (straight(PRECISION)) {
	    // do not subdivide the arc any further
	    Bezier bl, br;
	    b.subdivide(bl, br);

	    intersect(bl, result);
	    intersect(br, result);
	} else {
	    Arc al, ar;
	    subdivide(al, ar);
	    Bezier bl, br;
	    b.subdivide(bl, br);

	    al.intersect(bl, result);
	    al.intersect(br, result);
	    ar.intersect(bl, result);
	    ar.intersect(br, result);
	}
    }
}

//! Subdivide this arc into two.
void Arc::subdivide(Arc & l, Arc & r) const {
    if (iAlpha == 0.0 && iBeta == IpeTwoPi) {
	l = Arc(iM, Angle(0), Angle(IpePi));
	r = Arc(iM, Angle(IpePi), Angle(IpeTwoPi));
    } else {
	// delta is length of arc
	double delta = Angle(iBeta).normalize(iAlpha) - iAlpha;
	Angle gamma(iAlpha + delta / 2);
	l = Arc(iM, iAlpha, gamma);
	r = Arc(iM, gamma, iBeta);
    }
}

/*! Returns true if the difference between start- and endangle is less
 * than precision.
 */
bool Arc::straight(const double precision) const {
    if (iAlpha == 0.0 && iBeta == IpeTwoPi) return false;

    return Angle(iBeta).normalize(iAlpha) - iAlpha < precision;
}

// --------------------------------------------------------------------
