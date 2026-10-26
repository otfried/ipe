-- 
-- uitest -- Test program for ipeui library
-- 

function checker(d)
  print("Check", d, d:get("input"), d:get("check1"))
  local s = d:get("check1")
  if not s and d:get("list") == 1 then
    if d:get("combo") == 1 then d:accept(true) end
    if d:get("combo") == 2 then d:accept(false) end
  end

  d:setEnabled("list", s)
  if s then
    d:set("list", { "regen", "schnee", "hagel", "graupel", "eis", "taifun" })
  else
    d:set("combo", {"alpha", "beta", "gamma", "epsilon", "zeta", "theta",
		    "iota", "kappa", "lambda", "tau" })
  end
  d:set("list", 2)
  d:set("combo", 3)
end

function help(d)
  print("Help", d, d:get("combo"), d:get("list"))
  d:setEnabled("check1", true)
  local t = d:get("textedit")
  d:set("textedit", t .. " Surprise!")
  d:set("combo", 5)
end

local imageState = 0

-- cycles through a PNG file, a bitmap from memory, and a text
function toggleImage(d)
  imageState = (imageState + 1) % 3
  if imageState == 0 then
    d:set("image", { path="../../artwork/ipe.iconset/icon_64x64.png", zoom=1.3 })
  elseif imageState == 1 then
    -- created using pngtopam -alphapam ../../artwork/ipe.iconset/icon_64x64.png > image.pam
    local f = assert(io.open("image.pam", "rb"))
    local data = f:read("*all")
    f:close()
    -- skip the header
    local pixels = data:sub(#data - 64 * 64 * 4 + 1)
    d:set("image", { bitmap={ width=64, height=64, pixels=pixels }, zoom=0.5 })
  else
    d:set("image", { text="No image loaded" })
  end
end

function listAction(d)
  listItem = d:get("list")
  comboItem = d:get("combo")
  d:set("label1", "Text " .. listItem .. " - " .. comboItem)
  print("listAction: ", listItem, comboItem)
end

function dialog1()
  local d = ipeui.Dialog(appui, "Test 1")
  local text = "Here is some initial text,\nspread lovingly over many lines,\nwhich you can read at your leisure.\n한글"
  d:add("label1", "label", {label="Text"}, 1, 1)
  d:add("label2", "label", {label="Second line\nMore lines..."}, 2, 1)
  d:add("check1", "checkbox", {label="Check me", action=checker}, 2, 2)
  d:set("check1", true)
  d:setEnabled("check1", false)
  d:add("input", "input", {}, 3, 1, 1, 2)
  d:add("textedit", "text", {read_only=false}, 4, 1, 1, 2)
  d:set("textedit", text)
  d:set("ignore-escape", "textedit", text)
  d:add("list", "list", {"red", "green", "blue", "violet", "yellow", action=listAction}, 5, 1)
  -- d:setEnabled("list", false)
  d:set("list", 4)
  d:add("combo", "combo", {"red", "green", "blue", "violet", "yellow", action=listAction}, 5, 2)
  d:add("image", "image", {value={ path="../../artwork/ipe.iconset/icon_64x64.png" },
			   width=200, height=150}, 6, 1, 1, 2)
  d:setStretch("row", 4, 1)
  d:setStretch("row", 5, 1)
  -- d:setStretch("row", 6, 1)
  d:setStretch("column", 1, 1)
  d:set("combo", 3)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:addButton("help", "&Help", help)
  d:addButton("toggle", "&Toggle image", toggleImage)
  local r = d:execute({800, 700})
  print("Dialog returns", r)
  print("check1 is", d:get("check1"))
  print("input is", d:get("input"))
  print("text is", d:get("textedit"))
  print("list is", d:get("list"))
  print("combo is", d:get("combo"))
end

function dialog2() 
  local d = ipeui.Dialog(win, "Ipe Preferences")
  local s = "Ipe preferences are changed by creating a Lua source file."
  d:add("text", "text", { read_only=true }, 1, 1)
  d:set("text", s)
  d:setStretch("row", 1, 1)
  d:setStretch("column", 1, 1)
  d:addButton("ok", "Ok", "accept")
  d:execute({600, 300})
end

-- a bitmap filled with a color, with a darker border
local function colorBitmap(w, h, r, g, b)
  local rows = {}
  for y = 0, h - 1 do
    local row = {}
    for x = 0, w - 1 do
      local border = (x < 2 or y < 2 or x >= w - 2 or y >= h - 2)
      local f = border and 0.5 or 1.0
      row[#row + 1] = string.char(math.floor(r * f), math.floor(g * f),
				  math.floor(b * f), 255)
    end
    rows[#rows + 1] = table.concat(row)
  end
  return { width=w, height=h, pixels=table.concat(rows) }
end

function dialog3()
  local d = ipeui.Dialog(appui, "Sorter")
  local colors = { { 255, 0, 0 }, { 0, 200, 0 }, { 0, 0, 255 }, { 255, 200, 0 },
		   { 200, 0, 200 }, { 0, 200, 200 } }
  local items = { width=500, height=300 }
  for i, c in ipairs(colors) do
    items[i] = { label="Item " .. i, bitmap=colorBitmap(64, 48, c[1], c[2], c[3]) }
    -- items 2, 4, and 6 have a checkbox, item 4 is marked
    if i % 2 == 0 then items[i].mark = (i == 4) end
  end
  d:add("label", "label", {label="Right click to delete, cut, insert, mark"}, 1, 1)
  d:add("sorter", "sorter", items, 2, 1)
  d:setStretch("row", 2, 1)
  d:setStretch("column", 1, 1)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  local r = d:execute({600, 400})
  local order = d:get("sorter")
  print("Dialog returns", r, "sorter is { " .. table.concat(order, ", ") .. " }")
end

function dialog4()
  local d = ipeui.Dialog(appui, "Readonly sorter")
  local colors = { { 255, 0, 0 }, { 0, 200, 0 }, { 0, 0, 255 }, { 255, 200, 0 },
		   { 200, 0, 200 }, { 0, 200, 200 }, { 128, 128, 128 }, { 255, 128, 0 } }
  local items = { width=500, height=300, readonly=true }
  for i, c in ipairs(colors) do
    items[i] = { label="Symbol " .. i, bitmap=colorBitmap(64, 48, c[1], c[2], c[3]) }
  end
  d:add("label", "label", {label="Select one or more symbols"}, 1, 1)
  d:add("sorter", "sorter", items, 2, 1)
  d:setStretch("row", 2, 1)
  d:setStretch("column", 1, 1)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  local r = d:execute({600, 400})
  local selected = d:get("sorter")
  print("Dialog returns", r, "selected { " .. table.concat(selected, ", ") .. " }")
end

function showMenuInner(x, y)
  print("Show menu", x, y)
  local m = ipeui.Menu(appui)
  m:add("open", "Open")
  m:add("save", "Save")
  m:add("dialog 1", "Show dialog 1")
  m:add("dialog 2", "Show dialog 2")
  m:add("dialog 3", "Show sorter dialog")
  m:add("dialog 4", "Show readonly sorter dialog")
  m:add("messagebox", "MessageBox...")
  m:add("submenu1", "Submenu 1", { "alpha", "beta", "gamma" })
  m:add("submenu2", "Submenu 2", { "alpha", "beta", "gamma" },
	{"January", "February", "March"})
  m:add("submenu3", "Submenu 3", { "alpha", "beta", "gamma" },
	function (i, item) return "select " .. item end)
  m:add("submenu4", "Submenu 4", { "alpha", "beta", "gamma" }, nil, "beta")
  m:add("submenu5", "Submenu 5", { "alpha", "beta", "gamma" },
	function (i, item) return "Variant " .. item end,
	"gamma")
  m:add("color", "Colors", { "red", "green", "blue" }, nil, 
	function (i, item) 
	  if item == "red" then return 1, 0, 0
	  elseif item == "green" then return 0, 1, 0
	  else return 0, 0, 1
	  end
	end)
  m:add("start timer", "Start timer")
  m:add("collect garbage", "Collect garbage")
  local r,s,t = m:execute(x, y)
  print(r,s,t)
  action(r)
end

-- stores coroutine
local currentAction = nil

function show_menu(x, y)
  currentAction = coroutine.create(showMenuInner)
  coroutine.resume(currentAction, x, y)
end

local timtab = { elapse = function (t) print("Timer", t) end }

function actionInner(cmd)
  if cmd == "dialog 1" then
    dialog1()
  elseif cmd == "dialog 2" then
    dialog2()
  elseif cmd == "dialog 3" then
    dialog3()
  elseif cmd == "dialog 4" then
    dialog4()
  elseif cmd == "messagebox" then
    local r = ipeui.messageBox(appui, "warning", "Testing 123", "TEST 1 2 3",
			       "savediscardcancel")
    print("MessageBox returns", r)
  elseif cmd == "open" then
    print(ipeui.fileDialog(appui, "open", "Open file", 
			   { "All files (*.*)", "*.*" } ))
  elseif cmd == "save" then
    print(ipeui.fileDialog(appui, "save", "Save file", 
			   { "All files (*.*)", "*.*" } ))
  elseif cmd =="color" then
    print(ipeui.getColor(appui, "Choose a color", 0.5, 0.5, 0.0))
  end 
end

function action(cmd)
  print("Action", cmd)
  if cmd == "collect garbage" then
    collectgarbage("collect")
  elseif cmd == "start timer" then
    local t = ipeui.Timer(timtab, "elapse")
    t:setInterval(1000)
    t:start()
  else
    currentAction = coroutine.create(actionInner)
    coroutine.resume(currentAction, cmd)
  end
end

print("UI Test")
print("Current date and time are", ipeui.currentDateTime())
print("한글")
