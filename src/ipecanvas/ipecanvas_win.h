// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Canvas for Win32
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

#ifndef IPECANVASWIN_H
#define IPECANVASWIN_H
// --------------------------------------------------------------------

#include "ipecanvas.h"

#include <windows.h>

namespace ipe {

  class Canvas : public CanvasBase {
  public:
    static void init(HINSTANCE hInstance);
    static UINT getDpiForWindow(HWND hwnd);
    Canvas(HWND parent, HINSTANCE hInstance = nullptr);

    HWND windowId() const { return hwnd; }

    static HBITMAP createBitmap(uint8_t *p, int w, int h);

  private:
    void button(int button, bool down, int mod, Vector v);
    void key(WPARAM wParam, LPARAM lParam);
    void mouseMove(Vector v);
    void wndPaint();
    void updateSize();

    Vector location(HWND hwnd, POINT q, POINT h);
    LRESULT handlePointerEvent(HWND hwnd, UINT message,
			       WPARAM wParam, LPARAM lParam);
    LRESULT handlePanGesture(DWORD flags, POINTS &p);
    LRESULT handleZoomGesture(DWORD flags, POINTS &p, ULONG d);

    virtual void invalidate();
    virtual void invalidate(int x, int y, int w, int h);
    virtual void setCursor(TCursor cursor, double w, Color *color);
  private:
    static const wchar_t className[];
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				    WPARAM wParam, LPARAM lParam);
    static void mouseButton(Canvas *canvas, int button, bool down,
			    WPARAM wParam, LPARAM lParam);
  private:
    HWND hwnd;
    // Gesture support
    Vector iGestureStart;
    int iGestureDist;
    Vector iGesturePan;
    double iGestureZoom;
    UINT32 iPointerId;
    Vector iHimetric;
    bool isTransient;
  };

} // namespace

// --------------------------------------------------------------------
#endif
