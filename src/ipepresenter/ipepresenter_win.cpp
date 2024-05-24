// --------------------------------------------------------------------
// IpePresenter with Windows UI
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

#include "ipepresenter.h"
#include "ipepdfview_win.h"
#include "ipethumbs.h"

using namespace ipe;

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#define IDI_MYICON 1
#define IDD_INPUTBOX                    103
#define IDC_INPUTBOX_PROMPT             1000
#define IDC_INPUTBOX_EDIT 		1001

extern HBITMAP createBitmap(uint32_t *p, int w, int h);
extern int showPageSelectDialog(int width, int height, const char * title,
				HIMAGELIST il, std::vector<String> &labels, int startIndex);

// --------------------------------------------------------------------

BOOL setWindowText(HWND h, const char *s)
{
  int rw = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
  std::vector<wchar_t> ws(rw);
  MultiByteToWideChar(CP_UTF8, 0, s, -1, ws.data(), rw);
  std::vector<wchar_t> w;
  for (auto ch : ws) {
    if (ch != '\r') {
      if (ch == '\n')
	w.push_back('\r');
      w.push_back(ch);
    }
  }
  w.push_back(0);
  return SetWindowTextW(h, w.data());
}

// --------------------------------------------------------------------

class AppUi : public Presenter {
public:
  static void init(HINSTANCE hInstance);

  AppUi(HINSTANCE hInstance);
  void show(int nCmdShow);

  bool load(const char* fn);
  void cmd(int cmd);
  void toggleFullScreen();
  void fitBoxAll();
  void aboutIpePresenter();

private:
  void initUi();
  virtual ~AppUi();

  void setTimerValue();
  void setView();
  void setPdf();
  void setTime();
  void jumpTo();
  void selectPage();
  void layoutChildren();
  void zoomMain(int delta);
  void timerElapsed();
  void handlePdfViewMessage(int param, const Vector &pos);
  virtual void showType3Warning(const char *s) override;
  virtual void browseLaunch(bool launch, String dest) override;

  static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				  WPARAM wParam, LPARAM lParam);
  static const wchar_t className[];

  HWND hwnd;
  HWND hNotes;
  HWND hClock;

  HMENU hMenuBar;
  HFONT hFont;

  int iTime;
  bool iCountDown;
  bool iCountTime;

  PdfView *iCurrent;
  PdfView *iNext;
  PdfView *iScreen;

  int iMainPercentage;

  // for fullscreen mode
  bool iFullScreen;
  bool iWasMaximized;
  RECT iWindowRect;
  LONG iWindowStyle;
  LONG iWindowExStyle;

  HIMAGELIST iThumbNails;
};

const wchar_t AppUi::className[] = L"ipePresenterWindowClass";

AppUi::AppUi(HINSTANCE hInstance)
{
  iCurrent = nullptr;
  iNext = nullptr;
  iScreen = nullptr;
  iTime = 0;
  iCountDown = false;
  iCountTime = false;
  iMainPercentage = 70;
  iFullScreen = false;
  iThumbNails = nullptr;

  HWND hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, className, L"IpePresenter",
			      WS_OVERLAPPEDWINDOW,
			      CW_USEDEFAULT, CW_USEDEFAULT,
			      CW_USEDEFAULT, CW_USEDEFAULT,
			      nullptr, nullptr, hInstance, this);

  if (hwnd == nullptr) {
    MessageBoxA(nullptr, "AppUi window creation failed!", "Error!",
		MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void AppUi::initUi()
{
  hMenuBar = CreateMenu();

  /*
  HMENU hSubMenu = CreatePopupMenu();
  AppendMenuA(hSubMenu, MF_STRING, EOpen, "&Open\tCtrl+O");
  AppendMenuA(hSubMenu, MF_STRING, EQuit, "&Close\tCtrl+W");
  AppendMenuA(hMenu, MF_STRING | MF_POPUP, UINT_PTR(hSubMenu), "&File");
  */

  HMENU hSubMenu = CreatePopupMenu();
  AppendMenuA(hSubMenu, MF_STRING, EShowPresentation, "Show &Presentation\tF5");
  AppendMenuA(hSubMenu, MF_STRING, EFullScreen, "Full &Screen\tF11");
  AppendMenuA(hSubMenu, MF_STRING, EBlackout, "Blackout\tB");
  AppendMenuA(hSubMenu, MF_STRING, EZoomIn, "Larger\tCtrl++");
  AppendMenuA(hSubMenu, MF_STRING, EZoomOut, "Smaller\tCtrl+-");
  AppendMenuA(hMenuBar, MF_STRING | MF_POPUP, UINT_PTR(hSubMenu), "&View");

  hSubMenu = CreatePopupMenu();
  AppendMenuA(hSubMenu, MF_STRING, ESetTime, "&Set time\tL");
  AppendMenuA(hSubMenu, MF_STRING, EResetTime, "&Reset time\tR");
  AppendMenuA(hSubMenu, MF_STRING, ETimeCountdown, "Count down\t/");
  AppendMenuA(hSubMenu, MF_STRING, EToggleTimeCounting, "Count time\tT");
  AppendMenuA(hMenuBar, MF_STRING | MF_POPUP, UINT_PTR(hSubMenu), "&Time");

  hSubMenu = CreatePopupMenu();
  AppendMenuA(hSubMenu, MF_STRING, ENextView, "&Next view\tRight");
  AppendMenuA(hSubMenu, MF_STRING, EPreviousView, "&Previous view\tLeft");
  AppendMenuA(hSubMenu, MF_STRING, ENextPage, "&Next page\tN");
  AppendMenuA(hSubMenu, MF_STRING, EPreviousPage, "&Previous page\tP");
  AppendMenuA(hSubMenu, MF_STRING, EFirstView, "&First view\tHome");
  AppendMenuA(hSubMenu, MF_STRING, ELastView, "&Last view\tEnd");
  AppendMenuA(hSubMenu, MF_STRING, EJumpTo, "&Jump to\tJ");
  AppendMenuA(hSubMenu, MF_STRING, ESelectPage, "&Select page\tS");
  AppendMenuA(hMenuBar, MF_STRING | MF_POPUP, UINT_PTR(hSubMenu), "&Navigate");

  hSubMenu = CreatePopupMenu();
  AppendMenuA(hSubMenu, MF_STRING, EAbout, "&About IpePresenter");
  AppendMenuA(hMenuBar, MF_STRING | MF_POPUP, UINT_PTR(hSubMenu), "&Help");

  SetMenu(hwnd, hMenuBar);

  HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

  iCurrent = new PdfView(hwnd, hwnd, 0x00);
  iNext = new PdfView(hwnd, hwnd, 0x10);
  iScreen = new PdfView(nullptr, hwnd, 0x20, hInstance);
  iScreen->setBackground(Color(0, 0, 0));

  hNotes = CreateWindowExW(0, L"edit", nullptr,
			   WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
			   ES_READONLY | ES_LEFT | ES_MULTILINE |
			   ES_AUTOVSCROLL,
			   0, 0, 0, 0, hwnd,
			   nullptr, hInstance, nullptr);
  hClock = CreateWindowExW(0, L"static", nullptr,
			   WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER,
			   0, 0, 0, 0, hwnd,
			   nullptr, hInstance, nullptr);

  hFont = CreateFontW(48, 0, 0, 0, FW_DONTCARE,
		      FALSE, FALSE, FALSE, ANSI_CHARSET,
		      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		      DEFAULT_QUALITY,
		      DEFAULT_PITCH | FF_SWISS,
		      L"MS Shell Dlg");

  if (hFont != nullptr)
    SendMessage(hClock, WM_SETFONT, WPARAM (hFont), TRUE);

  SetTimer(hwnd, 1, 1000, nullptr);
}

AppUi::~AppUi()
{
  if (iThumbNails)
    ImageList_Destroy(iThumbNails);
  KillTimer(hwnd, 1);
  delete iScreen;
  ipeDebug("AppUi::~AppUi()");
}

// --------------------------------------------------------------------

void AppUi::layoutChildren()
{
  if (iCurrent == nullptr)
    return;  // no children yet

  RECT rc;
  GetClientRect(hwnd, &rc);

  int splitH = iMainPercentage * rc.right / 100;
  if (iPdf) {
    Rect box = mediaBox(-2);
    splitH = std::min(int(box.width() * rc.bottom / box.height()), splitH);
  }
  MoveWindow(iCurrent->windowId(), 0, 0, splitH, rc.bottom, TRUE);

  SIZE cts;
  int splitClock = 10 * rc.bottom / 100;
  HDC hdc = GetDC(hClock);
  if (hdc)
    SelectFont(hdc, hFont);
  if (hdc && GetTextExtentPoint32A(hdc, "1:23:45", 7, &cts)) {
    splitClock = cts.cy + 10;
  }

  int wid = rc.right - splitH - 10;
  MoveWindow(hClock, splitH + 10, 10, wid - 10, splitClock - 10, TRUE);

  int ht = rc.bottom / 2;  // some simple default
  if (iPdf) {
    Rect box = mediaBox(-2);
    ht = std::min(long(box.height() * wid / box.width()) + 10,
		  80 * (rc.bottom - splitClock - 10) / 100);
  }
  int splitV = rc.bottom - ht;

  MoveWindow(iNext->windowId(), splitH + 10, splitV + 10, wid, ht - 10, TRUE);
  MoveWindow(hNotes, splitH + 10, splitClock + 10,
	     wid, splitV - splitClock - 10, TRUE);
  InvalidateRect(hClock, nullptr, FALSE);
}

void AppUi::handlePdfViewMessage(int param, const Vector &pos)
{
  // ignore all messages before PDF file loaded
  if (!iPdf)
    return;
  int screen = param & 0xf0;
  // left button press on current view or screen view
  if ((param & 0x0f) == 1 && (screen == 0 || screen == 0x20)) {
    PdfView *source = (screen == 0x20) ? iScreen : iCurrent;
    const PdfDict *action = findLink(source->devToUser(pos));
    if (action) {
      interpretAction(action);
      setView();
      return;
    }
  }
  switch (param & 0x0f) {
  case 0:
    fitBoxAll();
    break;
  case 1:
    cmd(ENextView);
    break;
  case 2:
    cmd(EPreviousView);
    break;
  }
}

void AppUi::toggleFullScreen()
{
  HWND hwnd = iScreen->windowId();
  if (!IsWindowVisible(hwnd))
    return;
  if (!iFullScreen) {
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hmon, &mi))
      return;

    iWasMaximized = IsZoomed(hwnd);
    if (iWasMaximized)
      SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    GetWindowRect(hwnd, &iWindowRect);
    iWindowStyle = GetWindowLong(hwnd, GWL_STYLE);
    iWindowExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_STYLE,
		  iWindowStyle & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowLong(hwnd, GWL_EXSTYLE,
		  iWindowExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
				     WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
    SetWindowPos(hwnd, HWND_TOP,
		 mi.rcMonitor.left,
		 mi.rcMonitor.top,
		 mi.rcMonitor.right - mi.rcMonitor.left,
		 mi.rcMonitor.bottom - mi.rcMonitor.top,
		 SWP_SHOWWINDOW);
    iFullScreen = true;
  } else {
    SetWindowLong(hwnd, GWL_STYLE, iWindowStyle);
    SetWindowLong(hwnd, GWL_EXSTYLE, iWindowExStyle);

    SetWindowPos(hwnd, nullptr, iWindowRect.left, iWindowRect.top,
		 iWindowRect.right - iWindowRect.left,
		 iWindowRect.bottom - iWindowRect.top,
		 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (iWasMaximized)
      SendMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    iFullScreen = false;
  }
}

// --------------------------------------------------------------------

void AppUi::timerElapsed()
{
  if (iCountTime) {
    if (iCountDown) {
      if (iTime > 0)
	--iTime;
    } else
      ++iTime;
    setTime();
  }
}

// --------------------------------------------------------------------

bool AppUi::load(const char* fn)
{
  bool result = Presenter::load(fn);
  if (result) {
    setPdf();
    setView();
  }
  return result;
}

void AppUi::setPdf()
{
  iScreen->setPdf(iPdf.get(), iFonts.get());
  iCurrent->setPdf(iPdf.get(), iFonts.get());
  iNext->setPdf(iPdf.get(), iFonts.get());
}

void AppUi::setView()
{
  setViewPage(iScreen, iPdfPageNo);
  setViewPage(iCurrent, iPdfPageNo);
  setViewPage(iNext, iPdfPageNo < iPdf->countPages() - 1 ? iPdfPageNo + 1 : iPdfPageNo);

  setWindowText(hwnd, currentLabel().z());
  setWindowText(hNotes, iAnnotations[iPdfPageNo].z());
  setTime();
}

void AppUi::setTime()
{
  char buf[16];
  sprintf(buf, "%d:%02d:%02d", iTime / 3600, (iTime / 60) % 60,
	  iTime % 60);
  setWindowText(hClock, buf);
}

void AppUi::fitBoxAll()
{
  fitBox(mediaBox(-1), iCurrent);
  fitBox(mediaBox(-2), iNext);
  fitBox(mediaBox(-1), iScreen);
}

void AppUi::zoomMain(int delta)
{
  int nPerc = iMainPercentage + delta;
  if (50 <= nPerc && nPerc <= 80)
    iMainPercentage = nPerc;
  layoutChildren();
}

// --------------------------------------------------------------------

void AppUi::cmd(int cmd)
{
  // ipeDebug("Command %d", cmd);
  switch (cmd) {
  case EOpen:
    break;
  case EQuit:
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    break;
    //
  case EShowPresentation:
    if (IsWindowVisible(iScreen->windowId()))
      ShowWindow(iScreen->windowId(), SW_HIDE);
    else
      ShowWindow(iScreen->windowId(), SW_SHOWNOACTIVATE);
    break;
  case EFullScreen:
    toggleFullScreen();
    break;
  case EBlackout:
    iScreen->setBlackout(!iScreen->blackout());
    iScreen->updatePdf();
    break;
  case EZoomIn:
    zoomMain(+1);
    break;
  case EZoomOut:
    zoomMain(-1);
    break;
    //
  case EToggleTimeCounting:
    iCountTime = !iCountTime;
    CheckMenuItem(hMenuBar, EToggleTimeCounting, iCountTime ? MF_CHECKED : MF_UNCHECKED);
    break;
  case ETimeCountdown:
    iCountDown = !iCountDown;
    CheckMenuItem(hMenuBar, ETimeCountdown, iCountDown ? MF_CHECKED : MF_UNCHECKED);
    break;
  case ESetTime:
    setTimerValue();
    break;
    //
  case EResetTime:
    iTime = 0;
    setTime();
    break;
    //
  case ELeftMouse:
  case ENextView:
    nextView(+1);
    setView();
    break;
  case EOtherMouse:
  case EPreviousView:
    nextView(-1);
    setView();
    break;
  case ENextPage:
    nextPage(+1);
    setView();
    break;
  case EPreviousPage:
    nextPage(-1);
    setView();
    break;
  case EFirstView:
    firstView();
    setView();
    break;
  case ELastView:
    lastView();
    setView();
    break;
  case EJumpTo:
    jumpTo();
    break;
  case ESelectPage:
    selectPage();
    break;
  case EAbout:
    aboutIpePresenter();
    break;
  default:
    // unknown action
    return;
  }
}

void AppUi::browseLaunch(bool launch, String dest)
{
  ipeDebug("Launch %d %s", launch, dest.z());
  ShellExecuteA(nullptr, "open", dest.z(), nullptr, nullptr, SW_SHOWNORMAL);
}

// --------------------------------------------------------------------

static const char * const aboutText =
  "IpePresenter %d.%d.%d\n\n"
  "Copyright (c) 2020-2024 Otfried Cheong\n\n"
  "A presentation tool for giving PDF presentations "
  "created in Ipe or using beamer.\n"
  "Originally invented by Dmitriy Morozov, "
  "IpePresenter is now developed together with Ipe and released under the GNU Public License.\n"
  "See http://ipepresenter.otfried.org for details.\n\n"
  "If you are an IpePresenter fan and want to show others, have a look at the "
  "Ipe T-shirts (www.shirtee.com/en/store/ipe).\n\n"
  "Platinum and gold sponsors\n\n"
  " * Hee-Kap Ahn\n"
  " * Günter Rote\n"
  " * SCALGO\n"
  " * Martin Ziegler\n\n"
  "If you enjoy IpePresenter, feel free to treat the author on a cup of coffee at https://ko-fi.com/ipe7author.\n\n"
  "You can also become a member of the exclusive community of "
  "Ipe patrons (http://patreon.com/otfried). "
  "For the price of a cup of coffee per month you can make a meaningful contribution "
  "to the continuing development of IpePresenter and Ipe.";

void AppUi::aboutIpePresenter()
{
  std::vector<char> buf(strlen(aboutText) + 100);
  sprintf(buf.data(), aboutText,
	  IPELIB_VERSION / 10000,
	  (IPELIB_VERSION / 100) % 100,
	  IPELIB_VERSION % 100);

  const char *s = buf.data();
  int n = strlen(s);
  int rw = MultiByteToWideChar(CP_UTF8, 0, s, n, nullptr, 0);
  std::wstring wbuf;
  wbuf.resize(rw + 1, wchar_t(0));
  MultiByteToWideChar(CP_UTF8, 0, s, n, wbuf.data(), rw);

  MessageBoxW(hwnd, wbuf.data(), L"About IpePresenter",
	      MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
}

void AppUi::showType3Warning(const char *s)
{
  MessageBoxA(hwnd, s, "Type3 font detected",
	      MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
}

// --------------------------------------------------------------------

static String dialogInput;
static String dialogPrompt;

BOOL CALLBACK SetTimeProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  char buf[80];

  switch (message) {
  case WM_INITDIALOG:
    SetDlgItemTextA(hwndDlg, IDC_INPUTBOX_PROMPT, dialogPrompt.z());
    SetFocus(hwndDlg);
    break;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      dialogInput.erase();
      if (GetDlgItemTextA(hwndDlg, IDC_INPUTBOX_EDIT, buf, 80))
	dialogInput = String(buf);
      // Fall through
    case IDCANCEL:
      EndDialog(hwndDlg, wParam);
      return TRUE;
    }
  }
  return FALSE;
}

void AppUi::setTimerValue()
{
  HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
  dialogPrompt = "Enter time in minutes:";
  if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_INPUTBOX),
		hwnd, (DLGPROC) SetTimeProc) == IDOK) {
    Lex lex(dialogInput);
    int minutes = lex.getInt();
    iTime = 60 * minutes;
    setTime();
  }
}

void AppUi::jumpTo()
{
  HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
  dialogPrompt = "Enter page label:";
  if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_INPUTBOX),
		hwnd, (DLGPROC) SetTimeProc) == IDOK) {
    jumpToPage(dialogInput); // trim?
    setView();
  }
}

void AppUi::selectPage()
{
  constexpr int iconWidth = 250;
  // Create thumbnails
  if (iThumbNails == nullptr) {
    PdfThumbnail r(iPdf.get(), iconWidth);
    int nItems = iPdf->countPages();
    iThumbNails = ImageList_Create(r.width(), r.height(), ILC_COLOR32, nItems, 4);
    for (int i = 0; i < iPdf->countPages(); ++i) {
      Buffer bx = r.render(iPdf->page(i));
      HBITMAP b = createBitmap((uint32_t *) bx.data(), r.width(), r.height());
      ImageList_Add(iThumbNails, b, nullptr);
    }
  }

  const char *title = "IpePresenter: Select page";
  std::vector<String> labels;
  for (int i = 0; i < iPdf->countPages(); ++i)
    labels.push_back(pageLabel(i));

  int width = 800;
  int height = 600;
  RECT rect;
  if (GetWindowRect(hwnd, &rect)) {
    width = rect.right - rect.left - 50;
    height = rect.bottom - rect.top - 80;
  }
  int sel = showPageSelectDialog(width, height, title, iThumbNails, labels, iPdfPageNo);
  if (sel >= 0) {
    iPdfPageNo = sel;
    setView();
  }
}

// --------------------------------------------------------------------

LRESULT CALLBACK AppUi::wndProc(HWND hwnd, UINT message, WPARAM wParam,
				LPARAM lParam)
{
  AppUi *ui = (AppUi*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (message) {
  case WM_CREATE:
    {
      LPCREATESTRUCT p = (LPCREATESTRUCT) lParam;
      ui = (AppUi *) p->lpCreateParams;
      ui->hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ui);
      ui->initUi();
    }
    break;
  case WM_INITMENUPOPUP:
    if (ui && lParam == 0) {
      CheckMenuItem(ui->hMenuBar, EShowPresentation,
		    IsWindowVisible(ui->iScreen->windowId()) ? MF_CHECKED : MF_UNCHECKED);
      CheckMenuItem(ui->hMenuBar, EFullScreen, ui->iFullScreen ? MF_CHECKED : MF_UNCHECKED);
    }
    break;
  case WM_COMMAND:
    if (ui)
      ui->cmd(LOWORD(wParam));
    break;
  case WM_SIZE:
    if (ui)
      ui->layoutChildren();
    break;
  case WM_TIMER:
    if (ui)
      ui->timerElapsed();
    break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    delete ui;
    break;
  case PdfView::WM_PDFVIEW:
    if (ui) {
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      ui->handlePdfViewMessage(wParam, Vector(x, y));
    }
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

void AppUi::init(HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra	 = 0;
  wc.cbWndExtra	 = 0;
  wc.hInstance	 = hInstance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = className;
  wc.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MYICON));
  wc.hIconSm =
    HICON(LoadImage(GetModuleHandle(nullptr),
		    MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, 0));

  if (!RegisterClassExW(&wc)) {
    MessageBoxA(nullptr, "AppUi registration failed!", "Error!",
		MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  PdfView::init(hInstance);
}

void AppUi::show(int nCmdShow)
{
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);
}

// --------------------------------------------------------------------

static String getFileName()
{
  OPENFILENAMEW ofn;
  wchar_t szFileName[MAX_PATH] = L"";
  ZeroMemory(&ofn, sizeof(ofn));

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr; // hwnd?
  ofn.lpstrFilter = L"PDF Files\0*.pdf\0All Files\0*.*\0";
  ofn.lpstrFile = szFileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
  ofn.lpstrDefExt = L"pdf";

  if (GetOpenFileNameW(&ofn))
    return String(szFileName);
  return String();
}

// --------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
  Platform::initLib(IPELIB_VERSION);

  AppUi::init(hInstance);

  AppUi *ui = new AppUi(hInstance);

  String fname { lpCmdLine };

  if (fname.empty())
    fname = getFileName();

  if (fname.empty() || !ui->load(fname.z())) {
    MessageBoxA(nullptr, "Failed to load PDF file!", "IpePresenter Error!",
		MB_ICONEXCLAMATION | MB_OK);
    return 9;
  }

  ui->show(nCmdShow);

  ACCEL accel[] = {
    { FVIRTKEY, VK_PRIOR, AppUi::EPreviousView },
    { FVIRTKEY, VK_NEXT, AppUi::ENextView },
    { FVIRTKEY, VK_LEFT, AppUi::EPreviousView },
    { FVIRTKEY, VK_RIGHT, AppUi::ENextView },
    { FVIRTKEY, VK_UP, AppUi::EPreviousView },
    { FVIRTKEY, VK_DOWN, AppUi::ENextView },
    { FVIRTKEY|FCONTROL|FSHIFT, 0xbb, AppUi::EZoomIn },
    { FVIRTKEY|FCONTROL, 0xbd, AppUi::EZoomOut },
    { FVIRTKEY, 'N', AppUi::ENextPage },
    { FVIRTKEY, 'P', AppUi::EPreviousPage },
    { FVIRTKEY, 'T', AppUi::EToggleTimeCounting },
    { FVIRTKEY, 'J', AppUi::EJumpTo },
    { FVIRTKEY, 'S', AppUi::ESelectPage },
    { FVIRTKEY, 'L', AppUi::ESetTime },
    { FVIRTKEY, 'R', AppUi::EResetTime },
    { FVIRTKEY, VK_OEM_2, AppUi::ETimeCountdown },
    { FVIRTKEY, VK_HOME, AppUi::EFirstView },
    { FVIRTKEY, VK_END, AppUi::ELastView },
    { FVIRTKEY, VK_F11, AppUi::EFullScreen },
    { FVIRTKEY, VK_F5, AppUi::EShowPresentation },
    { FVIRTKEY, 'B', AppUi::EBlackout },
  };
  HACCEL hAccel = CreateAcceleratorTable(accel, sizeof(accel)/sizeof(ACCEL));

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return msg.wParam;
}

// --------------------------------------------------------------------
