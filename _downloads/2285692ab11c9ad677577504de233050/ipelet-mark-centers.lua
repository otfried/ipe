----------------------------------------------------------------------
-- Mark the center of the bounding box of every selected object
----------------------------------------------------------------------
--
-- This ipelet creates several new objects at once.  It uses the same
-- undo item as ipelet-red-paths.lua: the original page and the final page.

label = "Mark centers"

about = [[
Create a mark at the center of the bounding box of every selected object.
]]

function run(model)
  local page = model:page()
  local layer = page:active(model.vno)
  local t = { label = "mark centers",
	      pno = model.pno,
	      vno = model.vno,
	      original = page:clone(),
	      final = page:clone(),
	      undo = _G.revertOriginal,
	      redo = _G.revertFinal,
	    }

  -- Collect the centers first: inserting objects while iterating over
  -- the page would visit the new objects as well.
  local centers = {}
  for i, obj, sel in t.final:objects() do
    if sel then
      local box = t.final:bbox(i)
      centers[#centers + 1] = (box:bottomLeft() + box:topRight()) * 0.5
    end
  end

  for _, center in ipairs(centers) do
    -- model.attributes are the attributes currently set in the user interface
    local mark = ipe.Reference(model.attributes, "mark/disk(sx)", center)
    t.final:insert(nil, mark, nil, layer)
  end

  if #centers > 0 then
    model:register(t)
  end
end
