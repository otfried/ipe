// --------------------------------------------------------------------
// Lua bindings for Win32 dialogs
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

#include "ipeui_common.h"
#include "ipeui_wstring.h"

#include <windowsx.h>

// --------------------------------------------------------------------

#define IDBASE 9000
#define PAD 3
#define BORDER 6
#define BUTTONHEIGHT 14

// --------------------------------------------------------------------

void WString::init(const char *s, int len)
{
  int rw = MultiByteToWideChar(CP_UTF8, 0, s, len, nullptr, 0);
  resize(rw + 1, wchar_t(0));
  MultiByteToWideChar(CP_UTF8, 0, s, len, data(), rw);
}

BOOL setWindowText(HWND h, const char *s)
{
  WString ws(s);
  wchar_t *p = ws.data();
  std::vector<wchar_t> w;
  w.reserve(ws.size());
  while (*p) {
    wchar_t ch = *p++;
    if (ch != '\r') {
      if (ch == '\n')
	w.push_back('\r');
      w.push_back(ch);
    }
  }
  w.push_back(0);
  return SetWindowTextW(h, w.data());
}

void sendMessage(HWND h, UINT code, const char *t, WPARAM wParam)
{
  WString w(t);
  SendMessageW(h, code, wParam, (LPARAM) w.data());
}

static std::string wideToUtf8(const wchar_t *wbuf)
{
  int rm = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
  std::vector<char> multi(rm);
  WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, multi.data(), rm, nullptr, nullptr);
  return std::string(multi.data());
}

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
  WString w(s);
  const wchar_t *p = w.data();
  while (*p)
    t.push_back(*p++);
  t.push_back(0);
}

static void buildControl(std::vector<short> &t, short what, const char *s = nullptr)
{
  t.push_back(0xffff);
  t.push_back(what);
  if (s)
    buildString(t, s);
  else
    t.push_back(0);    // text
  t.push_back(0);      // creation data
}

// --------------------------------------------------------------------

class PDialog : public Dialog {
public:
  PDialog(lua_State *L0, WINID parent, const char *caption, const char *language);
  virtual ~PDialog();

protected:
  virtual void setMapped(lua_State *L, int idx);
  virtual bool buildAndRun(int w, int h);
  void buildElements(std::vector<short> &t);
  virtual void retrieveValues();
  virtual void enableItem(int idx, bool value);

  void computeDimensions(int &w, int &h);
  void getDimensions(SElement &m, int &x, int &y, int &w, int &h);
  void buildDimensions(std::vector<short> &t, SElement &m, int id);

  virtual void acceptDialog(lua_State *L);

  BOOL initDialog();
  BOOL dlgCommand(WPARAM wParam, LPARAM lParam);
  BOOL handleResize();
  static BOOL CALLBACK dialogProc(HWND hwndDlg, UINT message,
				  WPARAM wParam, LPARAM lParam);
private:
  int iBaseX, iBaseY;
  int iButtonX;
  std::vector<int> iRowHeight;
  std::vector<int> iColWidth;
};

PDialog::PDialog(lua_State *L0, WINID parent, const char *caption,
		 const char * language)
  : Dialog(L0, parent, caption, language)
{
  LONG base = GetDialogBaseUnits();
  iBaseX = LOWORD(base);
  iBaseY = HIWORD(base);
}

PDialog::~PDialog()
{
  // nothing yet
}

void PDialog::setMapped(lua_State *L, int idx)
{
  SElement &m = iElements[idx];
  HWND h = GetDlgItem(hDialog, idx+IDBASE);
  switch (m.type) {
  case ETextEdit:
  case EInput:
  case ELabel:
    setWindowText(h, m.text.c_str());
    break;
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
    CheckDlgButton(hDialog, idx+IDBASE,
		   (m.value ? BST_CHECKED : BST_UNCHECKED));
    break;
  default:
    break; // already handled in setUnmapped
  }
}

static void markupLog(HWND h, const std::string &text)
{
  const char *p = text.c_str();
  if (*p) ++p;
  int pos = 1;
  while (*p) {
    if (p[-1] == '\n' && p[0] == '!') {
      // found it
      const char *b = p;
      while (*p && *p != '\n')
	++p;
      SendMessage(h, EM_SETSEL, (WPARAM) pos, (LPARAM) pos + (p - b));
      int line = SendMessage(h, EM_LINEFROMCHAR, (WPARAM) pos, 0);
      SendMessage(h, EM_LINESCROLL, 0, (LPARAM) line - 1);
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

BOOL PDialog::initDialog()
{
  BOOL result = TRUE;
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    HWND h = GetDlgItem(hDialog, i+IDBASE);
    if (m.flags & EDisabled)
      EnableWindow(h, FALSE);
    switch (m.type) {
    case EInput:
    case ETextEdit:
      setWindowText(h, m.text.c_str());
      if (m.flags & EFocused) {
	SetFocus(h);
	result = FALSE; // we set the focus ourselves
      }
      if (m.flags & ELogFile)
	markupLog(h, m.text);
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
      CheckDlgButton(hDialog, i+IDBASE,
		     (m.value ? BST_CHECKED : BST_UNCHECKED));
      break;
    default:
      break;
    }
  }
  return result;
}

static std::string getEditText(HWND h)
{
  int n = GetWindowTextLengthW(h);
  if (n == 0)
    return std::string("");
  WCHAR wbuf[n+1];
  GetWindowTextW(h, wbuf, n+1);
  std::vector<wchar_t> w;
  wchar_t *p = wbuf;
  while (*p) {
    wchar_t ch = *p++;
    if (ch != '\r')
      w.push_back(ch);
  }
  w.push_back(0);
  return wideToUtf8(w.data());
}

void PDialog::retrieveValues()
{
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    HWND h = GetDlgItem(hDialog, i+IDBASE);
    switch (m.type) {
    case ETextEdit:
    case EInput:
      m.text = getEditText(h);
      break;
    case EList:
      m.value = ListBox_GetCurSel(h);
      if (m.value == LB_ERR)
	m.value = 0;
      break;
    case ECombo:
      m.value = ComboBox_GetCurSel(h);
      if (m.value == CB_ERR)
	m.value = 0;
      break;
    case ECheckBox:
      m.value = (IsDlgButtonChecked(hDialog, i+IDBASE) == BST_CHECKED);
      break;
    case ELabel:
    default:
      break; // nothing to do
    }
  }
}

void PDialog::getDimensions(SElement &m, int &x, int &y, int &w, int &h)
{
  x = BORDER;
  y = BORDER;
  w = 0;
  h = 0;
  if (m.row < 0) {
    y = BORDER;
    for (int i = 0; i < iNoRows; ++i)
      y += iRowHeight[i] + PAD;
    w = m.minWidth;
    h = BUTTONHEIGHT;
    x = iButtonX;
    iButtonX += w + PAD;
  } else {
    for (int i = 0; i < m.col; ++i)
      x += iColWidth[i] + PAD;
    for (int i = 0; i < m.row; ++i)
      y += iRowHeight[i] + PAD;
    w = iColWidth[m.col];
    for (int i = m.col + 1; i < m.col + m.colspan; ++i)
      w += iColWidth[i] + PAD;
    h = iRowHeight[m.row];
    for (int i = m.row + 1; i < m.row + m.rowspan; ++i)
      h += iRowHeight[i] + PAD;
  }
}

void PDialog::buildDimensions(std::vector<short> &t, SElement &m, int id)
{
  int x, y, w, h;
  getDimensions(m, x, y, w, h);
  t.push_back(x);
  t.push_back(y);
  t.push_back(w);
  t.push_back(h);
  t.push_back(id); // control id
}

BOOL PDialog::dlgCommand(WPARAM wParam, LPARAM lParam)
{
  int id = LOWORD(wParam);
  if (id == IDOK) {
    retrieveValues();
    EndDialog(hDialog, TRUE);
    return TRUE;
  }
  if (id == IDCANCEL) {
    retrieveValues();
    if (iIgnoreEscapeField < 0 ||
	iElements[iIgnoreEscapeField].text == iIgnoreEscapeText) {
      // allowed to close dialog
      EndDialog(hDialog, FALSE);
      return TRUE;
    }
  }
  if (id < IDBASE || id >= IDBASE + int(iElements.size()))
    return FALSE;
  SElement &m = iElements[id - IDBASE];
  if (m.flags & EAccept) {
    retrieveValues();
    EndDialog(hDialog, TRUE);
    return TRUE;
  } else if (m.flags & EReject) {
    retrieveValues();
    EndDialog(hDialog, FALSE);
    return TRUE;
  } else if (m.lua_method != LUA_NOREF)
    callLua(m.lua_method);
  return FALSE;
}

void PDialog::acceptDialog(lua_State *L)
{
  int accept = lua_toboolean(L, 2);
  retrieveValues();
  EndDialog(hDialog, accept);
}

static WNDPROC wpOrigProc;
static LRESULT subclassProc(HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam)
{
  if (message == WM_KEYDOWN) {
    if (wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000)) {
      SendMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
      return 0;
    }
  }
  return CallWindowProc(wpOrigProc, hwnd, message, wParam, lParam);
}


BOOL CALLBACK PDialog::dialogProc(HWND hwnd, UINT message,
				  WPARAM wParam, LPARAM lParam)
{
  PDialog *d = (PDialog *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (message) {
  case WM_INITDIALOG:
    d = (PDialog *) lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) d);
    d->hDialog = hwnd;
    // subclass all Edit controls
    // TODO: Use modern SetSubclass method
    for (int i = 0; i < int(d->iElements.size()); ++i) {
      if (d->iElements[i].type == ETextEdit)
	wpOrigProc = (WNDPROC)
	  SetWindowLongPtr(GetDlgItem(hwnd, i + IDBASE),
			   GWLP_WNDPROC, (LONG_PTR) &subclassProc);
    }
    return d->initDialog();

  case WM_COMMAND:
    if (d)
      return d->dlgCommand(wParam, lParam);
    else
      return FALSE;
  case WM_SIZE:
    if (d)
      return d->handleResize();
    return FALSE;
  case WM_DESTROY:
    // Remove the subclasses from text edits
    for (int i = 0; i < int(d->iElements.size()); ++i) {
      if (d->iElements[i].type == ETextEdit)
	SetWindowLongPtr(GetDlgItem(hwnd, i + IDBASE),
			 GWLP_WNDPROC, (LONG_PTR) wpOrigProc);
    }
    return FALSE;
  default:
    return FALSE;
  }
}

void PDialog::buildElements(std::vector<short> &t)
{
  for (int i = 0; i < int(iElements.size()); ++i) {
    if (t.size() % 2 != 0)
      t.push_back(0);
    SElement &m = iElements[i];
    int id = i + IDBASE;
    DWORD flags = WS_CHILD|WS_VISIBLE;
    switch (m.type) {
    case EButton:
      flags |= BS_TEXT|WS_TABSTOP|BS_FLAT;
      if (m.flags & EAccept)
	flags |= BS_DEFPUSHBUTTON;
      else
	flags |= BS_PUSHBUTTON;
      buildFlags(t, flags);
      buildDimensions(t, m, id);
      buildControl(t, 0x0080, m.text.c_str());  // button
      break;
    case ECheckBox:
      buildFlags(t, flags|BS_AUTOCHECKBOX|BS_TEXT|WS_TABSTOP);
      buildDimensions(t, m, id);
      buildControl(t, 0x0080, m.text.c_str());  // button
      break;
    case ELabel:
      buildFlags(t, flags|SS_LEFT);
      buildDimensions(t, m, id);
      buildControl(t, 0x0082, m.text.c_str());  // static text
      break;
    case EInput:
      buildFlags(t, flags|ES_LEFT|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL);
      buildDimensions(t, m, id);
      buildControl(t, 0x0081);  // edit
      break;
    case ETextEdit:
      flags |= ES_LEFT|WS_TABSTOP|WS_BORDER;
      flags |= ES_MULTILINE|ES_WANTRETURN|WS_VSCROLL;
      if (m.flags & EReadOnly)
	flags |= ES_READONLY;
      buildFlags(t, flags);
      buildDimensions(t, m, id);
      buildControl(t, 0x0081);  // edit
      break;
    case EList:
      buildFlags(t, flags|WS_TABSTOP|WS_VSCROLL|WS_BORDER);
      buildDimensions(t, m, id);
      buildControl(t, 0x0083);  // list box
      break;
    case ECombo:
      buildFlags(t, flags|CBS_DROPDOWNLIST|WS_TABSTOP);
      buildDimensions(t, m, id);
      buildControl(t, 0x0085);  // combo box
      break;
    default:
      break;
    }
  }
}

bool PDialog::buildAndRun(int w, int h)
{
  // converts dimensions to dialog units and possibly changes them
  computeDimensions(w, h);

  RECT rect;
  GetWindowRect(iParent, &rect);
  int pw = (rect.right - rect.left) * 4 / iBaseX;
  int ph = (rect.bottom - rect.top) * 8 / iBaseY;

  std::vector<short> t;

  // Dialog flags
  buildFlags(t, WS_POPUP | WS_BORDER | DS_SHELLFONT | WS_SIZEBOX |
	     WS_SYSMENU | DS_MODALFRAME | WS_CAPTION);
  t.push_back(iElements.size());
  t.push_back((pw - w)/2);  // offset of popup-window from parent window in dialog units
  t.push_back((ph - h)/2);
  t.push_back(w);  // should be dialog units
  t.push_back(h);
  // menu
  t.push_back(0);
  // class
  t.push_back(0);
  // title
  buildString(t, iCaption.c_str());
  // for DS_SHELLFONT
  t.push_back(10);
  buildString(t,"MS Shell Dlg");
  buildElements(t);
  int res =
    DialogBoxIndirectParamW((HINSTANCE) GetWindowLongPtr(iParent,
							 GWLP_HINSTANCE),
			    (LPCDLGTEMPLATE) t.data(),
			    iParent, (DLGPROC) dialogProc, (LPARAM) this);
  // retrieveValues() has been called before EndDialog!
  hDialog = nullptr; // already destroyed by Windows
  return (res > 0);
}

// converts (w, h) from pixels to dialog units and possibly increases them to fit dialog
void PDialog::computeDimensions(int &w, int &h)
{
  int minWidth[iNoCols];
  int minHeight[iNoRows];

  for (int i = 0; i < iNoCols; ++i)
    minWidth[i] = 0;
  for (int i = 0; i < iNoRows; ++i)
    minHeight[i] = 0;
  int buttonWidth = -PAD;

  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    if (m.row < 0) {  // button row
      buttonWidth += m.minWidth + PAD;
    } else {
      int wd = m.minWidth / m.colspan;
      for (int j = m.col; j < m.col + m.colspan; ++j) {
	if (wd > minWidth[j])
	  minWidth[j] = wd;
      }
      int ht = m.minHeight / m.rowspan;
      for (int j = m.row; j < m.row + m.rowspan; ++j) {
	if (ht > minHeight[j])
	  minHeight[j] = ht;
      }
    }
  }

  // Convert w and h to dialog units:
  w = w * 4 / iBaseX;
  h = h * 8 / iBaseY;

  while (int(iColStretch.size()) < iNoCols)
    iColStretch.push_back(0);
  while (int(iRowStretch.size()) < iNoRows)
    iRowStretch.push_back(0);

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

  if (totalW > w)
    w = totalW;
  if (totalH > h)
    h = totalH;
  if (buttonWidth + BORDER + BORDER > w)
    w = buttonWidth + BORDER + BORDER;

  iButtonX = (w - buttonWidth) / 2;

  int spareW = w - totalW;
  int spareH = h - totalH;
  iColWidth.resize(iNoCols);
  iRowHeight.resize(iNoRows);

  if (totalWStretch == 0) {
    // spread spareW equally
    int extra = spareW / iNoCols;
    for (int i = 0; i < iNoCols; ++i)
      iColWidth[i] = minWidth[i] + extra;
  } else {
    for (int i = 0; i < iNoCols; ++i) {
      int extra = spareW * iColStretch[i] / totalWStretch;
      iColWidth[i] = minWidth[i] + extra;
    }
  }

  if (totalHStretch == 0) {
    // spread spareH equally
    int extra = spareH / iNoRows;
    for (int i = 0; i < iNoRows; ++i)
      iRowHeight[i] = minHeight[i] + extra;
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

BOOL PDialog::handleResize()
{
  RECT rc;
  GetClientRect(hDialog, &rc);
  int bw = rc.right - rc.left;
  int bh = rc.bottom - rc.top;
  computeDimensions(bw, bh);

  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    int id = i + IDBASE;
    int x, y, w, h;
    getDimensions(m, x, y, w, h);
    HWND hwnd = GetDlgItem(hDialog, id);
    // calculation is in dialog units, so convert back to pixels
    MoveWindow(hwnd, x * iBaseX / 4 , y * iBaseY / 8 ,
	       w * iBaseX / 4, h * iBaseY / 8, TRUE);
  }
  return TRUE;
}

void PDialog::enableItem(int idx, bool value)
{
  EnableWindow(GetDlgItem(hDialog, idx+IDBASE), value);
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State *L)
{
  HWND parent = check_winid(L, 1);
  const char *s = luaL_checklstring(L, 2, nullptr);
  const char *language = "";
  if (lua_isstring(L, 3))
    language = luaL_checkstring(L, 3);

  Dialog **dlg = (Dialog **) lua_newuserdata(L, sizeof(Dialog *));
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
  virtual int add(lua_State *L);
  virtual int execute(lua_State *L);
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

PMenu::PMenu(HWND parent)
{
  hMenu = CreatePopupMenu();
  hwnd = parent;
}

PMenu::~PMenu()
{
  if (hMenu)
    DestroyMenu(hMenu);
  hMenu = nullptr;
  for (int i = 0; i < int(bitmaps.size()); ++i)
    DeleteObject(bitmaps[i]);
}

int PMenu::execute(lua_State *L)
{
  int vx = (int)luaL_checkinteger(L, 2);
  int vy = (int)luaL_checkinteger(L, 3);
  int result = TrackPopupMenu(hMenu,
			      TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON,
			      vx, vy, 0, hwnd, nullptr);
  if (1 <= result && result <= int(items.size())) {
    result -= 1;
    lua_pushstring(L, items[result].name.c_str());
    lua_pushinteger(L, items[result].itemIndex);
    if (items[result].itemName.c_str())
      lua_pushstring(L, items[result].itemName.c_str());
    else
      lua_pushstring(L, "");
    return 3;
  }
  return 0;
}

static HBITMAP colorIcon(double red, double green, double blue)
{
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
    for (int x = 0; x < cx; ++x) {
      SetPixel(memDC, x, y, rgb);
    }
  }
  DeleteDC(memDC);
  ReleaseDC(nullptr, hdc);
  return bm;
}

int PMenu::add(lua_State *L)
{
  const char *name = luaL_checklstring(L, 2, nullptr);
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
      luaL_argcheck(L, lua_istable(L, 5), 5,
		    "argument is not a function or table");
    const char *current = nullptr;
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
      const char *item = lua_tolstring(L, -1, nullptr);
      if (hastable) {
	lua_rawgeti(L, 5, i);
	luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
      } else if (hasmap) {
	lua_pushvalue(L, 5);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -3);  // name
	luacall(L, 2, 1);      // function returns label
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
      if (hascheck && !strcmp(item, current))
	CheckMenuItem(sm, id, MF_CHECKED);

      if (hascolor) {
	lua_pushvalue(L, 6);   // function
	lua_pushnumber(L, i);  // index
 	lua_pushvalue(L, -4);  // name
	luacall(L, 2, 3);      // function returns red, green, blue
	double red = luaL_checknumber(L, -3);
	double green = luaL_checknumber(L, -2);
	double blue = luaL_checknumber(L, -1);
	lua_pop(L, 3);         // pop result
	HBITMAP bits = colorIcon(red, green, blue);
	bitmaps.push_back(bits);
	SetMenuItemBitmaps(sm, id, MF_BYCOMMAND, bits, bits);
      }
      lua_pop(L, 2); // item, text
    }
    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR) sm, title.data());
  }
  return 0;
}

// --------------------------------------------------------------------

static int menu_constructor(lua_State *L)
{
  HWND hwnd = check_winid(L, 1);
  Menu **m = (Menu **) lua_newuserdata(L, sizeof(Menu *));
  *m = nullptr;
  luaL_getmetatable(L, "Ipe.menu");
  lua_setmetatable(L, -2);
  *m = new PMenu(hwnd);
  return 1;
}

// --------------------------------------------------------------------

class PTimer : public Timer {
public:
  PTimer(lua_State *L0, int lua_object, const char *method);
  virtual ~PTimer();

  virtual int setInterval(lua_State *L);
  virtual int active(lua_State *L);
  virtual int start(lua_State *L);
  virtual int stop(lua_State *L);

protected:
  void elapsed();
  static void CALLBACK TimerProc(HWND hwnd, UINT uMsg,
				 UINT_PTR idEvent, DWORD dwTime);
  static std::vector<PTimer *> all_timers;

protected:
  UINT_PTR iTimer;
  UINT iInterval;
};

std::vector<PTimer *> PTimer::all_timers;

void CALLBACK PTimer::TimerProc(HWND, UINT, UINT_PTR id, DWORD)
{
  for (int i = 0; i < int(all_timers.size()); ++i) {
    if (id == all_timers[i]->iTimer) {
      all_timers[i]->elapsed();
      return;
    }
  }
}

PTimer::PTimer(lua_State *L0, int lua_object, const char *method)
  : Timer(L0, lua_object, method)
{
  iTimer = 0;
  iInterval = 0;
  all_timers.push_back(this);
}

PTimer::~PTimer()
{
  if (iTimer)
    KillTimer(nullptr, iTimer);
  // remove it from all_timers
  for (int i = 0; i < int(all_timers.size()); ++i) {
    if (all_timers[i] == this) {
      all_timers.erase(all_timers.begin() + i);
      return;
    }
  }
}

void PTimer::elapsed()
{
  callLua();
  if (iSingleShot) {
    KillTimer(nullptr, iTimer);
    iTimer = 0;
  }
}

int PTimer::setInterval(lua_State *L)
{
  int t = (int)luaL_checkinteger(L, 2);
  iInterval = t;
  if (iTimer)
    SetTimer(nullptr, iTimer, iInterval, TimerProc);
  return 0;
}

int PTimer::active(lua_State *L)
{
  lua_pushboolean(L, (iTimer != 0));
  return 1;
}

int PTimer::start(lua_State *L)
{
  if (iTimer == 0)
    iTimer = SetTimer(nullptr, 0, iInterval, TimerProc);
  return 0;
}

int PTimer::stop(lua_State *L)
{
  if (iTimer) {
    KillTimer(nullptr, iTimer);
    iTimer = 0;
  }
  return 0;
}

// --------------------------------------------------------------------

static int timer_constructor(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  const char *method = luaL_checklstring(L, 2, nullptr);

  Timer **t = (Timer **) lua_newuserdata(L, sizeof(Timer *));
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

static COLORREF custom[16] = {
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff,
  0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff };

static int ipeui_getColor(lua_State *L)
{
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

static int ipeui_fileDialog(lua_State *L)
{
  static const char * const typenames[] = { "open", "save", nullptr };
  HWND hwnd = check_winid(L, 1);
  int type = luaL_checkoption(L, 2, nullptr, typenames);
  WString caption(luaL_checklstring(L, 3, nullptr));
  if (!lua_istable(L, 4))
    luaL_argerror(L, 4, "table expected for filters");
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

  const char *dir = nullptr;
  if (!lua_isnoneornil(L, 5))
    dir = luaL_checklstring(L, 5, nullptr);
  const char *name = nullptr;
  if (!lua_isnoneornil(L, 6))
    name = luaL_checklstring(L, 6, nullptr);
  int selected = 0;
  if (!lua_isnoneornil(L, 7))
    selected = luaL_checkinteger(L, 7);

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
  if (!result)
    return 0;

  std::string s = wideToUtf8(ofn.lpstrFile);
  lua_pushstring(L, s.c_str());
  lua_pushinteger(L, ofn.nFilterIndex);
  return 2;
}

// --------------------------------------------------------------------

static int ipeui_messageBox(lua_State *L)
{
  static const char * const options[] =  {
    "none", "warning", "information", "question", "critical", nullptr };
  static const char * const buttontype[] = {
    "ok", "okcancel", "yesnocancel", "discardcancel",
    "savediscardcancel", nullptr };

  HWND hwnd = check_winid(L, 1);
  int type = luaL_checkoption(L, 2, "none", options);
  const char *text = luaL_checklstring(L, 3, nullptr);
  const char *details = nullptr;
  if (!lua_isnoneornil(L, 4))
    details = luaL_checklstring(L, 4, nullptr);
  int buttons = 0;
  if (lua_isnumber(L, 5))
    buttons = (int)luaL_checkinteger(L, 5);
  else if (!lua_isnoneornil(L, 5))
    buttons = luaL_checkoption(L, 5, nullptr, buttontype);

  UINT uType = MB_APPLMODAL;
  switch (type) {
  case 0:
  default:
    break;
  case 1:
    uType |= MB_ICONWARNING;
    break;
  case 2:
    uType |= MB_ICONINFORMATION;
    break;
  case 3:
    uType |= MB_ICONQUESTION;
    break;
  case 4:
    uType |= MB_ICONERROR;
    break;
  }

  switch (buttons) {
  case 0:
  default:
    uType |= MB_OK;
    break;
  case 1:
    uType |= MB_OKCANCEL;
    break;
  case 2:
    uType |= MB_YESNOCANCEL;
    break;
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
  case IDYES:
    lua_pushnumber(L, 1);
    break;
  case IDNO:
  case IDIGNORE:
    lua_pushnumber(L, 0);
    break;
  case IDCANCEL:
  default:
    lua_pushnumber(L, -1);
    break;
  }
  return 1;
}

// --------------------------------------------------------------------

struct SDialogHandle {
  HWND hwnd;
  HANDLE thread;
};

BOOL CALLBACK waitDialogProc(HWND hwnd, UINT message,
			     WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_INITDIALOG: {
    SDialogHandle *d = (SDialogHandle *) lParam;
    d->hwnd = hwnd;
    if (d->thread) {
      ResumeThread(d->thread);
      // delay showing the dialog by 300ms
      // if Latex is fast, the dialog never gets shown
      Sleep(300);
    }
    return TRUE; }
  default:
    return FALSE;
  }
}

VOID CALLBACK waitCallback(PVOID lpParameter, BOOLEAN timerOrWaitFired)
{
  SDialogHandle *d = (SDialogHandle *) lpParameter;
  EndDialog(d->hwnd, 1);
}

DWORD CALLBACK waitLuaThreadProc(LPVOID lpParameter)
{
  lua_State *L = (lua_State *) lpParameter;
  lua_callk(L, 0, 0, 0, nullptr);
  return 0;
}

static int ipeui_wait(lua_State *L)
{
  Dialog **dlg = (Dialog **) luaL_testudata(L, 1, "Ipe.dialog");
  HWND parent = dlg ? (*dlg)->winId() : check_winid(L, 1);

  const char *cmd = nullptr;
  if (!lua_isfunction(L, 2))
    cmd = luaL_checklstring(L, 2, nullptr);

  const char *label = "Waiting for external editor";
  if (lua_isstring(L, 3))
    label = luaL_checklstring(L, 3, nullptr);

  std::vector<short> t;

  // Dialog flags
  buildFlags(t, WS_POPUP | WS_BORDER | DS_SHELLFONT |
	     WS_SYSMENU | DS_MODALFRAME | WS_CAPTION);
  t.push_back(1);
  t.push_back(0); // offset of popup-window from parent window
  t.push_back(0);
  t.push_back(240);
  t.push_back(60);
  // menu
  t.push_back(0);
  // class
  t.push_back(0);
  // title
  buildString(t, "Ipe: waiting");
  // for DS_SHELLFONT
  t.push_back(10);
  buildString(t,"MS Shell Dlg");
  if (t.size() % 2 != 0)
    t.push_back(0);
  buildFlags(t, WS_CHILD|WS_VISIBLE|SS_LEFT);
  t.push_back(40);
  t.push_back(20);
  t.push_back(120);
  t.push_back(20);
  t.push_back(IDBASE);
  buildControl(t, 0x0082, label);

  SDialogHandle dialogHandle = { nullptr, nullptr };

  HANDLE waitHandle;

  if (cmd) {
    // call external process

    // Declare and initialize process blocks
    PROCESS_INFORMATION processInformation;
    STARTUPINFOW startupInfo;
    memset(&processInformation, 0, sizeof(processInformation));
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    WString wcmd(cmd);

    int result = CreateProcessW(nullptr, wcmd.data(), nullptr, nullptr, FALSE,
				NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW,
				nullptr, nullptr, &startupInfo, &processInformation);
    if (result == 0)
      return 0;

    RegisterWaitForSingleObject(&waitHandle, processInformation.hProcess, waitCallback, &dialogHandle,
				INFINITE, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);

    DialogBoxIndirectParamW((HINSTANCE) GetWindowLongPtr(parent, GWLP_HINSTANCE),
			    (LPCDLGTEMPLATE) t.data(),
			    parent, (DLGPROC) waitDialogProc,
			    (LPARAM) &dialogHandle);

    UnregisterWait(waitHandle);
    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

  } else {
    // call Lua function

    dialogHandle.thread = CreateThread(nullptr, 0, &waitLuaThreadProc, L, CREATE_SUSPENDED, nullptr);
    if (dialogHandle.thread == nullptr)
      return 0;

    RegisterWaitForSingleObject(&waitHandle, dialogHandle.thread, waitCallback, &dialogHandle,
				INFINITE, WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE);

    lua_pushvalue(L, 2);

    DialogBoxIndirectParamW((HINSTANCE) GetWindowLongPtr(parent, GWLP_HINSTANCE),
			    (LPCDLGTEMPLATE) t.data(),
			    parent, (DLGPROC) waitDialogProc,
			    (LPARAM) &dialogHandle);
    UnregisterWait(waitHandle);
    CloseHandle(dialogHandle.thread);
  }
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State *L)
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  char buf[32];
  sprintf(buf, "%04d%02d%02d%02d%02d%02d",
	  st.wYear, st.wMonth, st.wDay,
	  st.wHour, st.wMinute, st.wSecond);
  lua_pushstring(L, buf);
  return 1;
}

static int ipeui_startBrowser(lua_State *L)
{
  const char *url = luaL_checklstring(L, 1, nullptr);
  long long int res = (long long int) ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
  lua_pushboolean(L, (res >= 32));
  return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
  { "Dialog", dialog_constructor },
  { "Menu", menu_constructor },
  { "Timer", timer_constructor },
  { "getColor", ipeui_getColor },
  { "fileDialog", ipeui_fileDialog },
  { "messageBox", ipeui_messageBox },
  { "waitDialog", ipeui_wait },
  { "currentDateTime", ipeui_currentDateTime },
  { "startBrowser", ipeui_startBrowser },
  { "downloadFileIfIpeWeb", ipeui_downloadFileIfIpeWeb },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

int luaopen_ipeui(lua_State *L)
{
  luaL_newlib(L, ipeui_functions);
  lua_setglobal(L, "ipeui");
  luaopen_ipeui_common(L);
  return 0;
}

// --------------------------------------------------------------------
