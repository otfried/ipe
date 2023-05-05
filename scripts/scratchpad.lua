#!/usr/bin/env ipescript 
-- -*- lua -*-
----------------------------------------------------------------------
-- Create a copy of an Ipe presentation for presentation inside Ipe
----------------------------------------------------------------------
--
-- Running this script as "ipescript scratchpad <input> <output>" will
-- create <output> as a copy of the Ipe document <input>,
-- but adding a new layer "scratchpad" to all pages of the input,
-- making this layer the active layer of every view,
-- and locking all input layers.
--
----------------------------------------------------------------------

if #argv ~= 2 then
  io.stderr:write("Usage: ipescript scratchpad <input> <output>\n")
  return
end

local inname = argv[1]
local outname = argv[2]

local doc = assert(ipe.Document(inname))

for pno = 1,#doc do
  print("Page: ", pno)
  local p = doc[pno]
  p:addLayer("scratchpad")
  for vno = 1,p:countViews() do
    p:setActive(vno, "scratchpad")
    p:setVisible(vno, "scratchpad", true)
  end
  for i, l in ipairs(p:layers()) do
    if l ~= "scratchpad" then
      p:setLocked(l, true)
    end
  end
end

doc:save(outname)
