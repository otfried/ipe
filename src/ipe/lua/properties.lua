----------------------------------------------------------------------
-- properties.lua
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

local function color_icon(sheets, name)
  local abs = sheets:find("color", name)
  if abs then
    return abs.r, abs.g, abs.b
  end
  return 0.0, 0.0, 0.0
end

function MODEL:propertiesPopup()
  self:updateCloseSelection()
  local sel = self:selection()
  if #sel > 1 then
    self:multiPopup(sel)
  elseif #sel == 0 then
    self:emptyPopup()
  else
    self:singlePopup(self:page():primarySelection())
  end
end

function MODEL:emptyPopup()
  local m = ipeui.Menu(self.ui:win())
  m:add("action_paste_at_cursor", "Paste at cursor")
  m:add("action_pan_here", "Pan canvas")
  m:add("grid", "Toggle grid")
  local gp = self.ui:globalPos()
  local item, num, value = m:execute(gp.x, gp.y)
  if item then
    if item:sub(1,7) == "action_" then
      self:action(item:sub(8))
    elseif item == "grid" then
      self:toggleGrid()
    end
  end
end

function MODEL:singlePopup(prim)
  local obj = self:page()[prim]
  local m = ipeui.Menu(self.ui:win())
  self["properties_" .. obj:type()](self, obj, m)
  m:add("action_custom", "Custom: " .. obj:getCustom())
  if prefs.tablet_menu then
    m:add("action_cut", "Cut")
    m:add("action_copy", "Copy")
    m:add("action_paste_at_cursor", "Paste at cursor")
    m:add("action_pan_here", "Pan canvas")
    m:add("grid", "Toggle grid")
  end
  local gp = self.ui:globalPos()
  local item, num, value = m:execute(gp.x, gp.y)
  if item then
    if item == "layer" then
      self:changeLayerOfPrimary(prim, value)
    elseif item:sub(1,7) == "action_" then
      self:action(item:sub(8))
    elseif item == "comment" then
      -- nothing to do
    elseif item == "grid" then
      self:toggleGrid()
    else
      self:setAttributeOfPrimary(prim, item, value)
    end
  end
end

--------------------------------------------------------------------

function MODEL:toggleGrid()
  local t = not self.ui:actionState("grid_visible");
  self.ui:setActionState("grid_visible", t)
  self:action_grid_visible()
end

function MODEL:changeLayerOfPrimary(prim, layer)
  local p = self:page()
  local t = { label="change layer to " .. layer,
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=p:layerOf(prim),
	      toLayer=layer,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:setLayerOf(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setLayerOf(t.primary, t.toLayer)
	   end
  self:register(t)
end

function MODEL:setAttributeOfPrimary(prim, prop, value)
  self:deselectSecondary()
  local p = self:page()
  local t = { label="set attribute " .. prop .. " to " .. tostring(value),
	      pno=self.pno,
	      vno=self.vno,
	      primary=prim,
	      original=p[prim]:clone(),
	      property=prop,
	      value=value,
	      stroke=self.attributes.stroke,
	      fill=self.attributes.fill,
	    }
  t.undo = function (t, doc)
	     doc[t.pno]:replace(t.primary, t.original)
	   end
  t.redo = function (t, doc)
	     doc[t.pno]:setAttribute(t.primary, t.property, t.value,
				     t.stroke, t.fill)
	   end
  self:register(t)
  if prop == "minipage" or prop == "textsize" or
    prop == "textstyle" or prop == "labelstyle" then
    self:autoRunLatex()
  end
  if p[prim]:type() == "text" and prop == "stroke" then
    self:autoRunLatex()
  end
end

----------------------------------------------------------------------

function MODEL:insertBasic(m, obj)
  local p = self:page()
  local layerL = p:layers()
  local pinnedL = { "none", "horizontal", "vertical", "fixed" }
  local transformationsL = { "translations", "rigid", "affine" }
  local layer = p:layerOf(p:primarySelection())
  local pinned = obj:get("pinned")
  local transformations = obj:get("transformations")
  m:add("layer", "Layer: " .. layer, layerL, nil, layer)
  m:add("pinned", "Pinned: " .. pinned, pinnedL, nil, pinned)
  m:add("transformations", "Transformations: " .. transformations,
	transformationsL, nil, transformations)
end

function MODEL:insertOpacity(m, obj)
  local opacityL = self.doc:sheets():allNames("opacity")
  local opacity = obj:get("opacity")
  m:add("opacity", "Opacity: " .. opacity, opacityL, nil, opacity)
  if obj:type() == "path" then
    local strokeOpacity = obj:get("strokeopacity")
    m:add("strokeopacity", "Stroke opacity: " .. strokeOpacity, opacityL, nil, strokeOpacity)
  end
end

----------------------------------------------------------------------

local function group_has_text(obj)
  for i = 1,obj:count() do
    if obj:elementType(i) == "text" then return true end
  end
  return false
end

local function decorationToName(i, s)
  if s == "normal" then
    return "no decoration"
  else
    return s:match("^decoration/(.+)$")
  end
end

local function decoration_names(model)
  local symbols = model.doc:sheets():allNames("symbol")
  local res = { "normal" }
  for _, name in ipairs(symbols) do
    if name:match("^decoration/") then
      res[#res + 1] = name
    end
  end
  return res
end

function MODEL:properties_group(obj, m)
  m:add("comment", "Group object")
  local s = string.format("%d elements", obj:count())
  local clip = obj:clip()
  local link_action = obj:text()
  if clip then
    s = s .. ", with clipping"
  end
  m:add("comment", s)
  if link_action == "" then
    m:add("action_set_link_action", "Set link action")
  else
    m:add("action_set_link_action", "Link action: " .. link_action)
  end
  m:add("decoration", "Decoration", decoration_names(self),
	decorationToName, obj:get("decoration"))
  self:insertBasic(m, obj)
  m:add("action_apply_properties", "Apply properties")
  if clip then
    m:add("action_remove_clipping", "Remove clipping")
    m:add("action_extract_clipping", "Extract clipping path")
  end
  if group_has_text(obj) then
    m:add("action_edit", "Edit text in group")
  end
  m:add("action_edit_group", "Edit group")
  m:add("action_edit_as_xml", "Edit as XML")
  m:add("action_ungroup", "Ungroup")
end

function MODEL:properties_path(obj, m)
  local sheet = self.doc:sheets()
  local colors = sheet:allNames("color")
  local pathmodes = { "stroke only", "stroke && fill", "fill only" }
  local pathmodeL = { "stroked", "strokedfilled", "filled" }
  local penL = sheet:allNames("pen")
  local dashstyleL = sheet:allNames("dashstyle")
  local arrowsizeL = sheet:allNames("arrowsize")
  local arrowshapeL = symbolNames(sheet, "arrow/", "(spx)")
  local tilingL = sheet:allNames("tiling")
  table.insert(tilingL, 1, "normal")
  local gradientL = sheet:allNames("gradient")
  table.insert(gradientL, 1, "normal")
  local linecapL = { "normal", "butt", "round", "square", }
  local linejoinL = { "normal", "miter", "round", "bevel", }
  local fillruleL = { "normal", "wind", "evenodd", }

  local shape = obj:shape()

  m:add("comment", "Path object")
  m:add("comment", string.format("%d subpaths", #shape))
  self:insertBasic(m, obj)

  local pm = obj:get("pathmode")
  m:add("pathmode", "Stroke && Fill: " .. pathmodes[indexOf(pm, pathmodeL)],
	pathmodeL, pathmodes, pm)
  if pm ~= "filled"  then
    m:add("stroke", "Stroke color: " .. colorString(obj:get("stroke")),
	  colors, nil,
	  function (i, name) return color_icon(self.doc:sheets(), name) end)
  end
  if pm ~= "stroked" then
    m:add("fill", "Fill color: " .. colorString(obj:get("fill")), colors, nil,
	  function (i, name) return color_icon(self.doc:sheets(), name) end)
  end

  local pen = obj:get("pen")
  m:add("pen", "Pen width: " .. pen, penL, nil, pen)
  local dashstyle = obj:get("dashstyle")
  m:add("dashstyle", "Dash style: " .. dashstyle, dashstyleL, nil, dashstyle)

  local farr = obj:get("farrow")
  local rarr = obj:get("rarrow")
  local boolnames = { "yes", "no" }
  local boolmap = { "true", "false" }
  local bool_to_yesno = { [false]="no", [true]="yes" }
  m:add("farrow", "Forward arrow: " .. bool_to_yesno[farr], boolmap,
	boolnames, tostring(farr))
  if farr then
    m:add("farrowsize", "Forward arrow size", arrowsizeL,
	  nil, obj:get("farrowsize"))
    m:add("farrowshape", "Forward arrow shape", arrowshapeL,
	  arrowshapeToName, obj:get("farrowshape"))
  end
  m:add("rarrow", "Reverse arrow: " .. bool_to_yesno[rarr], boolmap,
	boolnames, tostring(rarr))
  if rarr then
    m:add("rarrowsize", "Reverse arrow size", arrowsizeL,
	  nil, obj:get("rarrowsize"))
    m:add("rarrowshape", "Reverse arrow shape", arrowshapeL,
	  arrowshapeToName, obj:get("rarrowshape"))
  end

  local lc = obj:get("linecap")
  local lj = obj:get("linejoin")
  local fr = obj:get("fillrule")
  m:add("linecap", "Line cap: " .. lc, linecapL, nil, lc)
  m:add("linejoin", "Line join: " .. lj, linejoinL, nil, lj)
  m:add("fillrule", "Fill rule: " .. fr, fillruleL, nil, fr)

  if pm ~= "stroked" then
    local tiling = obj:get("tiling")
    local gradient = obj:get("gradient")
    m:add("tiling", "Tiling pattern: " .. tiling, tilingL, nil, tiling)
    m:add("gradient", "Gradient: " .. gradient, gradientL, nil, gradient)
  end
  self:insertOpacity(m, obj)

  if #shape > 1 then
    m:add("action_decompose", "Decompose path")
  end
  m:add("action_pick_properties", "Pick properties")
  m:add("action_apply_properties", "Apply properties")
  m:add("action_edit_as_xml", "Edit as XML")
  m:add("action_edit", "Edit path")
end

function MODEL:properties_text(obj, m)
  local sheet = self.doc:sheets()
  local minipage = obj:get("minipage")
  local colors = sheet:allNames("color")
  local textsizeL = sheet:allNames("textsize")
  local textstyleL = sheet:allNames("textstyle")
  local labelstyleL = sheet:allNames("labelstyle")
  m:add("comment", "Text object")
  self:insertBasic(m, obj)
  local t = minipage and "minipage" or "label"
  m:add("minipage", "Type: " .. t, {"true", "false"}, {"minipage", "label"},
	tostring(minipage))
  m:add("stroke", "Color: " .. colorString(obj:get("stroke")), colors,
	nil,
	function (i, name) return color_icon(self.doc:sheets(), name) end)
  local ts = obj:get("textsize")
  m:add("textsize", "Size: " .. ts, textsizeL, nil, ts)
  if minipage then
    local ts = obj:get("textstyle")
    m:add("textstyle", "Style: " .. ts, textstyleL, nil, ts)
  else
    local ts = obj:get("labelstyle")
    m:add("labelstyle", "Style: " .. ts, labelstyleL, nil, ts)
  end
  local ha = obj:get("horizontalalignment")
  m:add("horizontalalignment", "Horizontal alignment: " .. ha,
	{"left", "right", "hcenter"}, nil, ha)
  local va = obj:get("verticalalignment")
  m:add("verticalalignment", "Vertical alignment: " .. va,
	{"bottom", "baseline", "top", "vcenter"}, nil, va)

  self:insertOpacity(m, obj)

  if minipage then
    m:add("action_change_width", "Change width")
  end
  m:add("action_pick_properties", "Pick properties")
  m:add("action_apply_properties", "Apply properties")
  m:add("action_edit_as_xml", "Edit as XML")
  m:add("action_edit", "Edit text")
end

function MODEL:properties_image(obj, m)
  m:add("comment", "Image object")
  local info = obj:info()
  m:add("comment", string.format("%d x %d pixels",
				  info.width, info.height))
  m:add("comment", "Format " .. info.format)
  self:insertBasic(m, obj)
  self:insertOpacity(m, obj)
end

function MODEL:properties_reference(obj, m)
  local sheet = self.doc:sheets()
  local colors = sheet:allNames("color")
  local sizes = sheet:allNames("symbolsize")
  local pens = sheet:allNames("pen")
  m:add("comment", "Reference object")
  m:add("comment", "Symbol name: " .. obj:symbol())
  self:insertBasic(m, obj)
  m:add("stroke", "Stroke color: " .. colorString(obj:get("stroke")),
	colors, nil,
  	function (i, name) return color_icon(self.doc:sheets(), name) end)
  m:add("fill", "Fill color: " .. colorString(obj:get("fill")), colors, nil,
	function (i, name) return color_icon(self.doc:sheets(), name) end)
  local ss = obj:get("symbolsize")
  m:add("symbolsize", "Size: " .. ss, sizes, nil, ss)
  local pen = obj:get("pen")
  m:add("pen", "Pen: " .. pen, pens, nil, pen)
  m:add("action_pick_properties", "Pick properties")
  m:add("action_apply_properties", "Apply properties")
  m:add("action_edit_as_xml", "Edit as XML")
end

----------------------------------------------------------------------

local object_name = { text = "Text", path = "Path", image = "Image",
		       reference = "Reference", group = "Group", }

function MODEL:multiAttributes(m, tm)
  local layers = self:page():layers()
  local pinned = { "none", "horizontal", "vertical", "fixed" }
  local transformations = { "translations", "rigid", "affine" }
  m:add("layer", "Layer", layers)
  m:add("pinned", "Pinned", pinned)
  m:add("transformations", "Transformations", transformations)

  local sheet = self.doc:sheets()
  local colors = sheet:allNames("color")
  local sizes = sheet:allNames("symbolsize")
  local pens = sheet:allNames("pen")
  local opacity = sheet:allNames("opacity")
  local pathmodes = { "stroke only", "stroke && fill", "fill only" }
  local pathmode = { "stroked", "strokedfilled", "filled" }
  local dashstyle = sheet:allNames("dashstyle")
  local arrowsizes = sheet:allNames("arrowsize")
  local arrowshapes = symbolNames(sheet, "arrow/", "(spx)")
  local tilings = sheet:allNames("tiling")
  table.insert(tilings, 1, "normal")
  local gradients = sheet:allNames("gradient")
  table.insert(gradients, 1, "normal")
  local linecap = { "normal", "butt", "round", "square", }
  local linejoin = { "normal", "miter", "round", "bevel", }
  local fillrule = { "normal", "wind", "evenodd", }
  local textsize = sheet:allNames("textsize")
  local textstyle = sheet:allNames("textstyle")

  if tm.path or tm.text or tm.reference then
    m:add("stroke", "Stroke color", colors, nil,
	  function (i, name) return color_icon(sheet, name) end)
  end
  if tm.path or tm.reference then
    m:add("fill", "Fill color", colors, nil,
	  function (i, name) return color_icon(sheet, name) end)
  end
  if tm.path or tm.reference then m:add("pen", "Pen width", pens) end

  if tm.path then
    m:add("pathmode", "Stroke && Fill", pathmode, pathmodes)
    m:add("dashstyle", "Dash style", dashstyle)
    local boolnames = { "yes", "no" }
    local boolmap = { "true", "false" }
    m:add("farrow", "Forward arrow", boolmap, boolnames)
    m:add("farrowsize", "Forward arrow size", arrowsizes)
    m:add("farrowshape", "Forward arrow shape", arrowshapes, arrowshapeToName)
    m:add("rarrow", "Reverse arrow", boolmap, boolnames)
    m:add("rarrowsize", "Reverse arrow size", arrowsizes)
    m:add("rarrowshape", "Reverse arrow shape", arrowshapes, arrowshapeToName)
    m:add("linecap", "Line cap", linecap)
    m:add("linejoin", "Line join", linejoin)
    m:add("fillrule", "Fill rule", fillrule)
    m:add("tiling", "Tiling pattern", tilings)
    m:add("gradient", "Gradient", gradients)
  end

  if tm.text then
    m:add("textsize", "Text size", textsize)
    m:add("textstyle", "Text style", textstyle)
    m:add("horizontalalignment", "Horizontal alignment",
	  {"left", "right", "hcenter"})
    m:add("verticalalignment", "Vertical alignment",
	  {"bottom", "baseline", "top", "vcenter"})
  end

  if tm.reference then m:add("symbolsize", "Size", sizes) end
  if tm.text or tm.path then m:add("opacity", "Opacity", opacity) end
  if tm.path then m:add("strokeopacity", "Stroke opacity", opacity) end
  m:add("action_apply_properties", "Apply properties")
end

function MODEL:multiPopup()
  -- collect types of selected objects
  local p = self:page()
  local typmap = {}
  local count = 0
  for i,obj,sel,layer in self:page():objects() do
    if sel then typmap[obj:type()] = true; count = count + 1 end
  end
  local typcount = 0
  local ttype = nil
  for t in pairs(typmap) do typcount = typcount + 1; ttype = t end

  local m = ipeui.Menu(self.ui:win())
  if typcount == 1 then
    m:add("comment", string.format("%d %s objects", count, object_name[ttype]))
  else
    m:add("comment", string.format("%d objects", count))
  end

  self:multiAttributes(m, typmap)

  if count == 2 and typcount == 2 and typmap["group"] and typmap["path"] then
    m:add("action_add_clipping", "Add clipping path")
  end

  if typcount == 1 and ttype == 'path' then
    m:add("action_join", "Join paths")
    m:add("action_compose", "Compose paths")
  end

  if prefs.tablet_menu then
    m:add("action_cut", "Cut")
    m:add("action_copy", "Copy")
    m:add("action_paste_at_cursor", "Paste at cursor")
    m:add("action_pan_here", "Pan canvas")
    m:add("grid", "Toggle grid")
  end
  local gp = self.ui:globalPos()
  local item, num, value = m:execute(gp.x, gp.y)
  if item then
    if item == "layer" then
      self:action_move_to_layer(value)
    elseif item:sub(1,7) == "action_" then
      self:action(item:sub(8))
    elseif item == "comment" then
      -- nothing to do
    elseif item == "grid" then
      self:toggleGrid()
    else
      self:setAttribute(item, value)
    end
  end
end

--------------------------------------------------------------------

function MODEL:saction_pick_properties()
  local p = self:page()
  local prim = p:primarySelection()
  local obj = p[prim]
  self["pick_properties_" .. obj:type()](self, obj)
  self.ui:setAttributes(self.doc:sheets(), self.attributes)
end

function MODEL:pick_properties_group(obj)
  -- nothing
end

function MODEL:pick_properties_text(obj)
  local a = self.attributes
  a.stroke = obj:get("stroke")
  a.textsize = obj:get("textsize")
  if obj:get("minipage") then
    a.textstyle = obj:get("textstyle")
  else
    a.horizontalalignment = obj:get("horizontalalignment")
  end
  a.verticalalignment = obj:get("verticalalignment")
  a.opacity = obj:get("opacity")
end

function MODEL:pick_properties_path(obj)
  local a = self.attributes
  a.pathmode = obj:get("pathmode")
  if a.pathmode ~= "filled" then
    a.stroke = obj:get("stroke")
  end
  if a.pathmode ~= "stroked" then
    a.fill = obj:get("fill")
  end
  a.pen = obj:get("pen")
  a.dashstyle = obj:get("dashstyle")
  a.farrow = obj:get("farrow")
  a.rarrow = obj:get("rarrow")
  if a.farrow then
    a.farrowsize = obj:get("farrowsize")
    a.farrowshape = obj:get("farrowshape")
  end
  if a.rarrow then
    a.rarrowsize = obj:get("rarrowsize")
    a.rarrowshape = obj:get("rarrowshape")
  end
  a.linecap = obj:get("linecap")
  a.linejoin = obj:get("linejoin")
  a.fillrule = obj:get("fillrule")
  a.tiling = obj:get("tiling")
  a.gradient = obj:get("gradient")
  a.opacity = obj:get("opacity")
  a.strokeopacity = obj:get("strokeopacity")
end

function MODEL:pick_properties_image(obj)
  -- nothing
end

function MODEL:pick_properties_reference(obj)
  local a = self.attributes
  a.stroke = obj:get("stroke")
  a.fill = obj:get("fill")
  a.pen = obj:get("pen")
  a.symbolsize = obj:get("symbolsize")
end

----------------------------------------------------------------------

function MODEL:saction_apply_properties()
  local a = { }
  for k, v in pairs(self.attributes) do
    a[k] = v
  end
  local t = { label="apply properties",
	      pno = self.pno,
	      vno = self.vno,
	      attributes=a,
	      selection=self:selection(),
	      original=self:page():clone(),
	      undo=revertOriginal,
	    }
  t.redo = function (t, doc)
	     local p = doc[t.pno]
	     for i,obj,sel,layer in p:objects() do
	       if sel then
		 local ty = obj:type()
		 if ty == "text" then
		   apply_properties_text(obj, t.attributes)
		 elseif ty == "path" then
		   apply_properties_path(obj, t.attributes)
		 elseif ty == "reference" then
		   apply_properties_reference(obj, t.attributes)
		 end
	       end
	     end
	   end
  self:register(t)
end

function apply_properties_text(obj, a)
  obj:set("stroke", a.stroke)
  obj:set("textsize", a.textsize)
  if obj:get("minipage") then
    obj:set("textstyle", a.textstyle)
  else
    obj:set("horizontalalignment", a.horizontalalignment)
  end
  obj:set("verticalalignment", a.verticalalignment)
  obj:set("opacity", a.opacity)
end

function apply_properties_path(obj, a)
  obj:set("pathmode", a.pathmode)
  if a.pathmode ~= "filled" then
    obj:set("stroke", a.stroke)
  end
  if a.pathmode ~= "stroked" then
    obj:set("fill", a.fill)
  end
  obj:set("pen", a.pen)
  obj:set("dashstyle", a.dashstyle)
  obj:set("farrow", a.farrow)
  obj:set("rarrow", a.rarrow)
  if a.farrow then
    obj:set("farrowsize", a.farrowsize)
    obj:set("farrowshape", a.farrowshape)
  end
  if a.rarrow then
    obj:set("rarrowsize", a.rarrowsize)
    obj:set("rarrowshape", a.rarrowshape)
  end
  obj:set("linecap", a.linecap)
  obj:set("linejoin", a.linejoin)
  obj:set("fillrule", a.fillrule)
  obj:set("tiling", a.tiling)
  obj:set("gradient", a.gradient)
  obj:set("opacity", a.opacity)
  obj:set("strokeopacity", a.strokeopacity)
end

function apply_properties_reference(obj, a)
  obj:set("stroke", a.stroke)
  obj:set("fill", a.fill)
  obj:set("pen", a.pen)
  obj:set("symbolsize", a.symbolsize)
end

----------------------------------------------------------------------
