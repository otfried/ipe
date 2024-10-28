// --------------------------------------------------------------------
// ipe::PdfView for Win
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

#include "ipepdfview_win.h"
#include "ipecairopainter.h"

#include <cairo-win32.h>
#include <cairo.h>

#include <windows.h>
#include <windowsx.h>

using namespace ipe;

// --------------------------------------------------------------------

const wchar_t PdfView::className[] = L"ipePdfViewWindowClass";

// --------------------------------------------------------------------

void PdfView::invalidate() { InvalidateRect(hwnd, nullptr, FALSE); }

void PdfView::invalidate(int x, int y, int w, int h) {
    RECT r;
    r.left = x;
    r.right = x + w;
    r.top = y;
    r.bottom = y + h;
    InvalidateRect(hwnd, &r, FALSE);
}

// --------------------------------------------------------------------

void PdfView::updateSize() {
    RECT rc;
    GetClientRect(hwnd, &rc);
    iBWidth = iWidth = rc.right;
    iBHeight = iHeight = rc.bottom;
}

void PdfView::wndPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (!iWidth) updateSize();
    refreshSurface();

    int x0 = ps.rcPaint.left;
    int y0 = ps.rcPaint.top;
    int w = ps.rcPaint.right - x0;
    int h = ps.rcPaint.bottom - y0;

    // ipeDebug("wndPaint: %d %d", w, h);
    HBITMAP bits = CreateCompatibleBitmap(hdc, w, h);
    HDC bitsDC = CreateCompatibleDC(hdc);
    SelectObject(bitsDC, bits);
    cairo_surface_t * surface = cairo_win32_surface_create(bitsDC);

    cairo_t * cr = cairo_create(surface);
    cairo_translate(cr, -x0, -y0);
    cairo_set_source_surface(cr, iSurface, 0.0, 0.0);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    BitBlt(hdc, x0, y0, w, h, bitsDC, 0, 0, SRCCOPY);
    DeleteDC(bitsDC);
    DeleteObject(bits);
    EndPaint(hwnd, &ps);
}

// --------------------------------------------------------------------

LRESULT CALLBACK PdfView::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PdfView * view = (PdfView *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE: {
	assert(view == nullptr);
	LPCREATESTRUCT p = (LPCREATESTRUCT)lParam;
	view = (PdfView *)p->lpCreateParams;
	view->hwnd = hwnd;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)view);
    } break;
    case WM_COMMAND:
	if (view) PostMessage(view->target, WM_COMMAND, wParam, lParam);
	break;
    case WM_PAINT:
	if (view) view->wndPaint();
	return 0;
    case WM_SIZE:
	if (view) {
	    view->updateSize();
	    PostMessage(view->target, 0x8000, view->screen, 0);
	}
	break;
    case WM_LBUTTONDOWN:
	if (view) PostMessage(view->target, 0x8000, view->screen + 1, lParam);
	break;
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
	if (view) PostMessage(view->target, 0x8000, view->screen + 2, lParam);
	break;
    case WM_CLOSE: return 0; // do NOT allow default procedure to close
    case WM_DESTROY:
	ipeDebug("Windows PdfView is destroyed");
	delete view;
	break;
	// don't show in debug output:
    case WM_SETCURSOR:
    case WM_NCHITTEST: break;
    default:
	// ipeDebug("PdfView wndProc(%d)", message);
	break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// --------------------------------------------------------------------

PdfView::PdfView(HWND parent, HWND target, int screen, HINSTANCE hInstance)
    : target(target)
    , screen(screen) {
    if (hInstance == nullptr)
	hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
    HWND hwnd =
	(parent == nullptr)
	    ? CreateWindowExW(WS_EX_CLIENTEDGE, className, L"", WS_OVERLAPPEDWINDOW,
			      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			      nullptr, nullptr, hInstance, this)
	    : CreateWindowExW(WS_EX_CLIENTEDGE, className, L"", WS_CHILD | WS_VISIBLE, 0,
			      0, 0, 0, parent, nullptr, hInstance, this);
    if (hwnd == nullptr) {
	MessageBoxA(nullptr, "PdfView creation failed!", "Error!",
		    MB_ICONEXCLAMATION | MB_OK);
	exit(9);
    }
    assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void PdfView::init(HINSTANCE hInstance) {
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS; // CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = NULL; // actually child id
    wc.lpszClassName = className;
    wc.hIcon = NULL;
    wc.hIconSm = NULL;

    if (!RegisterClassExW(&wc)) {
	MessageBoxA(nullptr, "PdfView control registration failed!", "Error!",
		    MB_ICONEXCLAMATION | MB_OK);
	exit(9);
    }
}

// --------------------------------------------------------------------
