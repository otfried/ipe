// --------------------------------------------------------------------
// Ipelet for creating regular k-gons
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

#include "ipelet.h"
#include "ipepath.h"
#include "ipepage.h"

using namespace ipe;

// --------------------------------------------------------------------

class KGonIpelet : public Ipelet {
public:
  virtual int ipelibVersion() const { return IPELIB_VERSION; }
  virtual bool run(int, IpeletData *data, IpeletHelper *helper);
};

// --------------------------------------------------------------------

bool KGonIpelet::run(int, IpeletData *data, IpeletHelper *helper)
{
  Page *page = data->iPage;
  int sel = page->primarySelection();
  if (sel < 0) {
    helper->message("No selection");
    return false;
  }
  const Path *p = page->object(sel)->asPath();
  if (p == 0 || p->shape().countSubPaths() != 1 ||
      p->shape().subPath(0)->type() != SubPath::EEllipse) {
    helper->message("Primary selection is not a circle");
    return false;
  }


  String str = helper->getParameter("n");   // get default value from Lua wrapper
  if (!helper->getString("Enter k (number of corners)", str))
    return false;
  int k = Lex(str).getInt();
  if (k < 3 || k > 1000)
    return false;

  const Ellipse *e = p->shape().subPath(0)->asEllipse();
  Matrix m = p->matrix() * e->matrix();

  Vector center = m.translation();
  Vector v = m * Vector(1,0);
  double radius = (v - center).len();

  Curve *sp = new Curve;
  double alpha = 2.0 * IpePi / k;
  Vector v0 = center + radius * Vector(1,0);
  for (int i = 1; i < k; ++i) {
    Vector v1 = center + radius * Vector(Angle(i * alpha));
    sp->appendSegment(v0, v1);
    v0 = v1;
  }
  sp->setClosed(true);
  Shape shape;
  shape.appendSubPath(sp);
  Path *obj = new Path(data->iAttributes, shape);
  page->append(ESecondarySelected, data->iLayer, obj);
  helper->message("Created regular k-gon");
  return true;
}

// --------------------------------------------------------------------

IPELET_DECLARE Ipelet *newIpelet()
{
  return new KGonIpelet;
}

// --------------------------------------------------------------------
