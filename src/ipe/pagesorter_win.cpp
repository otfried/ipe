// --------------------------------------------------------------------
// Page sorter for Win32
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

#include "appui_win.h"
#include "ipecanvas_win.h"
#include "ipethumbs.h"
#include "ipeui_wstring.h"

#include "ipelua.h"

#include <windows.h>
#include <commctrl.h>

using namespace ipe;

extern HBITMAP createBitmap(uint32_t *p, int w, int h);

// --------------------------------------------------------------------

#define IDBASE 9000
#define PAD 3
#define BORDER 6
#define BUTTONHEIGHT 14

static void buildButton(std::vector<short> &t, UINT flags, int id,
			int h, int x, const char *s)
{
  if (t.size() % 2 != 0)
    t.push_back(0);
  buildFlags(t, flags|WS_CHILD|WS_VISIBLE|BS_TEXT|WS_TABSTOP|BS_FLAT);
  t.push_back(x);
  t.push_back(h - BORDER - BUTTONHEIGHT);
  t.push_back(64);
  t.push_back(BUTTONHEIGHT);
  t.push_back(IDBASE + id);
  t.push_back(0xffff);
  t.push_back(0x0080);
  buildString(t, s);
  t.push_back(0);
}

// --------------------------------------------------------------------

struct SData {
  HIMAGELIST hImageList;
  Document *doc;
  int pno;
  std::vector<int> pages;    // pages currently displayed
  std::vector<bool> marked;  // indexed by original page number
  std::vector<int> cutlist;  // pages having been cut
};

static void insertItem(HWND h, SData *d, int index, int page, bool marked)
{
  LVITEM lvI;
  lvI.mask = LVIF_TEXT | LVIF_IMAGE;
  lvI.iItem  = index;
  lvI.iImage = page;
  lvI.iSubItem = 0;

  char buf[16];

  String m;
  if (marked) {
    m.appendUtf8(0x26ab); // larger than 0x2022
    m += " ";
  }
  String t;
  if (d->pno >= 0) {
    Page *p = d->doc->page(d->pno);
    t = p->viewName(page);
    if (t != "")
      sprintf(buf, "%d: ", page+1);
    else
      sprintf(buf, "View %d", page+1);
  } else {
    Page *p = d->doc->page(page);
    t = p->title();
    if (t != "")
      sprintf(buf, "%d: ", page+1);
    else
      sprintf(buf, "Page %d", page+1);
  }
  WString ws(m + String(buf) + t);
  lvI.pszText = ws.data();
  (void) ListView_InsertItem(h, &lvI);
}

static void populateView(HWND lv, SData *d)
{
  // TODO send WM_SETREDRAW
  (void) ListView_DeleteAllItems(lv);
  for (int i = 0; i < int(d->pages.size()); ++i) {
    insertItem(lv, d, i, d->pages[i], d->marked[d->pages[i]]);
  }
}

static void deleteItems(HWND lv, SData *d, bool cut)
{
  int n = d->pages.size();
  if (cut)
    d->cutlist.clear();
  for (int i = n-1; i >= 0; --i) {
    if (ListView_GetItemState(lv, i, LVIS_SELECTED) & LVIS_SELECTED) {
      if (cut)
	d->cutlist.insert(d->cutlist.begin(), d->pages[i]);
      d->pages.erase(d->pages.begin() + i);
    }
  }
}

static void markItems(HWND lv, SData *d, bool mark)
{
  int n = d->pages.size();
  for (int i = n-1; i >= 0; --i) {
    if (ListView_GetItemState(lv, i, LVIS_SELECTED) & LVIS_SELECTED)
      d->marked[d->pages[i]] = mark;
  }
}

static void insertItems(HWND lv, SData *d, int index)
{
  while (!d->cutlist.empty()) {
    int p = d->cutlist.back();
    d->cutlist.pop_back();
    d->pages.insert(d->pages.begin() + index, p);
  }
}

static void showPopup(HWND parent, POINT pt, int index, SData *d, HWND lv)
{
  int selcnt = ListView_GetSelectedCount(lv);
  ipeDebug("Index %d, selected %d", index, selcnt);
  if (index < 0 || selcnt == 0)
    return;

  ClientToScreen(parent, &pt);
  HMENU hMenu = CreatePopupMenu();
  if (selcnt > 0) {
    AppendMenuA(hMenu, MF_STRING, 1, "Delete");
    AppendMenuA(hMenu, MF_STRING, 2, "Cut");
    if (selcnt > 1 || !d->marked[d->pages[index]])
      AppendMenuA(hMenu, MF_STRING, 5, "Mark");
    if (selcnt > 1 || d->marked[d->pages[index]])
      AppendMenuA(hMenu, MF_STRING, 6, "Unmark");
  }
  if (!d->cutlist.empty()) {
    char buf[32];
    if (d->pno >= 0)
      sprintf(buf, "Insert before view %d", d->pages[index] + 1);
    else
      sprintf(buf, "Insert before page %d", d->pages[index] + 1);
    AppendMenuA(hMenu, MF_STRING, 3, buf);
    if (d->pno >= 0)
      sprintf(buf, "Insert after view %d", d->pages[index] + 1);
    else
      sprintf(buf, "Insert after page %d", d->pages[index] + 1);
    AppendMenuA(hMenu, MF_STRING, 4, buf);
  }
  int result = TrackPopupMenu(hMenu,
			      TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON,
			      pt.x, pt.y, 0, parent, nullptr);
  DestroyMenu(hMenu);
  switch (result) {
  case 1: // Delete
    deleteItems(lv, d, false);
    populateView(lv, d);
    break;
  case 2: // Cut
    deleteItems(lv, d, true);
    populateView(lv, d);
    break;
  case 3: // Insert before
    insertItems(lv, d, index);
    populateView(lv, d);
    break;
  case 4: // Insert after
    insertItems(lv, d, index+1);
    populateView(lv, d);
    break;
  case 5: // Mark
    markItems(lv, d, true);
    populateView(lv, d);
    break;
  case 6: // Unmark
    markItems(lv, d, false);
    populateView(lv, d);
    break;
  default:
    // Menu canceled
    break;
  }
}

static void handleResize(HWND hwnd)
{
  int dpi = Canvas::getDpiForWindow(hwnd);
  HWND h = GetDlgItem(hwnd, IDBASE);
  HWND hOk = GetDlgItem(hwnd, IDBASE + 1);
  HWND hCancel = GetDlgItem(hwnd, IDBASE + 2);
  RECT rc, rc1;
  GetClientRect(hwnd, &rc);
  GetClientRect(hOk, &rc1);
  int bw = rc1.right - rc1.left;
  int bh = rc1.bottom - rc1.top;
  int b = 16 * dpi / 96;
  MoveWindow(h, b, b, rc.right - rc.left - b - b, rc.bottom - rc.top - b - b - b - bh, TRUE);
  MoveWindow(hCancel, rc.right - b - bw, rc.bottom - b - bh, bw, bh, TRUE);
  MoveWindow(hOk, rc.right - b - b - bw - bw, rc.bottom - b - bh, bw, bh, TRUE);
}

static INT_PTR CALLBACK dialogProc(HWND hwnd, UINT message,
				   WPARAM wParam, LPARAM lParam)
{
  SData *d = (SData *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  HWND h = GetDlgItem(hwnd, IDBASE);

  switch (message) {
  case WM_INITDIALOG: {
    d = (SData *) lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) d);
    SendMessage(h, LVM_SETIMAGELIST, (WPARAM) LVSIL_NORMAL,
		(LPARAM) d->hImageList);
    populateView(h, d);
    return TRUE; }
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDBASE + 1: // Ok
      EndDialog(hwnd, 1);
      return TRUE;
    case IDBASE + 2: // Cancel
    case IDCANCEL:
      EndDialog(hwnd, -1);
      return TRUE;
    default:
      return FALSE;
    }
  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code) {
    case NM_RCLICK:
      showPopup(hwnd, ((LPNMITEMACTIVATE) lParam)->ptAction,
		((LPNMITEMACTIVATE) lParam)->iItem, d, h);
      return TRUE;
    default:
      return FALSE;
    }
  case WM_SIZE:
    handleResize(hwnd);
    return TRUE;
  case WM_CLOSE:
    EndDialog(hwnd, -1);
    return TRUE;
  default:
    return FALSE;
  }
}

// --------------------------------------------------------------------

int AppUi::pageSorter(lua_State *L, Document *doc, int pno,
		      int width, int height, int thumbWidth)
{
  SData sData;
  sData.doc = doc;
  sData.pno = pno;

  // Create image list
  Thumbnail r(doc, thumbWidth);
  // Image list will be destroyed when ListView is destroyed
  if (pno >= 0) {
    const Page *p = doc->page(pno);
    sData.hImageList = ImageList_Create(thumbWidth, r.height(), ILC_COLOR32,
					p->countViews(), 4);
    for (int i = 0; i < p->countViews(); ++i) {
      sData.pages.push_back(i);
      sData.marked.push_back(p->markedView(i));
      Buffer bx = r.render(p, i);
      HBITMAP b = createBitmap((uint32_t *) bx.data(), thumbWidth, r.height());
      ImageList_Add(sData.hImageList, b, nullptr);
    }
  } else {
    sData.hImageList = ImageList_Create(thumbWidth, r.height(), ILC_COLOR32,
					doc->countPages(), 4);
    for (int i = 0; i < doc->countPages(); ++i) {
      sData.pages.push_back(i);
      Page *p = doc->page(i);
      sData.marked.push_back(p->marked());
      Buffer bx = r.render(p, p->countViews() - 1);
      HBITMAP b = createBitmap((uint32_t *) bx.data(), thumbWidth, r.height());
      ImageList_Add(sData.hImageList, b, nullptr);
    }
  }

  // Convert to dialog units:
  LONG base = GetDialogBaseUnits();
  int baseX = LOWORD(base);
  int baseY = HIWORD(base);
  int w = width * 4 / baseX;
  int h = height * 8 / baseY;

  std::vector<short> t;

  // Dialog flags
  buildFlags(t, WS_POPUP | WS_BORDER | DS_SHELLFONT | WS_SIZEBOX |
	     WS_SYSMENU | DS_MODALFRAME | WS_CAPTION);
  t.push_back(3);  // no of elements
  t.push_back(100);  // position of popup-window
  t.push_back(30);
  t.push_back(w);
  t.push_back(h);
  // menu
  t.push_back(0);
  // class
  t.push_back(0);
  // title
  if (pno >= 0)
    buildString(t, "Ipe: View sorter");
  else
    buildString(t, "Ipe: Page sorter");
  // for DS_SHELLFONT
  t.push_back(10);
  buildString(t,"MS Shell Dlg");
  // Page sorter control
  if (t.size() % 2 != 0)
    t.push_back(0);
  buildFlags(t, WS_CHILD|WS_VISIBLE|LVS_ICON|LVS_AUTOARRANGE|
	     WS_VSCROLL|WS_BORDER);
  t.push_back(BORDER);
  t.push_back(BORDER);
  t.push_back(w - 2 * BORDER);
  t.push_back(h - 2 * BORDER - PAD - BUTTONHEIGHT);
  t.push_back(IDBASE);
  buildString(t, WC_LISTVIEWA);
  t.push_back(0);
  t.push_back(0);
  buildButton(t, BS_DEFPUSHBUTTON, 1, h, w - BORDER - 128 - PAD, "Ok");
  buildButton(t, BS_PUSHBUTTON, 2, h, w - BORDER - 64, "Cancel");


  int res =
    DialogBoxIndirectParamW(nullptr, (LPCDLGTEMPLATE) t.data(),
			    nullptr, dialogProc, (LPARAM) &sData);

  if (res == 1) {
    int n = sData.pages.size();
    lua_createtable(L, n, 0);
    for (int i = 1; i <= n; ++i) {
      lua_pushinteger(L, sData.pages[i-1] + 1);
      lua_rawseti(L, -2, i);
    }
    int m = sData.marked.size();
    lua_createtable(L, m, 0);
    for (int i = 1; i <= m; ++i) {
      lua_pushboolean(L, sData.marked[i-1]);
      lua_rawseti(L, -2, i);
    }
    return 2;
  } else
    return 0;
}

// --------------------------------------------------------------------
