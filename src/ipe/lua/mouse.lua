----------------------------------------------------------------------
-- Ipe mouse buttons
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

-- Available actions are:
-- "select"
-- "translate", "rotate", "stretch",
-- "scale" (like "stretch" with Shift)
-- "pan"
-- "menu" (opens context menu)
-- "shredder" (erases object closest to mouse)
-- or write a Lua function with arguments (model, modifiers)

-- left and left_shift cannot be remapped

-- More than two modifiers are possible in the order
-- shift_control_alt_meta
-- e.g. left_shift_meta = "rotate"

-- If your mouse has additional buttons, then you can also use
-- button8, button9, and button10.
-- (It seems other buttons are not passed through by Qt.)

mouse = {
  left_alt = "translate",
  left_control = "select",
  left_command = "select",
  left_shift_control = "select",
  left_shift_command = "select",
  middle = "pan",
  right = "menu",
  right_alt = "rotate",
  right_control = "stretch",
  right_command = "stretch",
  right_shift_control = "scale",
  right_shift_command = "scale",
  right_shift = "pan",
  button8 = "shredder",
  button10 = "translate",
}

----------------------------------------------------------------------
