-- -*- lua -*-

if #argv ~= 1 then
  io.stderr:write("Usage: ipescript add-gradient <ipe-document>\n")
  return
end

local figname = argv[1]
local doc = assert(ipe.Document(figname))

local sheet = [[<ipestyle>
<gradient name="ball" type="radial" coords="-4 10 2 0 0 18">
 <stop offset="0" color="1 1 0"/>
 <stop offset="1" color="0 1 0"/>
</gradient>
</ipestyle>]]

local ts = ipe.Sheet(nil, sheet)

local s = doc:sheets()
for i = 1,s:count() do
  print(s:sheet(i):name())
end

-- add the gradient to the sheet on top of the stack
-- in general it is not a good idea to change the "basic" sheet,
-- better verify if a custom sheet for gradients exists already
-- and create it if not.
s:sheet(1):addFrom(ts, "gradient", "ball")
-- assert(doc:runLatex())
doc:save(figname .. ".new.ipe")
