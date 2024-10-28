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

#include <emscripten/val.h>

// --------------------------------------------------------------------

namespace ipe {

// --------------------------------------------------------------------

class JsPainter;

class Canvas : public CanvasBase {
public:
    Canvas(emscripten::val bottomCanvas, emscripten::val topCanvas);

    virtual void setCursor(TCursor cursor, double w = 1.0, Color * color = nullptr);

    void mouseButtonEvent(emscripten::val event, int button, bool press);
    void mouseMoveEvent(emscripten::val ev);
    void wheelEvent(emscripten::val ev);
    bool keyPressEvent(emscripten::val ev);

    void paint();
    void updateSize();

protected:
    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);
    void drawFifi(JsPainter & qp);

private:
    emscripten::val iPaintScheduler;
    emscripten::val iBottomCanvas;
    emscripten::val iTopCanvas;
    emscripten::val iBottomCtx;
    emscripten::val iTopCtx;
    bool iNeedPaint;
    double iDpr;
};

} // namespace ipe

// --------------------------------------------------------------------
#endif
