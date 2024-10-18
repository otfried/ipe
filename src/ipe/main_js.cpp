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
#include "controls_js.h"
#include "ipecanvas_js.h"

using namespace ipe;
using namespace ipelua;

#include "main_common.i"

#include <emscripten/bind.h>

#include <format>

using namespace emscripten;
using std::string;

// --------------------------------------------------------------------

static void setup_globals(lua_State *L, int width, int height,
			  double devicePixelRatio, std::string platform)
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
  lua_pushstring(L, platform.c_str());
  lua_setfield(L, -2, "platform");
  lua_pushliteral(L, "htmljs");
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

  lua_getglobal(L, "tonumber");
  lua_setglobal(L, "tonumber2");

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

// --------------------------------------------------------------------

int mainloop(lua_State *L)
{
  // this does nothing, the browser already runs an event loop
  return 0;
}

AppUi *startIpe(Canvas *canvas, int width, int height, double dpr,
		std::string platform)
{
  theCanvas = canvas;

  Platform::initLib(IPELIB_VERSION);
  lua_State *L = setup_lua();

  // TODO: Should we support command line options?
  
  lua_createtable(L, 0, 0);
  lua_setglobal(L, "argv");

  setup_globals(L, width, height, dpr, platform);

  lua_run_ipe(L, mainloop);

  theCanvas->setObserver(theAppUi);
  return theAppUi;
}

// TODO: simply pass a list of VAR=VALUE settings instead
static void initLib(std::string home, bool usePreloader) {
  if (usePreloader)
    putenv(strdup("IPEPRELOADER=1"));
  putenv(strdup("IPEDEBUG=1"));
  putenv(strdup(std::format("HOME={}", home).c_str()));
  ipe::Platform::initLib(ipe::IPELIB_VERSION);
}

static void ipeAction(AppUi *ui, std::string name)
{
  ui->action(name);
}

static void resumeLua(AppUi *ui, val arg) {
  ui->resumeLua(arg);
}

static void absoluteButton(AppUi *ui, val sel) {
  ui->luaAbsoluteButton(sel.as<std::string>().c_str());
}

static void selector(AppUi *ui, val sel, val value) {
  ui->luaSelector(sel.as<std::string>().c_str(),
		  value.as<std::string>().c_str());
}

static void paintPathView(AppUi *ui, val canvas) {
  ui->iPathView->paint(canvas);
}

static void layerAction(AppUi *ui, string name, string layer) {
  ui->luaLayerAction(String(name), String(layer));
}

static void showLayerBoxPopup(AppUi *ui, string layer, double x, double y) {
  ui->luaShowLayerBoxPopup(Vector(x, y), String(layer));
}

static void showPathStylePopup(AppUi *ui, double x, double y) {
  ui->luaShowPathStylePopup(Vector(x, y));
}

static void bookmarkSelected(AppUi *ui, int row) {
  ui->luaBookmarkSelected(row);
}

static val createTarball(std::string tex) {
  // need to send Latex source as a tarball
  Buffer tarHeader(512);
  char *p = tarHeader.data();
  memset(p, 0, 512);
  strcpy(p, "ipetemp.tex");
  strcpy(p + 100, "0000644"); // mode
  strcpy(p + 108, "0001750"); // uid 1000
  strcpy(p + 116, "0001750"); // gid 1000
  sprintf(p + 124, "%011o", (unsigned int) tex.size());
  p[136] = '0';  // time stamp, fudge it
  p[156] = '0';  // normal file
  // checksum
  strcpy(p + 148, "        ");
  uint32_t checksum = 0;
  for (const char *q = p; q < p + 512; ++q)
    checksum += uint8_t(*q);
  sprintf(p + 148, "%06o", checksum);
  p[155] = ' ';

  String tar;
  StringStream ss(tar);
  for (const char *q = p; q < p + 512;)
    ss.putChar(*q++);
  ss << tex;
  int i = tex.size();
  while ((i & 0x1ff) != 0) {  // fill a 512-byte block
    ss.putChar('\0');
    ++i;
  }
  for (int i = 0; i < 1024; ++i)  // add two empty blocks
    ss.putChar('\0');
  return val(typed_memory_view(tar.size(), (uint8_t *) tar.data()));
}

// --------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(ipe) {
  class_<ipe::Platform>("Platform")
    .class_function("initLib", &initLib);

  class_<AppUi>("AppUi")
    .function("action", &ipeAction, allow_raw_pointers())
    .function("resume", &resumeLua, allow_raw_pointers())
    .function("absoluteButton", &absoluteButton, allow_raw_pointers())
    .function("selector", &selector, allow_raw_pointers())
    .function("paintPathView", &paintPathView, allow_raw_pointers())
    .function("layerAction", &layerAction, allow_raw_pointers())
    .function("showLayerBoxPopup", &showLayerBoxPopup, allow_raw_pointers())
    .function("showPathStylePopup", &showPathStylePopup, allow_raw_pointers())
    .function("bookmarkSelected", &bookmarkSelected, allow_raw_pointers());
    
  function("startIpe", &startIpe, allow_raw_pointers());
  function("createTarball", &createTarball);
}

// --------------------------------------------------------------------
