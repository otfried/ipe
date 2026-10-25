#!/usr/bin/env ipescript 
-- -*- lua -*-
----------------------------------------------------------------------
-- Run a Lua ipelet on an Ipe document without starting the Ipe editor
----------------------------------------------------------------------
--
-- Usage:
--   ipescript run-ipelet [options] <ipelet.lua> <input.ipe> <output.ipe> [name=value ...]
--
-- This script loads the ipelet in the same restricted environment that
-- Ipe uses, calls it with a "model" object that behaves like the model
-- of the Ipe editor (using the real model.lua of Ipe, including undo
-- and redo), and saves the resulting document as <output>.  It then
-- undoes and redoes the ipelet's changes, and reports if the page is not
-- restored exactly -- the most common mistake in ipelets is modifying
-- the document without registering a proper undo item.
--
-- Options:
--   --method N    run method number N of the ipelet (default 1)
--   --page N      current page (default 1)
--   --view N      current view (default 1)
--   --select S    objects selected before running: "all" (default), "none",
--                 or a comma-separated list of object numbers
--   --cancel      dialogs are canceled instead of accepted
--
-- Dialogs cannot be shown.  Instead, a dialog behaves as if the user had
-- accepted it, with every element still having the value set by the
-- ipelet.  Arguments of the form name=value give the value of the dialog
-- element with that name instead: checkbox elements accept "true" or
-- "false", list and combo elements the item number, and all others a
-- string.
--
-- This is a testing tool: it has no canvas, no stylesheet selectors, and
-- no LaTeX, so ipelets that depend on these cannot be tested fully.
--
----------------------------------------------------------------------

local function usage()
  io.stderr:write("Usage: ipescript run-ipelet [options] <ipelet.lua> <input.ipe> ",
		  "<output.ipe> [name=value ...]\n",
		  "Options: --method N, --page N, --view N, --select S, --cancel\n")
  os.exit(1)
end

local opts = { method = "1", page = "1", view = "1", select = "all" }
local positional = {}
local answers = {}

do
  local i = 1
  while i <= #argv do
    local a = argv[i]
    if a == "--cancel" then
      opts.cancel = true
    elseif a:sub(1, 2) == "--" then
      local name = a:sub(3)
      if opts[name] == nil or i == #argv then usage() end
      opts[name] = argv[i + 1]
      i = i + 1
    elseif #positional < 3 then
      positional[#positional + 1] = a
    else
      local name, value = a:match("^([^=]+)=(.*)$")
      if not name then usage() end
      answers[name] = value
    end
    i = i + 1
  end
end
if #positional ~= 3 then usage() end
local ipeletName, inname, outname = positional[1], positional[2], positional[3]

----------------------------------------------------------------------
-- Environment of the Ipe editor that ipelets and model.lua depend on
----------------------------------------------------------------------

local luadir = ipe.folder("lua")
shortcuts = {}
mouse = {}
dofile(luadir .. "/prefs.lua")
dofile(luadir .. "/model.lua")

function revertOriginal(t, doc)
  doc:set(t.pno, t.original)
end

function revertFinal(t, doc)
  doc:set(t.pno, t.final)
end

function messageBox(parent, type, text, details, buttons)
  print("[messageBox] " .. text .. (details and ("\n" .. details) or ""))
  return 1
end

-- A dialog that cannot be shown, but behaves as if accepted
local Dialog = {}
Dialog.__index = Dialog

ipeui = {}

function ipeui.Dialog(parent, caption)
  print("[dialog] " .. tostring(caption))
  return setmetatable({ kinds = {}, items = {}, values = {} }, Dialog)
end

function Dialog:add(name, kind, options)
  self.kinds[name] = kind
  if kind == "checkbox" then
    self.values[name] = false
  elseif kind == "list" or kind == "combo" then
    self.items[name] = { table.unpack(options) }
    self.values[name] = 1
  elseif kind == "input" or kind == "text" then
    self.values[name] = ""
  end
end

function Dialog:addButton() end
function Dialog:setStretch() end
function Dialog:setEnabled() end
function Dialog:accept() end

function Dialog:set(name, value)
  if name == "ignore-escape" then return end
  local kind = self.kinds[name]
  assert(kind, "no such element in dialog: " .. tostring(name))
  if kind == "list" or kind == "combo" then
    if type(value) == "table" then
      self.items[name] = { table.unpack(value) }
      self.values[name] = 1
    elseif type(value) == "string" then
      for i, item in ipairs(self.items[name]) do
        if item == value then self.values[name] = i end
      end
    else
      self.values[name] = value
    end
  else
    self.values[name] = value
  end
end

function Dialog:get(name)
  local kind = self.kinds[name]
  assert(kind == "input" or kind == "text" or kind == "checkbox" or kind == "list"
	   or kind == "combo", "no suitable element in dialog: " .. tostring(name))
  local answer = answers[name]
  if answer ~= nil then
    if kind == "checkbox" then return answer == "true" or answer == "1" end
    if kind == "list" or kind == "combo" then return tonumber(answer) end
    return answer
  end
  return self.values[name]
end

function Dialog:execute()
  local names = {}
  for name in pairs(self.kinds) do names[#names + 1] = name end
  table.sort(names)
  for _, name in ipairs(names) do
    if self.kinds[name] ~= "label" and self.kinds[name] ~= "image" then
      print("[dialog]   " .. name .. " = " .. tostring(self:get(name)))
    end
  end
  return not opts.cancel
end

-- The user interface: no canvas, so everything is ignored
local ui = setmetatable({}, { __index = function (t, name)
  return function () end
end })
function ui.explain(self, text) print("[explain] " .. text) end
function ui.win() return nil end
function ui.pos() return ipe.Vector(0, 0) end
function ui.zoom() return 1.0 end
function ui.clipboard() return nil end

----------------------------------------------------------------------
-- Load ipelet and document
----------------------------------------------------------------------

local ipelet = { name = ipeletName:match("([^/]+)%.lua$") or ipeletName,
		 path = ipeletName, _G = _G, ipe = ipe, ipeui = ipeui,
		 math = math, string = string, table = table, assert = assert,
		 shortcuts = shortcuts, prefs = prefs, config = config,
		 mouse = mouse, ipairs = ipairs, pairs = pairs, print = print,
		 tonumber = tonumber, tostring = tostring }
do
  local fd = assert(io.open(ipeletName, "rb"))
  local f = assert(load(function () return fd:read("L") end, ipeletName, "bt", ipelet))
  fd:close()
  f()
end

local method = tonumber(opts.method)
local run
if ipelet.methods then
  assert(ipelet.methods[method], "the ipelet has no method number " .. method)
  run = ipelet.methods[method].run or ipelet.run
else
  run = ipelet.run
end
assert(run, "the ipelet defines no run function")

local doc, err = ipe.Document(inname)
if not doc then
  io.stderr:write("Cannot load '" .. inname .. "': " .. tostring(err) .. "\n")
  os.exit(1)
end

local model = setmetatable({}, MODEL)
model.doc = doc
model.pno = tonumber(opts.page)
model.vno = tonumber(opts.view)
model.file_name = inname
model.ui = ui
model.attributes = {}
for k, v in pairs(prefs.initial_attributes) do model.attributes[k] = v end
model.snap = { gridsize = prefs.initial.grid_size, anglesize = prefs.initial.angle_size,
	       snap_distance = prefs.snap_distance, origin = ipe.Vector(0, 0),
	       orientation = 0, with_axes = false }
model.save_timestamp = 0
model.pristine = false
model.undo = { { label = "original document" } }
model.redo = {}
model.recent_files = {}
assert(model.pno >= 1 and model.pno <= #doc, "no such page")
assert(model.vno >= 1 and model.vno <= model:page():countViews(), "no such view")

local page = model:page()
page:deselectAll()
if opts.select == "all" then
  for i = 1, #page do page:setSelect(i, 2) end
elseif opts.select ~= "none" then
  for n in opts.select:gmatch("%d+") do
    assert(tonumber(n) <= #page, "no object number " .. n)
    page:setSelect(tonumber(n), 2)
  end
end
page:ensurePrimarySelection()

----------------------------------------------------------------------
-- Run, save, and check undo and redo
----------------------------------------------------------------------

local function pageXml(d)
  return d[model.pno]:xml("ipepage")
end

local before = pageXml(doc)
print(string.format("Running %s%s on page %d, view %d with %d selected objects",
		    ipelet.name, ipelet.methods and (" method " .. method) or "",
		    model.pno, model.vno, #model:selection()))
local ok, msg = xpcall(run, debug.traceback, model, method)
if not ok then
  io.stderr:write("The ipelet failed:\n" .. tostring(msg) .. "\n")
  os.exit(1)
end

local after = pageXml(doc)
local status = 0
if #model.undo == 1 then
  print("The ipelet did not register an undo item.")
  if after ~= before then
    print("WARNING: the page has been modified without registering an undo item!")
    status = 1
  end
else
  print(string.format("%d undo item(s) registered: %s", #model.undo - 1,
		      table.concat((function ()
			local labels = {}
			for i = 2, #model.undo do labels[#labels + 1] = model.undo[i].label end
			return labels
		      end)(), "; ")))
  -- the real undo and redo actions of Ipe
  local saved = pageXml(doc)
  doc:save(outname)
  for _ = 2, #model.undo do model:action_undo() end
  if pageXml(doc) == before then
    print("Undo restores the original page: OK")
  else
    print("WARNING: undo does not restore the original page!")
    status = 1
  end
  for _ = 1, #model.redo do model:action_redo() end
  if pageXml(doc) == saved then
    print("Redo restores the result: OK")
  else
    print("WARNING: redo does not restore the result of the ipelet!")
    status = 1
  end
  print("Saved " .. outname)
  os.exit(status)
end
doc:save(outname)
print("Saved " .. outname)
os.exit(status)
