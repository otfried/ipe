----------------------------------------------------------------------
-- Draw a circle at the mouse position
----------------------------------------------------------------------
--
-- Creating a single new object is the simplest case: model:creation
-- registers the undo item for you.

label = "Circle"

about = [[
Draw a circle with radius 32 at the current mouse position.
]]

function run(model)
  local center = model.ui:pos()   -- mouse position, after snapping
  local radius = 32
  -- an ellipse is given by the matrix that maps the unit circle to it
  local shape = { { type = "ellipse";
		    ipe.Matrix(radius, 0, 0, radius, center.x, center.y) } }
  local obj = ipe.Path(model.attributes, shape)
  model:creation("create circle", obj)
end
