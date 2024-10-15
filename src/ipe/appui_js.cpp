// --------------------------------------------------------------------
// AppUi for QT
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

#include "appui_js.h"
#include "controls_js.h"
#include "ipecanvas_js.h"

#include "ipelua.h"

#include "ipethumbs.h"

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static emscripten::val tojs(const char *s) noexcept
{
  if (s == nullptr)
    return emscripten::val::null();
  return emscripten::val(std::string(s));
}

static emscripten::val tojs(Color c) noexcept
{
  emscripten::val v = emscripten::val::object();
  v.set("red", c.iRed.toDouble());
  v.set("green", c.iGreen.toDouble());
  v.set("blue", c.iBlue.toDouble());
  return v;
}

static void setInnerText(const char *element, const char *s)
{
  emscripten::val el = emscripten::val::global("document")
    .call<emscripten::val>("getElementById", std::string(element));
  el.set("innerText", std::string(s));
}

static emscripten::val jsUi() {
  return emscripten::val::global("window")["ipeui"];
}

static bool build_menus = true;

// --------------------------------------------------------------------

AppUi::AppUi(lua_State *L0, int model, Canvas *canvas)
  : AppUiBase(L0, model)
{
  iPathView = new PathView();
  iCanvas = canvas;
  if (build_menus)
    buildMenus();
  build_menus = false; // all Windows share the same main menu
  createIcon(String("pen"));
  createIcon(String("stop"));
  jsUi().call<void>("setupMenu");
}

AppUi::~AppUi()
{
  ipeDebug("AppUi destructor");
  delete iPathView;
}

void AppUi::createIcon(String name)
{
  String svgdir = Platform::latexDirectory() + "/icons/";
  String svgname = svgdir + name + ".svg";
  int pno = ipeIcon(name);
  if (pno >= 0) {
    bool dark = false;
    Document *doc = dark ? ipeIconsDark.get() : ipeIcons.get();
    Thumbnail thumbs(doc, 22);
    thumbs.setNoCrop(true);
    thumbs.saveRender(Thumbnail::ESVG, svgname.z(), doc->page(pno), 0, 1.0);
  }
}

// --------------------------------------------------------------------

static void requestMenu(const char *what, int id, const char *name,
			const char * title, int tag, const char * shortcut)
{
  jsUi().call<void>("buildMenu", tojs(what), id, tojs(name),
		    tojs(title), tag, tojs(shortcut));
}

void AppUi::addRootMenu(int id, const char *name)
{
  requestMenu("addRootMenu", id, name, nullptr, 0, nullptr);
}

void AppUi::addItem(int id, const char *title, const char *name)
{
  if (title == nullptr) {
    requestMenu("addSeparator", id, nullptr, nullptr, 0, nullptr);
  } else {
    if (name[0] == '@')
      name = name + 1;
    int tag = 0;
    if (name[0] == '*') {
      tag = 1;
      name = name + 1;
    } else if ((id == EModeMenu) ||
	       (String(name).find('|') >= 0)) {
      tag = 2; // radio button
    }
    lua_getglobal(L, "shortcuts");
    lua_getfield(L, -1, name);
    createIcon(String(name));
    requestMenu("addItem", id, name, title, tag,
		lua_isstring(L, -1) ? lua_tostring(L, -1) : nullptr);
    lua_pop(L, 2);
  }
}

static MENUHANDLE currentSubmenu = -1;

void AppUi::startSubMenu(int id, const char *name, int tag)
{
  currentSubmenu = tag;
  requestMenu("startSubMenu", id, name, nullptr, tag, nullptr);
}

void AppUi::addSubItem(const char *title, const char *name)
{
  addItem(-1, title, name);
}

MENUHANDLE AppUi::endSubMenu()
{
  return currentSubmenu;
}

// ------------------------------------------------------------------------

void AppUi::setRecentFileMenu(const std::vector<String> & names)
{
}

// --------------------------------------------------------------------

void AppUi::resetCombos()
{
  jsUi().call<void>("resetCombos");
}

void AppUi::addComboColors(AttributeSeq &sym, AttributeSeq &abs)
{
  iComboContents[EUiStroke].push_back(IPEABSOLUTE);
  iComboContents[EUiFill].push_back(IPEABSOLUTE);
  emscripten::val colors = emscripten::val::array();
  for (size_t i = 0; i < sym.size(); ++i) {
    emscripten::val color = emscripten::val::object();
    String s = sym[i].string();
    color.set("name", s.s());
    color.set("rgb", tojs(abs[i].color()));
    colors.call<void>("push", color);
    iComboContents[EUiStroke].push_back(s);
    iComboContents[EUiFill].push_back(s);
  }
  jsUi().call<void>("addComboColors", colors);
}

void AppUi::addCombo(int sel, String s)
{
  jsUi().call<void>("addCombo", sel, std::string(s.z()));
}

void AppUi::setComboCurrent(int sel, int idx)
{
  jsUi().call<void>("setComboCurrent", sel, idx);
}

void AppUi::setButtonColor(int sel, Color color)
{
  jsUi().call<void>("setButtonColor", sel, tojs(color));
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
  iPathView->set(all, sheet);
}

void AppUi::setCheckMark(String name, Attribute a)
{
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
  jsUi().call<void>("setNumbers", vno.s(), vm, pno.s(), pm);
}

void AppUi::setNotes(String notes)
{
  emscripten::val el = emscripten::val::global("document")
    .call<emscripten::val>("getElementById", std::string("notes"));
  el.set("value", notes.z());
}

void AppUi::setLayers(const Page *page, int view)
{
}

void AppUi::setBookmarks(int no, const String *s)
{
}

void AppUi::setToolVisible(int tool, bool vis)
{
  jsUi().call<void>("setToolVisible", tool, vis);
}

void AppUi::setZoom(double zoom)
{
  char s[16];
  sprintf(s, "(%dppi)", int(72.0 * zoom));
  setInnerText("resolution", s);
  iCanvas->setZoom(zoom);
}

void AppUi::setupSymbolicNames(const Cascade *sheet)
{
  AppUiBase::setupSymbolicNames(sheet);
  emscripten::val submenu1 = emscripten::val::array();
  for (String s : iComboContents[EUiGridSize] )
    submenu1.call<void>("push", s.s());
  jsUi().call<void>("setSubmenu", int(ESubmenuGridSize), submenu1);
  emscripten::val submenu2 = emscripten::val::array();
  for (String s : iComboContents[EUiAngleSize] )
    submenu2.call<void>("push", s.s());
  jsUi().call<void>("setSubmenu", int(ESubmenuAngleSize), submenu2);
  AttributeSeq seq;
  sheet->allNames(ETextStyle, seq);
  emscripten::val submenu3 = emscripten::val::array();
  for (auto &attr : seq)
    submenu3.call<void>("push", attr.string().s());
  jsUi().call<void>("setSubmenu", int(ESubmenuTextStyle), submenu3);
  seq.clear();
  sheet->allNames(ELabelStyle, seq);
  emscripten::val submenu4 = emscripten::val::array();
  for (auto &attr : seq)
    submenu4.call<void>("push", attr.string().s());
  jsUi().call<void>("setSubmenu", int(ESubmenuLabelStyle), submenu4);
}

// --------------------------------------------------------------------

void AppUi::setActionsEnabled(bool mode)
{
}

// --------------------------------------------------------------------

// Determine if action is checked
// Used for snapXXX, grid_visible, viewmarked, and pagemarked
bool AppUi::actionState(const char *name)
{
  emscripten::val as = jsUi()["actionState"];
  return as[name].as<bool>();
}

// Check/uncheck an action
// Used for snapXXX, grid_visible, to initialize mode_select
void AppUi::setActionState(const char *name, bool value)
{
  jsUi().call<void>("setActionState", tojs(name), value);
}

// --------------------------------------------------------------------

void AppUi::action(String name)
{
  luaAction(name);
}

// --------------------------------------------------------------------

int AppUi::pageSorter(lua_State *L, Document *doc, int pno,
		      int width, int height, int thumbWidth)
{
  return 0;
}

// --------------------------------------------------------------------

WINID AppUi::windowId()
{
  return this;
}

void AppUi::closeWindow()
{
  // handled by JS
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  // TODO: macOS can use the actual filename and whether the file has been modified
  // (documentEdited and representedFilename on BrowserWindow)
  emscripten::val htmlDocument = emscripten::val::global("document");
  htmlDocument.set("title", s);
}

void AppUi::setMouseIndicator(const char *s)
{
  setInnerText("mouse", s);
}

void AppUi::setSnapIndicator(const char *s)
{
  setInnerText("snapIndicator", s);
}

void AppUi::explain(const char *s, int t)
{
  // TODO: handle timeout t
  setInnerText("status", s);
}

void AppUi::showWindow(int width, int height, int x, int y, const Color & pathViewColor)
{
  iPathView->setColor(pathViewColor);
}

void AppUi::setFullScreen(int mode)
{
  // handled by JS
}

int AppUi::setClipboard(lua_State *L)
{
  std::string data = std::string(luaL_checklstring(L, 2, nullptr));
  jsUi().call<void>("setClipboard", data);
  return 0;
}

int AppUi::clipboard(lua_State *L)
{
  bool allowBitmap = lua_toboolean(L, 2);
  emscripten::val result =
    jsUi().call<emscripten::val>("getClipboard", allowBitmap);
  // this operation is async, it will later resume Lua with the result
  return 0;
}

bool AppUi::waitDialog(const char *cmd, const char *label)
{
  // cmd is either: "runlatex:<tex engine>" or "editor:"
  jsUi().call<void>("waitDialog", std::string(cmd), std::string(label));
  // this operation is async, it will later resume Lua with the result
  return false;
}

void AppUi::resumeLua(emscripten::val result)
{
  // calls model:resumeLua with an argument
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "resumeLua");
  lua_insert(L, -2); // before model
  if (result.isNull())
    lua_pushnil(L);
  else if (result.isString())
    lua_pushstring(L, result.as<std::string>().c_str());
  else if (result.isNumber())
    lua_pushnumber(L, result.as<double>());
  else
    lua_pushlightuserdata(L, &result);
  luacall(L, 2, 0);
}

// --------------------------------------------------------------------

int appui_preloadFile(lua_State *L)
{
  std::string fname{luaL_checklstring(L, 1, nullptr)};
  std::string tmpname{luaL_checklstring(L, 2, nullptr)};
  jsUi().call<void>("preloadFile", fname, tmpname);
  return 0;
}

int appui_persistFile(lua_State *L)
{
  std::string fname{luaL_checklstring(L, 1, nullptr)};
  jsUi().call<void>("persistFile", fname);
  return 0;
}

// --------------------------------------------------------------------

int CanvasBase::selectPageOrView(Document *doc, int page, int startIndex,
				 int pageWidth, int width, int height)
{
  // TODO
  return -1;
}

// ------------------------------------------------------------------------
