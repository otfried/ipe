----------------------------------------------------------------------
-- goodies ipelet
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

label = "Goodies"

revertOriginal = _G.revertOriginal

about = [[
Several small functions, like precise scale and rotate, precise
boxes, marking circle centers, regular k-gons.

This ipelet is part of Ipe.
]]

V = ipe.Vector

local function bounding_box(p)
  local box = ipe.Rect()
  for i,obj,sel,layer in p:objects() do
    if sel then box:add(p:bbox(i)) end
  end
  return box
end

local function boxshape(v1, v2)
  return { type="curve", closed=true;
	   { type="segment"; v1, V(v1.x, v2.y) },
	   { type="segment"; V(v1.x, v2.y), v2 },
	   { type="segment"; v2, V(v2.x, v1.y) } }
end

function preciseTransform(model, num)
  local p = model:page()
  if not p:hasSelection() then
    model.ui:explain("no selection")
    return
  end

  if (num==3 or num==4) and not model.snap.with_axes then
    model:warning("Cannot mirror at axis", "The coordinate system has not been set")
    return
  end

  -- check pinned
  for i, obj, sel, layer in p:objects() do
    if sel and obj:get("pinned") ~= "none" then
      model:warning("Cannot transform objects",
		    "At least one of the objects is pinned")
      return
    end
  end

  local matrix
  local label = methods[num].label
  if num == 1 then  -- mirror horizontal
    matrix = ipe.Matrix(-1, 0, 0, 1, 0, 0)
  elseif num == 2 then -- Mirror vertical
    matrix = ipe.Matrix(1, 0, 0, -1, 0, 0)
  elseif num == 3 then -- Mirror at x-axis
    matrix = (ipe.Rotation(model.snap.orientation)
	    * ipe.Matrix(1, 0, 0, -1, 0, 0)
	  * ipe.Rotation(-model.snap.orientation))
  elseif num == 4 then -- Mirror at y-axis
    matrix = (ipe.Rotation(model.snap.orientation)
	    * ipe.Matrix(-1, 0, 0, 1, 0, 0)
	  * ipe.Rotation(-model.snap.orientation))
  elseif num == 5 then   -- turn 90 degrees
    matrix = ipe.Matrix(0, 1, -1, 0, 0, 0)
  elseif num == 6 then   -- turn 180 degrees
    matrix = ipe.Matrix(-1, 0, 0, -1, 0, 0)
  elseif num == 7 then   -- turn 270 degrees
    matrix = ipe.Matrix(0, -1, 1, 0, 0, 0)
  elseif num == 8 then   -- rotate by angle
    local str = model:getString("Enter angle in degrees")
    if not str or str:match("^%s*$") then return end
    local degrees = tonumber(str)
    if not degrees then
      model:warning("Please enter angle in degrees")
      return
    end
    matrix = ipe.Rotation(math.pi * degrees / 180.0)
    label = "rotation by " .. degrees .. " degrees"
  elseif num == 9 then   -- stretch or scale
    local str = model:getString("Enter stretch factors")
    if not str or str:match("^%s*$") then return end
    if str:match("^[%+%-%d%.]+$") then
      local sx = tonumber(str)
      if sx == 0 then
	model:warning("Illegal scale factor",
		      "You cannot use a zero scale factor")
	return
      end
      label = "scale by " .. sx
      matrix = ipe.Matrix(sx, 0, 0, sx, 0, 0)
    else
      local ssx, ssy = str:match("^([%+%-%d%.]+)%s+([%+%-%d%.]+)$")
      if not ssx then
	model:warning("Please enter numeric stretch factors",
		      "You can either enter a single number to scale the object"
			.. " or two numbers to stretch in x and y directions.")
	return
      end
      local sx, sy = tonumber(ssx), tonumber(ssy)
      if sx == 0 or sy == 0 then
	model:warning("Illegal stretch factor",
		      "You cannot use a zero stretch factor")
	return
      end
      label = "stretch by " .. sx .. ", " .. sy
      matrix = ipe.Matrix(sx, 0, 0, sy, 0, 0)
    end
    if model.snap.with_axes then
      matrix = (ipe.Rotation(model.snap.orientation)
	      * matrix
	      * ipe.Rotation(-model.snap.orientation))
    end
  end

  local origin
  if model.snap.with_axes then
    origin = model.snap.origin
  else
    local box = bounding_box(p)
    origin = 0.5 * (box:bottomLeft() + box:topRight())
  end

  matrix = ipe.Translation(origin) * matrix * ipe.Translation(-origin)

  local t = { label = label,
	      pno = model.pno,
	      vno = model.vno,
	      selection = model:selection(),
	      original = model:page():clone(),
	      matrix = matrix,
	      undo = revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do p:transform(i, t.matrix) end
	   end
  model:register(t)
end

function rotateAxis(model, num)
  if not model.snap.with_axes then
    model:warning("Cannot rotate coordinate system", "The coordinate system has not been set")
    return
  end
  local str = model:getString("Enter angle in degrees")
  if not str or str:match("^%s*$") then return end
  local degrees = tonumber(str)
  if not degrees then
    model:warning("Please enter angle in degrees")
    return
  end
  local rad = math.pi * degrees / 180.0
  model.snap.orientation = model.snap.orientation + rad
  if model.snap.orientation >= 2 * math.pi then
    model.snap.orientation = model.snap.orientation - 2 * math.pi
  elseif model.snap.orientation < 0 then
    model.snap.orientation = model.snap.orientation + 2 * math.pi
  end
  model:setSnap()
end

function preciseBox(model)
  local dpmm = 72.0 / 25.4
  local str = model:getString("Enter width and height in mm")
  if not str or str:match("^%s*$") then return end
  local ssx, ssy = str:match("^([%+%-%d%.]+)%s+([%+%-%d%.]+)$")
  if not ssx then
    model:warning("Please enter selfwidth and height in mm",
		  "Separate the two numbers by a space.")
    return
  end
  local sx, sy = tonumber(ssx), tonumber(ssy)
  local corner = V(sx * dpmm, sy * dpmm)
  local origin = V(0,0)
  if model.snap.with_axes then
    origin = model.snap.origin
  end
  local shape = { boxshape(origin, origin + corner) }
  local obj = ipe.Path(model.attributes, shape)
  model:creation("create precise box", obj)
end

function boundingBox(model)
  if not model:page():hasSelection() then
    model.ui:explain("no selection")
    return
  end
  local box = bounding_box(model:page())
  local shape = { boxshape(box:bottomLeft(), box:topRight()) }
  local obj = ipe.Path(model.attributes, shape)
  model:creation("create bounding box", obj)
end

function mediaBox(model)
  local layout = model.doc:sheets():find("layout")
  local shape = { boxshape(-layout.origin, -layout.origin + layout.papersize) }
  local obj = ipe.Path(model.attributes, shape)
  model:creation("create mediabox", obj)
end

function checkPrimaryIsCircle(model, arc_ok)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local obj = p[prim]
  if obj:type() == "path" then
    local shape = obj:shape()
    if #shape == 1 then
      local s = shape[1]
      if s.type == "ellipse" then
	return prim, obj, s[1]:translation(), shape
      end
      if arc_ok and s.type == "curve" and #s == 1 and s[1].type == "arc" then
	return prim, obj, s[1].arc:matrix():translation(), shape
      end
    end
  end
  if arc_ok then
    model:warning("Primary selection is not an arc, a circle, or an ellipse")
  else
    model:warning("Primary selection is not a circle or an ellipse")
  end
end

function markCircleCenter(model)
  local prim, obj, pos, shape = checkPrimaryIsCircle(model, true)
  if not prim then return end
  local obj = ipe.Reference(model.attributes, model.attributes.markshape,
			    obj:matrix() * pos)
  model:creation("mark circle center", obj)
end

local function incorrect_input(model)
  model:warning("Cannot create parabolas",
		"Primary selection is not a segment, or " ..
		  "other selected objects are not marks")
end

function parabola(model)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local seg = p[prim]
  if seg:type() ~= "path" then incorrect_input(model) return end
  local shape = seg:shape()
  if #shape ~= 1 or shape[1].type ~= "curve" or
    #shape[1] ~= 1 or shape[1][1].type ~= "segment" then
    incorrect_input(model)
    return
  end

  local marks = {}
  for i,obj,sel,layer in p:objects() do
    if sel == 2 then
      if obj:type() ~= "reference" or obj:symbol():sub(1,5) ~= "mark/" then
	incorrect_input(model)
	return
      end
      marks[#marks + 1] = obj:matrix() * obj:position()
    end
  end

  if #marks == 0 then incorrect_input(model) return end

  local p0 = seg:matrix() * shape[1][1][1]
  local p1 = seg:matrix() * shape[1][1][2]

  local tfm = ipe.Translation(p0) * ipe.Rotation((p1 - p0):angle())
  local inv = tfm:inverse()
  -- treat x-interval from 0 to xmax
  local xmax = (p1 - p0):len()

  local parabolas = { }
  for i,pos in ipairs(marks) do
    local mrk = inv * pos
    local a = -mrk.x
    local b = xmax - mrk.x

    -- the following are the three control points for the unit
    -- parabola between x = a and x = b
    local q0 = V(a, a*a)
    local q1 = V(0.5*(a + b), a*b)
    local q2 = V(b, b*b)

    local curve = { type="curve", closed=false; { type="spline", q0, q1, q2 } }

    local obj = ipe.Path(model.attributes, { curve } )
    local stretch = 2.0 * mrk.y;
    local offs = V(mrk.x, mrk.y / 2.0)
    local m = (tfm * ipe.Translation(offs) *
	     ipe.Matrix(1, 0, 0, 1.0/stretch, 0, 0))
    obj:setMatrix(m);
    parabolas[#parabolas + 1] = obj
  end

  if #parabolas > 1 then
    local obj = ipe.Group(parabolas)
    model:creation("create parabolas", obj)
  else
    model:creation("create parabola", parabolas[1])
  end
end

function regularKGon(model)
  local prim, obj, pos, shape = checkPrimaryIsCircle(model, false)
  if not prim then return end

  local str = model:getString("Enter number of corners")
  if not str or str:match("^%s*$)") then return end
  local k = tonumber(str)
  if not k then
    model:warning("Enter a number between 3 and 1000!")
    return
  end

  local m = shape[1][1]
  local center = m:translation()
  local v = m * V(1,0)
  local radius = (v - center):len()

  local curve = { type="curve", closed=true }
  local alpha = 2 * math.pi / k
  local v0 = center + radius * V(1,0)
  for i = 1,k-1 do
    local v1 = center + radius * ipe.Direction(i * alpha)
    curve[#curve + 1] = { type="segment", v0, v1 }
    v0 = v1
  end

  local kgon = ipe.Path(model.attributes, { curve } )
  kgon:setMatrix(obj:matrix())
  model:creation("create regular k-gon", kgon)
end

methods = {
  { label = "Mirror horizontal", run=preciseTransform },
  { label = "Mirror vertical", run=preciseTransform },
  { label = "Mirror at x-axis", run=preciseTransform },
  { label = "Mirror at y-axis", run=preciseTransform },
  { label = "Turn 90 degrees", run=preciseTransform },
  { label = "Turn 180 degrees", run=preciseTransform },
  { label = "Turn 270 degrees", run=preciseTransform },
  { label = "Precise rotate", run=preciseTransform },
  { label = "Precise stretch", run=preciseTransform },
  { label = "Rotate coordinate system", run=rotateAxis },
  { label = "Insert precise box", run=preciseBox },
  { label = "Insert bounding box", run=boundingBox },
  { label = "Insert media box", run=mediaBox },
  { label = "Mark circle center", run=markCircleCenter },
  { label = "Make parabolas", run=parabola },
  { label = "Regular k-gon", run=regularKGon },
}

----------------------------------------------------------------------
