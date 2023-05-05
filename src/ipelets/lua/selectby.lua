----------------------------------------------------------------------
-- selectby ipelet
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

label = "Select by type/attribute"

about = [[
Select objects based on their type or attribute value.

This ipelet is part of Ipe.
]]

type = _G.type

function colorEqual(x, y)
  -- color components are stored as 3-digit precision fixed numbers
  return math.abs(x - y) < 0.0005
end

function equalProperty(a, b)
  if a == b then return true end
  if type(a) == "table" and type(b) == "table" then
    return colorEqual(a.r, b.r) and colorEqual(a.g, b.g) and
      colorEqual(a.b, b.b)
  end
  return false
end

function run(model)
  local s = "Please indicate the attributes on which to base object selection.\n" ..
    "Ipe will select all objects on the current page where the indicated attributes match the current setting in the UI."
  local d = ipeui.Dialog(model.ui:win(), "Select by type/attribute")
  d:add("label1", "label", {label=s}, 1, 1, 2, 2)
  local objtypes = { "All types", "path", "text", "reference", "image", "group" }
  d:add("objtype", "combo", objtypes, 3, 1)
  d:add("stroke", "checkbox", {label="Stroke color"}, 3, 2)
  d:add("pinned", "checkbox", {label="Pinning"}, 4, 1)
  d:add("opacity", "checkbox", {label="Opacity"}, 4, 2)

  d:add("label2", "label", {label=""}, 5, 1, 2, 1)

  d:add("pathlabel", "label", {label="Path objects"}, 6, 1)
  d:add("fill", "checkbox", {label="Fill color"}, 7, 1)
  d:add("pen", "checkbox", {label="Line width"}, 8, 1)
  d:add("dashstyle", "checkbox", {label="Dash style"}, 9, 1)
  d:add("pathmode", "checkbox", {label="Stroke && fill style"}, 10, 1)
  d:add("gradient", "checkbox", {label="Gradient pattern"}, 11, 1)
  d:add("tiling", "checkbox", {label="Tiling pattern"}, 12, 1)

  d:add("textlabel", "label", {label="Text objects"}, 6, 2)
  d:add("textsize", "checkbox", {label="Text size"}, 7, 2)
  d:add("textstyle", "checkbox", {label="Minipage style"}, 8, 2)
  d:add("labelstyle", "checkbox", {label="Label style"}, 9, 2)
  d:add("horizontalalignment", "checkbox", {label="Horizontal alignment"}, 10, 2)
  d:add("verticalalignment", "checkbox", {label="Vertical alignment"}, 11, 2)

  d:add("label3", "label", {label=""}, 13, 1, 2, 1)

  d:add("markshape", "checkbox", {label="Mark type"}, 14, 1)
  d:add("symbolsize", "checkbox", {label="Symbol size"}, 14, 2)
  d:add("transformations", "checkbox", {label="Allowed transformations"}, 15, 1)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  if not d:execute() then return end

  local keys = { "stroke", "pinned", "opacity", "fill", "pen", "dashstyle", "pathmode",
		 "gradient", "tiling", "textsize", "textstyle", "labelstyle", "horizontalalignment",
		 "verticalalignment", "markshape", "symbolsize", "transformations" }
  local needed = { }
  local needType = nil
  if d:get("objtype") ~= 1 then
    needType = objtypes[d:get("objtype")]
  end
  for _, k in ipairs(keys) do
    needed[k] = d:get(k)
  end
  -- print("---")
  -- _G.printTable(needed)
  -- perform selection
  local p = model:page()
  local a = model.attributes
  -- print("---")
  -- _G.printTable(a)
  -- print("---")
  for i,obj,sel,layer in p:objects(p) do
    local soundsGood = 2
    if needType and obj:type() ~= needType then
      soundsGood = nil
    end
    for _,k in ipairs(keys) do
      if needed[k] and not equalProperty(obj:get(k), a[k]) then
	soundsGood = nil
      end
    end
    p:setSelect(i, soundsGood)
  end
  p:ensurePrimarySelection()
end

----------------------------------------------------------------------
