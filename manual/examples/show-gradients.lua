-- -*- lua -*-

function printTable(t, indent)
  for k in pairs(t) do
    local v = t[k]
    if type(v) == "table" then 
      print(indent .. k .. ":")
      printTable(v, "  " .. indent)
    else
      print(indent .. k .. ": " .. tostring(v))
    end
  end
end

if #argv ~= 1 then
  io.stderr:write("Usage: ipescript show-gradients <ipe-document>\n")
  return
end

local figname = argv[1]
local doc = assert(ipe.Document(figname))
local s = doc:sheets()

for _,w in ipairs(s:allNames("gradient")) do
  print("Gradient: ", w)
  local g = s:find("gradient", w)
  printTable(g, "  ")
end
