#!/usr/bin/env ipescript 
-- -*- lua -*-
----------------------------------------------------------------------
-- Add stylesheet
----------------------------------------------------------------------
--
-- Run this script as "ipescript add-style <sheet.isy> <figure>"
-- 
-- It adds the stylesheet <sheet.isy> to the Ipe document <figure>.
--
----------------------------------------------------------------------

if #argv ~= 2 then
  io.stderr:write("Usage: ipescript add-style <sheet.isy> <figure>\n")
  return
end

local sheetname = argv[1]
local figname = argv[2]

local nsheet = assert(ipe.Sheet(sheetname))
local doc = assert(ipe.Document(figname))
doc:sheets():insert(1, nsheet)
assert(doc:runLatex())

-- make a backup of original
local f = assert(io.open(figname, "rb"))
local data = f:read("*all")
f:close()
f = assert(io.open(figname .. ".bak", "wb"))
f:write(data)
f:close()

-- now write updated figure back
doc:save(figname)
