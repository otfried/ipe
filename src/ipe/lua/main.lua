----------------------------------------------------------------------
-- Ipe
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

-- order is important
require "prefs"
require "model"
require "actions"
require "tools"
require "editpath"
require "properties"
require "shortcuts"
require "mouse"

----------------------------------------------------------------------

-- short names
V = ipe.Vector

-- store the model for the first window
-- Used only by OSX file_open_event
-- must be global, as it's changed from model.lua
first_model = nil

----------------------------------------------------------------------

function printTable(t)
  for k in pairs(t) do
    print(k, t[k])
  end
end

-- only used for saving
function formatFromFileName(fname)
  local s = string.lower(fname:sub(-4))
  if s == ".xml" or s == ".ipe" then return "xml" end
  if s == ".pdf" then return "pdf" end
  return nil
end

function revertOriginal(t, doc)
  doc:set(t.pno, t.original)
end

function revertFinal(t, doc)
  doc:set(t.pno, t.final)
end

function indexOf(el, list)
  for i,n in ipairs(list) do
    if n == el then return i end
  end
  return nil
end

function symbolNames(sheet, prefix, postfix)
  local list = sheet:allNames("symbol")
  local result = {}
  for _, n in ipairs(list) do
    if n:sub(1, #prefix) == prefix and n:sub(-#postfix) == postfix then
      result[#result + 1] = n
    end
  end
  return result
end

function stripPrefixPostfix(list, m, mm)
  local result = {}
  for _, n in ipairs(list) do
    result[#result + 1] = n:sub(m, -mm-1)
  end
  return result
end

function arrowshapeToName(i, s)
  return s:match("^arrow/(.+)%(s?f?p?x%)$")
end

function colorString(color)
  if type(color) == "string" then return color end
  -- else must be table
  return string.format("(%g,%g,%g)", color.r, color.g, color.b)
end

function extractElements(p, selection)
  local r = {}
  local l = {}
  for i,j in ipairs(selection) do
    r[#r + 1] = p[j-i+1]:clone()
    l[#l + 1] = p:layerOf(j-i+1)
    p:remove(j-i+1)
  end
  return r, l
end

-- make a list of all values from stylesheets
function allValues(sheets, kind)
  local syms = sheets:allNames(kind)
  local values = {}
  for _,sym in ipairs(syms) do
    values[#values + 1] = sheets:find(kind, sym)
  end
  return values
end

-- apply transformation to a shape
function transformShape(matrix, shape)
  local result = {}
  for _,path in ipairs(shape) do
    if path.type == "ellipse" or path.type == "closedspline" then
      for i = 1,#path do
	path[i] = matrix * path[i]
      end
    else -- must be "curve"
      for _,seg in ipairs(path) do
	for i = 1,#seg do
	  seg[i] = matrix * seg[i]
	end
	if seg.type == "arc" then
	  seg.arc = matrix * seg.arc
	end
      end
    end
  end
end

-- recompute matrix of an arc after an endpoint has been moved
function recomputeArcMatrix(seg, cpno)
  local alpha, beta = seg.arc:angles()
  local p = ipe.Direction(alpha)
  local q = ipe.Direction(beta)
  local cl
  if (seg[2]-seg[1]):sqLen() == 0 then return end
  if cpno == 1 then
    cl = ipe.LineThrough(V(0,0), q)
    p = seg.arc:matrix():inverse() * seg[cpno]
  else -- cpno == 2
    cl = ipe.LineThrough(V(0,0), p)
    q = seg.arc:matrix():inverse() * seg[cpno]
  end
  local center = ipe.Bisector(p, q):intersects(cl)
  if center and center:sqLen() < 1e10 then
    local radius = (p - center):len()
    alpha = (p - center):angle()
    beta = (q - center):angle()
    local m = (seg.arc:matrix() *
	     ipe.Matrix(radius, 0, 0, radius, center.x, center.y))
    seg.arc = ipe.Arc(m, alpha, beta)
  end
end

function findStyle(w, dir)
  if dir and ipe.fileExists(dir .. prefs.fsep .. w) then
    return dir .. prefs.fsep .. w
  end
  for _, d in ipairs(config.styleDirs) do
    local s = d .. prefs.fsep .. w
    if ipe.fileExists(s) then return s end
  end
end

-- show a message box
-- type is one of "none" "warning" "information" "question" "critical"
-- details may be nil
-- buttons may be nil (for "ok") or one of
-- "ok" "okcancel" "yesnocancel", "discardcancel", "savediscardcancel",
-- or a number from 0 to 4 corresponding to these
-- return 1 for ok/yes/save, 0 for no/discard, -1 for cancel
function messageBox(parent, type, text, details, buttons)
  if config.toolkit == "win" and buttons and
    (buttons == 3 or buttons == 4 or buttons == "discardcancel"
     or buttons == "savediscardcancel") then
    -- native Windows messagebox does not support these
    -- so we build our own dialog - this one has no icon, though
    local result = 0
    local d = ipeui.Dialog(parent, "Ipe")
    d:add("text", "label", {label=text .. "\n\n" .. details}, 1, 1)
    if buttons == 4 or buttons == "savediscardcancel" then
      d:addButton("ok", "&Save", function (d) result=1; d:accept(true) end)
    end
    d:addButton("discard", "&Discard", "accept")
    d:addButton("cancel", "&Cancel", "reject")
    local r = d:execute()
    if r then
      return result
    else
      return -1
    end
  else
    return ipeui.messageBox(parent, type, text, details, buttons)
  end
end

filter_ipe = { "Ipe files (*.ipe *.pdf *.eps *.xml)",
	       "*.ipe;*.pdf;*.eps;*.xml",
	       "All files (*.*)", "*.*" }
filter_save = { "XML (*.ipe *.xml)", "*.ipe;*.xml",
		"PDF (*.pdf)", "*.pdf" }
filter_stylesheets = { "Ipe stylesheets (*.isy)", "*.isy" }
if config.platform == "win" then
  filter_images = { "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)",
		    "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tiff" }
else
  filter_images = { "Images (*.png *.jpg *.jpeg)", "*.png;*.jpg;*.jpeg" }
end
filter_png = { "Images (*.png)", "*.png" }
filter_eps = { "Postscript files (*.eps)", "*.eps" }
filter_svg = { "SVG files (*.svg)", "*.svg" }

----------------------------------------------------------------------
-- This function is called to launch a file on MacOS X

function file_open_event(fname)
  if first_model and first_model.pristine then
    first_model:loadDocument(fname)
    first_model:action_fit_top()
  else
    local m = MODEL.new(nil, fname)
    m:action_fit_top()
  end
end

----------------------------------------------------------------------

-- msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx

local win32_conversions = {
  PgDown=0x22, PgUp=0x21, Home=0x24, End=0x23,
  Left=0x25, Up=0x26, Right=0x27, Down=0x28,
  insert=0x2d, delete=0x2e
}

function win32_shortcut_convert(s)
  local k = 0
  local done = false
  while not done do
    if s:sub(1,5) == "Ctrl+" then
      s = s:sub(6)
      k = k + 0x20000
    elseif s:sub(1,6) == "Shift+" then
      s = s:sub(7)
      k = k + 0x40000
    elseif s:sub(1,4) == "Alt+" then
      s = s:sub(5)
      k = k + 0x10000
    else
      done = true
    end
  end
  if s ~= "F" and s:sub(1,1) == "F" and tonumber(s:sub(2)) then
    return k + 0x6f + tonumber(s:sub(2))
  elseif win32_conversions[s] then
    return k + win32_conversions[s]
  elseif ("0" <= s and s <= "9") or ("A" <= s and s <= "Z") then
    return k + string.byte(s:sub(1,1))
  else
    return k + string.byte(s:sub(1,1)) + 0x80000
  end
end

function win32_shortcut_append(t, ts, s, id, ao)
  local sc = win32_shortcut_convert(s)
  t[#t+1] = sc
  t[#t+1] = id
  if ao then
    ts[#ts+1] = sc
    ts[#ts+1] = id
  end
end

function win32_shortcuts(ui)
  if config.toolkit ~= "win" then return end
  local accel = {}
  local accelsub = {}
  for i in pairs(shortcuts) do
    local id, alwaysOn = ui:actionInfo(i)
    local s = shortcuts[i]
    if type(s) == "table" then
      for j,s1 in ipairs(s) do
	win32_shortcut_append(accel, accelsub, s1, id, alwaysOn)
      end
    elseif s then
      win32_shortcut_append(accel, accelsub, s, id, alwaysOn)
    end
  end
  return accel, accelsub
end

----------------------------------------------------------------------

local function show_configuration()
  local s = config.version
  s = s .. "\nLua code: " .. package.path
  s = s .. "\nStyle directories: " .. table.concat(config.styleDirs, ", ")
  s = s .. "\nStyles for new documents: " .. table.concat(prefs.styles, ", ")
  s = s .. "\nAutosave file: " .. prefs.autosave_filename
  s = s .. "\nSave-as directory: " .. prefs.save_as_directory
  s = s .. "\nDocumentation: " .. config.docdir
  s = s .. "\nIpelets: " .. table.concat(config.ipeletDirs, ", ")
  s = s .. "\nLatex program path: " .. config.latexpath
  s = s .. "\nLatex directory: " .. config.latexdir
  s = s .. "\nIcons: " .. config.icons
  s = s .. "\nExternal editor: " .. (prefs.external_editor or "none")
  s = s .. "\n"
  io.stdout:write(s)
end

local function usage()
  io.stderr:write("Usage: ipe { -sheet <filename.isy> } [ <filename> ]\n")
  io.stderr:write("or:    ipe -show-configuration\n")
  io.stderr:write("or:    ipe --help\n")
end

--------------------------------------------------------------------

-- set locale so that "tonumber" will work right with decimal points
os.setlocale("C", "numeric")

local test1 = string.format("%g", 1.5)
local test2 = string.format("%g", tonumber("1.5"))
if test1 ~= "1.5" or test2 ~= "1.5" then
  m = "Formatting the number '1.5' results in '"
    .. test1 .. "'. "
    .. "Reading '1.5' results in '" .. test2 .. "'\n"
    .. "Therefore Ipe will not work correctly when loading or saving files. "
    .. "PLEASE REPORT THIS PROBLEM!\n"
    .. "As a workaround, you can start Ipe from the commandline like this: "
    .. "export LANG=C\nexport LC_NUMERIC=C\nipe"
  ipeui.messageBox(nil, "critical",
		   "Ipe is running with an incorrect locale", m)
  return
end

--------------------------------------------------------------------

local home = os.getenv("HOME")
local ipeletpath = os.getenv("IPELETPATH")
if ipeletpath then
  config.ipeletDirs = {}
  for w in string.gmatch(ipeletpath, prefs.fname_pattern) do
    if w == "_" then w = config.system_ipelets end
    if w:sub(1,4) == "ipe:" then
      w = config.ipedrive .. w:sub(5)
    end
    config.ipeletDirs[#config.ipeletDirs + 1] = w
  end
else
  config.ipeletDirs = { config.system_ipelets }
  if config.platform == "win" then
    local userdir = os.getenv("USERPROFILE")
    if userdir then
      config.ipeletDirs[#config.ipeletDirs + 1] = userdir .. "\\Ipelets"
    end
  else
    config.ipeletDirs[#config.ipeletDirs + 1] = home .. "/.ipe/ipelets"
    if config.platform == "apple" then
      config.ipeletDirs[#config.ipeletDirs + 1] = home.."/Library/Ipe/Ipelets"
    end
  end
end

local ipestyles = os.getenv("IPESTYLES")
if ipestyles then
  config.styleDirs = {}
  for w in string.gmatch(ipestyles, prefs.fname_pattern) do
    if w == "_" then w = config.system_styles end
    if w:sub(1,4) == "ipe:" then
      w = config.ipedrive .. w:sub(5)
    end
    config.styleDirs[#config.styleDirs + 1] = w
  end
else
  config.styleDirs = { config.system_styles }
  if config.platform ~= "win" then
    table.insert(config.styleDirs, 1, home .. "/.ipe/styles")
    if config.platform == "apple" then
      table.insert(config.styleDirs, 2, home .. "/Library/Ipe/Styles")
    end
  end
end

--------------------------------------------------------------------

function load_ipelets()
  for _,ft in ipairs(ipelets) do
    local fd = ipe.openFile(ft.path, "rb")
    local ff = assert(load(function () return fd:read("*L") end,
			   ft.path, "bt", ft))
    ff()
  end
end

-- look for ipelets
ipelets = {}
for _,w in ipairs(config.ipeletDirs) do
  if ipe.fileExists(w) then
    local files = ipe.directory(w)
    for i, f in ipairs(files) do
      if f:sub(-4) == ".lua" then
	ft = {}
	ft.name = f:sub(1,-5)
	ft.path = w .. prefs.fsep .. f
	ft.dllname = w .. prefs.fsep .. ft.name
	ft._G = _G
	ft.ipe = ipe
	ft.ipeui = ipeui
	ft.math = math
	ft.string = string
	ft.table = table
	ft.assert = assert
	ft.shortcuts = shortcuts
	ft.prefs = prefs
	ft.config = config
	ft.mouse = mouse
	ft.ipairs = ipairs
	ft.pairs = pairs
	ft.print = print
	ft.tonumber = tonumber
	ft.tostring = tostring
	ipelets[#ipelets + 1] = ft
      end
    end
  end
end

load_ipelets()

--------------------------------------------------------------------

recent_files = {}

function load_recent_files()
  local w = config.latexdir .. "recent_files.lua"
  if ipe.fileExists(w) then
    local r = {}
    local fd = ipe.openFile(w, "r")
    local ff = load(function () return fd:read("*L") end, w, "bt", r)
    if ff then
      ff()
      if r.recent_files then
	recent_files = r.recent_files
      end
    end
    fd:close()
  end
end

load_recent_files()

--------------------------------------------------------------------

if #argv == 1 and argv[1] == "-show-configuration" then
  show_configuration()
  return
end

if #argv == 1 and (argv[1] == "--help" or argv[1] == "-h") then
  usage()
  return
end

--------------------------------------------------------------------

local first_file = nil
local i = 1
local style_sheets = {}

while i <= #argv do
  if argv[i] == "-sheet" then
    if i == #argv then usage() return end
    style_sheets[#style_sheets + 1] = argv[i+1]
    i = i + 2
  else
    if i ~= #argv then usage() return end
    first_file = ipe.realPath(argv[i])
    i = i + 1
  end
end

-- Cocoa handles opening files itself, using open_file_event
if config.toolkit == "cocoa" then first_file = nil end

if #style_sheets > 0 then prefs.styles = style_sheets end

config.styleList = {}
for _,w in ipairs(prefs.styles) do
  if w:sub(-4) ~= ".isy" then w = w .. ".isy" end
  if not w:find(prefs.fsep) then w = findStyle(w) end
  config.styleList[#config.styleList + 1] = w
end

first_model = MODEL:new(first_file)
first_model:action_fit_top()
first_model.ui:setScreen(prefs.start_screen)

local acc, accsub = win32_shortcuts(first_model.ui)
mainloop(acc, accsub)

----------------------------------------------------------------------
