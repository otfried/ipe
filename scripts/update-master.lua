#!/usr/bin/env ipescript 
-- -*- lua -*-
----------------------------------------------------------------------
-- Update master-preamble stylesheet
----------------------------------------------------------------------
--
-- When using Ipe figures in a Latex document, it is convenient
-- to have access to some of the definitions from the document.
--
-- This Lua script makes this easy: In your Latex document
-- (e.g. master.tex), surround the interesting definitions using
-- %%BeginIpePreamble and %%EndIpePreamble, e.g. like this:
--
-- %%BeginIpePreamble
-- \usepackage{amsfonts}
-- \newcommand{\R}{\mathbb{R}}
-- %%EndIpePreamble
--
-- Running this script as "ipescript update-master master.tex" will
-- extract the Ipe definitions, and save them as a stylesheet
-- "master-preamble.isy"
--
-- Running this script as "ipescript update-master master.tex figures/*.ipe"
-- will in addition look at all the Ipe figures, and either add
-- "master-preamble.isy" to it, or update the stylesheet to the newest
-- version.
--
----------------------------------------------------------------------

if #argv == 0 then
  io.stderr:write("Usage: ipescript update-master <texfile> [ <figures> ]\n")
  return
end

local texname = argv[1]

local fignames = argv
table.remove(fignames, 1)

local f = io.open(texname, "r")
text = f:read("*all")
f:close()

local mat = text:match("%%%%BeginIpePreamble(.-)%%%%EndIpePreamble")
if not mat then
  io.stderr:write("No Ipe definitions found in '" .. texname .. "'\n")
  return
end

local out = io.open("master-preamble.isy", "w")
out:write('<ipestyle name="master-preamble">\n<preamble>\n')
for mat in text:gmatch("%%%%BeginIpePreamble(.-)%%%%EndIpePreamble") do
    out:write(mat)
end
out:write("</preamble>\n</ipestyle>\n")
out:close()

local nsheet = assert(ipe.Sheet("master-preamble.isy"))

io.stderr:write("Extracted definitions and created 'master-preamble.isy'\n")

for _,figname in ipairs(fignames) do
  io.stderr:write("Checking figure '" .. figname .. "'\n")
  local doc = assert(ipe.Document(figname))
  local index = nil
  for i=1,doc:sheets():count() do
    local sheet = doc:sheets():sheet(i)
    if sheet:name() == "master-preamble" then
      index = i
      break
    end
  end
  if index then
    io.stderr:write("Found 'master-preamble' stylesheet, updating it.\n")
    doc:sheets():insert(index, nsheet:clone())
    doc:sheets():remove(index + 1) -- remove old copy
  else
    io.stderr:write("Adding 'master-preamble' stylesheet.\n")
    doc:sheets():insert(1, nsheet:clone())
  end
  if figname:sub(-4) ~= ".ipe" and figname:sub(-4) ~= ".xml" then
    assert(doc:runLatex())
  end
  -- make a backup of original
  local f = assert(io.open(figname, "rb"))
  local data = f:read("*all")
  f:close()
  f = assert(io.open(figname .. ".bak", "wb"))
  f:write(data)
  f:close()
  -- now write updated figure back
  doc:save(figname)
end
