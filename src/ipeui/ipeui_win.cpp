// --------------------------------------------------------------------
// Lua bindings for Win32 dialogs
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

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

#include "ipeui_common.h"
#include "ipeui_wstring.h"

#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
// must be before this
#include <gdiplus.h>
// need IMF_SPELLCHECKING, only defined for RichEdit 8 (Windows 8+)
#define _RICHEDIT_VER 0x0800
#include <richedit.h>

#include <algorithm>
#include <memory>
#include <sstream>

// --------------------------------------------------------------------

#define IDBASE 9000
#define PAD 3
#define BORDER 6
#define BUTTONHEIGHT 14

// --------------------------------------------------------------------

void WString::init(const char * s, int len) {
    int rw = MultiByteToWideChar(CP_UTF8, 0, s, len, nullptr, 0);
    resize(rw + 1, wchar_t(0));
    MultiByteToWideChar(CP_UTF8, 0, s, len, data(), rw);
}

BOOL setWindowText(HWND h, const char * s) {
    WString ws(s);
    wchar_t * p = ws.data();
    std::vector<wchar_t> w;
    w.reserve(ws.size());
    while (*p) {
	wchar_t ch = *p++;
	if (ch != '\r') {
	    if (ch == '\n') w.push_back('\r');
	    w.push_back(ch);
	}
    }
    w.push_back(0);
    return SetWindowTextW(h, w.data());
}

void sendMessage(HWND h, UINT code, const char * t, WPARAM wParam) {
    WString w(t);
    SendMessageW(h, code, wParam, (LPARAM)w.data());
}

static std::string wideToUtf8(const wchar_t * wbuf) {
    int rm = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
    std::vector<char> multi(rm);
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, multi.data(), rm, nullptr, nullptr);
    return std::string(multi.data());
}

static void fillRect(HDC dc, const RECT & r, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

static void drawImagePreview(HDC dc, RECT rc, const ImageSpec & spec) {
    fillRect(dc, rc, RGB(255, 255, 220));
    HBRUSH frame = CreateSolidBrush(RGB(160, 160, 130));
    FrameRect(dc, &rc, frame);
    DeleteObject(frame);

    RECT body = rc;
    InflateRect(&body, -18, -16);

    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    std::vector<BYTE> bgr; // must outlive the bitmap
    if (spec.kind == ImageSpec::EFile && !spec.text.empty()) {
	// load without color correction, as in ipebitmap_win.cpp
	bitmap.reset(new Gdiplus::Bitmap(WString(spec.text).c_str(), FALSE));
    } else if (spec.kind == ImageSpec::EBitmap) {
	// GDI+ wants BGRA (not premultiplied)
	int stride = spec.width * 4;
	bgr.assign(size_t(stride) * spec.height, 0);
	const unsigned char * p = (const unsigned char *)spec.pixels.data();
	for (int y = 0; y < spec.height; ++y) {
	    BYTE * q = &bgr[size_t(y) * stride];
	    for (int x = 0; x < spec.width; ++x, p += 4, q += 4) {
		q[0] = p[2];
		q[1] = p[1];
		q[2] = p[0];
		q[3] = p[3];
	    }
	}
	bitmap.reset(new Gdiplus::Bitmap(spec.width, spec.height, stride,
					 PixelFormat32bppARGB, bgr.data()));
    }
    if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok) {
	double zoom = spec.zoom;
	double iw = bitmap->GetWidth();
	double ih = bitmap->GetHeight();
	double bw = body.right - body.left;
	double bh = body.bottom - body.top;
	double scale = std::min(bw / (iw / zoom), bh / (ih / zoom));
	scale = std::min(1.0, scale) / zoom;
	double w = iw * scale;
	double h = ih * scale;
	double x = 0.5 * (body.left + body.right) - 0.5 * w;
	double y = 0.5 * (body.top + body.bottom) - 0.5 * h;
	Gdiplus::Graphics graphics(dc);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics.DrawImage(bitmap.get(), (Gdiplus::REAL)x, (Gdiplus::REAL)y,
			   (Gdiplus::REAL)w, (Gdiplus::REAL)h);
	return;
    }

    SetBkMode(dc, TRANSPARENT);
    DrawTextA(dc,
	      (spec.kind == ImageSpec::EText && !spec.text.empty())
		  ? spec.text.c_str()
		  : "Preview unavailable",
	      -1, &body, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void buildFlags(std::vector<short> & t, DWORD flags) {
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

void buildString(std::vector<short> & t, const char * s) {
    WString w(s);
    const wchar_t * p = w.data();
    while (*p) t.push_back(*p++);
    t.push_back(0);
}

void buildControl(std::vector<short> & t, short what, const char * s) {
    t.push_back(0xffff);
    t.push_back(what);
    if (s)
	buildString(t, s);
    else
	t.push_back(0); // text
    t.push_back(0);     // creation data
}

// control identified by class name (e.g. L"RICHEDIT50W") rather than atom
void buildControlClass(std::vector<short> & t, const wchar_t * cls, const char * s) {
    while (*cls) t.push_back(*cls++);
    t.push_back(0);
    if (s)
	buildString(t, s);
    else
	t.push_back(0); // text
    t.push_back(0);     // creation data
}

// --------------------------------------------------------------------

class PDialog : public Dialog {
public:
    PDialog(lua_State * L0, WINID parent, const char * caption, const char * language);
    virtual ~PDialog();

    bool moveListSelection(int delta);

protected:
    virtual void setMapped(lua_State * L, int idx);
    virtual Result buildAndRun(int w, int h);
    void buildElements(std::vector<short> & t);
    virtual void retrieveValues();
    virtual void enableItem(int idx, bool value);

    void computeDimensions(int & w, int & h);
    void getDimensions(SElement & m, int & x, int & y, int & w, int & h);
    void buildDimensions(std::vector<short> & t, SElement & m, int id);

    virtual void acceptDialog(lua_State * L);

    BOOL initDialog();
    BOOL dlgCommand(WPARAM wParam, LPARAM lParam);
    BOOL dlgNotify(WPARAM wParam, LPARAM lParam);
    void populateSorter(int idx);
    void sorterMenu(int idx, HWND lv, POINT pt, int hit);
    BOOL handleResize();
    static BOOL CALLBACK dialogProc(HWND hwndDlg, UINT message, WPARAM wParam,
				    LPARAM lParam);

private:
    int iBaseX, iBaseY;
    int iButtonX;
    std::vector<int> iRowHeight;
    std::vector<int> iColWidth;
    bool iSettingText = false; // suppress callbacks for programmatic text changes
    // sorter elements: allow changes of the checkboxes made by this program
    bool iSorterUpdating = false;
    // sorter elements: the items cut from the list (number and state image),
    // indexed by element
    std::vector<std::vector<std::pair<int, int>>> iCutLists;
};

PDialog::PDialog(lua_State * L0, WINID parent, const char * caption,
		 const char * language)
    : Dialog(L0, parent, caption, language) {
    LONG base = GetDialogBaseUnits();
    iBaseX = LOWORD(base);
    iBaseY = HIWORD(base);
}

PDialog::~PDialog() {
    // nothing yet
}

void PDialog::setMapped(lua_State * L, int idx) {
    SElement & m = iElements[idx];
    HWND h = GetDlgItem(hDialog, idx + IDBASE);
    switch (m.type) {
    case ETextEdit:
    case EInput:
    case ELabel:
	iSettingText = true;
	setWindowText(h, m.text.c_str());
	iSettingText = false;
	break;
    case EImage: InvalidateRect(h, nullptr, TRUE); break;
    case ESorter: populateSorter(idx); break;
    case EList:
	if (!lua_isnumber(L, 3)) {
	    ListBox_ResetContent(h);
	    for (int j = 0; j < int(m.items.size()); ++j)
		sendMessage(h, LB_ADDSTRING, m.items[j].c_str());
	}
	ListBox_SetCurSel(h, m.value);
	break;
    case ECombo:
	if (!lua_isnumber(L, 3)) {
	    ComboBox_ResetContent(h);
	    for (int j = 0; j < int(m.items.size()); ++j)
		sendMessage(h, CB_ADDSTRING, m.items[j].c_str());
	}
	ComboBox_SetCurSel(h, m.value);
	break;
    case ECheckBox:
	CheckDlgButton(hDialog, idx + IDBASE, (m.value ? BST_CHECKED : BST_UNCHECKED));
	break;
    default: break; // already handled in setUnmapped
    }
}

static void markupLog(HWND h, const std::string & text) {
    const char * p = text.c_str();
    if (*p) ++p;
    int pos = 1;
    while (*p) {
	if (p[-1] == '\n' && p[0] == '!') {
	    // found it
	    const char * b = p;
	    while (*p && *p != '\n') ++p;
	    SendMessage(h, EM_SETSEL, (WPARAM)pos, (LPARAM)pos + (p - b));
	    int line = SendMessage(h, EM_LINEFROMCHAR, (WPARAM)pos, 0);
	    SendMessage(h, EM_LINESCROLL, 0, (LPARAM)line - 1);
	    return;
	}
	// take into account conversion done by setText
	if (*p == '\n')
	    pos += 2;
	else if (*p != '\r')
	    ++pos;
	++p;
    }
    // not found, nothing to be done
}

// --------------------------------------------------------------------
// The sorter element: a list view in icon view mode.  The user can select
// items and, unless the sorter is readonly, delete them, mark them, and move
// them (by cutting them and inserting them somewhere else) using a context
// menu.  The checkboxes (state images) of the list view are only displayed:
// the user cannot change them directly.

// state images: 0 means no checkbox, 1 is unchecked, 2 is checked
static int sorterStateImage(HWND lv, int i) {
    return (ListView_GetItemState(lv, i, LVIS_STATEIMAGEMASK) >> 12) & 0xf;
}

static bool sorterSelected(HWND lv, int i) {
    return (ListView_GetItemState(lv, i, LVIS_SELECTED) & LVIS_SELECTED) != 0;
}

// The number (starting from 1) of the item in the table given to the dialog
static int sorterNumber(HWND lv, int i) {
    LVITEMW item = {};
    item.mask = LVIF_PARAM;
    item.iItem = i;
    SendMessageW(lv, LVM_GETITEMW, 0, (LPARAM)&item);
    return int(item.lParam);
}

static std::vector<int> sorterSelectedRows(HWND lv) {
    std::vector<int> rows;
    int n = ListView_GetItemCount(lv);
    for (int i = 0; i < n; ++i)
	if (sorterSelected(lv, i)) rows.push_back(i);
    return rows;
}

// An icon of the given size, with the bitmap in its center.  The alpha channel
// has to be premultiplied for the image list.
static HBITMAP createIconBitmap(const ImageSpec & spec, int width, int height) {
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height; // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void * bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap || !bits) return bitmap;
    memset(bits, 0, size_t(width) * size_t(height) * 4);
    if (spec.kind == ImageSpec::EBitmap) {
	int ox = (width - spec.width) / 2;
	int oy = (height - spec.height) / 2;
	const unsigned char * p = (const unsigned char *)spec.pixels.data();
	for (int y = 0; y < spec.height; ++y) {
	    unsigned char * q = (unsigned char *)bits + (size_t(oy + y) * width + ox) * 4;
	    for (int x = 0; x < spec.width; ++x, p += 4, q += 4) {
		unsigned a = p[3];
		q[0] = (unsigned char)((p[2] * a + 127) / 255);
		q[1] = (unsigned char)((p[1] * a + 127) / 255);
		q[2] = (unsigned char)((p[0] * a + 127) / 255);
		q[3] = (unsigned char)a;
	    }
	}
    }
    return bitmap;
}

static void insertSorterItem(HWND lv, const std::vector<SorterItem> & items, int number,
			     int position, int stateImage) {
    WString label(items[number - 1].label.c_str());
    LVITEMW item = {};
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    item.iItem = position;
    item.pszText = label.data();
    // the image list has the icons of all items, in the order of the table
    item.iImage = number - 1;
    item.lParam = number;
    SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
    // with checkboxes, new items start with the state image "unchecked", and
    // the state given to LVM_INSERTITEM is not reliably used: set it explicitly
    ListView_SetItemState(lv, position, INDEXTOSTATEIMAGEMASK(stateImage),
			  LVIS_STATEIMAGEMASK);
}

void PDialog::populateSorter(int idx) {
    SElement & m = iElements[idx];
    HWND lv = GetDlgItem(hDialog, idx + IDBASE);
    iSorterUpdating = true;
    iCutLists[idx].clear();
    ListView_DeleteAllItems(lv);
    ListView_SetExtendedListViewStyleEx(lv, LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER,
					LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);

    // all icons get the size of the largest bitmap
    int iconWidth = 1, iconHeight = 1;
    for (const SorterItem & it : m.sorter) {
	if (it.image.kind != ImageSpec::EBitmap) continue;
	iconWidth = std::max(iconWidth, it.image.width);
	iconHeight = std::max(iconHeight, it.image.height);
    }
    int n = int(m.sorter.size());
    HIMAGELIST images = ImageList_Create(iconWidth, iconHeight, ILC_COLOR32, n, 4);
    for (const SorterItem & it : m.sorter) {
	HBITMAP bitmap = createIconBitmap(it.image, iconWidth, iconHeight);
	ImageList_Add(images, bitmap, nullptr);
	DeleteObject(bitmap);
    }
    // the list view destroys its image lists when it is destroyed
    HIMAGELIST old = ListView_SetImageList(lv, images, LVSIL_NORMAL);
    if (old) ImageList_Destroy(old);
    ListView_SetIconSpacing(lv, std::max(iconWidth, 80) + 40, iconHeight + 50);

    for (int i = 0; i < n; ++i) {
	const SorterItem & it = m.sorter[i];
	int state = it.hasMark ? (it.mark ? 2 : 1) : 0;
	insertSorterItem(lv, m.sorter, i + 1, i, state);
    }
    iSorterUpdating = false;
}

void PDialog::sorterMenu(int idx, HWND lv, POINT pt, int hit) {
    // the item under the pointer is selected, unless it was already
    if (hit >= 0 && !sorterSelected(lv, hit)) {
	ListView_SetItemState(lv, -1, 0, LVIS_SELECTED);
	ListView_SetItemState(lv, hit, LVIS_SELECTED, LVIS_SELECTED);
    }
    std::vector<int> rows = sorterSelectedRows(lv);
    bool canMark = false, canUnmark = false;
    for (int r : rows) {
	int state = sorterStateImage(lv, r);
	if (state == 1) canMark = true;
	if (state == 2) canUnmark = true;
    }
    std::vector<std::pair<int, int>> & cut = iCutLists[idx];
    if (rows.empty() && cut.empty()) return;

    HMENU menu = CreatePopupMenu();
    if (!rows.empty()) {
	AppendMenuA(menu, MF_STRING, 1, "&Delete");
	AppendMenuA(menu, MF_STRING, 2, "&Cut");
	if (canMark) AppendMenuA(menu, MF_STRING, 5, "&Mark");
	if (canUnmark) AppendMenuA(menu, MF_STRING, 6, "&Unmark");
    }
    if (!cut.empty()) {
	if (hit >= 0) {
	    AppendMenuA(menu, MF_STRING, 3, "Insert &before");
	    AppendMenuA(menu, MF_STRING, 4, "Insert &after");
	} else {
	    AppendMenuA(menu, MF_STRING, 3, "Insert at &end");
	}
    }
    ClientToScreen(lv, &pt);
    int result = TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON,
				pt.x, pt.y, 0, hDialog, nullptr);
    DestroyMenu(menu);

    SElement & m = iElements[idx];
    switch (result) {
    case 1: // Delete
	for (auto it = rows.rbegin(); it != rows.rend(); ++it)
	    ListView_DeleteItem(lv, *it);
	break;
    case 2: // Cut: the new cut items replace the old ones
	cut.clear();
	for (int r : rows) cut.push_back({sorterNumber(lv, r), sorterStateImage(lv, r)});
	for (auto it = rows.rbegin(); it != rows.rend(); ++it)
	    ListView_DeleteItem(lv, *it);
	break;
    case 3:   // Insert (before the item, or at the end)
    case 4: { // Insert after
	int pos = (hit < 0) ? ListView_GetItemCount(lv) : (result == 3 ? hit : hit + 1);
	iSorterUpdating = true;
	ListView_SetItemState(lv, -1, 0, LVIS_SELECTED);
	for (const auto & c : cut) {
	    insertSorterItem(lv, m.sorter, c.first, pos, c.second);
	    ListView_SetItemState(lv, pos, LVIS_SELECTED, LVIS_SELECTED);
	    ++pos;
	}
	cut.clear();
	iSorterUpdating = false;
    } break;
    case 5:   // Mark
    case 6: { // Unmark
	iSorterUpdating = true;
	for (int r : rows) {
	    if (sorterStateImage(lv, r) == 0) continue; // no checkbox
	    ListView_SetItemState(lv, r, INDEXTOSTATEIMAGEMASK(result == 5 ? 2 : 1),
				  LVIS_STATEIMAGEMASK);
	}
	iSorterUpdating = false;
    } break;
    default: return; // menu canceled
    }
    ListView_Arrange(lv, LVA_DEFAULT);
}

BOOL PDialog::dlgNotify(WPARAM, LPARAM lParam) {
    NMHDR * nm = (NMHDR *)lParam;
    int idx = int(nm->idFrom) - IDBASE;
    if (idx < 0 || idx >= int(iElements.size()) || iElements[idx].type != ESorter)
	return FALSE;
    switch (nm->code) {
    case LVN_ITEMCHANGING: {
	// the user cannot change the checkboxes (not by clicking, and not by keyboard)
	NMLISTVIEW * lvn = (NMLISTVIEW *)lParam;
	if (!iSorterUpdating && (lvn->uChanged & LVIF_STATE)
	    && ((lvn->uNewState ^ lvn->uOldState) & LVIS_STATEIMAGEMASK)) {
	    SetWindowLongPtr(hDialog, DWLP_MSGRESULT, TRUE); // reject the change
	    return TRUE;
	}
	return FALSE;
    }
    case NM_RCLICK:
	if (!(iElements[idx].flags & EReadOnly)) {
	    NMITEMACTIVATE * ia = (NMITEMACTIVATE *)lParam;
	    sorterMenu(idx, ia->hdr.hwndFrom, ia->ptAction, ia->iItem);
	}
	return TRUE;
    default: return FALSE;
    }
}

BOOL PDialog::initDialog() {
    BOOL result = TRUE;
    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	HWND h = GetDlgItem(hDialog, i + IDBASE);
	if (m.flags & EDisabled) EnableWindow(h, FALSE);
	switch (m.type) {
	case EInput:
	case ETextEdit:
	    iSettingText = true;
	    setWindowText(h, m.text.c_str());
	    iSettingText = false;
	    if (m.flags & EFocused) {
		SetFocus(h);
		result = FALSE; // we set the focus ourselves
	    }
	    if (m.flags & ELogFile) markupLog(h, m.text);
	    if (m.type == ETextEdit && !(m.flags & ELogFile)) {
		LRESULT opts = SendMessage(h, EM_GETLANGOPTIONS, 0, 0);
		SendMessage(h, EM_SETLANGOPTIONS, 0, opts | IMF_SPELLCHECKING);
	    }
	    break;
	case EList:
	    for (int j = 0; j < int(m.items.size()); ++j)
		sendMessage(h, LB_ADDSTRING, m.items[j].c_str());
	    ListBox_SetCurSel(h, m.value);
	    break;
	case ECombo:
	    for (int j = 0; j < int(m.items.size()); ++j)
		sendMessage(h, CB_ADDSTRING, m.items[j].c_str());
	    ComboBox_SetCurSel(h, m.value);
	    break;
	case ECheckBox:
	    CheckDlgButton(hDialog, i + IDBASE, (m.value ? BST_CHECKED : BST_UNCHECKED));
	    break;
	case ESorter: populateSorter(i); break;
	default: break;
	}
    }
    return result;
}

static std::string getEditText(HWND h) {
    int n = GetWindowTextLengthW(h);
    if (n == 0) return std::string("");
    WCHAR wbuf[n + 1];
    GetWindowTextW(h, wbuf, n + 1);
    std::vector<wchar_t> w;
    wchar_t * p = wbuf;
    while (*p) {
	wchar_t ch = *p++;
	if (ch != '\r') w.push_back(ch);
    }
    w.push_back(0);
    return wideToUtf8(w.data());
}

void PDialog::retrieveValues() {
    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	HWND h = GetDlgItem(hDialog, i + IDBASE);
	switch (m.type) {
	case ETextEdit:
	case EInput: m.text = getEditText(h); break;
	case EList:
	    m.value = ListBox_GetCurSel(h);
	    if (m.value == LB_ERR) m.value = 0;
	    break;
	case ECombo:
	    m.value = ComboBox_GetCurSel(h);
	    if (m.value == CB_ERR) m.value = 0;
	    break;
	case ECheckBox:
	    m.value = (IsDlgButtonChecked(hDialog, i + IDBASE) == BST_CHECKED);
	    break;
	case ESorter: {
	    // the numbers of the items in the list, negative if marked; for a
	    // readonly sorter the numbers of the selected items
	    m.order.clear();
	    int n = ListView_GetItemCount(h);
	    for (int k = 0; k < n; ++k) {
		int number = sorterNumber(h, k);
		if (m.flags & EReadOnly) {
		    if (sorterSelected(h, k)) m.order.push_back(number);
		} else {
		    m.order.push_back(sorterStateImage(h, k) == 2 ? -number : number);
		}
	    }
	} break;
	case ELabel:
	default: break; // nothing to do
	}
    }
}

void PDialog::getDimensions(SElement & m, int & x, int & y, int & w, int & h) {
    x = BORDER;
    y = BORDER;
    w = 0;
    h = 0;
    if (m.row < 0) {
	y = BORDER;
	for (int i = 0; i < iNoRows; ++i) y += iRowHeight[i] + PAD;
	w = m.minWidth;
	h = BUTTONHEIGHT;
	x = iButtonX;
	iButtonX += w + PAD;
    } else {
	for (int i = 0; i < m.col; ++i) x += iColWidth[i] + PAD;
	for (int i = 0; i < m.row; ++i) y += iRowHeight[i] + PAD;
	w = iColWidth[m.col];
	for (int i = m.col + 1; i < m.col + m.colspan; ++i) w += iColWidth[i] + PAD;
	h = iRowHeight[m.row];
	for (int i = m.row + 1; i < m.row + m.rowspan; ++i) h += iRowHeight[i] + PAD;
    }
}

void PDialog::buildDimensions(std::vector<short> & t, SElement & m, int id) {
    int x, y, w, h;
    getDimensions(m, x, y, w, h);
    t.push_back(x);
    t.push_back(y);
    t.push_back(w);
    t.push_back(h);
    t.push_back(id); // control id
}

BOOL PDialog::dlgCommand(WPARAM wParam, LPARAM lParam) {
    int id = LOWORD(wParam);
    if (id == IDOK) {
	retrieveValues();
	EndDialog(hDialog, TRUE);
	return TRUE;
    }
    if (id == IDCANCEL) {
	retrieveValues();
	if (iIgnoreEscapeField < 0
	    || iElements[iIgnoreEscapeField].text == iIgnoreEscapeText) {
	    // allowed to close dialog
	    EndDialog(hDialog, FALSE);
	    return TRUE;
	}
    }
    if (id < IDBASE || id >= IDBASE + int(iElements.size())) return FALSE;
    SElement & m = iElements[id - IDBASE];
    if (m.flags & EAccept) {
	retrieveValues();
	EndDialog(hDialog, TRUE);
	return TRUE;
    } else if (m.flags & EReject) {
	retrieveValues();
	EndDialog(hDialog, FALSE);
	return TRUE;
    } else if (m.type == EList && HIWORD(wParam) != LBN_SELCHANGE) {
	// ignore other list notifications, such as focus changes
    } else if (m.type == EInput) {
	// only react to edits, not to focus changes or programmatic changes
	if (HIWORD(wParam) == EN_CHANGE && !iSettingText && m.lua_method != LUA_NOREF)
	    callLua(m.lua_method);
    } else if (m.lua_method != LUA_NOREF)
	callLua(m.lua_method);
    return FALSE;
}

void PDialog::acceptDialog(lua_State * L) {
    int accept = lua_toboolean(L, 2);
    retrieveValues();
    EndDialog(hDialog, accept);
}

// While an input has the focus, Up/Down move the selection of the first list.
bool PDialog::moveListSelection(int delta) {
    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	if (m.type != EList || m.row < 0) continue;
	HWND h = GetDlgItem(hDialog, i + IDBASE);
	int n = ListBox_GetCount(h);
	if (n <= 0) return false;
	int cur = ListBox_GetCurSel(h);
	if (cur < 0) cur = 0;
	int next = std::max(0, std::min(n - 1, cur + delta));
	if (next != cur) {
	    ListBox_SetCurSel(h, next);
	    // programmatic changes do not send LBN_SELCHANGE
	    if (m.lua_method != LUA_NOREF) callLua(m.lua_method);
	}
	return true;
    }
    return false;
}

static WNDPROC wpOrigProc;
static WNDPROC wpOrigInputProc;
static LRESULT inputProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN && (wParam == VK_UP || wParam == VK_DOWN)) {
	PDialog * d = (PDialog *)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
	if (d && d->moveListSelection(wParam == VK_DOWN ? 1 : -1)) return 0;
    }
    return CallWindowProc(wpOrigInputProc, hwnd, message, wParam, lParam);
}

static LRESULT subclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN) {
	if (wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000)) {
	    SendMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
	    return 0;
	}
    }
    return CallWindowProc(wpOrigProc, hwnd, message, wParam, lParam);
}

BOOL CALLBACK PDialog::dialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PDialog * d = (PDialog *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message) {
    case WM_INITDIALOG:
	d = (PDialog *)lParam;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)d);
	d->hDialog = hwnd;
	// subclass all Edit controls
	// TODO: Use modern SetSubclass method
	for (int i = 0; i < int(d->iElements.size()); ++i) {
	    if (d->iElements[i].type == ETextEdit)
		wpOrigProc = (WNDPROC)SetWindowLongPtr(
		    GetDlgItem(hwnd, i + IDBASE), GWLP_WNDPROC, (LONG_PTR)&subclassProc);
	    else if (d->iElements[i].type == EInput)
		wpOrigInputProc = (WNDPROC)SetWindowLongPtr(
		    GetDlgItem(hwnd, i + IDBASE), GWLP_WNDPROC, (LONG_PTR)&inputProc);
	}
	return d->initDialog();

    case WM_COMMAND:
	if (d)
	    return d->dlgCommand(wParam, lParam);
	else
	    return FALSE;
    case WM_SIZE:
	if (d) return d->handleResize();
	return FALSE;
    case WM_NOTIFY:
	if (d) return d->dlgNotify(wParam, lParam);
	return FALSE;
    case WM_DRAWITEM:
	if (d && wParam >= IDBASE && wParam < IDBASE + d->iElements.size()) {
	    SElement & m = d->iElements[wParam - IDBASE];
	    if (m.type == EImage) {
		DRAWITEMSTRUCT * dis = (DRAWITEMSTRUCT *)lParam;
		drawImagePreview(dis->hDC, dis->rcItem, m.image);
		return TRUE;
	    }
	}
	return FALSE;
    case WM_DESTROY:
	// Remove the subclasses from text edits
	for (int i = 0; i < int(d->iElements.size()); ++i) {
	    if (d->iElements[i].type == ETextEdit)
		SetWindowLongPtr(GetDlgItem(hwnd, i + IDBASE), GWLP_WNDPROC,
				 (LONG_PTR)wpOrigProc);
	    else if (d->iElements[i].type == EInput)
		SetWindowLongPtr(GetDlgItem(hwnd, i + IDBASE), GWLP_WNDPROC,
				 (LONG_PTR)wpOrigInputProc);
	}
	return FALSE;
    default: return FALSE;
    }
}

void PDialog::buildElements(std::vector<short> & t) {
    for (int i = 0; i < int(iElements.size()); ++i) {
	if (t.size() % 2 != 0) t.push_back(0);
	SElement & m = iElements[i];
	int id = i + IDBASE;
	DWORD flags = WS_CHILD | WS_VISIBLE;
	switch (m.type) {
	case EButton:
	    flags |= BS_TEXT | WS_TABSTOP | BS_FLAT;
	    if (m.flags & EAccept)
		flags |= BS_DEFPUSHBUTTON;
	    else
		flags |= BS_PUSHBUTTON;
	    buildFlags(t, flags);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0080, m.text.c_str()); // button
	    break;
	case ECheckBox:
	    buildFlags(t, flags | BS_AUTOCHECKBOX | BS_TEXT | WS_TABSTOP);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0080, m.text.c_str()); // button
	    break;
	case ELabel:
	    buildFlags(t, flags | SS_LEFT);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0082, m.text.c_str()); // static text
	    break;
	case EImage:
	    buildFlags(t, flags | SS_OWNERDRAW);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0082, nullptr); // static frame
	    break;
	case EInput:
	    buildFlags(t, flags | ES_LEFT | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0081); // edit
	    break;
	case ETextEdit:
	    flags |= ES_LEFT | WS_TABSTOP | WS_BORDER;
	    flags |= ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL;
	    if (m.flags & EReadOnly) flags |= ES_READONLY;
	    buildFlags(t, flags);
	    buildDimensions(t, m, id);
	    buildControlClass(t, L"RICHEDIT50W", nullptr); // for spell checking
	    break;
	case EList:
	    buildFlags(t, flags | WS_TABSTOP | WS_VSCROLL | WS_BORDER | LBS_NOTIFY);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0083); // list box
	    break;
	case ECombo:
	    buildFlags(t, flags | CBS_DROPDOWNLIST | WS_TABSTOP);
	    buildDimensions(t, m, id);
	    buildControl(t, 0x0085); // combo box
	    break;
	case ESorter:
	    buildFlags(t, flags | WS_TABSTOP | WS_BORDER | LVS_ICON | LVS_SHOWSELALWAYS
			      | LVS_AUTOARRANGE);
	    buildDimensions(t, m, id);
	    buildControlClass(t, L"SysListView32", nullptr); // list view
	    break;
	default: break;
	}
    }
}

Dialog::Result PDialog::buildAndRun(int w, int h) {
    // converts dimensions to dialog units and possibly changes them
    computeDimensions(w, h);

    iCutLists.assign(iElements.size(), {});
    for (const SElement & m : iElements) {
	if (m.type != ESorter) continue;
	// the list view class has to be registered
	INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES};
	InitCommonControlsEx(&icex);
	break;
    }

    RECT rect;
    GetWindowRect(iParent, &rect);
    int pw = (rect.right - rect.left) * 4 / iBaseX;
    int ph = (rect.bottom - rect.top) * 8 / iBaseY;

    std::vector<short> t;

    // Dialog flags
    buildFlags(t, WS_POPUP | WS_BORDER | DS_SHELLFONT | WS_SIZEBOX | WS_SYSMENU
		      | DS_MODALFRAME | WS_CAPTION);
    t.push_back(iElements.size());
    t.push_back((pw - w)
		/ 2); // offset of popup-window from parent window in dialog units
    t.push_back((ph - h) / 2);
    t.push_back(w); // should be dialog units
    t.push_back(h);
    // menu
    t.push_back(0);
    // class
    t.push_back(0);
    // title
    buildString(t, iCaption.c_str());
    // for DS_SHELLFONT
    t.push_back(10);
    buildString(t, "MS Shell Dlg");
    buildElements(t);
    int res = DialogBoxIndirectParamW(
	(HINSTANCE)GetWindowLongPtr(iParent, GWLP_HINSTANCE), (LPCDLGTEMPLATE)t.data(),
	iParent, (DLGPROC)dialogProc, (LPARAM)this);
    // retrieveValues() has been called before EndDialog!
    hDialog = nullptr; // already destroyed by Windows
    return (res > 0) ? Result::ACCEPTED : Result::CLOSED;
}

// converts (w, h) from pixels to dialog units and possibly increases them to fit dialog
void PDialog::computeDimensions(int & w, int & h) {
    int minWidth[iNoCols];
    int minHeight[iNoRows];

    for (int i = 0; i < iNoCols; ++i) minWidth[i] = 0;
    for (int i = 0; i < iNoRows; ++i) minHeight[i] = 0;
    int buttonWidth = -PAD;

    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	if (m.row < 0) { // button row
	    buttonWidth += m.minWidth + PAD;
	} else {
	    // for EImage and ESorter, minWidth/minHeight are a pixel size, not
	    // dialog units
	    int pixelW = m.minWidth;
	    int pixelH = m.minHeight;
	    bool inPixels = (m.type == EImage || m.type == ESorter);
	    int dluW = inPixels ? pixelW * 4 / iBaseX : pixelW;
	    int dluH = inPixels ? pixelH * 8 / iBaseY : pixelH;
	    int wd = dluW / m.colspan;
	    for (int j = m.col; j < m.col + m.colspan; ++j) {
		if (wd > minWidth[j]) minWidth[j] = wd;
	    }
	    int ht = dluH / m.rowspan;
	    for (int j = m.row; j < m.row + m.rowspan; ++j) {
		if (ht > minHeight[j]) minHeight[j] = ht;
	    }
	}
    }

    // Convert w and h to dialog units:
    w = w * 4 / iBaseX;
    h = h * 8 / iBaseY;

    while (int(iColStretch.size()) < iNoCols) iColStretch.push_back(0);
    while (int(iRowStretch.size()) < iNoRows) iRowStretch.push_back(0);

    int totalW = BORDER + BORDER - PAD;
    int totalWStretch = 0;
    for (int i = 0; i < iNoCols; ++i) {
	totalW += minWidth[i] + PAD;
	totalWStretch += iColStretch[i];
    }
    int totalH = BORDER + BORDER + BUTTONHEIGHT;
    int totalHStretch = 0;
    for (int i = 0; i < iNoRows; ++i) {
	totalH += minHeight[i] + PAD;
	totalHStretch += iRowStretch[i];
    }

    if (totalW > w) w = totalW;
    if (totalH > h) h = totalH;
    if (buttonWidth + BORDER + BORDER > w) w = buttonWidth + BORDER + BORDER;

    iButtonX = (w - buttonWidth) / 2;

    int spareW = w - totalW;
    int spareH = h - totalH;
    iColWidth.resize(iNoCols);
    iRowHeight.resize(iNoRows);

    if (totalWStretch == 0) {
	// spread spareW equally
	int extra = spareW / iNoCols;
	for (int i = 0; i < iNoCols; ++i) iColWidth[i] = minWidth[i] + extra;
    } else {
	for (int i = 0; i < iNoCols; ++i) {
	    int extra = spareW * iColStretch[i] / totalWStretch;
	    iColWidth[i] = minWidth[i] + extra;
	}
    }

    if (totalHStretch == 0) {
	// spread spareH equally
	int extra = spareH / iNoRows;
	for (int i = 0; i < iNoRows; ++i) iRowHeight[i] = minHeight[i] + extra;
    } else {
	for (int i = 0; i < iNoRows; ++i) {
	    int extra = spareH * iRowStretch[i] / totalHStretch;
	    iRowHeight[i] = minHeight[i] + extra;
	}
    }

    /*
    fprintf(stderr, "iNoCols: %d, iColWidth = ", iNoCols);
    for (int i = 0; i < iNoCols; ++i)
      fprintf(stderr, "%d ", iColWidth[i]);
    fprintf(stderr, "\n");
    fflush(stderr);
    */
}

BOOL PDialog::handleResize() {
    RECT rc;
    GetClientRect(hDialog, &rc);
    int bw = rc.right - rc.left;
    int bh = rc.bottom - rc.top;
    computeDimensions(bw, bh);

    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	int id = i + IDBASE;
	int x, y, w, h;
	getDimensions(m, x, y, w, h);
	HWND hwnd = GetDlgItem(hDialog, id);
	// calculation is in dialog units, so convert back to pixels
	MoveWindow(hwnd, x * iBaseX / 4, y * iBaseY / 8, w * iBaseX / 4, h * iBaseY / 8,
		   TRUE);
	// MoveWindow may just blit the old bits, but the image needs to be rescaled
	if (m.type == EImage) InvalidateRect(hwnd, nullptr, TRUE);
    }
    return TRUE;
}

void PDialog::enableItem(int idx, bool value) {
    EnableWindow(GetDlgItem(hDialog, idx + IDBASE), value);
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State * L) {
    HWND parent = check_winid(L, 1);
    const char * s = luaL_checklstring(L, 2, nullptr);
    const char * language = "";
    if (lua_isstring(L, 3)) language = luaL_checkstring(L, 3);

    Dialog ** dlg = (Dialog **)lua_newuserdata(L, sizeof(Dialog *));
    *dlg = nullptr;
    luaL_getmetatable(L, "Ipe.dialog");
    lua_setmetatable(L, -2);
    *dlg = new PDialog(L, parent, s, language);
    return 1;
}

// --------------------------------------------------------------------

class PMenu : public Menu {
public:
    PMenu(HWND parent);
    virtual ~PMenu();
    virtual int add(lua_State * L);
    virtual int execute(lua_State * L);

private:
    struct Item {
	std::string name;
	std::string itemName;
	int itemIndex;
    };
    std::vector<Item> items;
    int currentId;
    HMENU hMenu;
    HWND hwnd;
    std::vector<HBITMAP> bitmaps;
};

PMenu::PMenu(HWND parent) {
    hMenu = CreatePopupMenu();
    hwnd = parent;
}

PMenu::~PMenu() {
    if (hMenu) DestroyMenu(hMenu);
    hMenu = nullptr;
    for (int i = 0; i < int(bitmaps.size()); ++i) DeleteObject(bitmaps[i]);
}

int PMenu::execute(lua_State * L) {
    int vx = (int)luaL_checkinteger(L, 2);
    int vy = (int)luaL_checkinteger(L, 3);
    int result = TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, vx,
				vy, 0, hwnd, nullptr);
    if (1 <= result && result <= int(items.size())) {
	result -= 1;
	lua_pushstring(L, items[result].name.c_str());
	if (items[result].itemName.c_str())
	    lua_pushstring(L, items[result].itemName.c_str());
	else
	    lua_pushstring(L, "");
	return 2;
    }
    return 0;
}

static HBITMAP colorIcon(double red, double green, double blue) {
    int r = int(red * 255.0);
    int g = int(green * 255.0);
    int b = int(blue * 255.0);
    COLORREF rgb = RGB(r, g, b);
    int cx = GetSystemMetrics(SM_CXMENUCHECK);
    int cy = GetSystemMetrics(SM_CYMENUCHECK);
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP bm = CreateCompatibleBitmap(hdc, cx, cy);
    SelectObject(memDC, bm);
    for (int y = 0; y < cy; ++y) {
	for (int x = 0; x < cx; ++x) { SetPixel(memDC, x, y, rgb); }
    }
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);
    return bm;
}

int PMenu::add(lua_State * L) {
    const char * name = luaL_checklstring(L, 2, nullptr);
    WString title(luaL_checklstring(L, 3, nullptr));
    if (lua_gettop(L) == 3) {
	AppendMenuW(hMenu, MF_STRING, items.size() + 1, title.data());
	Item item;
	item.name = name;
	item.itemIndex = 0;
	items.push_back(item);
    } else {
	luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
	bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
	bool hastable = !hasmap && !lua_isnoneornil(L, 5);
	bool hascolor = !lua_isnoneornil(L, 6) && lua_isfunction(L, 6);
	bool hascheck = !hascolor && !lua_isnoneornil(L, 6);
	if (hastable)
	    luaL_argcheck(L, lua_istable(L, 5), 5, "argument is not a function or table");
	const char * current = nullptr;
	if (hascheck) {
	    luaL_argcheck(L, lua_isstring(L, 6), 6,
			  "argument is not a function or string");
	    current = luaL_checklstring(L, 6, nullptr);
	}
	int no = lua_rawlen(L, 4);
	HMENU sm = CreatePopupMenu();
	for (int i = 1; i <= no; ++i) {
	    lua_rawgeti(L, 4, i);
	    luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
	    int id = items.size() + 1;
	    const char * item = lua_tolstring(L, -1, nullptr);
	    if (hastable) {
		lua_rawgeti(L, 5, i);
		luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
	    } else if (hasmap) {
		lua_pushvalue(L, 5);  // function
		lua_pushnumber(L, i); // index
		lua_pushvalue(L, -3); // name
		lua_call(L, 2, 1);    // function returns label
		luaL_argcheck(L, lua_isstring(L, -1), 5,
			      "function does not return string");
	    } else
		lua_pushvalue(L, -1);

	    WString text(tostring(L, -1));
	    AppendMenuW(sm, MF_STRING, id, text.data());
	    Item mitem;
	    mitem.name = name;
	    mitem.itemName = item;
	    mitem.itemIndex = i;
	    items.push_back(mitem);
	    if (hascheck && !strcmp(item, current)) CheckMenuItem(sm, id, MF_CHECKED);

	    if (hascolor) {
		lua_pushvalue(L, 6);  // function
		lua_pushnumber(L, i); // index
		lua_pushvalue(L, -4); // name
		lua_call(L, 2, 3);    // function returns red, green, blue
		double red = luaL_checknumber(L, -3);
		double green = luaL_checknumber(L, -2);
		double blue = luaL_checknumber(L, -1);
		lua_pop(L, 3); // pop result
		HBITMAP bits = colorIcon(red, green, blue);
		bitmaps.push_back(bits);
		SetMenuItemBitmaps(sm, id, MF_BYCOMMAND, bits, bits);
	    }
	    lua_pop(L, 2); // item, text
	}
	AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)sm, title.data());
    }
    return 0;
}

// --------------------------------------------------------------------

static int menu_constructor(lua_State * L) {
    HWND hwnd = check_winid(L, 1);
    Menu ** m = (Menu **)lua_newuserdata(L, sizeof(Menu *));
    *m = nullptr;
    luaL_getmetatable(L, "Ipe.menu");
    lua_setmetatable(L, -2);
    *m = new PMenu(hwnd);
    return 1;
}

// --------------------------------------------------------------------

class PTimer : public Timer {
public:
    PTimer(lua_State * L0, int lua_object, const char * method);
    virtual ~PTimer();

    virtual int setInterval(lua_State * L);
    virtual int active(lua_State * L);
    virtual int start(lua_State * L);
    virtual int stop(lua_State * L);

protected:
    void elapsed();
    static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    static std::vector<PTimer *> all_timers;

protected:
    UINT_PTR iTimer;
    UINT iInterval;
};

std::vector<PTimer *> PTimer::all_timers;

void CALLBACK PTimer::TimerProc(HWND, UINT, UINT_PTR id, DWORD) {
    for (int i = 0; i < int(all_timers.size()); ++i) {
	if (id == all_timers[i]->iTimer) {
	    all_timers[i]->elapsed();
	    return;
	}
    }
}

PTimer::PTimer(lua_State * L0, int lua_object, const char * method)
    : Timer(L0, lua_object, method) {
    iTimer = 0;
    iInterval = 0;
    all_timers.push_back(this);
}

PTimer::~PTimer() {
    if (iTimer) KillTimer(nullptr, iTimer);
    // remove it from all_timers
    for (int i = 0; i < int(all_timers.size()); ++i) {
	if (all_timers[i] == this) {
	    all_timers.erase(all_timers.begin() + i);
	    return;
	}
    }
}

void PTimer::elapsed() {
    callLua();
    if (iSingleShot) {
	KillTimer(nullptr, iTimer);
	iTimer = 0;
    }
}

int PTimer::setInterval(lua_State * L) {
    int t = (int)luaL_checkinteger(L, 2);
    iInterval = t;
    if (iTimer) SetTimer(nullptr, iTimer, iInterval, TimerProc);
    return 0;
}

int PTimer::active(lua_State * L) {
    lua_pushboolean(L, (iTimer != 0));
    return 1;
}

int PTimer::start(lua_State * L) {
    if (iTimer == 0) iTimer = SetTimer(nullptr, 0, iInterval, TimerProc);
    return 0;
}

int PTimer::stop(lua_State * L) {
    if (iTimer) {
	KillTimer(nullptr, iTimer);
	iTimer = 0;
    }
    return 0;
}

// --------------------------------------------------------------------

static int timer_constructor(lua_State * L) {
    luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
    const char * method = luaL_checklstring(L, 2, nullptr);

    Timer ** t = (Timer **)lua_newuserdata(L, sizeof(Timer *));
    *t = nullptr;
    luaL_getmetatable(L, "Ipe.timer");
    lua_setmetatable(L, -2);

    // create a table with weak reference to Lua object
    lua_createtable(L, 1, 1);
    lua_pushliteral(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1);
    int lua_object = luaL_ref(L, LUA_REGISTRYINDEX);
    *t = new PTimer(L, lua_object, method);
    return 1;
}

// --------------------------------------------------------------------

static COLORREF custom[16] = {0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
			      0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
			      0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
			      0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff};

static int ipeui_getColor(lua_State * L) {
    HWND hwnd = check_winid(L, 1);
    // const char *title = luaL_checkstring(L, 2);
    double r = luaL_checknumber(L, 3);
    double g = luaL_checknumber(L, 4);
    double b = luaL_checknumber(L, 5);

    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    cc.rgbResult = RGB(int(r * 255), int(g * 255), int(b * 255));
    cc.lpCustColors = custom;

    if (ChooseColor(&cc)) {
	lua_pushnumber(L, GetRValue(cc.rgbResult) / 255.0);
	lua_pushnumber(L, GetGValue(cc.rgbResult) / 255.0);
	lua_pushnumber(L, GetBValue(cc.rgbResult) / 255.0);
	return 3;
    } else
	return 0;
}

// --------------------------------------------------------------------

static int ipeui_fileDialog(lua_State * L) {
    static const char * const typenames[] = {"open", "save", nullptr};
    HWND hwnd = check_winid(L, 1);
    int type = luaL_checkoption(L, 2, nullptr, typenames);
    WString caption(luaL_checklstring(L, 3, nullptr));
    if (!lua_istable(L, 4)) luaL_argerror(L, 4, "table expected for filters");
    std::wstring filters;
    int nFilters = lua_rawlen(L, 4);
    for (int i = 1; i <= nFilters; ++i) {
	lua_rawgeti(L, 4, i);
	luaL_argcheck(L, lua_isstring(L, -1), 4, "filter entry is not a string");
	WString el(tostring(L, -1));
	filters += el;
	lua_pop(L, 1); // element i
    }
    filters.push_back(0); // terminate list

    const char * dir = nullptr;
    if (!lua_isnoneornil(L, 5)) dir = luaL_checklstring(L, 5, nullptr);
    const char * name = nullptr;
    if (!lua_isnoneornil(L, 6)) name = luaL_checklstring(L, 6, nullptr);
    int selected = 0;
    if (!lua_isnoneornil(L, 7)) selected = luaL_checkinteger(L, 7);

    OPENFILENAMEW ofn;
    wchar_t szFileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;

    ofn.lpstrFilter = filters.data();
    ofn.nFilterIndex = selected;

    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"ipe";
    if (name) {
	WString wname(name);
	wcsncpy(szFileName, wname.data(), MAX_PATH);
    }
    if (dir) {
	WString wdir(dir);
	ofn.lpstrInitialDir = wdir.data();
    }
    ofn.lpstrTitle = caption.data();

    BOOL result;
    if (type == 0) {
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	result = GetOpenFileNameW(&ofn);
    } else {
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	result = GetSaveFileNameW(&ofn);
    }
    if (!result) return 0;

    std::string s = wideToUtf8(ofn.lpstrFile);
    lua_pushstring(L, s.c_str());
    lua_pushinteger(L, ofn.nFilterIndex);
    return 2;
}

// --------------------------------------------------------------------

static int ipeui_messageBox(lua_State * L) {
    static const char * const options[] = {"none",     "warning",  "information",
					   "question", "critical", nullptr};
    static const char * const buttontype[] = {
	"ok", "okcancel", "yesnocancel", "discardcancel", "savediscardcancel", nullptr};

    HWND hwnd = check_winid(L, 1);
    int type = luaL_checkoption(L, 2, "none", options);
    const char * text = luaL_checklstring(L, 3, nullptr);
    const char * details = nullptr;
    if (!lua_isnoneornil(L, 4)) details = luaL_checklstring(L, 4, nullptr);
    int buttons = 0;
    if (lua_isnumber(L, 5))
	buttons = (int)luaL_checkinteger(L, 5);
    else if (!lua_isnoneornil(L, 5))
	buttons = luaL_checkoption(L, 5, nullptr, buttontype);

    UINT uType = MB_APPLMODAL;
    switch (type) {
    case 0:
    default: break;
    case 1: uType |= MB_ICONWARNING; break;
    case 2: uType |= MB_ICONINFORMATION; break;
    case 3: uType |= MB_ICONQUESTION; break;
    case 4: uType |= MB_ICONERROR; break;
    }

    switch (buttons) {
    case 0:
    default: uType |= MB_OK; break;
    case 1: uType |= MB_OKCANCEL; break;
    case 2: uType |= MB_YESNOCANCEL; break;
    case 3:
	// should be Discard Cancel
	uType |= MB_OKCANCEL;
	break;
    case 4:
	// should be Save Discard Cancel
	uType |= MB_YESNOCANCEL;
	break;
    }

    int ret = -1;
    if (details) {
	char buf[strlen(text) + strlen(details) + 8];
	sprintf(buf, "%s\n\n%s", text, details);
	WString wbuf(buf);
	ret = MessageBoxW(hwnd, wbuf.data(), L"Ipe", uType);
    } else
	ret = MessageBoxW(hwnd, WString(text).data(), L"Ipe", uType);

    switch (ret) {
    case IDOK:
    case IDYES: lua_pushnumber(L, 1); break;
    case IDNO:
    case IDIGNORE: lua_pushnumber(L, 0); break;
    case IDCANCEL:
    default: lua_pushnumber(L, -1); break;
    }
    return 1;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State * L) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[32];
    sprintf(buf, "%04d%02d%02d%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour,
	    st.wMinute, st.wSecond);
    lua_pushstring(L, buf);
    return 1;
}

static int ipeui_startBrowser(lua_State * L) {
    const char * url = luaL_checklstring(L, 1, nullptr);
    long long int res = (long long int)ShellExecuteA(nullptr, "open", url, nullptr,
						     nullptr, SW_SHOWNORMAL);
    lua_pushboolean(L, (res >= 32));
    return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
    {"Dialog", dialog_constructor},
    {"Menu", menu_constructor},
    {"Timer", timer_constructor},
    {"getColor", ipeui_getColor},
    {"fileDialog", ipeui_fileDialog},
    {"messageBox", ipeui_messageBox},
    {"currentDateTime", ipeui_currentDateTime},
    {"startBrowser", ipeui_startBrowser},
    {nullptr, nullptr}};

// --------------------------------------------------------------------

int luaopen_ipeui(lua_State * L) {
    luaL_newlib(L, ipeui_functions);
    lua_setglobal(L, "ipeui");
    luaopen_ipeui_common(L);
    return 0;
}

// --------------------------------------------------------------------
