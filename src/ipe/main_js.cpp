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

#include <emscripten.h>
#include <emscripten/val.h>

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

int mainloop(lua_State *L)
{
  // this does nothing, the browser already runs an event loop
  return 0;
}

// the UI
static AppUi *theAppUi = nullptr;

AppUiBase *createAppUi(lua_State *L0, int model)
{
  theAppUi = new AppUi(L0, model);
  return theAppUi;
}

// --------------------------------------------------------------------

inline Canvas *canvas()
{
  return (Canvas *) theAppUi->canvas();
}

EMSCRIPTEN_KEEPALIVE
extern "C" void canvasUpdate()
{
  canvas()->update();
}

EMSCRIPTEN_KEEPALIVE
extern "C" void canvasUpdateSize()
{
  canvas()->updateSize();
}

EMSCRIPTEN_KEEPALIVE
extern "C" void canvasMouseButtonEvent(EM_VAL ev, int button, bool press)
{
  canvas()->mouseButtonEvent(val::take_ownership(ev), button, press);
}

EMSCRIPTEN_KEEPALIVE
extern "C" void canvasMouseMoveEvent(EM_VAL ev)
{
  canvas()->mouseMoveEvent(val::take_ownership(ev));
}

EMSCRIPTEN_KEEPALIVE
extern "C" void canvasWheelEvent(EM_VAL ev)
{
  canvas()->wheelEvent(val::take_ownership(ev));
}

EMSCRIPTEN_KEEPALIVE
extern "C" bool canvasKeyPressEvent(EM_VAL ev)
{
  return canvas()->keyPressEvent(val::take_ownership(ev));
}

EMSCRIPTEN_KEEPALIVE
extern "C" void canvasSetAdditionalModifiers(int mod)
{
  canvas()->setAdditionalModifiers(mod);
}

// ------------------------------------------------------------------------

EMSCRIPTEN_KEEPALIVE
extern "C" void startIpe(int width, int height, double dpr, const char * platform) {
  lua_State *L = setup_lua();

  // In other versions, argv can be used to specify
  //  (1) the initial file to open,
  //  (2) the list of style sheets to use for new documents
  // Both functions are realized differently for ipe-web
  lua_createtable(L, 0, 0);
  lua_setglobal(L, "argv");

  setup_globals(L, width, height, dpr, platform);
  std::free((void *) platform);

  lua_run_ipe(L, mainloop);
}

EMSCRIPTEN_KEEPALIVE
extern "C" void resume(EM_VAL result) {
  theAppUi->resumeLua(val::take_ownership(result));
}

EMSCRIPTEN_KEEPALIVE
extern "C" void action(const char *name) {
  theAppUi->action(String(name));
  std::free((void *) name);
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void absoluteButton(const char * sel) {
  theAppUi->luaAbsoluteButton(sel);
  std::free((void *) sel);
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void selector(const char * sel, const char * value) {
  theAppUi->luaSelector(sel, value);
  std::free((void *) sel);
  std::free((void *) value);
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void paintPathView() {
  val doc = val::global("document");
  val canvas = doc.call<val>("getElementById", std::string("pathView"));
  theAppUi->iPathView->paint(canvas);
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void layerAction(const char *name, const char * layer) {
  theAppUi->luaLayerAction(String(name), String(layer));
  std::free((void *) name);
  std::free((void *) layer);
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void showLayerBoxPopup(const char * layer, double x, double y) {
  theAppUi->luaShowLayerBoxPopup(Vector(x, y), String(layer));
  std::free((void *) layer);
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void showPathStylePopup(double x, double y) {
  theAppUi->luaShowPathStylePopup(Vector(x, y));
}

EMSCRIPTEN_KEEPALIVE
extern "C"  void bookmarkSelected(int row) {
  theAppUi->luaBookmarkSelected(row);
}

// make sure the memory remains allocated while JS uses it
static String tarball;

EMSCRIPTEN_KEEPALIVE
extern "C" EM_VAL createTarball(const char * texfile) {
  tarball = Platform::createTarball(String(texfile));
  std::free((void *) texfile);
  val result = val(typed_memory_view(tarball.size(), (uint8_t *) tarball.data()));
  return result.release_ownership();
}

EMSCRIPTEN_KEEPALIVE
extern "C" EM_VAL ipeVersion() {
  val result = val::object();
  result.set("year", COPYRIGHT_YEAR);
  int v = Platform::libVersion();
  result.set("version", std::format("{}.{}.{}", v / 10000, (v / 100) % 100, v % 100));
  return result.release_ownership();
}

// --------------------------------------------------------------------
