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

static int ipeui_wait(lua_State *L)
{
  // PDialog *parent = nullptr;
  Dialog **dlg = (Dialog **) luaL_testudata(L, 1, "Ipe.dialog");
  // if (dlg != nullptr)
  // parent = (PDialog *) *dlg;
  (void) dlg;
  return 0;
}

// --------------------------------------------------------------------

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
  // *t = new PTimer(L, lua_object, method);
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
  // static const char * const typenames[] = { "open", "save", nullptr };

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

  // QWidget *parent = check_winid(L, 1);
  // int type = luaL_checkoption(L, 2, "none", options);
  // QString text = checkqstring(L, 3);
  // QString details;
  // if (!lua_isnoneornil(L, 4))
  // details = checkqstring(L, 4);
  int buttons = 0;
  if (lua_isnumber(L, 5))
    buttons = luaL_checkinteger(L, 5);
  else if (!lua_isnoneornil(L, 5))
    buttons = luaL_checkoption(L, 5, nullptr, buttontype);

  lua_pushnumber(L, -1);
  (void) options;
  (void) buttons;
  return 1;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State *L)
{
  /*
  QDateTime dt = QDateTime::currentDateTime();
  QString mod = QString::asprintf("%04d%02d%02d%02d%02d%02d",
				  dt.date().year(), dt.date().month(), dt.date().day(),
				  dt.time().hour(), dt.time().minute(), dt.time().second());
  */
  // push_string(L, mod);
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
  { "Dialog", dialog_constructor },
  { "Menu", menu_constructor },
  { "Timer", timer_constructor },
  { "getColor", ipeui_getColor },
  { "fileDialog", ipeui_fileDialog },
  { "messageBox", ipeui_messageBox },
  { "waitDialog", ipeui_wait },
  { "currentDateTime", ipeui_currentDateTime },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

int luaopen_ipeui(lua_State *L)
{
  luaL_newlib(L, ipeui_functions);
  lua_setglobal(L, "ipeui");
  luaopen_ipeui_common(L);
  return 0;
}

// --------------------------------------------------------------------
