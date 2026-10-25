----------------------------------------------------------------------
-- Scale the selection by a factor entered by the user
----------------------------------------------------------------------
--
-- This ipelet shows a dialog to ask for the scale factor, and then uses
-- model:transformation, which takes care of undo and redo when all you
-- want to do is to transform the selected objects by a matrix.

label = "Scale selection"

about = [[
Scale the selected objects around the center of their bounding box.
]]

function run(model)
  local selection = model:selection()  -- table with object numbers
  if #selection == 0 then
    model:warning("Nothing is selected")
    return
  end

  local d = ipeui.Dialog(model.ui:win(), "Scale selection")
  d:add("label", "label", { label = "Scale factor" }, 1, 1)
  d:add("factor", "input", { select_all = true }, 1, 2)
  d:set("factor", "2")
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  if not d:execute() then return end

  local factor = tonumber(d:get("factor"))
  if not factor or factor <= 0 then
    model:warning("The scale factor must be a positive number")
    return
  end

  local page = model:page()
  local box = ipe.Rect()
  for _, i in ipairs(selection) do box:add(page:bbox(i)) end
  local center = (box:bottomLeft() + box:topRight()) * 0.5

  local m = ipe.Translation(center) * ipe.Matrix(factor, 0, 0, factor)
            * ipe.Translation(-center)
  model:transformation("scale selection", m)
end
