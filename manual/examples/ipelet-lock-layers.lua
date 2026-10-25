----------------------------------------------------------------------
-- An ipelet with several methods, and an undo item with explicit
-- undo and redo functions
----------------------------------------------------------------------
--
-- Cloning the whole page is simple, but wasteful when only a small
-- thing changes.  Here the undo item remembers just what is needed to
-- go back: which layers were locked before.

label = "Lock layers"

about = [[
Lock or unlock all layers of the current page.
]]

-- The method is called with the model and the number of the method.
local function lock_layers(model, num)
  local locked = (num == 1)
  local page = model:page()
  local was_locked = {}
  for _, layer in ipairs(page:layers()) do
    was_locked[layer] = page:isLocked(layer)
  end

  local t = { label = locked and "lock all layers" or "unlock all layers",
	      pno = model.pno,
	      vno = model.vno,
	      locked = locked,
	      was_locked = was_locked,
	    }
  -- redo is also called to perform the action the first time
  t.redo = function (t, doc)
    for layer in pairs(t.was_locked) do
      doc[t.pno]:setLocked(layer, t.locked)
    end
  end
  t.undo = function (t, doc)
    for layer, was in pairs(t.was_locked) do
      doc[t.pno]:setLocked(layer, was)
    end
  end
  model:register(t)
end

methods = {
  { label = "Lock all layers", run = lock_layers },
  { label = "Unlock all layers", run = lock_layers },
}
