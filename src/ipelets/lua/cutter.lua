label = "Cutter"
category = "Geometry"

about = [[
The first method cuts selected paths at selected marks, creating new pieces 
without changing the originals.  Marks must lie within 0.002 points of a path.
Cubic B-splines are subdivided by knot insertion, as in the path editor.

The second method marks intersections between selected paths.

The third method replaces the primary polyline with equal-length pieces.
]]

methods = {
  { label = "Cut paths at marks" },
  { label = "Mark path intersections" },
  { label = "Split polyline" },
}

local tolerance = 0.002
local parameterTolerance = 1e-8

local function near(first, second)
  return (first - second):sqLen() <= tolerance * tolerance
end

local function insertKnot(points, knots, knot)
  local span = 1
  while knots[span + 1] <= knot do span = span + 1 end
  local result = {}
  for index = 1, span - 3 do result[index] = points[index] end
  for index = span - 2, span do
    local alpha = (knot - knots[index]) / (knots[index + 3] - knots[index])
    result[index] = (1 - alpha) * points[index - 1] + alpha * points[index]
  end
  for index = span + 1, #points + 1 do result[index] = points[index - 1] end
  table.insert(knots, span + 1, knot)
  return result
end

local function splitSplineAtKnot(segment, knot)
  local count = #segment - 3
  local points = {}
  for index = 1, #segment do points[index] = segment[index] end
  local knots = { 0, 0, 0, 0 }
  for index = 1, count - 1 do knots[#knots + 1] = index end
  for index = 1, 4 do knots[#knots + 1] = count end
  points = insertKnot(points, knots, knot)
  points = insertKnot(points, knots, knot)
  local left, right = { type = "spline" }, { type = "spline" }
  for index = 1, knot + 3 do left[index] = points[index] end
  for index = knot + 3, #points do right[#right + 1] = points[index] end
  return left, right
end

local function splitSplineInterval(segment, interval)
  local count = #segment - 3
  local left, rest = { type = "spline", segment[1] }, segment
  if interval > 1 then
    left, rest = splitSplineAtKnot(segment, interval - 1)
  end
  local bezier, right = rest, { type = "spline", segment[#segment] }
  if interval < count then bezier, right = splitSplineAtKnot(rest, 1) end
  return left, bezier, right
end

local function openClosedSpline(subpath)
  local count = #subpath
  local first = (1 / 3) * (subpath[count] + 2 * subpath[1])
  local second = (1 / 3) * (2 * subpath[1] + subpath[2])
  local start = 0.5 * (first + second)
  local segment = { type = "spline", start, second }
  for index = 2, count do segment[#segment + 1] = subpath[index] end
  segment[#segment + 1] = first
  segment[#segment + 1] = start
  return { type = "curve", closed = true, segment }
end

local function subdivideBezier(segment, parameter)
  local points = {}
  for index = 1, #segment do points[index] = segment[index] end
  local left, right = { type = "spline" }, { type = "spline" }
  left[1], right[#points] = points[1], points[#points]
  for level = 1, #segment - 1 do
    for index = 1, #segment - level do
      points[index] = (1 - parameter) * points[index] + parameter * points[index + 1]
    end
    left[level + 1] = points[1]
    right[#segment - level] = points[#segment - level]
  end
  return left, right
end

local function append(target, segments)
  for _, segment in ipairs(segments) do target[#target + 1] = segment end
end

local function splitSegment(segment, parameter)
  if segment.type == "segment" then
    local point = (1 - parameter) * segment[1] + parameter * segment[2]
    return { { type = "segment", segment[1], point } },
      { { type = "segment", point, segment[2] } }
  elseif segment.type == "arc" then
    local alpha, beta = segment.arc:angles()
    beta = ipe.normalizeAngle(beta, alpha)
    local angle = alpha + parameter * (beta - alpha)
    local left = ipe.Arc(segment.arc:matrix(), alpha, angle)
    local right = ipe.Arc(segment.arc:matrix(), angle, beta)
    local point = right:endpoints()
    return { { type = "arc", arc = left, segment[1], point } },
      { { type = "arc", arc = right, point, segment[2] } }
  elseif #segment <= 4 then
    local left, right = subdivideBezier(segment, parameter)
    return { left }, { right }
  else
    local knot = math.floor(parameter + 0.5)
    if knot > 0 and knot < #segment - 3
      and math.abs(parameter - knot) < parameterTolerance then
      local left, right = splitSplineAtKnot(segment, knot)
      return { left }, { right }
    end
    local interval = math.min(math.floor(parameter) + 1, #segment - 3)
    local before, bezier, after = splitSplineInterval(segment, interval)
    local left, right = subdivideBezier(bezier, parameter - interval + 1)
    local leftSegments, rightSegments = {}, { right }
    if #before > 1 then leftSegments[1] = before end
    leftSegments[#leftSegments + 1] = left
    if #after > 1 then rightSegments[#rightSegments + 1] = after end
    return leftSegments, rightSegments
  end
end

local function snapBezier(bezier, mark)
  local parameter = bezier:snap(mark)
  if not parameter then return end
  local first, second, third, last = bezier:controlpoints()
  for iteration = 1, 30 do
    local complement = 1 - parameter
    local derivative = 3 * complement * complement * (second - first)
      + 6 * complement * parameter * (third - second)
      + 3 * parameter * parameter * (last - third)
    local secondDerivative = 6 * complement * (third - 2 * second + first)
      + 6 * parameter * (last - 2 * third + second)
    local difference = bezier:point(parameter) - mark
    local denominator = derivative:sqLen() + (difference ^ secondDerivative)
    if math.abs(denominator) < 1e-20 then break end
    local nextParameter = math.max(0, math.min(1,
      parameter - (difference ^ derivative) / denominator))
    if math.abs(nextParameter - parameter) < 1e-12 then
      parameter = nextParameter
      break
    end
    parameter = nextParameter
  end
  return parameter, bezier:point(parameter)
end

local function findCut(curve, matrix, mark)
  for index, segment in ipairs(curve) do
    if (index > 1 or curve.closed) and near(matrix * segment[1], mark) then
      return index, 0
    end
    if index < #curve and near(matrix * segment[#segment], mark) then
      return index + 1, 0
    end
    if segment.type == "segment" then
      local first, last = matrix * segment[1], matrix * segment[2]
      local direction = last - first
      if direction:sqLen() > 0 then
        local parameter = ((mark - first) ^ direction) / direction:sqLen()
        if parameter > parameterTolerance and parameter < 1 - parameterTolerance
          and not near(first, mark) and not near(last, mark)
          and near(first + parameter * direction, mark) then
          return index, parameter
        end
      end
    elseif segment.type == "arc" then
      local alpha, beta = segment.arc:angles()
      beta = ipe.normalizeAngle(beta, alpha)
      local arc = ipe.Arc(matrix * segment.arc:matrix(), alpha, beta)
      local angle, point = arc:snap(mark)
      if angle and near(point, mark) then
        angle = ipe.normalizeAngle(angle, alpha)
        local parameter = (angle - alpha) / (beta - alpha)
        if parameter > parameterTolerance and parameter < 1 - parameterTolerance
          and not near(matrix * segment[1], mark)
          and not near(matrix * segment[2], mark) then
          return index, parameter
        end
      end
    else
      local beziers
      if #segment == 3 then
        beziers = { { segment[1], segment[2], segment[3] } }
      elseif #segment == 4 then
        beziers = { segment }
      else
        beziers = ipe.splineToBeziers(segment, false)
      end
      for interval, bezier in ipairs(beziers) do
        local geometry
        if #bezier == 3 then
          geometry = ipe.Quad(matrix * bezier[1], matrix * bezier[2],
                              matrix * bezier[3])
        else
          geometry = ipe.Bezier(matrix * bezier[1], matrix * bezier[2],
                                matrix * bezier[3], matrix * bezier[4])
        end
        local parameter, point = snapBezier(geometry, mark)
        if parameter and near(point, mark) then
          local knot = interval - 1 + parameter
          if knot > parameterTolerance and knot < #beziers - parameterTolerance
            and not near(matrix * segment[1], mark)
            and not near(matrix * segment[#segment], mark) then
            return index, knot
          end
        end
      end
    end
  end
end

local function splitCurve(curve, index, parameter)
  local left, right = { type = "curve", closed = false },
    { type = "curve", closed = false }
  for previous = 1, index - 1 do left[#left + 1] = curve[previous] end
  if parameter == 0 then
    right[1] = curve[index]
  else
    local leftSegments, rightSegments = splitSegment(curve[index], parameter)
    append(left, leftSegments)
    append(right, rightSegments)
  end
  for following = index + 1, #curve do right[#right + 1] = curve[following] end
  if curve.closed then
    append(right, left)
    return { right }
  end
  return { left, right }
end

local function prepareCurve(subpath)
  local curve
  if subpath.type == "closedspline" then
    curve = openClosedSpline(subpath)
  else
    curve = { type = "curve", closed = subpath.closed }
    for _, segment in ipairs(subpath) do
      if segment.type == "oldspline" or segment.type == "cardinal"
        or segment.type == "spiro" then
        append(curve, ipe.splineToBeziers(segment, false))
      else
        curve[#curve + 1] = segment
      end
    end
  end
  if curve.closed then
    local first, last = curve[1][1], curve[#curve][#curve[#curve]]
    if (first - last):sqLen() > 0 then
      curve[#curve + 1] = { type = "segment", last, first }
    end
  end
  return curve
end

local function cutEllipse(subpath, matrix, marks)
  local ellipse = ipe.Arc(matrix * subpath[1])
  for _, mark in ipairs(marks) do
    local angle, point = ellipse:snap(mark)
    if angle and near(point, mark) then
      local first = ipe.Arc(subpath[1], angle, angle + math.pi)
      local second = ipe.Arc(subpath[1], angle + math.pi, angle + 2 * math.pi)
      local start, middle = first:endpoints()
      return { type = "curve", closed = false,
               { type = "arc", arc = first, start, middle },
               { type = "arc", arc = second, middle, start } }
    end
  end
end

local function cutSubpath(subpath, matrix, marks)
  local curve, changed
  if subpath.type == "ellipse" then
    curve = cutEllipse(subpath, matrix, marks)
    if not curve then return {} end
    changed = true
  else
    curve = prepareCurve(subpath)
  end
  local pieces = { curve }
  for _, mark in ipairs(marks) do
    local index = 1
    while index <= #pieces do
      local segmentIndex, parameter = findCut(pieces[index], matrix, mark)
      if segmentIndex then
        local replacements = splitCurve(pieces[index], segmentIndex, parameter)
        table.remove(pieces, index)
        for offset = #replacements, 1, -1 do
          table.insert(pieces, index, replacements[offset])
        end
        changed = true
      else
        index = index + 1
      end
    end
  end
  return changed and pieces or {}
end

local function pathGeometry(path)
  local matrix = path:matrix()
  local result = {}
  for _, subpath in ipairs(path:shape()) do
    if subpath.type == "ellipse" then
      result[#result + 1] = { type = "arc", geometry = ipe.Arc(matrix * subpath[1]) }
    else
      for _, segment in ipairs(prepareCurve(subpath)) do
        if segment.type == "segment" then
          result[#result + 1] = { type = "segment",
            geometry = ipe.Segment(matrix * segment[1], matrix * segment[2]) }
        elseif segment.type == "arc" then
          result[#result + 1] = { type = "arc", geometry = matrix * segment.arc }
        else
          for _, bezier in ipairs(ipe.splineToBeziers(segment, false)) do
            result[#result + 1] = { type = "bezier",
              geometry = ipe.Bezier(matrix * bezier[1], matrix * bezier[2],
                                    matrix * bezier[3], matrix * bezier[4]) }
          end
        end
      end
    end
  end
  return result
end

local function intersections(first, second)
  if first.type == "arc" then
    return first.geometry:intersect(second.geometry)
  elseif second.type == "arc" then
    return second.geometry:intersect(first.geometry)
  elseif first.type == "bezier" then
    return first.geometry:intersect(second.geometry)
  elseif second.type == "bezier" then
    return second.geometry:intersect(first.geometry)
  else
    local point = first.geometry:intersects(second.geometry)
    return point and { point } or {}
  end
end

local function markIntersections(model)
  local page = model:page()
  local paths = {}
  for index, object, selected in page:objects() do
    if selected and object:type() == "path" then
      paths[#paths + 1] = pathGeometry(object)
    end
  end
  if #paths < 2 then
    model:warning("Select at least two paths.")
    return
  end
  local points = {}
  for first = 1, #paths - 1 do
    for second = first + 1, #paths do
      for _, firstGeometry in ipairs(paths[first]) do
        for _, secondGeometry in ipairs(paths[second]) do
          for _, point in ipairs(intersections(firstGeometry, secondGeometry)) do
            local duplicate = false
            for _, existing in ipairs(points) do
              if near(point, existing) then duplicate = true break end
            end
            if not duplicate then points[#points + 1] = point end
          end
        end
      end
    end
  end
  if #points == 0 then
    model.ui:explain("No intersections between the selected paths.")
    return
  end
  local transaction = { label = "mark path intersections",
                        pno = model.pno, vno = model.vno,
                        original = page:clone(), final = page:clone(),
                        undo = _G.revertOriginal, redo = _G.revertFinal }
  local layer = page:active(model.vno)
  transaction.final:deselectAll()
  for _, point in ipairs(points) do
    local mark = ipe.Reference(model.attributes, model.attributes.markshape, point)
    transaction.final:insert(nil, mark, 2, layer)
  end
  transaction.final:ensurePrimarySelection()
  model:register(transaction)
end

local function splitPolyLine(model)
  -- Get the data from the page and the user
  local str = model:getString("Enter number of pieces")
  if not str or str:match("^%s*$)") then return end
  local k = tonumber(str)
  if not k or k < 2 or k > 1000 or k ~= math.floor(k) then
    model:warning("Enter a number between 2 and 1000!")
    return
  end
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local selection = p[prim]
  if selection:type() ~= "path" or #selection:shape() ~= 1 then
    model:warning("Cannot split polyline", "Primary selection is not a continuous polyline")
    return
  end
  local m = selection:matrix()
  local polyLine = selection:shape()[1]

  -- We get the vertices, and the distance elapsed at each vertex
  local lengthElapsed = { 0 }
  local vertices = {}
  for i = 2, #polyLine + 1 do
    local segment = polyLine[i-1]
    if segment.type ~= "segment" then
      model:warning("Cannot split polyline", "The path must contain only straight line segments")
      return
    end
    local start = m * segment[1]
    vertices[i-1] = start
    local finish = m * segment[2]
    lengthElapsed[i]=(finish - start):len()+lengthElapsed[i-1]
  end
  vertices[#polyLine + 1] = m * polyLine[#polyLine][2]

  -- If it's a polygon, we have to add the start again
  if polyLine.closed then
    vertices[#vertices + 1] = vertices[1]
    lengthElapsed[#vertices] = lengthElapsed[#vertices - 1] + (vertices[1] - vertices[#vertices - 1]):len()
  end

  -- Point along a segment of a certain length from the start (a)
  local function directionAlongSegment(a, b, x)
    return a + x * (b - a):normalized()
  end

  local splitLength = lengthElapsed[#vertices] / k
  local splitVertices = { vertices[1] }

  -- Find where we need to put new vertices, the ones we split at
  local elapsed = 0
  local i = 1
  for j = 1, k - 1 do
    elapsed = elapsed + splitLength
    while lengthElapsed[i + 1] < elapsed do
      i = i + 1
    end
    splitVertices[j+1] = directionAlongSegment(vertices[i], vertices[i + 1], elapsed - lengthElapsed[i])
  end
  splitVertices[k + 1] = vertices[#vertices]

  -- Each piece runs from one split vertex to the next, original vertices are kept
  local pieces = {}
  local v = 2
  for j = 1, #splitVertices - 1 do
    local piece = {splitVertices[j]}
    while v < #vertices and lengthElapsed[v] < j * splitLength do
      -- skip duplicate points
      if (vertices[v] - piece[#piece]):len() > 1e-9 then
        piece[#piece + 1] = vertices[v]
      end
      v = v + 1
    end
    -- skip duplicate points
    if (splitVertices[j+1] - piece[#piece]):len() > 1e-9 then
      piece[#piece+1] = splitVertices[j+1]
    end
    pieces[#pieces+1] = piece
  end

  -- Draw the pieces
  local objs = {}
  for _, piece in ipairs(pieces) do
    local curve = { type = "curve", closed = false }
    for v = 1, #piece - 1 do
      curve[#curve+1] = { type = "segment", piece[v], piece[v+1] }
    end
    local obj = selection:clone()
    obj:setShape({ curve })
    obj:setMatrix(ipe.Matrix())
    objs[#objs+1] = obj
  end

  -- Replace the original
  local t = { label = "split polyline", pno = model.pno, vno = model.vno,
        original = p:clone(), undo = _G.revertOriginal }
  local layer = p:layerOf(prim)
  t.redo = function (t, doc)
    local page = doc[t.pno]
    page:remove(prim)
    for _, obj in ipairs(objs) do page:insert(nil, obj:clone(), nil, layer) end
  end
  model:register(t)
end

function run(model, method)
  if method == 2 then markIntersections(model) return end
  if method == 3 then splitPolyLine(model) return end
  local page = model:page()
  local layer = page:active(model.vno)
  local marks, paths = {}, {}
  for index, object, selected in page:objects() do
    if selected then
      if object:type() == "path" then
        paths[#paths + 1] = object
      elseif object:type() == "reference" and object:symbol():sub(1, 5) == "mark/" then
        marks[#marks + 1] = object:matrix() * object:position()
      end
    end
  end
  if #marks == 0 or #paths == 0 then
    model:warning("Select at least one path and at least one mark.")
    return
  end
  local objects = {}
  for _, path in ipairs(paths) do
    for _, subpath in ipairs(path:shape()) do
      for _, piece in ipairs(cutSubpath(subpath, path:matrix(), marks)) do
        local object = path:clone()
        object:setShape({ piece })
        objects[#objects + 1] = object
      end
    end
  end
  if #objects == 0 then
    model.ui:explain("No selected marks cut the selected paths.")
    return
  end
  local transaction = { label = "cut paths at marks",
                        pno = model.pno, vno = model.vno,
                        original = page:clone(), final = page:clone(),
                        undo = _G.revertOriginal, redo = _G.revertFinal }
  transaction.final:deselectAll()
  for _, object in ipairs(objects) do
    transaction.final:insert(nil, object, 2, layer)
  end
  transaction.final:ensurePrimarySelection()
  model:register(transaction)
end