// --------------------------------------------------------------------
// Lua bindings for UI, platform-neutral common part
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
#include <algorithm>

// --------------------------------------------------------------------

WINID check_winid(lua_State *L, int i)
{
  if (lua_isnil(L, i))
    return nullptr;
  WINID *w = (WINID *) luaL_checkudata(L, i, "Ipe.winid");
  return *w;
}

void push_winid(lua_State *L, WINID win)
{
  WINID *w = (WINID *) lua_newuserdata(L, sizeof(WINID));
  *w = win;
  luaL_getmetatable(L, "Ipe.winid");
  lua_setmetatable(L, -2);
}

static int winid_tostring(lua_State *L)
{
  check_winid(L, 1);
  lua_pushfstring(L, "GtkWidget@%p", lua_topointer(L, 1));
  return 1;
}

static const struct luaL_Reg winid_methods[] = {
  { "__tostring", winid_tostring },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

// Make sure not to create a cyclic reference to the dialog by capturing it inside
// the Lua method for an action, as this would stop it from being garbage collected.
Dialog::Dialog(lua_State *L0, WINID parent, const char *caption, const char *language)
  : iCaption(caption), iLanguage(language)
{
  L = L0;
  iParent = parent;
  iLuaDialog = LUA_NOREF;
  hDialog = nullptr;
  iIgnoreEscapeField = -1;
  iNoRows = 1;
  iNoCols = 1;
}

Dialog::~Dialog()
{
  //
}

void Dialog::callLua(int luaMethod)
{
  // only call back to Lua during execute()
  if (iLuaDialog == LUA_NOREF)
    return;

  lua_rawgeti(L, LUA_REGISTRYINDEX, luaMethod);
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaDialog);
  luacall(L, 1, 0);
}

// name, label, action
int Dialog::addButton(lua_State *L)
{
  SElement m;
  m.name = std::string(luaL_checklstring(L, 2, nullptr));
  m.type = EButton;
  m.lua_method = LUA_NOREF;
  m.flags = 0;
  m.row = -1;
  m.col = -1;
  m.rowspan = 1;
  m.colspan = 1;
  m.text = std::string(luaL_checklstring(L, 3, nullptr));
  if (lua_isstring(L, 4)) {
    const char *s = lua_tolstring(L, 4, nullptr);
    if (!strcmp(s, "accept"))
      m.flags |= EAccept;
    else if (!strcmp(s, "reject"))
      m.flags |= EReject;
    else
      luaL_argerror(L, 4, "unknown action");
  } else {
    luaL_argcheck(L, lua_isfunction(L, 4), 4, "unknown action");
    lua_pushvalue(L, 4);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  m.minHeight = 16;
  m.minWidth = 4 * m.text.length() + 8;
  if (m.minWidth < 64)
    m.minWidth = 64;
  iElements.push_back(m);
  return 0;
}

int Dialog::add(lua_State *L)
{
  static const char * const typenames[] =
    { "button", "text", "list", "label", "combo", "checkbox", "input", nullptr };

  SElement m;
  m.name = checkstring(L, 2);
  m.type = TType(luaL_checkoption(L, 3, nullptr, typenames));
  luaL_checktype(L, 4, LUA_TTABLE);
  m.lua_method = LUA_NOREF;
  m.flags = 0;
  m.row = luaL_checkinteger(L, 5) - 1;
  if (m.row < 0)
    m.row = iNoRows + 1 + m.row;
  m.col = luaL_checkinteger(L, 6) - 1;
  m.rowspan = 1;
  m.colspan = 1;
  if (!lua_isnoneornil(L, 7))
    m.rowspan = luaL_checkinteger(L, 7);
  if (!lua_isnoneornil(L, 8))
    m.colspan = luaL_checkinteger(L, 8);
  if (m.row + m.rowspan > iNoRows)
    iNoRows = m.row + m.rowspan;
  if (m.col + m.colspan > iNoCols)
    iNoCols = m.col + m.colspan;

  switch (m.type) {
  case EButton:
    addButtonItem(L, m);
    break;
  case ETextEdit:
    addTextEdit(L, m);
    break;
  case EList:
    addList(L, m);
    break;
  case ELabel:
    addLabel(L, m);
    break;
  case ECombo:
    addCombo(L, m);
    break;
  case ECheckBox:
    addCheckbox(L, m);
    break;
  case EInput:
    addInput(L, m);
    break;
  default:
    break;
  }
  iElements.push_back(m);
  return 0;
}

void Dialog::addLabel(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no label");
  m.text = std::string(lua_tolstring(L, -1, nullptr));
  lua_pop(L, 1); // label
  m.minHeight = 16;
  const char *p = m.text.c_str();
  int maxlen = 0;
  int curlen = 0;
  while (*p) {
    if (*p++ == '\n') {
      m.minHeight += 8;
      if (curlen > maxlen)
	maxlen = curlen;
      curlen = 0;
    }
    ++curlen;
  }
  if (curlen > maxlen)
    maxlen = curlen;
  m.minWidth = 4 * maxlen;
}

void Dialog::addButtonItem(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no button label");
  m.text = tostring(L, -1);
  lua_getfield(L, 4, "action");
  if (lua_isstring(L, -1)) {
    auto s = tostring(L, -1);
    if (s == "accept")
      m.flags |= EAccept;
    else if (s == "reject")
      m.flags |= EReject;
    else
      luaL_argerror(L, 4, "unknown action");
  } else if (lua_isfunction(L, -1)) {
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  } else if (!lua_isnil(L, -1))
    luaL_argerror(L, 4, "unknown action type");
  lua_pop(L, 2); // action, label
  m.minHeight = 16;
  m.minWidth = 4 * m.text.length() + 8;
  if (m.minWidth < 64)
    m.minWidth = 64;
}

void Dialog::addCheckbox(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "label");
  luaL_argcheck(L, lua_isstring(L, -1), 4, "no label");
  m.text = tostring(L, -1);
  lua_getfield(L, 4, "action");
  if (!lua_isnil(L, -1)) {
    luaL_argcheck(L, lua_isfunction(L, -1), 4, "unknown action type");
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  lua_pop(L, 2); // action, label
  m.value = 0;
  m.minHeight = 16;
  m.minWidth = 4 * m.text.length() + 32;
}

void Dialog::addInput(lua_State *L, SElement &m)
{
  m.minHeight = 12;
  m.minWidth = 100;
  lua_getfield(L, 4, "select_all");
  if (lua_toboolean(L, -1))
    m.flags |= ESelectAll;
  lua_getfield(L, 4, "focus");
  if (lua_toboolean(L, -1))
    m.flags |= EFocused;
  lua_getfield(L, 4, "color_picker");
  if (lua_toboolean(L, -1))
    m.flags |= EColorPicker;
  lua_pop(L, 3);
}

void Dialog::addTextEdit(lua_State *L, SElement &m)
{
  lua_getfield(L, 4, "read_only");
  if (lua_toboolean(L, -1))
    m.flags |= EReadOnly;
  lua_getfield(L, 4, "select_all");
  if (lua_toboolean(L, -1))
    m.flags |= ESelectAll;
  lua_getfield(L, 4, "focus");
  if (lua_toboolean(L, -1))
    m.flags |= EFocused;
  lua_getfield(L, 4, "syntax");
  if (!lua_isnil(L, -1)) {
    auto s = tostring(L, -1);
    if (s == "logfile") {
      m.flags |= ELogFile;
    } else if (s == "xml") {
      m.flags |= EXml;
    } else if (s == "latex") {
      m.flags |= ELatex;
    } else
      luaL_argerror(L, 4, "unknown syntax");
  }
  lua_getfield(L, 4, "spell_check");
  if (lua_toboolean(L, -1))
    m.flags |= ESpellCheck;
  lua_pop(L, 5); // spell_check, syntax, focus, select_all, read_only
  m.minHeight = 48;
  m.minWidth = 100;
}

void Dialog::setListItems(lua_State *L, int index, SElement &m)
{
  int no = lua_rawlen(L, index);
  m.minWidth = 48;
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, index, i);
    luaL_argcheck(L, lua_isstring(L, -1), index, "items must be strings");
    auto item = tostring(L, -1);
    int w = 4 * item.size() + 16;
    if (w > m.minWidth)
      m.minWidth = w;
    m.items.emplace_back(item);
    lua_pop(L, 1); // item
  }
  lua_getfield(L, index, "action");
  if (!lua_isnil(L, -1)) {
    luaL_argcheck(L, lua_isfunction(L, -1), index, "unknown action type");
    lua_pushvalue(L, -1);
    m.lua_method = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  lua_pop(L, 1); // action
}

void Dialog::addList(lua_State *L, SElement &m)
{
  setListItems(L, 4, m);
  m.value = 0;
  m.minHeight = 48;
}

void Dialog::addCombo(lua_State *L, SElement &m)
{
  setListItems(L, 4, m);
  m.value = 0;
  m.minHeight = 16;
}

int Dialog::findElement(lua_State *L, int index)
{
  auto name = checkstring(L, index);
  for (int i = 0; i < int(iElements.size()); ++i) {
    if (name == iElements[i].name)
      return i;
  }
  return luaL_argerror(L, index, "no such element in dialog");
}

int Dialog::set(lua_State *L)
{
  auto s = checkstring(L, 2);
  if (s == "ignore-escape") {
    iIgnoreEscapeField = findElement(L, 3);
    iIgnoreEscapeText = checkstring(L, 4);
    return 0;
  }

  int idx = findElement(L, 2);

  // set internal representation
  setUnmapped(L, idx);

  // if dialog is on screen, also set there
  if (iLuaDialog != LUA_NOREF)
    setMapped(L, idx);
  return 0;
}

void Dialog::setUnmapped(lua_State *L, int idx)
{
  SElement &m = iElements[idx];
  switch (m.type) {
  case ELabel:
  case ETextEdit:
  case EInput:
    m.text = checkstring(L, 3);
    break;
  case EList:
  case ECombo:
    if (lua_isnumber(L, 3)) {
      int n = luaL_checkinteger(L, 3);
      luaL_argcheck(L, 1 <= n && n <= int(m.items.size()), 3,
		    "list index out of bounds");
      m.value = n - 1;
    } else if (lua_isstring(L, 3)) {
      auto s = tostring(L, 3);
      auto it = std::find_if(m.items.cbegin(), m.items.cend(),
			     [&s](const std::string &el) { return s == el; });
      luaL_argcheck(L, it != m.items.end(), 3, "item not in list");
      m.value = (it - m.items.begin());
    } else {
      luaL_checktype(L, 3, LUA_TTABLE);
      m.items.clear();
      setListItems(L, 3, m);
      m.value = 0;
    }
    break;
  case ECheckBox:
    m.value = lua_toboolean(L, 3);
    break;
  default:
    luaL_argerror(L, 2, "no suitable element");
  }
}

int Dialog::get(lua_State *L)
{
  if (iLuaDialog != LUA_NOREF)
    retrieveValues();

  int idx = findElement(L, 2);
  SElement &m = iElements[idx];

  switch (m.type) {
  case ETextEdit:
  case EInput:
    lua_pushstring(L, m.text.c_str());
    return 1;
  case EList:
  case ECombo:
    lua_pushinteger(L, m.value + 1);
    return 1;
  case ECheckBox:
    lua_pushboolean(L, m.value);
    return 1;
  default:
    return luaL_argerror(L, 2, "no suitable element");
  }
}

Dialog::Result Dialog::execute(lua_State *L, int w, int h)
{
  // remember Lua object for dialog for callbacks
  lua_pushvalue(L, 1);
  iLuaDialog = luaL_ref(L, LUA_REGISTRYINDEX);
  return buildAndRun(w, h); // execute dialog
}

// garbage collection will call this on the main thread,
// and our own "L" might already have been collected
void Dialog::release(lua_State *LL)
{
  // discard references to dialog object and Lua methods
  for (int i = 0; i < int(iElements.size()); ++i) {
    luaL_unref(LL, LUA_REGISTRYINDEX, iElements[i].lua_method);
    iElements[i].lua_method = LUA_NOREF;
  }
  luaL_unref(LL, LUA_REGISTRYINDEX, iLuaDialog);
  iLuaDialog = LUA_NOREF;
}

int Dialog::setEnabled(lua_State *L)
{
  int idx = findElement(L, 2);
  bool value = lua_toboolean(L, 3);
  if (iLuaDialog != LUA_NOREF) // mapped
    enableItem(idx, value);
  else if (value)
    iElements[idx].flags &= ~EDisabled;
  else
    iElements[idx].flags |= EDisabled;
  return 0;
}

int Dialog::setStretch(lua_State *L)
{
  static const char * const typenames[] = { "row", "column", nullptr };

  while (int(iColStretch.size()) < iNoCols)
    iColStretch.push_back(0);
  while (int(iRowStretch.size()) < iNoRows)
    iRowStretch.push_back(0);

  int type = luaL_checkoption(L, 2, nullptr, typenames);
  int rowcol = (int)luaL_checkinteger(L, 3) - 1;
  int stretch = (int)luaL_checkinteger(L, 4);

  if (type == 0) {
    luaL_argcheck(L, 0 <= rowcol && rowcol < iNoRows, 3,
		  "Row index out of range");
    iRowStretch[rowcol] = stretch;
  } else {
    luaL_argcheck(L, 0 <= rowcol && rowcol < iNoCols, 3,
		  "Column index out of range");
    iColStretch[rowcol] = stretch;
  }
  return 0;
}

int Dialog::takeDown(lua_State *L)
{
  luaL_error(L, "Dialog::takeDown not implemented for this toolkit");
  return 0; 
}

// --------------------------------------------------------------------

static int dialog_tostring(lua_State *L)
{
  check_dialog(L, 1);
  lua_pushfstring(L, "Dialog@%p", lua_topointer(L, 1));
  return 1;
}

static int dialog_destructor(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  (*dlg)->release(L);
  delete (*dlg);
  *dlg = nullptr;
  return 0;
}

static int dialog_executeAsync(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  int w = 0;
  int h = 0;
  if (!lua_isnoneornil(L, 2)) {
    luaL_argcheck(L, lua_istable(L, 2), 2, "argument is not a table");
    lua_rawgeti(L, 2, 1);
    luaL_argcheck(L, lua_isnumber(L, -1), 2, "width is not a number");
    lua_rawgeti(L, 2, 2);
    luaL_argcheck(L, lua_isnumber(L, -1), 2, "height is not a number");
    w = lua_tointegerx(L, -2, nullptr);
    h = lua_tointegerx(L, -1, nullptr);
    lua_pop(L, 2); // w & h
  }
  Dialog::Result result = (*dlg)->execute(L, w, h);
  if (result == Dialog::Result::MODAL) {
    lua_pushboolean(L, false);
    lua_pushboolean(L, false);
  } else {
    (*dlg)->release(L);
    lua_pushboolean(L, true);
    lua_pushboolean(L, result == Dialog::Result::ACCEPTED);
  }
  return 2;
}

static int dialog_setStretch(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->setStretch(L);
}

static int dialog_add(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->add(L);
}

static int dialog_addButton(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->addButton(L);
}

static int dialog_set(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->set(L);
}

static int dialog_get(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->get(L);
}

static int dialog_setEnabled(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->setEnabled(L);
}

static int dialog_accept(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  (*dlg)->acceptDialog(L);
  return 0;
}

static int dialog_takeDown(lua_State *L)
{
  Dialog **dlg = check_dialog(L, 1);
  return (*dlg)->takeDown(L);
}

// --------------------------------------------------------------------

static const struct luaL_Reg dialog_methods[] = {
  { "__tostring", dialog_tostring },
  { "__gc", dialog_destructor },
  { "executeAsync", dialog_executeAsync }, // internal use
  { "setStretch", dialog_setStretch },
  { "add", dialog_add },
  { "addButton", dialog_addButton },
  { "set", dialog_set },
  { "get", dialog_get },
  { "setEnabled", dialog_setEnabled },
  { "accept", dialog_accept },
  { "takeDown", dialog_takeDown },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

Menu::~Menu()
{
  // empty
}

// --------------------------------------------------------------------

static int menu_tostring(lua_State *L)
{
  check_menu(L, 1);
  lua_pushfstring(L, "Menu@%p", lua_topointer(L, 1));
  return 1;
}

static int menu_destructor(lua_State *L)
{
  Menu **m = check_menu(L, 1);
  delete *m;
  *m = nullptr;
  return 0;
}

static int menu_execute(lua_State *L)
{
  Menu **m = check_menu(L, 1);
  return (*m)->execute(L);
}

static int menu_add(lua_State *L)
{
  Menu **m = check_menu(L, 1);
  return (*m)->add(L);
}

// --------------------------------------------------------------------

static const struct luaL_Reg menu_methods[] = {
  { "__tostring", menu_tostring },
  { "__gc", menu_destructor },
#ifdef IPEUI_JS
  { "executeAsync", menu_execute },
#else
  { "execute", menu_execute },
#endif
  { "add", menu_add },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

Timer::Timer(lua_State *L0, int lua_object, const char *method)
  : iMethod(method)
{
  L = L0;
  lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
  if (lua_tothread(L, -1) != L)
    luaL_error(L, "timer can only be created from main thread");
  lua_pop(L, 1);
  iLuaObject = lua_object;
  iSingleShot = false;
}

Timer::~Timer()
{
  luaL_unref(L, LUA_REGISTRYINDEX, iLuaObject);
}

void Timer::callLua()
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iLuaObject);
  lua_rawgeti(L, -1, 1); // get Lua object
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2); // pop weak table, nil
    return;
  }
  lua_getfield(L, -1, iMethod.c_str());
  if (lua_isnil(L, -1)) {
    lua_pop(L, 3); // pop weak table, table, nil
    return;
  }
  lua_remove(L, -3); // remove weak table
  lua_insert(L, -2); // stack is now: method, table
  luacall(L, 1, 0); // call method
}

int Timer::setSingleShot(lua_State *L)
{
  iSingleShot = lua_toboolean(L, 2);
  return 0;
}

// --------------------------------------------------------------------

static int timer_tostring(lua_State *L)
{
  check_timer(L, 1);
  lua_pushfstring(L, "Timer@%p", lua_topointer(L, 1));
  return 1;
}

static int timer_destructor(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  delete *t;
  *t = nullptr;
  return 0;
}

// --------------------------------------------------------------------

static int timer_start(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->start(L);
}

static int timer_stop(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->stop(L);
}

static int timer_active(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->active(L);
}

static int timer_setinterval(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->setInterval(L);
}

static int timer_setsingleshot(lua_State *L)
{
  Timer **t = check_timer(L, 1);
  return (*t)->setSingleShot(L);
}

// --------------------------------------------------------------------

static const struct luaL_Reg timer_methods[] = {
  { "__tostring", timer_tostring },
  { "__gc", timer_destructor },
  { "start", timer_start },
  { "stop", timer_stop },
  { "active", timer_active },
  { "setInterval", timer_setinterval },
  { "setSingleShot", timer_setsingleshot },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

static void make_metatable(lua_State *L, const char *name,
			   const struct luaL_Reg *methods)
{
  luaL_newmetatable(L, name);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);   /* metatable.__index = metatable */
  luaL_setfuncs(L, methods, 0);
  if (!strcmp(name, "Ipe.dialog")) {
    int ok = luaL_loadstring(L, "return function (d, s, l)"
			     "done, accepted = d:executeAsync(s, l)"
			     "if not done then accepted = d:takeDown(coroutine.yield()) end "
			     "return accepted end");
    if (ok != LUA_OK)
      luaL_error(L, "cannot prepare d:execute function");
    lua_call(L, 0, 1);
    lua_setfield(L, -2, "execute");
  }
#ifdef IPEUI_JS
  if (!strcmp(name, "Ipe.menu")) {
    int ok = luaL_loadstring(L, "return function (m, x, y)"
			     "m:executeAsync(x, y)"
			     "local r = coroutine.yield()"
			     "if r then return ipeui.val(r, 'action'),"
			     "ipeui.val(r, 'current') end end");
    if (ok != LUA_OK)
      luaL_error(L, "cannot prepare m:execute function");
    lua_call(L, 0, 1);
    lua_setfield(L, -2, "execute");
  }
#endif
  lua_pop(L, 1);
}

int luaopen_ipeui_common(lua_State *L)
{
  make_metatable(L, "Ipe.winid", winid_methods);
  make_metatable(L, "Ipe.dialog", dialog_methods);
  make_metatable(L, "Ipe.menu", menu_methods);
  make_metatable(L, "Ipe.timer", timer_methods);
  return 0;
}

// --------------------------------------------------------------------
