-- Make all paths in an Ipe document red.
--
-- Usage: ipescript red-paths <input> <output>
--
-- A script owns the document, so it can modify it directly.
-- Compare with ipelet-red-paths.lua, which does the same inside Ipe.

if #argv ~= 2 then
  io.stderr:write("Usage: ipescript red-paths <input> <output>\n")
  return
end

local doc = assert(ipe.Document(argv[1]))

for _, page in doc:pages() do
  for i, obj in page:objects() do
    if obj:type() == "path" then
      -- change the page (and not only the object), so that the page
      -- knows that the object's bounding box may have changed
      page:setAttribute(i, "stroke", "red")
    end
  end
end

assert(doc:save(argv[2]))
