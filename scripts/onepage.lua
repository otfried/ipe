#!/usr/bin/env ipescript 
-- -*- lua -*-
----------------------------------------------------------------------
-- Collapse pages of an Ipe document into a single page
----------------------------------------------------------------------
--
-- Running this script as "ipescript onepage <input> <output>" will
-- create <output> as a copy of the Ipe document <input>, but with
-- all pages of <input> collapsed into layers of a single page in
-- <output>.
--
----------------------------------------------------------------------

if #argv ~= 2 then
  io.stderr:write("Usage: ipescript onepage <input> <output>\n")
  return
end

local inname = argv[1]
local outname = argv[2]

local doc = assert(ipe.Document(inname))
local ndoc = ipe.Document()

local s = doc:sheets()
ndoc:replaceSheets(s:clone())

local t = doc:properties()
ndoc:setProperties(t)

local np = ndoc[1]

for pno = 1,#doc do
  print("Page: ", pno)
  local layer = "page_" .. pno
  np:addLayer(layer)
  np:setVisible(1, layer, true)
  local p = doc[pno]
  for _, obj, _, _ in p:objects() do
    np:insert(nil, obj:clone(), nil, layer)
  end
end

np:removeLayer("alpha")
ndoc:save(outname)
