-- recenter.lua
-- center objects on each page of the document

if #argv ~= 2 then
  io.stderr:write("Usage: ipescript recenter <inputfile> <outputfile>\n")
  return
end

inname = argv[1]
outname = argv[2]

doc = assert(ipe.Document(inname))

-- make sure we have size information for text
assert(doc:runLatex())

layout = doc:sheets():find("layout")
fs = layout.framesize

for i,p in doc:pages() do
  box = ipe.Rect()
  for j = 1,#p do
    box:add(p:bbox(j))
  end
  nx = (fs.x - box:width()) / 2
  ny = (fs.y - box:height()) / 2
  trans = ipe.Vector(nx, ny) - box:bottomLeft()
  m = ipe.Translation(trans)
  for j = 1,#p do
    p:transform(j, m)
  end
end
doc:save(outname)

