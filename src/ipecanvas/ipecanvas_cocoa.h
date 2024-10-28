// -*- objc -*-
// --------------------------------------------------------------------
// ipe::Canvas for Cocoa
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

#ifndef IPECANVAS_COCOA_H
#define IPECANVAS_COCOA_H

#include "ipecanvas.h"

#import <Cocoa/Cocoa.h>

// --------------------------------------------------------------------

class Canvas;

@interface IpeCanvasView : NSView

@property Canvas * canvas;

@end

// --------------------------------------------------------------------

class Canvas : public ipe::CanvasBase {
public:
    Canvas(IpeCanvasView * view);
    virtual ~Canvas();

    IpeCanvasView * view() { return iView; }

    void button(bool press, NSEvent * event);
    void mouseMove(NSEvent * event);
    bool key(NSEvent * event);
    void scrollWheel(NSEvent * event);
    void magnify(NSEvent * event);

    void drawRect(NSRect rect);

private:
    // void refreshLayer();
    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);
    virtual void setCursor(TCursor cursor, double w, ipe::Color * color);

private:
    IpeCanvasView * iView;
    // CGLayer *iLayer;
};

// --------------------------------------------------------------------
#endif
