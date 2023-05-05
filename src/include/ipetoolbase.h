// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Tool
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

#ifndef IPETOOLBASE_H
#define IPETOOLBASE_H

#include "ipegeo.h"
#include "ipepainter.h"

namespace ipe {

  class CanvasBase;

  class Tool {
  public:
    virtual ~Tool();

  public:
    virtual void draw(Painter &painter) const = 0;
    // left: 1, right:2, middle: 4,
    // xbutton1: 8, xbutton2: 0x10
    // plus 0x80 for double-click
    virtual void mouseButton(int button, bool press);
    virtual void mouseMove();
    virtual bool key(String text, int modifiers);
    virtual void snapVtx(const Vector &mouse, Vector &pos,
			 double &bound, bool cp) const;
  protected:
    Tool(CanvasBase *canvas);

  protected:
    CanvasBase *iCanvas;
  };
} // namespace

// --------------------------------------------------------------------
#endif
