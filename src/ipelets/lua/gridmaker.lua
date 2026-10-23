----------------------------------------------------------------------
-- Making grids
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

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

label = "Grid maker"

category = "Basic"

about = [[
    Make various kinds of grids.

    (1) A grid for two-point perspective drawings.
    (2) A triangular grid.
    (3) A hexagonal grid.
    (4) A polar grid.
]]

V = _G.V

-- How far to fill the two faces with the grid
fillpart = 0.5

function incorrect(model)
  model:warning("Primary selection must be a quadrilateral, with a vertical and a " ..
		"horizontal diagonal")
end

function has_grid_layer(p, only_if_unlocked)
  local layers = p:layers()
  for _, l in ipairs(layers) do
    if l == "GRID" then
      if only_if_unlocked then
	return not p:isLocked("GRID")
      end
      return true
    end
  end
  return false
end

function collect_quadrilateral(model)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local obj = p[prim]
  if obj:type() ~= "path" then incorrect(model) return end
  local shape = obj:shape()
  if (#shape ~= 1 or shape[1].type ~= "curve" or #shape[1] ~= 3 or not shape[1].closed) then
    return
  end
  for i = 1,3 do
    if shape[1][i].type ~= "segment" then return end
  end
  local m = obj:matrix()
  local corners = { m * shape[1][1][1] }
  for i = 1,3 do corners[#corners+1] = m * shape[1][i][2] end
  table.sort(corners,
	     function (p, q) if p.x ~= q.x then return p.x < q.x else return p.y < q.y end end)
  if corners[2].x ~= corners[3].x or corners[1].y ~= corners[4].y then return end
  return { left=corners[1], bottom=corners[2], top=corners[3], right=corners[4] }
end

function make_segment(attributes, p, q)
  local seg = { type="segment", p, q }
  local sb = { type="curve", closed=false, seg }
  local obj = ipe.Path(attributes, { sb }, false)
  return obj
end

function make_bottom(attributes, p0, step, q, r, t)
  local l = ipe.LineThrough(p0, r)
  local p = p0
  local lambdas = {}
  for i=1,100 do
    p = V(p.x + step, p.y)
    local s = ipe.LineThrough(q, p)
    local x = l:intersects(s)
    local lambda = (x - p0):len() / (r - p0):len()
    if lambda > fillpart then return lambdas end
    lambdas[#lambdas+1] = lambda
    t[#t+1] = make_segment(attributes, p, q)
  end
  return lambdas
end

function add_verticals(attributes, t, bottom, top, q, lambdas)
  for i, lambda in ipairs(lambdas) do
    t[#t+1] = make_segment(attributes, bottom + lambda * (q - bottom),
			   top + lambda * (q - top))
  end
end

function two_point_perspective(model, num)
  local quad = collect_quadrilateral(model)
  if not quad then
    incorrect(model)
    return
  end
  local gs = model.snap.gridsize
  local fleft = {}   -- left front face
  local fright = {}  -- right front face
  fleft[#fleft+1] = make_segment(model.attributes, quad.bottom, quad.top)
  fright[#fright+1] = make_segment(model.attributes, quad.bottom, quad.top)
  fright[#fright+1] = make_segment(model.attributes, quad.bottom, quad.right)
  fleft[#fleft+1] = make_segment(model.attributes, quad.bottom, quad.left)
  fright[#fright+1] = make_segment(model.attributes, quad.top, quad.right)
  fleft[#fleft+1] = make_segment(model.attributes, quad.top, quad.left)
  local y = gs * math.floor( quad.bottom.y / gs ) + gs
  while y < quad.top.y do
    local p = V(quad.bottom.x, y)
    local r = p + fillpart * (quad.right - p)
    local l = p + fillpart * (quad.left - p)
    fright[#fright+1] = make_segment(model.attributes, p, r)
    fleft[#fleft+1] = make_segment(model.attributes, p, l)
    y = y + gs
  end
  local bleft = {}   -- left bottom face
  local bright = {}  -- right bottom face
  if quad.bottom.y < quad.right.y then
    local lambdas1 = make_bottom(model.attributes, quad.bottom, -gs,
				 quad.right, quad.left, bright)
    local lambdas2 = make_bottom(model.attributes, quad.bottom, gs,
				 quad.left, quad.right, bleft)
    add_verticals(model.attributes, fleft, quad.bottom, quad.top, quad.left, lambdas1)
    add_verticals(model.attributes, fright, quad.bottom, quad.top, quad.right, lambdas2)
  end
  local elements = { ipe.Group(fleft), ipe.Group(fright) }
  if #bleft > 0 then
    elements[#elements+1] = ipe.Group(bleft)
  end
  if #bright > 0 then
    elements[#elements+1] = ipe.Group(bright)
  end
  local in_layer = model:page():active(model.vno)
  if has_grid_layer(model:page(), true) then
    in_layer = "GRID"
  end
  local t = { label="two-point perspective grid",
	      pno = model.pno,
	      vno = model.vno,
	      layer = in_layer,
	      elements = elements,
  }
  t.undo = function (t, doc)
    local p = doc[t.pno]
    for i = 1,#t.elements do
      p:remove(#p)
    end
    p:ensurePrimarySelection()
  end
  t.redo = function (t, doc)
    local p = doc[t.pno]
    for i,obj in ipairs(t.elements) do
      p:insert(nil, obj, 2, t.layer)
    end
    p:ensurePrimarySelection()
  end
  model:register(t)
end

function make_pruned_segment(att, p, q, fs)
  local left = ipe.Line(V(0,0), V(0,1))
  local right = ipe.Line(V(fs.x, 0), V(0,1))
  if q.x < p.x then
    local s = p
    p = q
    q = s
  end
  local p1 = p
  local q1 = q
  if p.x < 0 then -- need left pruning
    local s = ipe.LineThrough(p, q):intersects(left)
    if s then p1 = s end
  end
  if q.x > fs.x then -- need right pruning
    local s = ipe.LineThrough(p, q):intersects(right)
    if s then q1 = s end
  end
  return make_segment(att, p1, q1)
end

function triangular_grid(model, num)
  local layout = model.doc:sheets():find("layout")
  local gs = model.snap.gridsize
  local fs = layout.framesize
  local t = {}
  local y = 0
  local dy = 0.5 * gs * math.sqrt(3)
  while y <= fs.y do
    t[#t+1] = make_pruned_segment(model.attributes, V(0, y), V(fs.x, y), fs)
    y = y + dy
  end
  local dx = fs.y / math.sqrt(3)
  local x = 0
  while x < fs.x do
    t[#t+1] = make_pruned_segment(model.attributes, V(x, 0), V(x + dx, fs.y), fs)
    x = x + gs
  end
  x = 0
  while x < fs.x + dx do
    t[#t+1] = make_pruned_segment(model.attributes, V(x, 0), V(x - dx, fs.y), fs)
    x = x + gs
  end
  x = -gs
  while x > -dx do
    t[#t+1] = make_pruned_segment(model.attributes, V(x, 0), V(x + dx, fs.y), fs)
    x = x - gs
  end
  local g = ipe.Group(t)
  local p = model:page()
  local in_layer = p:active(model.vno)
  if has_grid_layer(p, true) then
    in_layer = "GRID"
  end
  local t = { label="create triangular grid", pno=model.pno, vno=model.vno, layer=in_layer, object=g }
  t.undo = function (t, doc) doc[t.pno]:remove(#doc[t.pno]) end
  t.redo = function (t, doc)
    doc[t.pno]:deselectAll()
    doc[t.pno]:insert(nil, t.object, 1, t.layer)
  end
  model:register(t)
end

function create_layer(model, num)
  if has_grid_layer(model:page()) then
    model:warning("GRID layer already exists")
    return
  end
  local t = { label="create GRID layer",
	      pno = model.pno,
	      vno = model.vno,
  }
  t.redo = function (t, doc)
    local p = doc[t.pno]
    p:addLayer("GRID")
    p:setVisible(t.vno, "GRID", true)
  end
  t.undo = function (t, doc)
    local p = doc[t.pno]
    p:removeLayer("GRID")
  end
  model:register(t)
end

function hexagonal_grid(model, num)
  local layout = model.doc:sheets():find("layout")
  local gs = model.snap.gridsize
  local fs = layout.framesize
  local t = {}
  local segments = {}
  local columns = math.floor(fs.x / (1.5 * gs))
  local dy = 0.5 * gs * math.sqrt(3)
    for col = 0, columns + 1 do
        local x = col * 1.5 * gs
        local offset = 0
        if col % 2 == 1 then
            offset = dy
        end
        local rows = math.floor((fs.y - offset) / (2 * dy))
        for row = 0, rows + 1 do
            local y = offset + row * 2 * dy
            segments[#segments + 1] =  {V(x - gs, y), V(x - gs / 2, y - dy)}
            segments[#segments + 1] =  {V(x - gs / 2, y - dy), V(x + gs / 2, y - dy)}
            segments[#segments + 1] =  {V(x + gs / 2, y - dy), V(x + gs, y)}
        end
    end
    local frameRect = ipe.Rect()
    frameRect:add(V(0,0))
    frameRect:add(V(fs.x,fs.y))
    local function pruneVertical(a, b, fs)
        if b.y < a.y then a, b = b, a end
        if a.y < 0 then a = a + (-a.y / (b.y - a.y)) * (b - a) end
        if b.y > fs.y then b = a + ((fs.y - a.y) / (b.y - a.y)) * (b - a) end
        return a, b
    end
    for _, s in ipairs(segments) do
      local segRect = ipe.Rect()
      segRect:add(s[1])
      segRect:add(s[2])
      if segRect:intersects(frameRect) then
        local a, b = pruneVertical(s[1], s[2], fs)
        -- Stop zero length pieces
        if (b - a):len() > 1e-9 then
            local seg = make_pruned_segment(model.attributes, a, b, fs)
            local ends = seg:shape()[1][1]
            if (ends[2] - ends[1]):len() > 1e-9 then
                t[#t + 1] = seg
            end
        end
      end
    end
    local g = ipe.Group(t)
    local p = model:page()
    local in_layer = p:active(model.vno)
    if has_grid_layer(p, true) then
        in_layer = "GRID"
    end
    local t = { label="create hexagonal grid", pno=model.pno, vno=model.vno, layer=in_layer, object=g }
    t.undo = function (t, doc) doc[t.pno]:remove(#doc[t.pno]) end
    t.redo = function (t, doc)
    doc[t.pno]:deselectAll()
    doc[t.pno]:insert(nil, t.object, 1, t.layer)
    end
    model:register(t)
end

function polar_grid(model, num)
    local layout = model.doc:sheets():find("layout")
    local gs = model.snap.gridsize
    local fs = layout.framesize
    local center = V(fs.x / 2, fs.y / 2)
    local gridAngle = math.rad(model.snap.anglesize)
    local spokes = math.ceil(360/model.snap.anglesize)
    local maxIncircleRadius = math.min(fs.x, fs.y)/2
    local t = {}

    -- Make the circles
    local circles = math.floor(maxIncircleRadius / gs)
    if circles == 0 then
        model.ui:explain("grid size is too large for the page")
        -- Otherwise it makes a bunch of 0 length segments
        return
    end
    for k = 1, circles do
        local circleRadius = k * gs
        local circle = { type = "ellipse", ipe.Matrix(circleRadius, 0, 0, circleRadius, center.x, center.y) }
        local circleObject = ipe.Path(model.attributes, { circle })
        circleObject:set("pathmode", "stroked")
        t[#t + 1] = circleObject
    end

    -- Make the spokes, not using lines incase grid angle does not evenly divide 360
    local maximumSpokeLength = circles*gs
    for k = 0, spokes - 1 do
        local angle = gridAngle * k
        local endPoint = center + maximumSpokeLength * V(math.cos(angle), math.sin(angle))
        t[#t + 1] = make_segment(model.attributes, center, endPoint)
    end

    local g = ipe.Group(t)
    local p = model:page()
    local in_layer = p:active(model.vno)
    if has_grid_layer(p, true) then
        in_layer = "GRID"
    end
    local t = { label = "Create polar grid", pno = model.pno, vno = model.vno, layer = in_layer, object = g }
    t.undo = function(t, doc) doc[t.pno]:remove(#doc[t.pno]) end
    t.redo = function(t, doc)
        doc[t.pno]:deselectAll()
        doc[t.pno]:insert(nil, t.object, 1, t.layer)
    end
    model:register(t)
end

methods = {
  { label="Create a GRID layer", run = create_layer },
  { label="Two-point perspective grid", run = two_point_perspective },
  { label="Triangular grid", run = triangular_grid },
  { label="Hexagonal grid", run = hexagonal_grid },
  { label="Polar grid", run = polar_grid },
}

----------------------------------------------------------------------
