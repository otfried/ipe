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
#include <emscripten/val.h>

#include <format>

using namespace ipe;
using namespace ipelua;
using namespace emscripten;
using std::string;

// --------------------------------------------------------------------

static const char *submenuNames[] = {
  "submenu-gridsize",
  "submenu-anglesize",
  "submenu-textstyle",
  "submenu-labelstyle",
  "submenu-selectlayer",
  "submenu-movelayer",
  "submenu-recentfiles",
};

static val tojs(const char *s) noexcept
{
  if (s == nullptr)
    return val::null();
  return val(string(s));
}

static val tojs(Color c) noexcept
{
  val v = val::object();
  v.set("red", c.iRed.toDouble());
  v.set("green", c.iGreen.toDouble());
  v.set("blue", c.iBlue.toDouble());
  return v;
}

static void setInnerText(const char *element, const char *s)
{
  val el = val::global("document")
    .call<val>("getElementById", string(element));
  el.set("innerText", string(s));
}

static val jsUi() {
  return val::global("window")["ipeui"];
}

static bool build_menus = true;

// --------------------------------------------------------------------

AppUi::AppUi(lua_State *L0, int model)
  : AppUiBase(L0, model)
{
  val doc = val::global("document");
  val bottomCanvas = doc.call<val>("getElementById", std::string("bottomCanvas"));
  val topCanvas = doc.call<val>("getElementById", std::string("topCanvas"));
  iCanvas = new Canvas(bottomCanvas, topCanvas);
  iPathView = new PathView();
  if (build_menus)
    buildMenus();
  build_menus = false; // all Windows share the same main menu
  createIcon(String("pen"));
  createIcon(String("shift_key"));
  createIcon(String("stop"));
  jsUi().call<void>("setupMenu");
  iCanvas->setObserver(this);
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
			const char * label, const char * shortcut)
{
  jsUi().call<void>("buildMenu", tojs(what), id, tojs(name),
		    tojs(label), tojs(shortcut));
}

void AppUi::addRootMenu(int id, const char *name)
{
  requestMenu("rootmenu", id, nullptr, name, nullptr);
}

void AppUi::addItem(int id, const char *label, const char *name)
{
  if (label == nullptr) {
    requestMenu("separator", id, nullptr, nullptr, nullptr);
  } else {
    if (name[0] == '@')
      name = name + 1;
    const char * type = "normal";
    if (name[0] == '*') {
      type = "checkbox";
      name = name + 1;
    } else if ((id == EModeMenu) ||
	       (String(name).find('|') >= 0)) {
      type = "radio";
    }
    lua_getglobal(L, "shortcuts");
    lua_getfield(L, -1, name);
    createIcon(String(name));
    requestMenu(type, id, name, label,
		lua_isstring(L, -1) ? lua_tostring(L, -1) : nullptr);
    lua_pop(L, 2);
  }
}

static MENUHANDLE currentSubmenu = -1;

void AppUi::startSubMenu(int id, const char *label, int tag)
{
  currentSubmenu = tag;
  if (ESubmenuGridSize <= tag && tag < ESubmenuFin)
    requestMenu("submenu", id, submenuNames[tag - ESubmenuGridSize], label, nullptr);
  else
    requestMenu("submenu", id, "submenu", label, nullptr);
}

void AppUi::addSubItem(const char *label, const char *name)
{
  addItem(-1, label, name);
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
  val colors = val::array();
  for (size_t i = 0; i < sym.size(); ++i) {
    val color = val::object();
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
  jsUi().call<void>("addCombo", sel, string(s.z()));
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
  jsUi().call<void>("paintPathView");
}

void AppUi::setCheckMark(String name, Attribute a)
{
  String s = name + "|" + a.string();
  jsUi().call<void>("setCheckMark", s.s());
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
  jsUi().call<void>("setNumbers", vno.s(), vm, pno.s(), pm);
}

void AppUi::setNotes(String notes)
{
  val el = val::global("document")
    .call<val>("getElementById", string("notes"));
  el.set("value", notes.z());
}

void AppUi::setLayers(const Page *page, int view)
{
  std::vector<int> objCounts;
  page->objectsPerLayer(objCounts);
  val layers = val::array();
  val items = val::array();
  for (int i = 0; i < page->countLayers(); ++i) {
    val item = val::object();
    item.set("name", page->layer(i).s());
    item.set("text", std::format("{} ({})", page->layer(i).z(), objCounts[i]));
    item.set("checked", page->visible(view, i));
    item.set("active", page->layer(i) == page->active(view));
    item.set("locked", page->isLocked(i));
    switch (page->snapping(i)) {
    case Page::SnapMode::Never:
      item.set("snap", "never");
      break;
    case Page::SnapMode::Always:
      item.set("snap", "always");
      break;
    default:
      item.set("snap", "normal");
      break;
    }
    layers.call<void>("push", item);
    items.call<void>("push", page->layer(i).s());
  }
  jsUi().call<void>("setLayers", layers);
  jsUi().call<void>("setSubmenu", tojs("submenu-selectlayer"),
		    tojs("selectinlayer-"), tojs("normal"), items);
  jsUi().call<void>("setSubmenu", tojs("submenu-movelayer"),
		    tojs("movetolayer-"), tojs("normal"), items);
}

void AppUi::setBookmarks(int no, const String *s)
{
  val bookmarks = val::array();
  for (int i = 0; i < no; ++i) 
    bookmarks.call<void>("push", s[i].s());
  jsUi().call<void>("setBookmarks", bookmarks);
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
  auto setSubmenu = [](int sm, val items) {
    const char * id = submenuNames[sm - ESubmenuGridSize];
    std::string action = std::string(id + 8) + "|";
    jsUi().call<void>("setSubmenu", tojs(id), action, tojs("radio"), items);
  };
  AppUiBase::setupSymbolicNames(sheet);
  val submenu1 = val::array();
  for (String s : iComboContents[EUiGridSize] )
    submenu1.call<void>("push", s.s());
  setSubmenu(ESubmenuGridSize, submenu1);
  val submenu2 = val::array();
  for (String s : iComboContents[EUiAngleSize] )
    submenu2.call<void>("push", s.s());
  setSubmenu(ESubmenuAngleSize, submenu2);
  AttributeSeq seq;
  sheet->allNames(ETextStyle, seq);
  val submenu3 = val::array();
  for (auto &attr : seq)
    submenu3.call<void>("push", attr.string().s());
  setSubmenu(ESubmenuTextStyle, submenu3);
  seq.clear();
  sheet->allNames(ELabelStyle, seq);
  val submenu4 = val::array();
  for (auto &attr : seq)
    submenu4.call<void>("push", attr.string().s());
  setSubmenu(ESubmenuLabelStyle, submenu4);
}

// --------------------------------------------------------------------

void AppUi::setActionsEnabled(bool mode)
{
  // TODO: disable actions during drawing
}

// --------------------------------------------------------------------

// Determine if action is checked
// Used for snapXXX, grid_visible, viewmarked, and pagemarked
bool AppUi::actionState(const char *name)
{
  val as = jsUi()["actionState"];
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

WINID AppUi::windowId()
{
  return this;
}

void AppUi::closeWindow()
{
  // handled by JS
}

void AppUi::setWindowCaption(bool mod, const char *caption, const char *fn)
{
  jsUi().call<void>("setTitle", mod, std::string(caption), std::string(fn));
}

void AppUi::setMouseIndicator(const char *s)
{
  setInnerText("mouse", s);
}

void AppUi::setSnapIndicator(const char *s)
{
  setInnerText("snapIndicator", s);
}

static void blankStatus(void * _arg) {
  setInnerText("status", "");
}

void AppUi::explain(const char *s, int t)
{
  setInnerText("status", s);
  if (t) {
    emscripten_async_call(blankStatus, nullptr, t);
  }
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
  string data = string(luaL_checklstring(L, 2, nullptr));
  jsUi().call<void>("setClipboard", data);
  return 0;
}

int AppUi::clipboard(lua_State *L)
{
  bool allowBitmap = lua_toboolean(L, 2);
  val result =
    jsUi().call<val>("getClipboard", allowBitmap);
  // this operation is async, it will later resume Lua with the result
  return 0;
}

bool AppUi::waitDialog(const char *cmd, const char *label)
{
  // cmd is either: "runlatex:<tex engine>" or "editor:"
  jsUi().call<void>("waitDialog", string(cmd), string(label));
  // this operation is async, it will later resume Lua with the result
  return false;
}

void AppUi::resumeLua(val result)
{
  // calls model:resumeLua with an argument
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "resumeLua");
  lua_insert(L, -2); // before model
  if (result.isNull())
    lua_pushnil(L);
  else if (result.isString())
    lua_pushstring(L, result.as<string>().c_str());
  else if (result.isNumber())
    lua_pushnumber(L, result.as<double>());
  else
    lua_pushlightuserdata(L, &result);
  luacall(L, 2, 0);
}

void AppUi::openFile(String fn)
{
  push_string(L, fn);
  wrapCall("loadDocument", 1);
}

// --------------------------------------------------------------------

int appui_preloadFile(lua_State *L)
{
  string fname{luaL_checklstring(L, 1, nullptr)};
  string tmpname{luaL_checklstring(L, 2, nullptr)};
  jsUi().call<void>("preloadFile", fname, tmpname);
  return 0;
}

int appui_persistFile(lua_State *L)
{
  string fname{luaL_checklstring(L, 1, nullptr)};
  jsUi().call<void>("persistFile", fname);
  return 0;
}

// --------------------------------------------------------------------

int AppUi::pageSorter(lua_State *L, Document *doc, int pno,
		      int width, int height, int thumbWidth)
{
  // TODO pageSorter
  explain("Not yet implemented", 0);
  return 0;
}

// --------------------------------------------------------------------

int CanvasBase::selectPageOrView(Document *doc, int page, int startIndex,
				 int pageWidth, int width, int height)
{
  // TODO selectPage
  return -1;
}

// ------------------------------------------------------------------------
