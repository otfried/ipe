// --------------------------------------------------------------------
// ipeluastyle.cpp
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

#include "ipelua.h"

#include "ipestyle.h"
#include "ipeiml.h"

#include <cerrno>
#include <cstring>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static const char * const set_names[] =
  { "preamble", "linecap", "linejoin", "fillrule", "symbol", "layout",
    "gradient", "titlestyle", nullptr };

int ipelua::test_option(lua_State *L, int i, const char * const *names)
{
  const char *s = lua_tolstring(L, i, nullptr);
  const char * const *p = names;
  int what = 0;
  while (*p) {
    if (!strcmp(s, *p))
      return what;
    ++what;
    ++p;
  }
  return -1;
}

// --------------------------------------------------------------------

void ipelua::push_sheet(lua_State *L, StyleSheet *s0, bool owned)
{
  SSheet *s = (SSheet *) lua_newuserdata(L, sizeof(SSheet));
  s->owned = owned;
  s->sheet = s0;
  luaL_getmetatable(L, "Ipe.sheet");
  lua_setmetatable(L, -2);
}

int ipelua::sheet_constructor(lua_State *L)
{
  if (lua_type(L, 1) == LUA_TSTRING) {
    String fname = check_filename(L, 1);
    FILE *fd = Platform::fopen(fname.z(), "rb");
    if (!fd) {
      lua_pushnil(L);
      lua_pushfstring(L, "fopen error: %s", strerror(errno));
      return 2;
    }
    FileSource source(fd);
    ImlParser parser(source);
    StyleSheet *sheet = parser.parseStyleSheet();
    fclose(fd);
    if (!sheet) {
      lua_pushnil(L);
      lua_pushfstring(L, "Parsing error at %d", parser.parsePosition());
      return 2;
    }
    push_sheet(L, sheet);
  } else if (lua_type(L, 2)== LUA_TSTRING) {
    size_t len;;
    const char *s = lua_tolstring(L, 2, &len);
    Buffer buf(s, len);
    BufferSource source(buf);
    ImlParser parser(source);
    StyleSheet *sheet = parser.parseStyleSheet();
    if (!sheet) {
      lua_pushnil(L);
      lua_pushfstring(L, "Parsing error at %d", parser.parsePosition());
      return 2;
    }
    push_sheet(L, sheet);
  } else
    push_sheet(L, new StyleSheet());
  return 1;
}

static int sheet_clone(lua_State *L)
{
  SSheet *p = check_sheet(L, 1);
  push_sheet(L, new StyleSheet(*p->sheet));
  return 1;
}

static int sheet_destructor(lua_State *L)
{
  SSheet *s = check_sheet(L, 1);
  if (s->owned)
    delete s->sheet;
  s->sheet = nullptr;
  return 0;
}

static int sheet_tostring(lua_State *L)
{
  check_sheet(L, 1);
  lua_pushfstring(L, "Sheet@%p", lua_topointer(L, 1));
  return 1;
}

// --------------------------------------------------------------------

// i must be positive
static Attribute check_absolute_attribute(Kind kind, lua_State *L, int i)
{
  switch (kind) {
  case EPen:
  case ESymbolSize:
  case EArrowSize:
  case ETextSize:
  case ETextStretch:
  case EOpacity:
  case EGridSize:
  case EAngleSize: {
    double v = luaL_checknumber(L, i);
    return Attribute(Fixed::fromInternal(int(v * 1000 + 0.5))); }
  case EColor: {
    Color color = check_color(L, i);
    return Attribute(color); }
  case EDashStyle: {
    const char *s = luaL_checklstring(L, i, nullptr);
    Attribute ds = Attribute::makeDashStyle(s);
    luaL_argcheck(L, !ds.isSymbolic(), i, "dashstyle is not absolute");
    return ds; }
  case ETextStyle:
  case ELabelStyle:
  case EEffect:
  case ETiling:
  case EGradient:
  case ESymbol:
    luaL_argerror(L, i, "cannot set absolute value of this kind");
    break;
  }
  return Attribute::NORMAL(); // placate compiler
}

static int sheet_add(lua_State *L)
{
  StyleSheet *s = check_sheet(L, 1)->sheet;
  const char *what = luaL_checklstring(L, 2, nullptr);
  if (!strcmp(what, "symbol")) {
    const char *name = luaL_checklstring(L, 3, nullptr);
    SObject *obj = check_object(L, 4);
    Symbol symbol;
    symbol.iObject = obj->obj->clone();
    symbol.iTransformations = ETransformationsAffine;
    s->addSymbol(Attribute(true, name), symbol);
  } else {
    Kind kind = Kind(luaL_checkoption(L, 2, nullptr, kind_names));
    const char *name = luaL_checklstring(L, 3, nullptr);
    Attribute sym(true, String(name));
    Attribute value = check_absolute_attribute(kind, L, 4);
    s->add(kind, sym, value);
  }
  return 0;
}

static int sheet_addfrom(lua_State *L)
{
  StyleSheet *s = check_sheet(L, 1)->sheet;
  StyleSheet *t = check_sheet(L, 2)->sheet;
  Kind kind = Kind(luaL_checkoption(L, 3, nullptr, kind_names));
  const char *name = luaL_checklstring(L, 4, nullptr);
  Attribute sym(true, name);
  switch (kind) {
  case EGradient: {
    const Gradient *g = t->findGradient(sym);
    if (!g)
      luaL_argerror(L, 4, "no such gradient");
    s->addGradient(sym, *g);
    break; }
  case EEffect: {
    const Effect *e = t->findEffect(sym);
    if (!e)
      luaL_argerror(L, 4, "no such effect");
    s->addEffect(sym, *e);
    break; }
  case ETiling: {
    const Tiling *g = t->findTiling(sym);
    if (!g)
      luaL_argerror(L, 4, "no such tiling");
    s->addTiling(sym, *g);
    break; }
  default:
    luaL_argerror(L, 3, "cannot handle this kind");
    break;
  }
  return 0;
}

static int sheet_remove(lua_State *L)
{
  StyleSheet *s = check_sheet(L, 1)->sheet;
  Kind kind = Kind(luaL_checkoption(L, 2, nullptr, kind_names));
  const char *name = luaL_checklstring(L, 3, nullptr);
  Attribute sym(true, name);
  s->remove(kind, sym);
  return 0;
}

static int sheet_isStandard(lua_State *L)
{
  SSheet *p = check_sheet(L, 1);
  lua_pushboolean(L, p->sheet->isStandard());
  return 1;
}

static int sheet_name(lua_State *L)
{
  SSheet *p = check_sheet(L, 1);
  String n = p->sheet->name();
  if (n.empty())
    lua_pushnil(L);
  else
    push_string(L, n);
  return 1;
}

static int sheet_xml(lua_State *L)
{
  SSheet *p = check_sheet(L, 1);
  bool with_bitmaps = lua_toboolean(L, 2);
  String data;
  StringStream stream(data);
  p->sheet->saveAsXml(stream, with_bitmaps);
  push_string(L, data);
  return 1;
}

static int sheet_setName(lua_State *L)
{
  SSheet *p = check_sheet(L, 1);
  const char *name = luaL_checklstring(L, 2, nullptr);
  p->sheet->setName(name);
  return 0;
}

static int sheet_set(lua_State *L)
{
  StyleSheet *s = check_sheet(L, 1)->sheet;
  int what = luaL_checkoption(L, 2, nullptr, set_names);
  switch (what) {
  case 0: // preamble
    s->setPreamble(luaL_checklstring(L, 3, nullptr));
    break;
  case 1: // linecap
    s->setLineCap(check_property(EPropLineCap, L, 3).lineCap());
    break;
  case 2: // linejoin
    s->setLineJoin(check_property(EPropLineJoin, L, 3).lineJoin());
    break;
  case 3: // fillrule
    s->setFillRule(check_property(EPropFillRule, L, 3).fillRule());
    break;
  default:
    luaL_argerror(L, 2, "invalid kind for 'set'");
    break;
  }
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg sheet_methods[] = {
  { "__gc", sheet_destructor },
  { "__tostring", sheet_tostring },
  { "clone", sheet_clone },
  { "xml", sheet_xml },
  { "add", sheet_add },
  { "addFrom", sheet_addfrom },
  { "remove", sheet_remove },
  { "set", sheet_set },
  { "isStandard", sheet_isStandard },
  { "name", sheet_name },
  { "setName", sheet_setName },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

void ipelua::push_cascade(lua_State *L, Cascade *s0, bool owned)
{
  SCascade *s = (SCascade *) lua_newuserdata(L, sizeof(SCascade));
  s->owned = owned;
  s->cascade = s0;
  luaL_getmetatable(L, "Ipe.cascade");
  lua_setmetatable(L, -2);
}

int ipelua::cascade_constructor(lua_State *L)
{
  push_cascade(L, new Cascade());
  return 1;
}

static int cascade_clone(lua_State *L)
{
  SCascade *s = check_cascade(L, 1);
  push_cascade(L, new Cascade(*s->cascade));
  return 1;
}

static int cascade_destructor(lua_State *L)
{
  SCascade *s = check_cascade(L, 1);
  if (s->owned)
    delete s->cascade;
  s->cascade = nullptr;
  return 0;
}

static int cascade_tostring(lua_State *L)
{
  check_cascade(L, 1);
  lua_pushfstring(L, "Cascade@%p", lua_topointer(L, 1));
  return 1;
}

// --------------------------------------------------------------------

// also works for symbol, gradient, tiling
static int cascade_allNames(lua_State *L)
{
  SCascade *p = check_cascade(L, 1);
  Kind kind = Kind(luaL_checkoption (L, 2, nullptr, kind_names));
  AttributeSeq seq;
  p->cascade->allNames(kind, seq);
  lua_createtable(L, seq.size(), 0);
  for (int i = 0; i < size(seq); ++i) {
    push_string(L, seq[i].string());
    lua_rawseti(L, -2, i + 1);
  }
  return 1;
}

static int push_layout(lua_State *L, const Layout &l)
{
  lua_createtable(L, 0, 7);
  push_vector(L, l.iPaperSize);
  lua_setfield(L, -2, "papersize");
  push_vector(L, l.iOrigin);
  lua_setfield(L, -2, "origin");
  push_vector(L, l.iFrameSize);
  lua_setfield(L, -2, "framesize");
  lua_pushnumber(L, l.iParagraphSkip);
  lua_setfield(L, -2, "paragraph_skip");
  lua_pushboolean(L, l.iCrop);
  lua_setfield(L, -2, "crop");
  return 1;
}

static int push_titlestyle(lua_State *L, const StyleSheet::TitleStyle &s)
{
  if (!s.iDefined)
    return 0;
  lua_createtable(L, 0, 5);
  push_vector(L, s.iPos);
  lua_setfield(L, -2, "pos");
  push_string(L, s.iSize.string());
  lua_setfield(L, -2, "size");
  push_string(L, s.iColor.string());
  lua_setfield(L, -2, "color");
  lua_pushstring(L, horizontal_alignment_names[s.iHorizontalAlignment]);
  lua_setfield(L, -2, "horizontalalignment");
  lua_pushstring(L, vertical_alignment_names[s.iVerticalAlignment]);
  lua_setfield(L, -2, "verticalalignment");
  return 1;
}

static int push_gradient(lua_State *L, const Gradient &g)
{
  lua_createtable(L, 0, 6);
  lua_pushstring(L, (g.iType == Gradient::EAxial) ? "axial" : "radial");
  lua_setfield(L, -2, "type");
  lua_createtable(L, 2, 0);
  push_vector(L, g.iV[0]);
  lua_rawseti(L, -2, 1);
  push_vector(L, g.iV[1]);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "v");
  lua_pushboolean(L, g.iExtend);
  lua_setfield(L, -2, "extend");
  push_matrix(L, g.iMatrix);
  lua_setfield(L, -2, "matrix");
  if (g.iType == Gradient::ERadial) {
    lua_createtable(L, 2, 0);
    lua_pushnumber(L, g.iRadius[0]);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, g.iRadius[1]);
    lua_rawseti(L, -2, 2);
    lua_setfield(L, -2, "radius");
  }
  lua_createtable(L, g.iStops.size(), 0);
  for (int i = 0; i < int(g.iStops.size()); ++i) {
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, g.iStops[i].offset);
    lua_setfield(L, -2, "offset");
    push_color(L, g.iStops[i].color);
    lua_setfield(L, -2, "color");
    lua_rawseti(L, -2, i+1);
  }
  lua_setfield(L, -2, "stops");
  return 1;
}

// find will also work for the values that are "set"
static int cascade_find(lua_State *L)
{
  Cascade *s = check_cascade(L, 1)->cascade;
  luaL_checktype(L, 2, LUA_TSTRING);
  int what = test_option(L, 2, set_names);
  switch (what) {
  case 0: // preamble
    push_string(L, s->findPreamble());
    break;
  case 1: // linecap
    push_attribute(L, Attribute(s->lineCap()));
    break;
  case 2: // linejoin
    push_attribute(L, Attribute(s->lineJoin()));
    break;
  case 3: // fillrule
    push_attribute(L, Attribute(s->fillRule()));
    break;
  case 4: { // symbol
    const char *name = luaL_checklstring(L, 3, nullptr);
    const Symbol *symbol = s->findSymbol(Attribute(true, name));
    if (symbol)
      push_object(L, symbol->iObject->clone());
    else
      lua_pushnil(L);
    break; }
  case 5:  // layout
    push_layout(L, *s->findLayout());
    break;
  case 6: { // gradient
    const char *name = luaL_checklstring(L, 3, nullptr);
    push_gradient(L, *s->findGradient(Attribute(true, name)));
    break; }
  case 7:  // titlestyle
    push_titlestyle(L, *s->findTitleStyle());
    break;
  default: {
    Kind kind = Kind(luaL_checkoption(L, 2, nullptr, kind_names));
    if (lua_isstring(L, 3)) {
      const char *name = luaL_checklstring(L, 3, nullptr);
      push_attribute(L, s->find(kind, Attribute(true, String(name))));
    } else {
      lua_pushvalue(L, 3);  // value is not symbolic, simply return the value itself
    }
    break; }
  }
  return 1;
}

static int cascade_has(lua_State *L)
{
  Cascade *p = check_cascade(L, 1)->cascade;
  Kind kind = Kind(luaL_checkoption(L, 2, nullptr, kind_names));
  const char *name = luaL_checklstring(L, 3, nullptr);
  Attribute sym(true, String(name));
  lua_pushboolean(L, p->has(kind, sym));
  return 1;
}

static int cascade_count(lua_State *L)
{
  Cascade *p = check_cascade(L, 1)->cascade;
  lua_pushnumber(L, p->count());
  return 1;
}

static int cascade_sheet(lua_State *L)
{
  Cascade *p = check_cascade(L, 1)->cascade;
  int index = (int)luaL_checkinteger(L, 2);
  luaL_argcheck(L, 1 <= index && index <= p->count(), 2, "index out of bounds");
  push_sheet(L, p->sheet(index - 1), false);
  return 1;
}

static int cascade_insert(lua_State *L)
{
  Cascade *p = check_cascade(L, 1)->cascade;
  int index = (int)luaL_checkinteger(L, 2);
  luaL_argcheck(L, 1 <= index && index <= p->count() + 1,
		2, "index out of bounds");
  SSheet *s = check_sheet(L, 3);
  StyleSheet *sheet = s->sheet;
  if (!s->owned)
    sheet = new StyleSheet(*s->sheet);
  p->insert(index - 1, sheet);
  s->owned = false;  // now owned by cascade
  return 0;
}

static int cascade_remove(lua_State *L)
{
  Cascade *p = check_cascade(L, 1)->cascade;
  int index = (int)luaL_checkinteger(L, 2);
  luaL_argcheck(L, 1 <= index && index <= p->count(),
		2, "index out of bounds");
  p->remove(index - 1);
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg cascade_methods[] = {
  { "__gc", cascade_destructor },
  { "__tostring", cascade_tostring },
  { "clone", cascade_clone },
  { "allNames", cascade_allNames },
  { "find", cascade_find },
  { "has", cascade_has },
  { "count", cascade_count },
  { "sheet", cascade_sheet },
  { "insert", cascade_insert },
  { "remove", cascade_remove },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

int ipelua::open_ipestyle(lua_State *L)
{
  make_metatable(L, "Ipe.sheet", sheet_methods);
  make_metatable(L, "Ipe.cascade", cascade_methods);

  return 0;
}

// --------------------------------------------------------------------

