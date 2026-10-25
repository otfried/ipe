----------------------------------------------------------------------
-- Make the selected paths red
----------------------------------------------------------------------
--
-- An ipelet must not modify the document directly.  Instead, it prepares
-- the new version of the page, and registers an undo item that switches
-- between the original and the final page.  This is the simplest way to
-- write an ipelet that changes existing objects.

label = "Red paths"

about = [[
Make the stroke color of all selected paths red.
]]

function run(model)
  local t = { label = "make paths red",
	      pno = model.pno,
	      vno = model.vno,
	      original = model:page():clone(),
	      final = model:page():clone(),
	      undo = _G.revertOriginal,
	      redo = _G.revertFinal,
	    }
  local changed = false
  for i, obj, sel, layer in t.final:objects() do
    if sel and obj:type() == "path" then
      if t.final:setAttribute(i, "stroke", "red") then changed = true end
    end
  end
  if changed then
    model:register(t)
  else
    model.ui:explain("No selected path to make red")
  end
end
