#!/usr/bin/env ipescript 
-- -*- lua -*-
----------------------------------------------------------------------
-- Update stylesheets in Ipe documents
----------------------------------------------------------------------
--
-- Running this script as "ipescript update-styles <figures>" will
-- update the stylesheets in the Ipe figures given.
--
-- To update a stylesheet, the script will look for a file with the
-- same name as the stylesheet plus the extension ".isy".  It searches
-- for this file in the current directory, plus all the directories
-- given in the IPESTYLES environment variable (with the same default
-- settings as in Ipe).
--
----------------------------------------------------------------------

local ipestyles = os.getenv("IPESTYLES")
local home = os.getenv("HOME")

if ipestyles then
  styleDirs = { "." }
  for w in string.gmatch(ipestyles, "[^:;]+") do
    if w == "_" then w = config.system_styles end
    styleDirs[#styleDirs + 1] = w
  end
else
  styleDirs = { ".", config.system_styles }
  if config.platform ~= "win" then
    table.insert(styleDirs, 2, home .. "/.ipe/styles")
    if config.platform == "apple" then
      table.insert(styleDirs, 3, home .. "/Library/Ipe/Styles")
    end
  end
end

function findStyle(w)
  for _, d in ipairs(styleDirs) do
    local s = d .. "/" .. w
    if ipe.fileExists(s) then return s end
  end
end

--------------------------------------------------------------------

if #argv == 0 then
  io.stderr:write("Usage: ipescript update-styles <figures>\n")
  return
end

local fignames = argv

for _,figname in ipairs(fignames) do
  io.stderr:write("Updating styles in figure '" .. figname .. "'\n")
  local doc = assert(ipe.Document(figname))
  for index=1,doc:sheets():count() do
    local sheet = doc:sheets():sheet(index)
    local name = sheet:name()
    if not name then
      io.stderr:write(" - unnamed stylesheet\n")
    elseif name == "standard" then
      io.stderr:write(" - standard stylesheet\n")
    else
      io.stderr:write(" - stylesheet '" .. name .. "'\n")
      local s = findStyle(name .. ".isy")
      if s then
	io.stderr:write("     updating from '" .. s .."'\n")
	local nsheet = assert(ipe.Sheet(s))
	doc:sheets():insert(index, nsheet)
	doc:sheets():remove(index + 1) -- remove old sheet
      end
    end
  end
  if figname:sub(-4) ~= ".ipe" and figname:sub(-4) ~= ".xml" then
    assert(doc:runLatex())
  end
  -- make a backup of original figure
  local f = assert(io.open(figname, "rb"))
  local data = f:read("*all")
  f:close()
  f = assert(io.open(figname .. ".bak", "wb"))
  f:write(data)
  f:close()
  -- now write updated figure back
  doc:save(figname)
end
