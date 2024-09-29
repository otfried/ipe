// --------------------------------------------------------------------
// Main function for JS
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

#include "ipebase.h"
#include "ipelua.h"

#include <cstdio>
#include <cstdlib>

#include "appui_js.h"
#include "ipecanvas_js.h"

using namespace ipe;
using namespace ipelua;

#include "main_common.i"

#include <emscripten/bind.h>

// --------------------------------------------------------------------

static void setup_globals(lua_State *L, int width, int height, double devicePixelRatio)
{
  lua_getglobal(L, "package");
  const char *luapath = getenv("IPELUAPATH");
  if (luapath)
    lua_pushstring(L, luapath);
  else {
#ifdef IPEBUNDLE
    push_string(L, Platform::ipeDir("lua", "?.lua"));
#else
    lua_pushliteral(L, IPELUADIR "/?.lua");
#endif
  }
  lua_setfield(L, -2, "path");

  lua_newtable(L);  // config table
  lua_pushliteral(L, "web");
  lua_setfield(L, -2, "platform");
  lua_pushliteral(L, "html");
  lua_setfield(L, -2, "toolkit");

#ifdef IPEBUNDLE
  setup_config(L, "system_styles", nullptr, "styles");
  setup_config(L, "system_ipelets", nullptr, "ipelets");
  setup_config(L, "docdir", "IPEDOCDIR", "doc");
#else
  setup_config(L, "system_styles", nullptr, IPESTYLEDIR);
  setup_config(L, "system_ipelets", nullptr, IPELETDIR);
  setup_config(L, "docdir", "IPEDOCDIR", IPEDOCDIR);
#endif

  setup_common_config(L);

  lua_createtable(L, 0, 2);
  lua_pushinteger(L, width);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, height);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "screen_geometry");

  lua_pushnumber(L, devicePixelRatio);
  lua_setfield(L, -2, "device_pixel_ratio");

  lua_setglobal(L, "config");

  lua_pushcfunction(L, ipe_tonumber);
  lua_setglobal(L, "tonumber");
}

// --------------------------------------------------------------------

// store these from the moment startIpe is called until we create the AppUi
static Canvas *theCanvas = nullptr;
static AppUi *theAppUi = nullptr;

AppUiBase *createAppUi(lua_State *L0, int model)
{
  theAppUi = new AppUi(L0, model, theCanvas);
  return theAppUi;
}

int mainloop(lua_State *L)
{
  // this does nothing, the browser already runs an event loop
  return 0;
}

AppUi *startIpe(Canvas *canvas, int width, int height, double dpr)
{
  theCanvas = canvas;

  Platform::initLib(IPELIB_VERSION);
  lua_State *L = setup_lua();

  // TODO: Should we support command line options?
  
  lua_createtable(L, 0, 1);
  lua_pushstring(L, "tiling.ipe");
  lua_rawseti(L, -2, 1);
  lua_setglobal(L, "argv");

  setup_globals(L, width, height, dpr);

  lua_run_ipe(L, mainloop);
  return theAppUi;
}

// --------------------------------------------------------------------

void ipeAction(AppUi *ui, std::string name)
{
  ui->action(String(name.c_str()));
}

// --------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(ipeui) {
  emscripten::class_<AppUi>("AppUi")
    .function("action", &ipeAction, emscripten::allow_raw_pointers());
  emscripten::function("startIpe", &startIpe, emscripten::allow_raw_pointers());
}

// --------------------------------------------------------------------
