// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Canvas for HTML, Javascript, and webassembly
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

#ifndef IPECANVAS_JS_H
#define IPECANVAS_JS_H

#include "ipecanvas.h"

#include <emscripten/bind.h>

// --------------------------------------------------------------------

namespace ipe {

  // --------------------------------------------------------------------

  class Canvas : public CanvasBase {
  public:
    Canvas(emscripten::val canvas);

    virtual void setCursor(TCursor cursor, double w = 1.0,
			   Color *color = nullptr);

  protected:
    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);
    void drawFifi();
    void paint();

  protected:
    /*
    virtual void paintEvent(QPaintEvent *ev);
    void mouseButton(QMouseEvent *ev, int button, bool press);
    virtual void mouseDoubleClickEvent(QMouseEvent *ev);
    virtual void mousePressEvent(QMouseEvent *ev) ;
    virtual void mouseReleaseEvent(QMouseEvent *ev);
    virtual void mouseMoveEvent(QMouseEvent *ev);
    virtual void tabletEvent(QTabletEvent *ev);
    virtual void wheelEvent(QWheelEvent *ev);
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual QSize sizeHint() const;
    */
  private:
    emscripten::val iCtx;
  };

} // namespace

// --------------------------------------------------------------------
#endif
