// --------------------------------------------------------------------
// Main function
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

#include <cstdio>
#include <cstdlib>

#include <gtk/gtk.h>

using namespace ipe;
using namespace ipelua;

#include "main_common.i"

// --------------------------------------------------------------------

static void setup_globals(lua_State *L)
{
  lua_getglobal(L, "package");
  const char *luapath = getenv("IPELUAPATH");
  if (luapath)
    lua_pushstring(L, luapath);
  else
    lua_pushliteral(L, IPELUADIR "/?.lua");
  lua_setfield(L, -2, "path");

  lua_newtable(L);  // config table
  lua_pushliteral(L, "unix");
  lua_setfield(L, -2, "platform");
  lua_pushliteral(L, "gtk");
  lua_setfield(L, -2, "toolkit");

  setup_config(L, "system_styles", 0, IPESTYLEDIR);
  setup_config(L, "system_ipelets", 0, IPELETDIR);
  setup_config(L, "docdir", "IPEDOCDIR", IPEDOCDIR);

  push_string(L, Platform::latexDirectory());
  lua_setfield(L, -2, "latexdir");
  push_string(L, Platform::latexPath());
  lua_setfield(L, -2, "latexpath");
  push_string(L, ipeIconDirectory());
  lua_setfield(L, -2, "icons");

  lua_pushfstring(L, "Ipe %d.%d.%d",
		  IPELIB_VERSION / 10000,
		  (IPELIB_VERSION / 100) % 100,
		  IPELIB_VERSION % 100);
  lua_setfield(L, -2, "version");

  GdkScreen* screen = gdk_screen_get_default();
  int width = gdk_screen_get_width(screen);
  int height = gdk_screen_get_height(screen);
  ipeDebug("Screen resolution is (%d x %d)", width, height);

  lua_createtable(L, 0, 2);
  lua_pushinteger(L, width);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, height);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "screen_geometry");

  lua_setglobal(L, "config");

  lua_pushcfunction(L, ipe_tonumber);
  lua_setglobal(L, "tonumber");
}

// --------------------------------------------------------------------

int mainloop(lua_State *L)
{
  gtk_main();
  return 0;
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);
  gtk_init(&argc, &argv);
  lua_State *L = setup_lua();

  // create table with arguments
  lua_createtable(L, 0, argc - 1);
  for (int i = 1; i < argc; ++i) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "argv");

  setup_globals(L);

  lua_run_ipe(L, mainloop);

  lua_close(L);
  return 0;
}

// --------------------------------------------------------------------
