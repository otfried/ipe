// --------------------------------------------------------------------
// AppUi
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
#include "tools.h"

#include "ipelua.h"

#include "ipecairopainter.h"

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------------------

const char * const AppUiBase::selectorNames[] =
  { "stroke", "fill", "pen", "dashstyle", "textsize", "markshape", "symbolsize",
    "opacity", "gridsize", "anglesize", "view", "page", "viewmarked", "pagemarked" };

AppUiBase::AppUiBase(lua_State *L0, int model)
{
  L = L0;
  iModel = model;
  isInkMode = false;

  iMouseIn = 0; // points
  iMouseFactor = 1.0;  // scale 1:1
  iCoordinatesFormat = "%g%s, %g%s";
  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "coordinates_format");
  if (lua_isstring(L, -1)) {
    iCoordinatesFormat = lua_tolstring(L, -1, nullptr);
  }
  lua_pop(L, 1); // coordinates_format

  lua_getfield(L, -1, "width_notes_bookmarks");
  if (lua_isnumber(L, -1)) {
    iWidthNotesBookmarks = lua_tointegerx(L, -1, nullptr);
  }
  lua_pop(L, 1); // width_notes_bookmarks

  iUiScale = 100;
  lua_getfield(L, -1, "ui_scaling");
  if (lua_isnumber(L, -1))
    iUiScale = lua_tointegerx(L, -1, nullptr);
  lua_pop(L, 1); // ui_scaling

  iToolbarScale = 100;
  lua_getfield(L, -1, "toolbar_scaling");
  if (lua_isnumber(L, -1))
    iToolbarScale = lua_tointegerx(L, -1, nullptr);
  lua_pop(L, 1); // toolbar_scaling

  // win_ui_gap separates the input elements vertically, so the
  // can be touched with a finger
  iUiGap = 0;
  lua_getfield(L, -1, "win_ui_gap");
  if (lua_isnumber(L, -1))
    iUiGap = lua_tointegerx(L, -1, nullptr);
  lua_pop(L, 1); // win_ui_gap

  // win_mini_edit leaves only the most important buttons on the Edit
  // toolbar
  lua_getfield(L, -1, "win_mini_edit");
  isMiniEdit = lua_toboolean(L, -1);
  lua_pop(L, 1); // win_mini_edit

  // win_left_panels_float makes the panels on the left float on top
  // of the canvas
  lua_getfield(L, -1, "win_left_panels_float");
  iLeftDockFloats = lua_toboolean(L, -1);
  lua_pop(L, 1); // win_left_panels_float

  iScalings.push_back(1);
  lua_getfield(L, -1, "scale_factors");
  if (lua_istable(L, -1)) {
    int n = lua_rawlen(L, -1);
    for (int i = 1; i <= n; ++i) {
      lua_rawgeti(L, -1, i);
      if (lua_isnumber(L, -1))
	iScalings.push_back(lua_tointegerx(L, -1, nullptr));
      lua_pop(L, 1); // number
    }
  }
  lua_pop(L, 2); // prefs, scale_factors
}

AppUiBase::~AppUiBase()
{
  ipeDebug("AppUiBase C++ destructor");
  luaL_unref(L, LUA_REGISTRYINDEX, iModel);
  // collect this model
  lua_gc(L, LUA_GCCOLLECT, 0);
}

int AppUiBase::dpi() const
{
  return 96;
}

// --------------------------------------------------------------------

void AppUiBase::buildMenus()
{
  addRootMenu(EFileMenu, "&File");
  addRootMenu(EEditMenu, "&Edit");
  addRootMenu(EPropertiesMenu, "P&roperties");
  addRootMenu(ESnapMenu, "&Snap");
  addRootMenu(EModeMenu, "&Mode");
  addRootMenu(EZoomMenu, "&Zoom");
  addRootMenu(ELayerMenu, "&Layers");
  addRootMenu(EViewMenu, "&Views");
  addRootMenu(EPageMenu, "&Pages");
  addRootMenu(EIpeletMenu, "&Ipelets");
  addRootMenu(EHelpMenu, "&Help");

  addItem(EFileMenu, "New Window", "new_window");
  addItem(EFileMenu, "New", "new");
  addItem(EFileMenu, "Open", "open");
  addItem(EFileMenu, "Save", "save");
  addItem(EFileMenu, "Save as", "save_as");
  addItem(EFileMenu);
  startSubMenu(EFileMenu, "Recent files", ESubmenuRecentFiles);
  iRecentFileMenu = endSubMenu();
  addItem(EFileMenu);
  addItem(EFileMenu, "Export as PNG", "export_png");
  addItem(EFileMenu, "Export as EPS", "export_eps");
  addItem(EFileMenu, "Export as SVG", "export_svg");
  addItem(EFileMenu);
  addItem(EFileMenu, "Insert image", "insert_image");
  addItem(EFileMenu);
  addItem(EFileMenu, "Automatically run Latex", "*auto_latex");
  addItem(EFileMenu, "Run Latex", "run_latex");
  addItem(EFileMenu);
  addItem(EFileMenu, "Close", "close");

  addItem(EEditMenu, "Undo", "undo");
  addItem(EEditMenu, "Redo", "redo");
  addItem(EEditMenu);
  addItem(EEditMenu, "Cut", "cut");
  addItem(EEditMenu, "Copy", "copy");
  addItem(EEditMenu, "Paste", "paste");
  addItem(EEditMenu, "Paste with layer", "paste_with_layer");
  addItem(EEditMenu, "Paste at cursor", "paste_at_cursor");
  addItem(EEditMenu, "Delete", "delete");
  addItem(EEditMenu);
  addItem(EEditMenu, "Group", "group");
  addItem(EEditMenu, "Ungroup", "ungroup");
  addItem(EEditMenu, "Front", "front");
  addItem(EEditMenu, "Back", "back");
  addItem(EEditMenu, "Forward", "forward");
  addItem(EEditMenu, "Backward", "backward");
  addItem(EEditMenu, "Just before", "before");
  addItem(EEditMenu, "Just behind", "behind");
  addItem(EEditMenu, "Duplicate", "duplicate");
  addItem(EEditMenu, "Select all", "select_all");
  addItem(EEditMenu, "Deselect all", "deselect_all");
  addItem(EEditMenu);
  addItem(EEditMenu, "Pick properties", "pick_properties");
  addItem(EEditMenu, "Apply properties", "apply_properties");
  addItem(EEditMenu);
  addItem(EEditMenu, "Insert text box", "insert_text_box");
  addItem(EEditMenu, "Change text width", "change_width");
  addItem(EEditMenu, "Edit object", "edit");
  addItem(EEditMenu, "Edit object as XML", "edit_as_xml");
  addItem(EEditMenu);
  addItem(EEditMenu, "Edit group", "edit_group");
  addItem(EEditMenu, "End group edit", "end_group_edit");
  addItem(EEditMenu);
  addItem(EEditMenu, "Document properties", "document_properties");
  addItem(EEditMenu, "Style sheets", "style_sheets");
  addItem(EEditMenu, "Update style sheets", "update_style_sheets");
  addItem(EEditMenu, "Check symbolic attributes", "check_style");

  startSubMenu(EPropertiesMenu, "Pinned");
  addSubItem("none", "pinned|none");
  addSubItem("horizontal", "pinned|horizontal");
  addSubItem("vertical", "pinned|vertical");
  addSubItem("fixed", "pinned|fixed");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Transformations");
  addSubItem("translations", "transformations|translations");
  addSubItem("rigid motions", "transformations|rigid");
  addSubItem("affine", "transformations|affine");
  endSubMenu();

  addItem(EPropertiesMenu);

  startSubMenu(EPropertiesMenu, "Minipage style", ESubmenuTextStyle);
  iTextStyleMenu = endSubMenu();
  startSubMenu(EPropertiesMenu, "Label style", ESubmenuLabelStyle);
  iLabelStyleMenu = endSubMenu();

  startSubMenu(EPropertiesMenu, "Horizontal alignment");
  addSubItem("left", "horizontalalignment|left");
  addSubItem("center", "horizontalalignment|hcenter");
  addSubItem("right", "horizontalalignment|right");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Vertical alignment");
  addSubItem("bottom", "verticalalignment|bottom");
  addSubItem("baseline", "verticalalignment|baseline");
  addSubItem("center", "verticalalignment|vcenter");
  addSubItem("top", "verticalalignment|top");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Transformable text");
  addSubItem("Yes", "transformabletext|true");
  addSubItem("No", "transformabletext|false");
  endSubMenu();

  startSubMenu(EPropertiesMenu, "Spline type");
  addSubItem("bspline", "splinetype|bspline");
  addSubItem("cardinal", "splinetype|cardinal");
  addSubItem("spiro", "splinetype|spiro");
  endSubMenu();

  addItem(EModeMenu, "Select objects (with Shift: non-destructive)",
	  "mode_select");
  addItem(EModeMenu, "Translate objects (with Shift: horizontal/vertical)",
	  "mode_translate");
  addItem(EModeMenu, "Rotate objects", "mode_rotate");
  addItem(EModeMenu, "Stretch objects (with Shift: scale objects)",
	  "mode_stretch");
  addItem(EModeMenu, "Shear objects", "mode_shear");
  addItem(EModeMenu, "Move graph nodes", "mode_graph");
  addItem(EModeMenu, "Pan the canvas", "mode_pan");
  addItem(EModeMenu, "Shred objects", "mode_shredder");
  addItem(EModeMenu, "Laser pointer", "mode_laser");
#ifndef IPEUI_JS  // breaks radio group
  addItem(EModeMenu);
#endif
  addItem(EModeMenu, "Text labels", "mode_label");
  addItem(EModeMenu, "Mathematical symbols", "mode_math");
  addItem(EModeMenu, "Paragraphs", "mode_paragraph");
  addItem(EModeMenu, "Marks", "mode_marks");
  addItem(EModeMenu, "Axis-parallel rectangles (with Shift: squares)",
	  "mode_rectangles1");
  addItem(EModeMenu, "Axis-parallel rectangles, by center (with Shift: squares)",
	  "mode_rectangles2");
  addItem(EModeMenu, "Rectangles (with Shift: squares)", "mode_rectangles3");
  addItem(EModeMenu, "Parallelograms (with Shift: axis-parallel)",
	  "mode_parallelogram");
  addItem(EModeMenu, "Lines and polylines", "mode_lines");
  addItem(EModeMenu, "Polygons", "mode_polygons");
  addItem(EModeMenu, "Splines", "mode_splines");
  addItem(EModeMenu, "Splinegons", "mode_splinegons");
  addItem(EModeMenu, "Circular arcs (by center, right and left point)",
    "mode_arc1");
  addItem(EModeMenu, "Circular arcs (by center, left and right point)",
    "mode_arc2");
  addItem(EModeMenu, "Circular arcs (by 3 points)", "mode_arc3");
  addItem(EModeMenu, "Circles (by center and radius)", "mode_circle1");
  addItem(EModeMenu, "Circles (by diameter)", "mode_circle2");
  addItem(EModeMenu, "Circles (by 3 points)", "mode_circle3");
  addItem(EModeMenu, "Ink", "mode_ink");

  // @ means the action can be used while drawing
  // * means the action is checkable (on/off)
  // Checkable actions work differently in Qt and Win32/Cocoa:
  //   Qt already toggles the state
  //   In Win32/Cocoa the action needs to toggle the state.

  addItem(ESnapMenu, "Snap to vertex", "@*snapvtx");
  addItem(ESnapMenu, "Snap to control point", "@*snapctl");
  addItem(ESnapMenu, "Snap to boundary", "@*snapbd");
  addItem(ESnapMenu, "Snap to intersection", "@*snapint");
  addItem(ESnapMenu, "Snap to grid", "@*snapgrid");
  addItem(ESnapMenu, "Snap to custom grid", "@*snapcustom");
  addItem(ESnapMenu, "Angular snap", "@*snapangle");
  addItem(ESnapMenu, "Automatic snap", "@*snapauto");
  addItem(ESnapMenu);
  startSubMenu(ESnapMenu, "Grid size", ESubmenuGridSize);
  iGridSizeMenu = endSubMenu();
  startSubMenu(ESnapMenu, "Radial angle", ESubmenuAngleSize);
  iAngleSizeMenu = endSubMenu();
  addItem(ESnapMenu);
  addItem(ESnapMenu, "Set origin", "@set_origin");
  addItem(ESnapMenu, "Set origin && snap", "@set_origin_snap");
  addItem(ESnapMenu, "Show axes", "@*show_axes");
  addItem(ESnapMenu, "Set direction", "@set_direction");
  addItem(ESnapMenu, "Set tangent direction", "@set_tangent_direction");
  addItem(ESnapMenu, "Reset direction", "@reset_direction");
  addItem(ESnapMenu, "Set line", "@set_line");
  addItem(ESnapMenu, "Set line && snap", "@set_line_snap");

  addItem(EZoomMenu, "Fullscreen", "@*fullscreen");
  addItem(EZoomMenu, "Grid visible", "@*grid_visible");
  addItem(EZoomMenu, "Pretty display", "@*pretty_display");

  startSubMenu(EZoomMenu, "Coordinates");
  addSubItem("points", "@coordinates|points");
  addSubItem("mm", "@coordinates|mm");
  addSubItem("m", "@coordinates|m");
  addSubItem("inch", "@coordinates|inch");
  endSubMenu();

  startSubMenu(EZoomMenu, "Coordinate scale");
  for (int s : iScalings) {
    char display[32];
    char action[32];
    if (s < 0)
      sprintf(display, "%d:1", -s);
    else
      sprintf(display, "1:%d", s);
    sprintf(action, "@scaling|%d", s);
    addSubItem(display, action);
  }
  endSubMenu();

  addItem(EZoomMenu);
  addItem(EZoomMenu, "Zoom in", "@zoom_in");
  addItem(EZoomMenu, "Zoom out", "@zoom_out");
  addItem(EZoomMenu, "Normal size", "@normal_size");
  addItem(EZoomMenu, "Fit page", "@fit_page");
  addItem(EZoomMenu, "Fit width", "@fit_width");
  addItem(EZoomMenu, "Fit page top", "@fit_top");
  addItem(EZoomMenu, "Fit objects", "@fit_objects");
  addItem(EZoomMenu, "Fit selection", "@fit_selection");
  addItem(EZoomMenu);
  addItem(EZoomMenu, "Pan here", "@pan_here");

  addItem(ELayerMenu, "New layer", "new_layer");
  addItem(ELayerMenu, "Rename active layer", "rename_active_layer");
  addItem(ELayerMenu);
  addItem(ELayerMenu, "Select all in active  layer",
    "select_in_active_layer");
  startSubMenu(ELayerMenu, "Select all in layer", ESubmenuSelectLayer);
  iSelectLayerMenu = endSubMenu();
  addItem(ELayerMenu, "Move to active layer", "move_to_active_layer");
  startSubMenu(ELayerMenu, "Move to layer", ESubmenuMoveLayer);
  iMoveToLayerMenu = endSubMenu();

  addItem(EViewMenu, "Next view", "next_view");
  addItem(EViewMenu, "Previous view", "previous_view");
  addItem(EViewMenu, "First view", "first_view");
  addItem(EViewMenu, "Last view", "last_view");
  addItem(EViewMenu);
  addItem(EViewMenu, "New layer, new view", "new_layer_view");
  addItem(EViewMenu, "New view", "new_view");
  addItem(EViewMenu, "Delete view", "delete_view");
  addItem(EViewMenu);
  addItem(EViewMenu, "Mark views from this view", "mark_from_view");
  addItem(EViewMenu, "Unmark views from this view", "unmark_from_view");
  addItem(EViewMenu);
  addItem(EViewMenu, "Jump to view", "jump_view");
  addItem(EViewMenu, "Edit view", "edit_view");
  addItem(EViewMenu, "View sorter", "view_sorter");

  addItem(EPageMenu, "Next page", "next_page");
  addItem(EPageMenu, "Previous page", "previous_page");
  addItem(EPageMenu, "First page", "first_page");
  addItem(EPageMenu, "Last page", "last_page");
  addItem(EPageMenu);
  addItem(EPageMenu, "New page", "new_page");
  addItem(EPageMenu, "Cut page", "cut_page");
  addItem(EPageMenu, "Copy page", "copy_page");
  addItem(EPageMenu, "Paste page", "paste_page");
  addItem(EPageMenu, "Delete page", "delete_page");
  addItem(EPageMenu);
  addItem(EPageMenu, "Jump to page", "jump_page");
  addItem(EPageMenu, "Edit title && sections", "edit_title");
  addItem(EPageMenu, "Edit notes", "edit_notes");
  addItem(EPageMenu, "Page sorter", "page_sorter");
  addItem(EPageMenu);
#ifndef IPEUI_QT
  // In Qt these are created using "toggleViewAction()"
  addItem(EPageMenu, "Notes", "@*toggle_notes");
  addItem(EPageMenu, "Bookmarks", "@*toggle_bookmarks");
#endif

  addItem(EHelpMenu, "Ipe &manual", "manual");
  addItem(EHelpMenu, "Preferences", "preferences");
  addItem(EHelpMenu, "Onscreen keyboard", "@keyboard");
  addItem(EHelpMenu, "Show &configuration", "show_configuration");
  addItem(EHelpMenu, "Show &libraries", "show_libraries");
  addItem(EHelpMenu, "&Ipelet information", "about_ipelets");
  addItem(EHelpMenu, "Enable online Latex-compilation", "cloud_latex");

  lua_getglobal(L, "prefs");
  lua_getfield(L, -1, "developer");
  if (lua_toboolean(L, -1)) {
    startSubMenu(EHelpMenu, "Developer");
    addSubItem("Reload ipelets", "developer_reload_ipelets");
    addSubItem("List shortcuts", "developer_list_shortcuts");
    endSubMenu();
  }
  lua_pop(L, 2); // developer, prefs

#ifndef IPEUI_COCOA
  addItem(EHelpMenu, "&About Ipe", "about");
#endif

  // build Ipelet menu
  lua_getglobal(L, "ipelets");
  int n = lua_rawlen(L, -1);
  for (int i = 1; i <= n; ++i) {
    lua_rawgeti(L, -1, i);
    lua_getfield(L, -1, "label");
    if (!lua_isstring(L, -1)) {
      lua_pop(L, 2); // label, ipelet
      continue;
    }
    String label(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);
    lua_getfield(L, -1, "name");
    String name(lua_tolstring(L, -1, nullptr));
    lua_pop(L, 1);
    lua_getfield(L, -1, "methods");
    char buf[20];
    if (lua_isnil(L, -1)) {
      String action("ipelet_1_");
      action += name;
      addItem(EIpeletMenu, label.z(), action.z());
    } else {
      int m = lua_rawlen(L, -1);
      startSubMenu(EIpeletMenu, label.z());
      for (int j = 1; j <= m; ++j) {
	lua_rawgeti(L, -1, j);
	lua_getfield(L, -1, "label");
	sprintf(buf, "ipelet_%d_", j);
	String action(buf);
	action += name;
	addSubItem(lua_tolstring(L, -1, nullptr), action.z());
	lua_pop(L, 2); // sublabel, method
      }
      endSubMenu();
    }
    lua_pop(L, 2); // methods, ipelet
  }
  lua_pop(L, 1);
}

// --------------------------------------------------------------------

void AppUiBase::canvasObserverWheelMoved(double xDegrees, double yDegrees,
					 int kind)
{
  if (xDegrees != 0.0 || yDegrees != 0.0) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
    lua_getfield(L, -1, "wheel_zoom");
    lua_insert(L, -2); // model
    lua_pushnumber(L, xDegrees);
    lua_pushnumber(L, yDegrees);
    lua_pushinteger(L, kind);
    luacall(L, 4, 0);
  } else
    // result of a zoom gesture (Windows only, currently)
    setZoom(iCanvas->zoom());
}

void AppUiBase::canvasObserverToolChanged(bool hasTool)
{
  setActionsEnabled(!hasTool || isInkMode);
}

static void adjust(double &x, int mode, double factor)
{
  if (ipe::abs(x) < 1e-12)
    x = 0.0;
  x *= factor;
  switch (mode) {
  case 1: // mm
    x = (x / 72.0) * 25.4;
    break;
  case 2: // m
    x = (x / 72000.0) * 25.4;
    break;
  case 3: // in
    x /= 72;
    break;
  default:
    break;
  }
}

static const char * const mouse_units[] = { "", " mm", " m", " in" };

void AppUiBase::canvasObserverPositionChanged()
{
  Vector v = iCanvas->CanvasBase::pos();
  const Snap &snap = iCanvas->snap();
  if (snap.iWithAxes) {
    v = v - snap.iOrigin;
    v = Linear(-snap.iDir) * v;
  }
  adjust(v.x, iMouseIn, iMouseFactor);
  adjust(v.y, iMouseIn, iMouseFactor);
  const char *units = mouse_units[iMouseIn];
  char s[256];
  sprintf(s, iCoordinatesFormat.z(), v.x, units, v.y, units);
  setMouseIndicator(s);
}

void AppUiBase::canvasObserverMouseAction(int button)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "mouseButtonAction");
  lua_insert(L, -2); // model
  push_button(L, button);
  luacall(L, 3, 0);
}

void AppUiBase::canvasObserverSizeChanged()
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "sizeChanged");
  lua_insert(L, -2); // model
  luacall(L, 1, 0);
}

// --------------------------------------------------------------------

int AppUiBase::actionInfo(lua_State *L) const
{
  return 0; // only Windows will override this
}

static void call_selector(lua_State *L, int model, String name)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, model);
  lua_getfield(L, -1, "selector");
  lua_pushvalue(L, -2); // model
  lua_remove(L, -3);
  push_string(L, name);
}

void AppUiBase::luaSelector(String name, String value)
{
  call_selector(L, iModel, name);
  if (value == "true")
    lua_pushboolean(L, true);
  else if (value == "false")
    lua_pushboolean(L, false);
  else
    push_string(L, value);
  luacall(L, 3, 0);
}

void AppUiBase::luaAbsoluteButton(const char *s)
{
  // calls model selector
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "absoluteButton");
  lua_insert(L, -2); // method, model
  lua_pushstring(L, s);
  luacall(L, 2, 0);
}

// --------------------------------------------------------------------

void AppUiBase::luaAction(String name)
{
  if (isInkMode && iCanvas->tool() != nullptr)
    return; // refuse any action while drawing ink
  if (name.left(12) == "coordinates|") {
    if (name.right(2) == "mm")
      iMouseIn = 1;
    else if (name.right(1) == "m")
      iMouseIn = 2;
    else if (name.right(4) == "inch")
      iMouseIn = 3;
    else
      iMouseIn = 0;
  } else if (name.left(8) == "scaling|") {
    Lex lex(name.substr(8));
    int s = lex.getInt();
    if (s < 0)
      iMouseFactor = 1.0 / -s;
    else
      iMouseFactor = s;
  } else if (name.find('|') >= 0) {
    // calls model selector
    int i = name.find('|');
    luaSelector(name.left(i), name.substr(i+1));
  } else {
    // calls model action
    lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
    lua_getfield(L, -1, "action");
    lua_insert(L, -2); // before model
    push_string(L, name);
    luacall(L, 2, 0);
  }
}

void AppUiBase::luaShowPathStylePopup(Vector v)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "showPathStylePopup");
  lua_insert(L, -2); // before model
  push_vector(L, v);
  luacall(L, 2, 0);
}

void AppUiBase::luaShowLayerBoxPopup(Vector v, String layer)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "showLayerBoxPopup");
  lua_insert(L, -2); // before model
  push_vector(L, v);
  push_string(L, layer);
  luacall(L, 3, 0);
}

void AppUiBase::luaLayerAction(String name, String layer)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "layerAction");
  lua_insert(L, -2); // before model
  push_string(L, name);
  push_string(L, layer);
  luacall(L, 3, 0);
}

void AppUiBase::luaBookmarkSelected(int index)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "bookmark");
  lua_insert(L, -2); // method, model
  lua_pushnumber(L, index + 1);
  luacall(L, 2, 0);
}

void AppUiBase::luaRecentFileSelected(String name)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, iModel);
  lua_getfield(L, -1, "recent_file");
  lua_insert(L, -2); // method, model
  push_string(L, name);
  luacall(L, 2, 0);
}

// --------------------------------------------------------------------

static String stripMark(Attribute mark)
{
  String s = mark.string();
  if (s.left(5) == "mark/") {
    int i = s.rfind('(');
    return s.substr(5, i > 0 ? i-5 : -1);
  } else
    return String();
}

void AppUiBase::showInCombo(const Cascade *sheet, Kind kind,
			    int sel, const char *deflt)
{
  AttributeSeq seq;
  sheet->allNames(kind, seq);
  if (!seq.size() && deflt != nullptr) {
    addCombo(sel, deflt);
    iComboContents[sel].push_back(deflt);
  }
  if (kind != EGridSize && kind != EAngleSize && kind != EDashStyle
      && kind != EOpacity) {
    addCombo(sel, IPEABSOLUTE);
    iComboContents[sel].push_back(IPEABSOLUTE);
  }
  for (const auto & att : seq) {
    String s = att.string();
    addCombo(sel, s);
    iComboContents[sel].push_back(s);
  }
}

void AppUiBase::showMarksInCombo(const Cascade *sheet)
{
  AttributeSeq seq;
  sheet->allNames(ESymbol, seq);
  for (const auto & att : seq) {
    String s = stripMark(att);
    if (!s.empty()) {
      addCombo(EUiMarkShape, s);
      iComboContents[EUiMarkShape].push_back(s);
    }
  }
}

void AppUiBase::setupSymbolicNames(const Cascade *sheet)
{
  resetCombos();
  for (int i = 0; i < EUiView; ++i)
    iComboContents[i].clear();
  AttributeSeq seq, absColor;
  sheet->allNames(EColor, seq);
  for (const auto & att : seq)
    absColor.push_back(sheet->find(EColor, att));
  addComboColors(seq, absColor);
  showInCombo(sheet, EPen, EUiPen);
  showInCombo(sheet, ETextSize, EUiTextSize);
  showInCombo(sheet, ESymbolSize, EUiSymbolSize);
  showInCombo(sheet, EDashStyle, EUiDashStyle);
  showInCombo(sheet, EOpacity, EUiOpacity);
  showMarksInCombo(sheet);
  showInCombo(sheet, EGridSize, EUiGridSize, "16pt");
  showInCombo(sheet, EAngleSize, EUiAngleSize, "45 deg");
}

void AppUiBase::setGridAngleSize(Attribute abs_grid, Attribute abs_angle)
{
  AttributeSeq seq;
  iCascade->allNames(EGridSize, seq);
  if (!seq.size())
    setComboCurrent(EUiGridSize, 0);
  for (int i = 0; i < size(seq); ++i) {
    if (iCascade->find(EGridSize, seq[i]) == abs_grid) {
      setComboCurrent(EUiGridSize, i);
      break;
    }
  }
  seq.clear();
  iCascade->allNames(EAngleSize, seq);
  if (!seq.size())
    setComboCurrent(EUiAngleSize, 0);
  for (int i = 0; i < size(seq); ++i) {
    if (iCascade->find(EAngleSize, seq[i]) == abs_angle) {
      setComboCurrent(EUiAngleSize, i);
      break;
    }
  }
}

// --------------------------------------------------------------------

void AppUiBase::setAttribute(int sel, Attribute a)
{
  String s = a.isSymbolic() ? a.string() : IPEABSOLUTE;
  for (int i = 0; i < int(iComboContents[sel].size()); ++i) {
    if (iComboContents[sel][i] == s) {
      setComboCurrent(sel, i);
      return;
    }
  }
}

void AppUiBase::setAttributes(const AllAttributes &all, Cascade *sheet)
{
  iAll = all;
  iCascade = sheet;

  setPathView(all, sheet);

  setAttribute(EUiStroke, iAll.iStroke);
  setAttribute(EUiFill, iAll.iFill);
  Color stroke = iCascade->find(EColor, iAll.iStroke).color();
  Color fill = iCascade->find(EColor, iAll.iFill).color();
  setButtonColor(EUiStroke, stroke);
  setButtonColor(EUiFill, fill);
  setAttribute(EUiPen, iAll.iPen);
  setAttribute(EUiTextSize, iAll.iTextSize);
  setAttribute(EUiSymbolSize, iAll.iSymbolSize);
  setAttribute(EUiDashStyle, iAll.iDashStyle);
  setAttribute(EUiOpacity, iAll.iOpacity);

  String s = stripMark(iAll.iMarkShape);
  for (int i = 0; i < int(iComboContents[EUiMarkShape].size()); ++i) {
    if (iComboContents[EUiMarkShape][i] == s) {
      setComboCurrent(EUiMarkShape, i);
      break;
    }
  }

  setCheckMark("horizontalalignment", Attribute(iAll.iHorizontalAlignment));
  setCheckMark("verticalalignment", Attribute(iAll.iVerticalAlignment));
  setCheckMark("splinetype", Attribute(iAll.iSplineType));
  setCheckMark("pinned", Attribute(iAll.iPinned));
  setCheckMark("transformabletext",
	       Attribute::Boolean(iAll.iTransformableText));
  setCheckMark("transformations", Attribute(iAll.iTransformations));
  setCheckMark("linejoin", Attribute(iAll.iLineJoin));
  setCheckMark("linecap", Attribute(iAll.iLineCap));
  setCheckMark("fillrule", Attribute(iAll.iFillRule));
}

int AppUiBase::ipeIcon(String action)
{
  if (!ipeIcons) {
    String fname = ipeIconDirectory() + "icons.ipe";
    if (!Platform::fileExists(fname))
      return -1;
    ipeIconsDark.reset(Document::loadWithErrorReport(fname.z()));
    ipeIcons.reset(new Document(*ipeIconsDark));
    ipeIcons->cascade()->remove(0);
  }
  return ipeIcons->findPage(action);
}

int AppUiBase::readImage(lua_State *L, String fn)
{
  // check if it is perhaps a JPEG file
  FILE *f = Platform::fopen(fn.z(), "rb");
  bool jpeg = false;
  if (f != nullptr) {
    jpeg = (std::fgetc(f) == 0xff && std::fgetc(f) == 0xd8);
    fclose(f);
  }

  ipeDebug("Dropping file %s (jpeg: %d)", fn.z(), jpeg);

  Vector dpi;
  const char *errmsg;
  Bitmap bm = jpeg ? Bitmap::readJpeg(fn.z(), dpi, errmsg) : Bitmap::readPNG(fn.z(), dpi, errmsg);

  if (bm.isNull())
    return 0;

  ipe::Rect r(Vector::ZERO, Vector(bm.width(), bm.height()));
  Image *img = new Image(r, bm);
  push_object(L, img);
  return 1;
}

// --------------------------------------------------------------------

