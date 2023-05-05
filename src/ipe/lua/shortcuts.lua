----------------------------------------------------------------------
-- Ipe keyboard shortcuts
----------------------------------------------------------------------
--[[

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

--]]

-- on OSX, Ctrl means the Command key (for platform compatibility)
-- if you really want the Control key, write "Control"
-- you can also write "Command" for the Command key for clarity.

-- if you want to use the cursor keys, they are "Left", "Right", "Up", "Down".

shortcuts = {
  new_window = nil,
  open = "Ctrl+O",
  close = "Ctrl+W",
  save = "Ctrl+S",
  save_as = nil,
  export_png = nil,
  export_eps = nil,
  export_svg = nil,
  insert_image = "Ctrl+Shift+O",
  run_latex = "Ctrl+L",
  absolute = "Ctrl+T",
  undo = "Ctrl+Z",
  redo = "Ctrl+Y",
  copy = "Ctrl+C",
  paste = nil,
  paste_with_layer = "Ctrl+Alt+V",
  paste_at_cursor = "Ctrl+V",
  cut = "Ctrl+X",
  delete = "delete",
  group = "Ctrl+G",
  ungroup = "Ctrl+U",
  front = "Ctrl+F",
  back = "Ctrl+B",
  forward = "Ctrl+Shift+F",
  backward = "Ctrl+Shift+B",
  before = nil,
  behind = nil,
  duplicate = "D",
  select_all = "Ctrl+A",
  deselect_all = "Alt+Ctrl+A",
  join_paths = "Ctrl+J",
  pick_properties = "Q",
  apply_properties = "Ctrl+Q",
  insert_text_box = "F10",
  edit = "Ctrl+E",
  edit_as_xml = nil,
  change_width = "Alt+W",
  document_properties = "Ctrl+Shift+P",
  style_sheets = "Ctrl+Shift+S",
  update_style_sheets = "Ctrl+Shift+U",
  mode_select = "S",
  mode_translate = "T",
  mode_rotate = "R",
  mode_stretch = "E",
  mode_pan = nil,
  mode_shredder = nil,
  mode_laser = "Shift+Q",
  mode_label = "L",
  mode_math = "Shift+4",
  mode_paragraph = "G",
  mode_marks = "M",
  mode_rectangles1 = "B",
  mode_rectangles2 = nil,
  mode_rectangles3 = nil,
  mode_parallelogram = nil,
  mode_lines = "P",
  mode_polygons = "Shift+P",
  mode_arc1 = "A",
  mode_arc2 = "Shift+A",
  mode_arc3 = "Alt+A",
  mode_circle1 = "O",
  mode_circle2 = "Shift+O",
  mode_circle3 = "Alt+O",
  mode_splines = "I",
  mode_splinegons = "Shift+I",
  mode_ink = "K",
  snapvtx = "F4",
  snapctl = "Shift+F4",
  snapbd = "F5",
  snapint = "F6",
  snapgrid = "F7",
  snapcustom = "Shift+F7",
  snapangle = "F8",
  snapauto = "F9",
  set_origin = "F1",
  set_origin_snap = "Ctrl+Shift+F1",
  show_axes = "Ctrl+F1",
  set_direction = "F2",
  reset_direction = "Ctrl+F2",
  set_tangent_direction = "Ctrl+F3",
  set_line = "F3",
  set_line_snap = "Shift+F3",
  fullscreen = "F11",
  normal_size = "/",
  grid_visible = "F12",
  pretty_display = "Ctrl+F12",
  zoom_in = "Ctrl+PgUp",
  zoom_out = "Ctrl+PgDown",
  fit_page = "\\",
  fit_width = "-",
  fit_top = nil,
  fit_objects = "=",
  fit_selection = "@",
  pan_here = "X",
  new_layer = "Ctrl+Shift+N",
  rename_active_layer = "Ctrl+Shift+R",
  select_in_active_layer = "Ctrl+Shift+A",
  move_to_active_layer = "Ctrl+Shift+M",
  next_view = "PgDown",
  previous_view = "PgUp",
  first_view = "Home",
  last_view = "End",
  new_layer_view = "Ctrl+Shift+I",
  new_view = nil,
  delete_view = nil,
  edit_view = "V",
  next_page = "Shift+PgDown",
  previous_page = "Shift+PgUp",
  first_page = "Shift+Home",
  last_page = "Shift+End",
  new_page = "Ctrl+I",
  cut_page = "Ctrl+Shift+X",
  copy_page = "Ctrl+Shift+C",
  paste_page = "Ctrl+Shift+V",
  delete_page = nil,
  edit_title = "Ctrl+P",
  edit_notes = "Ctrl+N",
  page_sorter = nil,
  jump_view = "Shift+J",
  jump_page = "J",
  ipelet_1_goodies = nil, -- Mirror horizontal
  ipelet_2_goodies = nil, -- Mirror vertical
  ipelet_3_goodies = nil, -- Mirror at x-axis
  ipelet_4_goodies = nil, -- Mirror at y-axis
  ipelet_5_goodies = nil, -- Turn 90 degrees
  ipelet_6_goodies = nil, -- Turn 180 degrees
  ipelet_7_goodies = nil, -- Turn 270 degrees
  ipelet_8_goodies = "Ctrl+R", -- Precise rotate
  ipelet_9_goodies = "Ctrl+K", -- Precise stretch
  ipelet_10_goodies = nil, -- Rotate coordinate system
  ipelet_11_goodies = nil, -- Insert precise box
  ipelet_12_goodies = nil, -- Insert bounding box
  ipelet_13_goodies = nil, -- Insert media box
  ipelet_14_goodies = nil, -- Mark circle center
  ipelet_15_goodies = nil, -- Make parabolas
  ipelet_16_goodies = nil, -- Regular k-gon
  ipelet_2_symbols = "Alt+Y", -- Use current symbol
}

if config.platform == "apple" then
  shortcuts.delete = "backspace"
  shortcuts.fullscreen = "Control+Command+F"
end

-- These are the shortcuts for the lines/polylines, polygons and splines
-- functions.
-- Contrary to the above, global shortcuts, the following rules apply:
-- + Use small letters ("a") for normal keys
-- + Use capital letters ("A") for Shift+A
-- + Any special keys (F1, Delete, ...) are not possible
-- + Any combination with the Ctrl key is not possible
-- + If you use a shortcut that is already in the global shortcut list,
--   it won't work!

shortcuts_linestool = {
  spline = "s",
  arc = "a",
  set_axis = "y",
}

----------------------------------------------------------------------
