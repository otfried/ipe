// --------------------------------------------------------------------
// ipe::Canvas for Win
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

// for touch gestures:  need at least Windows 7
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include "ipecanvas_win.h"
#include "ipecairopainter.h"
#include "ipetool.h"

#include <cairo.h>
#include <cairo-win32.h>

#include <windows.h>
#include <windowsx.h>

using namespace ipe;

// --------------------------------------------------------------------
// some definitions from Windows 8

#define WM_POINTERUPDATE 0x245 // 581
#define WM_POINTERDOWN 0x246   // 582
#define WM_POINTERUP 0x247     // 583
#define WM_POINTERCAPTURECHANGED 0x24c

typedef enum {
  PT_POINTER   = 0x00000001,
  PT_TOUCH     = 0x00000002,
  PT_PEN       = 0x00000003,
  PT_MOUSE     = 0x00000004,
  PT_TOUCHPAD  = 0x00000005
} POINTER_INPUT_TYPE;

typedef struct {
  POINTER_INPUT_TYPE         pointerType;
  UINT32                     pointerId;
  UINT32                     frameId;
  // POINTER_FLAGS              pointerFlags;
  UINT32                     pointerFlags;
  HANDLE                     sourceDevice;
  HWND                       hwndTarget;
  POINT                      ptPixelLocation;
  POINT                      ptHimetricLocation;
  POINT                      ptPixelLocationRaw;
  POINT                      ptHimetricLocationRaw;
  DWORD                      dwTime;
  UINT32                     historyCount;
  INT32                      inputData;
  DWORD                      dwKeyStates;
  UINT64                     PerformanceCount;
  // POINTER_BUTTON_CHANGE_TYPE ButtonChangeType;
  UINT32                     ButtonChangeType;
} POINTER_INFO;

typedef struct {
  POINTER_INFO pointerInfo;
  UINT32       penFlags;
  UINT32       penMask;
  UINT32       pressure;
  UINT32       rotation;
  INT32        tiltX;
  INT32        tiltY;
} POINTER_PEN_INFO;

// --------------------------------------------------------------------

typedef WINBOOL WINAPI (*LPSetGestureConfig)(HWND hwnd, DWORD dwReserved,
					     UINT cIDs,
					     PGESTURECONFIG pGestureConfig,
					     UINT cbSize);
static LPSetGestureConfig pSetGestureConfig = nullptr;

typedef WINBOOL WINAPI (*LPGetGestureInfo)(HGESTUREINFO hGestureInfo,
					   PGESTUREINFO pGestureInfo);
static LPGetGestureInfo pGetGestureInfo = nullptr;

typedef WINBOOL WINAPI (*LPCloseGestureInfoHandle)(HGESTUREINFO hGestureInfo);
static LPCloseGestureInfoHandle pCloseGestureInfoHandle = nullptr;

typedef WINBOOL WINAPI (*LPGetPointerType)(UINT32 pointerId,
					   POINTER_INPUT_TYPE *pointerType);
static LPGetPointerType pGetPointerType = nullptr;

typedef WINBOOL WINAPI (*LPGetPointerPenInfo)(UINT32 pointerId,
					      POINTER_PEN_INFO *penInfo);
static LPGetPointerPenInfo pGetPointerPenInfo = nullptr;

typedef WINBOOL WINAPI (*LPGetPointerPenInfoHistory)(UINT32 pointerId,
						     UINT32 *entriesCount,
						     POINTER_PEN_INFO *penInfo);
static LPGetPointerPenInfoHistory pGetPointerPenInfoHistory = nullptr;

typedef UINT WINAPI (*LPGetDpiForWindow)(HWND hwnd);
static LPGetDpiForWindow pGetDpiForWindow = nullptr;

// --------------------------------------------------------------------

const wchar_t Canvas::className[] = L"ipeCanvasWindowClass";

// --------------------------------------------------------------------

void Canvas::invalidate()
{
  InvalidateRect(hwnd, nullptr, FALSE);
}

void Canvas::invalidate(int x, int y, int w, int h)
{
  RECT r;
  r.left = x;
  r.right = x + w;
  r.top = y;
  r.bottom = y + h;
  // ipeDebug("invalidate %d %d %d %d\n", r.left, r.right, r.top, r.bottom);
  InvalidateRect(hwnd, &r, FALSE);
  /*
  if (immediate && !isTransient)
    UpdateWindow(hwnd); // force immediate update
  */
}

// --------------------------------------------------------------------

void Canvas::key(WPARAM wParam, LPARAM lParam)
{
  int mod = 0;
  if (GetKeyState(VK_SHIFT) < 0)
    mod |= EShift;
  if (GetKeyState(VK_CONTROL) < 0)
    mod |= EControl;
  if (GetKeyState(VK_MENU) < 0)
    mod |= EAlt;
  // ipeDebug("Canvas::key(%x, %x) %x", wParam, lParam, mod);
  String t;
  t.append(char(wParam));
  iTool->key(t, mod);
}

void Canvas::button(int button, bool down, int mod, Vector v)
{
  POINT p;
  p.x = int(v.x);
  p.y = int(v.y);
  ClientToScreen(hwnd, &p);
  iGlobalPos = Vector(p.x, p.y);
  computeFifi(v.x, v.y);
  mod |= iAdditionalModifiers;
  if (iTool)
    iTool->mouseButton(button | mod, down);
  else if (down && iObserver)
    iObserver->canvasObserverMouseAction(button | mod);
}

void Canvas::mouseMove(Vector v)
{
  // ipeDebug("Canvas::mouseMove %d %d", xPos, yPos);
  computeFifi(v.x, v.y);
  if (iTool)
    iTool->mouseMove();
  if (iObserver)
    iObserver->canvasObserverPositionChanged();
}

void Canvas::setCursor(TCursor cursor, double w, Color *color)
{
  if (cursor == EDotCursor) {
    // Windows switches to a dot automatically when using the pen,
    // not implemented
  } else {
    // HandCursor, CrossCursor not implemented,
    // but they are also not used in Ipe now.
  }
}

// --------------------------------------------------------------------

void Canvas::updateSize()
{
  RECT rc;
  GetClientRect(hwnd, &rc);
  iBWidth = iWidth = rc.right;
  iBHeight = iHeight = rc.bottom;
}

void Canvas::wndPaint()
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);

  if (!iWidth)
    updateSize();
  refreshSurface();

  int x0 = ps.rcPaint.left;
  int y0 = ps.rcPaint.top;
  int w = ps.rcPaint.right - x0;
  int h = ps.rcPaint.bottom - y0;

  // ipeDebug("wndPaint: %d %d", w, h);
  HBITMAP bits = CreateCompatibleBitmap(hdc, w, h);
  HDC bitsDC = CreateCompatibleDC(hdc);
  SelectObject(bitsDC, bits);
  cairo_surface_t *surface = cairo_win32_surface_create(bitsDC);

  cairo_t *cr = cairo_create(surface);
  cairo_translate(cr, -x0, -y0);
  cairo_set_source_surface(cr, iSurface, 0.0, 0.0);
  cairo_paint(cr);

  if (iFifiVisible)
    drawFifi(cr);

  if (iPage) {
    CairoPainter cp(iCascade, iFonts.get(), cr, iZoom, false);
    cp.transform(canvasTfm());
    cp.pushMatrix();
    drawTool(cp);
    cp.popMatrix();
  }
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  BitBlt(hdc, x0, y0, w, h, bitsDC, 0, 0, SRCCOPY);
  DeleteDC(bitsDC);
  DeleteObject(bits);
  EndPaint(hwnd, &ps);
}

// --------------------------------------------------------------------

void Canvas::mouseButton(Canvas *canvas, int button, bool down,
			 WPARAM wParam, LPARAM lParam)
{
  if (canvas) {
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    int mod = 0;
    if (wParam & MK_SHIFT)
      mod |= EShift;
    if (wParam & MK_CONTROL)
      mod |= EControl;
    if (GetKeyState(VK_MENU) < 0)
      mod |= EAlt;
    canvas->button(button, down, mod, Vector(xPos, yPos));
  }
}

Vector Canvas::location(HWND hwnd, POINT q, POINT h)
{
  POINT p = q;
  ScreenToClient(hwnd, &p);
  return Vector(h.x / iHimetric.x + p.x - q.x,
		h.y / iHimetric.y + p.y - q.y);
}

LRESULT Canvas::handlePointerEvent(HWND hwnd, UINT message,
				   WPARAM wParam, LPARAM lParam)
{
  if (message == WM_POINTERCAPTURECHANGED)
    return 0; // ignore and stop gesture processing
  UINT32 pointerId = LOWORD(wParam);
  POINTER_INPUT_TYPE pointerType;
  if (pGetPointerType && pGetPointerType(pointerId, &pointerType)
      && pointerType == PT_PEN) {
    POINTER_PEN_INFO penInfo;
    if (pGetPointerPenInfo(pointerId, &penInfo)) {
      // bool incontact = penInfo.pointerInfo.pointerFlags & 0x4;
      bool firstButton = penInfo.pointerInfo.pointerFlags & 0x10;
      bool secondButton = penInfo.pointerInfo.pointerFlags & 0x20;
      int bt = firstButton ? 1 : (secondButton ? 2: 0);
      if (iHimetric.x == 0.0 || pointerId != iPointerId) {
	iPointerId = pointerId;
	iHimetric = Vector(double(penInfo.pointerInfo.ptHimetricLocationRaw.x)
			   / penInfo.pointerInfo.ptPixelLocationRaw.x,
			   double(penInfo.pointerInfo.ptHimetricLocationRaw.y)
			   / penInfo.pointerInfo.ptPixelLocationRaw.y);
	ipeDebug("pointer hi %g", iHimetric.x);
      }
      Vector v = location(hwnd, penInfo.pointerInfo.ptPixelLocationRaw,
			  penInfo.pointerInfo.ptHimetricLocationRaw);
      UINT32 entriesCount = penInfo.pointerInfo.historyCount;
      if (message == WM_POINTERDOWN) {
	button(bt, true, 0, v);
      } else if (message == WM_POINTERUP) {
	button(bt, false, 0, v);
      } else {
	if (entriesCount > 1) {
	  POINTER_PEN_INFO *pi = new POINTER_PEN_INFO[entriesCount];
	  if (pGetPointerPenInfoHistory(pointerId, &entriesCount, pi)) {
	    for (int i = entriesCount - 1; i >= 0; --i) {
	      isTransient = (i > 0);
	      mouseMove(location(hwnd, pi[i].pointerInfo.ptPixelLocationRaw,
				 pi[i].pointerInfo.ptHimetricLocationRaw));
	    }
	  }
	  delete [] pi;
	} else
	  mouseMove(v);
      }
      return 0;
    }
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT Canvas::handlePanGesture(DWORD flags, POINTS &p)
{
  Vector v(p.x, -p.y);
  if (flags & GF_BEGIN) {
    iGestureStart = v;
    iGesturePan = pan();
  } else {
    v = v - iGestureStart;
    setPan(iGesturePan - (1.0/zoom()) * v);
    update();
  }
  return 0;
}

LRESULT Canvas::handleZoomGesture(DWORD flags, POINTS &p, ULONG d)
{
  if (flags & GF_BEGIN) {
    iGestureDist = d;
    iGestureZoom = zoom();
  } else {
    POINT q = { p.x, p.y };
    ScreenToClient(hwnd, &q);
    Vector origin = devToUser(Vector(q.x, q.y));
    Vector offset = iZoom * (pan() - origin);
    double nzoom = iGestureZoom * d / iGestureDist;
    setZoom(nzoom);
    setPan(origin + (1.0/nzoom) * offset);
    update();
    if (iObserver)
      // scroll wheel hasn't moved, but update display of ppi
      iObserver->canvasObserverWheelMoved(0, 0, 0);
  }
  return 0;
}

LRESULT CALLBACK Canvas::wndProc(HWND hwnd, UINT message, WPARAM wParam,
				 LPARAM lParam)
{
  Canvas *canvas = (Canvas *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (message) {
  case WM_CREATE:
    {
      assert(canvas == nullptr);
      LPCREATESTRUCT p = (LPCREATESTRUCT) lParam;
      canvas = (Canvas *) p->lpCreateParams;
      canvas->hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) canvas);
    }
    break;
  case WM_MOUSEACTIVATE:
    if (canvas)
      SetFocus(canvas->hwnd);
    return MA_ACTIVATE;
  case WM_KEYDOWN:
    if (lParam & 0x1000000) {
      // ipeDebug("Extended key: %d", wParam);
      if (canvas && canvas->iTool && wParam == VK_DELETE) {
	canvas->key(0x7f, lParam);
	return 0;
      }
    }
    break;
  case WM_CHAR:
    // ipeDebug("WM_CHAR: %d", wParam);
    if (canvas && canvas->iTool) {
      canvas->key(wParam, lParam);
      return 0;
    }
    break;
  case WM_MOUSEWHEEL:
    // ipeDebug("Mouse wheel %lx %lx", wParam, lParam);
    if (canvas && canvas->iObserver)
      canvas->iObserver->canvasObserverWheelMoved
	(0, GET_WHEEL_DELTA_WPARAM(wParam) / 8.0,
	 ((wParam & MK_CONTROL) ? 2 : 0));
    return 0;
  case WM_MOUSEHWHEEL:
    // ipeDebug("Mouse horizontal wheel %lx %lx", wParam, lParam);
    if (canvas && canvas->iObserver)
      canvas->iObserver->canvasObserverWheelMoved
	(GET_WHEEL_DELTA_WPARAM(wParam) / 8.0, 0, 0);
    break;
  case WM_PAINT:
    if (canvas)
      canvas->wndPaint();
    return 0;
  case WM_SIZE:
    if (canvas)
      canvas->updateSize();
    break;
  case WM_GESTURE: {
    if (!pGetGestureInfo || !canvas)
      break;  // GetGestureInfo not available (Windows Vista or older)
    GESTUREINFO gi = {0};
    gi.cbSize = sizeof(gi);
    BOOL res = pGetGestureInfo((HGESTUREINFO) lParam, &gi);
    if (!res)
      break;
    pCloseGestureInfoHandle((HGESTUREINFO) lParam);
    if (gi.dwID == GID_PAN)
      return canvas->handlePanGesture(gi.dwFlags, gi.ptsLocation);
    if (gi.dwID == GID_ZOOM)
      return canvas->handleZoomGesture(gi.dwFlags, gi.ptsLocation,
				       gi.ullArguments);
    break; }
  case WM_MOUSEMOVE:
    if (canvas) {
      int xPos = GET_X_LPARAM(lParam);
      int yPos = GET_Y_LPARAM(lParam);
      canvas->mouseMove(Vector(xPos, yPos));
    }
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
    // ignore touch events here
    // ipeDebug("extra = %x", GetMessageExtraInfo());
    if ((GetMessageExtraInfo() & 0xffffff80) != 0xff515780)
      mouseButton(canvas, 1, message == WM_LBUTTONDOWN, wParam, lParam);
    break;
  case WM_LBUTTONDBLCLK:
    mouseButton(canvas, 0x81, true, wParam, lParam);
    break;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
    mouseButton(canvas, 2, message == WM_RBUTTONDOWN, wParam, lParam);
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
    mouseButton(canvas, 4, message == WM_MBUTTONDOWN, wParam, lParam);
    break;
  case WM_XBUTTONDOWN:
  case WM_XBUTTONUP: {
    int but = (HIWORD(wParam) == XBUTTON2) ? 0x10 : 0x08;
    mouseButton(canvas, but, message == WM_XBUTTONDOWN, wParam, lParam);
    break; }
  case WM_GETMINMAXINFO: {
    LPMINMAXINFO mm = LPMINMAXINFO(lParam);
    ipeDebug("Canvas MINMAX %ld %ld", mm->ptMinTrackSize.x,
	     mm->ptMinTrackSize.y);
    break; }
  case WM_POINTERDOWN:
  case WM_POINTERUP:
  case WM_POINTERUPDATE:
  case WM_POINTERCAPTURECHANGED:
    if (canvas && canvas->isInkMode)
      return canvas->handlePointerEvent(hwnd, message, wParam, lParam);
    break;
  case WM_DESTROY:
    ipeDebug("Windows canvas is destroyed");
    delete canvas;
    break;
    // don't show in debug output:
  case WM_SETCURSOR:
  case WM_NCHITTEST:
    break;
  default:
    // ipeDebug("Canvas wndProc(%d)", message);
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

// --------------------------------------------------------------------

Canvas::Canvas(HWND parent, HINSTANCE hInstance) : iHimetric(0., 0.)
{
  if (hInstance == nullptr)
    hInstance = (HINSTANCE) GetWindowLongPtr(parent, GWLP_HINSTANCE);
  HWND hwnd = (parent == nullptr) ?
    CreateWindowExW(WS_EX_CLIENTEDGE, className, L"", WS_OVERLAPPEDWINDOW,
		    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		    nullptr, nullptr, hInstance, this) :
    CreateWindowExW(WS_EX_CLIENTEDGE, className, L"", WS_CHILD | WS_VISIBLE,
		    0, 0, 0, 0, parent, nullptr, hInstance, this);
  if (hwnd == nullptr) {
    MessageBoxA(nullptr, "Canvas creation failed!", "Error!",
		MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (pSetGestureConfig) {
    DWORD panWant = GC_PAN
      | GC_PAN_WITH_SINGLE_FINGER_VERTICALLY
      | GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY
      | GC_PAN_WITH_GUTTER
      | GC_PAN_WITH_INERTIA;
    DWORD panBlock = 0;
    GESTURECONFIG gestureConfig[] = {
      { GID_PAN, panWant, panBlock },
      { GID_ZOOM, GC_ZOOM, 0},
      { GID_TWOFINGERTAP, 0, GC_TWOFINGERTAP }
    };
    pSetGestureConfig(hwnd, 0, 3, gestureConfig, sizeof(GESTURECONFIG));
  }
  isTransient = false;
}

UINT Canvas::getDpiForWindow(HWND hwnd)
{
  if (pGetDpiForWindow) {
    return pGetDpiForWindow(hwnd);
  } else
    return 96;
}

void Canvas::init(HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_DBLCLKS; // CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra	 = 0;
  wc.cbWndExtra	 = 0;
  wc.hInstance	 = hInstance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
  wc.lpszMenuName = NULL;  // actually child id
  wc.lpszClassName = className;
  wc.hIcon = NULL;
  wc.hIconSm = NULL;

  if (!RegisterClassExW(&wc)) {
    MessageBoxA(nullptr, "Canvas control registration failed!", "Error!",
		MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }

  // Check if GetGestureInfo is available (i.e. at least Windows 7)
  HMODULE hDll = LoadLibraryA("user32.dll");
  if (hDll) {
    pSetGestureConfig = (LPSetGestureConfig)
      GetProcAddress(hDll, "SetGestureConfig");
    pGetGestureInfo = (LPGetGestureInfo)
      GetProcAddress(hDll, "GetGestureInfo");
    pCloseGestureInfoHandle = (LPCloseGestureInfoHandle)
      GetProcAddress(hDll, "CloseGestureInfoHandle");
    pGetPointerType = (LPGetPointerType)
      GetProcAddress(hDll, "GetPointerType");
    pGetPointerPenInfo = (LPGetPointerPenInfo)
      GetProcAddress(hDll, "GetPointerPenInfo");
    pGetPointerPenInfoHistory = (LPGetPointerPenInfoHistory)
      GetProcAddress(hDll, "GetPointerPenInfoHistory");
    pGetDpiForWindow = (LPGetDpiForWindow)
      GetProcAddress(hDll, "GetDpiForWindow");
  }
}

// --------------------------------------------------------------------
