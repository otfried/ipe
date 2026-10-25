-- --------------------------------------------------------------------
-- Doc-tests: run the examples of the manual chapter "Scripts and ipelets"
--
-- This script is run by "make -C ipelua doctest" (using ipescript).
-- It checks that
--  * every example script and ipelet in manual/examples does what the
--    manual says it does, by running it on a small document,
--  * every command shown in the console blocks of the chapter can be run
--    (in a temporary directory with the files named in the command), and
--    prints exactly the output shown,
--  * all Lua code blocks in the chapter at least compile,
--  * all files in manual/examples are used by the chapter, and all
--    files used by the chapter exist.
--
-- Environment variables (set by the Makefile):
--   IPESCRIPT   the ipescript program (default "ipescript")
--   IPEMANUAL   the manual directory (default "../../manual")
--   IPESCRIPTS  must contain the directories with run-ipelet.lua and
--               with the examples (and with this file)
--   IPELUAPATH  Lua path of Ipe's own Lua code (see run-ipelet)
-- --------------------------------------------------------------------

local ipescript = os.getenv("IPESCRIPT") or "ipescript"
local manual = os.getenv("IPEMANUAL") or "../../manual"
local chapter = manual .. "/82_ipelets.rst"
local exampledir = manual .. "/examples"

if os.getenv("IPEDEBUG") then
  io.stderr:write("Please unset IPEDEBUG, it changes the output of programs.\n")
  os.exit(1)
end

----------------------------------------------------------------------
-- Test framework
----------------------------------------------------------------------

local tests, failures = 0, 0

local function test(name, action)
  tests = tests + 1
  local ok, message = xpcall(action, debug.traceback)
  if ok then
    print("PASS " .. name)
  else
    failures = failures + 1
    print("FAIL " .. name .. "\n" .. tostring(message))
  end
end

local function check(condition, message)
  if not condition then error(message, 2) end
end

local function equal(actual, expected, what)
  if actual ~= expected then
    error(string.format("%s: expected %s, got %s", what or "value",
			tostring(expected), tostring(actual)), 2)
  end
end

local function contains(text, fragment, what)
  if not text:find(fragment, 1, true) then
    error(string.format("%s: expected to find '%s' in\n%s",
			what or "output", fragment, text), 2)
  end
end

local function near(actual, expected, what)
  if math.abs(actual - expected) > 1e-6 then
    error(string.format("%s: expected %g, got %g", what or "value", expected, actual), 2)
  end
end

----------------------------------------------------------------------
-- Temporary directory with the test documents
----------------------------------------------------------------------

local tmpdir = os.tmpname()
os.remove(tmpdir)
assert(ipe.mkdir(tmpdir))

local function tmp(name)
  return tmpdir .. "/" .. name
end

local function removeDirectory(dir)
  for _, f in ipairs(ipe.directory(dir)) do os.remove(dir .. "/" .. f) end
  os.remove(dir)
end

local function readfile(name)
  local f = assert(io.open(name, "rb"))
  local s = f:read("a")
  f:close()
  return s
end

local function copyfile(from, to)
  local data = readfile(from)
  local f = assert(io.open(to, "wb"))
  f:write(data)
  f:close()
end

local function cleanup()
  local dir = tmpdir .. "/doccmd"
  if ipe.fileExists(dir) then removeDirectory(dir) end
  removeDirectory(tmpdir)
end

local V = ipe.Vector

local function box(x1, y1, x2, y2, stroke)
  local shape = { { type = "curve", closed = true;
		    { type = "segment"; V(x1, y1), V(x2, y1) },
		    { type = "segment"; V(x2, y1), V(x2, y2) },
		    { type = "segment"; V(x2, y2), V(x1, y2) } } }
  return ipe.Path({ stroke = stroke, pen = "normal" }, shape)
end

-- A page with two boxes and a text, in layer "alpha"
local function fixture(locked)
  local doc = ipe.Document()
  local page = doc[1]
  page:insert(nil, box(100, 100, 200, 150, "blue"), nil, "alpha")
  page:insert(nil, box(250, 100, 300, 300, "black"), nil, "alpha")
  page:insert(nil, ipe.Text({ stroke = "black" }, "Hello world", V(100, 400)), nil, "alpha")
  if locked then page:setLocked("alpha", true) end
  return doc
end

assert(fixture(false):save(tmp("figure.ipe")))
assert(fixture(true):save(tmp("locked.ipe")))

----------------------------------------------------------------------
-- Running programs
----------------------------------------------------------------------

local function quote(s)
  return '"' .. s .. '"'
end

-- Run a shell command in directory dir; return output (stdout and
-- stderr), and exit status
local function shell(command, dir)
  local cmd = command .. " 2>&1"
  if dir then cmd = "cd " .. quote(dir) .. " && " .. cmd end
  local p = assert(io.popen(cmd, "r"))
  local output = p:read("a")
  local ok, how, code = p:close()
  return output, code
end

-- the example files that have been used
local used = {}

-- Run an ipelet with run-ipelet, with the given options and dialog
-- answers (name=value), on the given document.
-- Returns output, exit status, and the document produced.
local function runIpelet(name, options, input, answers)
  used[name] = true
  local out = tmp("result.ipe")
  os.remove(out)
  local output, status = shell(string.format("%s run-ipelet %s %s %s %s %s",
					     quote(ipescript), options or "",
					     quote(exampledir .. "/" .. name),
					     quote(tmp(input or "figure.ipe")), quote(out),
					     answers or ""))
  local result = ipe.Document(out)
  return output, status, result
end

local function runScript(name, args)
  used[name .. ".lua"] = true
  return shell(string.format("%s %s %s", quote(ipescript), name, args))
end

local function paths(page)
  local list = {}
  for i, obj in page:objects() do
    if obj:type() == "path" then list[#list + 1] = i end
  end
  return list
end

local function isRed(page, i)
  local c = page[i]:get("stroke")
  return c == "red"
end

local function undoOk(output)
  contains(output, "Undo restores the original page: OK")
  contains(output, "Redo restores the result: OK")
end

----------------------------------------------------------------------
-- Scripts
----------------------------------------------------------------------

test("count-objects script", function ()
  local output, status = runScript("count-objects", quote(tmp("figure.ipe")))
  equal(output, "Page 1: 3 objects (1 text, 2 path)\n", "output")
end)

test("red-paths script", function ()
  local out = tmp("red.ipe")
  local output, status = runScript("red-paths", quote(tmp("figure.ipe")) .. " "
				     .. quote(out))
  equal(output, "", "output")
  local doc = assert(ipe.Document(out))
  local page = doc[1]
  equal(#page, 3, "number of objects")
  for _, i in ipairs(paths(page)) do check(isRed(page, i), "path " .. i .. " is not red") end
  -- the text is not changed
  equal(page[3]:get("stroke"), "black", "stroke of text")
end)

----------------------------------------------------------------------
-- Ipelets
----------------------------------------------------------------------

test("ipelet-red-paths", function ()
  local output, status, doc = runIpelet("ipelet-red-paths.lua")
  equal(status, 0, "exit status")
  undoOk(output)
  local page = doc[1]
  equal(#page, 3, "number of objects")
  for _, i in ipairs(paths(page)) do check(isRed(page, i), "path " .. i .. " is not red") end
  equal(page[3]:get("stroke"), "black", "stroke of text")
end)

test("ipelet-red-paths changes only selected paths", function ()
  local output, status, doc = runIpelet("ipelet-red-paths.lua", "--select 2")
  equal(status, 0, "exit status")
  local page = doc[1]
  equal(page[1]:get("stroke"), "blue", "stroke of unselected path")
  check(isRed(page, 2), "selected path is not red")
end)

test("ipelet-red-paths with nothing to do registers no undo item", function ()
  local output, status = runIpelet("ipelet-red-paths.lua", "--select 3")
  equal(status, 0, "exit status")
  contains(output, "The ipelet did not register an undo item.")
end)

test("ipelet-mark-centers", function ()
  local output, status, doc = runIpelet("ipelet-mark-centers.lua", "--select 1,2")
  equal(status, 0, "exit status")
  undoOk(output)
  local page = doc[1]
  equal(#page, 5, "number of objects")
  local marks = {}
  for i, obj in page:objects() do
    if obj:type() == "reference" then
      marks[#marks + 1] = obj:position()
      equal(obj:symbol(), "mark/disk(sx)", "symbol")
    end
  end
  equal(#marks, 2, "number of marks")
  near(marks[1].x, 150, "x of first mark")
  near(marks[1].y, 125, "y of first mark")
  near(marks[2].x, 275, "x of second mark")
  near(marks[2].y, 200, "y of second mark")
end)

test("ipelet-circle", function ()
  local output, status, doc = runIpelet("ipelet-circle.lua", "--select none")
  equal(status, 0, "exit status")
  undoOk(output)
  local page = doc[1]
  equal(#page, 4, "number of objects")
  local shape = page[4]:shape()
  equal(shape[1].type, "ellipse", "type of shape")
  local m = shape[1][1]:elements()
  near(m[1], 32, "radius")
  near(m[4], 32, "radius")
  near(m[5], 0, "x of center")
  near(m[6], 0, "y of center")
end)

test("ipelet-scale scales around the center of the selection", function ()
  local output, status, doc = runIpelet("ipelet-scale.lua", "--select 1", nil, "factor=3")
  equal(status, 0, "exit status")
  contains(output, "factor = 3")
  undoOk(output)
  local page = doc[1]
  local bbox = page:bbox(1)
  near(bbox:width(), 300, "width")
  near(bbox:height(), 150, "height")
  local center = (bbox:bottomLeft() + bbox:topRight()) * 0.5
  near(center.x, 150, "x of center")
  near(center.y, 125, "y of center")
  -- other objects are unchanged
  near(page:bbox(2):width(), 50, "width of unselected object")
end)

test("ipelet-scale uses the default factor 2", function ()
  local output, status, doc = runIpelet("ipelet-scale.lua", "--select 1")
  equal(status, 0, "exit status")
  near(doc[1]:bbox(1):width(), 200, "width")
end)

test("ipelet-scale does nothing without selection or when canceled", function ()
  local output, status = runIpelet("ipelet-scale.lua", "--select none")
  equal(status, 0, "exit status")
  contains(output, "Nothing is selected")
  contains(output, "The ipelet did not register an undo item.")
  output, status = runIpelet("ipelet-scale.lua", "--cancel")
  equal(status, 0, "exit status")
  contains(output, "The ipelet did not register an undo item.")
end)

test("ipelet-scale rejects an illegal factor", function ()
  local output, status = runIpelet("ipelet-scale.lua", nil, nil, "factor=abc")
  equal(status, 0, "exit status")
  contains(output, "The scale factor must be a positive number")
  contains(output, "The ipelet did not register an undo item.")
end)

test("ipelet-lock-layers locks and unlocks", function ()
  local output, status, doc = runIpelet("ipelet-lock-layers.lua", "--method 1")
  equal(status, 0, "exit status")
  contains(output, "lock all layers")
  undoOk(output)
  check(doc[1]:isLocked("alpha"), "layer is not locked")
  output, status, doc = runIpelet("ipelet-lock-layers.lua", "--method 2", "locked.ipe")
  equal(status, 0, "exit status")
  contains(output, "unlock all layers")
  undoOk(output)
  check(not doc[1]:isLocked("alpha"), "layer is not unlocked")
end)

test("ipelet-wrong is detected by run-ipelet", function ()
  local output, status, doc = runIpelet("ipelet-wrong.lua")
  check(status ~= 0, "run-ipelet should fail")
  contains(output, "WARNING: the page has been modified without registering an undo item!")
end)

----------------------------------------------------------------------
-- The chapter itself
----------------------------------------------------------------------

local rst = readfile(chapter)
local lines = {}
for line in (rst .. "\n"):gmatch("(.-)\r?\n") do lines[#lines + 1] = line end

-- Content of the indented block following line i: lines, without the
-- indentation, up to the first non-blank line that is not indented
local function block(i)
  local result = {}
  local j = i + 1
  while j <= #lines and (lines[j] == "" or lines[j]:match("^%s")) do
    j = j + 1
  end
  -- skip directive options and the blank line after the directive
  for k = i + 1, j - 1 do result[#result + 1] = lines[k] end
  local indent
  for _, l in ipairs(result) do
    if l ~= "" then
      local n = #l:match("^(%s*)")
      indent = indent and math.min(indent, n) or n
    end
  end
  indent = indent or 0
  local text = {}
  for _, l in ipairs(result) do text[#text + 1] = l:sub(indent + 1) end
  while #text > 0 and text[#text] == "" do text[#text] = nil end
  while #text > 0 and text[1] == "" do table.remove(text, 1) end
  return text
end

test("all files used by the chapter exist, and all examples are used", function ()
  local referenced = {}
  for _, line in ipairs(lines) do
    local f = line:match("^%.%. literalinclude:: examples/(%S+)")
    f = f or line:match(":download:`[^<]*<examples/([^>]+)>`")
    if f then
      referenced[f] = true
      local fd = io.open(exampledir .. "/" .. f, "rb")
      check(fd, "the chapter uses the missing file examples/" .. f)
      fd:close()
    end
  end
  for _, f in ipairs(ipe.directory(exampledir)) do
    if f:match("%.lua$") then
      check(referenced[f], "examples/" .. f .. " is not used in the chapter")
    end
  end
end)

test("all examples are tested", function ()
  for _, f in ipairs(ipe.directory(exampledir)) do
    if f:match("%.lua$") then check(used[f], "examples/" .. f .. " is not tested") end
  end
end)

test("Lua code blocks and all examples compile", function ()
  local count = 0
  for i, line in ipairs(lines) do
    if line:match("^%.%. code%-block:: lua%s*$") then
      local code = table.concat(block(i), "\n")
      local f, err = load(code, "code block at line " .. i)
      check(f, err)
      count = count + 1
    end
  end
  check(count > 0, "no Lua code blocks found")
  for _, f in ipairs(ipe.directory(exampledir)) do
    if f:match("%.lua$") then
      local chunk, err = loadfile(exampledir .. "/" .. f)
      check(chunk, err)
    end
  end
end)

-- Run the commands shown in console blocks.  The files named in the
-- command are put into a temporary directory: figure.ipe is the test
-- document, and other .lua files are examples (X.lua can be the example
-- ipelet-X.lua).  A command that is followed by output must print it.
local function runDocCommand(command, expected)
  local words = {}
  for w in command:gmatch("%S+") do words[#words + 1] = w end
  check(words[1] == "ipescript", "unexpected command: " .. command)
  local dir = tmp("doccmd")
  if ipe.fileExists(dir) then removeDirectory(dir) end
  assert(ipe.mkdir(dir))
  copyfile(tmp("figure.ipe"), dir .. "/figure.ipe")
  for k = 2, #words do
    local w = words[k]
    if w:match("%.lua$") then
      for _, candidate in ipairs({ w, "ipelet-" .. w }) do
	local fd = io.open(exampledir .. "/" .. candidate, "rb")
	if fd then
	  fd:close()
	  used[candidate] = true
	  copyfile(exampledir .. "/" .. candidate, dir .. "/" .. w)
	  break
	end
      end
    end
  end
  local output, status = shell(quote(ipescript) .. command:sub(#"ipescript" + 1), dir)
  removeDirectory(dir)
  if #expected == 0 then
    -- without output, we only check that the command works
    check(status == 0, "command failed: " .. command .. "\n" .. output)
  else
    local actual = {}
    for line in output:gmatch("([^\n]*)\n") do actual[#actual + 1] = line end
    local e, a = table.concat(expected, "\n"), table.concat(actual, "\n")
    if e ~= a then
      error("output of '" .. command .. "' differs from the manual.\n--- manual:\n"
	    .. e .. "\n--- actual:\n" .. a)
    end
  end
end

-- A console block is either a list of commands (without prompt), or a
-- command with a "$ " prompt followed by its output.
test("commands in the console blocks of the chapter work", function ()
  local count = 0
  for i, line in ipairs(lines) do
    if line:match("^%.%. code%-block:: console%s*$") then
      local b = block(i)
      if b[1] and b[1]:match("^%$ ") then
	local command = b[1]:sub(3)
	local expected = { table.unpack(b, 2) }
	if command:match("^ipescript ") then
	  runDocCommand(command, expected)
	  count = count + 1
	end
      else
	for _, command in ipairs(b) do
	  if command:match("^ipescript ") then
	    runDocCommand(command, {})
	    count = count + 1
	  end
	end
      end
    end
  end
  check(count >= 4, "found only " .. count .. " ipescript commands in the chapter")
end)

----------------------------------------------------------------------

cleanup()
print(string.format("%d tests, %d failures", tests, failures))
os.exit(failures == 0 and 0 or 1)
