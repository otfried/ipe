// -*- C++ -*-
// --------------------------------------------------------------------
// Snapping.
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

#ifndef IPESNAP_H
#define IPESNAP_H

#include "ipegeo.h"
#include "ipetoolbase.h"

// --------------------------------------------------------------------

namespace ipe {

  class Page;

  class Snap {
  public:
    //! The different snap modes as bitmasks.
    enum TSnapModes { ESnapNone = 0,
		      ESnapVtx = 1, ESnapCtl = 2,
		      ESnapBd = 4, ESnapInt = 8,
		      ESnapGrid = 0x10, ESnapAngle = 0x20,
		      ESnapAuto = 0x40, ESnapCustom = 0x80 };

    int iSnap;           //!< Activated snapping modes (TSnapModes)
    bool iGridVisible;   //!< Is the grid visible?
    int iGridSize;       //!< Snap grid spacing.
    double iAngleSize;   //!< Angle for angular snapping.
    int iSnapDistance;   //!< Snap distance (in pixels).
    bool iWithAxes;      //!< Show coordinate system?
    Vector iOrigin;      //!< Origin of coordinate system
    Angle iDir;          //!< Direction of x-axis

    void intersectionSnap(const Vector &pos,  Vector &fifi,
			  const Page *page, int view,
			  double &snapDist) const noexcept;
    bool snapAngularIntersection(Vector &pos, const Line &l,
				 const Page *page, int view,
				 double snapDist) const noexcept;
    TSnapModes simpleSnap(Vector &pos, const Page *page, int view,
			  double snapDist, Tool *tool = nullptr) const noexcept;
    TSnapModes snap(Vector &pos, const Page *page, int view,
		    double snapDist, Tool *tool = nullptr,
		    Vector *autoOrg = nullptr) const noexcept;
    Line getLine(const Vector &mouse, const Vector &base) const noexcept;
    bool setEdge(const Vector &pos, const Page *page, int view) noexcept;
  };

} // namespace

// --------------------------------------------------------------------
#endif
