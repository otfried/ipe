// --------------------------------------------------------------------
// ipeluaobj.cpp
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

#include "ipelua.h"

#include "ipereference.h"
#include "ipegroup.h"
#include "ipepath.h"
#include "ipetext.h"
#include "ipeimage.h"
#include "ipeiml.h"

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static const char *const type_names[] =
  { "group", "path", "text", "image", "reference", nullptr };

static const char *const pinned_names[] =
  { "none", "horizontal", "vertical", "fixed", nullptr };

static const char *const pathmode_names[] =
  { "stroked", "strokedfilled", "filled", nullptr };

static const char *const transformation_names[] =
  { "translations", "rigid", "affine", nullptr };

const char *const ipelua::horizontal_alignment_names[] =
  { "left", "right", "hcenter", nullptr };

const char *const ipelua::vertical_alignment_names[] =
  { "bottom", "baseline", "top", "vcenter", nullptr };

const char *const ipelua::linejoin_names[] =
  { "normal", "miter", "round", "bevel", nullptr };

const char *const ipelua::linecap_names[] =
  { "normal", "butt", "round", "square", nullptr };

const char *const ipelua::fillrule_names[] =
  { "normal", "wind", "evenodd", nullptr };

const char * const ipelua::segtype_names[] =
  { "arc", "segment", "spline", "oldspline", "cardinal", "spiro", nullptr };

static int segtype_cp[] = { 2, 2, 0, 0, 0, 0 };

static const char *const splinetype_names[] =
  { "bspline", "cardinal", "spiro" };

// --------------------------------------------------------------------

void ipelua::push_string(lua_State *L, String str)
{
  lua_pushlstring(L, str.data(), str.size());
}

void ipelua::push_object(lua_State *L, Object *s0, bool owned)
{
  SObject *s = (SObject *) lua_newuserdata(L, sizeof(SObject));
  s->owned = owned;
  s->obj = s0;
  luaL_getmetatable(L, "Ipe.object");
  lua_setmetatable(L, -2);
}

void ipelua::push_color(lua_State *L, Color color)
{
  lua_createtable(L, 0, 3);
  lua_pushnumber(L, color.iRed.toDouble());
  lua_setfield(L, -2, "r");
  lua_pushnumber(L, color.iGreen.toDouble());
  lua_setfield(L, -2, "g");
  lua_pushnumber(L, color.iBlue.toDouble());
  lua_setfield(L, -2, "b");
}

void ipelua::push_attribute(lua_State *L, Attribute att)
{
  if (att.isBoolean())
    lua_pushboolean(L, att.boolean());
  else if (att.isSymbolic() || att.isString() || att.isEnum())
    push_string(L, att.string());
  else if (att.isNumber())
    lua_pushnumber(L, att.number().toDouble());
  else // must be color
    push_color(L, att.color());
}

// i must be positive
Color ipelua::check_color(lua_State *L, int i)
{
  luaL_checktype(L, i, LUA_TTABLE);
  lua_getfield(L, i, "r");
  lua_getfield(L, i, "g");
  lua_getfield(L, i, "b");
  double r = luaL_checknumber(L, -3);
  double g = luaL_checknumber(L, -2);
  double b = luaL_checknumber(L, -1);
  lua_pop(L, 3);
  Color color;
  color.iRed = Fixed::fromDouble(r);
  color.iGreen = Fixed::fromDouble(g);
  color.iBlue = Fixed::fromDouble(b);
  return color;
}

#if 0
// i must be positive
Attribute ipelua::check_attribute(lua_State *L, int i)
{
  if (lua_type(L, i) == LUA_TNUMBER) {
    double v = luaL_checknumber(L, i);
    return Attribute(Fixed::fromInternal(int(v * 1000 + 0.5)));
  } else if (lua_type(L, i) == LUA_TSTRING) {
    const char *s = luaL_checklstring(L, i, nullptr);
    if (!strcmp(s, "true"))
      return Attribute::Boolean(true);
    if (!strcmp(s, "false"))
      return Attribute::Boolean(false);
    if (('a' <= s[0] && s[0] <= 'z') || ('A' <= s[0] && s[0] <= 'Z'))
      return Attribute(true, s);
    else
      return Attribute(false, s);
  } else if (lua_type(L, i) == LUA_TTABLE) {
    Color color = check_color(L, i);
    return Attribute(color);
  } else if (lua_type(L, i) == LUA_TBOOLEAN) {
    return Attribute::Boolean(lua_toboolean(L, i));
  } else {
    luaL_argerror(L, i, "attribute expected");
    return Attribute::NORMAL();  // placate compiler
  }
}
#endif

// i must be positive
Attribute ipelua::check_color_attribute(lua_State *L, int i)
{
  if (lua_type(L, i) == LUA_TSTRING) {
    const char *s = luaL_checklstring(L, i, nullptr);
    return Attribute(true, s);
  } else {
    Color color = check_color(L, i);
    return Attribute(color);
  }
}

// i must be positive
Attribute ipelua::check_bool_attribute(lua_State *L, int i)
{
  static const char * const bool_names[] = { "false", "true" };
  if (lua_type(L, i) == LUA_TBOOLEAN)
    return Attribute::Boolean(lua_toboolean(L, i));
  int val = luaL_checkoption(L, i, nullptr, bool_names);
  return Attribute::Boolean(val);
}

// i must be positive
Attribute ipelua::check_number_attribute(lua_State *L, int i)
{
  if (lua_type(L, i) == LUA_TNUMBER) {
    double v = luaL_checknumber(L, i);
    return Attribute(Fixed::fromInternal(int(v * 1000 + 0.5)));
  }
  const char *s = luaL_checklstring(L, i, nullptr);
  return Attribute(true, s);
}

Attribute ipelua::check_property(Property prop, lua_State *L, int i)
{
  int val;
  switch (prop) {
  case EPropHorizontalAlignment:
    val = luaL_checkoption(L, i, nullptr, horizontal_alignment_names);
    return Attribute(THorizontalAlignment(val));
  case EPropVerticalAlignment:
    val = luaL_checkoption(L, i, nullptr, vertical_alignment_names);
    return Attribute(TVerticalAlignment(val));
  case EPropLineJoin:
    val = luaL_checkoption(L, i, nullptr, linejoin_names);
    return Attribute(TLineJoin(val));
  case EPropLineCap:
    val = luaL_checkoption(L, i, nullptr, linecap_names);
    return Attribute(TLineCap(val));
  case EPropFillRule:
    val = luaL_checkoption(L, i, nullptr, fillrule_names);
    return Attribute(TFillRule(val));
  case EPropPinned:
    val = luaL_checkoption(L, i, nullptr, pinned_names);
    return Attribute(TPinned(val));
  case EPropTransformations:
    val = luaL_checkoption(L, i, nullptr, transformation_names);
    return Attribute(TTransformations(val));
  case EPropPathMode:
    val = luaL_checkoption(L, i, nullptr, pathmode_names);
    return Attribute(TPathMode(val));
  case EPropSplineType:
    val = luaL_checkoption(L, i, nullptr, splinetype_names);
    return Attribute(TSplineType(val));
  case EPropPen:
  case EPropSymbolSize:
  case EPropFArrowSize:
  case EPropRArrowSize:
  case EPropTextSize:
    return check_number_attribute(L, i);
  case EPropWidth: { // absolute number only!
    double v = luaL_checknumber(L, i);
    return Attribute(Fixed::fromInternal(int(v * 1000 + 0.5))); }
  case EPropFArrowShape:
  case EPropRArrowShape:
  case EPropMarkShape:
  case EPropTextStyle:
  case EPropLabelStyle:
  case EPropOpacity:
  case EPropStrokeOpacity:
  case EPropGradient:
  case EPropDecoration:
  case EPropTiling:  // symbolic string only
    return Attribute(true, luaL_checklstring(L, i, nullptr));
  case EPropStrokeColor:
  case EPropFillColor:
    return check_color_attribute(L, i);
  case EPropDashStyle:
    return Attribute::makeDashStyle(luaL_checklstring(L, i, nullptr));
  case EPropFArrow:
  case EPropRArrow:
  case EPropMinipage:
  case EPropTransformableText:
    return check_bool_attribute(L, i);
  }
  return Attribute::NORMAL(); // placate compiler
}

static void get_attribute(lua_State *L, int i, Property prop,
			  const char *key, Attribute &att)
{
  lua_getfield(L, i, key);
  if (!lua_isnil(L, -1))
    att = check_property(prop, L, lua_gettop(L));  // arg must be positive
  lua_pop(L, 1);
}

static void get_boolean(lua_State *L, int i, const char *key, bool &att)
{
  lua_getfield(L, i, key);
  att = lua_toboolean(L, -1);
  lua_pop(L, 1);
}

static int get_option(lua_State *L, int i, const char *key,
		      const char *const *names)
{
  lua_getfield(L, i, key);
  int val;
  if (!lua_isnil(L, -1))
    val = luaL_checkoption(L, -1, nullptr, names);
  else
    val = -1;
  lua_pop(L, 1);
  return val;
}

// i must be positive
void ipelua::check_allattributes(lua_State *L, int i, AllAttributes &all)
{
  luaL_checktype(L, i, LUA_TTABLE);
  get_attribute(L, i, EPropStrokeColor, "stroke", all.iStroke);
  get_attribute(L, i, EPropFillColor, "fill", all.iFill);
  get_attribute(L, i, EPropDashStyle, "dashstyle", all.iDashStyle);
  get_attribute(L, i, EPropPen, "pen", all.iPen);
  get_boolean(L, i, "farrow", all.iFArrow);
  get_boolean(L, i, "rarrow", all.iRArrow);
  get_attribute(L, i, EPropFArrowShape, "farrowshape", all.iFArrowShape);
  get_attribute(L, i, EPropRArrowShape, "rarrowshape", all.iRArrowShape);
  get_attribute(L, i, EPropFArrowSize, "farrowsize", all.iFArrowSize);
  get_attribute(L, i, EPropRArrowSize, "rarrowsize", all.iRArrowSize);
  get_attribute(L, i, EPropSymbolSize, "symbolsize", all.iSymbolSize);
  get_attribute(L, i, EPropMarkShape, "markshape", all.iMarkShape);
  get_attribute(L, i, EPropTextSize, "textsize", all.iTextSize);
  get_boolean(L, i, "transformabletext", all.iTransformableText);
  get_attribute(L, i, EPropTextStyle, "textstyle", all.iTextStyle);
  get_attribute(L, i, EPropTextStyle, "labelstyle", all.iLabelStyle);
  get_attribute(L, i, EPropOpacity, "opacity", all.iOpacity);
  get_attribute(L, i, EPropStrokeOpacity, "strokeopacity", all.iStrokeOpacity);
  get_attribute(L, i, EPropTiling, "tiling", all.iTiling);
  get_attribute(L, i, EPropGradient, "gradient", all.iGradient);

  int t;
  t = get_option(L, i, "horizontalalignment", horizontal_alignment_names);
  if (t >= 0) all.iHorizontalAlignment = THorizontalAlignment(t);

  t = get_option(L, i, "verticalalignment", vertical_alignment_names);
  if (t >= 0) all.iVerticalAlignment = TVerticalAlignment(t);

  t = get_option(L, i, "linejoin", linejoin_names);
  if (t >= 0) all.iLineJoin = TLineJoin(t);

  t = get_option(L, i, "linecap", linecap_names);
  if (t >= 0) all.iLineCap = TLineCap(t);

  t = get_option(L, i, "fillrule", fillrule_names);
  if (t >= 0) all.iFillRule = TFillRule(t);

  t = get_option(L, i, "pinned", pinned_names);
  if (t >= 0) all.iPinned = TPinned(t);

  t = get_option(L, i, "transformations", transformation_names);
  if (t >= 0) all.iTransformations = TTransformations(t);

  t = get_option(L, i, "splinetype", splinetype_names);
  if (t >= 0) all.iSplineType = TSplineType(t);

  t = get_option(L, i, "pathmode", pathmode_names);
  if (t >= 0) all.iPathMode = TPathMode(t);
}

// --------------------------------------------------------------------

int ipelua::reference_constructor(lua_State *L)
{
  AllAttributes all;
  check_allattributes(L, 1, all);
  Attribute name(true, luaL_checklstring(L, 2, nullptr));
  Vector *v = check_vector(L, 3);
  Reference *r = new Reference(all, name, *v);
  push_object(L, r);
  return 1;
}

int ipelua::text_constructor(lua_State *L)
{
  AllAttributes all;
  check_allattributes(L, 1, all);
  const char *s = luaL_checklstring(L, 2, nullptr);
  Vector *v = check_vector(L, 3);
  double width = 10.0;
  Text::TextType type = Text::ELabel;
  if (lua_isnumber(L, 4)) {
    type = Text::EMinipage;
    width = luaL_checknumber(L, 4);
  }
  Text *t = new Text(all, s, *v, type, width);
  push_object(L, t);
  return 1;
}

int ipelua::path_constructor(lua_State *L)
{
  AllAttributes all;
  check_allattributes(L, 1, all);
  Shape shape = check_shape(L, 2);
  bool withArrows = lua_toboolean(L, 3);
  Path *p = new Path(all, shape, withArrows);
  push_object(L, p);
  return 1;
}

int ipelua::group_constructor(lua_State *L)
{
  luaL_checktype(L, 1, LUA_TTABLE);
  Group *g = new Group();
  // make sure Lua will collect it if exception happens
  push_object(L, g);
  int no = lua_rawlen(L, 1);
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, 1, i);
    luaL_argcheck(L, is_type(L, -1, "Ipe.object"), 1,
		  "element is not an Ipe object");
    SObject *p = (SObject *) lua_touserdata(L, -1);
    g->push_back(p->obj->clone());
    lua_pop(L, 1); // object i
  }
  return 1;
}

int ipelua::xml_constructor(lua_State *L)
{
  String s = luaL_checklstring(L, 1, nullptr);
  Buffer buffer(s.data(), s.size());
  BufferSource source(buffer);
  ImlParser parser(source);
  String tag = parser.parseToTag();
  if (tag == "ipeselection") {
    lua_newtable(L);  // the objects
    lua_newtable(L);  // their layers
    int index = 1;

    XmlAttributes attr;
    if (!parser.parseAttributes(attr))
      return 0;
    tag = parser.parseToTag();

    while (tag == "bitmap") {
      if (!parser.parseBitmap())
	return false;
      tag = parser.parseToTag();
    }

    for (;;) {
      if (tag == "/ipeselection")
	return 2;
      String layer;
      Object *obj = parser.parseObject(tag, layer);
      if (!obj)
	return 0;
      push_object(L, obj);
      lua_rawseti(L, -3, index);
      push_string(L, layer);
      lua_rawseti(L, -2, index);
      ++index;
      tag = parser.parseToTag();
    }
  } else {
    Object *obj = parser.parseObject(tag);
    if (obj) {
      push_object(L, obj);
      return 1;
    }
  }
  return 0;
}

// --------------------------------------------------------------------

static int object_destructor(lua_State *L)
{
  SObject *r = check_object(L, 1);
  if (r->owned && r->obj)
    delete r->obj;
  r->obj = nullptr;
  return 0;
}

static int object_tostring(lua_State *L)
{
  SObject *s = check_object(L, 1);
  lua_pushfstring(L, "Object(%s)@%p",
		  type_names[s->obj->type()],
		  lua_topointer(L, 1));
  return 1;
}

static int object_type(lua_State *L)
{
  SObject *s = check_object(L, 1);
  lua_pushstring(L, type_names[s->obj->type()]);
  return 1;
}

static int object_set(lua_State *L)
{
  SObject *s = check_object(L, 1);
  Property prop = Property(luaL_checkoption(L, 2, nullptr, property_names));
  Attribute value = check_property(prop, L, 3);
  s->obj->setAttribute(prop, value);
  return 0;
}

static int object_get(lua_State *L)
{
  SObject *s = check_object(L, 1);
  Property prop = Property(luaL_checkoption(L, 2, nullptr, property_names));
  Attribute value = s->obj->getAttribute(prop);
  push_attribute(L, value);
  return 1;
}

static int object_get_custom(lua_State *L)
{
  SObject *s = check_object(L, 1);
  Attribute value = s->obj->getCustom();
  push_attribute(L, value);
  return 1;
}

static int object_set_custom(lua_State *L)
{
  SObject *s = check_object(L, 1);
  const char *value = luaL_checklstring(L, 2, nullptr);
  s->obj->setCustom(Attribute(false, value));
  return 0;
}

static int object_position(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  luaL_argcheck(L, obj->type() == Object::EText ||
		obj->type() == Object::EReference, 1,
		"not a text or reference object");
  if (obj->asReference()) {
    push_vector(L, obj->asReference()->position());
    return 1;
  } else if (obj->asText()) {
    push_vector(L, obj->asText()->position());
    return 1;
  }
  return 0;
}

static int object_text(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  if (obj->type() == Object::EGroup) {
    push_string(L, obj->asGroup()->url());
  } else {
    luaL_argcheck(L, obj->type() == Object::EText, 1, "not a text object");
    push_string(L, obj->asText()->text());
  }
  return 1;
}

static int object_setText(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  String s = luaL_checklstring(L, 2, nullptr);
  if (obj->type() == Object::EGroup) {
    obj->asGroup()->setUrl(s);
  } else {
    luaL_argcheck(L, obj->type() == Object::EText, 1, "not a text object");
    obj->asText()->setText(s);
  }
  return 0;
}

static int object_text_dimensions(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  luaL_argcheck(L, obj->type() == Object::EText, 1, "not a text object");
  Text *t = obj->asText();
  lua_pushnumber(L, t->width());
  lua_pushnumber(L, t->height());
  lua_pushnumber(L, t->depth());
  return 3;
}

static int object_clone(lua_State *L)
{
  SObject *s = check_object(L, 1);
  push_object(L, s->obj->clone());
  return 1;
}

static int object_matrix(lua_State *L)
{
  SObject *s = check_object(L, 1);
  push_matrix(L, s->obj->matrix());
  return 1;
}

static int object_setMatrix(lua_State *L)
{
  SObject *s = check_object(L, 1);
  Matrix *m = check_matrix(L, 2);
  s->obj->setMatrix(*m);
  return 0;
}

static int object_elements(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  luaL_argcheck(L, obj->type() == Object::EGroup, 1, "not a group object");
  Group *g = obj->asGroup();
  lua_createtable(L, g->count(), 0);
  for (int i = 0; i < g->count(); ++i) {
    push_object(L, g->object(i)->clone());
    lua_rawseti(L, -2, i+1);
  }
  return 1;
}

static int object_element(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  luaL_argcheck(L, obj->type() == Object::EGroup, 1, "not a group object");
  int idx = (int) luaL_checkinteger(L, 2);
  Group *g = obj->asGroup();
  luaL_argcheck(L, 1 <= idx && idx <= g->count(), 2, "incorrect element index");
  push_object(L, g->object(idx-1)->clone());
  return 1;
}

static int object_elementType(lua_State *L)
{
  Object *obj = check_object(L, 1)->obj;
  luaL_argcheck(L, obj->type() == Object::EGroup, 1, "not a group object");
  int idx = (int) luaL_checkinteger(L, 2);
  Group *g = obj->asGroup();
  luaL_argcheck(L, 1 <= idx && idx <= g->count(), 2, "incorrect element index");
  lua_pushstring(L, type_names[g->object(idx-1)->type()]);
  return 1;
}

static int object_xml(lua_State *L)
{
  SObject *obj = check_object(L, 1);
  String s;
  StringStream stream(s);
  obj->obj->saveAsXml(stream, String());
  push_string(L, s);
  return 1;
}

static int object_addToBBox(lua_State *L)
{
  SObject *s = check_object(L, 1);
  Rect *r = check_rect(L, 2);
  Matrix *m = check_matrix(L, 3);
  bool cp = true;
  if (lua_type(L, 4) == LUA_TBOOLEAN)
    cp = lua_toboolean(L, 4);
  s->obj->addToBBox(*r, *m, cp);
  return 0;
}

// --------------------------------------------------------------------

static const char * const subpath_names[] =
  { "curve", "ellipse", "closedspline", nullptr };

static bool collect_cp(lua_State *L, std::vector<Vector> &cp)
{
  for (int i = 0; ; ++i) {
    lua_rawgeti(L, -1, i+1);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      return true;
    }
    if (!is_type(L, -1, "Ipe.vector"))
      return false;
    Vector *v = check_vector(L, -1);
    cp.push_back(*v);
    lua_pop(L, 1); // cp
  }
}

static SubPath *get_ellipse(lua_State *L, int index)
{
  lua_rawgeti(L, -1, 1);  // get matrix
  if (!is_type(L, -1, "Ipe.matrix"))
    luaL_error(L, "element %d has no matrix", index);
  Matrix *m = check_matrix(L, -1);
  lua_pop(L, 1);  // matrix
  return new Ellipse(*m);
}

static SubPath *get_closedspline(lua_State *L, int index)
{
  std::vector<Vector> cp;
  if (!collect_cp(L, cp))
    luaL_error(L, "non-vector control point in element %d", index);
  return new ClosedSpline(cp);
}

static SubPath *get_curve(lua_State *L, int index)
{
  std::unique_ptr<Curve> c(new Curve());
  lua_getfield(L, -1, "closed");
  if (!lua_isboolean(L, -1))
    luaL_error(L, "element %d has no 'closed' field", index);
  bool closed = lua_toboolean(L, -1);
  lua_pop(L, 1);  // closed
  for (int i = 0; ; ++i) {
    lua_rawgeti(L, -1, i+1);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      if (c->countSegments() == 0)
	luaL_error(L, "element %d has no segments", index);
      c->setClosed(closed);
      return c.release();
    }
    if (!lua_istable(L, -1))
      luaL_error(L, "segment %d of element %d is not a table", i+1, index);
    lua_getfield(L, -1, "type");
    if (!lua_isstring(L, -1))
      luaL_error(L, "segment %d of element %d has no type", i+1, index);
    int type = test_option(L, -1, segtype_names);
    if (type < 0)
      luaL_error(L, "segment %d of element %d has invalid type", i+1, index);
    lua_pop(L, 1); // pop type
    std::vector<Vector> cp;
    if (!collect_cp(L, cp))
      luaL_error(L, "non-vector control point in segment %d of element %d",
		 i+1, index);
    int cpn = segtype_cp[type];
    if (int(cp.size()) < 2 || (cpn > 0 && int(cp.size()) != cpn))
      luaL_error(L, "invalid # of control points in segment %d of element %d",
		 i+1, index);
    switch (type) {
    case CurveSegment::EArc: {
      lua_getfield(L, -1, "arc");
      if (!is_type(L, -1, "Ipe.arc"))
	luaL_error(L, "segment %d of element %d has no arc", i+1, index);
      Arc *a = check_arc(L, -1);
      lua_pop(L, 1); // arc
      c->appendArc(a->iM, cp[0], cp[1]);
      break; }
    case CurveSegment::ESegment:
      c->appendSegment(cp[0], cp[1]);
      break;
    case CurveSegment::ESpline:
      c->appendSpline(cp);
      break;
    case CurveSegment::EOldSpline:
      c->appendOldSpline(cp);
      break;
    case CurveSegment::ECardinalSpline: {
      lua_getfield(L, -1, "tension");
      if (!lua_isnumber(L, -1))
	luaL_error(L, "segment %d of element %d has no tension", i+1, index);
      float tension = lua_tonumberx(L, -1, nullptr);
      c->appendCardinalSpline(cp, tension);
      lua_pop(L, 1); // tension
      break; }
    case CurveSegment::ESpiroSpline:
      c->appendSpiroSpline(cp);
      break;
    default:
      break;
    }
    lua_pop(L, 1); // pop segment table
  }
}

// index must be positive
Shape ipelua::check_shape(lua_State *L, int index)
{
  luaL_checktype(L, index, LUA_TTABLE);
  Shape shape;
  for (int i = 0; ; ++i) {
    lua_rawgeti(L, index, i+1);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      return shape;
    }
    if (!lua_istable(L, -1))
      luaL_error(L, "element %d is not a table", i+1);
    lua_getfield(L, -1, "type");
    // stack: subpath, type
    if (!lua_isstring(L, -1))
      luaL_error(L, "element %d has no type", i+1);
    int type = test_option(L, -1, subpath_names);
    lua_pop(L, 1); // type
    switch (type) {
    case SubPath::EEllipse:
      shape.appendSubPath(get_ellipse(L, i+1));
      break;
    case SubPath::EClosedSpline:
      shape.appendSubPath(get_closedspline(L, i+1));
      break;
    case SubPath::ECurve:
      shape.appendSubPath(get_curve(L, i+1));
      break;
    default:
      luaL_error(L, "element %d has invalid type", i+1);
    }
    lua_pop(L, 1); // subpath
  }
}

static void push_segment(lua_State *L, const CurveSegment &seg)
{
  int fields = 1;
  if (seg.type() == CurveSegment::EArc) ++fields;
  if (seg.type() == CurveSegment::ECardinalSpline) ++fields;
  lua_createtable(L, seg.countCP(), fields);
  lua_pushstring(L, segtype_names[seg.type()]);
  lua_setfield(L, -2, "type");
  for (int i = 0; i < seg.countCP(); ++i) {
    push_vector(L, seg.cp(i));
    lua_rawseti(L, -2, i+1);
  }
  if (seg.type() == CurveSegment::EArc) {
    push_arc(L, seg.arc());
    lua_setfield(L, -2, "arc");
  }
  if (seg.type() == CurveSegment::ECardinalSpline) {
    lua_pushnumber(L, round(seg.tension() * 10000.0) / 10000.0);
    lua_setfield(L, -2, "tension");
  }
}

static void push_subpath(lua_State *L, const SubPath *sp)
{
  switch (sp->type()) {
  case SubPath::EEllipse:
    lua_createtable(L, 1, 1);
    lua_pushstring(L, "ellipse");
    lua_setfield(L, -2, "type");
    push_matrix(L, sp->asEllipse()->matrix());
    lua_rawseti(L, -2, 1);
    break;
  case SubPath::EClosedSpline: {
    const ClosedSpline *cs = sp->asClosedSpline();
    lua_createtable(L, cs->iCP.size(), 1);
    lua_pushstring(L, "closedspline");
    lua_setfield(L, -2, "type");
    for (int j = 0; j < size(cs->iCP); ++j) {
      push_vector(L, cs->iCP[j]);
      lua_rawseti(L, -2, j+1);
    }
    break; }
  case SubPath::ECurve: {
    const Curve *c = sp->asCurve();
    lua_createtable(L, c->countSegments(), 2);
    lua_pushstring(L, "curve");
    lua_setfield(L, -2, "type");
    lua_pushboolean(L, c->closed());
    lua_setfield(L, -2, "closed");
    for (int j = 0; j < c->countSegments(); ++j) {
      push_segment(L, c->segment(j));
      lua_rawseti(L, -2, j+1);
    }
    break; }
  }
}

static void push_shape(lua_State *L, const Shape &shape)
{
  lua_createtable(L, shape.countSubPaths(), 0);
  for (int i = 0; i < shape.countSubPaths(); ++i) {
    push_subpath(L, shape.subPath(i));
    lua_rawseti(L, -2, i+1);
  }
}

static int object_shape(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EPath, 1, "not a path object");
  const Shape &shape = s->asPath()->shape();
  push_shape(L, shape);
  return 1;
}

static int object_setShape(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EPath, 1, "not a path object");
  Shape shape = check_shape(L, 2);
  s->asPath()->setShape(shape);
  return 1;
}

static int object_count(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EGroup, 1, "not a group object");
  lua_pushnumber(L, s->asGroup()->count());
  return 1;
}

static int object_clip(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EGroup, 1, "not a group object");
  const Shape &shape = s->asGroup()->clip();
  if (shape.countSubPaths() > 0) {
    push_shape(L, shape);
    return 1;
  } else
    return 0;
}

static int object_setclip(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EGroup, 1, "not a group object");
  if (lua_isnoneornil(L, 2)) {
    s->asGroup()->setClip(Shape());
  } else {
    Shape shape = check_shape(L, 2);
    s->asGroup()->setClip(shape);
  }
  return 0;
}

static int object_symbol(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EReference, 1,
		"not a reference object");
  push_string(L, s->asReference()->name().string());
  return 1;
}

static int object_info(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EImage, 1, "not an image object");
  Bitmap bm = s->asImage()->bitmap();
  lua_createtable(L, 0, 7);
  lua_pushnumber(L, bm.width());
  lua_setfield(L, -2, "width");
  lua_pushnumber(L, bm.height());
  lua_setfield(L, -2, "height");
  String format;
  if (bm.isJpeg())
    format = "jpg";
  else {
    if (bm.isGray())
      format = "gray";
    else
      format = "rgb";
    if (bm.hasAlpha())
      format += " alpha";
    else if (bm.colorKey() >= 0)
      format += " colorkeyed";
  }
  push_string(L, format);
  lua_setfield(L, -2, "format");
  return 1;
}

static int object_savePixels(lua_State *L)
{
  Object *s = check_object(L, 1)->obj;
  luaL_argcheck(L, s->type() == Object::EImage, 1, "not an image object");
  String fname = luaL_checklstring(L, 2, nullptr);
  Bitmap bm = s->asImage()->bitmap();
  bm.savePixels(fname.z());
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg object_methods[] = {
  { "__tostring", object_tostring },
  { "__gc", object_destructor },
  { "type", object_type },
  { "set", object_set },
  { "get", object_get },
  { "setCustom", object_set_custom },
  { "getCustom", object_get_custom },
  { "xml", object_xml },
  { "clone", object_clone },
  { "matrix", object_matrix },
  { "setMatrix", object_setMatrix },
  { "addToBBox", object_addToBBox },
  { "position", object_position },
  { "shape", object_shape },
  { "setShape", object_setShape },
  { "count", object_count },
  { "clip", object_clip },
  { "setClip", object_setclip },
  { "symbol", object_symbol },
  { "info", object_info },
  { "savePixels", object_savePixels },
  { "position", object_position },
  { "text", object_text },
  { "setText", object_setText },
  { "dimensions", object_text_dimensions },
  { "elements", object_elements },
  { "element", object_element },
  { "elementType", object_elementType },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

int ipelua::open_ipeobj(lua_State *L)
{
  make_metatable(L, "Ipe.object", object_methods);

  return 0;
}

// --------------------------------------------------------------------
