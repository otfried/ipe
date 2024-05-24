// --------------------------------------------------------------------
// PageSelector for Win32
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

#include "ipecanvas_win.h"
#include "ipethumbs.h"

#include <windows.h>
#include <commctrl.h>

using namespace ipe;

extern HBITMAP createBitmap(uint32_t *p, int w, int h);

// --------------------------------------------------------------------

#define IDBASE 9000
#define BORDER 6

static void buildFlags(std::vector<short> &t, DWORD flags)
{
  union {
    DWORD dw;
    short s[2];
  } a;
  a.dw = flags;
  t.push_back(a.s[0]);
  t.push_back(a.s[1]);
  t.push_back(0);
  t.push_back(0);
}

static void buildString(std::vector<short> &t, const char *s)
{
  const char *p = s;
  while (*p)
    t.push_back(*p++);
  t.push_back(0);
}

// --------------------------------------------------------------------

struct SData {
  HIMAGELIST hImageList;
  std::vector<std::wstring> labels;
  int startIndex;
};

static void populateView(HWND h, SData *d)
{
  LVITEM lvI;
  lvI.mask = LVIF_TEXT | LVIF_IMAGE;
  lvI.iSubItem = 0;
  for (int i = 0; i < size(d->labels); ++i) {
    lvI.pszText = d->labels[i].data();
    lvI.iItem  = i;
    lvI.iImage = i;
    (void) ListView_InsertItem(h, &lvI);
  }
  SendMessage(h, LVM_ENSUREVISIBLE, (WPARAM) d->startIndex, FALSE);
}

static INT_PTR CALLBACK dialogProc(HWND hwnd, UINT message,
				   WPARAM wParam, LPARAM lParam)
{
  SData *d = (SData *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (message) {
  case WM_INITDIALOG: {
    d = (SData *) lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) d);
    HWND h = GetDlgItem(hwnd, IDBASE);
    SendMessage(h, LVM_SETIMAGELIST, (WPARAM) LVSIL_NORMAL, (LPARAM) d->hImageList);
    populateView(h, d);
    return (INT_PTR) TRUE; }
  case WM_COMMAND:
    if (LOWORD(wParam) == IDCANCEL) {
      EndDialog(hwnd, FALSE);
      return TRUE;
    }
    return FALSE;
  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code) {
    case LVN_ITEMACTIVATE:
      EndDialog(hwnd, ((LPNMITEMACTIVATE) lParam)->iItem);
      return (INT_PTR) TRUE;
    default:
      return (INT_PTR) FALSE;
    }
  case WM_CLOSE:
    EndDialog(hwnd, -1);
    return (INT_PTR) TRUE;
  default:
    return (INT_PTR) FALSE;
  }
}

int showPageSelectDialog(int width, int height, const char * title,
			 HIMAGELIST il, std::vector<String> &labels, int startIndex)
{
  SData sData;
  sData.hImageList = il;
  sData.startIndex = startIndex;
  for (int i = 0; i < size(labels); ++i)
    sData.labels.push_back(labels[i].w());

  // Convert to dialog units:
  LONG base = GetDialogBaseUnits();
  int baseX = LOWORD(base);
  int baseY = HIWORD(base);
  int w = width * 4 / baseX;
  int h = height * 8 / baseY;

  std::vector<short> t;

  // Dialog flags
  buildFlags(t, WS_POPUP | WS_BORDER | DS_SHELLFONT |
	     WS_SYSMENU | DS_MODALFRAME | WS_CAPTION);
  t.push_back(1);  // no of elements
  t.push_back(100);  // position of popup-window
  t.push_back(30);
  t.push_back(w);
  t.push_back(h);
  // menu
  t.push_back(0);
  // class
  t.push_back(0);
  // title
  buildString(t, title);
  // for DS_SHELLFONT
  t.push_back(10);
  buildString(t,"MS Shell Dlg");
  // Item
  if (t.size() % 2 != 0)
    t.push_back(0);
  // LVS_SHAREIMAGELISTS: do not destroy imagelist when dialog is destroyed
  buildFlags(t, WS_CHILD|WS_VISIBLE|LVS_ICON|LVS_SHAREIMAGELISTS|
	     WS_VSCROLL|LVS_SINGLESEL|WS_BORDER);
  t.push_back(BORDER);
  t.push_back(BORDER);
  t.push_back(w - 2 * BORDER);
  t.push_back(h - 2 * BORDER);
  t.push_back(IDBASE);    // control id
  buildString(t, WC_LISTVIEWA);
  t.push_back(0);
  t.push_back(0);

  int res = DialogBoxIndirectParamW(nullptr, (LPCDLGTEMPLATE) t.data(),
				    nullptr, dialogProc, (LPARAM) &sData);
  return res;
}

// --------------------------------------------------------------------

//! Show dialog to select a page or a view.
/*! If \a page is negative (the default), shows thumbnails of all
    pages of the document in a dialog.  If the user selects a page,
    the page number is returned. If the dialog is canceled, -1 is
    returned.

    If \a page is non-negative, all views of this page are shown, and
    the selected view number is returned.

    \a pageWidth is the width of the page thumbnails (the height is
    computed automatically).
*/
int CanvasBase::selectPageOrView(Document *doc, int page, int startIndex,
				 int pageWidth, int width, int height)
{
  // Create image list
  Thumbnail r(doc, pageWidth);
  int nItems = (page >= 0) ? doc->page(page)->countViews() :
    doc->countPages();
  // Image list will be destroyed when ListView is destroyed
  HIMAGELIST il = ImageList_Create(pageWidth, r.height(), ILC_COLOR32,
				   nItems, 4);
  if (page >= 0) {
    Page *p = doc->page(page);
    for (int i = 0; i < p->countViews(); ++i) {
      Buffer bx = r.render(p, i);
      HBITMAP b = createBitmap((uint32_t *) bx.data(), pageWidth, r.height());
      ImageList_Add(il, b, nullptr);
    }
  } else {
    for (int i = 0; i < doc->countPages(); ++i) {
      Page *p = doc->page(i);
      Buffer bx = r.render(p, p->countViews() - 1);
      HBITMAP b = createBitmap((uint32_t *) bx.data(), pageWidth, r.height());
      ImageList_Add(il, b, nullptr);
    }
  }
  const char *title = (page >= 0) ? "Ipe: Select view" : "Ipe: Select page";

  std::vector<String> labels;
  if (page >= 0) {
    Page *p = doc->page(page);
    for (int i = 0; i < p->countViews(); ++i) {
      String text;
      StringStream ss(text);
      if (!p->viewName(i).empty())
	ss << i+1 << ": " << p->viewName(i);
      else
	ss << "View " << i+1;
      labels.push_back(text);
    }
  } else {
    for (int i = 0; i < doc->countPages(); ++i) {
      Page *p = doc->page(i);
      String t = p->title();
      String text;
      StringStream ss(text);
      if (t != "")
	ss << i+1 << ": " << t;
      else
	ss << "Page " << i+1;
      labels.push_back(text);
    }
  }

  int result = showPageSelectDialog(width, height, title, il, labels, startIndex);
  ImageList_Destroy(il);
  return result;
}

// --------------------------------------------------------------------

HBITMAP createBitmap(uint32_t *p, int w, int h)
{
  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = h;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biCompression = BI_RGB;
  void *pbits = nullptr;
  HBITMAP bm = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &pbits, nullptr, 0);
  uint8_t *q = (uint8_t *) pbits;
  int stride = (3 * w + 3) & ~3;
  for (int y = 0; y < h; ++y) {
    uint32_t *src = p + w * y;
    uint8_t *dst = q + stride * (h - 1 - y);
    for (int x = 0; x < w; ++x) {
      uint32_t pixel = *src++;
      *dst++ = pixel & 0xff;         // B Windows ordering
      *dst++ = (pixel >> 8) & 0xff;  // G
      *dst++ = (pixel >> 16) & 0xff; // R
    }
  }
  return bm;
}

// --------------------------------------------------------------------
