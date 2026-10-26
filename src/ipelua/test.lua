if not ipe then
  if arg and arg[1] then
    assert(package.loadlib(arg[1], "luaopen_ipe"))()
  else
    require("ipe")
  end
end

local tests, failures, knownFailures = 0, 0, 0
local vector = ipe.Vector
local pi = math.pi
local covered, registered, originals, excluded = {}, {}, {}, {}
local exclusions = {
  ipe = {
    fileFormat = "reads a file", fileExists = "queries the file system",
    realPath = "resolves a file-system path", mkdir = "creates a directory",
    directory = "reads a directory", openFile = "opens a file", readImage = "reads a file",
    Ipelet = "loads a native library; its run method also requires an editor helper",
  },
  object = {
    savePixels = "writes a file",
    saveImage = "writes a file",
  },
  document = {
    save = "writes a file", exportPages = "writes files", exportView = "writes files",
    runLatex = "uses files and an external process",
    prepareLatexRun = "writes LaTeX source files",
    howToRunLatex = "reads the LaTeX cache configuration",
    completeLatexRun = "reads LaTeX output files",
  },
}

local function trackMethods(owner, methods)
  for name, implementation in pairs(methods) do
    if type(implementation) == "function" then
      local key = owner .. "." .. name
      local reason = exclusions[owner] and exclusions[owner][name]
      if name == "__gc" then
        excluded[key] = "exercised by automatic garbage collection, not called manually"
      else
        originals[#originals + 1] = {methods, name, implementation}
        if reason then
          excluded[key] = reason
          methods[name] = function() error("file-system API excluded from tests: " .. key) end
        else
          registered[key] = true
          methods[name] = function(...)
            covered[key] = true
            if owner == "ipe" then
              if name == "Document" then assert(select("#", ...) == 0, "file loading excluded") end
              if name == "Sheet" then assert(type((...)) ~= "string", "file loading excluded") end
            end
            return implementation(...)
          end
        end
      end
    end
  end
end

local samples = {
  vector = ipe.Vector(), matrix = ipe.Matrix(), rect = ipe.Rect(),
  line = ipe.Line(ipe.Vector(), ipe.Vector(1, 0)),
  segment = ipe.Segment(ipe.Vector(), ipe.Vector(1, 0)),
  bezier = ipe.Bezier(ipe.Vector(), ipe.Vector(1, 0), ipe.Vector(2, 0), ipe.Vector(3, 0)),
  arc = ipe.Arc(ipe.Matrix()), object = ipe.Group({}), page = ipe.Page(),
  document = ipe.Document(), sheet = ipe.Sheet(), cascade = ipe.Sheets(),
}
for owner, sample in pairs(samples) do trackMethods(owner, getmetatable(sample)) end
trackMethods("ipe", ipe)
vector = ipe.Vector

local function equal(actual, expected)
  assert(actual == expected,
         string.format("expected %s, got %s", tostring(expected), tostring(actual)))
end

local function near(actual, expected, tolerance)
  assert(math.abs(actual - expected) <= (tolerance or 1e-8),
         string.format("expected %.12g, got %.12g", expected, actual))
end

local function point(actual, expected, tolerance)
  near(actual.x, expected.x, tolerance)
  near(actual.y, expected.y, tolerance)
end

local function rectangle(left, bottom, right, top)
  local result = ipe.Rect()
  result:add(vector(left, bottom))
  result:add(vector(right, top))
  return result
end

local function bounds(actual, left, bottom, right, top)
  assert(not actual:isEmpty())
  near(actual:left(), left)
  near(actual:bottom(), bottom)
  near(actual:right(), right)
  near(actual:top(), top)
end

local function raises(action, fragment)
  local success, message = pcall(action)
  assert(not success, "expected an error")
  if fragment then
    assert(tostring(message):find(fragment, 1, true), tostring(message))
  end
end

local function test(name, action, knownIssue, expectedError)
  tests = tests + 1
  local success, message = xpcall(action, debug.traceback)
  if knownIssue and not success and tostring(message):find(expectedError, 1, true) then
    knownFailures = knownFailures + 1
    print("XFAIL " .. name .. ": " .. knownIssue)
  elseif knownIssue and success then
    failures = failures + 1
    print("XPASS " .. name .. ": remove the known-issue marker")
  elseif success then
    print("PASS " .. name)
  else
    failures = failures + 1
    print("FAIL " .. name .. "\n" .. message)
  end
end

test("vectors: constructors, coordinates, arithmetic and equality", function()
  equal(vector(), vector(0, 0))
  local first, second = vector(3, 4), vector(-1, 2)
  equal(first.x, 3)
  equal(first.y, 4)
  equal(first + second, vector(2, 6))
  equal(first - second, vector(4, 2))
  equal(-first, vector(-3, -4))
  equal(2 * first, vector(6, 8))
  equal(first * 2, vector(6, 8))
  equal(first ^ second, 5)
  equal(first .. second, 5)
  assert(first ~= second)
  assert(tostring(first):find("3", 1, true))
  equal(first.unknown, nil)
  raises(function() first.x = 7 end)
  raises(function() return vector("bad", 0) end)
end)

test("vectors: lengths, directions, factorization and angles", function()
  local value = vector(3, 4)
  equal(value:len(), 5)
  equal(value:sqLen(), 25)
  point(value:normalized(), vector(0.6, 0.8))
  equal(value:orthogonal(), vector(-4, 3))
  local unit = vector()
  equal(value:factorize(unit), 5)
  point(unit, vector(0.6, 0.8))
  near(vector(0, 1):angle(), pi / 2)
  point(ipe.Direction(pi / 2), vector(0, 1))
  near(ipe.normalizeAngle(-pi / 2, 0), 3 * pi / 2)
  near(ipe.normalizeAngle(3 * pi, -pi), -pi)
end)

test("matrices: constructors, coefficients and composition", function()
  assert(ipe.Matrix():isIdentity())
  local transform = ipe.Matrix(2, 0, 0, 3, 4, 5)
  equal(transform, ipe.Matrix({2, 0, 0, 3, 4, 5}))
  assert(transform ~= ipe.Matrix())
  equal(ipe.Matrix(2, 0, 0, 3), transform:linear())
  equal(transform:translation(), vector(4, 5))
  equal(transform * vector(1, 2), vector(6, 11))
  equal(ipe.Translation(4, 5), ipe.Translation(vector(4, 5)))
  equal(ipe.Translation(4, 5) * transform:linear(), transform)
  point(ipe.Rotation(pi / 2) * vector(1, 0), vector(0, 1))
  for index, expected in ipairs({2, 0, 0, 3, 4, 5}) do
    equal(transform:elements()[index], expected)
    equal(transform:coeff()[index], expected)
  end
  assert(tostring(transform):find("2", 1, true))
  raises(function() return ipe.Matrix(1, 2) end, "incorrect arguments")
  raises(function() return ipe.Matrix({1, 2}) end)
end)

test("matrices: inverse and singularity", function()
  local transform = ipe.Matrix(2, 1, 1, 3, 4, 5)
  assert(not transform:isSingular(1e-8))
  point(transform:inverse() * (transform * vector(7, 9)), vector(7, 9))
  local singular = ipe.Matrix(1, 2, 2, 4)
  assert(singular:isSingular(0))
  assert(ipe.Matrix(1, 0, 0, 1e-10):isSingular(1e-8))
  raises(function() return singular:inverse() end, "singular")
end)

test("rectangles: mutation, corners, containment and clipping", function()
  local area = ipe.Rect()
  assert(area:isEmpty())
  area:add(vector(1, 2))
  area:add(vector(5, 8))
  bounds(area, 1, 2, 5, 8)
  equal(area:width(), 4)
  equal(area:height(), 6)
  equal(area:bottomLeft(), vector(1, 2))
  equal(area:topRight(), vector(5, 8))
  equal(area:topLeft(), vector(1, 8))
  equal(area:bottomRight(), vector(5, 2))
  assert(area:contains(vector(1, 2)))
  assert(not area:contains(vector(0, 0)))
  assert(area:contains(rectangle(2, 3, 4, 7)))
  assert(not area:contains(rectangle(0, 0, 4, 7)))
  assert(area:intersects(rectangle(4, 7, 9, 9)))
  assert(not area:intersects(rectangle(10, 10, 12, 12)))
  area:add(rectangle(-1, -2, 2, 3))
  bounds(area, -1, -2, 5, 8)
  area:clipTo(rectangle(0, 0, 3, 4))
  bounds(area, 0, 0, 3, 4)
  assert(tostring(area):find("Rect", 1, true))
  area:clipTo(rectangle(10, 10, 12, 12))
  assert(area:isEmpty())
end)

test("lines: constructors, projection, orientation and intersections", function()
  local horizontal = ipe.Line(vector(), vector(1, 0))
  equal(horizontal:point(), vector())
  equal(horizontal:dir(), vector(1, 0))
  equal(horizontal:normal(), vector(0, 1))
  equal(horizontal:side(vector(0, 2)), 1)
  equal(horizontal:side(vector(0, -2)), -1)
  equal(horizontal:side(vector(2, 0)), 0)
  equal(horizontal:distance(vector(1, -3)), 3)
  equal(horizontal:project(vector(2, 3)), vector(2, 0))
  local vertical = ipe.LineThrough(vector(2, -1), vector(2, 1))
  point(horizontal:intersects(vertical), vector(2, 0))
  equal(horizontal:intersects(ipe.Line(vector(0, 1), vector(1, 0))), nil)
  near(ipe.Bisector(vector(-2, 0), vector(2, 0)):distance(vector(0, 7)), 0)
  assert(tostring(horizontal):find("Line", 1, true))
  raises(function() return ipe.Bisector(vector(), vector()) end, "distinct")
end)

test("segments: endpoints, projection, distances and intersections", function()
  local segment = ipe.Segment(vector(0, 0), vector(4, 0))
  local first, last = segment:endpoints()
  equal(first, vector())
  equal(last, vector(4, 0))
  equal(segment:line():distance(vector(2, 3)), 3)
  equal(segment:project(vector(2, 3)), vector(2, 0))
  equal(segment:project(vector(5, 3)), nil)
  equal(segment:distance(vector(2, 3)), 3)
  equal(segment:distance(vector(7, 4)), 5)
  local crossing = ipe.Segment(vector(2, -1), vector(2, 1))
  point(segment:intersects(crossing), vector(2, 0))
  point(segment:intersects(crossing:line()), vector(2, 0))
  equal(segment:intersects(ipe.Segment(vector(5, -1), vector(5, 1))), nil)
  equal(segment:intersects(ipe.Line(vector(0, 1), vector(1, 0))), nil)
  assert(tostring(segment):find("Segment", 1, true))
end)

test("beziers: control points, evaluation, bounds and snapping", function()
  local curve = ipe.Bezier(vector(), vector(1, 0), vector(2, 0), vector(3, 0))
  for index, control in ipairs({curve:controlpoints()}) do
    equal(control, vector(index - 1, 0))
  end
  equal(curve:point(0), vector())
  equal(curve:point(1), vector(3, 0))
  point(curve:point(0.5), vector(1.5, 0))
  bounds(curve:bbox(), -0.5, -0.5, 3.5, 0.5)
  local parameter, position = curve:snap(vector(1.5, 1))
  near(parameter, 0.5, 1e-5)
  point(position, vector(1.5, 0), 1e-5)
  local quad = ipe.Quad(vector(), vector(1, 2), vector(2, 0))
  point(quad:point(0.5), vector(1, 1))
  assert(tostring(curve):find("Bezier", 1, true))
end)

test("beziers: line, segment and curve intersections", function()
  local curve = ipe.Bezier(vector(), vector(1, 0), vector(2, 0), vector(3, 0))
  local crossing = ipe.Bezier(vector(1, -2), vector(1, -1), vector(1, 1), vector(1, 2))
  local line = ipe.Line(vector(1, -2), vector(0, 1))
  for _, other in ipairs({line, ipe.Segment(vector(1, -2), vector(1, 2)), crossing}) do
    local intersections = curve:intersect(other)
    equal(#intersections, 1)
    point(intersections[1], vector(1, 0), 1e-5)
  end
  equal(#curve:intersect(ipe.Line(vector(0, 2), vector(1, 0))), 0)
end)

test("arcs: constructors, angles, bounds, transformations and snapping", function()
  local circle = ipe.Arc(ipe.Matrix())
  assert(circle:isEllipse())
  bounds(circle:bbox(), -1, -1, 1, 1)
  local quarter = ipe.Arc(ipe.Matrix(), 0, pi / 2)
  assert(not quarter:isEllipse())
  local first, last = quarter:endpoints()
  point(first, vector(1, 0))
  point(last, vector(0, 1))
  local alpha, beta = quarter:angles()
  near(alpha, 0)
  near(beta, pi / 2)
  equal(quarter:matrix(), ipe.Matrix())
  bounds(quarter:bbox(), 0, 0, 1, 1)
  local fromPoints = ipe.Arc(ipe.Matrix(), vector(1, 0), vector(0, 1))
  point(select(2, fromPoints:endpoints()), last)
  bounds((ipe.Translation(2, 3) * circle):bbox(), 1, 2, 3, 4)
  local angle, position = circle:snap(vector(2, 0))
  near(angle, 0)
  point(position, vector(1, 0))
  assert(tostring(circle):find("Arc", 1, true))
end)

test("arcs: line, segment, bezier and arc intersections", function()
  local circle = ipe.Arc(ipe.Matrix())
  local crossing = ipe.Bezier(vector(-2, 0), vector(-1, 0), vector(1, 0), vector(2, 0))
  for _, other in ipairs({ipe.Line(vector(), vector(1, 0)),
                         ipe.Segment(vector(-2, 0), vector(2, 0)), crossing,
                         ipe.Arc(ipe.Translation(1, 0))}) do
    local intersections = circle:intersect(other)
    equal(#intersections, 2)
    for _, position in ipairs(intersections) do near(position:len(), 1, 1e-5) end
  end
  equal(#circle:intersect(ipe.Line(vector(0, 2), vector(1, 0))), 0)
end)

local function lineShape()
  return {{type = "curve", closed = false,
           {type = "segment", vector(), vector(10, 0)}}}
end

local function pathObject(attributes)
  return ipe.Path(attributes or {}, lineShape())
end

test("objects: common methods, metadata, cloning and XML round-trips", function()
  local objects = {
    group = ipe.Group({pathObject()}),
    path = pathObject(),
    text = ipe.Text({}, "a < b & c", vector(2, 3)),
    image = ipe.Image(rectangle(1, 2, 5, 8), "unused-image.png"),
    reference = ipe.Reference({}, "mark/disk(sx)", vector(2, 3)),
  }
  for kind, object in pairs(objects) do
    equal(object:type(), kind)
    assert(tostring(object):find(kind, 1, true))
    equal(object:matrix(), ipe.Matrix())
    object:setMatrix(ipe.Translation(10, 20))
    equal(object:matrix(), ipe.Translation(10, 20))
    object:setCustom("custom data")
    equal(object:getCustom(), "custom data")
    object:setAnimate("identity:one")
    equal(object:getAnimate(), "identity:one")
    local copy = object:clone()
    equal(copy:xml(), object:xml())
    copy:setMatrix(ipe.Matrix())
    copy:setCustom("copy")
    equal(object:matrix(), ipe.Translation(10, 20))
    equal(object:getCustom(), "custom data")
    local parsed = assert(ipe.Object(object:xml()))
    equal(parsed:type(), kind)
    equal(parsed:xml(), object:xml())
    object:setCustom(nil)
    object:setAnimate(nil)
    equal(object:getCustom(), "undefined")
    equal(object:getAnimate(), "undefined")
  end
  equal(ipe.Object("<unknown/>"), nil)
  raises(function() return ipe.Group({vector()}) end, "not an Ipe object")
end)

test("objects: XML escaping of custom metadata", function()
  local object = pathObject()
  object:setCustom("custom <&\" data")
  local parsed = ipe.Object(object:xml())
  assert(parsed, "serialized custom metadata must be valid XML")
  equal(parsed:getCustom(), object:getCustom())
end)

test("objects: absolute, symbolic, boolean and enumeration attributes", function()
  local object = pathObject({pen = 2.5, stroke = {r = 0.2, g = 0.4, b = 0.6},
                             fill = "red", pathmode = "strokedfilled"})
  near(object:get("pen"), 2.5)
  near(object:get("stroke").g, 0.4)
  equal(object:get("fill"), "red")
  equal(object:get("pathmode"), "strokedfilled")
  local attributes = {
    pen = "heavier", stroke = "blue", fill = "green", dashstyle = "[3 2] 0",
    farrow = true, rarrow = false, farrowsize = 7, rarrowsize = "large",
    farrowshape = "arrow/normal(spx)", rarrowshape = "arrow/normal(spx)",
    pathmode = "filled", linejoin = "round", linecap = "square", fillrule = "evenodd",
    pinned = "horizontal", transformations = "rigid", opacity = "opaque",
    strokeopacity = "opaque", gradient = "test-gradient", strokegradient = "test-gradient",
    tiling = "test-tiling", variant = "test-variant",
  }
  for property, expected in pairs(attributes) do
    object:set(property, expected)
    equal(object:get(property), expected)
  end
  object:set("farrow", "false")
  equal(object:get("farrow"), false)
  object:set("rarrow", "true")
  equal(object:get("rarrow"), true)
  equal(object:get("textsize"), "undefined")
  object:set("textsize", 18)
  equal(object:get("textsize"), "undefined")
  raises(function() object:get("not-a-property") end)
  raises(function() object:set("linejoin", "invalid") end)
  raises(function() object:set("stroke", {r = 1}) end)
  raises(function() object:set("farrow", "maybe") end)
end)

test("paths: bounding boxes, shape replacement and independent shape tables", function()
  local object = pathObject()
  local area = ipe.Rect()
  object:addToBBox(area, ipe.Translation(2, 3))
  bounds(area, 2, 3, 12, 3)
  local shape = object:shape()
  equal(shape[1].type, "curve")
  equal(shape[1].closed, false)
  equal(shape[1][1].type, "segment")
  equal(shape[1][1][2], vector(10, 0))
  shape[1][1][2] = vector(20, 0)
  equal(object:shape()[1][1][2], vector(10, 0))
  object:setShape(shape)
  equal(object:shape()[1][1][2], vector(20, 0))
  object:setMatrix(ipe.Translation(1, 2))
  local exact = ipe.Rect()
  object:addToBBox(exact, ipe.Matrix(), false)
  bounds(exact, 1, 2, 21, 2)
  local arrows = ipe.Path({farrow = true, rarrow = true}, lineShape(), true)
  assert(arrows:get("farrow") and arrows:get("rarrow"))
end)

test("paths: all curve segment and subpath representations", function()
  for _, kind in ipairs({"spline", "oldspline", "cardinal", "spiro"}) do
    local segment = {type = kind, vector(), vector(2, 1), vector(4, 1), vector(6, 0)}
    if kind == "cardinal" then segment.tension = 0.5 end
    local object = ipe.Path({}, {{type = "curve", closed = false, segment}})
    local output = object:shape()[1][1]
    equal(output.type, kind)
    equal(#output, 4)
    for index = 1, 4 do equal(output[index], segment[index]) end
    if kind == "cardinal" then near(output.tension, 0.5) end
    object:set("splinetype", kind == "spline" and "bspline" or
               kind == "oldspline" and "bspline" or kind)
  end
  local shape = {
    {type = "ellipse", ipe.Matrix(2, 0, 0, 3, 4, 5)},
    {type = "closedspline", vector(), vector(2, 0), vector(1, 2)},
    {type = "curve", closed = true,
     {type = "arc", vector(1, 0), vector(0, 1), arc = ipe.Arc(ipe.Matrix(), 0, pi / 2)},
     {type = "segment", vector(0, 1), vector(1, 0)}},
  }
  local object = ipe.Path({}, shape)
  local output = object:shape()
  equal(#output, 3)
  equal(output[1][1], shape[1][1])
  equal(output[2].type, "closedspline")
  equal(output[2][3], vector(1, 2))
  equal(output[3].closed, true)
  equal(output[3][1].type, "arc")
  equal(output[3][1].arc:matrix(), ipe.Matrix())
  equal(ipe.Object(object:xml()):xml(), object:xml())
end)

test("paths: malformed shape validation", function()
  local invalid = {
    {false}, {{}}, {{type = "bad"}}, {{type = "ellipse"}},
    {{type = "closedspline", "not a vector"}},
    {{type = "curve"}}, {{type = "curve", closed = false}},
    {{type = "curve", closed = false, false}},
    {{type = "curve", closed = false, {}}},
    {{type = "curve", closed = false, {type = "invalid"}}},
    {{type = "curve", closed = false, {type = "segment", vector()}}},
    {{type = "curve", closed = false, {type = "segment", vector(), "bad"}}},
    {{type = "curve", closed = false, {type = "arc", vector(), vector(1, 0)}}},
    {{type = "curve", closed = false, {type = "cardinal", vector(), vector(1, 0)}}},
  }
  for _, shape in ipairs(invalid) do
    raises(function() return ipe.Path({}, shape) end)
  end
end)

test("text and references: content, positions, dimensions and attributes", function()
  local label = ipe.Text({textsize = 12}, "hello", vector(2, 3))
  equal(label:position(), vector(2, 3))
  equal(label:text(), "hello")
  label:setText("new <&> text")
  equal(label:text(), "new <&> text")
  equal(label:get("minipage"), false)
  local width, height, depth = label:dimensions()
  assert(type(width) == "number" and type(height) == "number" and type(depth) == "number")
  local minipage = ipe.Text({}, "paragraph", vector(), 42)
  equal(minipage:get("minipage"), true)
  equal(minipage:get("width"), 42)
  for property, expected in pairs({textsize = 14, textstyle = "normal", labelstyle = "normal",
                                    width = 50, horizontalalignment = "hcenter",
                                    verticalalignment = "top"}) do
    minipage:set(property, expected)
    equal(minipage:get(property), expected)
  end
  minipage:set("transformabletext", true)
  equal(minipage:get("transformations"), "affine")
  minipage:set("transformabletext", false)
  equal(minipage:get("transformations"), "translations")
  equal(select(1, minipage:dimensions()), 50)
  local reference = ipe.Reference({symbolsize = 3}, "mark/disk(sx)", vector(5, 6))
  equal(reference:symbol(), "mark/disk(sx)")
  equal(reference:position(), vector(5, 6))
  equal(reference:get("symbolsize"), 3)
  reference:set("markshape", "mark/box(sx)")
  equal(reference:symbol(), "mark/box(sx)")
end)

test("groups: cloned elements, clipping and links", function()
  local original = pathObject()
  local group = ipe.Group({original, ipe.Text({}, "text", vector())})
  original:setCustom("changed")
  equal(group:count(), 2)
  equal(group:elementType(1), "path")
  equal(group:elementType(2), "text")
  equal(group:element(1):getCustom(), "undefined")
  group:element(1):setCustom("detached")
  equal(group:element(1):getCustom(), "undefined")
  local elements = group:elements()
  equal(#elements, 2)
  elements[2]:setText("detached")
  equal(group:element(2):text(), "text")
  equal(group:clip(), nil)
  group:setClip(lineShape())
  equal(group:clip()[1][1][2], vector(10, 0))
  group:setClip(nil)
  equal(group:clip(), nil)
  group:setClip(lineShape())
  group:setClip()
  equal(group:clip(), nil)
  group:setText("https://example.org/?a=1&b=2")
  equal(group:text(), "https://example.org/?a=1&b=2")
  equal(ipe.Object(group:xml()):text(), group:text())
  raises(function() group:element(0) end, "element index")
  raises(function() group:elementType(3) end, "element index")
  local clone = group:clone()
  local before = group:xml()
  local replacement = ipe.Text({}, "replacement", vector())
  replacement:set("pinned", "fixed")
  clone:replace(1, replacement)
  replacement:setText("changed")
  equal(clone:element(1):text(), "replacement")
  equal(clone:get("pinned"), "fixed")
  equal(clone:text(), group:text())
  equal(group:xml(), before)
  clone:replace(1, original)
  equal(clone:get("pinned"), "none")
  raises(function() group:replace(0, original) end, "element index")
  raises(function() group:replace(3, original) end, "element index")
  raises(function() group:replace(1, vector()) end)
end)

test("images: rectangles, external paths and independent copies", function()
  local area = rectangle(10, 20, 110, 220)
  local image = ipe.Image(area, "unused.png")
  bounds(image:rect(), 10, 20, 110, 220)
  equal(image:info().path, "unused.png")
  image:setPath("another-unused.png")
  equal(image:info().path, "another-unused.png")
  image:rect():add(vector(-100, -100))
  area:add(vector(-200, -200))
  bounds(image:rect(), 10, 20, 110, 220)
  image:setMatrix(ipe.Matrix(2, 0, 0, 3, 50, 60))
  bounds(image:rect(), 10, 20, 110, 220)
  local box = ipe.Rect()
  image:addToBBox(box, ipe.Matrix())
  bounds(box, 70, 120, 270, 720)
  image:set("opacity", "half")
  equal(image:get("opacity"), "half")
  raises(function() return ipe.Image(area, pathObject()) end, "not an image object")
end)

test("objects: subtype guards", function()
  local object = pathObject()
  for _, method in ipairs({"position", "text", "dimensions", "count", "clip", "symbol",
                           "rect", "info", "elements"}) do
    raises(function() return object[method](object) end, "not a")
  end
  raises(function() object:setText("bad") end, "not a text")
  raises(function() object:setPath("unused") end, "not an image")
  raises(function() object:setClip(nil) end, "not a group")
  raises(function() object:element(1) end, "not a group")
  raises(function() object:elementType(1) end, "not a group")
  raises(function() object:replace(1, object) end, "not a group")
  raises(function() object:setBitmap(object) end, "not an image")
  local group = ipe.Group({})
  raises(function() group:shape() end, "not a path")
  raises(function() group:setShape({}) end, "not a path")
end)

test("splines: all conversion algorithms, continuity and validation", function()
  for _, kind in ipairs({"spline", "oldspline", "cardinal", "spiro"}) do
    local controls = {type = kind, tension = 0.5,
                      vector(), vector(2, 1), vector(4, 1), vector(6, 0)}
    for _, closed in ipairs({false, true}) do
      local segments = ipe.splineToBeziers(controls, closed)
      assert(#segments > 0)
      for index, segment in ipairs(segments) do
        equal(segment.type, "spline")
        equal(#segment, 4)
        if index > 1 then equal(segment[1], segments[index - 1][4]) end
      end
      if closed then
        point(segments[#segments][4], segments[1][1])
      else
        point(segments[1][1], controls[1])
        point(segments[#segments][4], controls[#controls])
      end
    end
  end
  raises(function() ipe.splineToBeziers(false, false) end, "not a table")
  raises(function() ipe.splineToBeziers({false}, false) end, "not a vector")
  raises(function() ipe.splineToBeziers({vector(), vector(1, 0)}, false) end, "no type")
  raises(function() ipe.splineToBeziers({type = "segment"}, false) end, "invalid type")
  raises(function() ipe.splineToBeziers({type = "cardinal"}, false) end, "no tension")
end)

test("images: embedded bitmap parsing, metadata and bitmap constructor", function()
  for _, fixture in ipairs({
    {space = "DeviceRGB", data = "ff0000", format = "rgb"},
    {space = "DeviceGray", data = "80", format = "gray"},
    {space = "DeviceRGBAlpha", data = "80ff0000", format = "rgb alpha", length = 4},
  }) do
    local xml = string.format([[<ipeselection>
<bitmap id="1" width="1" height="1" BitsPerComponent="8" ColorSpace="%s" length="%d">%s</bitmap>
<image layer="alpha" rect="0 0 1 1" bitmap="1"/>
</ipeselection>]], fixture.space, fixture.length or (#fixture.data / 2), fixture.data)
    local objects, layers = ipe.Object(xml)
    equal(#objects, 1)
    equal(layers[1], "alpha")
    local image = objects[1]
    equal(image:type(), "image")
    equal(image:info().width, 1)
    equal(image:info().height, 1)
    equal(image:info().format, fixture.format)
    equal(image:info().path, nil)
    local copy = ipe.Image(rectangle(2, 3, 12, 23), image)
    equal(copy:info().format, fixture.format)
    bounds(copy:rect(), 2, 3, 12, 23)
    copy:setPath("unused.png")
    equal(copy:info().path, "unused.png")
    copy:setPath("")
    equal(copy:info().format, fixture.format)
    local external = ipe.Image(rectangle(2, 3, 12, 23), "unused.png")
    external:setMatrix(ipe.Translation(5, 6))
    external:set("opacity", "half")
    external:setCustom("metadata")
    external:setAnimate("animation")
    local original = external:clone()
    local before = external:xml()
    external:setBitmap(image)
    equal(external:info().path, nil)
    equal(external:info().format, fixture.format)
    equal(external:xml():gsub('bitmap="[^"]*"', 'path="unused.png"'), before)
    equal(original:info().path, "unused.png")
    raises(function() external:setBitmap(original) end, "no internal bitmap")
    raises(function() external:setBitmap(pathObject()) end, "not an image")
  end
end)

local function contains(values, expected)
  for _, value in ipairs(values) do
    if value == expected then return true end
  end
  return false
end

local function fixtureSheet()
  return assert(ipe.Sheet(nil, [[<ipestyle name="fixture">
<layout paper="200 300" origin="10 20" frame="180 260" skip="8" crop="no"/>
<titlestyle pos="20 250" size="large" color="red" halign="center" valign="top"/>
<gradient name="axial" type="axial" coords="0 0 10 0" extend="yes">
<stop offset="0" color="1 0 0"/><stop offset="1" color="0 0 1"/>
</gradient>
<gradient name="radial" type="radial" coords="0 0 0 2 3 10">
<stop offset="0" color="0 0 0"/><stop offset="1" color="1 1 1"/>
</gradient>
<tiling name="hatch" angle="45" step="4" width="1"/>
<effect name="fade" duration="2" transition="1" effect="1"/>
<textstyle name="paragraph" begin="before" end="after"/>
</ipestyle>]]))
end

test("stylesheets: constructors, names, scalar and color attributes", function()
  local sheet = ipe.Sheet()
  equal(sheet:name(), nil)
  equal(sheet:isStandard(), false)
  sheet:setName("test")
  equal(sheet:name(), "test")
  for _, kind in ipairs({"pen", "symbolsize", "arrowsize", "textsize", "textstretch",
                         "opacity", "gridsize", "anglesize"}) do
    sheet:add(kind, "test", 2.5)
    near(tonumber(sheet:find(kind, "test")), 2.5)
    assert(contains(sheet:allNames(kind), "test"))
    sheet:setAttribute(kind, "test", "3.25")
    near(tonumber(sheet:find(kind, "test")), 3.25)
    sheet:remove(kind, "test")
    equal(sheet:find(kind, "test"), nil)
    sheet:remove(kind, "missing")
  end
  sheet:add("color", "accent", {r = 0.2, g = 0.4, b = 0.6})
  equal(sheet:find("color", "accent"), "0.2 0.4 0.6")
  sheet:setAttribute("color", "accent", "1 0 0")
  equal(sheet:find("color", "accent"), "1 0 0")
  sheet:add("dashstyle", "custom", "[3 2] 0")
  equal(sheet:find("dashstyle", "custom"), "[3 2] 0")
  sheet:setAttribute("dashstyle", "custom", "[4 2] 1")
  equal(sheet:find("dashstyle", "custom"), "[4 2] 1")
  sheet:setAttribute("textsize", "tex", "\\large")
  equal(sheet:find("textsize", "tex"), "\\large")
  assert(tostring(sheet):find("Sheet", 1, true))
end)

test("stylesheets: cloning, in-memory XML and argument validation", function()
  local sheet = fixtureSheet()
  local copy = sheet:clone()
  equal(copy:xml(false), sheet:xml(false))
  copy:setName("copy")
  equal(sheet:name(), "fixture")
  copy:add("pen", "unique", 7)
  equal(sheet:find("pen", "unique"), nil)
  local parsed = assert(ipe.Sheet(nil, sheet:xml(true)))
  equal(parsed:xml(true), sheet:xml(true))
  local invalid, message = ipe.Sheet(nil, "<bad/>")
  equal(invalid, nil)
  assert(type(message) == "string")
  raises(function() sheet:add("invalid", "name", 1) end)
  raises(function() sheet:add("pen", "name", "symbolic") end)
  raises(function() sheet:add("textstyle", "name", "bad") end)
  raises(function() sheet:add("dashstyle", "name", "symbolic") end)
  raises(function() sheet:find("gradient", "axial") end, "no simple attribute")
  raises(function() sheet:setAttribute("textstyle", "name", "bad") end)
  raises(function() sheet:setAttribute("color", "name", "symbolic") end)
  raises(function() sheet:set("symbol", "bad") end)
end)

test("stylesheets: defaults, symbols and complex definition copying", function()
  local sheet, source = ipe.Sheet(), fixtureSheet()
  for kind, value in pairs({preamble = "\\newcommand{\\test}{ok}", linecap = "round",
                           linejoin = "bevel", fillrule = "evenodd"}) do
    sheet:set(kind, value)
  end
  local symbol = pathObject()
  sheet:add("symbol", "test-symbol", symbol)
  symbol:setCustom("detached")
  for _, definition in ipairs({{"gradient", "axial"}, {"gradient", "radial"},
                               {"tiling", "hatch"}, {"effect", "fade"}}) do
    sheet:addFrom(source, definition[1], definition[2])
    assert(contains(sheet:allNames(definition[1]), definition[2]))
  end
  raises(function() sheet:addFrom(source, "gradient", "absent") end, "no such gradient")
  raises(function() sheet:addFrom(source, "tiling", "absent") end, "no such tiling")
  raises(function() sheet:addFrom(source, "effect", "absent") end, "no such effect")
  raises(function() sheet:addFrom(source, "pen", "absent") end, "cannot handle")
  local cascade = ipe.Sheets()
  cascade:insert(1, sheet)
  equal(cascade:find("preamble"), "\\newcommand{\\test}{ok}\n")
  equal(cascade:find("linecap"), "round")
  equal(cascade:find("linejoin"), "bevel")
  equal(cascade:find("fillrule"), "evenodd")
  equal(cascade:find("symbol", "test-symbol"):getCustom(), "undefined")
  local detached = cascade:find("symbol", "test-symbol")
  detached:setCustom("detached")
  equal(cascade:find("symbol", "test-symbol"):getCustom(), "undefined")
  equal(cascade:find("symbol", "absent"), nil)
end)

test("cascades: precedence, lookup, cloning and sheet ownership", function()
  local cascade = ipe.Sheets()
  equal(cascade:count(), 0)
  local lower, higher = ipe.Sheet(), ipe.Sheet()
  lower:setName("lower")
  higher:setName("higher")
  lower:add("pen", "shared", 2)
  higher:add("pen", "shared", 5)
  higher:add("color", "accent", {r = 1, g = 0, b = 0})
  cascade:insert(1, lower)
  cascade:insert(1, higher)
  equal(cascade:count(), 2)
  equal(cascade:find("pen", "shared"), 5)
  local absoluteColor = {r = 0.2, g = 0.4, b = 0.6}
  equal(cascade:find("color", absoluteColor), absoluteColor)
  equal(cascade:find("color", "accent").r, 1)
  assert(cascade:has("pen", "shared"))
  assert(not cascade:has("pen", "absent"))
  equal(#cascade:allNames("pen"), 1)
  equal(cascade:sheet(1):name(), "higher")
  cascade:sheet(1):add("pen", "shared", 6)
  equal(cascade:find("pen", "shared"), 6)
  local copy = cascade:clone()
  copy:sheet(1):add("pen", "shared", 9)
  equal(cascade:find("pen", "shared"), 6)
  local second = ipe.Sheets()
  second:insert(1, cascade:sheet(1))
  second:sheet(1):setName("independent")
  equal(cascade:sheet(1):name(), "higher")
  cascade:remove(1)
  equal(cascade:find("pen", "shared"), 2)
  assert(tostring(cascade):find("Cascade", 1, true))
  raises(function() cascade:sheet(0) end, "out of bounds")
  raises(function() cascade:insert(3, ipe.Sheet()) end, "out of bounds")
  raises(function() cascade:remove(2) end, "out of bounds")
end)

test("cascades: absolute numeric lookup", function()
  equal(ipe.Sheets():find("pen", 4.5), 4.5)
end)

test("cascades: layout, title style and gradient structures", function()
  local cascade = ipe.Sheets()
  cascade:insert(1, fixtureSheet())
  local layout = cascade:find("layout")
  equal(layout.papersize, vector(200, 300))
  equal(layout.origin, vector(10, 20))
  equal(layout.framesize, vector(180, 260))
  equal(layout.paragraph_skip, 8)
  equal(layout.crop, false)
  local title = cascade:find("titlestyle")
  equal(title.pos, vector(20, 250))
  equal(title.size, "large")
  equal(title.color, "red")
  equal(title.horizontalalignment, "hcenter")
  equal(title.verticalalignment, "top")
  local axial = cascade:find("gradient", "axial")
  equal(axial.type, "axial")
  equal(axial.v[1], vector())
  equal(axial.v[2], vector(10, 0))
  equal(axial.extend, true)
  equal(axial.matrix, ipe.Matrix())
  equal(#axial.stops, 2)
  equal(axial.stops[1].offset, 0)
  equal(axial.stops[1].color.r, 1)
  equal(axial.stops[2].offset, 1)
  equal(axial.stops[2].color.b, 1)
  local radial = cascade:find("gradient", "radial")
  equal(radial.type, "radial")
  equal(radial.radius[1], 0)
  equal(radial.radius[2], 10)
  equal(radial.v[2], vector(2, 3))
end)

test("pages: defaults, layers, ordering, locking and snapping", function()
  local page = ipe.Page()
  equal(#page, 0)
  equal(page:countLayers(), 1)
  equal(page:layers()[1], "alpha")
  equal(page:countViews(), 1)
  equal(page:active(1), "alpha")
  assert(page:visible(1, "alpha"))
  equal(page:addLayer("beta"), "beta")
  local automatic = page:addLayer()
  assert(automatic ~= "alpha" and automatic ~= "beta")
  equal(page:countLayers(), 3)
  page:renameLayer("beta", "renamed")
  equal(page:layers()[2], "renamed")
  page:moveLayer("renamed", 1)
  equal(page:layers()[1], "renamed")
  page:reorderLayers({"alpha", automatic, "renamed"})
  equal(table.concat(page:layers(), ","), "alpha," .. automatic .. ",renamed")
  equal(page:isLocked("renamed"), false)
  page:setLocked("renamed", true)
  equal(page:isLocked("renamed"), true)
  page:setLocked("renamed", false)
  for _, mode in ipairs({"never", "visible", "always"}) do
    page:setSnapping("renamed", mode)
    equal(page:snapping("renamed"), mode)
  end
  page:setLayerData("renamed", "layer <&> data")
  equal(page:layerData("renamed"), "layer <&> data")
  page:removeLayer(automatic)
  equal(page:countLayers(), 2)
  assert(tostring(page):find("Page", 1, true))
  raises(function() page:isLocked("absent") end, "layer does not exist")
  raises(function() page:setSnapping("alpha", "invalid") end)
  raises(function() page:moveLayer("alpha", 0) end, "invalid target index")
  raises(function() page:reorderLayers(false) end, "not a table")
  raises(function() page:reorderLayers({false}) end, "not a string")
end)

test("pages: views, visibility, effects, marking and names", function()
  local page = ipe.Page()
  page:addLayer("beta")
  page:insert(nil, pathObject(), nil, "beta")
  page:insertView(2, "beta")
  equal(page:countViews(), 2)
  equal(page:active(2), "beta")
  page:setActive(2, "alpha")
  equal(page:active(2), "alpha")
  page:setVisible(2, "beta", true)
  assert(page:visible(2, "beta"))
  assert(page:visible(2, 1))
  page:setVisible(2, "beta", false)
  assert(not page:visible(2, "beta"))
  assert(not page:visible(2, 1))
  page:setEffect(2, "fade")
  equal(page:effect(2), "fade")
  page:setMarkedView(2, true)
  equal(page:markedView(2), true)
  page:setMarkedView(2, false)
  equal(page:markedView(2), false)
  page:setViewName(2, "second view")
  equal(page:viewName(2), "second view")
  page:removeView(1)
  equal(page:countViews(), 1)
  equal(page:viewName(1), "second view")
  page:clearViews()
  equal(page:countViews(), 0)
  page:insertView(1, "alpha")
  equal(page:countViews(), 1)
  raises(function() page:active(0) end, "invalid view index")
  raises(function() page:removeView(2) end, "invalid view index")
  raises(function() page:visible(1, "absent") end, "layer does not exist")
end)

test("pages: attribute maps and per-layer transformation matrices", function()
  local page = ipe.Page()
  page:addLayer("beta")
  equal(#page:viewMap(1), 0)
  local mappings = {}
  for _, kind in ipairs({"pen", "symbolsize", "arrowsize", "opacity", "color",
                         "dashstyle", "symbol"}) do
    mappings[#mappings + 1] = {kind = kind, map = "source", to = "target"}
  end
  page:setViewMap(1, mappings)
  local output = page:viewMap(1)
  equal(#output, #mappings)
  for index, mapping in ipairs(output) do
    equal(mapping.kind, mappings[index].kind)
    equal(mapping.map, "source")
    equal(mapping.to, "target")
  end
  mappings[1].to = "detached"
  equal(page:viewMap(1)[1].to, "target")
  page:setViewMap(1, {})
  equal(#page:viewMap(1), 0)
  equal(next(page:layerMatrices(1)), nil)
  page:setLayerMatrices(1, {alpha = ipe.Translation(2, 3), beta = ipe.Matrix()})
  equal(page:layerMatrices(1).alpha, ipe.Translation(2, 3))
  equal(page:layerMatrices(1).beta, nil)
  page:setLayerMatrices(1, {})
  equal(next(page:layerMatrices(1)), nil)
  for _, invalid in ipairs({{false}, {{}}, {{kind = "invalid"}}, {{kind = "pen"}}}) do
    raises(function() page:setViewMap(1, invalid) end)
  end
  raises(function() page:setLayerMatrices(1, {absent = ipe.Matrix()}) end, "no such layer")
  raises(function() page:setLayerMatrices(1, {alpha = false}) end)
end)

test("pages: object insertion, iteration, mutation and selection", function()
  local page = ipe.Page()
  local original = pathObject()
  page:addLayer("beta")
  page:insert(nil, original, 1, "alpha")
  page:insert(1, ipe.Text({}, "label", vector()), 2, "beta")
  original:setCustom("detached")
  equal(#page, 2)
  equal(page[1]:type(), "text")
  equal(page[2]:getCustom(), "undefined")
  equal(page:layerOf(1), "beta")
  equal(page:select(1), 2)
  equal(page:select(2), 1)
  equal(page:primarySelection(), 2)
  assert(page:hasSelection())
  local count = 0
  for index, object, selected, layer in page:objects() do
    count = count + 1
    equal(index, count)
    equal(object:type(), page[index]:type())
    equal(selected, page:select(index))
    equal(layer, page:layerOf(index))
  end
  equal(count, 2)
  page:setLayerOf(1, "alpha")
  equal(page:layerOf(1), "alpha")
  page:deselectAll()
  assert(not page:hasSelection())
  equal(page:primarySelection(), nil)
  equal(page:select(1), nil)
  page:setSelect(1, 2)
  page:ensurePrimarySelection()
  equal(page:primarySelection(), 1)
  equal(page:select(1), 1)
  page:setSelect(1, nil)
  assert(not page:hasSelection())
  page:replace(1, original)
  equal(page[1]:type(), "path")
  original:setCustom("changed again")
  equal(page[1]:getCustom(), "detached")
  page:remove(2)
  equal(#page, 1)
  equal(page.unknown, nil)
  raises(function() return page[0] end, "invalid object index")
  raises(function() page:remove(2) end, "invalid object index")
  raises(function() page:setLayerOf(1, "absent") end, "layer does not exist")
end)

test("pages: bounds, invalidation, transforms, distance and edge finding", function()
  local page = ipe.Page()
  page:insert(nil, pathObject(), nil, "alpha")
  bounds(page:bbox(1), 0, 0, 10, 0)
  page:transform(1, ipe.Translation(2, 3))
  bounds(page:bbox(1), 2, 3, 12, 3)
  page[1]:setMatrix(ipe.Translation(5, 6))
  page:invalidateBBox(1)
  bounds(page:bbox(1), 5, 6, 15, 6)
  near(page:distance(1, vector(10, 8), 100), 2)
  equal(page:distance(1, vector(10, 100), 1), 1)
  equal(page:setAttribute(1, "pen", 2), true)
  equal(page:setAttribute(1, "pen", 2), false)
  equal(page[1]:get("pen"), 2)
  local origin, direction = page:findEdge(1, vector(7, 6.25))
  point(origin, vector(7, 6))
  near(direction, 0)
  equal(page:findEdge(1, vector(100, 100)), nil)
end)

test("pages: titles, variants, sections, notes and page style", function()
  local page = ipe.Page()
  page:setTitle(nil, "default title", false)
  equal(page:title(), "default title")
  equal(select(2, page:title()), false)
  page:setTitle("speaker", "speaker title", true)
  local title, skipped = page:title("speaker")
  equal(title, "speaker title")
  equal(skipped, true)
  equal(page:title(), "default title")
  page:setSections({section = "chapter", subsection = "part"})
  equal(page:sections().section, "chapter")
  equal(page:sections().subsection, "part")
  page:setSections({})
  equal(page:sections().section, nil)
  equal(page:sections().subsection, nil)
  page:setNotes("notes <&>\nsecond line")
  equal(page:notes(), "notes <&>\nsecond line")
  page:setMarked(false)
  equal(page:marked(), false)
  page:setMarked(true)
  equal(page:marked(), true)
  page:setStyle("presentation")
  equal(page:style(), "presentation")
  page:setStyle("normal")
  equal(page:style(), "normal")
end)

test("pages: cloning and both XML formats", function()
  local page = ipe.Page()
  page:addLayer("beta")
  page:insert(nil, pathObject(), 1, "alpha")
  page:insert(nil, ipe.Text({}, "text <&>", vector()), nil, "beta")
  page:setNotes("notes <&>")
  page:setTitle(nil, "title <&>", false)
  page:setLayerData("beta", "data <&>")
  page:setViewName(1, "named view")
  local copy = page:clone()
  equal(copy:xml("ipepage"), page:xml("ipepage"))
  copy[1]:setCustom("detached")
  copy:setNotes("changed")
  equal(page[1]:getCustom(), "undefined")
  equal(page:notes(), "notes <&>")
  local parsed = assert(ipe.Page(page:xml("ipepage")))
  equal(parsed:xml("ipepage"), page:xml("ipepage"))
  local objects, layers = ipe.Object(page:xml("ipeselection"))
  equal(#objects, 1)
  equal(objects[1]:type(), "path")
  equal(layers[1], "alpha")
  equal(ipe.Page("<bad/>"), nil)
  raises(function() page:xml("invalid") end)
end)

test("documents: construction, pages, iteration and clone ownership", function()
  local document = ipe.Document()
  equal(#document, 1)
  equal(#document[1], 0)
  equal(document:countTotalViews(), 1)
  local page = ipe.Page()
  page:setNotes("original")
  page:insertView(2, "alpha")
  document:append(page)
  document:insert(1, page)
  equal(#document, 3)
  equal(document:countTotalViews(), 5)
  page:setNotes("detached")
  equal(document[1]:notes(), "original")
  equal(document[3]:notes(), "original")
  local count = 0
  for index, current in document:pages() do
    count = count + 1
    equal(index, count)
    equal(current:notes(), document[index]:notes())
  end
  equal(count, 3)
  local old = document:set(2, page)
  equal(old:notes(), "")
  equal(document[2]:notes(), "detached")
  page:setNotes("changed")
  equal(document[2]:notes(), "detached")
  local removed = document:remove(3)
  equal(removed:notes(), "original")
  equal(#document, 2)
  equal(document.unknown, nil)
  assert(tostring(document):find("Document", 1, true))
  raises(function() return document[0] end, "invalid page number")
  raises(function() document:remove(3) end, "invalid page number")
  raises(function() document:insert(4, page) end, "invalid page number")
  local clone = document:clone()
  equal(#clone, #document)
  for key, value in pairs(document:properties()) do
    equal(clone:properties()[key], value)
  end
  clone[1]:setNotes("clone")
  equal(document[1]:notes(), "original")
  clone:sheets():sheet(1):add("pen", "clone-only", 17)
  equal(document:sheets():sheet(1):find("pen", "clone-only"), nil)
end)

test("documents: property round-trips and partial updates", function()
  local document = ipe.Document()
  local properties = {
    title = "title <&>", author = "author", subject = "subject", keywords = "one two",
    language = "en", preamble = "\\newcommand{\\test}{ok}", created = "D:20260101000000",
    modified = "D:20261021000000", creator = "Lua binding tests", fullscreen = true,
    numberpages = true, sequentialtext = true, tex = "luatex", variant = "speaker",
  }
  document:setProperties(properties)
  for key, expected in pairs(properties) do equal(document:properties()[key], expected) end
  document:setProperties({title = "changed", variant = "speaker"})
  equal(document:properties().title, "changed")
  equal(document:properties().author, "author")
  for _, engine in ipairs({"default", "pdftex", "xetex", "luatex"}) do
    document:setProperties({tex = engine, variant = "speaker"})
    equal(document:properties().tex, engine)
  end
  document:setProperties({fullscreen = false, numberpages = false, sequentialtext = false})
  equal(document:properties().fullscreen, false)
  equal(document:properties().numberpages, false)
  equal(document:properties().sequentialtext, false)
  raises(function() document:setProperties(false) end)
end)

test("documents: stylesheet replacement, borrowed cascades and style checking", function()
  local document = ipe.Document()
  local cascade = ipe.Sheets()
  local sheet = ipe.Sheet()
  sheet:setName("document styles")
  sheet:add("pen", "test", 3)
  cascade:insert(1, sheet)
  local old = document:replaceSheets(cascade)
  assert(old:count() >= 1)
  equal(document:sheets():count(), 1)
  equal(document:sheets():find("pen", "test"), 3)
  document:sheets():sheet(1):add("pen", "borrowed", 4)
  equal(document:sheets():find("pen", "borrowed"), 4)
  local second = ipe.Document()
  local secondOld = second:replaceSheets(document:sheets())
  assert(secondOld:count() >= 1)
  second:sheets():sheet(1):add("pen", "test", 9)
  equal(document:sheets():find("pen", "test"), 3)
  local object = pathObject({pen = "undefined-test-pen"})
  document[1]:insert(nil, object, nil, "alpha")
  assert(contains(document:checkStyle(), "undefined-test-pen"))
  document:sheets():sheet(1):add("pen", "undefined-test-pen", 2)
  assert(not contains(document:checkStyle(), "undefined-test-pen"))
end)

test("folders: path construction", function()
  for _, name in ipairs({"lua", "icons", "doc", "styles", "ipelets", "scripts", "latex",
                         "user-styles", "user-ipelets", "user-scripts", "config"}) do
    local folder = ipe.folder(name)
    equal(type(folder), "string")
    assert(#folder > 0)
    local filename = ipe.folder(name, "test-file")
    equal(type(filename), "string")
    assert(filename:find("test-file", 1, true))
  end
  raises(function() ipe.folder("unknown") end)
end)

test("ownership: garbage collection of owned and borrowed handles", function()
  for iteration = 1, 100 do
    local document = ipe.Document()
    local page = document[1]
    page:insert(nil, pathObject(), nil, "alpha")
    local object = page[1]
    local cascade = document:sheets()
    local sheet = cascade:sheet(1)
    local clonedPage, clonedObject = page:clone(), object:clone()
    local clonedCascade, clonedSheet = cascade:clone(), sheet:clone()
    collectgarbage("collect")
    equal(#document, 1)
    equal(#page, 1)
    equal(object:type(), "path")
    assert(cascade:count() > 0)
    assert(sheet:isStandard())
    equal(#clonedPage, 1)
    equal(clonedObject:type(), "path")
    equal(clonedCascade:count(), cascade:count())
    equal(clonedSheet:isStandard(), sheet:isStandard())
  end
  collectgarbage("collect")
end)

test("coverage: every registered in-memory entry point is exercised", function()
  local missing, count = {}, 0
  for name in pairs(registered) do
    count = count + 1
    if not covered[name] then missing[#missing + 1] = name end
  end
  table.sort(missing)
  assert(#missing == 0, "untested bindings: " .. table.concat(missing, ", "))
  print(string.format("Covered all %d registered in-memory entry points", count))
end)

for _, original in ipairs(originals) do original[1][original[2]] = original[3] end
local skipped = {}
for name in pairs(excluded) do
  if not name:find(".__gc", 1, true) then skipped[#skipped + 1] = name end
end
table.sort(skipped)
for _, name in ipairs(skipped) do print("SKIP " .. name .. ": " .. excluded[name]) end
print("SKIP file-loading constructor overloads: file-system access")
print(string.format("\n%d tests, %d failures, %d known failures", tests, failures, knownFailures))
if failures > 0 or knownFailures > 0 then os.exit(1) end
