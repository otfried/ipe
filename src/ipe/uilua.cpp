// --------------------------------------------------------------------
// Lua bindings for user interface
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

#include "appui.h"
#include "ipecanvas.h"
#include "tools.h"

#include "ipelua.h"

#include "ipethumbs.h"

#include <cstdio>
#include <cstdlib>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static CanvasBase *check_canvas(lua_State *L, int i)
{
  AppUiBase **ui = (AppUiBase **) luaL_checkudata(L, i, "Ipe.appui");
  return (*ui)->canvas();
}

inline AppUiBase **check_appui(lua_State *L, int i)
{
  return (AppUiBase **) luaL_checkudata(L, i, "Ipe.appui");
}

static int appui_tostring(lua_State *L)
{
  check_appui(L, 1);
  lua_pushfstring(L, "AppUi@%p", lua_topointer(L, 1));
  return 1;
}

/* When the Lua model is collected, its "ui" userdata will be garbage
   collected as well.  At this point, the C++ object has long been
   deleted. */
static int appui_destructor(lua_State *L)
{
  check_appui(L, 1);
  ipeDebug("AppUi Lua destructor");
  return 0;
}

// --------------------------------------------------------------------

static int appui_setPage(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  Page *page = check_page(L, 2)->page;
  int pno = luaL_checkinteger(L, 3) - 1;
  int view = check_viewno(L, 4, page);
  Cascade *sheets = check_cascade(L, 5)->cascade;
  (*ui)->canvas()->setPage(page, pno, view, sheets);
  return 0;
}

static int appui_setResources(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Document **d = check_document(L, 2);
  const PdfResources *res = (*d)->resources();
  canvas->setResources(res);
  return 0;
}

static int appui_pan(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Vector p = canvas->pan();
  push_vector(L, p);
  return 1;
}

static int appui_setPan(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Vector *v = check_vector(L, 2);
  canvas->setPan(*v);
  return 0;
}

static int appui_zoom(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  lua_pushnumber(L, canvas->zoom());
  return 1;
}

static int appui_setZoom(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  (*ui)->setZoom(luaL_checknumber(L, 2));
  return 0;
}

static int appui_setSnapIndicator(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  (*ui)->setSnapIndicator(luaL_checklstring(L, 2, nullptr));
  return 0;
}

static int appui_pos(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  push_vector(L, canvas->pos());
  return 1;
}

static int appui_globalPos(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  push_vector(L, canvas->globalPos());
  return 1;
}

static int appui_unsnappedPos(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  push_vector(L, canvas->unsnappedPos());
  return 1;
}

static int appui_simpleSnapPos(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  push_vector(L, canvas->simpleSnapPos());
  return 1;
}

static int appui_setFifiVisible(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  canvas->setFifiVisible(lua_toboolean(L, 2));
  return 0;
}

static int appui_setInkMode(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  bool ink = lua_toboolean(L, 2);
  (*ui)->setInkMode(ink);
  (*ui)->canvas()->setInkMode(ink);
  return 0;
}

static int appui_setSelectionVisible(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  canvas->setSelectionVisible(lua_toboolean(L, 2));
  return 0;
}

static int appui_setSnap(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Snap snap = canvas->snap();
  get_snap(L, 2, snap);
  canvas->setSnap(snap);
  return 0;
}

static int appui_setAutoOrigin(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Vector *v = check_vector(L, 2);
  canvas->setAutoOrigin(*v);
  return 0;
}

static int appui_update(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  if (lua_isnone(L, 2) || lua_isboolean(L, 2)) {
    if (lua_isnone(L, 2) || lua_toboolean(L, 2))
      canvas->update();
    else
      canvas->updateTool();
  } else {
    ipe::Rect *r = check_rect(L, 2);
    Vector bl = canvas->userToDev(r->topLeft());
    Vector tr = canvas->userToDev(r->bottomRight());
    canvas->invalidate(int(bl.x-1), int(bl.y-1),
		       int(tr.x - bl.x + 2),
		       int(tr.y - bl.y + 2));
  }
  return 0;
}

static int appui_finishTool(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  (*ui)->canvas()->finishTool();
  (*ui)->explain("", 0);
  return 0;
}

static int appui_canvasSize(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  CanvasBase *canvas = (*ui)->canvas();
  push_vector(L, Vector(canvas->canvasWidth(), canvas->canvasHeight()));
  lua_pushinteger(L, (*ui)->dpi());
  return 2;
}

static void check_rgb(lua_State *L, int i, int &r, int &g, int &b)
{
  r = int(1000 * luaL_checknumber(L, i) + 0.5);
  g = int(1000 * luaL_checknumber(L, i+1) + 0.5);
  b = int(1000 * luaL_checknumber(L, i+2) + 0.5);
  luaL_argcheck(L, (0 <= r && r <= 1000 &&
		    0 <= g && g <= 1000 &&
		    0 <= b && b <= 1000), 2,
		"color components must be between 0.0 and 1.0");
}

static int appui_setCursor(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  if (lua_isnumber(L, 2)) {
    double s = lua_tonumberx(L, 2, nullptr);
    int r, g, b;
    check_rgb(L, 3, r, g, b);
    Color color(r, g, b);
    canvas->setCursor(CanvasBase::EDotCursor, s, &color);
  } else if (lua_isstring(L, 2)) {
    static const char * const cursor_names[] =
      { "standard", "hand", "cross", nullptr };
    CanvasBase::TCursor t =
      CanvasBase::TCursor(luaL_checkoption(L, 2, nullptr, cursor_names));
    canvas->setCursor(t);
  } else
    canvas->setCursor(CanvasBase::EStandardCursor);
  return 0;
}

static int appui_setNumbering(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  bool t = lua_toboolean(L, 2);
  CanvasBase::Style s = canvas->canvasStyle();
  s.numberPages = t;
  canvas->setCanvasStyle(s);
  return 0;
}

static int appui_setPretty(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  bool t = lua_toboolean(L, 2);
  CanvasBase::Style s = canvas->canvasStyle();
  s.pretty = t;
  canvas->setCanvasStyle(s);
  return 0;
}

static int appui_setScreen(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  static const char * const screen_names[] = { "normal", "maximized", "full", nullptr };
  int t = luaL_checkoption(L, 2, nullptr, screen_names);
  (*ui)->setFullScreen(t);
  return 0;
}

static int appui_type3Font(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  lua_pushboolean(L, canvas->type3Font());
  return 1;
}

// --------------------------------------------------------------------

static int appui_pantool(lua_State *L)
{
  ipeDebug("pantool");
  CanvasBase *canvas = check_canvas(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  PanTool *tool = new PanTool(canvas, page, view);
  canvas->setTool(tool);
  return 0;
}

static int appui_selecttool(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  double selectDistance = luaL_checknumber(L, 4);
  bool nonDestructive = lua_toboolean(L, 5);
  SelectTool *tool = new SelectTool(canvas, page, view, selectDistance,
				    nonDestructive);
  canvas->setTool(tool);
  return 0;
}

static int appui_transformtool(lua_State *L)
{
  static const char * const option_names[] = {
    "translate", "scale", "stretch", "rotate", "shear", nullptr };
  CanvasBase *canvas = check_canvas(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  int type = luaL_checkoption(L, 4, nullptr, option_names);
  bool withShift = lua_toboolean(L, 5);
  lua_pushvalue(L, 6);
  int method = luaL_ref(L, LUA_REGISTRYINDEX);
  IpeTransformTool *tool =
    new IpeTransformTool(canvas, page, view, TransformTool::TType(type),
			 withShift, L, method);
  if (tool->isValid()) {
    canvas->setTool(tool);
    lua_pushboolean(L, true);
    return 1;
  } else {
    delete tool;
    return 0;
  }
}

static int luatool_setcolor(lua_State *L)
{
  LuaTool *tool = (LuaTool *) lua_touserdata(L, lua_upvalueindex(1));
  int r, g, b;
  check_rgb(L, 1, r, g, b);
  tool->setColor(Color(r, g, b));
  return 0;
}

static int shapetool_setshape(lua_State *L)
{
  ShapeTool *tool = (ShapeTool *) lua_touserdata(L, lua_upvalueindex(1));
  Shape shape = check_shape(L, 1);
  int which = 0;
  if (!lua_isnoneornil(L, 2))
    which = (int)luaL_checkinteger(L, 2);
  double pen = 1.0;
  if (lua_isnumber(L, 3))
    pen = luaL_checknumber(L, 3);
  tool->setShape(shape, which, pen);
  return 0;
}

static int shapetool_setsnapping(lua_State *L)
{
  ShapeTool *tool = (ShapeTool *) lua_touserdata(L, lua_upvalueindex(1));
  bool snap = lua_toboolean(L, 1);
  bool skipLast = lua_toboolean(L, 2);
  tool->setSnapping(snap, skipLast);
  return 0;
}

static int shapetool_setmarks(lua_State *L)
{
  ShapeTool *tool = (ShapeTool *) lua_touserdata(L, lua_upvalueindex(1));
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  int no = lua_rawlen(L, 1);
  tool->clearMarks();
  for (int i = 1; i + 1 <= no; i += 2) {
    lua_rawgeti(L, 1, i);
    luaL_argcheck(L, is_type(L, -1, "Ipe.vector"), 1,
		  "element is not a vector");
    Vector *v = check_vector(L, -1);
    lua_rawgeti(L, 1, i+1);
    luaL_argcheck(L, lua_isnumber(L, -1), 1, "element is not a number");
    int t = lua_tointegerx(L, -1, nullptr);
    luaL_argcheck(L, ShapeTool::EVertex <= t && t < ShapeTool::ENumMarkTypes,
		  1, "number is not a mark type");
    lua_pop(L, 2); // v, t
    tool->addMark(*v, ShapeTool::TMarkType(t));
  }
  return 0;
}

static int pastetool_setmatrix(lua_State *L)
{
  PasteTool *tool = (PasteTool *) lua_touserdata(L, lua_upvalueindex(1));
  Matrix *m = check_matrix(L, 1);
  tool->setMatrix(*m);
  return 0;
}

static int appui_shapetool(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  lua_pushvalue(L, 2);
  int luatool = luaL_ref(L, LUA_REGISTRYINDEX);
  ShapeTool *tool = new ShapeTool(canvas, L, luatool);
  // add methods to Lua tool
  lua_rawgeti(L, LUA_REGISTRYINDEX, luatool);
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, luatool_setcolor, 1);
  lua_setfield(L, -2, "setColor");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, shapetool_setshape, 1);
  lua_setfield(L, -2, "setShape");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, shapetool_setmarks, 1);
  lua_setfield(L, -2, "setMarks");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, shapetool_setsnapping, 1);
  lua_setfield(L, -2, "setSnapping");
  canvas->setTool(tool);
  return 0;
}

static int appui_pastetool(lua_State *L)
{
  CanvasBase *canvas = check_canvas(L, 1);
  Object *obj = check_object(L, 2)->obj;
  lua_pushvalue(L, 3);
  int luatool = luaL_ref(L, LUA_REGISTRYINDEX);
  PasteTool *tool = new PasteTool(canvas, L, luatool, obj->clone());
  // add methods to Lua tool
  lua_rawgeti(L, LUA_REGISTRYINDEX, luatool);
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, luatool_setcolor, 1);
  lua_setfield(L, -2, "setColor");
  lua_pushlightuserdata(L, tool);
  lua_pushcclosure(L, pastetool_setmatrix, 1);
  lua_setfield(L, -2, "setMatrix");
  canvas->setTool(tool);
  return 0;
}

// --------------------------------------------------------------------

static int appui_win(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  push_winid(L, (*ui)->windowId());
  return 1;
}

static int appui_close(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  (*ui)->closeWindow();
  return 0;
}

static int appui_clipboard(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  return (*ui)->clipboard(L);
}

static int appui_setClipboard(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  return (*ui)->setClipboard(L);
}

// This is only used on Windows to compute the shortcuts...
static int appui_actionInfo(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  return (*ui)->actionInfo(L);
}

static int appui_actionState(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  const char *name = luaL_checklstring(L, 2, nullptr);
  lua_pushboolean(L, (*ui)->actionState(name));
  return 1;
}

static int appui_setActionState(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  const char *name = luaL_checklstring(L, 2, nullptr);
  bool val = lua_toboolean(L, 3);
  (*ui)->setActionState(name, val);
  return 0;
}

static int appui_setupSymbolicNames(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  Cascade *sheets = check_cascade(L, 2)->cascade;
  (*ui)->setupSymbolicNames(sheets);
  return 0;
}

static int appui_setAttributes(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  Cascade *sheets = check_cascade(L, 2)->cascade;
  AllAttributes all;
  check_allattributes(L, 3, all);
  (*ui)->setAttributes(all, sheets);
  return 0;
}

static int appui_setGridAngleSize(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  Attribute gs = check_number_attribute(L, 2);
  Attribute as = check_number_attribute(L, 3);
  (*ui)->setGridAngleSize(gs, as);
  return 0;
}

static int appui_setLayers(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  Page *page = check_page(L, 2)->page;
  int view = check_viewno(L, 3, page);
  (*ui)->setLayers(page, view);
  return 0;
}

static int appui_setNumbers(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  String vno;
  String pno;
  if (!lua_isnil(L, 2))
    vno = luaL_checklstring(L, 2, nullptr);
  bool vm = lua_toboolean(L, 3);
  if (!lua_isnil(L, 4))
    pno = luaL_checklstring(L, 4, nullptr);
  bool pm = lua_toboolean(L, 5);
  (*ui)->setNumbers(vno, vm, pno, pm);
  return 0;
}

static int appui_setBookmarks(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  luaL_argcheck(L, lua_istable(L, 2), 2, "argument is not a table");
  int no = lua_rawlen(L, 2);
  std::vector<String> bm;
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, 2, i);
    luaL_argcheck(L, lua_isstring(L, -1), 2, "item is not a string");
    bm.push_back(String(lua_tolstring(L, -1, nullptr)));
    lua_pop(L, 1);
  }
  (*ui)->setBookmarks(no, bm.data());
  return 0;
}

static int appui_setWindowTitle(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  bool mod = lua_toboolean(L, 2);
  const char *s = luaL_checklstring(L, 3, nullptr);
  (*ui)->setWindowCaption(mod, s);
  return 0;
}

static int appui_setNotes(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  const char *s = luaL_checklstring(L, 2, nullptr);
  (*ui)->setNotes(s);
  return 0;
}

static int appui_setRecentFiles(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  luaL_argcheck(L, lua_istable(L, 2), 2, "argument is not a table");
  int no = lua_rawlen(L, 2);
  std::vector<String> s;
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, 2, i);
    luaL_argcheck(L, lua_isstring(L, -1), 2, "item is not a string");
    s.push_back(String(lua_tolstring(L, -1, nullptr)));
    lua_pop(L, 1);
  }
  (*ui)->setRecentFileMenu(s);
  return 0;
}

static int appui_showTool(lua_State *L)
{
  static const char * const option_names[] = {
    "properties", "bookmarks", "notes", "layers", nullptr };

  AppUiBase **ui = check_appui(L, 1);
  int m = luaL_checkoption(L, 2, nullptr, option_names);
  bool s = lua_toboolean(L, 3);
  (*ui)->setToolVisible(m, s);
  return 0;
}

static int appui_explain(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  const char *s = luaL_checklstring(L, 2, nullptr);
  int t = 4000;
  if (lua_isnumber(L, 3))
    t = lua_tointegerx(L, 3, nullptr);
  (*ui)->explain(s, t);
  return 0;
}

// --------------------------------------------------------------------

void get_page_sorter_size(lua_State *L, int &width, int &height,
			  int &thumbWidth)
{
  thumbWidth = 300;
  width = 600;
  height = 480;

  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "page_sorter_size");
  if (lua_istable(L, -1)) {
    lua_rawgeti(L, -1, 1);
    if (lua_isnumber(L, -1))
      width = lua_tointegerx(L, -1, nullptr);
    lua_rawgeti(L, -2, 2);
    if (lua_isnumber(L, -1))
      height = lua_tointegerx(L, -1, nullptr);
    lua_pop(L, 2);
  }
  lua_pop(L, 1); // page_sorter_size

  lua_getfield(L, -1, "thumbnail_width");
  if (lua_isnumber(L, -1))
    thumbWidth = lua_tointegerx(L, -1, nullptr);
  lua_pop(L, 1); // thumbnail_width, prefs
}

static int appui_selectPage(lua_State *L)
{
  check_appui(L, 1);  // not actually used now
  Document **doc = check_document(L, 2);
  int page = -1;
  if (lua_isnumber(L, 3)) {
    page = lua_tointegerx(L, 3, nullptr);
    luaL_argcheck(L, 1 <= page && page <= (*doc)->countPages(),
		  3, "invalid page number");
  }

  int startIndex = 1;
  if (lua_isnumber(L, 4)) {
    startIndex = lua_tointegerx(L, 4, nullptr);
    int maxIndex = (page < 0) ? (*doc)->countPages() :
      (*doc)->page(page-1)->countViews();
    luaL_argcheck(L, 1 <= startIndex && startIndex <= maxIndex,
		  4, "invalid start index");
  }

  int width, height, thumbWidth;
  get_page_sorter_size(L, width, height, thumbWidth);

  int sel = CanvasBase::selectPageOrView(*doc, page - 1, startIndex - 1,
					 thumbWidth, width, height);
  if (sel >= 0) {
    lua_pushinteger(L, sel + 1);
    return 1;
  } else
    return 0;
}

static int appui_pageSorter(lua_State *L)
{
  AppUiBase **ui = check_appui(L, 1);
  Document **doc = check_document(L, 2);
  int page = -1;
  if (lua_isnumber(L, 3)) {
    page = lua_tointegerx(L, 3, nullptr);
    luaL_argcheck(L, 1 <= page && page <= (*doc)->countPages(),
		  3, "invalid page number");
  }

  int width, height, thumbWidth;
  get_page_sorter_size(L, width, height, thumbWidth);

  return (*ui)->pageSorter(L, *doc, page - 1, width, height, thumbWidth);
}

static const char *const render_formats[] = {"svg", "png", "eps", "pdf", nullptr };

static int appui_renderPage(lua_State *L)
{
  // AppUiBase **ui = check_appui(L, 1);  // not used
  Document **doc = check_document(L, 2);
  int pageno = luaL_checkinteger(L, 3);
  int viewno = luaL_checkinteger(L, 4);
  Thumbnail::TargetFormat fm =
    Thumbnail::TargetFormat(luaL_checkoption(L, 5, nullptr, render_formats));
  const char *dst = luaL_checklstring(L, 6, nullptr);
  double zoom = luaL_checknumber(L, 7);
  Thumbnail tn(*doc, 0);
  tn.setTransparent(lua_toboolean(L, 8));
  tn.setNoCrop(lua_toboolean(L, 9));
  const Page *page = (*doc)->page(pageno - 1);
  tn.saveRender(fm, dst, page, viewno - 1, zoom);
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg appui_methods[] = {
  { "__tostring", appui_tostring },
  { "__gc", appui_destructor },
  // --------------------------------------------------------------------
  { "setPage", appui_setPage},
  { "pan", appui_pan},
  { "setPan", appui_setPan},
  { "zoom", appui_zoom},
  { "setZoom", appui_setZoom},
  { "setResources", appui_setResources},
  { "pos", appui_pos},
  { "globalPos", appui_globalPos},
  { "unsnappedPos", appui_unsnappedPos},
  { "simpleSnapPos", appui_simpleSnapPos },
  { "setFifiVisible", appui_setFifiVisible},
  { "setInkMode", appui_setInkMode },
  { "setSelectionVisible", appui_setSelectionVisible},
  { "setSnap", appui_setSnap},
  { "setSnapIndicator", appui_setSnapIndicator},
  { "setAutoOrigin", appui_setAutoOrigin },
  { "update", appui_update},
  { "finishTool", appui_finishTool},
  { "canvasSize", appui_canvasSize },
  { "setCursor", appui_setCursor },
  { "setNumbering", appui_setNumbering },
  { "setPretty", appui_setPretty },
  { "setScreen", appui_setScreen },
  { "type3Font", appui_type3Font },
  // --------------------------------------------------------------------
  { "panTool", appui_pantool },
  { "selectTool", appui_selecttool },
  { "transformTool", appui_transformtool },
  { "shapeTool", appui_shapetool },
  { "pasteTool", appui_pastetool },
  // --------------------------------------------------------------------
  { "win", appui_win},
  { "close", appui_close},
  { "setClipboard", appui_setClipboard },
  { "clipboard", appui_clipboard },
  { "setActionState", appui_setActionState },
  { "actionState", appui_actionState },
  { "actionInfo", appui_actionInfo },
  { "explain", appui_explain },
  { "setWindowTitle", appui_setWindowTitle },
  { "setupSymbolicNames", appui_setupSymbolicNames },
  { "setAttributes", appui_setAttributes },
  { "setGridAngleSize", appui_setGridAngleSize },
  { "setLayers", appui_setLayers },
  { "setNumbers", appui_setNumbers },
  { "setBookmarks", appui_setBookmarks },
  { "setNotes", appui_setNotes },
  { "setRecentFiles", appui_setRecentFiles },
  { "showTool", appui_showTool },
  { "selectPage", appui_selectPage },
  { "pageSorter", appui_pageSorter },
  { "renderPage", appui_renderPage },
  { nullptr, nullptr }
};

// --------------------------------------------------------------------

static int appui_constructor(lua_State *L)
{
  luaL_checktype(L, 1, LUA_TTABLE); // this is the model

  AppUiBase **ui = (AppUiBase **) lua_newuserdata(L, sizeof(AppUiBase *));
  *ui = nullptr;
  luaL_getmetatable(L, "Ipe.appui");
  lua_setmetatable(L, -2);

  lua_pushvalue(L, 1);
  int model = luaL_ref(L, LUA_REGISTRYINDEX);
  *ui = createAppUi(L, model);

  CanvasBase::Style style;
  style.pretty = false;
  style.paperColor = Color(1000,1000,1000);
  style.primarySelectionColor = Color(1000,0,0);
  style.secondarySelectionColor = Color(1000,0,1000);
  style.selectionSurroundColor = Color(1000, 1000, 0);
  style.primarySelectionWidth = 3.0;
  style.secondarySelectionWidth = 2.0;
  style.selectionSurroundWidth = 6.0;
  style.gridLineColor = Color(300, 300, 300);
  style.classicGrid = false;
  style.thinLine = 0.2;
  style.thickLine = 0.9;
  style.thinStep = 1;
  style.thickStep = 4;
  style.paperClip = false;
  style.numberPages = false;

  Color pathViewColor = Color(1000, 1000, 800);

  lua_getglobal(L, "prefs");

  lua_getfield(L, -1, "canvas_style");
  if (!lua_isnil(L, -1)) {
    lua_getfield(L, -1, "paper_color");
    if (!lua_isnil(L, -1))
      style.paperColor = check_color(L, lua_gettop(L));
    lua_pop(L, 1); // paper_color

    lua_getfield(L, -1, "primary_color");
    if (!lua_isnil(L, -1))
      style.primarySelectionColor = check_color(L, lua_gettop(L));
    lua_pop(L, 1); // primary_color

    lua_getfield(L, -1, "secondary_color");
    if (!lua_isnil(L, -1))
      style.secondarySelectionColor = check_color(L, lua_gettop(L));
    lua_pop(L, 1); // secondary_color

    lua_getfield(L, -1, "surround_color");
    if (!lua_isnil(L, -1))
      style.selectionSurroundColor = check_color(L, lua_gettop(L));
    lua_pop(L, 1); // surround_color

    lua_getfield(L, -1, "primary_width");
    if (lua_isnumber(L, -1))
      style.primarySelectionWidth = lua_tonumberx(L, -1, nullptr);
    lua_pop(L, 1); // primary_width

    lua_getfield(L, -1, "secondary_width");
    if (lua_isnumber(L, -1))
      style.secondarySelectionWidth = lua_tonumberx(L, -1, nullptr);
    lua_pop(L, 1); // secondary_width

    lua_getfield(L, -1, "surround_width");
    if (lua_isnumber(L, -1))
      style.selectionSurroundWidth = lua_tonumberx(L, -1, nullptr);
    lua_pop(L, 1); // surround_width

    lua_getfield(L, -1, "grid_line_color");
    if (!lua_isnil(L, -1))
      style.gridLineColor = check_color(L, lua_gettop(L));
    lua_pop(L, 1); // grid_line_color

    lua_getfield(L, -1, "path_view_color");
    if (!lua_isnil(L, -1))
      pathViewColor = check_color(L, lua_gettop(L));
    lua_pop(L, 1); // path_view_color

    lua_getfield(L, -1, "classic_grid");
    style.classicGrid = lua_toboolean(L, -1);
    lua_pop(L, 1); // classic_grid

    lua_getfield(L, -1, "thin_grid_line");
    if (lua_isnumber(L, -1))
      style.thinLine = lua_tonumberx(L, -1, nullptr);
    lua_pop(L, 1); // thin_grid_line

    lua_getfield(L, -1, "thick_grid_line");
    if (lua_isnumber(L, -1))
      style.thickLine = lua_tonumberx(L, -1, nullptr);
    lua_pop(L, 1); // thick_grid_line

    lua_getfield(L, -1, "thin_step");
    if (lua_isnumber(L, -1))
      style.thinStep = lua_tointegerx(L, -1, nullptr);
    lua_pop(L, 1); // thin_step

    lua_getfield(L, -1, "thick_step");
    if (lua_isnumber(L, -1))
      style.thickStep = lua_tointegerx(L, -1, nullptr);
    lua_pop(L, 1); // thick_step
  }
  lua_pop(L, 1); // canvas_style

  int width = -1, height = -1, x = -1, y = -1;
  lua_getfield(L, -1, "window_size");
  if (lua_istable(L, -1)) {
    lua_rawgeti(L, -1, 1);
    if (lua_isnumber(L, -1))
      width = lua_tointegerx(L, -1, nullptr);
    lua_rawgeti(L, -2, 2);
    if (lua_isnumber(L, -1))
      height = lua_tointegerx(L, -1, nullptr);
    lua_rawgeti(L, -3, 3);
    if (lua_isnumber(L, -1))
      x = lua_tointegerx(L, -1, nullptr);
    lua_rawgeti(L, -4, 4);
    if (lua_isnumber(L, -1))
      y = lua_tointegerx(L, -1, nullptr);
    lua_pop(L, 4);
  }
  lua_pop(L, 1); // window_size

  lua_pop(L, 1); // prefs

  (*ui)->canvas()->setCanvasStyle(style);

  (*ui)->showWindow(width, height, x, y, pathViewColor);

  return 1;
}

// --------------------------------------------------------------------

int luaopen_appui(lua_State *L)
{
  lua_pushcfunction(L, appui_constructor);
  lua_setglobal(L, "AppUi");
  make_metatable(L, "Ipe.appui", appui_methods);
  return 0;
}

// --------------------------------------------------------------------
