-- Count the objects in an Ipe document, page by page.
--
-- Usage: ipescript count-objects <document>

if #argv ~= 1 then
  io.stderr:write("Usage: ipescript count-objects <document>\n")
  return
end

local doc = assert(ipe.Document(argv[1]))

for pno, page in doc:pages() do
  -- count the objects of every type (group, text, path, image, reference)
  local counts = {}
  for _, obj, _, layer in page:objects() do
    local t = obj:type()
    counts[t] = (counts[t] or 0) + 1
  end
  local parts = {}
  for t, n in pairs(counts) do
    parts[#parts + 1] = n .. " " .. t
  end
  table.sort(parts)
  print(string.format("Page %d: %d objects (%s)", pno, #page, table.concat(parts, ", ")))
end
