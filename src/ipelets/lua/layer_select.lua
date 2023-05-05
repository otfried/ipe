----------------------------------------------------------------------
-- select or deselect all layers in current view
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

label = "Select or deselect all layers in current view"

revertOriginal = _G.revertOriginal

about = [[
Select or deselect all layers in the current view
]]

function make_visible(model, num)
  local vis = (num == 1)
  local label
  if vis then
    label = "make all layers visible in view " .. model.vno
  else
    label = "make all layers invisible in view " .. model.vno
  end
  local t = { label=label,
	      pno=model.pno,
	      vno=model.vno,
	      original=model:page():clone(),
	      undo=revertOriginal
	    }
  t.redo = function (t, doc)
     for i, l in ipairs(doc[t.pno]:layers()) do
	doc[t.pno]:setVisible(t.vno, l, vis)
     end
  end
  model:register(t)
end

function run(model)
   local p = model:page()
   p:deselectAll()
end

methods = {
  { label = "Select all layers", run=make_visible },
  { label = "Deselect all layers", run=make_visible },
}

-- shortcuts.ipelet_1_layer_select = "Alt+Ctrl+A"
-- shortcuts.ipelet_2_layer_select = "Alt+Ctrl+B"
