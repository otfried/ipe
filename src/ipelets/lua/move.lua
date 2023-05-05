----------------------------------------------------------------------
-- move ipelet
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

label = "Move"

revertOriginal = _G.revertOriginal

about = [[
Moving objects in small steps, best used with shortcuts.

Original Ipe 6 version by Gunter Schenck, this ipelet is now part of Ipe.
]]

methods = {
  { label = "right 1pt", dx=1 },
  { label = "left 1pt", dx=-1 },
  { label = "up 1pt", dy=1 },
  { label = "down 1pt", dy=-1 },
  { label = "right-up 1pt", dx=1, dy=1 },
  { label = "left-up 1pt", dx=-1, dy=1},
  { label = "right-down 1pt", dx=1, dy=-1 },
  { label = "left-down 1pt", dx=-1, dy=-1 },
  { label = "right 0.1pt", dx=0.1 },
  { label = "left 0.1pt", dx=-0.1 },
  { label = "up 0.1pt", dy=0.1 },
  { label = "down 0.1pt", dy=-0.1 },
  { label = "right-up 0.1pt", dx=0.1, dy=0.1 },
  { label = "left-up 0.1pt", dx=-0.1, dy=0.1 },
  { label = "right-down 0.1pt", dx=0.1, dy=-0.1 },
  { label = "left-down 0.1pt", dx=-0.1, dy=-0.1 },
  { label = "right 10pt", dx=10 },
  { label = "left 10pt", dx=-10 },
  { label = "up 10pt", dy=10 },
  { label = "down 10pt", dy=-10 },
  { label = "right-up 10pt", dx=10, dy=10 },
  { label = "left-up 10pt", dx=-10, dy=10 },
  { label = "right-down 10pt", dx=10, dy=-10 },
  { label = "left-down 10pt", dx=-10, dy=-10 },
}

shortcuts.ipelet_1_move = "Ctrl+6"
shortcuts.ipelet_2_move = "Ctrl+4"
shortcuts.ipelet_3_move = "Ctrl+8"
shortcuts.ipelet_4_move = "Ctrl+2"
shortcuts.ipelet_5_move = "Ctrl+9"
shortcuts.ipelet_6_move = "Ctrl+7"
shortcuts.ipelet_7_move = "Ctrl+3"
shortcuts.ipelet_8_move = "Ctrl+1"
shortcuts.ipelet_9_move = "Alt+6"
shortcuts.ipelet_10_move = "Alt+4"
shortcuts.ipelet_11_move = "Alt+8"
shortcuts.ipelet_12_move = "Alt+2"
shortcuts.ipelet_13_move = "Alt+9"
shortcuts.ipelet_14_move = "Alt+7"
shortcuts.ipelet_15_move = "Alt+3"
shortcuts.ipelet_16_move = "Alt+1"
shortcuts.ipelet_17_move = "Ctrl+Alt+6"
shortcuts.ipelet_18_move = "Ctrl+Alt+4"
shortcuts.ipelet_19_move = "Ctrl+Alt+8"
shortcuts.ipelet_20_move = "Ctrl+Alt+2"
shortcuts.ipelet_21_move = "Ctrl+Alt+9"
shortcuts.ipelet_22_move = "Ctrl+Alt+7"
shortcuts.ipelet_23_move = "Ctrl+Alt+3"
shortcuts.ipelet_24_move = "Ctrl+Alt+1"

-- TODO: check if objects are pinned
function run(model, num)
  local p = model:page()
  if not p:hasSelection() then
    model.ui:explain("no selection")
    return
  end
  local dx = methods[num].dx
  if not dx then dx = 0 end
  local dy = methods[num].dy
  if not dy then dy = 0 end

  local pin = {}
  for i, obj, sel, lay in p:objects() do
    if sel then pin[obj:get("pinned")] = true end
  end

  if pin.fixed or pin.horizontal and dx ~= 0 or pin.vertical and dy ~= 0 then
    model:warning("Cannot move objects",
		  "Some object is pinned and cannot be moved")
    return
  end

  local t = { label = "move by (" .. dx .. ", " .. dy .. ")",
	      pno = model.pno,
	      vno = model.vno,
	      selection = model:selection(),
	      original = model:page():clone(),
	      matrix = ipe.Matrix(1, 0, 0, 1, dx, dy),
	      undo = revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do p:transform(i, t.matrix) end
	   end
  model:register(t)
end

----------------------------------------------------------------------
