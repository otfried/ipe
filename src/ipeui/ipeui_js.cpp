// --------------------------------------------------------------------
// Lua bindings for Qt dialogs
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

#include <emscripten/bind.h>
#include <emscripten/val.h>

// --------------------------------------------------------------------

static int dialog_constructor(lua_State *L)
{
  // QWidget *parent = check_winid(L, 1);
  const char *s = luaL_checkstring(L, 2);
  const char *language = "";
  if (lua_isstring(L, 3))
    language = luaL_checkstring(L, 3);

  Dialog **dlg = (Dialog **) lua_newuserdata(L, sizeof(Dialog *));
  *dlg = nullptr;
  luaL_getmetatable(L, "Ipe.dialog");
  lua_setmetatable(L, -2);
  // *dlg = new PDialog(L, parent, s, language);
  (void) s;
  (void) language;
  return 1;
}

// --------------------------------------------------------------------

static int menu_constructor(lua_State *L)
{
  // QWidget *parent = check_winid(L, 1);
  Menu **m = (Menu **) lua_newuserdata(L, sizeof(Menu *));
  *m = nullptr;
  luaL_getmetatable(L, "Ipe.menu");
  lua_setmetatable(L, -2);

  // *m = new PMenu(parent);
  return 1;
}

// --------------------------------------------------------------------

class PTimer : public Timer {
public:
  PTimer(lua_State *L0, int lua_object, const char *method);
  virtual ~PTimer();

  virtual int setInterval(lua_State *L) override;
  virtual int active(lua_State *L) override;
  virtual int start(lua_State *L) override;
  virtual int stop(lua_State *L) override;
private:
  emscripten::val iTimer;
};

PTimer::PTimer(lua_State *L0, int lua_object, const char *method)
  : Timer(L0, lua_object, method)
{
  /*
  iTimer = new QTimer();
  connect(iTimer, &QTimer::timeout, [&]() {
      if (iSingleShot)
	iTimer->stop();
      callLua();
    });
  */
}

PTimer::~PTimer()
{
  // delete iTimer;
}

int PTimer::setInterval(lua_State *L)
{
  int t = (int)luaL_checkinteger(L, 2);
  (void) t;
  // iTimer->setInterval(t);
  return 0;
}

int PTimer::active(lua_State *L)
{
  lua_pushboolean(L, false); // iTimer->isActive());
  return 1;
}

int PTimer::start(lua_State *L)
{
  // iTimer->start();
  return 0;
}

int PTimer::stop(lua_State *L)
{
  // iTimer->stop();
  return 0;
}

// ------------------------------------------------------------------------

static int timer_constructor(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  const char *method = luaL_checkstring(L, 2);

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
  (void) method;
  (void) lua_object;
  return 1;
}

// --------------------------------------------------------------------

static int ipeui_getColor(lua_State *L)
{
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_fileDialog(lua_State *L)
{
  static const char * const typenames[] = { "open", "save", nullptr };

  void * parent = check_winid(L, 1);
  (void) parent;

  int type = luaL_checkoption(L, 2, nullptr, typenames);
  std::string caption = checkstring(L, 3);
  if (!lua_istable(L, 4))
    luaL_argerror(L, 4, "table expected for filters");
  std::vector<std::string> filters;
  int nFilters = lua_rawlen(L, 4);
  for (int i = 1; i <= nFilters; i += 2) { // skip Windows versions
    lua_rawgeti(L, 4, i);
    luaL_argcheck(L, lua_isstring(L, -1), 4, "filter entry is not a string");
    filters.push_back(checkstring(L, -1));
    lua_pop(L, 1); // element i
  }

  std::string dir;
  if (!lua_isnoneornil(L, 5))
    dir = checkstring(L, 5);
  std::string path;
  if (!lua_isnoneornil(L, 6))
    path = checkstring(L, 6);
  int selected = 0;
  if (!lua_isnoneornil(L, 7))
    selected = luaL_checkinteger(L, 7);

  emscripten::val arg = emscripten::val::object();
  arg.set("type", typenames[type]);
  arg.set("caption", caption);
  arg.set("filters", filters);
  arg.set("dir", dir);
  arg.set("path", path);
  arg.set("selected", selected);
  emscripten::val::global("window")["ipeui"].call<void>("fileDialog", arg);
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_messageBox(lua_State *L)
{
  static const char * const options[] =  {
    "none", "warning", "information", "question", "critical", nullptr };
  static const char * const buttontype[] = {
    "ok", "okcancel", "yesnocancel", "discardcancel",
    "savediscardcancel", nullptr };

  void * parent = check_winid(L, 1);
  (void) parent;
  int type = luaL_checkoption(L, 2, "none", options);
  std::string text = checkstring(L, 3);
  std::string details;
  if (!lua_isnoneornil(L, 4))
    details = checkstring(L, 4);
  int buttons = 0;
  if (lua_isnumber(L, 5))
    buttons = luaL_checkinteger(L, 5);
  else if (!lua_isnoneornil(L, 5))
    buttons = luaL_checkoption(L, 5, nullptr, buttontype);

  emscripten::val arg = emscripten::val::object();
  arg.set("type", type);
  arg.set("text", text);
  arg.set("details", details);
  arg.set("buttons", buttons);
  emscripten::val::global("window")["ipeui"].call<void>("messageBox", arg);
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State *L)
{
  emscripten::val dt = emscripten::val::global("Date").new_();
  char buf[15];
  sprintf(buf, "%04d%02d%02d%02d%02d%02d",
	  dt.call<int>("getFullYear"),
	  dt.call<int>("getMonth") + 1,
	  dt.call<int>("getDate"),
	  dt.call<int>("getHours"),
	  dt.call<int>("getMinutes"),
	  dt.call<int>("getSeconds"));
  lua_pushstring(L, buf);
  return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
  { "Dialog", dialog_constructor },
  { "Menu", menu_constructor },
  { "Timer", timer_constructor },
  { "getColor", ipeui_getColor },
  { "fileDialogAsync", ipeui_fileDialog },
  { "messageBoxAsync", ipeui_messageBox },
  { "currentDateTime", ipeui_currentDateTime },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

void addMethod(lua_State *L, const char * name, const char * luacode)
{
  int ok = luaL_loadstring(L, luacode);
  if (ok != LUA_OK)
    luaL_error(L, "cannot prepare function");
  lua_call(L, 0, 1);
  lua_setfield(L, -2, name);
}

int luaopen_ipeui(lua_State *L)
{
  luaL_newlib(L, ipeui_functions);
  addMethod(L, "messageBox",
	    "return function (...) ipeui.messageBoxAsync(...)"
	    "return coroutine.yield() end");
  addMethod(L, "fileDialog",
	    "return function (...) ipeui.fileDialogAsync(...)"
	    "return coroutine.yield() end");
  lua_setglobal(L, "ipeui");
  luaopen_ipeui_common(L);
  return 0;
}

// --------------------------------------------------------------------
