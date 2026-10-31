local cutter = { _G = _G, ipe = ipe, math = math, string = string,
                 table = table, assert = assert, ipairs = ipairs,
                 pairs = pairs, print = print, tonumber = tonumber }
assert(loadfile("src/ipelets/lua/cutter.lua", "t", cutter))()
assert(cutter.category == "Geometry", "incorrect ipelet category")

function revertOriginal(transaction, document)
  document:set(transaction.pno, transaction.original)
end

function revertFinal(transaction, document)
  document:set(transaction.pno, transaction.final)
end

local function geometries(shape, matrix)
  local result = {}
  for _, subpath in ipairs(shape) do
    if subpath.type == "ellipse" then
      result[#result + 1] = ipe.Arc(matrix * subpath[1])
    elseif subpath.type == "closedspline" then
      for _, bezier in ipairs(ipe.splineToBeziers(subpath, true)) do
        result[#result + 1] = ipe.Bezier(matrix * bezier[1], matrix * bezier[2],
                                        matrix * bezier[3], matrix * bezier[4])
      end
    else
      for _, segment in ipairs(subpath) do
        if segment.type == "segment" then
          local first, last = matrix * segment[1], matrix * segment[2]
          result[#result + 1] = ipe.Bezier(first, first + (1 / 3) * (last - first),
                                          first + (2 / 3) * (last - first), last)
        elseif segment.type == "arc" then
          result[#result + 1] = matrix * segment.arc
        else
          for _, bezier in ipairs(ipe.splineToBeziers(segment, false)) do
            result[#result + 1] = ipe.Bezier(matrix * bezier[1], matrix * bezier[2],
                                            matrix * bezier[3], matrix * bezier[4])
          end
        end
      end
      if subpath.closed then
        local first = matrix * subpath[1][1]
        local lastSegment = subpath[#subpath]
        local last = matrix * lastSegment[#lastSegment]
        if (first - last):sqLen() > 0 then
          result[#result + 1] = ipe.Bezier(last, last + (1 / 3) * (first - last),
                                          last + (2 / 3) * (first - last), first)
        end
      end
    end
  end
  return result
end

local function liesOn(point, curves)
  for _, curve in ipairs(curves) do
    local parameter, snapped = curve:snap(point)
    if parameter and curve.point then
      local low, high = math.max(0, parameter - 0.125), math.min(1, parameter + 0.125)
      for iteration = 1, 70 do
        local left, right = low + (high - low) / 3, high - (high - low) / 3
        if (curve:point(left) - point):sqLen() < (curve:point(right) - point):sqLen() then
          high = right
        else
          low = left
        end
      end
      snapped = curve:point(0.5 * (low + high))
    end
    if parameter and (point - snapped):len() < 0.0001 then return true end
  end
  return false
end

local function checkGeometry(source, target)
  for _, curve in ipairs(source) do
    for step = 0, 20 do
      local point
      if curve.point then
        point = curve:point(step / 20)
      else
        local alpha, beta = curve:angles()
        beta = ipe.normalizeAngle(beta, alpha)
        if curve:isEllipse() then beta = alpha + 2 * math.pi end
        local angle = alpha + step / 20 * (beta - alpha)
        point = curve:matrix() * ipe.Vector(math.cos(angle), math.sin(angle))
      end
      assert(liesOn(point, target), "cutting changed the geometry at " .. tostring(point))
    end
  end
end

local function test(name, shape, marks, expected, matrix, retainSpline)
  matrix = matrix or ipe.Matrix()
  local document = ipe.Document()
  local page = document[1]
  page:addLayer("pieces")
  page:setActive(1, "pieces")
  local path = ipe.Path({ stroke = "red", pen = 2, farrow = true }, shape, true)
  path:setMatrix(matrix)
  path:setCustom("cutter regression")
  page:insert(nil, path, 1, "alpha")
  for _, position in ipairs(marks) do
    local mark = ipe.Reference({}, "mark/disk(sx)", position)
    mark:setMatrix(matrix)
    page:insert(nil, mark, 2, "alpha")
  end
  local originalCount, before = #page, page:xml("ipepage")
  local originals = {}
  for index = 1, #page do originals[index] = page[index]:xml() end
  local model = { doc = document, pno = 1, vno = 1,
                  ui = { explain = function() end },
                  warning = function() end }
  function model:page() return self.doc[1] end
  function model:register(transaction)
    assert(not self.transaction, "multiple undo items")
    self.transaction = transaction
    transaction.redo(transaction, self.doc)
  end
  cutter.run(model, 1)
  page = document[1]
  assert(#page == originalCount + expected,
         name .. ": expected " .. expected .. " pieces, got " .. (#page - originalCount))
  local final = page:xml("ipepage")
  for index = 1, originalCount do
    assert(page[index]:xml() == originals[index],
           "an original object was changed")
    assert(page:layerOf(index) == "alpha", "an original object was moved")
  end
  local pieces, hasSpline = {}, false
  for index = originalCount + 1, #page do
    local piece = page[index]
    assert(piece:matrix() == matrix and piece:get("stroke") == "red"
           and piece:get("pen") == 2 and piece:get("farrow") == true
           and piece:getCustom() == "cutter regression", "attributes were lost")
    assert(page:layerOf(index) == "pieces", "piece was not inserted in the active layer")
    for _, geometry in ipairs(geometries(piece:shape(), matrix)) do
      pieces[#pieces + 1] = geometry
    end
    for _, segment in ipairs(piece:shape()[1]) do
      assert(#segment > 4 or (segment[1] - segment[#segment]):len() > 1e-8,
             "degenerate piece")
      if segment.type == "spline" and #segment > 4 then hasSpline = true end
    end
  end
  if expected > 0 then
    local original = geometries(shape, matrix)
    checkGeometry(original, pieces)
    checkGeometry(pieces, original)
    if retainSpline then assert(hasSpline, "unaffected spline spans were flattened") end
    local transaction = assert(model.transaction)
    transaction.undo(transaction, document)
    assert(document[1]:xml("ipepage") == before, "undo changed the original page")
    transaction.redo(transaction, document)
    assert(document[1]:xml("ipepage") == final, "redo changed the result")
  else
    assert(not model.transaction and before == final, "no-op changed the page")
  end
  print(name .. ": OK")
end

local vector = ipe.Vector
local line = { { type = "curve", closed = false,
                 { type = "segment", vector(0, 0), vector(100, 0) } } }
test("line, unordered and duplicate marks", line,
     { vector(70, 0), vector(30, 0), vector(30, 0), vector(0, 0), vector(100, 0) }, 3)
test("off-path marks and endpoints", line,
     { vector(0, 0), vector(100, 0), vector(30, 0.01) }, 0)
test("transformed line", line, { vector(30, 0), vector(70, 0) }, 3,
     ipe.Matrix(2, 0.5, -0.25, 3, 41, -17))

local spline = { type = "spline", vector(0, 0), vector(10, 35), vector(30, -20),
                 vector(50, 45), vector(70, -15), vector(90, 30), vector(110, 0) }
local intervals = ipe.splineToBeziers(spline, false)
local function point(interval, parameter)
  local bezier = intervals[interval]
  return ipe.Bezier(bezier[1], bezier[2], bezier[3], bezier[4]):point(parameter)
end
test("B-spline interior cut retains spline spans",
     { { type = "curve", closed = false, spline } }, { point(1, 0.4) }, 2, nil, true)
test("B-spline cut exactly at a knot",
  { { type = "curve", closed = false, spline } }, { point(2, 0) }, 2, nil, true)
test("B-spline multiple cuts and knots",
     { { type = "curve", closed = false, spline } },
     { point(3, 0.7), point(1, 0.4), point(3, 0.2), point(2, 0) }, 5)
test("transformed B-spline", { { type = "curve", closed = false, spline } },
     { point(2, 0.4), point(4, 0.2) }, 3, ipe.Matrix(2, 0.5, -0.25, 3, 41, -17))

local cubic = { type = "spline", vector(0, 0), vector(20, 50),
       vector(80, -30), vector(100, 0) }
local cubicGeometry = ipe.Bezier(cubic[1], cubic[2], cubic[3], cubic[4])
test("cubic Bezier", { { type = "curve", closed = false, cubic } },
  { cubicGeometry:point(0.2), cubicGeometry:point(0.8) }, 3)
local quadratic = { type = "spline", vector(0, 0), vector(50, 70), vector(100, 0) }
local quadraticGeometry = ipe.Quad(quadratic[1], quadratic[2], quadratic[3])
test("quadratic Bezier", { { type = "curve", closed = false, quadratic } },
  { quadraticGeometry:point(0.2), quadraticGeometry:point(0.8) }, 3)

local closedSpline = { type = "closedspline", vector(0, 0), vector(100, 0),
           vector(100, 100), vector(0, 100) }
local closedIntervals = ipe.splineToBeziers(closedSpline, true)
local function closedPoint(interval, parameter)
  local bezier = closedIntervals[interval]
  return ipe.Bezier(bezier[1], bezier[2], bezier[3], bezier[4]):point(parameter)
end
test("closed B-spline", { closedSpline },
  { closedPoint(1, 0.4), closedPoint(3, 0.6) }, 2)
test("closed B-spline at its seam", { closedSpline }, { closedPoint(4, 0) }, 1)

local polygon = { { type = "curve", closed = true,
                    { type = "segment", vector(0, 0), vector(100, 0) },
                    { type = "segment", vector(100, 0), vector(100, 100) },
                    { type = "segment", vector(100, 100), vector(0, 100) } } }
test("closed path including closing edge", polygon,
     { vector(20, 0), vector(0, 40), vector(100, 100) }, 3)
test("closed path cut at starting vertex", polygon, { vector(0, 0) }, 1)

local ellipseMatrix = ipe.Matrix(30, 0, 0, 20, 0, 0)
test("ellipse with multiple marks", { { type = "ellipse", ellipseMatrix } },
     { vector(30, 0), vector(0, -20), vector(-30, 0) }, 3)
local arc = ipe.Arc(ellipseMatrix, 2, -1)
local first, last = arc:endpoints()
test("wrapped arc", { { type = "curve", closed = false,
                        { type = "arc", arc = arc, first, last } } },
     { ellipseMatrix * vector(math.cos(3), math.sin(3)),
       ellipseMatrix * vector(math.cos(4), math.sin(4)) }, 3)

  local savedMarks = { vector(186.415, 732.117), vector(408.651, 660.759),
           vector(416.78, 660.127), vector(415.448, 668.689),
           vector(341.882, 776.538) }
  local savedVertices = { vector(128, 624), vector(144, 720), vector(368, 784),
              vector(432, 688), vector(336, 576), vector(304, 688),
              vector(224, 576) }
  local savedPolygon = { type = "curve", closed = true }
  for index = 1, #savedVertices - 1 do
    savedPolygon[#savedPolygon + 1] = { type = "segment", savedVertices[index],
                  savedVertices[index + 1] }
  end
  test("saved polygon with rounded marks", { savedPolygon }, savedMarks, 4)
  test("shared mark cuts the polygon", { savedPolygon }, { savedMarks[1] }, 1)
  local savedSpline = { type = "spline", vector(144, 800), vector(192, 688),
            vector(304, 720), vector(400, 640), vector(496, 672) }
  test("saved B-spline with rounded marks",
    { { type = "curve", closed = false, savedSpline } }, savedMarks, 4)
  test("shared mark cuts the spline",
    { { type = "curve", closed = false, savedSpline } }, { savedMarks[1] }, 2)
  local savedArc = ipe.Arc(ipe.Matrix(161.044, 0, 0, 161.044, 257.043, 639.652),
            vector(128, 736), vector(272, 800))
  test("saved arc with rounded marks and matrix",
    { { type = "curve", closed = false,
        { type = "arc", arc = savedArc, vector(128, 736), vector(272, 800) } } },
    savedMarks, 4)

local function testIntersections(name, paths, expected)
  local document = ipe.Document()
  local page = document[1]
  page:addLayer("marks")
  page:setActive(1, "marks")
  for _, entry in ipairs(paths) do
    local path = ipe.Path({}, entry.shape)
    path:setMatrix(entry.matrix or ipe.Matrix())
    page:insert(nil, path, 2, "alpha")
  end
  page:ensurePrimarySelection()
  local before, originals = page:xml("ipepage"), {}
  for index = 1, #page do originals[index] = page[index]:xml() end
  local model = { doc = document, pno = 1, vno = 1,
                  attributes = { markshape = "mark/cross(sx)", stroke = "red",
                                 symbolsize = 4 },
                  ui = { explain = function() end }, warning = function() end }
  function model:page() return self.doc[1] end
  function model:register(transaction)
    assert(not self.transaction, "multiple undo items")
    self.transaction = transaction
    transaction.redo(transaction, self.doc)
  end
  cutter.run(model, 2)
  page = document[1]
  assert(#page == #paths + #expected,
         name .. ": expected " .. #expected .. " marks, got " .. (#page - #paths))
  for index = 1, #paths do
    assert(page[index]:xml() == originals[index] and page:layerOf(index) == "alpha",
           "an original path was changed")
  end
  local found = {}
  for index = #paths + 1, #page do
    local mark = page[index]
    assert(mark:type() == "reference" and mark:symbol() == "mark/cross(sx)"
           and mark:get("stroke") == "red" and mark:get("symbolsize") == 4,
           "mark style was not preserved")
    assert(page:layerOf(index) == "marks", "mark was not inserted in the active layer")
    local position, match = mark:matrix() * mark:position(), nil
    for expectedIndex, point in ipairs(expected) do
      if (position - point):len() < 0.002 then match = expectedIndex break end
    end
    assert(match and not found[match], "unexpected or duplicate intersection mark")
    found[match] = true
  end
  if #expected > 0 then
    local after, transaction = page:xml("ipepage"), assert(model.transaction)
    transaction.undo(transaction, document)
    assert(document[1]:xml("ipepage") == before, "undo did not restore paths")
    transaction.redo(transaction, document)
    assert(document[1]:xml("ipepage") == after, "redo did not restore marks")
  else
    assert(not model.transaction and page:xml("ipepage") == before, "no-op changed page")
  end
  print(name .. ": OK")
end

local function linePath(first, last, matrix)
  return { shape = { { type = "curve", closed = false,
                      { type = "segment", first, last } } }, matrix = matrix }
end

testIntersections("crossing lines", {
  linePath(vector(0, 0), vector(100, 0)),
  linePath(vector(50, -50), vector(50, 50)),
}, { vector(50, 0) })
testIntersections("duplicate intersection from three paths", {
  linePath(vector(0, 0), vector(100, 0)),
  linePath(vector(50, -50), vector(50, 50)),
  linePath(vector(0, -50), vector(100, 50)),
}, { vector(50, 0) })
testIntersections("disjoint paths", {
  linePath(vector(0, 0), vector(100, 0)),
  linePath(vector(0, 20), vector(100, 20)),
}, {})
testIntersections("transformed paths", {
  linePath(vector(0, 0), vector(100, 0), ipe.Translation(0, 20)),
  linePath(vector(0, 0), vector(0, 100), ipe.Translation(40, 0)),
}, { vector(40, 20) })
testIntersections("ellipse and segment", {
  { shape = { { type = "ellipse", ellipseMatrix } } },
  linePath(vector(-50, 0), vector(50, 0)),
}, { vector(-30, 0), vector(30, 0) })
testIntersections("quadratic and segment", {
  { shape = { { type = "curve", closed = false, quadratic } } },
  linePath(vector(50, 0), vector(50, 100)),
}, { quadraticGeometry:point(0.5) })
testIntersections("polygon closing edge intersections", {
  { shape = polygon },
  linePath(vector(-20, 50), vector(120, 50)),
}, { vector(0, 50), vector(100, 50) })
testIntersections("B-spline and segment", {
  { shape = { { type = "curve", closed = false, spline } } },
  linePath(vector(point(2, 0.5).x, -50), vector(point(2, 0.5).x, 100)),
}, { point(2, 0.5) })
testIntersections("closed B-spline and segment", {
  { shape = { closedSpline } },
  linePath(vector(-20, 50), vector(120, 50)),
}, { closedPoint(1, 0.5), closedPoint(3, 0.5) })
local intersectionArc = ipe.Arc(ellipseMatrix, 0, math.pi)
local arcStart, arcEnd = intersectionArc:endpoints()
local straightBezier = { type = "spline", vector(0, -30), vector(0, -10),
                         vector(0, 10), vector(0, 30) }
testIntersections("arc and cubic Bezier", {
  { shape = { { type = "curve", closed = false,
                { type = "arc", arc = intersectionArc, arcStart, arcEnd } } } },
  { shape = { { type = "curve", closed = false, straightBezier } } },
}, { vector(0, 20) })
testIntersections("cubic and quadratic Beziers", {
  { shape = { { type = "curve", closed = false, straightBezier } } },
  { shape = { { type = "curve", closed = false,
                { type = "spline", vector(-20, 0), vector(0, 20), vector(20, 0) } } } },
}, { vector(0, 10) })
testIntersections("intersecting ellipses", {
  { shape = { { type = "ellipse", ipe.Matrix(20, 0, 0, 20, 0, 0) } } },
  { shape = { { type = "ellipse", ipe.Matrix(20, 0, 0, 20, 20, 0) } } },
}, { vector(10, math.sqrt(300)), vector(10, -math.sqrt(300)) })

local function testSplitPolyline(name, shape, answer, expected, matrix)
  matrix = matrix or ipe.Matrix()
  local document = ipe.Document()
  local page = document[1]
  page:addLayer("pieces")
  page:setActive(1, "pieces")
  local path = ipe.Path({ stroke = "red", pen = 2 }, shape)
  path:setMatrix(matrix)
  page:insert(nil, path, 1, "alpha")
  local other = ipe.Reference({}, "mark/disk(sx)", vector(200, 200))
  page:insert(nil, other, 2, "alpha")
  local before, otherXml = page:xml("ipepage"), page[2]:xml()
  local model = { doc = document, pno = 1, vno = 1,
                  ui = { explain = function() end }, warning = function() end }
  function model:page() return self.doc[1] end
  function model:getString() return answer end
  function model:register(transaction)
    assert(not self.transaction, "multiple undo items")
    self.transaction = transaction
    transaction.redo(transaction, self.doc)
  end
  cutter.run(model, 3)
  page = document[1]
  if expected == 0 then
    assert(not model.transaction and page:xml("ipepage") == before, "no-op changed page")
  else
    assert(#page == expected + 1, name .. ": incorrect number of pieces")
    assert(page[1]:xml() == otherXml, "another selected object was changed")
    local originalGeometry = geometries(shape, matrix)
    local originalLength = 0
    for _, geometry in ipairs(originalGeometry) do
      originalLength = originalLength + (geometry:point(1) - geometry:point(0)):len()
    end
    local pieces = {}
    for index = 2, #page do
      local piece, length = page[index], 0
      assert(piece:type() == "path" and piece:matrix():isIdentity()
             and piece:get("stroke") == "red" and piece:get("pen") == 2,
             "piece attributes were changed")
      assert(page:layerOf(index) == "alpha", "split polyline changed source layer")
      for _, geometry in ipairs(geometries(piece:shape(), piece:matrix())) do
        length = length + (geometry:point(1) - geometry:point(0)):len()
        pieces[#pieces + 1] = geometry
      end
      assert(math.abs(length - originalLength / expected) < 1e-7,
             "pieces do not have equal length")
    end
    checkGeometry(originalGeometry, pieces)
    checkGeometry(pieces, originalGeometry)
    local after, transaction = page:xml("ipepage"), assert(model.transaction)
    transaction.undo(transaction, document)
    assert(document[1]:xml("ipepage") == before, "undo did not restore polyline")
    transaction.redo(transaction, document)
    assert(document[1]:xml("ipepage") == after, "redo did not restore pieces")
  end
  print(name .. ": OK")
end

testSplitPolyline("split line into equal pieces", line, "4", 4)
local bentPolyline = { { type = "curve", closed = false,
                         { type = "segment", vector(0, 0), vector(30, 0) },
                         { type = "segment", vector(30, 0), vector(30, 70) } } }
testSplitPolyline("split polyline preserves vertices", bentPolyline, "4", 4)
testSplitPolyline("split closed polygon", polygon, "3", 3)
testSplitPolyline("split transformed polyline", bentPolyline, "3", 3,
                  ipe.Matrix(2, 0.5, -0.25, 3, 41, -17))
testSplitPolyline("cancel polyline splitting", line, nil, 0)
testSplitPolyline("reject invalid piece count", line, "1", 0)
testSplitPolyline("reject spline for polyline splitting",
                  { { type = "curve", closed = false, spline } }, "3", 0)

print("All cutter tests passed.")