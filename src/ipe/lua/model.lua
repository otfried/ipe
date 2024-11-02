----------------------------------------------------------------------
-- model.lua
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

--]]

MODEL = {}
MODEL.__index = MODEL

MODEL.snapmodes = { "snapvtx", "snapctl", "snapbd", "snapint", "snapgrid",
		    "snapangle", "snapcustom", "snapauto"}

function MODEL:new(fname)
  local model = {}
  setmetatable(model, MODEL)
  model:init(fname)
  return model
end

function MODEL:init(fname)
  self.attributes = prefs.initial_attributes

  self.snap = {
    snapvtx = prefs.snap.vertex,
    snapctl = prefs.snap.ctrlpoints,
    snapbd = prefs.snap.boundary,
    snapint = prefs.snap.intersection,
    snapgrid = prefs.snap.grid,
    snapangle = prefs.snap.angle,
    snapcustom = prefs.snap.custom,
    snapauto = prefs.snap.autoangle,
    grid_visible = prefs.initial.grid_visible,
    pretty_display = prefs.initial.pretty_display,
    gridsize = prefs.initial.grid_size,
    anglesize = prefs.initial.angle_size,
    snap_distance = prefs.snap_distance,
    with_axes = false,
    origin = ipe.Vector(0,0),
    orientation = 0
  }

  self.save_timestamp = 0
  self.ui = AppUi(self)
  self.pristine = false
  self.first_show = true
  self.current_action = nil
  self.okay_close = false

  self.type3_font = false

  self.recent_files = recent_files

  self.auto_latex = prefs.auto_run_latex
  self.ui:setActionState("auto_latex", self.auto_latex)

  self.mode = "select"
  self.ui:setActionState("mode_select", true)
  self.ui:setActionState("grid_visible", self.snap.grid_visible)
  self.ui:setActionState("pretty_display", self.snap.pretty_display)
  self.ui:setActionState("show_axes", self.snap.with_axes)

  for i,e in ipairs(MODEL.snapmodes) do
    self.ui:setActionState(e, self.snap[e])
  end
  self:setSnap()
  self.ui:setFifiVisible(true)

  for dock, vis in pairs(prefs.tools_visible) do
    self.ui:showTool(dock, vis)
  end

  if prefs.autosave_interval then
    self.timer = ipeui.Timer(self, "autosave")
    self.timer:setInterval(1000 * prefs.autosave_interval) -- millisecs
    self.timer:start()
  end

  local err = nil
  if fname then
    if ipe.fileExists(fname) then
      err = self:tryLoadDocument(fname)
    else
      self:newDocument()
      self.file_name = fname
      self:setCaption()
    end
  end
  if not self.doc then
    self:newDocument()
    self.pristine = true
  end

  self:updateRecentFiles(fname)

  if err then
    self:warning("Document '" .. fname .. "' could not be opened", err)
  end

  if self.auto_latex then
    self.first_latex_timer = ipeui.Timer(self, "runFirstTimeLatex")
    self.first_latex_timer:setSingleShot(true)
    self.first_latex_timer:setInterval(0) -- next iteration of event loop
    self.first_latex_timer:start()
  end
end

function MODEL:runFirstTimeLatex()
  if self.first_latex_timer then
    self.first_latex_timer = nil -- free timer
    self:wrapCall(self.runLatex, self)
  end
end

function MODEL:sizeChanged()
  if self.first_show then
    self:action_fit_top()
  end
  self.first_show = false
end

function MODEL:dpiChange(oldDpi, newDpi)
  -- adjust zoom by ratio of the two dpi values
  local nzoom = self.ui:zoom() * newDpi / oldDpi
  if nzoom > prefs.max_zoom then nzoom = prefs.max_zoom end
  if nzoom < prefs.min_zoom then nzoom = prefs.min_zoom end
  self.ui:setZoom(nzoom)
  self.ui:update()
end

----------------------------------------------------------------------

-- every call from the UI into Lua code must be wrapped:
-- 1. inside a thread, so actions can yield and wait for UI or
--    background activity
-- 2. if an error happens in Lua code, there will be a message and
--    Ipe continues to run, instead of crashing.

function MODEL:wrapCall(f, ...)
  local wrapper = function(f, ...)
    local result, err = xpcall(f, debug.traceback, ...)
    if not result then
      messageBox(nil, "critical",
		 "Lua error\n\n"..
		   "Data may have been corrupted. \n" ..
		   "Save your file!",
		 err)
    end
  end
  if self.current_action then
    local status = coroutine.status(self.current_action)
    if status == "suspended" then
      messageBox(nil, "warning",
		 "An operation is still waiting for a dialog, Latex, "
		   .. "or an external editor, yet a new action happens.")
      coroutine.close(self.current_action)
    elseif status == "normal" then
      print("DANGER! THIS SHOULD NOT HAPPEN!")
      print("Calling into Lua while an operation is ongoing.  What's going on?")
    end
  end
  self.current_action = coroutine.create(wrapper)
  coroutine.resume(self.current_action, f, ...)
end

-- called by the UI to resume when Lua has yielded in an async operation
function MODEL:resumeLua(...)
  if self.current_action then coroutine.resume(self.current_action, ...) end
end

function MODEL:preloadFile(fname)
  if config.platform == "electron" then
    self.ui.js("preloadFile", fname, os.tmpname())
    coroutine.yield()
  end
end

function MODEL:persistFile(fname)
  if config.platform == "electron" then
    self.ui.js("persistFile", fname)
    return coroutine.yield()
  elseif config.platform == "web" then
    self.ui.js("persistFile", fname)
    return true
  else
    return true
  end
end

function MODEL:preloadFileExists()
  if config.platform == "electron" then
    self.ui.js("preloadFileExists")
    coroutine.yield()
  end
end

function MODEL:clipboard(allowBitmap)
  if config.toolkit == "htmljs" then
    self.ui:getClipboardAsync(allowBitmap)
    return coroutine.yield()
  else
    return self.ui:getClipboard(allowBitmap)
  end
end

----------------------------------------------------------------------

function MODEL:resetGridSize()
  self.snap.gridsize = prefs.initial.grid_size
  self.snap.anglesize = prefs.initial.angle_size
  local gridsizes = allValues(self.doc:sheets(), "gridsize")
  if #gridsizes == 0 then gridsizes = { 16 } end
  if not indexOf(self.snap.gridsize, gridsizes) then
    self.snap.gridsize = gridsizes[1]
  end
  local anglesizes = allValues(self.doc:sheets(), "anglesize")
  if #anglesizes == 0 then anglesizes = { 45 } end
  if not indexOf(self.snap.anglesize, anglesizes) then
    self.snap.anglesize = anglesizes[1]
  end
  self.ui:setGridAngleSize(self.snap.gridsize, self.snap.anglesize)
end

function MODEL:print_attributes()
  print("----------------------------------------")
  for k in pairs(self.attributes) do
    print(k, self.attributes[k])
  end
  print("----------------------------------------")
end

function MODEL:getString(msg, caption, start)
  if caption == nil then caption = "Ipe" end
  local d = ipeui.Dialog(self.ui:win(), caption)
  d:add("label", "label", {label=msg}, 1, 1)
  d:add("text", "input", {select_all=true}, 2, 1)
  d:addButton("ok", "Ok", "accept")
  d:addButton("cancel", "Cancel", "reject")
  if start then d:set("text", start) end
  if d:execute() then
    return d:get("text")
  end
end

function MODEL:getColorJS(msg, r, g, b, caption)
  if caption == nil then caption = "Ipe" end
  local d = ipeui.Dialog(self.ui:win(), caption)
  d:add("label", "label", {label=msg}, 1, 1)
  d:add("color", "input", {color_picker=true}, 2, 1)
  d:addButton("ok", "Ok", "accept")
  d:addButton("cancel", "Cancel", "reject")
  d:set("color", string.format("#%02x%02x%02x",
			       math.floor(r * 255),
			       math.floor(g * 255),
			       math.floor(b * 255)))
  if d:execute() then
    local s = d:get("color")
    if #s == 7 and s:sub(1,1) == "#" then
      r = tonumber2(s:sub(2,3), 16) / 255.0
      g = tonumber2(s:sub(4,5), 16) / 255.0
      b = tonumber2(s:sub(6,7), 16) / 255.0
      return r, g, b
    end
  end
end

function MODEL:getDouble(caption, label, value, minv, maxv)
  local s = self:getString(label, caption, tostring(value))
  if s then
    local n = tonumber(s)
    if n and minv <= n and n <= maxv then
      return n
    end
  end
end

function MODEL:waitDialog(cmd, text)
  local done = self.ui:waitDialog(cmd, text)
  if not done then coroutine.yield() end
end

----------------------------------------------------------------------

-- Return current page
function MODEL:page()
  return self.doc[self.pno]
end

-- return table with selected objects on current page
function MODEL:selection()
  local t = {}
  for i,obj,sel,layer in self:page():objects() do
    if sel then t[#t+1] = i end
  end
  return t
end

function MODEL:markAsUnmodified()
  self.save_timestamp = self.save_timestamp + 1
  self.undo[#self.undo].save_timestamp = self.save_timestamp
end

-- is document modified?
function MODEL:isModified()
  return (self.undo[#self.undo].save_timestamp ~= self.save_timestamp)
end

----------------------------------------------------------------------

-- set window caption
function MODEL:setCaption()
  local s = "Ipe "
  if config.toolkit == "qt" then
    s = s .. "[*]"
  elseif self:isModified() then
    s = s .. '*'
  end
  if self.file_name then
    s = s .. '"' .. self.file_name .. '" '
  else
    s = s .. "[unsaved] "
  end
--[[
  if #self.doc > 1 then
    s = s .. string.format("Page %d/%d ", self.pno, #self.doc)
  end
  if self:page():countViews() > 1 then
    s = s .. string.format("(View %d/%d) ", self.vno, self:page():countViews())
  end
--]]
  self.ui:setWindowTitle(self:isModified(), s, self.file_name)
end

function MODEL:setSnap()
  local function ind(is_on, s, letter)
    if is_on then return s .. letter
    else return s .. "-" end
  end

  self.ui:setSnap(self.snap)
  self.ui:setFifiVisible(true)
  local s = ind(self.snap.snapvtx, "", "v")
  s = ind(self.snap.snapctl, s, "c")
  s = ind(self.snap.snapbd, s, "b")
  s = ind(self.snap.snapint, s, "x")
  s = ind(self.snap.snapgrid, s, "+")
  s = ind(self.snap.snapangle, s, "r")
  s = ind(self.snap.snapcustom, s, "#")
  s = ind(self.snap.snapauto, s, "a")
  s = s .. " " .. self.snap.gridsize
  s = s .. " " .. self.snap.anglesize
  self.ui:setSnapIndicator(s)
end

-- show a warning messageBox
function MODEL:warning(text, details)
  messageBox(self.ui:win(), "warning", text, details)
end

function numberFormatForMax(m)
  if m < 10 then
    return "%d"
  elseif m < 100 then
    return "%02d"
  else
    return "%03d"
  end
end

-- Set canvas to current page
function MODEL:setPage()
  local p = self:page()
  self.ui:setPage(p, self.pno, self.vno, self.doc:sheets())
  self.ui:setLayers(p, self.vno)
  self.ui:setNumbering(self.doc:properties().numberpages)
  self.ui:update()
  self:setCaption()
  self:setBookmarks()
  self.ui:setNotes(p:notes())
  -- to avoid changing the width of the properties gadget, base the
  -- format on the max # of views and on the # of pages
  if #self.doc > 1 or p:countViews() > 1 then
    local vno
    local pno
    local pnofmt = numberFormatForMax(#self.doc)
    local maxNoViews = 1
    for _,q in self.doc:pages() do
      if q:countViews() > maxNoViews then maxNoViews = q:countViews() end
    end
    local vnofmt = numberFormatForMax(maxNoViews)
    pno = string.format(prefs.page_button_prefix .. pnofmt .. "/" .. pnofmt, self.pno, #self.doc)
    vno = string.format(prefs.view_button_prefix .. vnofmt .. "/" .. vnofmt, self.vno, p:countViews())
    self.ui:setNumbers(vno, p:markedView(self.vno), pno, p:marked())
  else
    self.ui:setNumbers(nil, p:markedView(self.vno), nil, p:marked())
  end
end

function MODEL:getBookmarks()
  local b = {}
  for _,p in self.doc:pages() do
    local t = p:titles()
    if t.section then
      if t.section ~= "" then b[#b+1] = t.section end
    elseif t.title ~= "" then b[#b+1] = t.title end
    if t.subsection then
      if t.subsection ~= "" then b[#b+1] = "  " .. t.subsection end
    elseif t.title ~= "" then b[#b+1] = "   " .. t.title end
  end
  return b
end

function MODEL:setBookmarks()
  self.ui:setBookmarks(self:getBookmarks())
end

function MODEL:findPageForBookmark(index)
  local count = 0
  for i,p in self.doc:pages() do
    local t = p:titles()
    if t.section then
      if t.section ~= "" then count = count + 1 end
    elseif t.title ~= "" then count = count + 1 end
    if t.subsection then
      if t.subsection ~= "" then count = count + 1 end
    elseif t.title ~= "" then count = count + 1 end
    if count >= index then return i end
  end
end

function MODEL:checkType3Font()
  if not self.type3_font and self.ui:type3Font() then
    self.type3_font = true
    self:warning("Type3 font detected",
		 "It appears your document uses a Type3 font.\n\n" ..
		   "These are bitmapped fonts, typically created by Latex from a Metafont source.\n\n" ..
		   "Ipe cannot display these fonts (you'll see a box instead), and since they are " ..
		   "bitmaps, they will not look good when you scale your figure later.\n\n" ..
		   "A modern Latex installation should not normally use Type3 fonts. You could " ..
		   "try to install the 'cm-super' package to avoid using Type3 fonts.")
  end
end

----------------------------------------------------------------------

-- Deselect all objects not in current view, or in a locked layer.
function MODEL:deselectNotInView()
  local p = self:page()
  for i,obj,sel,layer in p:objects() do
    if not p:visible(self.vno, i) or p:isLocked(layer) then
      p:setSelect(i, nil)
    end
  end
  p:ensurePrimarySelection()
end

-- Deselect all but primary selection
function MODEL:deselectSecondary()
  local p = self:page()
  for i,obj,sel,layer in p:objects() do
    if sel ~= 1 then p:setSelect(i, nil) end
  end
end

-- If no object is selected, find closest object not in a locked layer.
-- If such an object is found, select it and return true.
-- In all other cases, return false.

function MODEL:updateCloseSelection()
  local pos = self.ui:unsnappedPos()
  local p = self:page()

  local prim = p:primarySelection()
  if prim then return false end

  -- no selection: find closest object
  local closest
  local bound = 1000000
  for i,obj,sel,layer in p:objects() do
     if p:visible(self.vno, i) and not p:isLocked(layer) then
	local d = p:distance(i, pos, bound)
	if d < bound then closest = i; bound = d end
     end
  end
  if closest then
    p:setSelect(closest, 1)
    return true
  else
    return false
  end
end

----------------------------------------------------------------------

-- Move to next/previous view
function MODEL:nextView(delta)
  if 1 <= self.vno + delta and self.vno + delta <= self:page():countViews() then
    self.vno = self.vno + delta;
    self:deselectNotInView()
  elseif 1 <= self.pno + delta and self.pno + delta <= #self.doc then
    self.pno = self.pno + delta
    if delta > 0 then
      self.vno = 1;
    else
      self.vno = self:page():countViews()
    end
  end
end

-- Move to next/previous page
function MODEL:nextPage(delta)
  if 1 <= self.pno + delta and self.pno + delta <= #self.doc then
    self.pno = self.pno + delta
    if delta > 0 then
      self.vno = 1;
    else
      self.vno = self:page():countViews()
    end
  end
end

-- change zoom and pan to fit box on screen
function MODEL:fitBox(box)
  if box:isEmpty() then return end
  local cs = self.ui:canvasSize()
  local xfactor = 20.0
  local yfactor = 20.0
  if box:width() > 0 then xfactor = cs.x / box:width() end
  if box:height() > 0 then yfactor = cs.y / box:height() end
  local zoom = math.min(xfactor, yfactor)

  if zoom > prefs.max_zoom then zoom = prefs.max_zoom end
  if zoom < prefs.min_zoom then zoom = prefs.min_zoom end

  self.ui:setPan(0.5 * (box:bottomLeft() + box:topRight()))
  self.ui:setZoom(zoom)
  self.ui:update()
end

----------------------------------------------------------------------

function MODEL:showSource()
  local fname = ipe.folder("latex", "ipetemp.tex")
  self:waitDialog(string.format(prefs.external_editor, fname),
		  "Waiting for external editor")
end

function MODEL:latexErrorBox(log)
  local d = ipeui.Dialog(self.ui:win(), "Ipe: error running Latex")
  d:add("label", "label",
	{ label="An error occurred during the Pdflatex run. " ..
	  "Please consult the logfile below." },
	1, 1)
  d:add("text", "text", { read_only=true, syntax="logfile", focus=true }, 2, 1)
  if prefs.external_editor then
    d:addButton("editor", "&Source", function () self.showSource() end)
  end
  d:addButton("ok", "Ok", "accept")
  d:set("text", log)
  d:setStretch("row", 2, 1)
  d:execute(prefs.latexlog_size)
end

function MODEL:runLatex()
  self.ui:type3Font() -- reset flag in canvas
  self.type3_font = false
  local success, errmsg, result, log
  if prefs.freeze_in_latex then
    success, errmsg, result, log = self.doc:runLatex(self.file_name)
  else
    success, converter, errmsg, result = self.doc:prepareLatexRun()
    if converter then
      self:waitDialog(self.doc:howToRunLatex(self.file_name), "Compiling Latex")
      success, errmsg, result, log = self.doc:completeLatexRun(converter)
    end
  end
  if success then
    self.ui:setResources(self.doc)
    self.ui:update()
    return true
  elseif result == "latex" then
    self:latexErrorBox(log)
    return false
  elseif result == "runlatex" then
    local urlFile = ipe.folder("latex", "url1.txt")
    if ipe.fileExists(urlFile) then
      self:warning("Ipe did not succeed to run Latex to process your text objects.",
		   "Ipe tried to use Latex in the cloud.\n\n"
		     .. "Perhaps you have no internet connectivity right now?\n\n"
		     .. "If you have a local Latex installation, you can "
		     .. "disable Latex-compilation in the cloud "
		     .. "from the Help menu.")
    else
      self:warning("Ipe did not succeed to run Latex to process your text objects.",
		   "You either have no Latex installation on your computer, "
		     .. "or the Latex executable is not on your command path.\n\n"
		     .. "If you prefer, you can enable Latex-compilation in the cloud "
		     .. "from the Help menu.")
    end
  else
    self:warning("An error occurred during the Pdflatex run", errmsg)
  end
end

function MODEL:autoRunLatex()
  if self.auto_latex then
    self:runLatex()
  end
end

-- checks if document is modified, and asks user if it can be discarded
-- returns true if object is unmodified or user confirmed discarding
function MODEL:checkModified()
  if self:isModified() then
    return messageBox(self.ui:win(), "question",
				 "The document has been modified",
				 "Do you wish to discard the current document?",
				 "discardcancel") == 0
  else
    return true
  end
end

----------------------------------------------------------------------

function MODEL:newDocument()
  self.pno = 1
  self.vno = 1
  self.file_name = nil

  self.undo = { {} }
  self.redo = {}
  self:markAsUnmodified()

  self.doc = ipe.Document()
  for _, w in ipairs(config.styleList) do
    local sheet = ipe.Sheet(w)
    if not sheet then
      self:warning("Style sheet '" .. w .. "' could not be read.")
    else
      self.doc:sheets():insert(1, sheet)
    end
  end
  local p = self.doc:properties()
  p.tex = prefs.tex_engine
  self.doc:setProperties(p)

  self:setPage()

  self.ui:setupSymbolicNames(self.doc:sheets())
  self.ui:setAttributes(self.doc:sheets(), self.attributes)
  self:resetGridSize()

  self:action_normal_size()
  self:setSnap()

  self.ui:explain("New document")
end

function MODEL:loadDocument(fname)
  self:preloadFile(fname)
  local err = self:tryLoadDocument(fname)
  if err then
    self:warning("Document '" .. fname .. "' could not be opened", err)
  end
end

-- returns error message if there was an error
function MODEL:tryLoadDocument(fname)
  local doc, err = ipe.Document(fname)
  if doc then
    self.doc = doc
    self.file_name = fname
    self.pno = 1
    self.vno = 1

    self.undo = { {} }
    self.redo = {}
    self:markAsUnmodified()

    self:setPage()

    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
    self:resetGridSize()

    local syms = self.doc:checkStyle()

    self:action_normal_size()
    self:setSnap()

    self.ui:explain("Document '" .. fname .. "' loaded")

    self:updateRecentFiles(fname)

    if self.auto_latex and not self.first_show then
      self:runLatex()
    end

    if #syms > 0 then
      self:warning("The document contains symbolic attributes " ..
		   "that are not defined in the style sheet:",
		 "* " .. table.concat(syms, "\n* "))
    end
    return nil
  else
    return err
  end
end

function MODEL:saveDocument(fname)
  if not fname then
    fname = self.file_name
  end
  local fm = formatFromFileName(fname)
  if not fm then
    self:warning("File not saved!",
		 "You must save as *.xml, *.ipe, or *.pdf")
    return
  end

  -- run Latex if format is not XML
  if fm ~= "xml" and not self:runLatex() then
    self.ui:explain("Latex error - file not saved")
    return
  end

  local props = self.doc:properties()
  props.modified = "D:" .. ipeui.currentDateTime()
  if props.created == "" then props.created = props.modified end
  props.creator = config.version
  self.doc:setProperties(props)

  if not self.doc:save(fname, fm) or not self:persistFile(fname) then
    self:warning("File not saved!", "Error saving the document")
    return
  end

  if fm == "xml" and #prefs.auto_export > 0 then
    self:auto_export(fname)
  end

  self:markAsUnmodified()
  self.ui:explain("Saved document '" .. fname .. "'")
  self.file_name = fname
  self:setCaption()
  self:updateRecentFiles(fname)
  return true
end

function MODEL:auto_export(fname)
  if not self:runLatex() then
    self.ui:explain("Latex error - could not auto-export!")
  else
    for _, format  in ipairs(prefs.auto_export) do
      local ename = fname:sub(1,-4) .. format
      if format == "pdf" then
	if not self.doc:save(ename, "pdf", { export=true } ) then
	  self:warning("Auto-exporting failed",
		       "I could not export in PDF format to file '" .. ename .. "'.")
	end
      else
	self.ui:renderPage(self.doc, 1, 1,
			   format, ename, prefs.auto_export_resolution / 72.0,
			   true, false) -- transparent, nocrop
      end
      if not self:persistFile(ename) then
	self:warning("Auto-exporting failed",
		     "I could not persist the exported file '" .. ename .. "'.")
      end
    end
  end
end

function MODEL:updateRecentFiles(fname)
  if fname then
    local i = indexOf(fname, self.recent_files)
    if i then
      table.remove(self.recent_files, i)
    end
    table.insert(self.recent_files, 1, fname)
    while #self.recent_files > prefs.num_recent_files do
      table.remove(self.recent_files)
    end
    local w = ipe.folder("latex", "recent_files.lua")
    local fd = ipe.openFile(w, "w")
    if fd then
      fd:write("recent_files = {\n")
      for _, f in ipairs(self.recent_files) do
	fd:write(string.format(" %q,\n", f))
      end
      fd:write("}\n")
      fd:close()
    end
  end
  self.ui:setRecentFiles(self.recent_files)
end

-- on OSX called without a MODEL
function action_recent_file(fname)
  MODEL.new(nil, fname)
end

function MODEL:recent_file(fname)
  if not self:checkModified() then return end
  self:loadDocument(fname)
end

----------------------------------------------------------------------

function MODEL:autosave()
  -- only autosave if document has been modified
  if not self:isModified() then return end
  local f
  if self.file_name then
    if prefs.autosave_filename:find("%%s") then
      f = self.file_name:match(prefs.basename_pattern) or self.file_name
      f = string.format(prefs.autosave_filename, f)
    else
      f = prefs.autosave_filename
    end
    if f:sub(1, 1) ~= prefs.fsep then -- relative filename
      local d = self.file_name:match(prefs.dir_pattern)
      if d then
        f = d .. prefs.fsep .. f
      end
    end
  else
    f = prefs.autosave_unnamed
  end
  self.ui:explain("Autosaving to " .. f .. "...")
  if not self.doc:save(f, "xml") then
    messageBox(self.ui:win(), "critical",
	       "Autosaving failed!\nFilename: " .. f)
  end
end

----------------------------------------------------------------------

function MODEL:closeEvent()
  if self == first_model then first_model = nil end
  if self:isModified() then
    local r = messageBox(self.ui:win(), "question",
			 "The document has been modified",
			 "Do you wish to save the document?",
			 "savediscardcancel")
    if r == 0 or (r == 1 and self:action_save()) then
      self.okay_close = true
      self.ui:close()
    end
  else
    self.okay_close = true
    self.ui:close()
  end
end

----------------------------------------------------------------------

-- TODO:  limit on undo stack size?
function MODEL:registerOnly(t)
  self.pristine = false
  -- store it on undo stack
  self.undo[#self.undo + 1] = t
  -- flush redo stack
  self.redo = {}
  self:setPage()
end

function MODEL:register(t)
  -- store selection
  t.original_selection = self:selection()
  t.original_primary = self:page():primarySelection()
  -- perform action
  t.redo(t, self.doc)
  self:registerOnly(t)
  self.ui:explain(t.label)
  if t.style_sheets_changed then
    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
    self:resetGridSize()
  end
end

function MODEL:creation(label, obj)
  local p = self:page()
  local active = p:active(self.vno)
  if not p:visible(self.vno, active) then
    self:warning("Active layer is invisible",
		 "You have just created an object in layer '" ..
		   active .. "'.\n\n" ..
		   "This layer is currently not visible, so don't be surprised " ..
		   "that you can't see your new object!")
  end
  local t = { label=label, pno=self.pno, vno=self.vno,
	      layer=self:page():active(self.vno), object=obj }
  t.undo = function (t, doc) doc[t.pno]:remove(#doc[t.pno]) end
  t.redo = function (t, doc)
	     doc[t.pno]:deselectAll()
	     doc[t.pno]:insert(nil, t.object, 1, t.layer)
	   end
  self:register(t)
end

function MODEL:transformation(mode, m, deselect)
  local t = { label = mode,
	      pno = self.pno,
	      vno = self.vno,
	      selection = self:selection(),
	      original = self:page():clone(),
	      matrix = m,
	      undo = revertOriginal,
	      deselect = deselect,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do
	       p:transform(i, t.matrix)
	     end
	     if t.deselect then
	       p:deselectAll()
	     end
	   end
  self:register(t)
end

function MODEL:setAttribute(prop, value)
  local t = { label="set attribute " .. prop .. " to " .. tostring(value),
	      pno=self.pno,
	      vno=self.vno,
	      selection=self:selection(),
	      original=self:page():clone(),
	      property=prop,
	      value=value,
	      undo=revertOriginal,
	      stroke=self.attributes.stroke,
	      fill=self.attributes.fill,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     local changed = false
	     for _,i in ipairs(t.selection) do
	       if p:setAttribute(i, t.property, t.value, t.stroke, t.fill) then
		 changed = true
	       end
	     end
	     return changed
	   end
  if (t.redo(t, self.doc)) then
    t.original_selection = t.selection
    t.original_primary = self:page():primarySelection()
    self:registerOnly(t)
  end
end

----------------------------------------------------------------------

HELPER = {}
HELPER.__index = HELPER

function HELPER.new(model, parameters)
  local helper = {}
  setmetatable(helper, HELPER)
  helper.model = model
  helper.parameters = parameters
  return helper
end

function HELPER:message(m)
  self.model.ui:explain(m)
end

function HELPER:messageBox(text, details, buttons)
  return messageBox(self.model.ui:win(), nil, text, details, buttons)
end

function HELPER:getString(m, s)
  return self.model:getString(m, nil, s)
end

function MODEL:runIpelet(label, ipelet, num, parameters)
  local helper = HELPER.new(self, parameters)
  local t = { label="ipelet '" .. label .."'",
	      pno=self.pno,
	      vno=self.vno,
	      original=self:page():clone(),
	      undo=revertOriginal,
	      redo=revertFinal,
	      original_selection = self:selection(),
	      original_primary = self:page():primarySelection(),
	    }
  local need_undo = ipelet:run(num or 1, self:page(), self.doc,
			       self.pno, self.vno,
			       self:page():active(self.vno),
			       self.attributes, self.snap,
			       helper)
  if need_undo then
    t.final = self:page():clone()
    self:registerOnly(t)
  end
  self:setPage()
end

----------------------------------------------------------------------

function MODEL:action_undo()
  if #self.undo <= 1 then
    self.ui:explain("No more undo information available")
    return
  end
  t = self.undo[#self.undo]
  table.remove(self.undo)
  t.undo(t, self.doc)
  self.ui:explain("Undo '" .. t.label .. "'")
  self.redo[#self.redo + 1] = t
  if t.pno then
    self.pno = t.pno
  elseif t.pno0 then
    self.pno = t.pno0
  end
  if t.vno then
    self.vno = t.vno
  elseif t.vno0 then
    self.vno = t.vno0
  end
  local p = self:page()
  p:deselectAll()
  if t.original_selection then
    for _, no in ipairs(t.original_selection) do
      p:setSelect(no, 2)
    end
  end
  if t.original_primary then
    p:setSelect(t.original_primary, 1)
  end
  self:setPage()
  if t.style_sheets_changed then
    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
    self:resetGridSize()
  end
end

function MODEL:action_redo()
  if #self.redo == 0 then
    self.ui:explain("No more redo information available")
    return
  end
  t = self.redo[#self.redo]
  table.remove(self.redo)
  t.redo(t, self.doc)
  self.ui:explain("Redo '" .. t.label .. "'")
  self.undo[#self.undo + 1] = t
  if t.pno then
    self.pno = t.pno
  elseif t.pno1 then
    self.pno = t.pno1
  end
  if t.vno then
    self.vno = t.vno
  elseif t.vno1 then
    self.vno = t.vno1
  end
  self:page():deselectAll()
  self:setPage()
  if t.style_sheets_changed then
    self.ui:setupSymbolicNames(self.doc:sheets())
    self.ui:setAttributes(self.doc:sheets(), self.attributes)
    self:resetGridSize()
  end
end

----------------------------------------------------------------------
