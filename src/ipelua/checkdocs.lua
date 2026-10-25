-- --------------------------------------------------------------------
-- Check that all Lua bindings implemented in C++ are mentioned in
-- bindings.txt.
--
-- This is a static check: it reads the luaL_Reg tables from the C++
-- sources and looks for each registered name in the part of
-- bindings.txt that documents the corresponding class.  A name counts
-- as documented if it appears as ":name" or ".name" in that part
-- (for functions of the "ipe" and "ipeui" tables, as "ipe.name" or
-- "ipeui.name" anywhere in the file).  It cannot check that the
-- documentation is correct, only that no binding has been forgotten.
--
-- Usage:   lua5.4 ipelua/checkdocs.lua    or    make -C ipelua checkdocs
-- Exit status is 1 if undocumented bindings were found.
-- --------------------------------------------------------------------

-- this script lives in the ipelua directory, next to the other source directories
local srcdir = ((arg and arg[0] or ""):match("^(.*)/[^/]*$") or ".") .. "/.."

local function readfile(name)
  local f = assert(io.open(srcdir .. "/" .. name, "rb"))
  local s = f:read("a")
  f:close()
  return s
end

-- --------------------------------------------------------------------
-- Which array documents where.
-- 'where' lists ids of \section or \page commands in bindings.txt.
-- 'extra' lists methods that are added from Lua code inside C++.
-- --------------------------------------------------------------------

local classes = {
  { "vector", "ipelua/ipeluageo.cpp", "vector_methods", {"luavector"} },
  { "matrix", "ipelua/ipeluageo.cpp", "matrix_methods", {"luamatrix"} },
  { "rect", "ipelua/ipeluageo.cpp", "rect_methods", {"luarect"} },
  { "line", "ipelua/ipeluageo.cpp", "line_methods", {"lualine"} },
  { "segment", "ipelua/ipeluageo.cpp", "segment_methods", {"luasegment"} },
  { "bezier", "ipelua/ipeluageo.cpp", "bezier_methods", {"luabezier"} },
  { "arc", "ipelua/ipeluageo.cpp", "arc_methods", {"luaarc"} },
  { "object", "ipelua/ipeluaobj.cpp", "object_methods", {"luaobj"} },
  { "page", "ipelua/ipeluapage.cpp", "page_methods", {"luapage1"} },
  { "sheet", "ipelua/ipeluastyle.cpp", "sheet_methods", {"luastyle"} },
  { "cascade", "ipelua/ipeluastyle.cpp", "cascade_methods", {"luacascade"} },
  { "document", "ipelua/ipelib.cpp", "document_methods", {"luadoc"} },
  { "ipelet", "ipelua/ipeluaipelet.cpp", "ipelet_methods", {"luaother"} },
  { "appui", "ipe/uilua.cpp", "appui_methods", {"luaappui"} },
  { "dialog", "ipeui/ipeui_common.cpp", "dialog_methods", {"luaipeuidialogs"},
    extra = {"execute"} },
  { "menu", "ipeui/ipeui_common.cpp", "menu_methods", {"luaipeuimenus"},
    extra = {"execute"} },
  { "timer", "ipeui/ipeui_common.cpp", "timer_methods", {"luaipeuitimers"} },
}

-- Tables of global functions: name of table, files, array name.
local functions = {
  { "ipe", {"ipelua/ipelib.cpp"}, "ipelib_functions" },
  { "ipeui", {"ipeui/ipeui_qt.cpp", "ipeui/ipeui_gtk.cpp", "ipeui/ipeui_win.cpp",
              "ipeui/ipeui_cocoa.cpp", "ipeui/ipeui_js.cpp"}, "ipeui_functions" },
}

-- Registered names that need no documentation, with the reason.
-- Names starting with "__" (metamethods) are always skipped, as they
-- are documented as operators.  Names ending in "Async" are skipped as well:
-- they are wrapped by Lua code that does the yield and resume.
local ignored = {
}

-- --------------------------------------------------------------------

local function registeredNames(source, array)
  local body = source:match("luaL_Reg%s+" .. array .. "%[%]%s*=%s*(%b{})")
  if not body then return nil end
  local names = {}
  for name in body:gmatch('{%s*"([^"]+)"%s*,') do
    names[#names + 1] = name
  end
  return names
end

-- Text of the \section or \page with the given id, including subsections.
local doctext = readfile("ipelua/bindings.txt")

local function docPart(id)
  local startPos
  for pos, kind, name in doctext:gmatch("()\\(%a+)%s+(%w+)") do
    if (kind == "section" or kind == "page") and name == id then
      startPos = pos
      break
    end
  end
  assert(startPos, "no \\section or \\page " .. id .. " in bindings.txt")
  local endPos = #doctext
  local rest = startPos + 1
  while true do
    local s, _, kind = doctext:find("\n(\\%a+)", rest)
    local c = doctext:find("\n/%*!", rest)
    if c and (not s or c < s) then
      endPos = c
      break
    end
    if not s then break end
    if kind == "\\section" or kind == "\\page" then
      endPos = s
      break
    end
    rest = s + 1
  end
  return doctext:sub(startPos, endPos)
end

local function skipped(owner, name)
  return name:sub(1, 2) == "__" or name:match("Async$") or ignored[owner .. "." .. name]
end

local function mentioned(text, name, prefix)
  local escaped = name:gsub("%p", "%%%0")
  if prefix then
    return text:find(prefix .. "%." .. escaped .. "%f[^%w_]") ~= nil
  end
  return text:find("[:%.]" .. escaped .. "%f[^%w_]") ~= nil
end

local problems, checked = 0, 0

local function report(owner, name)
  problems = problems + 1
  print(string.format("undocumented: %s.%s", owner, name))
end

local sources = {}
local function source(file)
  sources[file] = sources[file] or readfile(file)
  return sources[file]
end

for _, c in ipairs(classes) do
  local owner, file, array, ids = c[1], c[2], c[3], c[4]
  local names = registeredNames(source(file), array)
  if not names then
    problems = problems + 1
    print(string.format("cannot find luaL_Reg %s in %s", array, file))
  else
    local text = ""
    for _, id in ipairs(ids) do text = text .. docPart(id) end
    for _, extra in ipairs(c.extra or {}) do names[#names + 1] = extra end
    for _, name in ipairs(names) do
      if not skipped(owner, name) then
        checked = checked + 1
        if not mentioned(text, name) then report(owner, name) end
      end
    end
  end
end

for _, f in ipairs(functions) do
  local owner, files, array = f[1], f[2], f[3]
  -- backends differ in which functions they provide (for example,
  -- some have synchronous and some asynchronous variants), so use the union
  local union = {}
  for _, file in ipairs(files) do
    local names = registeredNames(source(file), array)
    if not names then
      problems = problems + 1
      print(string.format("cannot find luaL_Reg %s in %s", array, file))
    else
      for _, name in ipairs(names) do union[name] = true end
    end
  end
  local sorted = {}
  for name in pairs(union) do sorted[#sorted + 1] = name end
  table.sort(sorted)
  for _, name in ipairs(sorted) do
    if not skipped(owner, name) then
      checked = checked + 1
      if not mentioned(doctext, name, owner) then report(owner, name) end
    end
  end
end

print(string.format("Checked %d bindings, %d problems", checked, problems))
os.exit(problems == 0 and 0 or 1)
