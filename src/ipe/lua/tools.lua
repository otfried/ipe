----------------------------------------------------------------------
-- tools.lua
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

function externalEditor(d, field)
  local text = d:get(field)
  local fname = os.tmpname()
  if prefs.editable_textfile then
    fname = prefs.editable_textfile
  end
  local f = io.open(fname, "w")
  f:write(text)
  f:close()
  ipeui.waitDialog(d, string.format(prefs.external_editor, fname))
  f = io.open(fname, "r")
  text = f:read("*all")
  f:close()
  os.remove(fname)
  d:set(field, text)
  if prefs.editor_closes_dialog and not prefs.auto_external_editor then
    d:accept(true)
  end
end

function addEditorField(d, field)
  if prefs.external_editor then
    d:addButton("editor", "&Editor", function (d) externalEditor(d, field) end)
  end
end

----------------------------------------------------------------------

local function circleshape(center, radius)
  return { type="ellipse";
	   ipe.Matrix(radius, 0, 0, radius, center.x, center.y) }
end

local function segmentshape(v1, v2)
  return { type="curve", closed=false; { type="segment"; v1, v2 } }
end

local function boxshape(v1, v2)
  return { type="curve", closed=true;
	   { type="segment"; v1, V(v1.x, v2.y) },
	   { type="segment"; V(v1.x, v2.y), v2 },
	   { type="segment"; v2, V(v2.x, v1.y) } }
end

local function arcshape(center, radius, alpha, beta)
  local a = ipe.Arc(ipe.Matrix(radius, 0, 0, radius, center.x, center.y),
		    alpha, beta)
  local v1 = center + radius * ipe.Direction(alpha)
  local v2 = center + radius * ipe.Direction(beta)
  return { type="curve", closed=false; { type="arc", arc=a; v1, v2 } }
end

local function rarcshape(center, radius, alpha, beta)
  local a = ipe.Arc(ipe.Matrix(radius, 0, 0, -radius, center.x, center.y),
		    alpha, beta)
  local v1 = center + radius * ipe.Direction(alpha)
  local v2 = center + radius * ipe.Direction(beta)
  return { type="curve", closed=false; { type="arc", arc=a; v1, v2 } }
end

----------------------------------------------------------------------

local VERTEX = 1
local SPLINE = 2
local ARC = 3

LINESTOOL = {}
LINESTOOL.__index = LINESTOOL

-- mode is one of "lines", "polygons", "splines"
function LINESTOOL:new(model, mode)
  local tool = {}
  setmetatable(tool, LINESTOOL)
  tool.model = model
  tool.mode = mode
  tool.splinetype = "spline"
  if model.attributes.splinetype == "cardinal" then
    tool.splinetype = "cardinal"
  elseif model.attributes.splinetype == "spiro" then
    tool.splinetype = "spiro"
  end
  tool.tension = model.attributes.tension
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:setAutoOrigin(v)
  if mode == "splines" then
    tool.t = { VERTEX, SPLINE }
  else
    tool.t = { VERTEX, VERTEX }
  end
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  tool.setSnapping(true, true)
  return tool
end

function LINESTOOL:last()
  return self.t[#self.t]
end

function LINESTOOL:has_segs(count)
  if #self.t < count then return false end
  local result = true
  for i = #self.t - count + 1,#self.t do
    if self.t[i] ~= VERTEX then return false end
  end
  return true
end

local function compute_arc(p0, pmid, p1)
  if p0 == p1 then return { type="segment", p0, p1 } end
  local l1 = ipe.Line(p0, (pmid- p0):normalized())
  local l2 = ipe.Line(p0, l1:normal())
  local bi1 = ipe.Bisector(p0, p1)
  local center = l2:intersects(bi1)
  local side = l1:side(p1)
  if side == 0.0 or not center then return { type="segment", p0, p1 } end
  local u = p0 - center
  local alpha = side * u:angle()
  local beta = side * ipe.normalizeAngle((p1 - center):angle(), alpha)
  local radius = u:len()
  local m = { radius, 0, 0, side * radius, center.x, center.y }
  return { type = "arc", p0, p1, arc = ipe.Arc(ipe.Matrix(m), alpha, beta) }
end

-- compute orientation of tangent to previous segment at its final point
function LINESTOOL:compute_orientation()
  if self.t[#self.t - 2] ~= ARC then
    return (self.v[#self.v - 1] - self.v[#self.v - 2]):angle()
  else
    -- only arc needs special handling
    local arc = compute_arc(self.v[#self.v - 3], self.v[#self.v - 2],
			    self.v[#self.v - 1])
    if arc.type == "arc" then
      local alpha, beta = arc.arc:angles()
      local dir = ipe.Direction(beta + math.pi / 2)
      return (arc.arc:matrix():linear() * dir):angle()
    else
      return (self.v[#self.v - 1] - self.v[#self.v - 3]):angle()
    end
  end
end

function LINESTOOL:compute()
  self.shape = { type="curve", closed=(self.mode == "polygons") }
  local i = 2
  while i <= #self.v do
    -- invariant: t[i-1] == VERTEX
    local seg
    if self.t[i] == VERTEX then
      seg = { type="segment", self.v[i-1], self.v[i] }
      i = i + 1
    elseif self.t[i] == SPLINE then
      local j = i
      while j <= #self.v and self.t[j] == SPLINE do j = j + 1 end
      if j > #self.v then j = #self.v end
      seg = { type = self.splinetype }
      if self.splinetype == "cardinal" then seg.tension = prefs.spline_tension end
      for k = i-1,j do seg[#seg+1] = self.v[k] end
      i = j + 1
    elseif self.t[i] == ARC then
      seg = compute_arc(self.v[i-1], self.v[i], self.v[i+1])
      i = i + 2
    end
    self.shape[#self.shape + 1] = seg
  end
  self.setShape( { self.shape } )
end

function LINESTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  self.v[#self.v] = v
  if button == 0x81 then
    -- double click
    -- the first click already added a vertex, do not use second one
    if #self.v == 2 then return end
    table.remove(self.v)
    table.remove(self.t)
    button = 2
  end
  if modifiers.control and button == 1 then button = 2 end
  if button == 2 then
    if self:last() == SPLINE then self.t[#self.t] = VERTEX end
    self:compute()
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape }, true)
    -- if it is just a point, force line cap to round
    if #self.shape == 1 and self.shape[1].type == "segment" and
      self.shape[1][1] == self.shape[1][2] then
      obj:set("linecap", "round")
    end
    self.model:creation("create path", obj)
    return
  end
  self.v[#self.v + 1] = v
  self.model.ui:setAutoOrigin(v)
  if self:last() == SPLINE then
    local typ = modifiers.shift and VERTEX or SPLINE
    self.t[#self.t] = typ
    self.t[#self.t + 1] = typ
  else
    self.t[#self.t + 1] = VERTEX
  end
  self:compute()
  self.model.ui:update(false) -- update tool
  self:explain()
end

function LINESTOOL:mouseMove()
  self.v[#self.v] = self.model.ui:pos()
  self:compute()
  self.model.ui:update(false) -- update tool
  self:explain()
end

function LINESTOOL:explain()
  local s
  if self:last() == SPLINE then
    s = ("Left: add ctrl point | Shift+Left: switch to line mode" ..
	 " | Del: delete ctrl point")
  else
    s = "Left: add vtx | Del: delete vtx"
  end
  s = s .. " | Right, Ctrl-Left, or Double-Left: final vtx"
  if self:has_segs(2) then
    s = s .. " | " .. shortcuts_linestool.spline .. ": spline mode"
  end
  if self:has_segs(3) then
    s = s .. " | " .. shortcuts_linestool.arc .. ": circle arc"
  end
  if #self.v > 2 and self.t[#self.t - 1] == VERTEX then
    s = s .. " | " .. shortcuts_linestool.set_axis ..": set axis"
  end
  self.model.ui:explain(s, 0)
end

function LINESTOOL:key(text, modifiers)
  if text == prefs.delete_key then  -- Delete
    if #self.v > 2 then
      table.remove(self.v)
      table.remove(self.t)
      if self:last() == ARC then
	self.t[#self.t] = VERTEX
      end
      self:compute()
      self.model.ui:update(false)
      self:explain()
    else
      self.model.ui:finishTool()
    end
    return true
  elseif text == "\027" then
    self.model.ui:finishTool()
    return true
  elseif self:has_segs(2) and text == shortcuts_linestool.spline then
    self.t[#self.t] = SPLINE
    self:compute()
    self.model.ui:update(false)
    self:explain()
    return true
  elseif self:has_segs(3) and text == shortcuts_linestool.arc then
    self.t[#self.t - 1] = ARC
    self:compute()
    self.model.ui:update(false)
    self:explain()
    return true
  elseif #self.v > 2 and self.t[#self.t - 1] == VERTEX and
    text == shortcuts_linestool.set_axis then
    -- set axis
    self.model.snap.with_axes = true
    self.model.ui:setActionState("show_axes", self.model.snap.with_axes)
    self.model.snap.snapangle = true
    self.model.snap.origin = self.v[#self.v - 1]
    self.model.snap.orientation = self:compute_orientation()
    self.model:setSnap()
    self.model.ui:setActionState("snapangle", true)
    self.model.ui:update(true)   -- redraw coordinate system
    self:explain()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

BOXTOOL = {}
BOXTOOL.__index = BOXTOOL

function BOXTOOL:new(model, square, mode)
  local tool = {}
  setmetatable(tool, BOXTOOL)
  tool.model = model
  tool.mode = mode
  tool.square = square
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:shapeTool(tool)
  tool.cur = 2
  tool.setColor(1.0, 0, 0)
  return tool
end

function BOXTOOL:compute()
  self.v[self.cur] = self.model.ui:pos()
  local d = self.v[2] - self.v[1]
  local sign = V(d.x > 0 and 1 or -1, d.y > 0 and 1 or -1)
  local dd = math.max(math.abs(d.x), math.abs(d.y))
  if self.mode == "rectangles1" or self.mode == "rectangles2" then
    if self.square then
      self.v[2] = self.v[1] + dd * sign
    end
    if self.mode == "rectangles1" then
      self.shape = boxshape(self.v[1], self.v[2])
    else
      local u = self.v[2] - self.v[1]
      self.shape = boxshape(self.v[1] - u, self.v[2])
    end
  elseif self.mode == "rectangles3" and self.square then
    local u = self.v[2] - self.v[1]
    local lu = V(-u.y, u.x)
    self.shape = { type="curve", closed=true;
		   { type="segment"; self.v[1], self.v[2] },
		   { type="segment"; self.v[2], self.v[2] + lu },
		   { type="segment"; self.v[2] + lu, self.v[1] + lu } }
  else
    if self.cur == 2 and self.square then -- make first-segment axis-parallel
      if math.abs(d.x) > math.abs(d.y) then
	self.v[2] = self.v[1] + V(sign.x * dd, 0)
      else
	self.v[2] = self.v[1] + V(0, sign.y * dd)
      end
    end
    if self.cur == 2 then
      self.shape = segmentshape(self.v[1], self.v[2])
    else
      if self.v[2] ~= self.v[1] and self.v[3] ~= self.v[2] and
	self.mode == "rectangles3" then
	local l1 = ipe.LineThrough(self.v[1], self.v[2])
	local l2 = ipe.Line(self.v[2], l1:normal())
	self.v[3] = l2:project(self.v[3])
      end
      local u = self.v[3] - self.v[2]
      self.shape = { type="curve", closed=true;
		     { type="segment"; self.v[1], self.v[2] },
		     { type="segment"; self.v[2], self.v[3] },
		     { type="segment"; self.v[3], self.v[1] + u } }
    end
  end
end

function BOXTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  self:compute()
  if self.cur == 2 and (self.mode == "parallelogram" or
			(self.mode == "rectangles3" and not self.square)) then
    self.cur = 3
    self.v[3] = self.v[2]
    return
  end
  self.model.ui:finishTool()
  local obj = ipe.Path(self.model.attributes, { self.shape })
  self.model:creation("create box", obj)
end

function BOXTOOL:mouseMove()
  self:compute()
  self.setShape( { self.shape } )
  self.model.ui:update(false) -- update tool
end

function BOXTOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

SPLINEGONTOOL = {}
SPLINEGONTOOL.__index = SPLINEGONTOOL

function SPLINEGONTOOL:new(model)
  local tool = {}
  setmetatable(tool, SPLINEGONTOOL)
  tool.model = model
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function SPLINEGONTOOL:compute(update)
  if #self.v == 2 then
    self.shape = segmentshape(self.v[1], self.v[2])
  else
    self.shape = { type="closedspline" }
    for _,v in ipairs(self.v) do self.shape[#self.shape + 1] = v end
  end
  if update then
    self.setShape( { self.shape } )
    self.model.ui:update(false) -- update tool
  end
end

function SPLINEGONTOOL:explain()
  local s = "Left: Add vertex "
    .. "| Right, ctrl-left, or double-left: Add final vertex"
    .. "| Del: Delete vertex"
  self.model.ui:explain(s, 0)
end

function SPLINEGONTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  self.v[#self.v] = v
  if button == 0x81 then
    -- double click
    -- the first click already added a vertex, do not use second one
    if #self.v == 2 then return end
    table.remove(self.v)
    button = 2
  end
  if modifiers.control and button == 1 then button = 2 end
  if button == 2 then
    self:compute(false)
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape })
    self.model:creation("create splinegon", obj)
    return
  end
  self.v[#self.v + 1] = v
  self:compute(true)
  self:explain()
end

function SPLINEGONTOOL:mouseMove()
  self.v[#self.v] = self.model.ui:pos()
  self:compute(true)
end

function SPLINEGONTOOL:key(text, modifiers)
  if text == prefs.delete_key then  -- Delete
    if #self.v > 2 then
      table.remove(self.v)
      self:compute(true)
      self:explain()
    else
      self.model.ui:finishTool()
    end
    return true
  elseif text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    self:explain()
    return false
  end
end

----------------------------------------------------------------------

CIRCLETOOL = {}
CIRCLETOOL.__index = CIRCLETOOL

function CIRCLETOOL:new(model, mode)
  local tool = {}
  setmetatable(tool, CIRCLETOOL)
  tool.model = model
  tool.mode = mode
  local v = model.ui:pos()
  tool.v = { v, v, v }
  tool.cur = 2
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function CIRCLETOOL:compute()
  if self.mode == "circle1" then
    self.shape = circleshape(self.v[1], (self.v[2] - self.v[1]):len())
  elseif self.mode == "circle2" then
    self.shape = circleshape(0.5 * (self.v[1] + self.v[2]),
			     (self.v[2] - self.v[1]):len() / 2.0)
  elseif self.mode == "circle3" then
    if self.cur == 2 or self.v[3] == self.v[2] then
      self.shape = circleshape(0.5 * (self.v[1] + self.v[2]),
			       (self.v[2] - self.v[1]):len() / 2.0)
    else
      local l1 = ipe.LineThrough(self.v[1], self.v[2])
      if math.abs(l1:side(self.v[3])) < 1e-9 then
	self.shape = segmentshape(self.v[1], self.v[2])
      else
	local l2 = ipe.LineThrough(self.v[2], self.v[3])
	local bi1 = ipe.Bisector(self.v[1], self.v[2])
	local bi2 = ipe.Bisector(self.v[2], self.v[3])
	local center = bi1:intersects(bi2)
	self.shape = circleshape(center, (self.v[1] - center):len())
      end
    end
  end
end

function CIRCLETOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  -- refuse point identical to previous
  if v == self.v[self.cur - 1] then return end
  self.v[self.cur] = v
  self:compute()
  if self.cur == 3 or (self.mode ~= "circle3" and self.cur == 2) then
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape })
    self.model:creation("create circle", obj)
  else
    self.cur = self.cur + 1
    self.model.ui:update(false)
  end
end

function CIRCLETOOL:mouseMove()
  self.v[self.cur] = self.model.ui:pos()
  self:compute()
  self.setShape({ self.shape })
  self.model.ui:update(false) -- update tool
end

function CIRCLETOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

ARCTOOL = {}
ARCTOOL.__index = ARCTOOL

function ARCTOOL:new(model, mode)
  local tool = {}
  setmetatable(tool, ARCTOOL)
  tool.model = model
  tool.mode = mode
  local v = model.ui:pos()
  tool.v = { v, v, v }
  tool.cur = 2
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 0)
  return tool
end

function ARCTOOL:compute()
  local u = self.v[2] - self.v[1]
  if self.cur == 2 then
    if self.mode == "arc3" then
      self.shape = circleshape(0.5 * (self.v[1] + self.v[2]), u:len() / 2.0)
    else
      self.shape = circleshape(self.v[1], u:len())
    end
    return
  end
  local alpha = u:angle()
  local beta = (self.v[3] - self.v[1]):angle()
  if self.mode == "arc1" then
    self.shape = arcshape(self.v[1], u:len(), alpha, beta)
  elseif self.mode == "arc2" then
    self.shape = rarcshape(self.v[1], u:len(), alpha, beta)
  else
    local l1 = ipe.LineThrough(self.v[1], self.v[2])
    if math.abs(l1:distance(self.v[3])) < 1e-9 or self.v[3] == self.v[2] then
      self.shape = segmentshape(self.v[1], self.v[3])
    else
      local l2 = ipe.LineThrough(self.v[2], self.v[3])
      local bi1 = ipe.Line(0.5 * (self.v[1] + self.v[2]), l1:normal())
      local bi2 = ipe.Line(0.5 * (self.v[2] + self.v[3]), l2:normal())
      local center = bi1:intersects(bi2)
      u = self.v[1] - center
      alpha = u:angle()
      beta = ipe.normalizeAngle((self.v[2] - center):angle(), alpha)
      local gamma = ipe.normalizeAngle((self.v[3] - center):angle(), alpha)
      if gamma > beta then
	self.shape = arcshape(center, u:len(), alpha, gamma)
      else
	self.shape = rarcshape(center, u:len(), alpha, gamma)
      end
    end
  end
end

function ARCTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  local v = self.model.ui:pos()
  -- refuse point identical to previous
  if v == self.v[self.cur - 1] then return end
  self.v[self.cur] = v
  self:compute()
  if self.cur == 3 then
    self.model.ui:finishTool()
    local obj = ipe.Path(self.model.attributes, { self.shape }, true)
    self.model:creation("create arc", obj)
  else
    self.cur = self.cur + 1
    self.model.ui:update(false)
  end
end

function ARCTOOL:mouseMove()
  self.v[self.cur] = self.model.ui:pos()
  self:compute()
  self.setShape({ self.shape })
  self.model.ui:update(false) -- update tool
end

function ARCTOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

local function path_len(points)
  local t = 0
  for i=2,#points do
    t = t + (points[i] - points[i-1]):len()
  end
  return t
end

-- perform moving average
local function smooth_path(v)
  local M = prefs.ink_smoothing
  local w = { v[1] }
  for i = 2,#v-1 do
    local p = V(0,0)
    for j = i-M, i+M do
      local q
      if j < 1 then q = v[1] elseif j > #v then q = v[#v] else q = v[i] end
      p = p + q
    end
    w[#w+1] = (1/(2*M+1)) * p
  end
  w[#w+1] = v[#v]
  return w
end

local function simplify_path(points, tolerance)
  local marked = {}
  marked[1] = true
  marked[#points] = true
  local stack = { { 1, #points } }

  -- mark points to keep
  while #stack > 0 do
    local pair = table.remove(stack)
    local first = pair[1]
    local last = pair[2]
    local max_dist = 0
    local index = nil
    local seg = ipe.Segment(points[first], points[last])
    for i = first + 1,last do
      local d = seg:distance(points[i])
      if d > max_dist then
        index = i
        max_dist = d
      end
    end
    if max_dist > tolerance then
      marked[index] = true
      stack[#stack + 1] = { first, index }
      stack[#stack + 1] = { index, last }
    end
  end
  -- only use marked vertices
  local out = {}
  for i, point in ipairs(points) do
    if marked[i] then
      out[#out+1] = point
    end
  end
  return out
end

-- vector for control points around p2
local function tang(p1, p2, p3)
  return (p2-p1):normalized() + (p3 - p2):normalized()
end

-- first control point between p2 and p3
local function cp1(p1, p2, p3)
  local tangent = tang(p1, p2, p3):normalized()
  local vlen = (p3 - p2):len() / 3.0
  return p2 + vlen * tangent
end

-- second control point between p2 and p3
local function cp2(p2, p3, p4)
  local tangent = tang(p2, p3, p4):normalized()
  local vlen = (p3 - p2):len() / 3.0
  return p3 - vlen * tangent
end

local function compute_spline(w)
  local shape = { }
  if #w > 2 then
    local cp = cp2(w[1], w[2], w[3])
    local seg = { w[1], cp, w[2] }
    seg.type = "spline"
    shape[1] = seg
  else
    local seg = { w[1], w[2] }
    seg.type = "segment"
    shape[1] = seg
  end
  for i = 2,#w-2 do
    local q1 = w[i-1]
    local q2 = w[i]
    local q3 = w[i+1]
    local q4 = w[i+2]
    local cpa = cp1(q1, q2, q3)
    local cpb = cp2(q2, q3, q4)
    local seg = { q2, cpa, cpb, q3}
    seg.type = "spline"
    shape[#shape + 1] = seg
  end
  if #w > 2 then
    local cp = cp1(w[#w-2], w[#w-1], w[#w])
    local seg = { w[#w-1], cp, w[#w] }
    seg.type = "spline"
    shape[#shape + 1] = seg
  end
  return shape
end

----------------------------------------------------------------------

INKTOOL = {}
INKTOOL.__index = INKTOOL

function INKTOOL:new(model)
  local tool = {}
  setmetatable(tool, INKTOOL)
  tool.model = model
  local v = model.ui:pos()
  tool.v = { v }
  -- print("make ink tool")
  model.ui:shapeTool(tool)
  local s = model.doc:sheets():find("color", model.attributes.stroke)
  tool.setColor(s.r, s.g, s.b)
  tool.w = model.doc:sheets():find("pen", model.attributes.pen)
  model.ui:setCursor(tool.w, s.r, s.g, s.b)
  return tool
end

function INKTOOL:compute(incremental)
  local v = self.v
  if incremental and #v > 2 then
    self.shape[#self.shape+1]={ type="segment", v[#v-1], v[#v]}
  else
    self.shape = { type="curve", closed=false }
    for i = 2, #v do
      self.shape[#self.shape + 1] = { type="segment", v[i-1], v[i] }
    end
  end
  self.setShape( { self.shape }, 0, self.w * self.model.ui:zoom())
end

function INKTOOL:mouseButton(button, modifiers, press)
  if self.shape then
    self.v = smooth_path(self.v)
    self.v = simplify_path(self.v, prefs.ink_tolerance / self.model.ui:zoom())
  else
    self.v = { self.v[1], self.v[1] }
  end
  if prefs.ink_spline then
    self.shape = compute_spline(self.v)
    self.shape.type = "curve"
    self.shape.closed = false
  else
    self:compute(false)
  end
  local obj = ipe.Path(self.model.attributes, { self.shape })
  -- round linecaps are prettier for handwriting
  obj:set("linecap", "round")
  obj:set("linejoin", "round")
  if path_len(self.v) * self.model.ui:zoom() < prefs.ink_dot_length then
    -- pen was pressed and released with no or very little movement
    if prefs.ink_dot_wider then
      obj:set("pen", self.w * prefs.ink_dot_wider)
    end
    self.model:creation("create ink dot", obj)
  else
    self.model:creation("create ink path", obj)
  end
  self.model:page():deselectAll()  -- ink isn't selected after creation
  self.model.ui:finishTool()
end

function INKTOOL:mouseMove()
  local v1 = self.v[#self.v]
  local v2 = self.model.ui:pos()
  if (v1-v2):len() * self.model.ui:zoom() > prefs.ink_min_movement then
    -- do not update if motion is too small
    self.v[#self.v + 1] = v2
    self:compute(true)
    local r = ipe.Rect()
    local offset = V(self.w, self.w)
    r:add(v1)
    r:add(v2)
    r:add(r:bottomLeft() - offset)
    r:add(r:topRight() + offset)
    self.model.ui:update(r) -- update rectangle containing new segment
  end
end

function INKTOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

function MODEL:shredObject()
  local bound = prefs.close_distance
  local pos = self.ui:unsnappedPos()
  local p = self:page()

  local closest
  for i,obj,sel,layer in p:objects() do
     if p:visible(self.vno, i) and not p:isLocked(layer) then
	local d = p:distance(i, pos, bound)
	if d < bound then closest = i; bound = d end
     end
  end

  if closest then
    local t = { label="shred object",
		pno = self.pno,
		vno = self.vno,
		num = closest,
		original=self:page():clone(),
		undo=revertOriginal,
	      }
    t.redo = function (t, doc)
	       local p = doc[t.pno]
	       p:remove(t.num)
	       p:ensurePrimarySelection()
	     end
    self:register(t)
  else
    self.ui:explain("No object found to shred")
  end
end

----------------------------------------------------------------------

function MODEL:createMark()
  local obj = ipe.Reference(self.attributes, self.attributes.markshape,
			    self.ui:pos())
  self:creation("create mark", obj)
end

----------------------------------------------------------------------

function MODEL:createText(mode, pos, width, pinned)
  local prompt = "Enter Latex source"
  local stylekind = "labelstyle"
  local styleatt = self.attributes.labelstyle
  local explainer = "create text"
  if mode == "math" then
    prompt = "Enter Latex source for math formula"
  end
  if mode == "paragraph" then
    styleatt = self.attributes.textstyle
    stylekind = "textstyle"
    explainer = "create text paragraph"
  end
  local styles = self.doc:sheets():allNames(stylekind)
  local sizes = self.doc:sheets():allNames("textsize")
  local d = ipeui.Dialog(self.ui:win(), "Create text object")
  d:add("label", "label", { label=prompt }, 1, 1, 1, 2)
  d:add("style", "combo", styles, -1, 3)
  d:add("size", "combo", sizes, -1, 4)
  d:add("text", "text", { syntax="latex", focus=true,
			  spell_check=prefs.spell_check }, 0, 1, 1, 4)
  addEditorField(d, "text")
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 2, 1)
  d:set("ignore-escape", "text", "")
  local style = indexOf(styleatt, styles)
  if not style then style = indexOf("normal", styles) end
  local size = indexOf(self.attributes.textsize, sizes)
  if not size then size = indexOf("normal", sizes) end
  d:set("style", style)
  d:set("size", size)
  if mode == "math" then
    d:set("style", "math")
    d:setEnabled("style", false)
  end
  if prefs.auto_external_editor then
    externalEditor(d, "text")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    local t = d:get("text")
    local style = styles[d:get("style")]
    local size = sizes[d:get("size")]
    local obj = ipe.Text(self.attributes, t, pos, width)
    obj:set("textsize", size)
    obj:set(stylekind, style)
    if pinned then
      obj:set("pinned", "horizontal")
    end
    self:creation(explainer, obj)
    self:autoRunLatex()
  end
end

----------------------------------------------------------------------

function MODEL:action_insert_text_box()
  local layout = self.doc:sheets():find("layout")
  local p = self:page()
  local r = ipe.Rect()
  local m = ipe.Matrix()
  for i, obj, sel, layer in p:objects() do
    if p:visible(self.vno, i) and obj:type() == "text" then
      obj:addToBBox(r, m)
    end
  end
  local y = layout.framesize.y
  if not r:isEmpty() and r:bottom() < layout.framesize.y then
    y = r:bottom() - layout.paragraph_skip
  end
  self:createText("paragraph", ipe.Vector(0, y), layout.framesize.x, true)
end

----------------------------------------------------------------------

PARAGRAPHTOOL = {}
PARAGRAPHTOOL.__index = PARAGRAPHTOOL

function PARAGRAPHTOOL:new(model)
  local tool = {}
  setmetatable(tool, PARAGRAPHTOOL)
  tool.model = model
  local v = model.ui:pos()
  tool.v = { v, v }
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 1.0)
  return tool
end

function PARAGRAPHTOOL:compute()
  self.v[2] = V(self.model.ui:pos().x, self.v[1].y - 20)
  self.shape = boxshape(self.v[1], self.v[2])
end

function PARAGRAPHTOOL:mouseButton(button, modifiers, press)
  if not press then return end
  self:compute()
  self.model.ui:finishTool()
  local pos = V(math.min(self.v[1].x, self.v[2].x), self.v[1].y)
  local wid = math.abs(self.v[2].x - self.v[1].x)
  self.model:createText("paragraph", pos, wid)
end

function PARAGRAPHTOOL:mouseMove()
  self:compute()
  self.setShape( { self.shape } )
  self.model.ui:update(false) -- update tool
end

function PARAGRAPHTOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

local function collect_edges(p, box, view)
  local edges = {}
  for i, obj, sel, layer in p:objects() do
    if obj:type() == "path" and (view == nil or p:visible(view, i)) then
      local shape = obj:shape()
      local m = obj:matrix()
      if #shape == 1 and shape[1].type == "curve"
	and shape[1].closed == false then
	local curve = shape[1]
	local head = curve[1][1]
	local seg = curve[#curve]
	local tail = seg[#seg]
	if box:contains(m * head) then
	  edges[#edges + 1] = { obj=obj, head=true, objno=i }
	elseif box:contains(m * tail) then
	  edges[#edges + 1] = { obj=obj, head=false, objno=i }
	end
      end
    end
  end
  return edges
end

----------------------------------------------------------------------

GRAPHTOOL = {}
GRAPHTOOL.__index = GRAPHTOOL

local function findClosestVertex(model)
  local bound = prefs.close_distance
  local pos = model.ui:unsnappedPos()
  local p = model:page()

  local closest
  for i,obj,sel,layer in p:objects() do
    if p:visible(model.vno, i) and not p:isLocked(layer) then
      if obj:type() == "group" or obj:type() == "reference" or
        obj:type() == "text" then
	local d = p:distance(i, pos, bound)
	if d < bound then closest = i; bound = d end
      end
    end
  end

  if closest then
    -- deselect all, and select only closest object
    p:deselectAll()
    p:setSelect(closest, 1)
    return true
  else
    return p:hasSelection()
  end
end

function GRAPHTOOL:new(model, moveInvisibleEdges)
  if not findClosestVertex(model) then return end
  local p = model:page()
  self.prim = p:primarySelection()
  local view = model.vno
  if moveInvisibleEdges then view = nil end
  local tool = {}
  setmetatable(tool, GRAPHTOOL)
  tool.model = model
  tool.orig = model.ui:pos()
  tool.box = p:bbox(self.prim)
  tool.edges = collect_edges(p, tool.box, view)
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0.0, 0.0)
  return tool
end

local function moveEndpoint(obj, head, translation)
  local shape = obj:shape()
  transformShape(obj:matrix(), shape)
  local curve = shape[1]
  if head then
    curve[1][1] = curve[1][1] + translation
    if curve[1].type == "arc" then
      recomputeArcMatrix(curve[1], 1)
    end
  else
    local seg = curve[#curve]
    seg[#seg] = seg[#seg] + translation
    if seg.type == "arc" then
      recomputeArcMatrix(seg, #seg)
    end
  end
  return shape
end

function GRAPHTOOL:compute()
  self.t = self.model.ui:pos() - self.orig
  self.shape = { boxshape(self.box:bottomLeft() + self.t,
			  self.box:topRight() + self.t) }
  for i = 1,#self.edges do
    self.shape[i+1] = moveEndpoint(self.edges[i].obj, self.edges[i].head, self.t)[1]
  end
end

function GRAPHTOOL:mouseMove()
  self:compute()
  self.setShape(self.shape)
  self.model.ui:update(false) -- update tool
end

local function apply_node_mode(t, doc)
  local p = doc[t.pno]
  p:transform(t.primary, ipe.Translation(t.translation))
  for _,e in ipairs(t.edges) do
    p[e.objno]:setShape(moveEndpoint(p[e.objno], e.head, t.translation))
    p[e.objno]:setMatrix(ipe.Matrix())
  end
end

function GRAPHTOOL:mouseButton(button, modifiers, press)
  if press then return end
  self:compute()
  self.model.ui:finishTool()
  local edges = {}
  for i = 1,#self.edges do
    self.edges[i].obj = nil
  end

  local t = { label = "move graph vertex",
	      pno = self.model.pno,
	      vno = self.model.vno,
	      primary = self.prim,
	      original = self.model:page():clone(),
	      translation = self.t,
	      edges = self.edges,
	      undo = revertOriginal,
	      redo = apply_node_mode,
	    }
  self.model:register(t)
end

function GRAPHTOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

CHANGEWIDTHTOOL = {}
CHANGEWIDTHTOOL.__index = CHANGEWIDTHTOOL

function CHANGEWIDTHTOOL:new(model, prim, obj)
  local tool = {}
  setmetatable(tool, CHANGEWIDTHTOOL)
  tool.model = model
  tool.prim = prim
  tool.obj = obj
  tool.pos = obj:matrix() * obj:position()
  tool.wid = obj:get("width")
  tool.align = obj:get("horizontalalignment")
  if tool.align == "right" then
    tool.posfactor = -1
    tool.dir = -1
  elseif tool.align == "hcenter" then
    tool.posfactor = -0.5
    tool.dir = 1
  else
    tool.posfactor = 0
    tool.dir = 1
  end
  model.ui:shapeTool(tool)
  tool.setColor(1.0, 0, 1.0)
  local pos = tool.pos + V(tool.posfactor * tool.wid, 0)
  tool.setShape( { boxshape(pos, pos + V(tool.wid, -20)) } )
  model.ui:update(false) -- update tool
  return tool
end

function CHANGEWIDTHTOOL:compute()
  local w = self.model.ui:pos()
  self.nwid = self.wid + self.dir * (w.x - self.v.x)
  local pos = self.pos + V(self.posfactor * self.nwid, 0)
  self.shape = boxshape(pos, pos + V(self.nwid, -20))
end

function CHANGEWIDTHTOOL:mouseButton(button, modifiers, press)
  if press then
    if not self.v then self.v = self.model.ui:pos() end
    if self.align == "hcenter" and self.v.x < self.pos.x then self.dir = -1 end
  else
    self:compute()
    self.model.ui:finishTool()
    if self.nwid <= 0 then
      self.model:warning("The width of a text object should be positive.")
      return
    end
    self.model:setAttributeOfPrimary(self.prim, "width", self.nwid)
    self.model:autoRunLatex()
  end
end

function CHANGEWIDTHTOOL:mouseMove()
  if self.v then
    self:compute()
    self.setShape( { self.shape } )
    self.model.ui:update(false) -- update tool
  end
end

function CHANGEWIDTHTOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

function MODEL:action_change_width()
  local p = self:page()
  local prim = p:primarySelection()
  if not prim or p[prim]:type() ~= "text" or not p[prim]:get("minipage") then
    self.ui:explain("no selection or not a minipage object")
    return
  end
  CHANGEWIDTHTOOL:new(self, prim, p[prim])
end

----------------------------------------------------------------------

function MODEL:startTransform(mode, withShift)
  local deselect = self:updateCloseSelection()
  if mode == "stretch" and withShift then mode = "scale" end
  self.ui:transformTool(self:page(), self.vno, mode, withShift,
			function (m) self:transformation(mode, m, deselect) end)
end

function MODEL:startModeTool(modifiers)
  if self.mode == "select" then
    self.ui:selectTool(self:page(), self.vno,
		       prefs.select_distance, modifiers.shift)
  elseif (self.mode == "translate" or self.mode == "stretch"
	  or self.mode == "rotate" or self.mode == "shear") then
    self:startTransform(self.mode, modifiers.shift)
  elseif self.mode == "pan" then
    self.ui:panTool(self:page(), self.vno)
  elseif self.mode == "shredder" then
    self:shredObject()
  elseif self.mode == "graph" then
    GRAPHTOOL:new(self, modifiers.shift)
  elseif self.mode:sub(1,10) == "rectangles" or self.mode == "parallelogram" then
    BOXTOOL:new(self, modifiers.shift, self.mode)
  elseif self.mode == "splinegons" then
    SPLINEGONTOOL:new(self)
  elseif (self.mode == "lines" or self.mode == "polygons" or
	  self.mode == "splines") then
    LINESTOOL:new(self, self.mode)
  elseif self.mode:sub(1,6) == "circle" then
    CIRCLETOOL:new(self, self.mode)
  elseif self.mode:sub(1,3) == "arc" then
    ARCTOOL:new(self, self.mode)
  elseif self.mode == "ink" then
    INKTOOL:new(self)
  elseif self.mode == "marks" then
    self:createMark()
  elseif self.mode == "label" or self.mode == "math" then
    self:createText(self.mode, self.ui:pos())
  elseif self.mode == "paragraph" then
    PARAGRAPHTOOL:new(self)
  elseif self.mode == "laser" then
    LASERTOOL:new(self)
  else
    print("start mode tool:", self.mode)
  end
end

local mouse_mappings = {
  select = function (m, mo)
	     m.ui:selectTool(m:page(), m.vno, prefs.select_distance, mo.shift)
	   end,
  translate = function (m, mo) m:startTransform("translate", mo.shift) end,
  rotate = function (m, mo) m:startTransform("rotate", mo.shift) end,
  stretch = function (m, mo) m:startTransform("stretch", mo.shift) end,
  scale = function (m, mo) m:startTransform("stretch", true) end,
  pan = function (m, mo) m.ui:panTool(m:page(), m.vno) end,
  menu = function (m, mo) m:propertiesPopup() end,
  shredder = function (m, mo) m:shredObject() end,
}

function MODEL:mouseButtonAction(button, modifiers)
  -- print("Mouse button", button, modifiers.alt, modifiers.control)
  if button == 0x81 then button = 1 end -- left double-click
  if button == 1 and not modifiers.alt and
    not modifiers.command and not modifiers.control and not modifiers.meta then
    self:startModeTool(modifiers)
  else
    local s = ""
    if button == 1 then s = "left"
    elseif button == 2 then s = "right"
    elseif button == 4 then s = "middle"
    elseif button == 8 then s = "button8"
    elseif button == 16 then s = "button9"
    elseif button == 0 then
      -- This is a hack because of the Qt limitation.
      -- It really means any other button.
      s = "button10"
    end
    if modifiers.shift then s = s .. "_shift" end
    if modifiers.control then s = s .. "_control" end
    if modifiers.alt then s = s .. "_alt" end
    if modifiers.meta then s = s .. "_meta" end
    if modifiers.command then s = s .. "_command" end
    local r = mouse[s]
    if type(r) == "string" then r = mouse_mappings[r] end
    if r then
      r(self, modifiers)
    else
      print("No mouse action defined for " .. s)
    end
  end
end

----------------------------------------------------------------------

function apply_text_edit(d, data, run_latex)
  -- refuse to do anything with empty text
  if string.match(d:get("text"), "^%s*$") then return end
  if data.obj:text() == d:get("text") and
     (not data.size or data.obj:get("textsize") == data.sizes[d:get("size")]) and
     (not data.style or data.obj:get(data.stylekind) == data.styles[d:get("style")]) then
     return -- hasn't changed since last time
  end
  local model = data.model
  local final = data.obj:clone()
  final:setText(d:get("text"))
  if data.style then final:set(data.stylekind, data.styles[d:get("style")]) end
  if data.size then final:set("textsize", data.sizes[d:get("size")]) end
  local t = { label="edit text",
	      pno=model.pno,
	      vno=model.vno,
	      original=data.obj:clone(),
	      primary=data.prim,
	      final=final,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.final)
	   end
  model:register(t)
  -- need to update data.obj for the next run!
  data.obj = final
  if run_latex then model:runLatex() end
end

function MODEL:action_edit_text(prim, obj)
  local mp = obj:get("minipage")
  local stylekind = "labelstyle"
  if mp then stylekind = "textstyle" end
  local d = ipeui.Dialog(self.ui:win(), "Edit text object")
  local data = { model=self,
		 stylekind = stylekind,
		 styles = self.doc:sheets():allNames(stylekind),
		 sizes = self.doc:sheets():allNames("textsize"),
		 prim=prim,
		 obj=obj,
	       }
  d:add("label", "label", { label="Edit latex source" }, 1, 1, 1, 2)
  d:add("text", "text", { syntax="latex", focus=true,
			  spell_check=prefs.spell_check}, 0, 1, 1, 4)
  d:addButton("apply", "&Apply",
	      function (d) apply_text_edit(d, data, true) end)
  addEditorField(d, "text")
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 2, 1)
  d:set("text", obj:text())
  d:set("ignore-escape", "text", obj:text())
  data.style = indexOf(obj:get(stylekind), data.styles)
  if data.style then
    d:add("style", "combo", data.styles, 1, 3)
    d:set("style", data.style)
  end
  data.size = indexOf(obj:get("textsize"), data.sizes)
  if data.size then
    d:add("size", "combo", data.sizes, 1, 4)
    d:set("size", data.size)
  end
  if prefs.auto_external_editor then
    externalEditor(d, "text")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    if string.match(d:get("text"), "^%s*$") then return end
    apply_text_edit(d, data, self.auto_latex)
  end
end

function MODEL:accept_group_text_edit(prim, group, t, tobj)
  local els = group:elements()
  els[t] = tobj
  local final = ipe.Group(els)
  -- copy properties
  final:set("pinned", group:get("pinned"))
  final:set("transformations", group:get("transformations"))
  final:setMatrix(group:matrix())
  final:setText(group:text())
  final:setClip(group:clip())
  final:set("decoration", group:get("decoration"))
  local t = { label="edit text in group",
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=group:clone(),
	      final=final,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.final)
	   end
  self:register(t)
  self:autoRunLatex()
end

function MODEL:action_edit_group_text(prim, obj)
  local t = nil
  for i = 1,obj:count() do
    if obj:elementType(i) == "text" then t = i end
  end
  if not t then
    self:warning("Cannot edit object",
		 "Only groups containing text can be edited")
    return
  end
  local tobj = obj:element(t)
  local d = ipeui.Dialog(self.ui:win(), "Edit text in group object")
  d:add("label", "label", { label="Edit latex source" }, 1, 1, 1, 2)
  d:add("text", "text", { syntax="latex", focus=true,
			  spell_check=prefs.spell_check}, 0, 1, 1, 4)
  addEditorField(d, "text")
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 2, 1)
  d:set("text", tobj:text())
  d:set("ignore-escape", "text", tobj:text())
  if prefs.auto_external_editor then
    externalEditor(d, "text")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    if string.match(d:get("text"), "^%s*$") then return end
    local final = tobj:clone()
    final:setText(d:get("text"))
    self:accept_group_text_edit(prim, obj, t, final)
  end
end

function MODEL:action_edit()
  local p = self:page()
  local prim = p:primarySelection()
  if not prim then self.ui:explain("no selection") return end
  local obj = p[prim]
  if obj:type() == "text" then
    self:action_edit_text(prim, obj)
  elseif obj:type() == "path" then
    self:action_edit_path(prim, obj)
  elseif obj:type() == "group" then
    self:action_edit_group_text(prim, obj)
  else
    self:warning("Cannot edit " .. obj:type() .. " object",
		 "Only text objects, path objects, and groups with text can be edited")
  end
end

----------------------------------------------------------------------

local function start_group_edit(t, doc)
  local p = doc[t.pno]
  local layers = p:layers()
  local layerOfPrim = p:layerOf(t.primary)
  local activeLayer = p:active(t.vno)
  local activeIndex = indexOf(activeLayer, layers)
  local g = p[t.primary]
  local elements = g:elements()
  local matrix = g:matrix()
  local layer = "EDIT-GROUP"
  -- make sure new layer name is unique
  while indexOf(layer, layers) do
    layer = layer .. "*"
  end
  local data = "active=" .. activeLayer .. ";primary=" .. layerOfPrim .. ";"
  if g:get("decoration") ~= "normal" then
    data = data .. "decoration=" .. g:get("decoration"):sub(12) .. ";"
  end
  if g:text() ~= "" then
    data = data .. "url=" .. g:text() .. ";"
  end
  data = data .. "locked="
  for _,l in ipairs(layers) do
    if p:isLocked(l) then data = data .. l .. "," end
  end
  p:remove(t.primary)
  p:addLayer(layer)
  p:setLayerData(layer, data)
  p:moveLayer(layer, activeIndex + 1)
  for _,obj in ipairs(elements) do
    p:insert(nil, obj, nil, layer)
    p:transform(#p, matrix)
  end
  p:deselectAll()
  p:setActive(t.vno, layer)
  p:setVisible(t.vno, layer, true)
  for _,l in ipairs(layers) do p:setLocked(l, true) end
end

local function end_group_edit(t, doc)
  local p = doc[t.pno]
  local activeLayer = p:active(t.vno)
  local data = p:layerData(activeLayer)
  local elements = {}
  for i, obj, sel, layer in p:objects() do
    if layer == activeLayer then
      elements[#elements + 1] = obj:clone()
    end
  end
  for i = #p,1,-1 do
    if p:layerOf(i) == activeLayer then
      p:remove(i)
    end
  end
  p:removeLayer(activeLayer)
  local group = ipe.Group(elements)
  for _,l in ipairs(p:layers()) do p:setLocked(l, false) end
  print("End group edit: ", data)
  local newActive = nil
  local layerOfGroup = nil
  for w in string.gmatch(data, "%w+=[^;]*") do
    if w:sub(1, 11) == "decoration=" then
      group:set("decoration", "decoration/" .. w:sub(12))
    end
    if w:sub(1, 7) == "active=" then
      newActive = w:sub(8)
    end
    if w:sub(1, 8) == "primary=" then
      layerOfGroup = w:sub(9)
    end
    if w:sub(1, 7) == "locked=" then
      locked = w:sub(8)
      for lock in string.gmatch(locked, "[^,]+") do
	p:setLocked(lock, true)
      end
    end
    if w:sub(1, 4) == "url=" then
      group:setText(w:sub(5))
    end
  end
  -- to be safe, if the page was tampered with
  if not newActive then
    newActive = p:layers()[1] -- just use first layer of the page
  end
  if p:isLocked(newActive) then p:setLocked(newActive, false) end
  if not layerOfGroup then layerOfGroup = newActive end
  p:setActive(t.vno, newActive)
  p:insert(nil, group, 1, layerOfGroup)
end

function MODEL:saction_edit_group()
  local p = self:page()
  local prim = p:primarySelection()
  if p[prim]:type() ~= "group" then
    self.ui:explain("primary selection is not a group")
    return
  end
  local t = { label="start group edit",
	      pno = self.pno,
	      vno = self.vno,
	      primary = prim,
	      original = p:clone(),
	      undo = revertOriginal,
	      redo = start_group_edit,
	    }
  self:register(t)
end

function MODEL:action_end_group_edit()
  local p = self:page()
  if (not string.match(p:active(self.vno), "^EDIT%-GROUP")) or
    p:countLayers() < 2 then
    self:warning("Cannot end group edit",
		 "Active layer is not a group edit layer")
    return
  end
  local t = { label="end group edit",
	      pno = self.pno,
	      vno = self.vno,
	      original = p:clone(),
	      undo = revertOriginal,
	      redo = end_group_edit,
	    }
  self:register(t)
end

----------------------------------------------------------------------

PASTETOOL = {}
PASTETOOL.__index = PASTETOOL

function PASTETOOL:new(model, elements, pos)
  local tool = {}
  _G.setmetatable(tool, PASTETOOL)
  tool.model = model
  tool.elements = elements
  tool.start = model.ui:pos()
  if pos then tool.start = pos end
  local obj = ipe.Group(elements)
  tool.pinned = obj:get("pinned")
  model.ui:pasteTool(obj, tool)
  tool.setColor(1.0, 0, 0)
  tool:computeTranslation()
  tool.setMatrix(ipe.Translation(tool.translation))
  return tool
end

function PASTETOOL:computeTranslation()
  self.translation = self.model.ui:pos() - self.start
  if self.pinned == "horizontal" or self.pinned == "fixed" then
    self.translation = V(0, self.translation.y)
  end
  if self.pinned == "vertical" or self.pinned == "fixed" then
    self.translation = V(self.translation.x, 0)
  end
end

function PASTETOOL:mouseButton(button, modifiers, press)
  self:computeTranslation()
  self.model.ui:finishTool()
  local t = { label="paste objects at cursor",
	      pno = self.model.pno,
	      vno = self.model.vno,
	      elements = self.elements,
	      layer = self.model:page():active(self.model.vno),
	      translation = ipe.Translation(self.translation),
	    }
  t.undo = function (t, doc)
	     local p = doc[t.pno]
	     for i = 1,#t.elements do p:remove(#p) end
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i,obj in ipairs(t.elements) do
	       p:insert(nil, obj, 2, t.layer)
	       p:transform(#p, t.translation)
	     end
	     p:ensurePrimarySelection()
	   end
  self.model:page():deselectAll()
  self.model:register(t)
end

function PASTETOOL:mouseMove()
  self:computeTranslation()
  self.setMatrix(ipe.Translation(self.translation))
  self.model.ui:update(false) -- update tool
end

function PASTETOOL:key(text, modifiers)
  if text == "\027" then
    self.model.ui:finishTool()
    return true
  else
    return false
  end
end

function MODEL:action_paste_at_cursor()
  local data = self.ui:clipboard(true) -- allow bitmap
  if not data then
    self:warning("Nothing to paste")
    return
  end
  if type(data) == "string" then
    if data:sub(1,13) ~= "<ipeselection" then
      self:warning("No Ipe selection to paste")
      return
    end
    local pos
    local px, py = data:match('^<ipeselection pos="([%d%.]+) ([%d%.]+)"')
    if px then pos = V(tonumber(px), tonumber(py)) end
    local elements = ipe.Object(data)
    if not elements then
      self:warning("Could not parse Ipe selection on clipboard")
      return
    end
    PASTETOOL:new(self, elements, pos)
  else
    -- pasting bitmap
    PASTETOOL:new(self, { data }, V(0, 0))
  end
end

----------------------------------------------------------------------

LASERTOOL = {}
LASERTOOL.__index = LASERTOOL

function LASERTOOL:new(model)
  local tool = {}
  _G.setmetatable(tool, LASERTOOL)
  tool.model = model
  tool.pos = model.ui:pos()
  model.ui:shapeTool(tool)
  local p = prefs.laser_pointer.color
  tool.setColor(p.r, p.g, p.b)
  tool:setPosShape()
  return tool
end

function LASERTOOL:setPosShape()
  local radius = prefs.laser_pointer.radius
  self.pos = self.model.ui:pos()
  local shape = { type="ellipse"; ipe.Matrix(radius, 0, 0, radius, self.pos.x, self.pos.y) }
  self.setShape( { shape }, 0, prefs.laser_pointer.pen)
  self.model.ui:update(false) -- update tool
end

function LASERTOOL:mouseButton(button, modifiers, press)
  if not press then
    self.model.ui:finishTool()
  end
end

function LASERTOOL:mouseMove()
  self:setPosShape()
end

function LASERTOOL:key(text, modifiers)
  return false
end

----------------------------------------------------------------------
