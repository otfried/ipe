// --------------------------------------------------------------------
// Snapping
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

#include "ipesnap.h"
#include "ipepage.h"
#include "ipegroup.h"
#include "ipereference.h"
#include "ipepath.h"

using namespace ipe;

/*! \defgroup high Ipe Utilities
  \brief Classes to manage Ipe documents and objects.

  This module contains classes used in the implementation of the Ipe
  program itself.  The only classes from this module you may be
  interested in are Visitor (which is essential to traverse an Ipe
  object structure), and perhaps Snap (if you are writing an Ipelet
  whose behavior depends on the current snap setting in the Ipe
  program).
*/

/*! \class ipe::Snap
  \ingroup high
  \brief Performs snapping operations, and stores snapping state.

*/

// --------------------------------------------------------------------

class CollectSegs : public Visitor {
public:
  CollectSegs(const Vector &mouse, double snapDist,
	      const Page *page, int view);

  virtual void visitGroup(const Group *obj);
  virtual void visitPath(const Path *obj);

public:
  std::vector<Segment> iSegs;
  std::vector<Bezier> iBeziers;
  std::vector<bool> iBeziersCont; // true if continuation of previous bezier
  std::vector<Arc> iArcs;

private:
  std::vector<Matrix> iMatrices;
  Vector iMouse;
  double iDist;
};

CollectSegs::CollectSegs(const Vector &mouse, double snapDist,
			 const Page *page, int view)
  : iMouse(mouse), iDist(snapDist)
{
  iMatrices.push_back(Matrix()); // identity matrix
  if (view < 0) {
    int gridLayer = page->findLayer("GRID");
    if (gridLayer < 0) return;  // nothing
    for (int i = 0; i < page->count(); ++i) {
      if (page->layerOf(i) == gridLayer)
	page->object(i)->accept(*this);
    }
  } else {
    for (int i = 0; i < page->count(); ++i) {
      if (page->objSnapsInView(i, view))
	page->object(i)->accept(*this);
    }
  }
}

void CollectSegs::visitGroup(const Group *obj)
{
  iMatrices.push_back(iMatrices.back() * obj->matrix());
  for (Group::const_iterator it = obj->begin(); it != obj->end(); ++it)
    (*it)->accept(*this);
  iMatrices.pop_back();
}

// TODO: use bounding boxes for subsegs/beziers to speed up ?
void CollectSegs::visitPath(const Path *obj)
{
  Bezier b;
  Arc arc;
  Matrix m = iMatrices.back() * obj->matrix();
  for (int i = 0; i < obj->shape().countSubPaths(); ++i) {
    const SubPath *sp = obj->shape().subPath(i);
    switch (sp->type()) {
    case SubPath::EEllipse:
      if (sp->distance(iMouse, m, iDist) < iDist)
	iArcs.push_back(m * Arc(sp->asEllipse()->matrix()));
      break;
    case SubPath::EClosedSpline: {
      std::vector<Bezier> bez;
      bool cont = false;
      sp->asClosedSpline()->beziers(bez);
      for (const Bezier & bz : bez) {
	b = m * bz;
	if (b.distance(iMouse, iDist) < iDist) {
	  iBeziers.push_back(b);
	  iBeziersCont.push_back(cont);
	  cont = true;
	} else
	  cont = false;
      }
      break; }
    case SubPath::ECurve: {
      const Curve *ssp = sp->asCurve();
      for (int j = 0; j < ssp->countSegmentsClosing(); ++j) {
	CurveSegment seg = ssp->segment(j);
	switch (seg.type()) {
	case CurveSegment::ESegment:
	  if (seg.distance(iMouse, m, iDist) < iDist)
	    iSegs.push_back(Segment(m * seg.cp(0), m * seg.cp(1)));
	  break;
	case CurveSegment::EArc:
	  arc = m * seg.arc();
	  if (arc.distance(iMouse, iDist) < iDist)
	    iArcs.push_back(arc);
	  break;
	case CurveSegment::EOldSpline:
	case CurveSegment::ESpline:
	case CurveSegment::ESpiroSpline:
	case CurveSegment::ECardinalSpline: {
	  std::vector<Bezier> bez;
	  seg.beziers(bez);
	  bool cont = false;
	  for (const Bezier & bz : bez) {
	    b = m * bz;
	    if (b.distance(iMouse, iDist) < iDist) {
	      iBeziers.push_back(b);
	      iBeziersCont.push_back(cont);
	      cont = true;
	    } else
	      cont = false;
	  }
	  break; }
	}
      }
      break; }
    }
  }
}

// --------------------------------------------------------------------

/*! Find line through \a base with slope determined by angular snap
  size and direction. */
Line Snap::getLine(const Vector &mouse, const Vector &base) const noexcept
{
  Angle alpha = iDir;
  Vector d = mouse - base;

  if (d.len() > 2.0) {
    alpha = d.angle() - iDir;
    alpha.normalize(0.0);
    alpha = iAngleSize * int(alpha / iAngleSize + 0.5) + iDir;
  }
  return Line(base, Vector(alpha));
}

//! Perform intersection snapping.
void Snap::intersectionSnap(const Vector &pos,  Vector &fifi,
			    const Page *page, int view,
			    double &snapDist) const noexcept
{
  CollectSegs segs(pos, snapDist, page, view);

  Vector v;
  std::vector<Vector> pts;

  // 1. seg-seg intersections
  for (int i = 0; i < size(segs.iSegs); ++i) {
    for (int j = i + 1; j < size(segs.iSegs); ++j) {
      if (segs.iSegs[i].intersects(segs.iSegs[j], v))
	pts.push_back(v);
    }
  }

  // 2. bezier-bezier and bezier-seg intersections
  for (int i = 0; i < size(segs.iBeziers); ++i) {
    for (int j = i + 1; j < size(segs.iBeziers); ++j) {
      if (j > i+1 || !segs.iBeziersCont[j])
	segs.iBeziers[i].intersect(segs.iBeziers[j], pts);
    }
    for (int j = 0; j < size(segs.iSegs); ++j)
      segs.iBeziers[i].intersect(segs.iSegs[j], pts);
  }

  // 3. arc-arc, arc-bezier, and arc-segment intersections
  for (int i = 0; i < size(segs.iArcs); ++i) {
    for (int j = i+1; j < size(segs.iArcs); ++j)
      segs.iArcs[i].intersect(segs.iArcs[j], pts);
    for (int j = 0; j < size(segs.iBeziers); ++j)
      segs.iArcs[i].intersect(segs.iBeziers[j], pts);
    for (int j = 0; j < size(segs.iSegs); ++j)
      segs.iArcs[i].intersect(segs.iSegs[j], pts);
  }

  double d = snapDist;
  Vector pos1 = pos;
  double d1;
  for (const auto & pt : pts) {
    if ((d1 = (pos - pt).len()) < d) {
      d = d1;
      pos1 = pt;
    }
  }

  if (d < snapDist) {
    fifi = pos1;
    snapDist = d;
  }
}

//! Perform snapping to intersection of angular line and pos.
bool Snap::snapAngularIntersection(Vector &pos, const Line &l,
				   const Page *page, int view,
				   double snapDist) const noexcept
{
  CollectSegs segs(pos, snapDist, page, view);

  std::vector<Vector> pts;
  Vector v;

  for (std::vector<Segment>::const_iterator it = segs.iSegs.begin();
       it != segs.iSegs.end(); ++it) {
    if (it->intersects(l, v))
      pts.push_back(v);
  }
  for (std::vector<Arc>::const_iterator it = segs.iArcs.begin();
       it != segs.iArcs.end(); ++it) {
    it->intersect(l, pts);
  }
  for (std::vector<Bezier>::const_iterator it = segs.iBeziers.begin();
       it != segs.iBeziers.end(); ++it) {
    it->intersect(l, pts);
  }

  double d = snapDist;
  Vector pos1 = pos;
  double d1;

  for (const auto & pt : pts) {
    if ((d1 = (pos - pt).len()) < d) {
      d = d1;
      pos1 = pt;
    }
  }

  if (d < snapDist) {
    pos = pos1;
    return true;
  }
  return false;
}

//! Tries vertex, intersection, boundary, and grid snapping.
/*! If snapping occurred, \a pos is set to the new user space position. */
Snap::TSnapModes Snap::simpleSnap(Vector &pos, const Page *page, int view,
				  double snapDist, Tool *tool) const noexcept
{
  double d = snapDist;
  Vector fifi = pos;

  // highest priority: vertex snapping
  if (iSnap & ESnapVtx) {
    for (int i = 0; i < page->count(); ++i) {
      if (page->objSnapsInView(i, view))
	page->snapVtx(i, pos, fifi, d);
    }
    if (tool)
      tool->snapVtx(pos, fifi, d, false);
  }

  double dvtx = d;
  Vector fifiCtl = pos;
  if (iSnap & ESnapCtl) {
    for (int i = 0; i < page->count(); ++i) {
      if (page->objSnapsInView(i, view))
	page->snapCtl(i, pos, fifiCtl, d);
    }
    if (tool)
      tool->snapVtx(pos, fifiCtl, d, true);
  }

  double dctl = d;
  Vector fifiX = pos;
  if (iSnap & ESnapInt)
    intersectionSnap(pos, fifiX, page, view, d);

  // Return if snapping has occurred
  if (d < dctl) {
    pos = fifiX;
    return ESnapInt;
  } else if (d < dvtx) {
    pos = fifiCtl;
    return ESnapCtl;
  } else if (d < snapDist) {
    pos = fifi;
    return ESnapVtx;
  }

  // boundary snapping
  if (iSnap & ESnapBd) {
    for (int i = 0; i < page->count(); ++i) {
      if (page->objSnapsInView(i, view))
	page->snapBnd(i, pos, fifi, d);
    }
    if (d < snapDist) {
      pos = fifi;
      return ESnapBd;
    }
  }

  // custom grid snapping
  if (iSnap & ESnapCustom) {
    intersectionSnap(pos, fifi, page, -1, d);
    if (d < snapDist) {
      pos = fifi;
      return ESnapCustom;
    }
  }

  // grid snapping: always occurs
  if (iSnap & ESnapGrid) {
    int grid = iGridSize;
    fifi.x = grid * int(pos.x / grid + (pos.x > 0 ? 0.5 : -0.5));
    fifi.y = grid * int(pos.y / grid + (pos.y > 0 ? 0.5 : -0.5));
    pos = fifi;
    return ESnapGrid;
  }

  return ESnapNone;
}

//! Performs snapping of position \a pos.
/*! Returns \c true if snapping occurred. In that case \a pos is set
  to the new user space position.

  Automatic angular snapping occurs if \a autoOrg is not null --- the
  value is then used as the origin for automatic angular snapping.
*/
Snap::TSnapModes Snap::snap(Vector &pos, const Page *page, int view,
			    double snapDist, Tool *tool,
			    Vector *autoOrg) const noexcept
{
  // automatic angular snapping and angular snapping both on?
  if (autoOrg && (iSnap & ESnapAuto) && (iSnap & ESnapAngle)) {
    // only one possible point!
    Line angular = getLine(pos, iOrigin);
    Line automat = getLine(pos, *autoOrg);
    Vector v;
    if (angular.intersects(automat, v) && v.sqLen() < 1e10) {
      pos = v;
      return ESnapAngle;
    }
    // if the two lines do not intersect, use following case
  }

  // case of only one angular snapping mode
  if ((iSnap & ESnapAngle) || (autoOrg && (iSnap & ESnapAuto))) {
    Vector org;
    if (iSnap & ESnapAngle)
      org = iOrigin;
    else
      org = *autoOrg;
    Line l = getLine(pos, org);
    pos = l.project(pos);
    if (iSnap & ESnapBd)
      snapAngularIntersection(pos, l, page, view, snapDist);
    return ESnapAngle;
  }

  // we are not in any angular snapping mode
  return simpleSnap(pos, page, view, snapDist, tool);
}

//! Set axis origin and direction from edge near mouse.
/*! Returns \c true if successful. */
bool Snap::setEdge(const Vector &pos, const Page *page, int view) noexcept
{
  // bound cannot be too small, as distance to Bezier
  // is computed based on an approximation of precision 1.0
  CollectSegs segs(pos, 2.0, page, view);

  if (!segs.iSegs.empty()) {
    Segment seg = segs.iSegs.back();
    Line l = seg.line();
    iOrigin = l.project(pos);
    Vector dir = l.dir();
    if ((iOrigin - seg.iP).len() > (iOrigin - seg.iQ).len())
      dir = -dir;
    iDir = dir.angle();
    return true;
  } else if (!segs.iArcs.empty()) {
    Arc arc = segs.iArcs.back();
    Angle alpha;
    (void) arc.distance(pos, 3.0, iOrigin, alpha);
    iDir = (arc.iM.linear() * Vector(Angle(alpha + IpeHalfPi))).angle();
    return true;
  } else if (!segs.iBeziers.empty()) {
    Bezier bez = segs.iBeziers.back();
    double t;
    double bound = 2.0;
    if (!bez.snap(pos, t, iOrigin, bound))
      return false;
    iDir = bez.tangent(t).angle();
    return true;
  }

  return false;
}

// --------------------------------------------------------------------
