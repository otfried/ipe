----------------------------------------------------------------------
-- Euclid ipelet
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

label = "Euclidean Geometry"

about = [[
Create the incircle or the three excircles of a triangle.

Original Ipe 6 version by Jari Lappalainen, this ipelet is now part of
Ipe.
]]

function incorrect(model)
  model:warning("Primary selection is not a triangle")
end

function collect_vertices(model)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end

  local obj = p[prim]
  if obj:type() ~= "path" then incorrect(model) return end

  local shape = obj:shape()
  if (#shape ~= 1 or shape[1].type ~= "curve" or #shape[1] ~= 2
      or shape[1][1].type ~= "segment" or shape[1][2].type ~= "segment")
  then
    incorrect(model)
    return
  end

  local m = obj:matrix()
  local a = m * shape[1][1][1]
  local b = m * shape[1][1][2]
  local c = m * shape[1][2][2]
  return a, b, c
end

function angle_bisector(origin, dir1, dir2)
  assert(dir1:sqLen() > 0)
  assert(dir2:sqLen() > 0)
  local bisector = dir1:normalized() + dir2:normalized()
  if bisector:sqLen() == 0 then bisector = dir1:orthogonal() end
  return ipe.LineThrough(origin, origin + bisector)
end

function create_circle(model, center, radius)
  local shape =  { type="ellipse";
		   ipe.Matrix(radius, 0, 0, radius, center.x, center.y) }
  return ipe.Path(model.attributes, { shape } )
end

function incircle(model, a, b, c)
  local b1 = angle_bisector(a, b - a, c - a)
  local b2 = angle_bisector(b, c - b, a - b)
  local center = b1:intersects(b2)
  if (center) then
    local AB = ipe.LineThrough(a, b)
    local radius = AB:distance(center)
    return create_circle(model, center, radius)
  end
end

function excircle(model, a, b, c)
  local b1 = angle_bisector(a, b - a, c - a)
  local b2 = angle_bisector(b, c - b, a - b)
  local n1 = b1:normal()
  local n2 = b2:normal()
  local nl1 = ipe.LineThrough(a, a + n1)
  local nl2 = ipe.LineThrough(b, b + n2)
  local center = nl1:intersects(nl2)
  if center then
    local AB = ipe.LineThrough(a, b)
    local radius = AB:distance(center)
    return create_circle(model, center, radius)
  end
end

function create_incircle(model)
  local a, b, c = collect_vertices(model)
  if not a then return end

  local obj = incircle(model, a, b, c)
  if obj then
    model:creation("create incircle of triangle", obj)
  end
end

function create_excircles(model)
  local a, b, c = collect_vertices(model)
  if not a then return end

  local circles = {}
  local obj = excircle(model, a, b, c)
  if obj then circles[#circles + 1] = obj end
  obj = excircle(model, b, c, a)
  if obj then circles[#circles + 1] = obj end
  obj = excircle(model, c, a, b)
  if obj then circles[#circles + 1] = obj end

  local group = ipe.Group(circles)
  model:creation("create excircles of triangles", group)
end

methods = {
  { label="Incircle of triangle", run = create_incircle },
  { label="Excircles of triangle", run = create_excircles },
}

----------------------------------------------------------------------
