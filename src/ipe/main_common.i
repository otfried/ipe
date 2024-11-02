// --------------------------------------------------------------------
// Common code for all platforms
// --------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>

// for version info only
#include "ipefonts.h"
#include <zlib.h>

extern int luaopen_ipeui(lua_State *L);
extern int luaopen_appui(lua_State *L);

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
  lua_pushvalue(L, 1);    // pass error message
  lua_pushinteger(L, 2);  // skip this function and traceback
  luacall(L, 2, 1);       // call debug.traceback
  return 1;
}

static void setup_common_config(lua_State *L)
{
  push_string(L, Fonts::freetypeVersion());
  lua_setfield(L, -2, "freetype_version");
  lua_pushfstring(L, "%s / %s", CAIRO_VERSION_STRING, cairo_version_string());
  lua_setfield(L, -2, "cairo_version");
  lua_pushliteral(L, ZLIB_VERSION);
  lua_setfield(L, -2, "zlib_version");
  push_string(L, Platform::spiroVersion());
  lua_setfield(L, -2, "spiro_version");
  push_string(L, Platform::gslVersion());
  lua_setfield(L, -2, "gsl_version");

  push_string(L, Platform::latexPath());
  lua_setfield(L, -2, "latexpath");

  lua_pushfstring(L, "Ipe %d.%d.%d", 
		  IPELIB_VERSION / 10000,
		  (IPELIB_VERSION / 100) % 100,
		  IPELIB_VERSION % 100);
  lua_setfield(L, -2, "version");

  push_string(L, Platform::ipeDrive());
  lua_setfield(L, -2, "ipedrive");
} 

// --------------------------------------------------------------------

/* Replacement for Lua's tonumber function,
   which is locale-dependent. */
static int ipe_tonumber(lua_State *L)
{
  const char *s = luaL_checklstring(L, 1, nullptr);
  int iValue;
  double dValue;
  int result = Platform::toNumber(s, iValue, dValue);
  switch (result) {
  case 2:
    lua_pushnumber(L, dValue);
    break;
  case 1:
    lua_pushinteger(L, iValue);
    break;
  case 0:
  default:
    lua_pushnil(L);
  }
  return 1;
}

static lua_State *setup_lua()
{
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaopen_ipe(L);
  luaopen_ipeui(L);
  luaopen_appui(L);
  return L;
}

static bool lua_run_ipe(lua_State *L, lua_CFunction fn)
{
  lua_pushcfunction(L, fn);
  lua_setglobal(L, "mainloop");
  
  // run Ipe
  lua_pushcfunction(L, traceback);
  assert(luaL_loadstring(L, "require \"main\"") == 0);

  if (lua_pcallk(L, 0, 0, -2, 0, nullptr)) {
    const char *errmsg = lua_tolstring(L, -1, nullptr);
    fprintf(stderr, "%s\n", errmsg);
    return false;
  }
  return true;
}

// --------------------------------------------------------------------
