// -*- C++ -*-
// --------------------------------------------------------------------
// Drawable shapes
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

#ifndef IPESHAPE_H
#define IPESHAPE_H

#include "ipegeo.h"

// --------------------------------------------------------------------

namespace ipe {

  class Painter;

  class Ellipse;
  class ClosedSpline;
  class Curve;

  class CurveSegment {
  public:
    enum Type { EArc, ESegment, ESpline, EOldSpline, ECardinalSpline, ESpiroSpline };

    //! Type of segment.
    Type type() const;
    //! Number of control points.
    inline int countCP() const { return iNumCP; }
    //! Return control point.
    inline Vector cp(int i) const { return cps()[i]; }
    //! Return last control point.
    Vector last() const;
    //! Matrix (if Type() == EArc).
    Matrix matrix() const;

    float tension() const;
    Arc arc() const;
    void beziers(std::vector<Bezier> &bez) const;

    void draw(Painter &painter) const;
    void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    double distance(const Vector &v, const Matrix &m, double bound) const;
    void snapVtx(const Vector &mouse, const Matrix &m,
		 Vector &pos, double &bound, bool cp) const;
    void snapBnd(const Vector &mouse, const Matrix &m,
		 Vector &pos, double &bound) const;
  private:
    CurveSegment(const Curve *curve, int index);

    const Vector *cps() const;

  private:
    const Curve *iCurve;
    int index; // index of the segment in the curve
    int iNumCP;

    friend class Curve;
  };

  class SubPath {
  public:
    //! The subpath types.
    enum Type { ECurve, EEllipse, EClosedSpline };
    virtual ~SubPath() = 0;
    //! Return type of this subpath.
    virtual Type type() const = 0;
    virtual bool closed() const;

    virtual const Ellipse *asEllipse() const;
    virtual const ClosedSpline *asClosedSpline() const;
    virtual const Curve *asCurve() const;

    //! Save subpath to XML stream.
    virtual void save(Stream &stream) const = 0;
    //! Draw subpath (does not call drawPath()).
    virtual void draw(Painter &painter) const = 0;
    //! Add subpath to box.
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const = 0;
    //! Return distance from \a v to subpath transformed by \a m.
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const = 0;
    //! Snap to vertex.
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound, bool cp) const = 0;
    //! Snap to boundary of subpath.
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const = 0;
  };

  class Ellipse : public SubPath {
  public:
    Ellipse(const Matrix &m);
    virtual Type type() const;
    virtual const Ellipse *asEllipse() const;
    //! Return matrix that transforms unit circle to the ellipse.
    inline Matrix matrix() const { return iM; }

    virtual void save(Stream &stream) const;
    virtual void draw(Painter &painter) const;
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const;
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound, bool cp) const;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
  private:
    Matrix iM;
  };

  class ClosedSpline : public SubPath {
  public:
    ClosedSpline(const std::vector<Vector> &v);
    virtual Type type() const;
    virtual const ClosedSpline *asClosedSpline() const;
    void beziers(std::vector<Bezier> &bez) const;
    virtual void save(Stream &stream) const;
    virtual void draw(Painter &painter) const;
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const;
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound, bool cp) const;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
  public:
    std::vector<Vector> iCP; // control points
  };

  class Curve : public SubPath {
  public:
    Curve();
    virtual Type type() const;
    inline virtual bool closed() const { return iClosed; }
    virtual const Curve *asCurve() const;
    virtual void save(Stream &stream) const;
    virtual void draw(Painter &painter) const;
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const;
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound, bool cp) const;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;

    /*! \brief Return number of segments.
      This does not include the closing segment for a closed path. */
    int countSegments() const { return iClosed ? iSeg.size() - 1 : iSeg.size(); }
    //! Return number of segments including the closing segment.
    int countSegmentsClosing() const { return iSeg.size(); }

    CurveSegment segment(int i) const;
    CurveSegment closingSegment() const;

    void appendSegment(const Vector &v0, const Vector &v1);
    void appendArc(const Matrix &m, const Vector &v0, const Vector &v1);
    void appendSpline(const std::vector<Vector> &v) {
      appendSpline(v, CurveSegment::ESpline); }
    void appendOldSpline(const std::vector<Vector> &v) {
      appendSpline(v, CurveSegment::EOldSpline); }
    void appendCardinalSpline(const std::vector<Vector> &v, float tension);
    void appendSpiroSpline(const std::vector<Vector> &v);
    void appendSpiroSplinePrecomputed(const std::vector<Vector> &v, int sep);
    void setClosed(bool closed);

  private:
    void appendSpline(const std::vector<Vector> &v, CurveSegment::Type type);

  private:
    struct Seg {
      CurveSegment::Type iType;
      int32_t iLastCP;
      union {
	int32_t iMatrix;
	float iTension;
	// index into iCP separating precomputed Bezier control points
	// from the spiro control points: the last precomputed cp
	int32_t iBezier;
      };
    };
    bool iClosed;
    std::vector<Seg> iSeg;
    std::vector<Vector> iCP; // control points
    std::vector<Matrix> iM;  // for arcs

    friend class CurveSegment;
  };

  inline CurveSegment::Type CurveSegment::type() const { return iCurve->iSeg[index].iType; }

  inline const Vector *CurveSegment::cps() const {
    return iCurve->iCP.data() + (iCurve->iSeg[index].iLastCP - iNumCP + 1); }

  inline Vector CurveSegment::last() const {
    return iCurve->iCP[iCurve->iSeg[index].iLastCP]; }

  inline Matrix CurveSegment::matrix() const {
    return iCurve->iM[iCurve->iSeg[index].iMatrix]; }

  class Shape {
  public:
    Shape();
    explicit Shape(const Rect &rect);
    explicit Shape(const Segment &seg);
    explicit Shape(const Vector &center, double radius);
    explicit Shape(const Vector &center, double radius,
		   double alpha0, double alpha1);

    ~Shape();
    Shape(const Shape &rhs);
    Shape &operator=(const Shape &rhs);

    bool load(String data);
    void save(Stream &stream) const;

    void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    double distance(const Vector &v, const Matrix &m, double bound) const;
    void snapVtx(const Vector &mouse, const Matrix &m,
		 Vector &pos, double &bound, bool cp) const;
    void snapBnd(const Vector &mouse, const Matrix &m,
		 Vector &pos, double &bound) const;

    //! Return number of subpaths.
    inline int countSubPaths() const { return iImp->iSubPaths.size(); }
    //! Return subpath.
    inline const SubPath *subPath(int i) const { return iImp->iSubPaths[i]; }

    bool isSegment() const;

    void appendSubPath(SubPath *sp);

    void draw(Painter &painter) const;
  private:
    typedef std::vector<SubPath *> SubPathSeq;
    struct Imp {
      ~Imp();
      int iRefCount;
      SubPathSeq iSubPaths;
    };
    Imp *iImp;
  };

} // namespace

// --------------------------------------------------------------------
#endif

