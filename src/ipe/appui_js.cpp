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
#include "ipecanvas_js.h"

#include "ipelua.h"

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static bool build_menus = true;

AppUi::AppUi(lua_State *L0, int model, Canvas *canvas)
  : AppUiBase(L0, model)
{
  iCanvas = canvas;
  buildMenus();
  build_menus = false; // all Windows share the same main menu
}

AppUi::~AppUi()
{
  ipeDebug("AppUi C++ destructor");
}

// --------------------------------------------------------------------

static emscripten::val tojs(const char *s) noexcept {
  if (s == nullptr)
    return emscripten::val::null();
  return emscripten::val(std::string(s));
}

static void requestMenu(const char *what, int id, const char *name,
			const char * title, int tag, const char * shortcut)
{
  emscripten::val window = emscripten::val::global("window");
  window.call<void>("buildMenu", tojs(what), id, tojs(name),
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
  // for (int i = 0; i < EUiView; ++i)
  // iSelector[i]->clear();
}

void AppUi::addComboColors(AttributeSeq &sym, AttributeSeq &abs)
{
}

void AppUi::addCombo(int sel, String s)
{
}

void AppUi::setComboCurrent(int sel, int idx)
{
}

void AppUi::setButtonColor(int sel, Color color)
{
}

void AppUi::setPathView(const AllAttributes &all, Cascade *sheet)
{
}

void AppUi::setCheckMark(String name, Attribute a)
{
}

void AppUi::setNumbers(String vno, bool vm, String pno, bool pm)
{
}

void AppUi::setNotes(String notes)
{
}

void AppUi::setLayers(const Page *page, int view)
{
}

void AppUi::setBookmarks(int no, const String *s)
{
}

void AppUi::setToolVisible(int m, bool vis)
{
}

void AppUi::setZoom(double zoom)
{
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
  return false;
}

// Check/uncheck an action
// Used for snapXXX, grid_visible, to initialize mode_select
void AppUi::setActionState(const char *name, bool value)
{
}

// --------------------------------------------------------------------

void AppUi::action(String name)
{
  // if (name.left(5) == "mode_")
  // iModeIndicator->setPixmap(prefsPixmap(name));
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
}

void AppUi::setWindowCaption(bool mod, const char *s)
{
  // setWindowModified(mod);
  // setWindowTitle(QString::fromUtf8(s));
}

void AppUi::setMouseIndicator(const char *s)
{
  // iMouse->setText(s);
}

void AppUi::setSnapIndicator(const char *s)
{
  // iSnapIndicator->setText(s);
}

void AppUi::explain(const char *s, int t)
{
  // statusBar()->showMessage(QString::fromUtf8(s), t);
}

void AppUi::showWindow(int width, int height, int x, int y, const Color & pathViewColor)
{
  /*
  iPathView->setColor(pathViewColor);
  if (width > 0 && height > 0)
    resize(width, height);
  if (x >= 0 && y >= 0)
    move(x, y);
  show();
  */
}

void AppUi::setFullScreen(int mode)
{
}

int AppUi::setClipboard(lua_State *L)
{
  return 0;
}

int AppUi::clipboard(lua_State *L)
{
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

