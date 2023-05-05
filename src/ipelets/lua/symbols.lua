----------------------------------------------------------------------
-- symbols ipelet
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

label = "Symbols"

revertOriginal = _G.revertOriginal

about = [[
Functions for creating, using, and editing symbols.

This ipelet is part of Ipe.
]]

V = ipe.Vector
current_symbol = nil
current_group = nil
current_element = nil

----------------------------------------------------------------------

PASTETOOL = {}
PASTETOOL.__index = PASTETOOL

function PASTETOOL:new(model, name)
  local tool = {}
  _G.setmetatable(tool, PASTETOOL)
  tool.model = model
  tool.pos = model.ui:pos()
  tool.name = name
  tool.size = 1
  if name:match("%(s?f?p?x%)$") then
    tool.size = model.doc:sheets():find("symbolsize",
					model.attributes.symbolsize)
  end
  local obj = model.doc:sheets():find("symbol", name)
  model.ui:pasteTool(obj, tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function PASTETOOL:mouseButton(button, modifiers, press)
  self.model.ui:finishTool()
  local obj = ipe.Reference(self.model.attributes, self.name, self.pos)
  self.model:creation("create symbol", obj)
end

function PASTETOOL:mouseMove(button, modifiers)
  self.pos = self.model.ui:pos()
  self.setMatrix(ipe.Matrix(self.size, 0, 0, self.size, self.pos.x, self.pos.y))
  self.model.ui:update(false) -- update tool
end

function PASTETOOL:key(code, modifiers, text)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

function select_symbol(model)
  local s = model.doc:sheets():allNames("symbol")
  local symbolsByGroup = {}
  for _, sym in ipairs(s) do
    local i = sym:find("/")
    local g, n
    if i then
      g = sym:sub(1, i-1)
      n = sym:sub(i+1)
    else
      g = ""
      n = sym
    end
    if not symbolsByGroup[g] then
      symbolsByGroup[g] = {}
    end
    local sbg = symbolsByGroup[g]
    sbg[#sbg+1] = n
  end
  local groups = {}
  for g,sbg in pairs(symbolsByGroup) do
    groups[#groups+1] = g
  end
  table.sort(groups)
  local d = ipeui.Dialog(model.ui:win(), "Use symbol")
  groups.action = function (d)
    		    local g = d:get("group")
		    if g ~= current_group then
		       current_group = g
		       local syms = symbolsByGroup[groups[g]]
		       d:set("select", syms)
		    end
		  end
  d:add("label", "label", { label = "Select symbol" }, 1, 1, 1, 3)
  d:add("group", "combo", groups, 2, 1, 1, 3)
  if current_group == nil then current_group = 1 end
  d:set("group", current_group)
  d:add("select", "combo", symbolsByGroup[groups[current_group]], 3, 1, 1, 3)
  if current_element then
    d:set("select", current_element)
  end
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("column", 1, 1)
  if not d:execute() then return end
  current_element = d:get("select")
  local g = groups[current_group]
  local sym = symbolsByGroup[g][current_element]
  if g ~= "" then g = g .. "/" end
  return g .. sym
end

function use_symbol(model, num)
  local name = select_symbol(model)
  if not name then return end
  if num == 1 then
    PASTETOOL:new(model, name)
  else -- clone symbol
    local obj = model.doc:sheets():find("symbol", name)
    model:creation("clone symbol", obj)
  end
end

function use_current_symbol(model, num)
  if not current_symbol then
    model.ui:explain("current symbol has not been set")
  else
    PASTETOOL:new(model, current_symbol)
  end
end

function select_current_symbol(model, num)
  local name = select_symbol(model)
  if not name then return end
  current_symbol = name
end

----------------------------------------------------------------------

function create_symbol(model, num)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local str = model:getString("Enter name of new symbol")
  if not str or str:match("^%s*$") then return end
  local name = str:match("^%s*%S+%s*$")
  local old = model.doc:sheets():find("symbol", name)
  if old then
    local r = ipeui.messageBox(model.ui:win(), "question",
			       "Symbol '" .. name .. "' already exists",
			       "Do you want to proceed?",
			       "okcancel")
    if r <= 0 then return end
  end

  if num == 3 then -- new stylesheet
    local sheet = ipe.Sheet()
    sheet:add("symbol", name, p[prim])
    local t = { label = methods[num].label,
		sheet = sheet,
	      }
    t.redo = function (t, doc)
	       doc:sheets():insert(1, t.sheet:clone())
	     end
    t.undo = function (t, doc)
	       doc:sheets():remove(1)
	     end
    model:register(t)
  else  -- top stylesheet
    local sheet = model.doc:sheets():sheet(1)
    local t = { label = methods[num].label,
		original = sheet:clone(),
		final = sheet:clone(),
	      }
    t.final:add("symbol", name, p[prim])
    t.redo = function (t, doc)
	       doc:sheets():remove(1)
	       doc:sheets():insert(1, t.final:clone())
	     end
    t.undo = function (t, doc)
	       doc:sheets():remove(1)
	       doc:sheets():insert(1, t.original:clone())
	     end
    model:register(t)
  end
end

----------------------------------------------------------------------

methods = {
  { label = "use symbol", run = use_symbol },
  { label = "use current symbol", run = use_current_symbol },
  { label = "create symbol (in new style sheet)", run = create_symbol },
  { label = "create symbol (in top style sheet)", run = create_symbol },
  { label = "clone symbol", run = use_symbol },
  { label = "select current symbol", run = select_current_symbol }
}

----------------------------------------------------------------------
