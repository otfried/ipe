----------------------------------------------------------------------
-- WRONG: an ipelet that modifies the document without an undo item
----------------------------------------------------------------------
--
-- This ipelet is an example of what *not* to do.  The page is changed
-- directly, so Ipe does not know about the change: the document is not
-- marked as modified, and undo and redo no longer work correctly.
-- Run it with ipescript run-ipelet to see the warning.

label = "Wrong"

about = "Do not write ipelets like this."

function run(model)
  local page = model:page()
  for i, obj, sel in page:objects() do
    if sel and obj:type() == "path" then
      page:setAttribute(i, "stroke", "red")   -- no undo item!
    end
  end
end
