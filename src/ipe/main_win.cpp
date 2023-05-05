// --------------------------------------------------------------------
// Main function for Win32
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

#include "ipebase.h"
#include "ipelua.h"

#include <windows.h>
#include <shlobj.h>

#include "appui_win.h"

#include <commctrl.h>

using namespace ipe;
using namespace ipelua;

#include "main_common.i"

// --------------------------------------------------------------------

static void setup_win_config(lua_State *L, const char *var,
			     int folderId)
{
  String s;
  wchar_t szPath[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, folderId, nullptr, 0, szPath)))
    s = String(szPath);
  else
    s = "C:";
  push_string(L, s);
  lua_setfield(L, -2, var);
}

static void setup_globals(lua_State *L)
{
  lua_getglobal(L, "package");
  const char *luapath = getenv("IPELUAPATH");
  if (luapath)
    lua_pushstring(L, luapath);
  else
    push_string(L, Platform::ipeDir("lua", "?.lua"));
  lua_setfield(L, -2, "path");

  lua_newtable(L);  // config table
  lua_pushliteral(L, "win");
  lua_setfield(L, -2, "platform");
  lua_pushliteral(L, "win");
  lua_setfield(L, -2, "toolkit");

  setup_config(L, "system_styles", nullptr, "styles");
  setup_config(L, "system_ipelets", nullptr, "ipelets");
  setup_config(L, "docdir", "IPEDOCDIR", "doc");
  setup_win_config(L, "desktop", CSIDL_DESKTOP);
  setup_win_config(L, "documents", CSIDL_PERSONAL);

  setup_common_config(L);

  int cx = GetSystemMetrics(SM_CXSCREEN);
  int cy = GetSystemMetrics(SM_CYSCREEN);
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, cx);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, cy);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "screen_geometry");

  lua_setglobal(L, "config");

  lua_pushcfunction(L, ipe_tonumber);
  lua_setglobal(L, "tonumber");
}

// --------------------------------------------------------------------

static HACCEL makeAccel(lua_State *L, int arg)
{
  luaL_argcheck(L, lua_istable(L, arg), arg, "Argument is not a table");
  int no = lua_rawlen(L, arg);
  luaL_argcheck(L, no > 0, arg, "Table must have at least one shortcut");
  std::vector<ACCEL> accel;
  for (int i = 1; i <= no; i += 2) {
    lua_rawgeti(L, arg, i);
    lua_rawgeti(L, arg, i+1);
    ACCEL a;
    int key = luaL_checkinteger(L, -2);
    a.key = key & 0xffff;
    a.cmd = luaL_checkinteger(L, -1);
    a.fVirt = (((key & 0x10000) ? FALT : 0) |
	       ((key & 0x20000) ? FCONTROL : 0) |
	       ((key & 0x40000) ? FSHIFT : 0) |
	       ((key & 0x80000) ? 0 : FVIRTKEY));
    accel.push_back(a);
    if (0x30 <= a.key && a.key <= 0x39 && (a.fVirt & FVIRTKEY)) {
      a.key += 0x30;
      accel.push_back(a);  // add numpad version
    }
    lua_pop(L, 2);
  }

  HACCEL hAccel = CreateAcceleratorTable(accel.data(), accel.size());
  return hAccel;
}

int mainloop(lua_State *L)
{
  HACCEL hAccelAll = makeAccel(L, 1);
  HACCEL hAccelSub = makeAccel(L, 2);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    HWND target = GetAncestor(msg.hwnd, GA_ROOT);
    HACCEL acc = AppUi::isDrawing(target) ? hAccelSub : hAccelAll;
    if (!TranslateAccelerator(target, acc, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
  Platform::initLib(IPELIB_VERSION);

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC   = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES |
    ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&icex);

  AppUi::init(hInstance, nCmdShow);

  lua_State *L = setup_lua();

  // setup command line argument
  wchar_t *w = GetCommandLineW();
  int argc;
  wchar_t **argv = CommandLineToArgvW(w, &argc);

  // create table with arguments
  lua_createtable(L, 0, argc - 1);
  for (int i = 1; i < argc; ++i) {
    String p(argv[i]);
    lua_pushstring(L, p.z());
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "argv");

  setup_globals(L);

  lua_run_ipe(L, mainloop);

  lua_close(L);
  return 0;
}

// --------------------------------------------------------------------
