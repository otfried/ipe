// --------------------------------------------------------------------
// Ipelet for computing various Voronoi diagrams
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

#include <stdio.h>
#include <cstdlib>

extern "C" {
#include "qhull_ra.h"
}

#include "ipelib.h"

using namespace ipe;

// some slack when deciding if facets are vertical,
// to handle degenerate situations
static constexpr double eps = 1e-5;

// --------------------------------------------------------------------

struct DelaunayEdge {
public:
  DelaunayEdge(int x, int y) : a(x), b(y) { /* nothing */ }
  DelaunayEdge() : a(-1), b(-1) { /* nothing */ }
public:
  int a,b;
};

inline bool operator<(const DelaunayEdge& x, const DelaunayEdge& y)
{
  return (x.a > y.a || (x.a == y.a && x.b > y.b));
}

inline bool operator!=(const DelaunayEdge& x, const DelaunayEdge& y)
{
  return (x.a != y.a || x.b != y.b);
}

// --------------------------------------------------------------------

class VoronoiIpelet : public Ipelet {
public:
  VoronoiIpelet();
  virtual int ipelibVersion() const { return IPELIB_VERSION; }
  virtual bool run(int function, IpeletData *data, IpeletHelper *helper);
private:
  void addVoronoiEdge(facetT *facet, facetT *neighbor);
  void addInfiniteEdge(facetT *facet, facetT *neighbor);
  void voronoiTreatFacet(qhT *qh, facetT *facet);
  void addDelaunayEdge(int from, int to);
  void delaunayTreatFacet(qhT *qh, facetT *facet);
private:
  int iVoronoiSign;
  std::vector<Vector> iSites;
  std::vector<Segment> iEdges;
  std::vector<DelaunayEdge> iDelaunay;
  double iInfiniteEdgeLength;
};

VoronoiIpelet::VoronoiIpelet()
{
  iInfiniteEdgeLength = 100.0;
}

// --------------------------------------------------------------------

class CollectVisitor : public Visitor {
public:
  CollectVisitor(std::vector<Vector> &sites);
  virtual void visitGroup(const Group *obj);
  virtual void visitPath(const Path *obj);
  virtual void visitReference(const Reference *obj);
private:
  std::vector<Vector> &iSites;
  std::list<Matrix> iStack;
};

CollectVisitor::CollectVisitor(std::vector<Vector> &sites)
  : iSites(sites)
{
  iStack.push_back(Matrix()); // id matrix
}

void CollectVisitor::visitGroup(const Group *obj)
{
  iStack.push_back(iStack.back() * obj->matrix());
  for (Group::const_iterator it = obj->begin(); it != obj->end(); ++it)
    (*it)->accept(*this);
  iStack.pop_back();
}

void CollectVisitor::visitPath(const Path *obj)
{
  Matrix m = iStack.back() * obj->matrix();
  Shape shape = obj->shape();
  for (int i = 0; i < shape.countSubPaths(); ++i) {
    const Curve *curve = shape.subPath(i)->asCurve();
    if (curve) {
      iSites.push_back(m * curve->segment(0).cp(0));
      for (int j = 0; j < curve->countSegments(); ++j)
	iSites.push_back(m * curve->segment(j).last());
    }
  }
}

void CollectVisitor::visitReference(const Reference *obj)
{
  String s = obj->name().string();
  if (s.left(5) == "mark/") {
    iSites.push_back(iStack.back() * obj->matrix() * obj->position());
  }
}

// --------------------------------------------------------------------

//
// readpoints put points into structure for qhull
//
// returns:
//  number of points, array of point coordinates
//

static coordT *readpoints(qhT *qh,
			  const std::vector<Vector> &sites, int mode,
			  int *numpoints)
{
  coordT *points, *coords;

  if (mode < 2 || mode > 3)
    *numpoints= sites.size();
  else if (mode == 2)
    *numpoints = sites.size() * (sites.size() - 1) / 2;
  else if (mode == 3)
    *numpoints = sites.size() * (sites.size() - 1) * (sites.size() - 2) / 6;

  qh->normal_size = 3 * sizeof(coordT); /* for tracing with qh_printpoint */
  coords = points = new coordT[*numpoints * 3];

  if (mode < 2 || mode == 4) {
    for (const auto & site : sites) {
      *(coords++)= site.x;
      *(coords++)= site.y;
      *(coords++)= site.x * site.x + site.y * site.y;
    }
  } else if (mode == 2) {
    for (int i = 0; i < size(sites) - 1; i++) {
      for (int j = i + 1; j < size(sites); j++) {
	*(coords++)= (sites[i].x + sites[j].x) / 2.0;
	*(coords++)= (sites[i].y + sites[j].y) / 2.0;
	*(coords++)= (sites[i].x * sites[i].x + sites[i].y * sites[i].y
	  + sites[j].x * sites[j].x + sites[j].y * sites[j].y) / 2.0;
      }
    }
  } else if (mode == 3) {
    for (int i = 0; i < size(sites) - 2; i++) {
      for (int j = i + 1; j < size(sites) - 1; j++) {
	for (int k = j + 1; k < size(sites); k++) {
	  *(coords++)= (sites[i].x + sites[j].x + sites[k].x) / 3.0;
	  *(coords++)= (sites[i].y + sites[j].y + sites[k].y) / 3.0;
	  *(coords++)=
	    (sites[i].x * sites[i].x + sites[i].y * sites[i].y
	     + sites[j].x * sites[j].x + sites[j].y * sites[j].y
	     + sites[k].x * sites[k].x + sites[k].y * sites[k].y)
	    / 3.0;
	}
      }
    }
  } else if (mode == 5) {
#if 0
    // for medial axis
    int j = size(sites) - 1;
    pl_unitvec d;
    for (int i = 0; i < size(sites); i++) {
      d = normalized(sites[i] - sites[j]).normal();
      // facet has equation (d.x, d.y, 1) * (x,y,z) = c = dot(d, sites[i]);
      // dualize it to point (-d.x/2, -d.y/2, -c)
      *(coords++)= -d.x / 2.0;
      *(coords++)= -d.y / 2.0;
      *(coords++)= -dot(d, sites[i]);
      j = i;
    }
#endif
  }
  return points;
}

// --------------------------------------------------------------------

// returns Voronoi vertex dual to facet
inline Vector voronoi_vertex(facetT *facet)
{
  return Vector(-0.5 * facet->normal[0]/facet->normal[2],
		-0.5 * facet->normal[1]/facet->normal[2]);
}

void VoronoiIpelet::addVoronoiEdge(facetT *facet, facetT *neighbor)
{
  if (facet->id < neighbor->id) {
    iEdges.push_back(Segment(voronoi_vertex(facet), voronoi_vertex(neighbor)));
  }
}

void VoronoiIpelet::addInfiniteEdge(facetT *facet, facetT *neighbor)
{
  Vector dir;
  Vector v = voronoi_vertex(facet);

  if (fabs(neighbor->normal[2]) < eps) {
    // neighboring facet is vertical
    dir = Vector(neighbor->normal[0], neighbor->normal[1]);
  } else {
    dir = v - voronoi_vertex(neighbor);
  }
  dir = dir.normalized();
  iEdges.push_back(Segment(v, v + iInfiniteEdgeLength * dir));
}

void VoronoiIpelet::voronoiTreatFacet(qhT *qh, facetT *facet)
{
  facetT *neighbor, **neighborp;

  if (!facet) return;
  if (qh_skipfacet(qh, facet)) return;
  if (facet == qh_MERGEridge) return;
  if (facet == qh_DUPLICATEridge) return;

  if (iVoronoiSign * facet->normal[2] >= -eps) return;

  FOREACHneighbor_(facet) {
    if (neighbor != qh_MERGEridge && neighbor != qh_DUPLICATEridge) {
      if (iVoronoiSign * neighbor->normal[2] < -eps) {
	// make Voronoi edge between the two facets
	addVoronoiEdge(facet, neighbor);
      } else {
	addInfiniteEdge(facet, neighbor);
      }
    }
  }
}

// --------------------------------------------------------------------

void VoronoiIpelet::addDelaunayEdge(int from, int to)
{
  if (from < to)
    iDelaunay.push_back(DelaunayEdge(to, from));
  else
    iDelaunay.push_back(DelaunayEdge(from, to));
}

void VoronoiIpelet::delaunayTreatFacet(qhT *qh, facetT *facet)
{
  setT *vertices;
  vertexT *vertex, **vertexp;

  if (!facet) return;
  if (qh_skipfacet(qh, facet)) return;
  if (facet == qh_MERGEridge) return;
  if (facet == qh_DUPLICATEridge) return;

  if (facet->normal[2] >= 0.0) return;

  vertices= qh_facet3vertex(qh, facet);
  int id, first_id = -1, last_id = -1;
  FOREACHvertex_(vertices) {
    id = qh_pointid(qh, vertex->point);
    if (last_id >= 0) {
      addDelaunayEdge(last_id, id);
      last_id = id;
    } else {
      last_id = first_id = id;
    }
  }
  addDelaunayEdge(last_id, first_id);
  qh_settempfree(qh, &vertices);
}

// --------------------------------------------------------------------

#if 0
    // for medial axis: single convex polygon only
    for (Object *ob = ium_input; ob; ob = ob->next) {
      if (ob->type == IPE_LINE) {
	if (size(sites) > 0 || !ob->w.line->closed) {
	  ium_message = "can handle single convex polygon only";
	  ium_end();
	}
	// check whether the polygon is really convex
	pl_polygon pgn(ob->w.line->v);
	if (!pgn.is_convex()) {
	  ium_message = "can handle single convex polygon only";
	  ium_end();
	}
	if (pgn.is_clockwise())
	  pgn.invert_orientation();
	sites = pgn.all_vertices();
      } else if (ob->type != IPE_TEXT) {
	ium_message = "can handle single convex polygon only";
	ium_end();
      }
    }
#endif

bool VoronoiIpelet::run(int function, IpeletData *data, IpeletHelper *helper)
{
  ipeDebug("VoronoiIpelet::run(%d)", function);
  if (function == 5) {
    char buf[32];
    std::sprintf(buf, "%g", iInfiniteEdgeLength);
    String el(buf);
    if (helper->getString("Length of infinite edges (in points):", el))
      iInfiniteEdgeLength = std::strtod(el.z(), nullptr);
    return false;
  }
  iVoronoiSign = (function == 4) ? -1 : 1;

  iSites.clear();
  CollectVisitor vis(iSites);

  for (int it = 0; it < data->iPage->count(); ++it) {
    if (data->iPage->select(it))
      data->iPage->object(it)->accept(vis);
  }
  if (iSites.size() < 4) {
    helper->messageBox("You need to select at least four sites", nullptr, 0);
    return false;
  }

  qhT qh_qh;                // Qhull's data structure.
  qhT *qh= &qh_qh;

  QHULL_LIB_CHECK           // Check for compatible library

  ipeDebug("qh_meminit");
  qh_meminit(qh, stderr);
  qh_initqhull_start(qh, stdin, stdout, stderr);

  int numpoints = 0;
  coordT *points = nullptr;

  if (!setjmp(qh->errexit)) {

    points = readpoints(qh, iSites, function, &numpoints);
    qh_initqhull_globals(qh, points, numpoints, 3, False);
    ipeDebug("qh_initqhull_mem()");
    qh_initqhull_mem(qh);
    /* mem.c and set.c are initialized */
    qh_initqhull_buffers(qh);
    qh_initthresholds(qh, qh->qhull_command);
    if (qh->SCALEinput)
      qh_scaleinput(qh);
    if (qh->ROTATErandom >= 0) {
      qh_randommatrix(qh, qh->gm_matrix, qh->hull_dim, qh->gm_row);
      qh_gram_schmidt(qh, qh->hull_dim, qh->gm_row);
      qh_rotateinput(qh, qh->gm_row);
    }
    qh_qhull(qh);
    qh_check_output(qh);
#ifndef WIN32
    // this crashes on Windows.  Why?
    ipeDebug("qh_produce_output()");
    qh_produce_output(qh);
#endif
    if (qh->VERIFYoutput && !qh->FORCEoutput && !qh->STOPpoint && !qh->STOPcone)
      qh_check_points(qh);

    // now create segments for Ipe

    facetT *facet, *facetlist = qh->facet_list;

    FORALLfacet_(facetlist) {
      if (!function)
	delaunayTreatFacet(qh, facet);
      else
	voronoiTreatFacet(qh, facet);
    }

    Group *group = new Group;

    if (!function) {
      std::sort(iDelaunay.begin(), iDelaunay.end());
      for (int j = 0; j < size(iDelaunay); j++) {
	if (!j || iDelaunay[j] != iDelaunay[j-1]) {
 	  Vector a(points[3*iDelaunay[j].a], points[3*iDelaunay[j].a + 1]);
 	  Vector b(points[3*iDelaunay[j].b], points[3*iDelaunay[j].b + 1]);
	  group->push_back(new Path(data->iAttributes, Shape(Segment(a, b))));
	}
      }
    } else {
      for (const auto & edge : iEdges)
	group->push_back(new Path(data->iAttributes, Shape(edge)));
    }
    data->iPage->append(ESecondarySelected, data->iLayer, group);
  }
  ipeDebug("qh_freehull(True)");
  qh_freeqhull(qh, True);

  delete [] points;

  iEdges.clear();
  iSites.clear();
  iDelaunay.clear();

  return true;
}

// --------------------------------------------------------------------

IPELET_DECLARE Ipelet *newIpelet()
{
  return new VoronoiIpelet;
}

// --------------------------------------------------------------------
