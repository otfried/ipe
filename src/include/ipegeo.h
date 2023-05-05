// -*- C++ -*-
// --------------------------------------------------------------------
// Geometric primitives
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

#ifndef IPEGEO_H
#define IPEGEO_H

#include "ipebase.h"

#include <cmath>

// --------------------------------------------------------------------

#define IpePi 3.1415926535897932385
#define IpeTwoPi 6.2831853071795862
#define IpeHalfPi 1.5707963267948966

namespace ipe {

  /*! \ingroup geo
    Maximum of two values.
  */
  template<class T>
  inline T max(const T &lhs, const T &rhs)
  {
    return (lhs > rhs) ? lhs : rhs;
  }

  /*! \ingroup geo
    Minimum of two values.
  */
  template<class T>
  inline T min(const T &lhs, const T &rhs)
  {
    return (lhs < rhs) ? lhs : rhs;
  }

  /*! \ingroup geo
    Absolute value.
  */
  inline double abs(double val)
  {
    return (val > 0) ? val : -val;
  }


// --------------------------------------------------------------------

  class Angle {
  public:
    //! Construct uninitialized angle.
    inline explicit Angle() { /* nothing */ }
    //! Construct an angle (in radians).
    inline Angle(double alpha) : iAlpha(alpha) { }
    //! Construct an angle in degrees.
    inline static Angle Degrees(double alpha) {
      return Angle(alpha * IpePi / 180.0); }
    //! Return value (in radians).
    inline operator double() const { return iAlpha; }
    double degrees() const;
    Angle normalize(double lowlimit);
    bool liesBetween(Angle small, Angle large) const;
  private:
    double iAlpha;
  };

  // --------------------------------------------------------------------

  class Vector {
  public:
    //! Uninitialized vector.
    Vector() { /* no initialization */ }
    explicit Vector(Angle alpha);
    //! Construct a vector.
    explicit Vector(double x0, double y0) : x(x0), y(y0) { }
    //! Return square of Euclidean length
    inline double sqLen() const;
    double len() const;
    Angle angle() const;
    Vector normalized() const;
    Vector orthogonal() const;
    double factorize(Vector &unit) const;
    bool snap(const Vector &mouse, Vector &pos, double &bound) const;

    inline bool operator==(const Vector &rhs) const;
    inline bool operator!=(const Vector &rhs) const;
    inline void operator+=(const Vector &rhs);
    inline void operator-=(const Vector &rhs);
    inline void operator*=(double rhs);
    inline Vector operator+(const Vector &rhs) const;
    inline Vector operator-(const Vector &rhs) const;
    inline Vector operator*(double rhs) const;
    inline Vector operator-() const;

    static Vector ZERO;

  public:
    double x; //!< Coordinates are public.
    double y; //!< Coordinates are public.
  };

  /*! \relates Vector */
  Stream &operator<<(Stream &stream, const Vector &rhs);

  // --------------------------------------------------------------------

  class Rect {
  public:
    //! Create empty rectangle.
    explicit Rect() : iMin(1,0), iMax(-1,0) { }
    //! Create rectangle containing just the point \a c.
    explicit Rect(const Vector &c) : iMin(c), iMax(c) { }
    explicit Rect(const Vector &c1, const Vector &c2);

    //! Make rectangle empty.
    void clear() { iMin.x = 1.0; iMax.x = -1.0; iMin.y = iMax.y = 0; }
    //! True if rectangle is empty.
    int isEmpty() const { return iMin.x > iMax.x; }
    //! Return left side.
    inline double left() const { return iMin.x; }
    //! Return right side.
    inline double right() const { return iMax.x; }
    //! Return bottom side.
    inline double bottom() const { return iMin.y; }
    //! Return top side.
    inline double top() const { return iMax.y; }
    //! Return top right corner.
    inline Vector topRight() const { return iMax; }
    //! Return bottom left corner.
    inline Vector bottomLeft() const { return iMin; }
    //! Return top left corner.
    inline Vector topLeft() const { return Vector(iMin.x, iMax.y); }
    //! Return bottom right corner.
    inline Vector bottomRight() const { return Vector(iMax.x, iMin.y); }
    //! Return center of rectangle.
    inline Vector center() const { return (iMin + iMax) * 0.5; }

    //! Return width.
    double width() const { return iMax.x - iMin.x; }
    //! Return height.
    double height() const { return iMax.y - iMin.y; }
    void addPoint(const Vector &rhs);
    void addRect(const Rect &rhs);
    void clipTo(const Rect &rhs);
    bool contains(const Vector &rhs) const;
    bool contains(const Rect &rhs) const;
    bool certainClearance(const Vector &v, double bound) const;
    bool intersects(const Rect &rhs) const;
  private:
    Vector iMin;   //!< Lower-left corner.
    Vector iMax;   //!< Top-right corner.
  };

  /*! \relates Rect */
  Stream &operator<<(Stream &stream, const Rect &rhs);

  // --------------------------------------------------------------------

  class Line {
  public:
    //! Create default line (x-axis).
    explicit Line() : iP(0.0, 0.0), iDir(1.0, 0.0) { }
    explicit Line(const Vector &p, const Vector &dir);
    static Line through(const Vector &p, const Vector &q);
    double side(const Vector &p) const;
    inline Vector normal() const;
    double distance(const Vector &v) const;
    bool intersects(const Line &line, Vector &pt);
    Vector project(const Vector &v) const;
    inline Vector dir() const;

  public:
    //! Point on the line.
    Vector iP;
  private:
    Vector iDir; // unit vector
  };

  // --------------------------------------------------------------------

  class Segment {
  public:
    //! Create uninitialized segment
    Segment() { /* nothing */ }
    explicit Segment(const Vector &p, const Vector &q) : iP(p), iQ(q) { }
    inline Line line() const;
    double distance(const Vector &v, double bound) const;
    double distance(const Vector &v) const;
    bool project(const Vector &v, Vector &projection) const;
    bool intersects(const Segment &seg, Vector &pt) const;
    bool intersects(const Line &l, Vector &pt) const;
    bool snap(const Vector &mouse, Vector &pos, double &bound) const;
  public:
    //! First endpoint.
    Vector iP;
    //! Second endpoint.
    Vector iQ;
  };

  // --------------------------------------------------------------------

  class Bezier {
  public:
    //! Default constructor, uninitialized curve.
    inline Bezier() { /* nothing */ }
    inline Bezier(const Vector &p0, const Vector &p1,
		  const Vector &p2, const Vector &p3);
    Vector point(double t) const;
    Vector tangent(double t) const;
    double distance(const Vector &v, double bound);
    bool straight(double precision) const;
    void subdivide(Bezier &l, Bezier &r) const;
    void approximate(double precision, std::vector<Vector> &result) const;
    Rect bbox() const;
    bool snap(const Vector &v, double &t, Vector &pos, double &bound) const;
    static Bezier quadBezier(const Vector &p0, const Vector &p1,
			     const Vector &p2);
    static void oldSpline(int n,  const Vector *v,
			  std::vector<Bezier> &result);
    static void spline(int n,  const Vector *v,
		       std::vector<Bezier> &result);
    static void cardinalSpline(int n,  const Vector *v, double tension,
			       std::vector<Bezier> &result);
    static void spiroSpline(int n,  const Vector *v, std::vector<Bezier> &result);
    static void closedSpline(int n,  const Vector *v,
			     std::vector<Bezier> &result);

    void intersect(const Line &l, std::vector<Vector> &result) const;
    void intersect(const Segment &l, std::vector<Vector> &result) const;
    void intersect(const Bezier &b, std::vector<Vector> &result) const;
  public:
    Vector iV[4];
  };

  // --------------------------------------------------------------------

  class Linear {
  public:
    Linear();
    explicit Linear(Angle angle);
    inline explicit Linear(double m11, double m21, double m12, double m22);
    explicit Linear(String str);
    Linear inverse() const;
    inline bool isIdentity() const;
    inline Vector operator*(const Vector &rhs) const;
    inline bool operator==(const Linear &rhs) const;
    inline double determinant() const;
  public:
    double a[4];
  };

  /*! \relates Linear */
  Stream &operator<<(Stream &stream, const Linear &rhs);

  // --------------------------------------------------------------------

  class Matrix {
  public:
    Matrix();
    inline Matrix(const Linear &linear);
    inline explicit Matrix(const Linear &linear, const Vector &t);
    inline explicit Matrix(double m11, double m21, double m12, double m22,
			   double t1, double t2);
    inline explicit Matrix(const Vector &v);
    explicit Matrix(String str);
    Matrix inverse() const;
    inline Vector operator*(const Vector &rhs) const;
    inline Bezier operator*(const Bezier &rhs) const;
    inline Vector translation() const;
    inline Linear linear() const;
    inline double determinant() const;
    inline bool isIdentity() const;
    inline bool operator==(const Matrix &rhs) const;
  public:
    double a[6];
  };

  /*! \relates Matrix */
  Stream &operator<<(Stream &stream, const Matrix &rhs);

  // --------------------------------------------------------------------

  class Arc {
  public:
    inline Arc();
    inline Arc(const Matrix &m, Angle alpha, Angle beta);
    inline Arc(const Matrix &m);
    Arc(const Matrix &m0, const Vector &begp, const Vector &endp);

    inline bool isEllipse() const;

    double distance(const Vector &v, double bound) const;
    double distance(const Vector &v, double bound,
		    Vector &pos, Angle &angle) const;
    Rect bbox() const;
    inline Vector beginp() const;
    inline Vector endp() const;

    void intersect(const Line &l, std::vector<Vector> &result) const;
    void intersect(const Segment &s, std::vector<Vector> &result) const;
    void intersect(const Arc &a, std::vector<Vector> &result) const;
    void intersect(const Bezier &b, std::vector<Vector> &result) const;
  private:
    void subdivide(Arc &l, Arc &r) const;
    bool straight(const double precision) const;

  public:
    Matrix iM;
    Angle iAlpha;
    Angle iBeta;
  };

  // --------------------------------------------------------------------

  //! Return square of vector's length.
  inline double Vector::sqLen() const
  {
    return (x * x + y * y);
  }

  //! Equality.
  inline bool Vector::operator==(const Vector &rhs) const
  {
    return x == rhs.x && y == rhs.y;
  }

  //! Inequality.
  inline bool Vector::operator!=(const Vector &rhs) const
  {
    return x != rhs.x || y != rhs.y;
  }

  //! Vector-addition.
  inline void Vector::operator+=(const Vector &rhs)
  {
    x += rhs.x; y += rhs.y;
  }

  //! Vector-subtraction.
  inline void Vector::operator-=(const Vector &rhs)
  {
    x -= rhs.x; y -= rhs.y;
  }

  //! Multiply vector by scalar.
  inline void Vector::operator*=(double rhs)
  {
    x *= rhs; y *= rhs;
  }

  //! Vector-addition.
  inline Vector Vector::operator+(const Vector &rhs) const
  {
    Vector result = *this; result += rhs; return result;
  }

  //! Vector-subtraction.
  inline Vector Vector::operator-(const Vector &rhs) const
  {
    Vector result = *this; result -= rhs; return result;
  }

  //! Vector * scalar.
  inline Vector Vector::operator*(double rhs) const
  {
    Vector result = *this; result *= rhs; return result;
  }

  //! Scalar * vector. \relates Vector
  inline Vector operator*(double lhs, const Vector &rhs)
  {
    return Vector(lhs * rhs.x, lhs * rhs.y);
  }

  //! Dotproduct of two vectors. \relates Vector
  inline double dot(const Vector &lhs, const Vector &rhs)
  {
    return lhs.x * rhs.x + lhs.y * rhs.y;
  }

  //! Return a normal vector pointing to the left of the directed line.
  inline Vector Line::normal() const
  {
    return Vector(-iDir.y, iDir.x);
}

  //! Return direction of line.
  inline Vector Line::dir() const
  {
    return iDir;
  }

  //! Return directed line supporting the segment.
  inline Line Segment::line() const
  {
    return Line(iP, (iQ - iP).normalized());
  }

  //! Unary minus for Vector.
  inline Vector Vector::operator-() const
  {
    return -1 * *this;
  }

  //! Constructor with four control points.
  inline Bezier::Bezier(const Vector &p0, const Vector &p1,
			const Vector &p2, const Vector &p3)
  {
    iV[0] = p0; iV[1] = p1; iV[2] = p2; iV[3] = p3;
  }

  //! Transform Bezier spline. \relates Matrix
  inline Bezier Matrix::operator*(const Bezier &rhs) const
  {
    return Bezier(*this * rhs.iV[0], *this * rhs.iV[1],
		  *this * rhs.iV[2], *this * rhs.iV[3]);
  };

  // --------------------------------------------------------------------

  //! Construct unit circle.
  inline Arc::Arc() : iAlpha(0.0), iBeta(IpeTwoPi)
  {
    // nothing
  }

  //! Construct with given parameters.
  inline Arc::Arc(const Matrix &m, Angle alpha, Angle beta)
    : iM(m), iAlpha(alpha), iBeta(beta)
  {
    // nothing
  }

  //! Construct an ellipse
  inline Arc::Arc(const Matrix &m)
    : iM(m), iAlpha(0.0), iBeta(IpeTwoPi)
  {
    // nothing
  }

  //! Is this an entire ellipse?
  inline bool Arc::isEllipse() const
  {
    return iAlpha == 0.0 && iBeta == IpeTwoPi;
  }

  //! Return begin point of arc.
  inline Vector Arc::beginp() const
  {
    return iM * Vector(Angle(iAlpha));
  }

  //! Return end point of arc.
  inline Vector Arc::endp() const
  {
    return iM * Vector(Angle(iBeta));
  }

  // --------------------------------------------------------------------

  //! Create identity matrix.
  inline Linear::Linear()
  {
    a[0] = a[3] = 1.0;
    a[1] = a[2] = 0.0;
  }

  //! Create linear matrix with given coefficients.
  inline Linear::Linear(double m11, double m21, double m12, double m22)
  {
    a[0] = m11; a[1] = m21; a[2] = m12; a[3] = m22;
  }

  //! Linear matrix times vector. \relates Linear
  inline Vector Linear::operator*(const Vector &rhs) const
  {
    return Vector(a[0] * rhs.x + a[2] * rhs.y,
		  a[1] * rhs.x + a[3] * rhs.y);
  };

  //! Is this the identity matrix?
  inline bool Linear::isIdentity() const
  {
    return (a[0] == 1.0 && a[1] == 0.0 &&
	    a[2] == 0.0 && a[3] == 1.0);
  }

  //! Linear matrix multiplication. \relates Linear
  inline Linear operator*(const Linear &lhs, const Linear &rhs)
  {
    Linear m;
    m.a[0] = lhs.a[0] * rhs.a[0] + lhs.a[2] * rhs.a[1];
    m.a[1] = lhs.a[1] * rhs.a[0] + lhs.a[3] * rhs.a[1];
    m.a[2] = lhs.a[0] * rhs.a[2] + lhs.a[2] * rhs.a[3];
    m.a[3] = lhs.a[1] * rhs.a[2] + lhs.a[3] * rhs.a[3];
    return m;
  }

  //! Check for equality of two linear matrices.
  inline bool Linear::operator==(const Linear &rhs) const
  {
    return (a[0] == rhs.a[0] && a[1] == rhs.a[1] &&
	    a[2] == rhs.a[2] && a[3] == rhs.a[3]);
  }

  //! Return determinant of a linear matrix.
  inline double Linear::determinant() const
  {
    return (a[0] * a[3] - a[1] * a[2]);
  }

  // --------------------------------------------------------------------

  //! Create matrix with given coefficients.
  inline Matrix::Matrix(double m11, double m21, double m12, double m22,
			double t1, double t2)
  {
    a[0] = m11; a[1] = m21; a[2] = m12; a[3] = m22;
    a[4] = t1; a[5] = t2;
  }

  //! Create linear matrix.
  inline Matrix::Matrix(const Linear &linear)
  {
    a[0] = linear.a[0]; a[1] = linear.a[1];
    a[2] = linear.a[2]; a[3] = linear.a[3];
    a[4] = a[5] = 0.0;
  }

  inline Matrix::Matrix(const Linear &linear, const Vector &t)
  {
    a[0] = linear.a[0]; a[1] = linear.a[1];
    a[2] = linear.a[2]; a[3] = linear.a[3];
    a[4] = t.x; a[5] = t.y;
  }

  //! Matrix times vector. \relates Matrix
  inline Vector Matrix::operator*(const Vector &rhs) const
  {
    return Vector(a[0] * rhs.x + a[2] * rhs.y + a[4],
		  a[1] * rhs.x + a[3] * rhs.y + a[5]);
  };

  //! Is this the identity matrix?
  inline bool Matrix::isIdentity() const
  {
    return (a[0] == 1.0 && a[1] == 0.0 &&
	    a[2] == 0.0 && a[3] == 1.0 &&
	    a[4] == 0.0 && a[5] == 0.0);
  }

  //! Create identity matrix.
  inline Matrix::Matrix()
  {
    a[0] = a[3] = 1.0;
    a[1] = a[2] = a[4] = a[5] = 0.0;
  }

  //! Matrix multiplication. \relates Matrix
  inline Matrix operator*(const Matrix &lhs, const Matrix &rhs)
  {
    Matrix m;
    m.a[0] = lhs.a[0] * rhs.a[0] + lhs.a[2] * rhs.a[1];
    m.a[1] = lhs.a[1] * rhs.a[0] + lhs.a[3] * rhs.a[1];
    m.a[2] = lhs.a[0] * rhs.a[2] + lhs.a[2] * rhs.a[3];
    m.a[3] = lhs.a[1] * rhs.a[2] + lhs.a[3] * rhs.a[3];
    m.a[4] = lhs.a[0] * rhs.a[4] + lhs.a[2] * rhs.a[5] + lhs.a[4];
    m.a[5] = lhs.a[1] * rhs.a[4] + lhs.a[3] * rhs.a[5] + lhs.a[5];
    return m;
  }

  //! Create translation matrix.
  inline Matrix::Matrix(const Vector &v)
  {
    a[0] = a[3] = 1.0;
    a[1] = a[2] = 0.0;
    a[4] = v.x;
    a[5] = v.y;
  }

  //! Return translation component.
  inline Vector Matrix::translation() const
  {
    return Vector(a[4], a[5]);
  }

  //! Return determinant of the matrix.
  inline double Matrix::determinant() const
  {
    return a[0]*a[3]-a[1]*a[2];
  }

  //! Check for equality of two matrices.
  inline bool Matrix::operator==(const Matrix &rhs) const
  {
    return (a[0] == rhs.a[0] && a[1] == rhs.a[1] && a[2] == rhs.a[2] &&
	    a[3] == rhs.a[3] && a[4] == rhs.a[4] && a[5] == rhs.a[5]);
  }

  //! Return linear transformation component of this affine transformation.
  inline Linear Matrix::linear() const
  {
    return Linear(a[0], a[1], a[2], a[3]);
  }

  //! Transform arc. \relates Matrix
  // must be here because it uses Matrix multiplication above
  inline Arc operator*(const Matrix &lhs, const Arc &rhs)
  {
    return Arc(lhs * rhs.iM, rhs.iAlpha, rhs.iBeta);
  }

} // namespace

// --------------------------------------------------------------------
#endif
