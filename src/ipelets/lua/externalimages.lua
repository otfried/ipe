----------------------------------------------------------------------
-- External images
----------------------------------------------------------------------
--[[

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

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

label = "External images"

category = "Images"

about = [[
    Helpers for managing external vs. internal images.

    Document copies include images in nested groups, but leave stylesheet
    symbols unchanged. External copies store all page images in the new
    document's .images folder, using timestamp names with numeric suffixes
    when needed.
]]

local function refresh(model, num)
  model.ui:setImageFolder("")
  model.ui:setImageFolder(model:imageFolder())
end

local function realPath(model, path)
  if path:sub(1,1) == "/" or path:sub(1,2) == "./" then
    return path
  else
    return model:imageFolder() .. path
  end
end

local function internal_image(model, obj)
  local rpath = realPath(model, obj:info().path)
  local img, res = ipe.readImage(rpath)
  if not img then
    model:warning("Could not read image", res)
    return
  end
  local copy = obj:clone()
  copy:setBitmap(img)
  return copy
end

local function image_internal(model, num)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local obj = p[prim]
  if obj:type() ~= "image" or not obj:info().path then
    model.ui:explain("object is not an external image")
    return
  end
  local img = internal_image(model, obj)
  if not img then return end
  local t = { label="make image internal",
	      pno=model.pno,
	      primary=prim,
	      original=obj:clone(),
	      final=img,
	    }
  t.undo = function (t, doc)
    doc[t.pno]:replace(t.primary, t.original)
  end
  t.redo = function (t, doc)
    doc[t.pno]:replace(t.primary, t.final)
  end
  model:register(t)
end

local function image_external(model, num)
  local p = model:page()
  local prim = p:primarySelection()
  if not prim then model.ui:explain("no selection") return end
  local obj = p[prim]
  if obj:type() ~= "image" or not obj:info().width then
    model.ui:explain("object is not an internal image")
    return
  end
  if not model.file_name then
    self:warning("Cannot create external image",
		 "Document does not yet have a file name, so we have no image folder")
    return
  end
  local extension = ".png"
  if obj:info().format == "jpg" then
    extension = ".jpg"
  end
  local path = model:getString("Enter image path", nil,
			       ipeui.currentDateTime() .. extension)
  if not path then return end
  local rpath = realPath(model, path)
  if ipe.fileExists(rpath) then
    self:warning("A file with this name already exists", rpath)
    return nil
  end
  if not obj:saveImage(rpath) then
    model:warning("Could not save image", rpath)
    return
  end
  local img = obj:clone()
  img:setPath(path)
  local t = { label="make image external",
	      pno=model.pno,
	      primary=prim,
	      original=obj:clone(),
	      final=img,
	    }
  t.undo = function (t, doc)
    doc[t.pno]:replace(t.primary, t.original)
  end
  t.redo = function (t, doc)
    doc[t.pno]:replace(t.primary, t.final)
  end
  model:register(t)
end

local function convert_object(obj, convert_image)
  if obj:type() == "image" then
    local copy = convert_image(obj)
    return copy, copy ~= obj
  elseif obj:type() == "group" then
    local changed = false
    for i = 1, obj:count() do
      local element = obj:element(i)
      local copy, replaced = convert_object(element, convert_image)
      if not copy then return end
      if replaced then
	obj:replace(i, copy)
	changed = true
      end
    end
    return obj, changed
  end
  return obj
end

local function document_images(model, num)
  local external = (num == 4)
  local kind = external and "external" or "internal"
  local s, f =
    ipeui.fileDialog(model.ui:win(), "save", "Save with " .. kind .. " images as",
		    _G.filter_save)
  if not s then return end

  local fm = _G.formatFromFileName(s)
  if not fm then
    local extensions = { ".ipe", ".pdf" }
    s = s .. extensions[f]
    fm = _G.formatFromFileName(s)
  end
  if _G.config.toolkit ~= "cocoa" and ipe.fileExists(s) then
    local b = ipeui.messageBox(model.ui:win(), "question", "File already exists!",
			      "Do you wish to overwrite '" .. s .. "' ?", "okcancel")
    if b ~= 1 then return end
  end

  local folder = model:imageFolder()
  if external then
    local base = s
    if s:sub(-4) == ".ipe" or s:sub(-4) == ".pdf" then base = s:sub(1, -5) end
    folder = base .. ".images" .. "/"
    if not ipe.fileExists(folder) then
      local ok, errmsg = ipe.mkdir(folder)
      if not ok then
	model:warning("Could not create image folder", errmsg)
	return
      end
    end
  end
  local function convert_image(obj)
    if not external then
      if obj:info().path then return internal_image(model, obj) end
      return obj
    end
    local image = obj
    if obj:info().path then
      image = internal_image(model, obj)
      if not image then return end
    end
    local extension = image:info().format == "jpg" and ".jpg" or ".png"
    local base = ipeui.currentDateTime()
    local path = base .. extension
    local suffix = 0
    while ipe.fileExists(folder .. path) do
      suffix = suffix + 1
      path = base .. "-" .. suffix .. extension
    end
    if not image:saveImage(folder .. path) or not model:persistFile(folder .. path) then
      model:warning("Could not save image", folder .. path)
      return
    end
    local copy = obj:clone()
    copy:setPath(path)
    return copy
  end

  local doc = model.doc:clone()
  for _, page in doc:pages() do
    for i = 1, #page do
      local obj = page[i]
      local converted = convert_object(obj, convert_image)
      if not converted then return end
      if converted ~= obj then page:replace(i, converted) end
    end
  end
  if fm == "pdf" then
    local ok, errmsg = doc:runLatex(model.file_name)
    if not ok then
      model:warning("File not saved!", errmsg)
      return
    end
  end
  if not doc:save(s, fm, {}, folder) or not model:persistFile(s) then
    model:warning("File not saved!", "Error saving the document")
    return
  end
  model.ui:explain("Saved document '" .. s .. "'")
end

methods = {
  { label="Refresh external images", run = refresh },
  { label="Make image external", run = image_external },
  { label="Make image internal", run = image_internal },
  { label="Create document with external images", run = document_images },
  { label="Create document with internal images", run = document_images },
}

----------------------------------------------------------------------
