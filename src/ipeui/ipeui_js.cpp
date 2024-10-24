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

#include <emscripten.h>
#include <emscripten/val.h>

using emscripten::val;

static val jsUi() {
  return val::global("window")["ipeui"];
}

// --------------------------------------------------------------------

class PDialog : public Dialog {
public:
  PDialog(lua_State *L0, WINID parent, const char *caption, const char * language);
  bool ignoresEscapeKey();
  void callLuaMethod(int method);
  virtual int takeDown(lua_State *L);

protected:
  virtual void setMapped(lua_State *L, int idx);
  virtual void enableItem(int idx, bool value);
  virtual void acceptDialog(lua_State *L);

  virtual Dialog::Result buildAndRun(int w, int h);
  virtual void retrieveValues();

  val element(const SElement &m);

private:
  val iOptions; // dialog description for JS
};

// --------------------------------------------------------------------

static const char *typenames[] = {
  "button", "textedit", "list", "label", "combo", "checkbox", "input",
};

val PDialog::element(const SElement &m)
{
  val w = val::object();
  w.set("name", m.name);
  w.set("type", std::string(typenames[m.type]));
  w.set("text", m.text);
  w.set("flags", m.flags);
  w.set("value", m.value);
  if (!m.items.empty()) {
    val items = val::array();
    for (int k = 0; k < int(m.items.size()); ++k)
      items.call<void>("push", m.items[k]);
    w.set("items", items);
  }
  w.set("method", m.lua_method != LUA_NOREF ? val(m.lua_method) : val::null());
  w.set("row", m.row);
  w.set("col", m.col);
  w.set("rowspan", m.rowspan);
  w.set("colspan", m.colspan);
  return w;
}

PDialog::PDialog(lua_State *L0, WINID parent, const char *caption, const char *language)
  : Dialog(L0, parent, caption, language)
{
}

void PDialog::callLuaMethod(int method)
{
  callLua(method);
}

void PDialog::setMapped(lua_State *L, int idx)
{
  jsUi().call<void>("dialogSet", element(iElements[idx]));
}

void PDialog::enableItem(int idx, bool value)
{
  jsUi().call<void>("dialogSetEnabled", iElements[idx].name, value);
}

void PDialog::acceptDialog(lua_State *L)
{
  luaL_error(L, "Dialog:acceptDialog is not implemented for JS dialogs");
}

bool PDialog::ignoresEscapeKey()
{
  if (iIgnoreEscapeField >= 0) {
    retrieveValues();
    if (iElements[iIgnoreEscapeField].text != iIgnoreEscapeText)
      return true;  // text has been modified, do not allow ESC
  }
  return false;
}

void PDialog::retrieveValues()
{
  val values = jsUi().call<val>("dialogRetrieveValues", iOptions);
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    if (m.row < 0)  continue;
    val value = values[m.name];
    if (!value.isUndefined()) {
      if (m.type == ETextEdit || m.type == EInput)
	m.text = value.as<std::string>();
      else if (m.type == ECheckBox)
	m.value = value.as<bool>() ? 1 : 0;
      else if (m.type == EList || m.type == ECombo)
	m.value = value.as<int>();
    }
  }
}

Dialog::Result PDialog::buildAndRun(int w, int h)
{
  val buttons = val::array();
  val elements = val::array();  
  for (int i = 0; i < int(iElements.size()); ++i) {
    SElement &m = iElements[i];
    if (m.row < 0) {
      // a button in the button bar
      val b = val::object();  
      b.set("name", m.text);
      b.set("flags", m.flags);
      buttons.call<void>("push", b);
    } else {
      elements.call<void>("push", element(m));
    }
  }
  while (int(iColStretch.size()) < iNoCols)
    iColStretch.push_back(0);
  while (int(iRowStretch.size()) < iNoRows)
    iRowStretch.push_back(0);
  val rowstretch = val::array();
  for (int s : iRowStretch)
    rowstretch.call<void>("push", s);
  val colstretch = val::array();
  for (int s : iColStretch)
    colstretch.call<void>("push", s);
  iOptions = val::object();
  iOptions.set("type", std::string("dialog"));
  iOptions.set("caption", iCaption);
  iOptions.set("buttons", buttons);
  iOptions.set("elements", elements);
  iOptions.set("rowstretch", rowstretch);
  iOptions.set("colstretch", colstretch);
  iOptions.set("dialogId", (uintptr_t) this);
  jsUi().call<void>("showDialog", iOptions);
  return Result::MODAL;
}

int PDialog::takeDown(lua_State *L)
{
  int result = luaL_checkinteger(L, 2);
  release(L); // release references to Lua objects
  retrieveValues();
  lua_pushboolean(L, result == 1);
  return 1;
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State *L)
{
  void * parent = check_winid(L, 1);
  const char *caption = luaL_checkstring(L, 2);
  const char *language = "";
  if (lua_isstring(L, 3))
    language = luaL_checkstring(L, 3);

  Dialog **dlg = (Dialog **) lua_newuserdata(L, sizeof(Dialog *));
  *dlg = nullptr;
  luaL_getmetatable(L, "Ipe.dialog");
  lua_setmetatable(L, -2);
  *dlg = new PDialog(L, parent, caption, language);
  return 1;
}

// --------------------------------------------------------------------

class PMenu : public Menu {
public:
  PMenu();
  virtual int add(lua_State *L);
  virtual int execute(lua_State *L);
private:
  val iItems;
};

PMenu::PMenu()
{
  iItems = val::array();
}

int PMenu::execute(lua_State *L)
{
  int vx = (int) luaL_checknumber(L, 2);
  int vy = (int) luaL_checknumber(L, 3);
  jsUi().call<void>("showPopupMenu", vx, vy, iItems);
  return 0;
}

int PMenu::add(lua_State *L)
{
  val item = val::object();
  item.set("name", checkstring(L, 2));
  item.set("label", checkstring(L, 3));
  if (lua_gettop(L) > 3) {
    luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
    bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
    bool hastable = !hasmap && !lua_isnoneornil(L, 5);
    bool hascolor = !lua_isnoneornil(L, 6) && lua_isfunction(L, 6);
    bool hascheck = !hascolor && !lua_isnoneornil(L, 6);
    if (hastable)
      luaL_argcheck(L, lua_istable(L, 5), 5,
		    "argument is not a function or table");
    if (hascheck) {
      luaL_argcheck(L, lua_isstring(L, 6), 6,
		    "argument is not a function or string");
      item.set("current", checkstring(L, 6));
    }
    int no = lua_rawlen(L, 4);
    val submenu = val::array();
    for (int i = 1; i <= no; ++i) {
      val subitem = val::object();
      lua_rawgeti(L, 4, i);
      luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
      std::string item = tostring(L, -1);
      subitem.set("name", item);
      std::string label = item;
      if (hastable) {
	lua_rawgeti(L, 5, i);
	luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
	label = tostring(L, -1);
	lua_pop(L, 1);
      }
      if (hasmap) {
	lua_pushvalue(L, 5);   // function
	lua_pushnumber(L, i);  // index
	lua_pushvalue(L, -3);  // name
	lua_call(L, 2, 1);     // function returns label
	luaL_argcheck(L, lua_isstring(L, -1), 5,
		      "function does not return string");
	label = tostring(L, -1);
	lua_pop(L, 1);         // pop result
      }
      subitem.set("label", label);
      if (hascolor) {
	val color = val::object();
	lua_pushvalue(L, 6);   // function
	lua_pushnumber(L, i);  // index
	lua_pushvalue(L, -3);  // name
	lua_call(L, 2, 3);     // function returns red, green, blue
	color.set("red", luaL_checknumber(L, -3));
	color.set("green", luaL_checknumber(L, -2));
	color.set("blue", luaL_checknumber(L, -1));
	lua_pop(L, 3);         // pop result
	subitem.set("color", color);
      }
      lua_pop(L, 1); // item
      submenu.call<void>("push", subitem);
    }
    item.set("submenu", submenu);
  }
  iItems.call<void>("push", item);
  return 0;
}

static int menu_constructor(lua_State *L)
{
  // void *parent = check_winid(L, 1);
  Menu **m = (Menu **) lua_newuserdata(L, sizeof(Menu *));
  *m = nullptr;
  luaL_getmetatable(L, "Ipe.menu");
  lua_setmetatable(L, -2);
  *m = new PMenu();
  return 1;
}

// --------------------------------------------------------------------

class PTimer : public Timer {
public:
  PTimer(lua_State *L0, int lua_object, const char *method);

  void trigger();

  virtual int setInterval(lua_State *L) override;
  virtual int active(lua_State *L) override;
  virtual int start(lua_State *L) override;
  virtual int stop(lua_State *L) override;
private:
  val iTimer;
};

PTimer::PTimer(lua_State *L0, int lua_object, const char *method)
  : Timer(L0, lua_object, method)
{
  iTimer = val::module_property("createIpeTimer")();
  iTimer.set("timerId", (uintptr_t) this);
}

int PTimer::setInterval(lua_State *L)
{
  int t = luaL_checkinteger(L, 2);
  iTimer.set("interval", t);
  return 0;
}

int PTimer::active(lua_State *L)
{
  lua_pushboolean(L, iTimer["active"].as<bool>());
  return 1;
}

int PTimer::start(lua_State *L)
{
  iTimer.set("singleShot", iSingleShot);
  iTimer.call<void>("start");
  return 0;
}

int PTimer::stop(lua_State *L)
{
  iTimer.set("stopped", true);
  return 0;
}

void PTimer::trigger()
{
  callLua();
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
  return 1;
}

// --------------------------------------------------------------------

static int ipeui_fileDialog(lua_State *L)
{
  static const char * const typenames[] = { "open", "save", nullptr };

  // void * parent = check_winid(L, 1);
  int type = luaL_checkoption(L, 2, nullptr, typenames);
  std::string caption = checkstring(L, 3);
  if (!lua_istable(L, 4))
    luaL_argerror(L, 4, "table expected for filters");
  int nFilters = lua_rawlen(L, 4);
  val filters = val::array();
  for (int i = 1; i <= nFilters; i += 2) { // skip Windows versions
    lua_rawgeti(L, 4, i);
    luaL_argcheck(L, lua_isstring(L, -1), 4, "filter entry is not a string");
    filters.call<void>("push", val(checkstring(L, -1)));
    lua_pop(L, 1); // element i
  }

  val dir = val::null();
  if (!lua_isnoneornil(L, 5))
    dir = val(checkstring(L, 5));
  val path = val::null();
  if (!lua_isnoneornil(L, 6))
    path = val(checkstring(L, 6));
  int selected = 0;
  if (!lua_isnoneornil(L, 7))
    selected = luaL_checkinteger(L, 7);

  val arg = val::object();
  arg.set("type", std::string(typenames[type]));
  arg.set("caption", caption);
  arg.set("filters", filters);
  arg.set("dir", dir);
  arg.set("path", path);
  arg.set("selected", selected);
  jsUi().call<void>("fileDialog", arg);
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

  // void * parent = check_winid(L, 1);
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

  val arg = val::object();
  arg.set("type", std::string(options[type]));
  arg.set("text", text);
  arg.set("details", details);
  arg.set("buttons", buttons);
  jsUi().call<void>("messageBox", arg);
  return 0;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State *L)
{
  val dt = val::global("Date").new_();
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
  { "fileDialogAsync", ipeui_fileDialog },
  { "messageBoxAsync", ipeui_messageBox },
  { "currentDateTime", ipeui_currentDateTime },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

EM_JS(void, addJSClasses, (), {
class IpeTimer {
  constructor () {
    this.active = false;
    this.singleShot = false;
  }

  start() {
    this.active = true;
    this.stopped = false;
    const trigger = () => {
      if (!this.stopped) {
        Module['_triggerIpeTimer'](this.timerId);
        if (!this.singleShot)
          setTimeout(trigger, this.interval);
      }
    };
    setTimeout(trigger, this.interval);
  }
}

Module['createIpeTimer'] = () => new IpeTimer();
  });

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
	    "return coroutine.yield(), 1 end");
  lua_setglobal(L, "ipeui");
  luaopen_ipeui_common(L);
  addJSClasses();
  return 0;
}

// --------------------------------------------------------------------

EMSCRIPTEN_KEEPALIVE
extern "C" void triggerIpeTimer(uintptr_t timerId)
{
  PTimer * timer = (PTimer *) timerId;
  timer->trigger();
}

EMSCRIPTEN_KEEPALIVE
extern "C" bool dialogIgnoresEscapeKey(uintptr_t dialogId)
{
  PDialog * d = (PDialog *) dialogId;
  return d->ignoresEscapeKey();
}

EMSCRIPTEN_KEEPALIVE
extern "C" void dialogCallLua(uintptr_t dialogId, int method)
{
  PDialog * d = (PDialog *) dialogId;
  d->callLuaMethod(method);
}

// --------------------------------------------------------------------
