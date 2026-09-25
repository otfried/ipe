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
  d:add("list", "list", {"red", "green", "blue", "violet", "yellow"}, 5, 1)
  -- d:setEnabled("list", false)
  d:set("list", 4)
  d:add("combo", "combo", {"red", "green", "blue", "violet", "yellow"}, 5, 2)
  d:setStretch("row", 4, 1)
  d:setStretch("row", 5, 1)
  d:setStretch("column", 1, 1)
  d:set("combo", 3)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:addButton("help", "&Help", help )
  local r = d:execute({800, 600})
  print("Dialog returns", r)
  print("check1 is", d:get("check1"))
  print("input is", d:get("input"))
  print("text is", d:get("textedit"))
  print("list is", d:get("list"))
  print("combo is", d:get("combo"))
end

function show_menu(x, y)
  print("Show menu", x, y)
  local m = ipeui.Menu(appui)
  m:add("open", "Open")
  m:add("save", "Save")
  m:add("dialog 1", "Show dialog")
  m:add("messagebox", "MessageBox...")
  m:add("color", "Colors", { "red", "green", "blue" }, nil, 
	function (i, item) 
	  if item == "red" then return 1, 0, 0
	  elseif item == "green" then return 0, 1, 0
	  else return 0, 0, 1
	  end
	end)
  m:add("name", "Submenu", { "alpha", "beta", "gamma" },
	function (i, item) return "select " .. item end,
	"gamma")
  m:add("start timer", "Start timer")
  m:add("collect garbage", "Collect garbage")
  local r,s,t = m:execute(x, y)
  print(r,s,t)
  action(r)
end

local timtab = { elapse = function (t) print("Timer", t) end }

function action(cmd)
  print("Action", cmd)
  if cmd == "dialog 1" then
    dialog1()
  elseif cmd == "collect garbage" then
    collectgarbage("collect")
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
  elseif cmd == "start timer" then
    local t = ipeui.Timer(timtab, "elapse")
    t:setInterval(1000)
    t:start()
  end 
end

print("UI Test")
print("Current date and time are", ipeui.currentDateTime())

print("한글")
