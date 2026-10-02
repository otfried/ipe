----------------------------------------------------------------------
-- visual style sheet editor
----------------------------------------------------------------------

label = "Visual style sheet editor"

about = [[
Create and edit style sheets visually.

This ipelet is provided by Dmitriy Morozov and now part of Ipe.
]]

local function sheets_unique_name(sheets, base)
  local used = {}
  for i = 1,sheets:count() do
    local name = sheets:sheet(i):name()
    if name then used[name] = true end
  end
  if not used[base] then return base end
  local n = 2
  while used[base .. " " .. n] do n = n + 1 end
  return base .. " " .. n
end

local function sheets_new(model)
  local sheets = model.doc:sheets()
  local name = sheets_unique_name(sheets, "new")
  while true do
    name = model:getString("Name of new stylesheet", "New stylesheet", name)
    if not name then return end
    name = name:match("^%s*(.-)%s*$")
    if name == "" then
      model:warning("Cannot create stylesheet", "The stylesheet name cannot be empty")
    elseif name ~= sheets_unique_name(sheets, name) then
      model:warning("Cannot create stylesheet",
		    "A stylesheet with this name already exists")
    else
      break
    end
  end
  local sheet = ipe.Sheet()
  sheet:setName(name)
  local t = { label="new style sheet",
	      sheet = sheet,
	      style_sheets_changed = true,
	    }
  t.undo = function (t, doc)
    doc:sheets():remove(1)
  end
  t.redo = function (t, doc)
    doc:sheets():insert(1, t.sheet)
  end
  model:register(t)
end

local visual_style_categories = {
  { label="Colors", kind="color", color=true, default="#000000",
    help="Color values are #rrggbb or three numbers between 0 and 1." },
  { label="Pen widths", kind="pen", default="1",
    help="Pen widths are numbers in Ipe points." },
  { label="Dash styles", kind="dashstyle", default="[4] 0",
    help="Dash styles use Ipe syntax, for example [4 2] 0." },
  { label="Text sizes", kind="textsize", default="\\large",
    help="Text sizes are numbers or LaTeX size commands such as \\large." },
  { label="Symbol sizes", kind="symbolsize", default="3",
    help="Symbol sizes are numbers in Ipe points." },
  { label="Arrow sizes", kind="arrowsize", default="7",
    help="Arrow sizes are numbers in Ipe points." },
  { label="Opacity", kind="opacity", default="1",
    help="Opacity values are numbers between 0 and 1." },
  { label="Grid sizes", kind="gridsize", default="8",
    help="Grid sizes are numbers in Ipe points." },
  { label="Angle sizes", kind="anglesize", default="45",
    help="Angle sizes are numbers in degrees." },
}

local function visual_category_labels()
  local r = {}
  for i,c in ipairs(visual_style_categories) do r[i] = c.label end
  return r
end

local function visual_entry_names(entries)
  local r = {}
  for i,e in ipairs(entries) do r[i] = e.name end
  return r
end

local function visual_unique_name(entries, base)
  local used = {}
  for _,e in ipairs(entries) do used[e.name] = true end
  if not used[base] then return base end
  local n = 2
  while used[base .. " " .. n] do n = n + 1 end
  return base .. " " .. n
end

local function visual_rgb_to_hex(value)
  local rgb = {}
  for s in value:gmatch("[^%s]+") do rgb[#rgb + 1] = tonumber(s) end
  if #rgb == 1 then rgb[2], rgb[3] = rgb[1], rgb[1] end
  if #rgb ~= 3 or not rgb[1] or not rgb[2] or not rgb[3] then return value end
  for i = 1,3 do
    rgb[i] = math.max(0, math.min(255, math.floor(255 * rgb[i] + 0.5)))
  end
  return string.format("#%02x%02x%02x", rgb[1], rgb[2], rgb[3])
end

local function visual_hex_to_rgb(value)
  value = value:match("^%s*(.-)%s*$")
  local function hex_digit(c)
    local b = string.byte(c)
    if string.byte("0") <= b and b <= string.byte("9") then
      return b - string.byte("0")
    elseif string.byte("a") <= b and b <= string.byte("f") then
      return b - string.byte("a") + 10
    elseif string.byte("A") <= b and b <= string.byte("F") then
      return b - string.byte("A") + 10
    end
  end
  local function hex_byte(s)
    return 16 * hex_digit(s:sub(1, 1)) + hex_digit(s:sub(2, 2))
  end
  local hex = value:match("^#(%x%x%x%x%x%x)$")
  if hex then
    local r = hex_byte(hex:sub(1, 2))
    local g = hex_byte(hex:sub(3, 4))
    local b = hex_byte(hex:sub(5, 6))
    return string.format("%.6g %.6g %.6g", r / 255, g / 255, b / 255)
  end
  local r1, g1, b1 = value:match("^#(%x)(%x)(%x)$")
  if r1 then
    local r = hex_byte(r1 .. r1)
    local g = hex_byte(g1 .. g1)
    local b = hex_byte(b1 .. b1)
    return string.format("%.6g %.6g %.6g", r / 255, g / 255, b / 255)
  end
  return value
end

local visual_preview_width = 300
local visual_preview_height = 130
local visual_preview_scale = 8
local visual_preview_serial = 0

local function visual_number(value, fallback)
  return tonumber(value) or fallback
end

local function visual_segment_shape(v1, v2)
  return { type="curve", closed=false; { type="segment"; v1, v2 } }
end

local function visual_box_shape(v1, v2)
  return { type="curve", closed=true;
    { type="segment"; v1, ipe.Vector(v1.x, v2.y) },
    { type="segment"; ipe.Vector(v1.x, v2.y), v2 },
    { type="segment"; v2, ipe.Vector(v2.x, v1.y) } }
end

local function visual_arc_shape(center, radius, alpha, beta)
  local arc = ipe.Arc(ipe.Matrix(radius, 0, 0, radius, center.x, center.y),
                      alpha, beta)
  return { type="curve", closed=false;
    { type="arc", arc=arc;
      center + radius * ipe.Direction(alpha),
      center + radius * ipe.Direction(beta) } }
end

local function visual_add_object(page, obj)
  page:insert(nil, obj, 1, "alpha")
end

local function visual_add_preview_objects(page, c, preview_name, value)
  local V = ipe.Vector
  if c.kind == "color" then
    visual_add_object(page, ipe.Path({ stroke="black", fill=preview_name,
                                       pathmode="strokedfilled" },
                                     { visual_box_shape(V(0, 0), V(130, 65)) }))
  elseif c.kind == "pen" then
    visual_add_object(page, ipe.Path({ stroke="black", pen=preview_name },
                                     { visual_segment_shape(V(0, 0), V(180, 0)) }))
  elseif c.kind == "dashstyle" then
    visual_add_object(page, ipe.Path({ stroke="black", pen="fat", dashstyle=preview_name },
                                     { visual_segment_shape(V(0, 0), V(180, 0)) }))
  elseif c.kind == "textsize" then
    visual_add_object(page, ipe.Text({ stroke="black", textsize=preview_name },
                                     "Sample", V(0, 0)))
  elseif c.kind == "symbolsize" then
    visual_add_object(page, ipe.Reference({ stroke="black", fill="red",
                                            symbolsize=preview_name },
                                           "mark/disk(sx)", V(0, 0)))
  elseif c.kind == "arrowsize" then
    visual_add_object(page, ipe.Path({ stroke="black", pen="fat", farrow=true,
                                       farrowsize=preview_name,
                                       farrowshape="arrow/normal(spx)" },
                                     { visual_segment_shape(V(0, 0), V(170, 0)) }, true))
  elseif c.kind == "opacity" then
    visual_add_object(page, ipe.Path({ stroke="black", fill="blue",
                                       pathmode="strokedfilled" },
                                     { visual_box_shape(V(0, 0), V(80, 60)) }))
    visual_add_object(page, ipe.Path({ stroke="black", fill="red", opacity=preview_name,
                                       pathmode="strokedfilled" },
                                     { visual_box_shape(V(45, 10), V(125, 70)) }))
  elseif c.kind == "gridsize" then
    local step = math.max(1, visual_number(value, 8))
    for x = 0,160,step do
      visual_add_object(page, ipe.Path({ stroke="0.7", pen="normal" },
                                       { visual_segment_shape(V(x, 0), V(x, 80)) }))
    end
    for y = 0,80,step do
      visual_add_object(page, ipe.Path({ stroke="0.7", pen="normal" },
                                       { visual_segment_shape(V(0, y), V(160, y)) }))
    end
    visual_add_object(page, ipe.Path({ stroke="black", pen="fat" },
                                     { visual_segment_shape(V(0, 0), V(160, 80)) }))
  elseif c.kind == "anglesize" then
    local alpha = math.rad(visual_number(value, 45))
    local origin = V(0, 0)
    local radius = 42
    visual_add_object(page, ipe.Path({ stroke="black", pen="normal" },
                                     { visual_segment_shape(origin, V(150, 0)) }))
    visual_add_object(page, ipe.Path({ stroke="red", pen="fat" },
                                     { visual_segment_shape(origin,
                                         radius * 3 * ipe.Direction(alpha)) }))
    visual_add_object(page, ipe.Path({ stroke="black", pen="normal" },
                                     { visual_arc_shape(origin, radius, 0, alpha) }))
  end
end

local function visual_preview_spec(dd, c, value)
  local preview_name = "__preview_textsize"
  if c.kind ~= "textsize" then preview_name = "__preview_" .. c.kind end
  local doc = ipe.Document()
  local sheets = dd.sheets:clone()
  local sheet = ipe.Sheet()
  sheet:setName("__preview")
  local ok = _G.pcall(function () sheet:setAttribute(c.kind, preview_name, value) end)
  if not ok then return nil end
  sheets:insert(1, sheet)
  doc:replaceSheets(sheets)
  local p = doc[1]
  visual_add_preview_objects(p, c, preview_name, value)
  if c.kind == "textsize" then
    ok = doc:runLatex(dd.model.file_name)
    if not ok then return nil end
  end
  visual_preview_serial = visual_preview_serial + 1
  local png = ipe.folder("latex", string.format("style-preview-%d.png",
                                                visual_preview_serial))
  dd.model.ui:renderPage(doc, 1, 1, "png", png,
                         dd.model.ui:zoom() * visual_preview_scale, true, false)
  return string.format("imagefile|%s|%g", png, visual_preview_scale)
end

local function visual_set_preview(d, dd, c, value)
  local spec = visual_preview_spec(dd, c, value)
  d:set("preview", spec or "unavailable|Preview unavailable")
end

local function visual_load_sheet(sheet)
  local data = {}
  for ci,c in ipairs(visual_style_categories) do
    data[ci] = {}
    for _,name in ipairs(sheet:allNames(c.kind)) do
      data[ci][#data[ci] + 1] = { name=name, value=sheet:find(c.kind, name) }
    end
  end
  return data
end

local function visual_set_fields(d, dd, st)
  local c = visual_style_categories[st.cat]
  local entries = st.data[st.cat]
  local names = visual_entry_names(entries)
  st.updating = true
  d:set("items", names)
  if #entries == 0 then
    st.current = nil
    d:set("name", "")
    d:set("value", c.color and (c.default or "#000000") or "")
    visual_set_preview(d, dd, c, c.color and visual_hex_to_rgb(c.default or "#000000") or c.default)
  else
    st.current = math.max(1, math.min(st.current or 1, #entries))
    d:set("items", st.current)
    d:set("name", entries[st.current].name)
    if c.color then
      d:set("value", visual_rgb_to_hex(entries[st.current].value))
    else
      d:set("value", entries[st.current].value)
    end
    visual_set_preview(d, dd, c, entries[st.current].value)
  end
  d:set("value_label", c.color and "Color" or "Value")
  d:set("help", c.help)
  d:setEnabled("value", true)
  st.updating = false
end

local function visual_apply_current(d, dd, st)
  local c = visual_style_categories[st.cat]
  local entries = st.data[st.cat]
  local name = d:get("name")
  if name == "" and not st.current and #entries == 0 then return true end
  if name == "" then
    dd.model:warning("Cannot update stylesheet", "The symbolic name cannot be empty")
    return false
  end
  local value = c.color and visual_hex_to_rgb(d:get("value")) or d:get("value")
  if value == "" then
    dd.model:warning("Cannot update stylesheet", "The value cannot be empty")
    return false
  end
  local current = st.current or (#entries + 1)
  for i = #entries,1,-1 do
    if entries[i].name == name and i ~= current then
      table.remove(entries, i)
      if i < current then current = current - 1 end
    end
  end
  entries[current] = { name=name, value=value }
  st.current = current
  st.updating = true
  d:set("items", visual_entry_names(entries))
  d:set("items", st.current)
  if c.color then d:set("value", visual_rgb_to_hex(value)) end
  visual_set_preview(d, dd, c, value)
  st.updating = false
  return true
end

local function visual_apply_to_sheet(d, dd, st)
  if not visual_apply_current(d, dd, st) then return nil end
  local nsheet = dd.sheet:clone()
  for ci,c in ipairs(visual_style_categories) do
    for _,name in ipairs(nsheet:allNames(c.kind)) do
      nsheet:remove(c.kind, name)
    end
    for _,entry in ipairs(st.data[ci]) do
      local ok, msg = _G.pcall(function ()
        nsheet:setAttribute(c.kind, entry.name, entry.value)
      end)
      if not ok then
        dd.model:warning("Cannot update stylesheet",
                         string.format("%s '%s': %s", c.label, entry.name, msg))
        return nil
      end
    end
  end
  local parsed, msg = ipe.Sheet(nil, nsheet:xml(true))
  if not parsed then
    dd.model:warning("Cannot update stylesheet", msg)
    return nil
  end
  return nsheet
end

local function sheets_visual_edit(model)
  local sheets = model.doc:sheets()
  local index = 1 -- always edit top stylesheet
  local sheet = sheets:sheet(index)
  if sheet:isStandard() then return end
  local dd = {
    sheets = sheets,
    sheet = sheet,
    model = model
  }
  
  local cats = visual_category_labels()
  local st = { cat=1, current=1, data=visual_load_sheet(dd.sheet) }
  cats.action = function (d)
    if st.updating then return end
    if not visual_apply_current(d, dd, st) then return end
    st.cat = d:get("category")
    st.current = 1
    visual_set_fields(d, dd, st)
  end
  local first_names = visual_entry_names(st.data[1])
  first_names.action = function (d)
    if st.updating then return end
    st.current = d:get("items")
    visual_set_fields(d, dd, st)
  end

  local d = ipeui.Dialog(model.ui:win(), "Visual stylesheet editor")
  d:add("category_label", "label", { label="Category" }, 1, 1)
  d:add("category", "combo", cats, 1, 2, 1, 5)
  d:add("items", "list", first_names, 2, 1, 5, 3)
  d:add("name_label", "label", { label="Name" }, 2, 4)
  d:add("name", "input", { select_all=true }, 2, 5, 1, 2)
  d:add("value_label", "label", { label="Value" }, 3, 4)
  d:add("value", "input", {}, 3, 5, 1, 2)
  d:add("help", "label", { label="" }, 4, 4, 1, 3)
  d:add("preview_label", "label", { label="Preview" }, 6, 4)
  d:add("preview", "image", { width=visual_preview_width,
                               height=visual_preview_height }, 6, 5, 1, 2)
  d:add("add", "button", { label="Add",
    action=function (d)
      local c = visual_style_categories[st.cat]
      local entries = st.data[st.cat]
      local name = d:get("name")
      if st.current and entries[st.current] and name == entries[st.current].name then
        name = "new"
      end
      if name == "" then name = "new" end
      local value = c.color and visual_hex_to_rgb(d:get("value")) or d:get("value")
      if value == "" then
        value = c.color and visual_hex_to_rgb(c.default) or c.default
      end
      entries[#entries + 1] = {
        name=visual_unique_name(entries, name),
        value=value,
      }
      st.current = #entries
      visual_set_fields(d, dd, st)
    end }, 7, 3)
  d:add("delete", "button", { label="Delete",
    action=function (d)
      local entries = st.data[st.cat]
      if st.current and entries[st.current] then
        table.remove(entries, st.current)
        st.current = math.min(st.current, #entries)
        visual_set_fields(d, dd, st)
      end
    end }, 7, 4)
  d:add("apply", "button", { label="Apply / Preview",
    action=function (d) visual_apply_current(d, dd, st) end }, 7, 5)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 5, 1)
  d:setStretch("column", 2, 4)
  d:setStretch("column", 5, 1)
  d:setStretch("column", 6, 4)
  visual_set_fields(d, dd, st)

  if not d:execute({ 680, 520 }) then return end
  local nsheet = visual_apply_to_sheet(d, dd, st)
  if not nsheet then return end

  local t = { label="visual edit of style sheet '" .. sheet:name() .. "'",
	      original = sheet:clone(),
	      sheet = nsheet,
	      index = index,
	      style_sheets_changed = true,
	    }
  t.undo = function (t, doc)
    doc:sheets():remove(t.index)
    doc:sheets():insert(t.index, t.original)
  end
  t.redo = function (t, doc)
    doc:sheets():remove(t.index)
    doc:sheets():insert(t.index, t.sheet)
  end
  model:register(t)
end

methods = {
  { label = "New style sheet", run=sheets_new },
  { label = "Visually edit style sheet", run=sheets_visual_edit },
}

----------------------------------------------------------------------
