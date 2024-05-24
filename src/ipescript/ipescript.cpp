// --------------------------------------------------------------------
// ipescript.cpp
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

using namespace ipe;
using namespace ipelua;

#ifdef WIN32
#define IPEPATHSEP (';')
#else
#define IPEPATHSEP (':')
#endif

// --------------------------------------------------------------------

static int traceback (lua_State *L)
{
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
  lua_getfield(L, -1, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 3);
    return 1;
  }
  lua_pushvalue(L, 1);             // pass error message
  lua_pushinteger(L, 2);           // skip this function and traceback
  luacall(L, 2, 1);                // call debug.traceback
  return 1;
}

static void setup_config(lua_State *L, const char *var, const char *conf)
{
#ifdef IPEBUNDLE
  push_string(L, Platform::ipeDir(conf));
#else
  lua_pushstring(L, conf);
#endif
  lua_setfield(L, -2, var);
}

// --------------------------------------------------------------------

static lua_State *setup_lua()
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaopen_ipe(L);
  return L;
}

static void setup_globals(lua_State *L)
{
  lua_getglobal(L, "package");
  const char *pscripts = getenv("IPESCRIPTS");
  if (pscripts) {
    String scripts(pscripts);
    String s;
    int i = 0;
    while (i < scripts.size()) {
      int j = i;
      while (i < scripts.size() && scripts[i] != IPEPATHSEP)
	i += 1;
      String d = scripts.substr(j, i-j);
      if (d == "_")
#ifdef IPEBUNDLE
	d = Platform::ipeDir("scripts", nullptr);
#else
	d = IPESCRIPTDIR;
#endif
      d += "/?.lua";
      if (!s.empty())
	s += ';';
      s += d;
      i += 1;
    }
    ipeDebug("package.path = %s", s.z());
    lua_pushstring(L, s.z());
  } else {
    String s = "./?.lua;";
#ifndef WIN32
    const char *home = getenv("HOME");
    if (home) {
      s += home;
      s += "/.ipe/scripts/?.lua;";
#ifdef __APPLE__
      s += home;
      s += "/Library/Ipe/Scripts/?.lua;";
#endif
    }
#endif
#ifdef IPEBUNDLE
    s += Platform::ipeDir("scripts", "?.lua");
#else
    s += IPESCRIPTDIR "/?.lua";
#endif
    ipeDebug("package.path = %s", s.z());
    lua_pushstring(L, s.z());
  }
  lua_setfield(L, -2, "path");

  lua_newtable(L);  // config table
#if defined(WIN32)
  lua_pushliteral(L, "win");
#elif defined(__APPLE__)
  lua_pushliteral(L, "apple");
#else
  lua_pushliteral(L, "unix");
#endif
  lua_setfield(L, -2, "platform");

#ifdef IPEBUNDLE
  setup_config(L, "system_styles", "styles");
#else
  setup_config(L, "system_styles", IPESTYLEDIR);
#endif
  push_string(L, Platform::latexDirectory());
  lua_setfield(L, -2, "latexdir");

  lua_pushfstring(L, "Ipe %d.%d.%d",
		  IPELIB_VERSION / 10000,
		  (IPELIB_VERSION / 100) % 100,
		  IPELIB_VERSION % 100);
  lua_setfield(L, -2, "version");

  lua_setglobal(L, "config");
}

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr,
	  "Usage: ipescript <script> { <arguments> }\n"
	  "Ipescript runs a script from your scripts directories with\n"
	  "the given arguments.\n"
	  "Do not include the .lua extension in the script name.\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  Platform::initLib(IPELIB_VERSION);

  if (argc < 2)
    usage();

  String s = "require \"";
  s += argv[1];
  s += "\"";

  lua_State *L = setup_lua();

  // create table with arguments
  lua_createtable(L, 0, argc - 2);
  for (int i = 2; i < argc; ++i) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - 1);
  }
  lua_setglobal(L, "argv");

  setup_globals(L);

  // run script
  lua_pushcfunction(L, traceback);
  assert(luaL_loadstring(L, s.z()) == 0);

  if (lua_pcallk(L, 0, 0, -2, 0, nullptr)) {
    const char *errmsg = lua_tolstring(L, -1, nullptr);
    fprintf(stderr, "%s\n", errmsg);
  }

  lua_close(L);
  return 0;
}

// --------------------------------------------------------------------
