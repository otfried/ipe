// --------------------------------------------------------------------
// PathView widget for Win32
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

#include "controls_win.h"

#include "ipecairopainter.h"

#include <cairo.h>
#include <cairo-win32.h>

using namespace ipe;

const wchar_t PathView::className[] = L"ipePathViewClass";

// --------------------------------------------------------------------

PathView::PathView(HWND parent, int id)
{
  iCascade = nullptr;
  idBase = id;

  HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, className, L"",
			      WS_CHILD | WS_VISIBLE,
			      0, 0, 0, 0,
			      parent, nullptr,
			      (HINSTANCE) GetWindowLongPtr(parent, GWLP_HINSTANCE),
			      this);
  if (hwnd == nullptr) {
    MessageBoxA(nullptr, "PathView creation failed!", "Error!",
		MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void PathView::setColor(const Color & color)
{
  iColor = color;
  InvalidateRect(hwnd, nullptr, FALSE);
}

void PathView::set(const AllAttributes &all, Cascade *sheet)
{
  iCascade = sheet;
  iAll = all;
  InvalidateRect(hwnd, nullptr, FALSE);
}

LRESULT CALLBACK PathView::wndProc(HWND hwnd, UINT message, WPARAM wParam,
				   LPARAM lParam)
{
  PathView *pv = (PathView *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (message) {
  case WM_CREATE:
    {
      assert(pv == nullptr);
      LPCREATESTRUCT p = (LPCREATESTRUCT) lParam;
      pv = (PathView *) p->lpCreateParams;
      pv->hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pv);
    }
    break;
  case WM_PAINT:
    if (pv)
      pv->wndPaint();
    return 0;
  case WM_LBUTTONDOWN:
    if (pv)
      pv->button(LOWORD(lParam), HIWORD(lParam));
    break;
  case WM_RBUTTONUP:
    if (pv) {
      pv->pos.x = LOWORD(lParam);
      pv->pos.y = HIWORD(lParam);
      ClientToScreen(pv->hwnd, (LPPOINT) &pv->pos);
      SendMessage(GetParent(hwnd), WM_COMMAND, pv->idBase, (LPARAM) hwnd);
    }
    break;
  case WM_DESTROY:
    delete pv;
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

void PathView::wndPaint()
{
  InvalidateRect(hwnd, nullptr, FALSE);

  RECT rc;
  GetClientRect(hwnd, &rc);
  int w = rc.right;
  int h = rc.bottom;

  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);
  cairo_surface_t *surface = cairo_win32_surface_create(hdc);
  cairo_t *cc = cairo_create(surface);

  cairo_set_source_rgb(cc, iColor.iRed.toDouble(),
		       iColor.iGreen.toDouble(),
		       iColor.iBlue.toDouble());
  cairo_rectangle(cc, 0, 0, w, h);
  cairo_fill(cc);

  if (iCascade) {
    cairo_translate(cc, 0, h);
    double zoom = w / 100.0;
    cairo_scale(cc, zoom, -zoom);
    Vector v0 = (1.0/zoom) * Vector(0.1 * w, 0.5 * h);
    Vector v1 = (1.0/zoom) * Vector(0.7 * w, 0.5 * h);
    Vector u1 = (1.0/zoom) * Vector(0.88 * w, 0.8 * h);
    Vector u2 = (1.0/zoom) * Vector(0.80 * w, 0.5 * h);
    Vector u3 = (1.0/zoom) * Vector(0.88 * w, 0.2 * h);
    Vector u4 = (1.0/zoom) * Vector(0.96 * w, 0.5 * h);
    Vector mid = 0.5 * (v0 + v1);
    Vector vf = iAll.iFArrowShape.isMidArrow() ? mid : v1;
    Vector vr = iAll.iRArrowShape.isMidArrow() ? mid : v0;

    CairoPainter painter(iCascade, nullptr, cc, 3.0, false, false);
    painter.setPen(iAll.iPen);
    painter.setDashStyle(iAll.iDashStyle);
    painter.setStroke(iAll.iStroke);
    painter.setFill(iAll.iFill);
    painter.pushMatrix();
    painter.newPath();
    painter.moveTo(v0);
    painter.lineTo(v1);
    painter.drawPath(EStrokedOnly);
    if (iAll.iFArrow)
      Path::drawArrow(painter, vf, Angle(0),
		      iAll.iFArrowShape, iAll.iFArrowSize,  80.0);
    if (iAll.iRArrow)
      Path::drawArrow(painter, vr, Angle(IpePi),
		      iAll.iRArrowShape, iAll.iRArrowSize, -80.0);
    painter.setDashStyle(Attribute::NORMAL());
    painter.setTiling(iAll.iTiling);
    painter.newPath();
    painter.moveTo(u1);
    painter.lineTo(u2);
    painter.lineTo(u3);
    painter.lineTo(u4);
    painter.closePath();
    painter.drawPath(iAll.iPathMode);
    painter.popMatrix();
  }

  cairo_surface_flush(surface);
  cairo_destroy(cc);
  cairo_surface_destroy(surface);
  EndPaint(hwnd, &ps);
}

void PathView::button(int x, int y)
{
  RECT rc;
  GetClientRect(hwnd, &rc);
  int w = rc.right;

  if (x < w * 3 / 10) {
    iAction = iAll.iRArrow ? "rarrow|false" : "rarrow|true";
    SendMessage(GetParent(hwnd), WM_COMMAND, idBase+1, (LPARAM) hwnd);
  } else if (x > w * 4 / 10 && x < w * 72 / 100) {
    iAction = iAll.iFArrow ? "farrow|false" : "farrow|true";
    SendMessage(GetParent(hwnd), WM_COMMAND, idBase+1, (LPARAM) hwnd);
  } else if (x > w * 78 / 100) {
    switch (iAll.iPathMode) {
    case EStrokedOnly:
      iAction = "pathmode|strokedfilled";
      break;
    case EStrokedAndFilled:
      iAction = "pathmode|filled";
      break;
    case EFilledOnly:
      iAction = "pathmode|stroked";
      break;
    }
    SendMessage(GetParent(hwnd), WM_COMMAND, idBase+1, (LPARAM) hwnd);
  }
  // show new status
  InvalidateRect(hwnd, nullptr, FALSE);
}

void PathView::init(HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0; // CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra	 = 0;
  wc.cbWndExtra	 = 0;
  wc.hInstance	 = hInstance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
  wc.lpszMenuName = nullptr;  // actually child id
  wc.lpszClassName = className;
  wc.hIcon = nullptr;
  wc.hIconSm = nullptr;

  if (!RegisterClassEx(&wc)) {
    MessageBoxA(nullptr, "PathView control registration failed!", "Error!",
		MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
}

// --------------------------------------------------------------------
