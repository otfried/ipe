// --------------------------------------------------------------------
// uitest with Windows UI
// --------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <vector>

extern int luaopen_ipeui(lua_State * L);
extern void push_winid(lua_State * L, HWND hwnd);

// --------------------------------------------------------------------

static int traceback(lua_State * L) {
    if (!lua_isstring(L, 1)) /* 'message' not a string? */
	return 1;            /* keep it intact */
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_getfield(L, -1, "debug");
    if (!lua_istable(L, -1)) {
	lua_pop(L, 1);
	return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
	lua_pop(L, 2);
	return 1;
    }
    lua_pushvalue(L, 1);   // pass error message
    lua_pushinteger(L, 2); // skip this function and traceback
    lua_call(L, 2, 1);     // call debug.traceback
    return 1;
}

// --------------------------------------------------------------------

static const char * const actions[] = {
    "quit",  "dialog 1",   "collect garbage", "open",          "save",
    "color", "messagebox", "set clipboard",   "get clipboard", "start timer"};

#define NUM_ACTIONS (sizeof(actions) / sizeof(const char *))
#define IDBASE 9001

class AppUi {
public:
    static void init(HINSTANCE hInstance);
    static AppUi * create(HINSTANCE hInstance, lua_State * L0);
    void show(int nCmdShow);

    void cmd(int cmd);
    void showMenu(int x, int y);
    void showDialog();

    HWND windowId() const { return hwnd; }

private:
    AppUi();
    ~AppUi();

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam,
				    LPARAM lParam);
    static const wchar_t className[];

    HWND hwnd;
    lua_State * L;
};

const wchar_t AppUi::className[] = L"uitestWindowClass";

AppUi::AppUi() { L = 0; }

AppUi::~AppUi() { fprintf(stderr, "AppUi::~AppUi()\n"); }

// --------------------------------------------------------------------

void AppUi::showMenu(int x, int y) {
    lua_getglobal(L, "show_menu");
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    lua_call(L, 2, 0);
}

void AppUi::cmd(int cmd) {
    if (cmd == IDBASE) {
	PostMessage(hwnd, WM_CLOSE, 0, 0);
    } else {
	// all other actions call lua code
	lua_getglobal(L, "action");
	lua_pushstring(L, actions[cmd - IDBASE]);
	lua_call(L, 1, 0);
    }
}

// --------------------------------------------------------------------

LRESULT CALLBACK AppUi::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AppUi * ui = (AppUi *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE: {
	ui = new AppUi;
	ui->hwnd = hwnd;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ui);

	HMENU hMenu = CreateMenu();

	HMENU hSubMenu = CreatePopupMenu();
	for (int i = 0; i < int(NUM_ACTIONS); ++i)
	    AppendMenuA(hSubMenu, MF_STRING, IDBASE + i, actions[i]);
	AppendMenuA(hMenu, MF_STRING | MF_POPUP, UINT(hSubMenu), "&File");

	SetMenu(hwnd, hMenu);
    } break;
    case WM_COMMAND:
	if (ui) ui->cmd(LOWORD(wParam));
	break;
    case WM_RBUTTONUP:
	if (ui) {
	    POINT p;
	    p.x = LOWORD(lParam);
	    p.y = HIWORD(lParam);
	    ClientToScreen(hwnd, &p);
	    ui->showMenu(p.x, p.y);
	}
	break;
    case WM_CLOSE: DestroyWindow(hwnd); break;
    case WM_DESTROY:
	PostQuitMessage(0);
	delete ui;
	break;
    default: break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void AppUi::init(HINSTANCE hInstance) {
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
	MessageBoxA(NULL, "AppUi registration failed!", "Error!",
		    MB_ICONEXCLAMATION | MB_OK);
	exit(9);
    }
}

AppUi * AppUi::create(HINSTANCE hInstance, lua_State * L0) {
    HWND hwnd = CreateWindowExW(
	WS_EX_CLIENTEDGE, className, L"UI Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
	CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
	MessageBoxA(NULL, "AppUi window creation failed!", "Error!",
		    MB_ICONEXCLAMATION | MB_OK);
	exit(9);
    }
    AppUi * ui = (AppUi *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ui->L = L0;
    return ui;
}

void AppUi::show(int nCmdShow) {
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
}

// --------------------------------------------------------------------

int mainloop(lua_State * L) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
		   int nCmdShow) {
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_ipeui(L);

    AppUi::init(hInstance);

    AppUi * ui = AppUi::create(hInstance, L);

    ui->show(nCmdShow);

    push_winid(L, ui->windowId());
    lua_setglobal(L, "appui");

    lua_pushcfunction(L, traceback);
    int res = luaL_loadfile(L, "uitest.lua");
    if (res != 0) {
	fprintf(stderr, "Could not load uitest.lua: %d\n", res);
	return 1;
    }
    if (lua_pcall(L, 0, 0, -2)) {
	const char * errmsg = lua_tostring(L, -1);
	fprintf(stderr, "%s\n", errmsg);
	return 1;
    }

    lua_pushcfunction(L, mainloop);
    if (lua_pcall(L, 0, 0, -2)) {
	const char * errmsg = lua_tostring(L, -1);
	fprintf(stderr, "%s\n", errmsg);
	return 1;
    }

    lua_close(L);
    return 0;
}

// --------------------------------------------------------------------
