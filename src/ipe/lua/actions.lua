----------------------------------------------------------------------
-- actions.lua
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

-- main entry point to all actions
-- (except mouseaction, selector)

-- protects call, then distributes to action_xxx method or
-- saction_xxx methods (the latter are only called if a selection exists)
function MODEL:action(a)
  local result, err = xpcall(function () self:paction(a) end,
			     debug.traceback)
  if not result then
    messageBox(nil, "critical",
		     "Lua error\n\n"..
		       "Data may have been corrupted. \n" ..
		       "Save your file!",
		     err)
  end
end

function MODEL:paction(a1)
  -- work-around for bug in Qt 5.5, to be replaced with something better:
  local a = a1:gsub("&", "")
  -- print("MODEL:paction(" .. a .. ")")
  if a:sub(1,5) == "mode_" then
    self.mode = a:sub(6)
    self.ui:setInkMode(self.mode == "ink")
    if prefs.no_ink_snap then
      -- disable snapping in ink mode
      if self.mode == "ink" then
	self.savedsnap = {}
	for i,e in ipairs(MODEL.snapmodes) do
	  self.savedsnap[e] = self.snap[e]
	  self.snap[e] = false
	end
      elseif self.savedsnap then
	-- reenable snapping when switching back to another mode
	for i,e in ipairs(MODEL.snapmodes) do
	  self.snap[e] = self.savedsnap[e]
	end
	self.savedsnap = nil
      end
      for i,e in ipairs(MODEL.snapmodes) do
	self.ui:setActionState(e, self.snap[e])
      end
      self:setSnap()
    end
  elseif a:sub(1,4) == "snap" then
    self.snap[a] = self.ui:actionState(a)
    self:setSnap()
    self.ui:setFifiVisible(true)
  elseif a:sub(1,7) == "ipelet_" then
    self:action_ipelet(a)
  elseif a:sub(1,14) == "selectinlayer-" then
    self:action_select_in_layer(a:sub(15))
  elseif a:sub(1,12) == "movetolayer-" then
    self:action_move_to_layer(a:sub(13))
  else
    local f = self["action_" .. a]
    local ff = self["saction_" .. a]
    if f then
      f(self)
    elseif ff then
      if not self:page():hasSelection() then
	self.ui:explain("no selection")
	return
      end
      ff(self)
    else
      self:warning("Operation '" .. a .. "' is not yet implemented")
    end
  end
  self:checkType3Font()
end

local function has_text(page)
  for i,obj,sel,layer in page:objects() do
    if sel and obj:type() == "text" then return true end
  end
  return false
end

-- Attribute selector
function MODEL:selector(prop, value)
  if prop == "gridsize" or prop == "anglesize" then
    local abs = self.doc:sheets():find(prop, value)
    self.snap[prop] = abs
    self:setSnap()
    self:setPage()
    self.ui:setGridAngleSize(self.snap.gridsize, self.snap.anglesize)
    return
  end
  if prop == "markshape" then
    local s = "mark/" .. value
    local name
    for _, ms in ipairs(self.doc:sheets():allNames("symbol")) do
      if ms:sub(1, #s) == s then
	name = ms
	break
      end
    end
    if not name then return end -- not found
    value = name
  end
  if value == "<absolute>" then
    local proptype = prop
    if prop == "stroke" or prop == "fill" then proptype = "color" end
    value = self.attributes[prop]
    if type(value) == "string" then
      value = self:findOldValue(prop, proptype)
    end
  end
  self.attributes[prop] = value
  self.ui:setAttributes(self.doc:sheets(), self.attributes)
  if self:page():hasSelection() then
    self:setAttribute(prop, value)
    if prop == "textsize" or (prop == "stroke" and has_text(self:page())) then
      self:autoRunLatex()
    end
  end
  -- self:print_attributes()
end

function MODEL:set_absolute(prop, value)
  self.attributes[prop] = value
  self.ui:setAttributes(self.doc:sheets(), self.attributes)
  if self:page():hasSelection() then
    self:setAttribute(prop, value)
  end
  -- self:print_attributes()
end

function MODEL:layerAction(a, layer, target)
  local name = a
  local arg = nil
  if a:sub(1,4) == "snap" then
    name = "snap"
    arg = a:sub(5)
  elseif a:sub(-2) == "on" then
    name = a:sub(1,-3)
    arg = true
  elseif a:sub(-3) == "off" then
    name = a:sub(1,-4)
    arg = false
  else
    arg = target
  end
  local f = self["layeraction_" .. name]
  if f then
    f(self, layer, arg)
  else
    print("Unimplemented layer action:", a, layer, name, arg)
  end
end

function MODEL:bookmark(index)
  -- print("Bookmark", index)
  self.pno = self:findPageForBookmark(index)
  self.vno = 1
  self:setPage()
end

function MODEL:findOldValue(button, btype)
  local old = self.attributes[button]
  if type(old) == "string" then
    old = self.doc:sheets():find(btype, old)
  end
  return old
end

function MODEL:absoluteButton(button)
  -- print("Button:", button)
  if button == "stroke" or button == "fill" then
    local old = self:findOldValue(button, "color")
    r, g, b = ipeui.getColor(self.ui:win(), "Select " .. button .. " color",
			     old.r, old.g, old.b)
    if r then
      self:set_absolute(button, { r = r, g = g, b = b });
    elseif config.platform == "apple" then
      self:warning("Selecting absolute colors",
      	           "I have displayed the MacOS color selection dialog.\n\n" ..
		   "After you have found your color in this dialog, " ..
		   "you'll need to press the absolute color button again to " ..
		   "apply the new color to your selection in Ipe.")
    end
  elseif button == "pen" then
    local old = self:findOldValue(button, button)
    local d = self:getDouble("Select pen", "Pen width:", old, 0, 1000)
    if d then
      self:set_absolute(button, d)
    end
  elseif button == "textsize" then
    local old = self:findOldValue(button, button)
    local d = self:getDouble("Select text size", "Text size in points:",
			     10, 2, 1000)
    if d then
      self:set_absolute(button, d)
      self:autoRunLatex()
    end
  elseif button == "symbolsize" then
    local old = self:findOldValue(button, button)
    local d = self:getDouble("Select symbol size", "Symbol size:",
			     old, 0, 1000)
    if d then
      self:set_absolute(button, d)
    end
  elseif button == "view" then
    self:action_jump_view()
  elseif button == "page" then
    self:action_jump_page()
  elseif button == "viewmarked" then
    self:action_mark_view(self.ui:actionState(button))
  elseif button == "pagemarked" then
    self:action_mark_page(self.ui:actionState(button))
  else
    print("Unknown button: ", button)
  end
end

function MODEL:action_jump_view()
  local d = self.ui:selectPage(self.doc, self.pno, self.vno)
  if d then
    self.vno = d
    self:setPage()
  end
end

function MODEL:action_jump_page()
  local d = self.ui:selectPage(self.doc, nil, self.pno)
  if d then
    self.pno = d
    self.vno = 1
    self:setPage()
  end
end

function MODEL:action_mark_view(m)
  local t = { label="set view mark to " .. tostring(m),
	      pno=self.pno,
	      vno=self.vno,
	      original=self:page():markedView(self.vno),
	      final=m,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setMarkedView(t.vno, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setMarkedView(t.vno, t.final)
	   end
  self:register(t)
end

function MODEL:mark_views_from_view(m, label)
  local t = { label=label,
	      pno=self.pno,
	      vno=self.vno,
	      original=self:page():clone(),
	      undo=revertOriginal,
	      final=m,
	    }
  t.redo = function (t, doc)
	     for i = t.vno, doc[t.pno]:countViews() do
	       doc[t.pno]:setMarkedView(i, t.final)
	     end
	   end
  self:register(t)
end

function MODEL:action_mark_from_view()
  self:mark_views_from_view(true, "mark some views")
end

function MODEL:action_unmark_from_view()
  self:mark_views_from_view(false, "unmark some views")
end

function MODEL:action_mark_page(m)
  local t = { label="set page mark to " .. tostring(m),
	      pno=self.pno,
	      original=self:page():marked(),
	      final=m,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setMarked(t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setMarked(t.final)
	   end
  self:register(t)
end

function MODEL:action_stop()
  self.ui:finishTool()
end

----------------------------------------------------------------------

function MODEL:showPathStylePopup(v)
  local a = self.attributes
  local m = ipeui.Menu(self.ui:win())
  local sheet = self.doc:sheets()
  m:add("pathmode", "Stroke && Fill",
	{ "stroked", "strokedfilled", "filled"},
	{ "stroke only", "stroke && fill", "fill only" },
	a.pathmode)
  local dashstyles = sheet:allNames("dashstyle")
  m:add("dashstyle", "Dash style", dashstyles, nil, a.dashstyle)
  local arrowsizes = sheet:allNames("arrowsize")
  m:add("farrowsize", "Forward arrow size", arrowsizes, nil, a.farrowsize)
  m:add("rarrowsize", "Reverse arrow size", arrowsizes, nil, a.rarrowsize)
  local arrowshapes = symbolNames(sheet, "arrow/", "(spx)")
  m:add("farrowshape", "Forward arrow shape", arrowshapes,
	arrowshapeToName, a.farrowshape)
  m:add("rarrowshape", "Reverse arrow shape", arrowshapes,
	arrowshapeToName, a.rarrowshape)

  local opacities = sheet:allNames("opacity")
  m:add("opacity", "Opacity", opacities, nil, a.opacity)
  m:add("strokeopacity", "Stroke opacity", opacities, nil, a.strokeopacity)

  local tilings = sheet:allNames("tiling")
  table.insert(tilings, 1, "normal")
  m:add("tiling", "Tiling pattern", tilings, nil, a.tiling)

  local gradients = sheet:allNames("gradient")
  table.insert(gradients, 1, "normal")
  m:add("gradient", "Gradient pattern", gradients, nil, a.gradient)

  m:add("linejoin", "Line join", { "normal", "miter", "round", "bevel" },
	nil, a.linejoin)
  m:add("linecap", "Line cap", { "normal", "butt", "round", "square" },
	nil, a.linecap)
  m:add("fillrule", "Fill rule", { "normal", "evenodd", "wind" },
	nil, a.fillrule)

  local r, n, value = m:execute(v.x, v.y)
  if r then self:selector(r, value) end
end

----------------------------------------------------------------------

function MODEL:showLayerBoxPopup(v, layer)
  local p = self:page()
  local m = ipeui.Menu(self.ui:win())
  local active = p:active(self.vno)
  m:add("visible", "Make " .. layer .. " visible from view "
	.. self.vno .. " onwards")
  m:add("invisible", "Make " .. layer .. " invisible from view "
	.. self.vno .. " onwards")
  m:add("rename", "Rename " .. layer)
  m:add("delete", "Delete " .. layer)
  if layer ~= active then
    m:add("merge", "Merge into " .. active)
  end
  if p:isLocked(layer) then
    m:add("lockoff", "Unlock " .. layer)
  else
    m:add("lockon", "Lock " .. layer)
  end
  local sm = p:snapping(layer)
  if sm ~= "never" then
    m:add("snapnever", "Disable snapping for " .. layer)
  end
  if sm ~= "visible" then
    m:add("snapvisible", "Snap to " .. layer .. " when visible")
  end
  if sm ~= "always" then
    m:add("snapalways", "Always snap to " .. layer)
  end
  local layers = p:layers()
  if #layers > 1 then
    local targets = {}
    local i0 = indexOf(layer,layers)
    if i0 ~= 1 then targets[1] = "_top_" end
    for i,l in ipairs(layers) do
      if i ~= i0 - 1 and i ~= i0 then
	targets[#targets + 1] = l
      end
    end
    m:add("move", "Move " .. layer, targets,
	  function (i, l) if l == "_top_" then return "to top"
	    else return "after " .. l end end)
  end
  local r, no, subitem = m:execute(v.x, v.y)
  if r then
    self:layerAction(r, layer, subitem)
  end
end

----------------------------------------------------------------------

function MODEL:layeraction_make_visible(layer, arg, vis)
  local p = self:page()
  local label
  if vis then
    label = "make layer " .. layer .. " visible from view "
      .. self.vno .. " onwards"
  else
    label = "make layer " .. layer .. " invisible from view "
      .. self.vno .. " onwards"
  end
  local t = { label=label,
	      pno=self.pno,
	      vno=self.vno,
	      original=p:clone(),
	      layer=layer,
	      undo=revertOriginal
	    }
  t.redo = function (t, doc)
	     for j = t.vno, doc[t.pno]:countViews() do
	       doc[t.pno]:setVisible(j, t.layer, vis)
	     end
	   end
  self:register(t)
end

function MODEL:layeraction_visible(layer, arg)
  self:layeraction_make_visible(layer, arg, true)
end

function MODEL:layeraction_invisible(layer, arg)
  self:layeraction_make_visible(layer, arg, false)
end

function MODEL:layeraction_select(layer, arg)
  local p = self:page()
  local t = { label="set visibility of layer " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      layer=layer,
	      original=p:visible(self.vno, layer),
	      visible=arg,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setVisible(t.vno, t.layer, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setVisible(t.vno, t.layer, t.visible)
	   end
  self:register(t)
  self:deselectNotInView()
end

function MODEL:layeraction_lock(layer, arg)
  local p = self:page()
  for j = 1, p:countViews() do
    if layer == p:active(j) then
      self:warning("Cannot lock layer '" .. layer .. "'.",
		   "Layer '" .. layer .. "' is the active layer of view "
		     .. j .. ".")
      return
    end
  end
  local t = { label="set locking of layer " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      layer=layer,
	      original=p:isLocked(layer),
	      locked=arg,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setLocked(t.layer, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setLocked(t.layer, t.locked)
	   end
  self:register(t)
  self:deselectNotInView()
  self:setPage()
end

function MODEL:layeraction_snap(layer, arg)
  local p = self:page()
  local t = { label="set snapping in layer " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      layer=layer,
	      original=p:snapping(layer),
	      snapping=arg,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setSnapping(t.layer, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setSnapping(t.layer, t.snapping)
	   end
  self:register(t)
end

function MODEL:layeraction_move(layer, arg)
  local p = self:page()
  local target = 1
  local current = indexOf(layer, p:layers())
  if arg ~= "_top_" then
    target = indexOf(arg, p:layers())
    if target < current then target = target + 1 end
  end
  local t = { label="move layer " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      original=p:clone(),
	      layer=layer,
	      target=target,
	      undo=revertOriginal
	    }
  t.redo = function (t, doc)
	     doc[t.pno]:moveLayer(t.layer, t.target)
	   end
  self:register(t)
end

function MODEL:layeraction_active(layer)
  local p = self:page()
  if p:isLocked(layer) then
    self:warning("Cannot change active layer",
		 "A locked layer cannot be the active layer")
    return
  end
  local t = { label="select active layer " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      original=p:active(self.vno),
	      final=layer,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setActive(t.vno, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setActive(t.vno, t.final)
	   end
  self:register(t)
end

function MODEL:layeraction_rename(layer)
  local p = self:page()
  local name = self:getString("Enter new name for layer '" .. layer .. "'",
			      nil, layer)
  if not name then return end
  name = string.gsub(name, "%s+", "_")
  name = string.gsub(name, "\"+", "_")
  if indexOf(name, p:layers()) then
    self:warning("Cannot rename layer '" .. layer .. "'.",
		 "The name '" .. name .. "' is already in use.")
    return
  end
  local t = { label="rename layer " .. layer .. " to " .. name,
	      pno=self.pno,
	      vno=self.vno,
	      original=layer,
	      final=name,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:renameLayer(t.final, t.original)
	     for i = 1,p:countViews() do
	       if p:active(i) == t.final then p:setActive(i, t.original) end
	     end
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:renameLayer(t.original, t.final)
	     for i = 1,p:countViews() do
	       if p:active(i) == t.original then p:setActive(i, t.final) end
	     end
	   end
  self:register(t)
end

function MODEL:layeraction_delete(layer)
  local p = self:page()
  for j = 1, p:countViews() do
    if layer == p:active(j) then
      self:warning("Cannot delete layer '" .. layer .. "'.",
		   "Layer '" .. layer .. "' is the active layer of view "
		     .. j .. ".")
      return
    end
  end
  local t = { label="delete layer " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      original=p:clone(),
	      layer=layer,
	      undo=revertOriginal
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = #p,1,-1 do
	       if p:layerOf(i) == t.layer then
		 p:remove(i)
	       end
	     end
	     p:removeLayer(t.layer)
	   end
  self:register(t)
end

function MODEL:layeraction_merge(layer)
  local p = self:page()
  local active = p:active(self.vno)
  for j = 1, p:countViews() do
    if layer == p:active(j) then
      self:warning("Cannot remove layer '" .. layer .. "'.",
		   "Layer '" .. layer .. "' is the active layer of view "
		     .. j .. ".")
      return
    end
  end
  local t = { label="merge layer " .. layer .. " into " .. active,
	      pno=self.pno,
	      vno=self.vno,
	      original=p:clone(),
	      layer=layer,
	      target=active,
	      undo=revertOriginal
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = 1,#p do
	       if p:layerOf(i) == t.layer then
		 p:setLayerOf(i, t.target)
	       end
	     end
	     p:removeLayer(t.layer)
	   end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:action_ipelet(a)
  local i = a:find("_",8)
  local method = tonumber(a:sub(8,i-1))
  local name = a:sub(i+1)
  for _, ipelet in ipairs(ipelets) do
    if ipelet.name == name then
      if (ipelet.methods and ipelet.methods[method] and
	  ipelet.methods[method].run) then
	ipelet.methods[method].run(self, method)
      else
	ipelet.run(self, method)
      end
      self:page():ensurePrimarySelection()
      break
    end
  end
  self.ui:update()
end

----------------------------------------------------------------------

function action_new_window()
  MODEL.new()
end

function MODEL:action_new_window()
  MODEL.new()
end

function MODEL:action_run_latex()
  self:runLatex()
end

function MODEL:action_close()
  self.ui:close()
end

function MODEL:action_auto_latex()
  self.auto_latex = self.ui:actionState("auto_latex")
  -- print("Auto is", self.auto_latex)
end

----------------------------------------------------------------------

function MODEL:action_new()
  if not self:checkModified() then return end
  self:newDocument()
end

function action_open()
  local s, f = ipeui.fileDialog(nil, "open", "Open file", filter_ipe)
  if s then
    MODEL.new(nil, s)
  end
end

function MODEL:action_open()
  if not self:checkModified() then return end
  local s, f = ipeui.fileDialog(self.ui:win(), "open", "Open file", filter_ipe)
  if s then
    self:loadDocument(s)
  else
    self.ui:explain("Loading canceled")
  end
end

function MODEL:action_save()
  if not self.file_name then
    return self:action_save_as()
  else
    return self:saveDocument()
  end
end

function MODEL:action_save_as()
  local dir
  if self.file_name then dir = self.file_name:match(prefs.dir_pattern) end
  if not dir and #self.recent_files > 0 then
    dir = self.recent_files[1]:match(prefs.dir_pattern)
  end
  if not dir then
    dir = prefs.save_as_directory
  end
  local name
  local filter
  if self.file_name then
    name = self.file_name
    local fmt = formatFromFileName(self.file_name)
    if fmt then
      filter = indexOf(fmt, { "xml", "pdf" })
      name = self.file_name:sub(1,-5)
    end
  end
  local s, f =
    ipeui.fileDialog(self.ui:win(), "save", "Save file as",
		     filter_save, dir, name, filter)
  local fmap = { ".ipe", ".pdf" }
  if s then
    if not formatFromFileName(s) then
      s = s .. fmap[f]
    end
    -- Cocoa handles the overwrite dialog itself
    if config.toolkit ~= "cocoa" and ipe.fileExists(s) then
      local b = messageBox(self.ui:win(), "question",
			   "File already exists!",
			   "Do you wish to overwrite '" .. s .. "' ?",
			   "okcancel")
      if b ~= 1 then
	self.ui:explain("File not saved")
	return
      end
    end
    return self:saveDocument(s)
  end
end

function MODEL:compute_rect(w, h, res)
  if res == V(0,0) then res = V(72.0, 72.0) end

  local frame = self.doc:sheets():find("layout").framesize

  local dx = (w * 72.0) / res.x
  local dy = (h * 72.0) / res.y

  local xfactor = 1.0
  if (dx > frame.x) then
    xfactor = frame.x / dx
  end
  local  yfactor = 1.0
  if (dy > frame.y) then
    yfactor = frame.y / dy
  end
  local factor = yfactor
  if (xfactor < yfactor) then factor = xfactor end
  dx = dx * factor
  dy = dy * factor

  local v = 0.5 * V(frame.x - dx, frame.y - dy)
  local rect = ipe.Rect()
  rect:add(v)
  rect:add(v + V(dx, dy))
  return rect
end

function MODEL:export(format)
  local dir
  if self.file_name then dir = self.file_name:match(prefs.dir_pattern) end
  if not dir then dir = prefs.save_as_directory end
  local title, filter
  if format == "png" then
    title = "Export to PNG"
    filter = filter_png
  elseif format == "eps" then
    title = "Export to EPS"
    filter = filter_eps
  else
    title = "Export to SVG"
    filter = filter_svg
  end
  local s, f =
    ipeui.fileDialog(self.ui:win(), "save", title, filter, dir)
  if s then
    if config.toolkit ~= "cocoa" and ipe.fileExists(s) then
      local b = messageBox(self.ui:win(), "question",
			   "File already exists!",
			   "Do you wish to overwrite?\n\n" .. s,
			   "okcancel")
      if b ~= 1 then
	self.ui:explain("Document not exported")
	return
      end
    end
    self.ui:renderPage(self.doc, self.pno, self.vno,
       		       format, s, self.ui:zoom(),
		       true, false) -- transparent, nocrop
  end
end

function MODEL:action_export_eps()
  self:export("eps")
end

function MODEL:action_export_png()
  self:export("png")
end

function MODEL:action_export_svg()
  self:export("svg")
end

function MODEL:action_insert_image()
  if not self.insert_image_dir and self.file_name then
    self.insert_image_dir = self.file_name:match(prefs.dir_pattern)
  end
  local s, f = ipeui.fileDialog(self.ui:win(), "open", "Insert image",
				filter_images, self.insert_image_dir)
  if s then
    self.insert_image_dir = s:match(prefs.dir_pattern)
    local format = "png"
    if s:sub(-4):lower() == ".jpg" or s:sub(-5):lower() == ".jpeg"
    then format = "jpeg" end
    local bitmap, res = ipe.readImage(s, format)
    if not bitmap then
      messageBox(self.ui:win(), "warning", "Could not read image",
		 res, "ok")
    else
      local info = bitmap:info()
      local r = self:compute_rect(info.width, info.height, res)
      local obj = ipe.Image(r, bitmap)
      self:creation("insert image", obj)
    end
  else
    self.ui:explain("Insert image canceled")
  end
end

----------------------------------------------------------------------

-- this function is called without arguments on OSX from ipeAlwaysAction
function action_manual()
  local url = config.docdir .. "/manual.html"
  if ipe.fileExists(url) then
    url = "file:///" ..  url
  else
    url = prefs.manual_url
  end
  if ipeui.startBrowser then
    ipeui.startBrowser(url)
  else
    os.execute(prefs.browser:format(url))
  end
end

function MODEL:action_manual()
  action_manual()
end

-- this function is called without arguments on OSX from ipeAlwaysAction
function action_show_libraries(win)
  local s = "Ipe relies on the following fine pieces of software:\n"
  s = s .. "\n * Pdftex, Xetex, or Luatex"
  s = s .. "\n * " .. _VERSION .. " (" .. math.floor(collectgarbage("count")) .. " kB used)"
  s = s .. "\n * The font rendering library " .. config.freetype_version
  s = s .. "\n * The rendering library Cairo " .. config.cairo_version
  s = s .. "\n * The clothoid spline library libspiro " .. config.spiro_version
  s = s .. "\n * The GNU scientific library gsl " .. config.gsl_version
  if config.qt_version then
    s = s .. "\n * The GUI toolkit Qt " .. config.qt_version
  end
  s = s .. "\n * The compression library zlib " .. config.zlib_version
  messageBox(win, "information", "Ipe libraries", s)
end

-- this function is called without arguments on OSX from ipeAlwaysAction
function action_show_configuration(win, dpi)
  local s = ""
  s = s .. " * Lua code: " .. package.path
  s = s .. "\n * Style directories:\n  - " ..
    table.concat(config.styleDirs, "\n  - ")
  s = s .. "\n * Styles for new documents: " ..
    table.concat(prefs.styles, ", ")
  s = s .. "\n * Autosave file: " .. prefs.autosave_filename
  s = s .. "\n * Save-as directory: " .. prefs.save_as_directory
  s = s .. "\n * Documentation: " .. config.docdir
  s = s .. "\n * Ipelets:\n   - " .. table.concat(config.ipeletDirs, "\n   - ")
  s = s .. "\n * Latex program path: " .. config.latexpath
  s = s .. "\n * Latex directory: " .. config.latexdir
  s = s .. "\n * Icons: " .. config.icons
  s = s .. "\n * Screen geometry: " .. config.screen_geometry[1] .. " x " .. config.screen_geometry[2]
  if config.platform == "win" then
    s = s .. "\n * Monitor resolution: " .. dpi .. " dpi"
  end
  if config.toolkit == "qt" then
    s = s .. "\n * Device pixel ratio: " .. config.device_pixel_ratio
  end
  s = s .. "\n * External editor: " .. (prefs.external_editor or "none")
  messageBox(win, "information", "Ipe configuration", s)
end

function MODEL:action_show_configuration()
  local cs, dpi = self.ui:canvasSize()
  action_show_configuration(self.ui:win(), dpi)
end

function MODEL:action_show_libraries()
  action_show_libraries(self.ui:win())
end

-- this function is called without arguments on OSX from ipeAlwaysAction
function action_about_ipelets(win)
  local s = ""
  for i,v in ipairs(ipelets) do
    if v.label and v.about then
      s = s .. "### " .. v.label .. " ###\n\n" .. v.about .. "\n"
    end
  end
  local d = ipeui.Dialog(win, "Ipe: About the ipelets")
  d:add("text", "text", { read_only=true }, 1, 1)
  d:set("text", s)
  d:addButton("ok", "Ok", "accept")
  d:execute(prefs.latexlog_size)
end

function MODEL:action_about_ipelets()
  action_about_ipelets(self.ui:win())
end

function MODEL:action_preferences()
  local d = ipeui.Dialog(win, "Ipe: Preferences")
  local s = "Ipe preferences are changed by creating a Lua source file.\n\n"
  s = s .. "You can find the available options in 'prefs.lua', 'shortcuts.lua', and 'mouse.lua'"
  s = s .. " (all on the path '" .. package.path .. "').\n\n"
  s = s .. "Normally, you would not modify those files directly.\n\nInstead, create a new file '"
  if config.platform == "win" then
    s = s .. config.ipeletDirs[#config.ipeletDirs] .. "\\" .. "customization.lua"
  else
    s = s .. "~/.ipe/ipelets/customization.lua"
  end
  s = s .. "', place your changes in this file, and restart Ipe.\n\n"
  s = s .. "For further details, please read the manual."
  d:add("text", "text", { read_only=true }, 1, 1)
  d:set("text", s)
  d:addButton("ok", "Ok", "accept")
  d:execute(prefs.latexlog_size)
end

function MODEL:action_keyboard()
  local s = prefs.keyboard
  if s then
    if config.platform == "win" then
      ipeui.startBrowser(s)
    else
      os.execute(s)
    end
  else
    self:warning("No onscreen keyboard defined.",
		 "Edit preferences to define an onscreen keyboard.")
  end
end

function MODEL:action_developer_reload_ipelets()
  load_ipelets()
  self.ui:explain("Ipelets reloaded")
end

function MODEL:action_developer_list_shortcuts()
  local keys = {}
  for n,k in pairs(shortcuts) do
    if type(k) == "table" then
      for _,k1 in ipairs(k) do
	keys[#keys+1] = k1
      end
    else
      keys[#keys+1] = k
    end
  end
  table.sort(keys)
  printTable(keys)
  local dupl = 0
  local prev = nil
  for _, k in ipairs(keys) do
    if k == prev then
      print("Duplicate shortcut assignment: ", k)
      dupl = dupl + 1
    end
    prev = k
  end
  print(dupl, " duplicate shortcuts found")
end

function MODEL:action_cloud_latex()
  local d = ipeui.Dialog(win, "Ipe: Latex in the cloud")
  local urlFile = config.latexdir .. "url1.txt"
  if ipe.fileExists(urlFile) then
    local f = ipe.openFile(urlFile, "r")
    local url = f:read("a")
    url = url:match("^(%S+)%s*$")
    f:close()
    d:add("label1", "label",
	  { label="You are currently using the following online Latex-compilation service:" }, 1, 1, 1, 3)
    d:add("url", "label", { label=url }, 2, 2)
    d:add("label2", "label",
	  { label="Please keep in mind that the contents of all your text objects "
	    .. "is transmitted to this service at every Latex compilation." }, 3, 1, 1, 3)
    d:add("label3", "label",
	  { label = "You may not want to use online compilation for confidential information." }, 4, 1, 1, 3)
    d:addButton("ok", "Disable online compilation", "accept")
    d:addButton("cancel", "&Cancel", "reject")
    if d:execute() then
      os.remove(urlFile)
    end
  else
    d:add("label1", "label",
	  { label="Would you like to use the following online Latex-compilation service?" }, 1, 1, 1, 3)
    d:add("url", "label", { label=prefs.latex_service_url }, 2, 2)
    d:add("label2", "label",
	  { label="Please keep in mind that the contents of all your text objects "
	    .. "is transmitted to this service at every Latex compilation." }, 3, 1, 1, 3)
    d:add("label3", "label",
	  { label = "You may not want to use online compilation for confidential information." }, 4, 1, 1, 3)
    d:addButton("ok", "Enable online compilation", "accept")
    d:addButton("cancel", "&Cancel", "reject")
    if d:execute() then
      local f = ipe.openFile(urlFile, "w")
      f:write(prefs.latex_service_url, "\n")
      f:close()
    end
  end
end

----------------------------------------------------------------------

function MODEL:setOrigin(snapping)
  self.snap.origin = self.ui:simpleSnapPos()
  self.snap.with_axes = true
  self.ui:setActionState("show_axes", self.snap.with_axes)
  if snapping then
    self.snap.snapangle = true
    self.ui:setActionState("snapangle", true)
  end
  self:setSnap()
  self.ui:setFifiVisible(true)
  self.ui:update()
end

function MODEL:action_set_origin()
  self:setOrigin(false)
end

function MODEL:action_set_origin_snap()
  self:setOrigin(true)
end

function MODEL:action_show_axes()
  self.snap.with_axes = self.ui:actionState("show_axes");
  if not self.snap.with_axes then
    self.snap.snapangle = false
    self.ui:setActionState("snapangle", false)
  end
  self:setSnap()
  self.ui:update()
end

function MODEL:action_set_direction()
  self.snap.with_axes = true;
  self.ui:setActionState("show_axes", self.snap.with_axes)
  self.snap.orientation = (self.ui:simpleSnapPos() - self.snap.origin):angle()
  self.ui:setSnap(self.snap)
  self.ui:update()
end

local function find_arc_under_point_in_object(obj, m, pos)
  if obj:type() == "group" then
    local m1 = m * obj:matrix()
    for i = 1,obj:count() do
      local t = obj:elementType(i)
      if t == "path" or t == "group" then
	local obj1 = obj:element(i)
	local r = ipe.Rect()
	obj1:addToBBox(r, m1)
	if r:contains(pos) then
	  local arc = find_arc_under_point_in_object(obj1, m1, pos)
	  if arc then return arc end
	end
      end
    end
  elseif obj:type() == "path" then
    local m1 = m * obj:matrix()
    local shape = obj:shape()
    for _, sp in ipairs(shape) do
      if sp.type == "ellipse" then
	local m2 = m1 * sp[1]
	local post = m2:inverse() * pos
	if math.abs(post:sqLen() - 1.0) < 1e-3 then
	  return m2
	end
      elseif sp.type == "curve" then
	for i = 1,#sp do
	  if sp[i].type == "arc" then
	    local m2 = m1 * sp[i].arc:matrix()
	    local post = m2:inverse() * pos
	    if math.abs(post:sqLen() - 1.0) < 1e-3 then
	      return m2
	    end
	  end
	end
      end
    end
  end
end

local function find_arc_under_point(p, pos)
  local m = ipe.Matrix()
  for i, obj, sel, layer in p:objects() do
    if p:bbox(i):contains(pos) then
      local arc = find_arc_under_point_in_object(obj, m, pos)
      if arc then return arc end
    end
  end
end

function MODEL:action_set_tangent_direction()
  local p = self:page()
  local pos, dir = p:findEdge(self.vno, self.ui:unsnappedPos())
  if not pos then
    self.ui:explain("Mouse is not on an edge")
    return false
  end
  local arc = find_arc_under_point(p, pos)
  if not arc then
    self.ui:explain("Mouse is not on a circular arc")
    return false
  end
  local p = arc:inverse() * self.snap.origin
  local d = p:len()
  if d < 1.0 then
    self.ui:explain("Origin is inside the ellipse")
    return false
  end
  local r = math.sqrt(d * d - 1.0)
  local c = ipe.Arc(ipe.Matrix(r, 0, 0, r, p.x, p.y))
  local qs = c:intersect(ipe.Arc(ipe.Matrix()))
  if #qs ~= 2 then return false end
  local pos1 = arc:inverse() * pos
  local q = qs[1]
  if (pos1 - qs[2]):sqLen() < (pos1 - q):sqLen() then q = qs[2] end
  self.snap.with_axes = true;
  self.ui:setActionState("show_axes", self.snap.with_axes)
  self.snap.orientation = (arc * q - self.snap.origin):angle()
  self.ui:setSnap(self.snap)
  self.ui:update()
  return true
end

function MODEL:action_reset_direction()
  self.snap.orientation = 0
  self:setSnap()
  self.ui:update()
end

function MODEL:setLine(snapping)
  local origin, dir = self:page():findEdge(self.vno, self.ui:unsnappedPos())
  if not origin then
    self.ui:explain("Mouse is not on an edge")
  else
    self.snap.with_axes = true;
    self.ui:setActionState("show_axes", self.snap.with_axes)
    self.snap.orientation = dir
    self.snap.origin = origin
    self.ui:update()
    if snapping then
      self.snap.snapangle = true
      self.ui:setFifiVisible(true)
      self.ui:setActionState("snapangle", true)
    end
    self:setSnap()
  end
end

function MODEL:action_set_line()
  self:setLine(false)
end

function MODEL:action_set_line_snap()
  self:setLine(true)
end

----------------------------------------------------------------------

function MODEL:action_select_all()
  local p = self:page()
  for i,obj,sel,layer in p:objects() do
    if not sel and p:visible(self.vno, i) and not p:isLocked(layer) then
      self:page():setSelect(i, 2)
    end
  end
  p:ensurePrimarySelection()
  self.ui:update(false)
end

function MODEL:action_deselect_all()
  self:page():deselectAll()
  self.ui:update(false)
end

function MODEL:action_select_in_active_layer()
  local p = self:page()
  local active = p:active(self.vno)
  p:deselectAll()
  for i,obj,sel,layer in p:objects() do
    if layer == active then self:page():setSelect(i, 2) end
  end
  p:ensurePrimarySelection()
  self.ui:update(false)
end

function MODEL:action_select_in_layer(lay)
  local p = self:page()
  p:deselectAll()
  for i,obj,sel,layer in p:objects() do
    if layer == lay then self:page():setSelect(i, 2) end
  end
  p:ensurePrimarySelection()
  self.ui:update(false)
end

function MODEL:saction_move_to_active_layer()
  local p = self:page()
  local active = p:active(self.vno)
  local t = { label="move to layer " .. active,
	      pno = self.pno,
	      vno = self.vno,
	      selection=self:selection(),
	      original=p:clone(),
	      active=active,
	      undo=revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do p:setLayerOf(i, t.active) end
	   end
  self:register(t)
end

function MODEL:action_move_to_layer(lay)
  local p = self:page()
  if not p:hasSelection() then
    self.ui:explain("no selection")
    return
  end
  local t = { label="move to layer " .. lay,
	      pno = self.pno,
	      vno = self.vno,
	      selection=self:selection(),
	      original=p:clone(),
	      layer=lay,
	      undo=revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do p:setLayerOf(i, t.layer) end
	   end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:action_grid_visible()
  self.snap.grid_visible = self.ui:actionState("grid_visible");
  self:setSnap()
  self.ui:update()
end

function MODEL:action_pretty_display()
  self.ui:setPretty(self.ui:actionState("pretty_display"));
  self.ui:update()
end

function MODEL:action_zoom_in()
  local nzoom = prefs.zoom_factor * self.ui:zoom()
  if nzoom > prefs.max_zoom then nzoom = prefs.max_zoom end
  self.ui:setZoom(nzoom)
  self.ui:update()
end

function MODEL:action_zoom_out()
  local nzoom = self.ui:zoom() / prefs.zoom_factor
  if nzoom < prefs.min_zoom then nzoom = prefs.min_zoom end
  self.ui:setZoom(nzoom)
  self.ui:update()
end

-- kind is usually 0 except:
-- kind == 1 on OSX when using a trackpad,
-- and kind == 2 when holding control
function MODEL:wheel_zoom(xdelta, ydelta, kind)
  -- print("wheel_zoom", xdelta, ydelta, kind)
  local zoom = self.ui:zoom()
  local opts = prefs.scroll
  if kind == 1 then opts = prefs.trackpad_scroll end
  if kind ~= 2 and opts.pan then
    local p = self.ui:pan()
    local ppd = opts.pixel_per_degree / zoom
    p = p + V(opts.direction.x * xdelta * ppd, opts.direction.y * ydelta * ppd)
    self.ui:setPan(p)
  else
    local origin = self.ui:unsnappedPos()
    local offset = zoom * (self.ui:pan() - origin)
    local base = opts.zoom_per_degree
    local nzoom = zoom * base ^ ydelta
    if nzoom > prefs.max_zoom then nzoom = prefs.max_zoom end
    if nzoom < prefs.min_zoom then nzoom = prefs.min_zoom end
    self.ui:setZoom(nzoom)
    self.ui:setPan(origin + (1/nzoom) * offset)
  end
  self.ui:update()
end

-- Change resolution to 72 dpi and maximize interesting visible area.
-- As suggested by Rene:

-- 1) scale to the proper size, with the center of the canvas as the
--    origin of the scaling.

-- 2) If there is a horizontal and/or vertical translation that makes
--    a larger part of the *bounding box* of the objects visible, then
--    translate (and maximize the part of the bounding box that is
--    visible).

-- 3) If there is a horizontal and/or vertical translation that makes
--    a larger part of the paper visible, then translate (and maximize
--    the part of the paper that is visible), under the restriction
--    that no part of the bounding box of the objects may be moved
--    `out of sight' in this step. (Note that there may be objects
--    outside the paper).

local function adjustPan(cmin, cmax, omin, omax, pmin, pmax)
  local dx = 0;

  -- if objects stick out on both sides, there is nothing we can do
  if omin <= cmin and omax >= cmax then return dx end

  if omax > cmax and omin > cmin then
    -- we can see more objects if we shift canvas right
    dx = math.min(omin - cmin, omax - cmax)
  elseif omin < cmin and omax < cmax then
    -- we can see more objects if we shift canvas left
    dx = -math.min(cmin - omin, cmax - omax)
  end

  -- shift canvas
  cmin = cmin + dx
  cmax = cmax + dx

  -- if canvas fully contained in media, done
  if pmin <= cmin and pmax >= cmax then return dx end

  -- if media contained in canvas, can't improve
  if cmin < pmin and pmax < cmax then return dx end

  if pmin > cmin then
    -- improvement possible by shifting canvas right
    if omin > cmin then
      dx = dx + math.min(omin - cmin, pmin - cmin, pmax - cmax)
    end
  else
    -- improvement possible by shifting canvas left
    if omax < cmax then
      dx = dx - math.min(cmax - omax, cmax - pmax, cmin - pmin)
    end
  end
  return dx
end

function MODEL:action_normal_size()
  local p = self:page()
  self.ui:setZoom(prefs.normal_resolution / 72.0)
  local layout = self.doc:sheets():find("layout")
  local paper = ipe.Rect()
  paper:add(-layout.origin)
  paper:add(-layout.origin + layout.papersize)
  local bbox = ipe.Rect()
  for i = 1,#p do bbox:add(p:bbox(i)) end
  local canvas = ipe.Rect()
  canvas:add(self.ui:pan() - 0.5 * self.ui:canvasSize())
  canvas:add(self.ui:pan() + 0.5 * self.ui:canvasSize())
  local pan = V(adjustPan(canvas:left(), canvas:right(),
			  bbox:left(), bbox:right(),
			  paper:left(), paper:right()),
		adjustPan(canvas:bottom(), canvas:top(),
			  bbox:bottom(), bbox:top(),
			  paper:bottom(), paper:top()))
  self.ui:setPan(self.ui:pan() + pan)
  self.ui:update()
end

function MODEL:action_fit_page()
  local layout = self.doc:sheets():find("layout")
  local box = ipe.Rect()
  box:add(-layout.origin - V(2,2))
  box:add(-layout.origin + layout.papersize + V(2,2))
  self:fitBox(box);
end

function MODEL:action_fit_width()
  local layout = self.doc:sheets():find("layout")
  local box = ipe.Rect()
  local y0 = self.ui:pan().y
  box:add(V(-layout.origin.x - 2, y0 - 2))
  box:add(V(-layout.origin.x + layout.papersize.x + 2, y0 + 2))
  self:fitBox(box);
end

function MODEL:action_fit_top()
  self:action_fit_width()  -- sets zoom and x-pan correctly
  local layout = self.doc:sheets():find("layout")
  local x0 = self.ui:pan().x
  local ht = 0.5 * self.ui:canvasSize().y / self.ui:zoom()
  local y0 = -layout.origin.y + layout.papersize.y + 2 - ht
  self.ui:setPan(V(x0, y0))
end

function MODEL:action_fit_objects()
  local box = ipe.Rect()
  local m = ipe.Matrix()
  local p = self:page()
  for i,obj,_,layer in p:objects() do
    if p:visible(self.vno, i) then obj:addToBBox(box, m, false) end
  end
  self:fitBox(box);
end

function MODEL:saction_fit_selection()
  local box = ipe.Rect()
  local m = ipe.Matrix()
  local p = self:page()
  for i,obj,sel,_ in p:objects() do
    if sel then obj:addToBBox(box, m, false) end
  end
  self:fitBox(box);
end

function MODEL:action_pan_here()
  v = self.ui:unsnappedPos()
  self.ui:setPan(v)
  self.ui:update()
end

----------------------------------------------------------------------

function MODEL:action_next_view()
  self:nextView(1)
  self:setPage()
end

function MODEL:action_previous_view()
  self:nextView(-1)
  self:setPage()
end

function MODEL:action_first_view()
  self.vno = 1
  self:setPage()
end

function MODEL:action_last_view()
  self.vno = self:page():countViews()
  self:setPage()
end

function MODEL:action_new_view()
  local t = { label="new view",
	      pno = self.pno,
	      vno0 = self.vno,
	      vno1 = self.vno + 1,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:removeView(t.vno1)
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     p:insertView(t.vno1, p:active(t.vno0))
	     for i,layer in ipairs(p:layers()) do
	       p:setVisible(t.vno1, layer, p:visible(t.vno0, layer))
	     end
	   end
  self:register(t)
  self:nextView(1)
  self:setPage()
end

function MODEL:action_new_layer_view()
  local t = { label="new layer and view",
	      pno = self.pno,
	      vno0 = self.vno,
	      vno1 = self.vno + 1,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:removeLayer(t.layer)
	     doc[t.pno]:removeView(t.vno1)
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     p:insertView(t.vno1, p:active(t.vno0))
	     for i,layer in ipairs(p:layers()) do
	       p:setVisible(t.vno1, layer, p:visible(t.vno0, layer))
	     end
	     t.layer = doc[t.pno]:addLayer()
	     p:setVisible(t.vno1, t.layer, true)
	     p:setActive(t.vno1, t.layer)
	   end
  self:register(t)
  self:nextView(1)
  self:setPage()
end

function MODEL:action_delete_view()
  local p = self:page()
  if p:countViews() < 2 then
    self.ui:explain("cannot delete single view of a page")
    return
  end
  local t = { label="delete view",
	      pno = self.pno,
	      vno0 = self.vno,
	      vno1 = self.vno,
	      original = p:clone(),
	      undo = revertOriginal,
	    }
  if self.vno == p:countViews() then t.vno1 = self.vno - 1 end
  t.redo = function (t, doc)
	     doc[t.pno]:removeView(t.vno0)
	   end
  t.redo(t, self.doc)
  self.vno = t.vno1
  self:registerOnly(t)
  self.ui:explain(t.label)
end

function attribute_map(map)
  local s = ""
  for i,mapping in ipairs(map) do
    s = s .. mapping.kind .. ":" .. mapping.map .. "=" .. mapping.to .. ";\n"
  end
  return s
end

function parse_attribute_map(s)
  local kinds = { "pen", "symbolsize", "arrowsize", "opacity", "color", "dashstyle", "symbol" }
  local map = {}
  for kind, from, to in string.gmatch(s, "(%l+):([^=]+)=([^;]+);") do
    if indexOf(kind, kinds) then  -- silently ignore unknown kinds
      map[#map+1] = { kind=kind, map=from, to=to }
    end
  end
  return map
end

function layer_matrices(tfm)
  local s = ""
  for l, m in pairs(tfm) do
    s = s .. l .. "=" .. tostring(m) .. ";\n"
  end
  return s
end

function parse_layer_matrices(s)
  local map = {}
  for l, ms in string.gmatch(s, "([^%s=]+)=%s*%[([%d%s.e-]+)%]%s*;") do
    local m = {}
    for k in string.gmatch(ms, "%S+") do
      m[#m+1] = tonumber(k)
    end
    if #m == 6 then
      map[l] = ipe.Matrix(m)
    end
  end
  return map
end

function MODEL:action_edit_view()
  local p = self:page()
  local effect0 = p:effect(self.vno)
  local name0 = p:viewName(self.vno)
  local map0 = p:viewMap(self.vno)
  local tfm0 = p:layerMatrices(self.vno)
  local list = self.doc:sheets():allNames("effect")
  table.insert(list, 1, "normal")
  local d = ipeui.Dialog(self.ui:win(), "Edit view")
  local label = "View " .. self.vno .. " of " .. p:countViews()
  d:add("label", "label", { label=label }, 1, 1, 2, 1)
  d:add("name-label", "label", { label="Name" }, 0, 1)
  d:add("name", "input", {}, -1, 2)
  d:add("map-label", "label", { label="Attribute map" }, 0, 1)
  d:add("map", "text", {}, -1, 2)
  d:add("tfm-label", "label", { label="Transformations" }, 0, 1)
  d:add("tfm", "text", {}, -1, 2)
  d:add("effect-label", "label", { label="Effect" }, 0, 1)
  d:add("combo", "combo", list, -1, 2)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:set("name", name0)
  d:set("map", attribute_map(map0))
  d:set("tfm", layer_matrices(tfm0))
  d:set("combo", indexOf(effect0, list))
  if not d:execute() then return end
  local name1 = d:get("name")
  local effect1 = list[d:get("combo")]
  local map1 = parse_attribute_map(d:get("map"))
  local tfm1 = parse_layer_matrices(d:get("tfm"))
  local t = { label="modify view " .. self.vno,
	      pno = self.pno,
	      vno = self.vno,
	      name0 = name0,
	      effect0 = effect0,
	      map0 = map0,
	      tfm0 = tfm0,
	      name1 = name1,
	      effect1 = effect1,
	      map1 = map1,
	      tfm1 = tfm1,
	    }
  t.undo = function (t, doc)
    local p = doc[t.pno]
    p:setEffect(t.vno, t.effect0)
    p:setViewName(t.vno, t.name0)
    p:setViewMap(t.vno, t.map0)
    p:setLayerMatrices(t.vno, t.tfm0)
  end
  t.redo = function (t, doc)
    local p = doc[t.pno]
    p:setEffect(t.vno, t.effect1)
    p:setViewName(t.vno, t.name1)
    p:setViewMap(t.vno, t.map1)
    p:setLayerMatrices(t.vno, t.tfm1)
  end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:action_next_page()
  self:nextPage(1)
  self:setPage()
end

function MODEL:action_previous_page()
  self:nextPage(-1)
  self:setPage()
end

function MODEL:action_first_page()
  self.pno = 1
  self.vno = 1
  self:setPage()
end

function MODEL:action_last_page()
  self.pno = #self.doc
  self.vno = 1
  self:setPage()
end

function MODEL:action_copy_page()
  local data = self:page():xml("ipepage")
  self.ui:setClipboard(data)
  self.ui:explain("page copied to clipboard")
end

function MODEL:action_cut_page()
  if #self.doc == 1 then
    self:warning("Cannot cut page", "This is the only page of the document.")
    return
  end
  local t = { label="cut page " .. self.pno,
	      pno0 = self.pno,
	      pno1 = self.pno,
	      vno = 1,
	      original = self:page():clone(),
	    }
  if self.pno == #self.doc then t.pno1 = self.pno - 1 end
  t.undo = function (t, doc)
	     doc:insert(t.pno0, t.original)
	   end
  t.redo = function (t, doc)
	     doc:remove(t.pno0)
	   end
  t.redo(t, self.doc)
  self.pno = t.pno1
  self.vno = 1
  self:registerOnly(t)
  self.ui:explain(t.label .. " and copied to clipboard")
  local data = t.original:xml("ipepage")
  self.ui:setClipboard(data)
end

function MODEL:action_new_page()
  local t = { label="new page",
	      pno0 = self.pno,
	      pno1 = self.pno + 1,
	      vno0 = self.vno,
	      vno1 = 1,
	    }
  t.undo = function (t, doc)
	     doc:remove(t.pno1)
	   end
  t.redo = function (t, doc)
	     doc:insert(t.pno1, ipe.Page())
	   end
  self:register(t)
  self:nextPage(1)
  self:setPage()
end

function MODEL:action_paste_page()
  local data = self.ui:clipboard()
  if data:sub(1,9) ~= "<ipepage>" then
    self:warning("No Ipe page to paste")
    return
  end
  local p = ipe.Page(data)
  if not p then
    self:warning("Could not parse Ipe page on clipboard")
    return
  end
  local t = { label="paste page",
	      pno0 = self.pno,
	      pno1 = self.pno + 1,
	      vno0 = self.vno,
	      vno1 = 1,
	      page = p,
	    }
  t.undo = function (t, doc)
	     doc:remove(t.pno1)
	   end
  t.redo = function (t, doc)
	     doc:insert(t.pno1, t.page)
	   end
  self:register(t)
  self:nextPage(1)
  self:setPage()
  self:autoRunLatex()
end

function MODEL:action_delete_page()
  if #self.doc == 1 then
    self:warning("Cannot delete page",
		 "This is the only page of the document.")
    return
  end
  local t = { label="delete page " .. self.pno,
	      pno0 = self.pno,
	      pno1 = self.pno,
	      vno = 1,
	      original = self:page():clone(),
	    }
  if self.pno == #self.doc then t.pno1 = self.pno - 1 end
  t.undo = function (t, doc)
	     doc:insert(t.pno0, t.original)
	   end
  t.redo = function (t, doc)
	     doc:remove(t.pno0)
	   end
  t.redo(t, self.doc)
  self.pno = t.pno1
  self.vno = 1
  self:registerOnly(t)
  self.ui:explain(t.label)
end

local function update_dialog(d, sec)
  local on = d:get("t" .. sec)
  d:setEnabled(sec, not on)
  if on then d:set(sec, "") end
end

function MODEL:action_edit_title()
  local d = ipeui.Dialog(self.ui:win(), "Ipe: Edit page title and sections", self.doc:properties().language)
  d:add("label1", "label", { label="Page title"}, 1, 1, 1, 4)
  d:add("title", "text", {}, 0, 1, 1, 4)
  d:add("label2", "label", { label="Sections"}, 0, 1, 1, 4)
  d:add("label3", "label", { label="Section:" }, 0, 1)
  d:add("tsection", "checkbox",
	{ label="Use &title",
	  action=function (d) update_dialog(d, "section") end
	} , -1, 2)
  d:add("section", "input", {}, 0, 2, 1, 3)
  d:add("label4", "label", { label="Subsection:" }, 0, 1)
  d:add("tsubsection", "checkbox",
	{ label="Use titl&e",
	  action=function (d) update_dialog(d, "subsection") end
	} , -1, 2)
  d:add("subsection", "input", {}, 0, 2, 1, 3)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 2, 1)
  -- setup original values
  local ti = self:page():titles()
  d:set("title", ti.title)
  if ti.section then
    d:set("section", ti.section)
  else
    d:set("tsection", true)
    d:setEnabled("section", false)
  end
  if ti.subsection then
    d:set("subsection", ti.subsection)
  else
    d:set("tsubsection", true)
    d:setEnabled("subsection", false)
  end
  if not d:execute() then return end
  local final = { title = d:get("title") }
  if not d:get("tsection") then
    final.section = d:get("section")
  end
  if not d:get("tsubsection") then
    final.subsection = d:get("subsection")
  end
  if prefs.automatic_use_title and ti.title ~= final.title then
    final.section = true
    final.tsection = true
  end
  local t = { label="change title and sections of page " .. self.pno,
	      pno = self.pno,
	      vno = self.vno,
	      original = ti,
	      final = final,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setTitles(t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setTitles(t.final)
	   end
  self:register(t)
  self:autoRunLatex()
end

function MODEL:action_edit_notes()
  local d = ipeui.Dialog(self.ui:win(), "Ipe: Edit page notes", self.doc:properties().language)
  d:add("label1", "label", { label="Page notes"}, 1, 1)
  d:add("notes", "text", {}, 0, 1)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 2, 1)
  d:setStretch("column", 1, 1)
  -- setup original values
  local n = self:page():notes()
  d:set("notes", n)
  if not d:execute() then return end
  local final = d:get("notes")
  if string.match(final, "^%s*$") then
    final = ""
  end
  local t = { label="change notes on page " .. self.pno,
	      pno = self.pno,
	      vno = self.vno,
	      original = n,
	      final = final,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setNotes(t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setNotes(t.final)
	   end
  self:register(t)
end

----------------------------------------------------------------------

-- rearrange pages of document
-- new order is given by arr
-- 0 entries in arr are taken from newpages
-- returns list of pages that remained unused
function arrange_pages(doc, arr, newpages)
  -- take all pages from document
  local pg = {}
  while #doc > 0 do
    pg[#pg+1] = doc:remove(1)
  end
  -- insert pages again in new order
  for i = 1,#arr do
    local no = arr[i]
    if no > 0 then
      doc:append(pg[no])  -- clones the page
      pg[no] = nil        -- mark as used
    else
      doc:append(table.remove(newpages, 1))
    end
  end
  -- return unused pages
  local unused = {}
  for i = 1,#pg do
    if pg[i] then unused[#unused+1] = pg[i] end
  end
  return unused
end

function mark_pages(doc, marks)
  local originalMarks = {}
  for i = 1,#doc do
    local p = doc[i]
    originalMarks[i] = p:marked()
    p:setMarked(marks[i])
  end
  return originalMarks
end

function MODEL:action_page_sorter()
  local arr, marks = self.ui:pageSorter(self.doc)
  if not arr then return end -- canceled
  if #arr == 0 then
    self:warning("You cannot delete all pages of the document")
    return
  end
  -- printTable(arr)
  -- printTable(marks)
  -- compute reverse 'permutation'
  local rev = {}
  for i = 1,#self.doc do rev[i] = 0 end
  for i = 1,#arr do rev[arr[i]] = i end
  local t = { label="rearrangement of pages",
	      pno = 1,
	      vno = 1,
	      arr = arr,
	      rev = rev,
	      marks = marks,
	      originalMarks = marks
	    }
  t.undo = function (t, doc)
	     arrange_pages(doc, t.rev, t.deleted)
	     mark_pages(doc, t.originalMarks)
	   end
  t.redo = function (t, doc)
	     t.originalMarks = mark_pages(doc, t.marks)
	     t.deleted = arrange_pages(doc, t.arr, {})
	   end
  self.pno = 1
  self.vno = 1
  self:register(t)
end

----------------------------------------------------------------------

-- rearrange views of a page
-- new order is given by arr
function arrange_views(p, arr, marks)
  -- collect view information from the page
  local views = {}
  local layers = p:layers()
  for i = 1,p:countViews() do
    local visible = {}
    for _,l in ipairs(layers) do
      visible[l] = p:visible(i, l)
    end
    local v = {
      name=p:viewName(i),
      marked=marks[i],
      active=p:active(i),
      effect=p:effect(i),
      visible=visible,
    }
    views[#views+1] = v
  end
  p:clearViews()
  -- insert views in new order
  for i = 1,#arr do
    local v = views[arr[i]]
    p:insertView(i, v.active)
    p:setEffect(i, v.effect)
    p:setMarkedView(i, v.marked)
    p:setViewName(i, v.name)
    for _,l in ipairs(layers) do
      p:setVisible(i, l, v.visible[l])
    end
  end
end

function MODEL:action_view_sorter()
  local arr, marks = self.ui:pageSorter(self.doc, self.pno)
  if not arr then return end -- canceled
  if #arr == 0 then
    self:warning("You cannot delete all views of the page")
    return
  end
  -- printTable(arr)
  -- printTable(marks)
  local t = { label="rearrangement of views",
	      pno=self.pno,
	      vno=1,
	      arr=arr,
	      marks=marks,
	      original=self:page():clone(),
	      undo=revertOriginal,
	    }
  t.redo = function (t, doc)
	     arrange_views(doc[t.pno], t.arr, t.marks)
	   end
  self.vno = 1
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:action_toggle_notes()
  self.ui:showTool("notes", self.ui:actionState("toggle_notes"))
  self.ui:setNotes(self:page():notes())
end

function MODEL:action_toggle_bookmarks()
  self.ui:showTool("bookmarks", self.ui:actionState("toggle_bookmarks"))
  self:setBookmarks()
end

----------------------------------------------------------------------

function MODEL:action_new_layer()
  local t = { label="new layer",
	      pno = self.pno,
	      vno = self.vno,
	    }
  t.redo = function (t, doc)
	     t.layer = doc[t.pno]:addLayer()
	   end
  t.undo = function (t, doc)
	     doc[t.pno]:removeLayer(t.layer)
	   end
  self:register(t)
end

function MODEL:action_rename_active_layer()
  local p = self:page()
  local active = p:active(self.vno)
  self:layeraction_rename(active)
end

----------------------------------------------------------------------

function MODEL:saction_delete()
  local t = { label="delete",
	      pno = self.pno,
	      vno = self.vno,
	      selection=self:selection(),
	      original=self:page():clone(),
	      undo=revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = #t.selection,1,-1 do
	       p:remove(t.selection[i])
	     end
	   end
  self:register(t)
end

function MODEL:saction_edit_as_xml()
  local prim = self:page():primarySelection()
  local xml = self:page()[prim]:xml()
  local d = ipeui.Dialog(self.ui:win(), "Edit as XML")
  d:add("xml", "text", { syntax="xml", focus=true }, 1, 1)
  addEditorField(d, "xml")
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("row", 1, 1);
  d:set("xml", xml)
  if prefs.auto_external_editor then
    externalEditor(d, "xml")
  end
  if ((prefs.auto_external_editor and prefs.editor_closes_dialog)
    or d:execute(prefs.editor_size)) then
    local obj = ipe.Object(d:get("xml"))
    if not obj then self:warning("Cannot parse XML") return end

    local t = { label="edit object in XML",
		pno = self.pno,
		vno = self.vno,
		primary = prim,
		original = self:page()[prim]:clone(),
		final = obj,
	      }
    t.undo = function (t, doc)
	       doc[t.pno]:replace(t.primary, t.original)
	     end
    t.redo = function (t, doc)
	       doc[t.pno]:replace(t.primary, t.final)
	     end
    self:register(t)
    if obj:type() == "text" then self:autoRunLatex() end
  end
end
----------------------------------------------------------------------

function MODEL:saction_group()
  local p = self:page()
  local selection = self:selection()
  local elements = {}
  for _,i in ipairs(selection) do
    elements[#elements + 1] = p[i]:clone()
  end
  local final = ipe.Group(elements)
  p:deselectAll()
  local t = { label="group",
	      pno = self.pno,
	      vno = self.vno,
	      original = p:clone(),
	      selection = selection,
	      layer = p:active(self.vno),
	      final = final,
	      undo = revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = #t.selection,1,-1 do p:remove(t.selection[i]) end
	     p:insert(nil, t.final, 1, t.layer)
	   end
  self:register(t)
end

-- does not pass pinning and allowed transformation to elements
function MODEL:saction_ungroup()
  local p = self:page()
  local prim = p:primarySelection()
  if p[prim]:type() ~= "group" then
    self.ui:explain("primary selection is not a group")
    return
  end
  p:deselectAll()
  local t = { label="ungroup",
	      pno = self.pno,
	      vno = self.vno,
	      original = p[prim]:clone(),
	      primary = prim,
	      originalLayer = p:layerOf(prim),
	      layer = p:active(self.vno),
	      elements = p[prim]:elements(),
	      matrix = p[prim]:matrix(),
	    }
  t.undo = function (t, doc)
	     local p = doc[t.pno]
	     for i = 1,#t.elements do p:remove(#p) end
	     p:insert(t.primary, t.original, nil, t.originalLayer)
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     p:remove(t.primary)
	     for _,obj in ipairs(t.elements) do
	       p:insert(nil, obj, 2, t.layer)
	       p:transform(#p, t.matrix)
	     end
	     p:ensurePrimarySelection()
	   end
  self:register(t)
end

function MODEL:saction_front()
  local p = self:page()
  local selection = self:selection()
  local t = { label="front",
	      pno = self.pno,
	      vno = self.vno,
	      original = p:clone(),
	      selection = selection,
	      primary = indexOf(p:primarySelection(), selection),
	      undo = revertOriginal,
	    }
  p:deselectAll()
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     local r,l = extractElements(p, t.selection)
	     for i,obj in ipairs(r) do
	       p:insert(nil, obj, (i == t.primary) and 1 or 2, l[i])
	     end
	   end
  self:register(t)
end

function MODEL:saction_back()
  local p = self:page()
  local selection = self:selection()
  local t = { label="back",
	      pno = self.pno,
	      vno = self.vno,
	      original = p:clone(),
	      selection = selection,
	      primary = indexOf(p:primarySelection(), selection),
	      undo = revertOriginal,
	    }
  p:deselectAll()
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     local r,l = extractElements(p, t.selection)
	     for i,obj in ipairs(r) do
	       p:insert(i, obj, (i == t.primary) and 1 or 2, l[i])
	     end
	   end
  self:register(t)
end

function MODEL:saction_duplicate()
  local p = self:page()
  local t = { label="duplicate",
	      pno = self.pno,
	      vno = self.vno,
	      selection = self:selection(),
	      primary = self:page():primarySelection(),
	      layer = self:page():active(self.vno),
	    }
  t.undo = function (t, doc)
	     local p = doc[t.pno]
	     for i = 1,#t.selection do p:remove(#p) end
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for _,i in ipairs(t.selection) do
	       p:insert(nil, p[i]:clone(), 2, t.layer)
	       p:setSelect(i, nil)
	       if i == t.primary then p:setSelect(#p, 1) end
	     end
	   end
  self:register(t)
end

local function moveTargetToSource (t, doc)
  local p = doc[t.pno]
  local target = (t.target > t.source) and (t.target - 1) or t.target
  local obj = p[target]:clone()
  local layer = p:layerOf(target)
  p:remove(target)
  p:insert(t.source, obj, nil, layer)
end

local function moveSourceToTarget (t, doc)
  local p = doc[t.pno]
  local obj = p[t.source]:clone()
  local layer = p:layerOf(t.source)
  p:insert(t.target, obj, t.select, layer)
  p:remove((t.target < t.source) and (t.source + 1) or  t.source)
end

-- Move selection one step forward
-- In other words:
-- take object in front of nearest selection and
-- place it behind furthest selection
function MODEL:saction_forward()
  local p = self:page()
  local selection = self:selection()
  local furthest = nil
  local nearest = nil
  for i,obj,sel,layer in p:objects() do
    if sel then
      nearest = i
      if not furthest then furthest = i end
    end
  end
  if nearest == #p then self.ui:explain("already at front") return end
  local t = { label="move selection forward",
	      pno = self.pno,
	      vno = self.vno,
	      source = nearest + 1,
	      target = furthest,
	      select = nil,
	      undo = moveTargetToSource,
	      redo = moveSourceToTarget,
	    }
  self:register(t)
end

-- Move selection one step back
-- In other words:
-- take object behind furthest selection and
-- place it in front of nearest selection
function MODEL:saction_backward()
  local p = self:page()
  local selection = self:selection()
  local furthest = nil
  local nearest = nil
  for i,obj,sel,layer in p:objects() do
    if sel then
      nearest = i
      if not furthest then furthest = i end
    end
  end
  if furthest == 1 then self.ui:explain("already at back") return end
  local t = { label="move selection backward",
	      pno = self.pno,
	      vno = self.vno,
	      source = furthest - 1,
	      target = nearest + 1,
	      select = nil,
	      undo = moveTargetToSource,
	      redo = moveSourceToTarget,
	    }
  self:register(t)
end

-- Moves the primary selection just before the highest secondary.
function MODEL:saction_before()
  local p = self:page()
  local prim = p:primarySelection()
  local sec = nil
  for i,obj,sel,layer in p:objects() do
    if sel == 2 then sec = i end
  end
  if not sec then
    self.ui:explain("no secondary selection")
    return
  end
  if prim == sec + 1 then self.ui:explain("nothing to do") return end
  local t = { label="move just before secondary selection",
	      pno = self.pno,
	      vno = self.vno,
	      source = prim,
	      target = sec + 1,
	      select = 1,
	      undo = moveTargetToSource,
	      redo = moveSourceToTarget,
	    }
  self:register(t)
end

-- Moves the primary selection just behind the lowest secondary.
function MODEL:saction_behind()
  local p = self:page()
  local prim = p:primarySelection()
  local sec = nil
  for i,obj,sel,layer in p:objects() do
    if sel == 2 then sec = i break end
  end
  if not sec then
    self.ui:explain("no secondary selection")
    return
  end
  if prim == sec - 1 then self.ui:explain("nothing to do") return end
  local t = { label="move just behind secondary selection",
	      pno = self.pno,
	      vno = self.vno,
	      source = prim,
	      target = sec,
	      select = 1,
	      undo = moveTargetToSource,
	      redo = moveSourceToTarget,
	    }
  self:register(t)
end

function MODEL:selectionWithCursor()
  local data = self:page():xml("ipeselection")
  assert(data:sub(1,13) == "<ipeselection")
  local pos = self.ui:pos()
  return (string.format('<ipeselection pos="%g %g"', pos.x, pos.y)
	.. data:sub(14))
end

function MODEL:action_copy()
  local data = self:selectionWithCursor()
  self.ui:setClipboard(data)
  self.ui:explain("selection copied to clipboard")
end

function MODEL:action_cut()
  local data = self:selectionWithCursor()
  self.ui:setClipboard(data)
  local t = { label="cut selection",
	      pno = self.pno,
	      vno = self.vno,
	      selection=self:selection(),
	      original=self:page():clone(),
	      undo=revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i = #t.selection,1,-1 do
	       p:remove(t.selection[i])
	     end
	   end
  self:register(t)
end

function MODEL:action_paste()
  local data = self.ui:clipboard(true)  -- allow a bitmap
  if not data then
    self:warning("Nothing to paste")
    return
  end
  local elements
  if type(data) == "string" then
    if data:sub(1,13) ~= "<ipeselection" then
      self:warning("No Ipe selection to paste")
      return
    end
    elements = ipe.Object(data)
    if not elements then
      self:warning("Could not parse Ipe selection on clipboard")
      return
    end
  else
    elements = { data }
  end
  local p = self:page()
  local t = { label="paste objects",
	      pno = self.pno,
	      vno = self.vno,
	      elements = elements,
	      layer = p:active(self.vno),
	    }
  t.undo = function (t, doc)
	     local p = doc[t.pno]
	     for i = 1,#t.elements do p:remove(#p) end
	   end
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i,obj in ipairs(t.elements) do
	       p:insert(nil, obj, 2, t.layer)
	     end
	     p:ensurePrimarySelection()
	   end
  p:deselectAll()
  self:register(t)
  self:autoRunLatex()
end

function MODEL:action_paste_with_layer()
  local data = self.ui:clipboard(false)
  if not data then
    self:warning("Nothing to paste")
    return
  end
  if type(data) ~= "string" or data:sub(1,13) ~= "<ipeselection" then
    self:warning("No Ipe selection to paste")
    return
  end
  local elements, layers = ipe.Object(data)
  if not elements then
    self:warning("Could not parse Ipe selection on clipboard")
    return
  end
  local p = self:page()
  local active = p:active(self.vno)
  local pLayers = p:layers()
  local impossible = {}
  for i,l in ipairs(layers) do
    if l == "" or indexOf(l, pLayers) == nil or p:isLocked(l) then
      if indexOf(l, impossible) == nil then
	impossible[#impossible+1] = l
      end
      layers[i] = active  -- place in active layer instead
    end
  end
  if #impossible > 0 then
    local impossibleText = ""
    for _,l in ipairs(impossible) do
      impossibleText = impossibleText .. l .. ", "
    end
    self:warning("Impossible to paste to specified layer",
		 "Some of the layers you are trying to paste to either do " ..
		   "not exist on this page, or are locked.\n\n" ..
		   "I have used the active layer instead.\n\n" ..
		   "The following layer names could not be used:\n\n" .. impossibleText)
  end
  local t = { label="paste objects with layer",
	      pno = self.pno,
	      vno = self.vno,
	      elements = elements,
	      layers = layers,
	      original = p:clone(),
	      undo = revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i,obj in ipairs(t.elements) do
	       p:insert(nil, obj, 2, t.layers[i])
	     end
	     p:ensurePrimarySelection()
	   end
  p:deselectAll()
  self:register(t)
  self:autoRunLatex()
end

----------------------------------------------------------------------

function MODEL:action_check_style()
  local syms = self.doc:checkStyle()
  if #syms == 0 then
    self.ui:explain("no undefined symbols")
  else
    self:warning("Undefined symbolic attributes:",
		 "\n - " .. table.concat(syms, "\n - ") .. "\n")
  end
end

function MODEL:action_update_style_sheets()
  local dir = nil
  if self.file_name then dir = string.gsub(self.file_name, "[^/\\]+$", "") end
  if dir == "" then dir = "." end
  local sheets = self.doc:sheets():clone()
  local qlog = ""
  for index=1,sheets:count() do
    local sheet = sheets:sheet(index)
    local name = sheet:name()
    if not name then
      qlog = qlog .. " - unnamed stylesheet\n"
    elseif name == "standard" then
      qlog = qlog .. " - standard stylesheet\n"
    else
      qlog = qlog .. " - stylesheet '" .. name .. "'\n"
      local s = findStyle(name .. ".isy", dir)
      if s then
	qlog = qlog .. "     updating from '" .. s .."'\n"
	local nsheet = ipe.Sheet(s)
	if not nsheet then
	  qlog = qlog .. "    ! failed to load '" .. s .. "'!\n"
	else
	  sheets:insert(index, nsheet)
	  sheets:remove(index + 1) -- remove old sheet
	end
      end
    end
  end

  messageBox(self.ui:win(), "information",
		   "Result of updating stylesheets:", qlog)

  local t = { label="update style sheets",
	      style_sheets_changed = true,
	      final = sheets,
	      original = self.doc:sheets():clone(),
	    }
  t.undo = function (t, doc)
	     t.final = doc:replaceSheets(t.original)
	   end
  t.redo = function (t, doc)
	     t.original = doc:replaceSheets(t.final)
	   end
  self:register(t)
  self:action_check_style()
  self:autoRunLatex()
end

local function sheets_namelist(list)
  local r = {}
  for i,s in ipairs(list) do
    r[i] = list[i]:name()
    if not r[i] then r[i] = "<unnamed>" end
  end
  return r
end

local function sheets_add(d, dd)
  local i = d:get("list")
  if not i then i = 1 end
  local s, f = ipeui.fileDialog(dd.model.ui:win(), "open", "Add stylesheet",
				filter_stylesheets)
  if not s then return end
  local sheet, msg = ipe.Sheet(s)
  if not sheet then
    dd.model:warning("Cannot load stylesheet", msg)
    return
  end
  table.insert(dd.list, i, sheet)
  d:set("list", sheets_namelist(dd.list))
  d:set("list", i)
  dd.modified = true
end

local function sheets_edit(d, dd)
  if not prefs.external_editor then
    dd.model:warning("Cannot edit stylesheet",
		     "No external editor defined")
    return
  end
  local i = d:get("list")
  if not i or dd.list[i]:isStandard() then return end
  local data = dd.list[i]:xml(true)
  local fname = os.tmpname()
  if prefs.editable_textfile then
    fname = prefs.editable_textfile
  end
  local f = io.open(fname, "w")
  f:write(data)
  f:close()
  local sheet, msg
  while not sheet do
    ipeui.waitDialog(d, string.format(prefs.external_editor, fname))
    sheet, msg = ipe.Sheet(fname)
    if not sheet then
      local r = messageBox(dd.model.ui:win(), "question",
			   "Cannot load stylesheet - try again?",
			   msg, "okcancel")
      if r <= 0 then
	os.remove(fname)
	return
      end
    end
  end
  dd.list[i] = sheet
  d:set("list", sheets_namelist(dd.list))
  d:set("list", i)
  dd.modified = true
  os.remove(fname)
end

local function sheets_del(d, dd)
  local i = d:get("list")
  if not i then return end
  if dd.list[i]:isStandard() then
    dd.model:warning("Cannot delete stylesheet",
		     "The standard stylesheet cannot be removed")
    return
  end
  table.remove(dd.list, i)
  d:set("list", sheets_namelist(dd.list))
  dd.modified = true
end

local function sheets_up(d, dd)
  local i = d:get("list")
  if not i or i == 1 then return end
  local s = dd.list[i-1]
  dd.list[i-1] = dd.list[i]
  dd.list[i] = s
  d:set("list", sheets_namelist(dd.list))
  d:set("list", i-1)
  dd.modified = true
end

local function sheets_down(d, dd)
  local i = d:get("list")
  if not i or i == #dd.list then return end
  local s = dd.list[i+1]
  dd.list[i+1] = dd.list[i]
  dd.list[i] = s
  d:set("list", sheets_namelist(dd.list))
  d:set("list", i+1)
  dd.modified = true
end

local function sheets_save(d, dd)
  local i = d:get("list")
  if not i then return end
  local s, f = ipeui.fileDialog(dd.model.ui:win(), "save", "Save stylesheet",
				filter_stylesheets)
  if s then
    if s:sub(-4) ~= ".isy" then s = s .. ".isy" end
    local data = dd.list[i]:xml(true)
    local f, msg = ipe.openFile(s, "wb")
    if not f then
      dd.model:warning("Cannot save stylesheet", msg)
      return
    end
    f:write('<?xml version="1.0"?>\n')
    f:write('<!DOCTYPE ipestyle SYSTEM "ipe.dtd">\n')
    f:write(data)
    f:close()
    dd.model.ui:explain("Stylesheet saved to " .. s)
  end
end

function MODEL:action_style_sheets()
  local sheets = self.doc:sheets()
  local dd = { list = {},
	       model = self,
	       modified = false
	     }
  for i = 1,sheets:count() do
    dd.list[i] = sheets:sheet(i):clone()
  end
  local d = ipeui.Dialog(self.ui:win(), "Ipe style sheets")
  d:add("label1", "label", { label="Style sheets"}, 1, 1, 1, 4)
  d:add("list", "list", sheets_namelist(dd.list), 2, 1, 7, 3)
  d:add("add", "button",
	{ label="&Add", action=function (d) sheets_add(d, dd) end }, 2, 4)
  d:add("del", "button",
	{ label="Del", action=function (d) sheets_del(d, dd) end }, 3, 4)
  d:add("edit", "button",
	{ label="Edit", action=function (d) sheets_edit(d, dd) end }, 4, 4)
  d:add("up", "button",
	{ label="&Up", action=function (d) sheets_up(d, dd) end }, 5, 4)
  d:add("down", "button",
	{ label="&Down", action=function (d) sheets_down(d, dd) end }, 6, 4)
  d:add("save", "button",
	{ label="&Save", action=function (d) sheets_save(d, dd) end }, 7, 4)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("column", 2, 1)
  if not d:execute() or not dd.modified then return end
  local t = { label="modify style sheets",
	      final = ipe.Sheets(),
	      style_sheets_changed = true,
	    }
  for i,s in ipairs(dd.list) do
    t.final:insert(i, s)
  end
  t.undo = function (t, doc)
	     t.final = doc:replaceSheets(t.original)
	   end
  t.redo = function (t, doc)
	     t.original = doc:replaceSheets(t.final)
	   end
  self:register(t)
  self:action_check_style()
end

----------------------------------------------------------------------

function MODEL:action_document_properties()
  local engines = { "default", "pdftex", "xetex", "luatex" }
  local p = self.doc:properties()
  local d = ipeui.Dialog(self.ui:win(), "Ipe document properties", self.doc:properties().language)
  d:add("label1", "label", { label="Title"}, 1, 1)
  d:add("title", "input", {}, -1, 2, 1, 6)
  d:add("label2", "label", { label="Author"}, 0, 1)
  d:add("author", "input", {}, -1, 2, 1, 6)
  d:add("label3", "label", { label="Subject"}, 0, 1)
  d:add("subject", "input", {}, -1, 2, 1, 6)
  d:add("label4", "label", { label="Keywords"}, 0, 1)
  d:add("keywords", "input", {}, -1, 2, 1, 6)
  d:add("label4b", "label", { label="Language"}, 0, 1)
  d:add("language", "input", {}, -1, 2, 1, 6)
  d:add("label5", "label", { label="Latex engine"}, 0, 1)
  d:add("tex", "combo", engines, -1, 2, 1, 6)
  d:add("label6", "label", { label="Latex preamble"}, 0, 1)
  d:add("preamble", "text", {}, -1, 2, 2, 6)
  d:add("label7", "label", { label="Page mode"}, 9, 1)
  d:add("fullscreen", "checkbox", { label="&Full screen"}, -1, 2)
  d:add("numberpages", "checkbox", { label="&Number pages"}, -1, 3)
  d:add("sequentialtext", "checkbox", { label="&Sequential text"}, -1, 4)
  d:add("label8", "label", { label="Created"}, 0, 1)
  d:add("created", "label",  { label="" }, -1, 2)
  d:add("label9", "label", { label="Modified"}, 0, 1)
  d:add("modified", "label",  { label="" }, -1, 2)
  d:add("label10", "label", { label="Creator" }, 0, 1)
  d:add("creator", "label",  { label="" }, -1, 2)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  d:setStretch("column", 5, 1)
  d:setStretch("row", 8, 1)
  addEditorField(d, "preamble")
  for n in pairs(p) do d:set(n, p[n]) end
  if not d:execute() then return end

  local t = { label="modify document properties",
	      original = p,
	      final = {},
	    }
  for n in pairs(p) do
    if n == "creator" or n == "created" or n == "modified" then
      t.final[n] = p[n]
    elseif n == "tex" then
      t.final[n] = engines[d:get(n)]
    else
      t.final[n] = d:get(n)
    end
  end
  t.undo = function (t, doc) doc:setProperties(t.original) end
  t.redo = function (t, doc) doc:setProperties(t.final) end
  self:register(t)
  self:autoRunLatex()
end

----------------------------------------------------------------------

function MODEL:saction_custom()
  local p = self:page()
  local prim = p:primarySelection()
  local obj = p[prim]
  local custom = self:getString("Enter custom value", nil, obj:getCustom())
  if not custom then return end
  local t = { label="set custom",
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=obj:getCustom(),
	      custom=custom,
	    }
  t.undo = function (t, doc)
             doc[t.pno][t.primary]:setCustom(t.original)
	   end
  t.redo = function (t, doc)
             doc[t.pno][t.primary]:setCustom(t.custom)
           end
  self:register(t)
end

----------------------------------------------------------------------

function MODEL:saction_set_link_action()
  local p = self:page()
  local prim = p:primarySelection()
  local obj = p[prim]
  assert(obj:type() == "group")
  local link_action = self:getString("Enter link action", nil, obj:text())
  if not link_action then return end
  if link_action:match("^%s*$)") then link_action = "" end
  local t = { label="set link action",
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=obj:clone(),
	      link_action=link_action,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno][t.primary]:setText(t.link_action)
	   end
  self:register(t)
end

function MODEL:saction_remove_clipping()
  local p = self:page()
  local prim = p:primarySelection()
  local obj = p[prim]
  assert(obj:type() == "group")
  local t = { label="remove clipping",
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=obj:clone(),
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno][t.primary]:setClip()
	     doc[t.pno]:invalidateBBox(t.primary)
	   end
  self:register(t)
end

function MODEL:saction_extract_clipping()
  local p = self:page()
  local prim = p:primarySelection()
  local obj = p[prim]
  assert(obj:type() == "group")
  local shape = obj:clip()
  transformShape(obj:matrix(), shape)
  local obj = ipe.Path(self.attributes, shape)
  self:creation("extract clipping path", obj)
end

function MODEL:saction_add_clipping()
  local p = self:page()
  local sel = self:selection()
  if #sel ~= 2 then
    self:warning("Must have exactly one two selected objects")
    return
  end
  local prim = sel[1]
  local sec = sel[2]
  if p[prim]:type() ~= "group" then
    prim = sel[2]
    sec = sel[1]
  end
  local obj = p[prim]
  if obj:type() ~= "group" or p[sec]:type() ~= "path" then
    self:warning("Must select exactly one group and one path")
    return
  end

  local shape = p[sec]:shape()
  transformShape(p[sec]:matrix(), shape)
  transformShape(obj:matrix():inverse(), shape)

  local t = { label = "add clipping",
	      pno = self.pno,
	      vno = self.vno,
	      primary = prim,
	      secondary = sec,
	      original = obj:clone(),
	      path = p[sec]:clone(),
	      shape = shape,
	      layer = p:layerOf(sec)
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:insert(t.secondary, t.path, nil, t.layer)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno][t.primary]:setClip(t.shape)
	     doc[t.pno]:invalidateBBox(t.primary)
	     doc[t.pno]:remove(t.secondary)
	   end
  self:register(t)
end

----------------------------------------------------------------------
