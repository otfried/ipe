
label = "Split views into SVG files"

about = [[
Export every view of the document into SVG format.
]]

function run(model)
  local fname = model.file_name
  if fname == nil then fname = "unknown" end
  local svgname = fname
  local i = fname:find("%.[^.]+$")
  if i then
    svgname = fname:sub(1,i-1)
  end

  -- need latex information to save in SVG
  assert(model:runLatex())

  for pno, page in model.doc:pages() do
    print("Saving page " .. pno .. "\n") 
    for vno = 1, page:countViews() do
      local outname = svgname .. "-" .. pno .. "-" .. vno .. ".svg"
      print("Saving view " .. pno .. "-" .. vno .. " as " .. outname .. "\n") 
      model.ui:renderPage(model.doc, pno, vno, "svg", outname, model.ui:zoom(),
			  true, false) -- transparent, nocrop
    end
  end
end
