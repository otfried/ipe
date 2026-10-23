----------------------------------------------------------------------
-- Document variants
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

label = "Document variants"

category = "Styles"

about = [[
Functions to work with document variants and multi-language documents.
]]

function localizeText(model)
  local d = ipeui.Dialog(model.ui:win(), "Localize document")
  local variantL = model.doc:sheets():allNames("variant")
  if #variantL == 0 then
    model:warning("No variants known", "You need to add variants to the document first")
    return
  end
  local s = [[
I will change every text object in the entire document that is not math mode
and that does not already have a variant set to the following variant:
]]
  d:add("label1", "label", {label=s}, 0, 1)
  d:add("variant", "combo", variantL, 0, 1)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  if not d:execute() then return end
  local variant = variantL[d:get("variant")]

  local changes = {}
  for pno, page in model.doc:pages() do
    for i, obj, sel, layer in page:objects() do
      if obj:type() == "text" and obj:get("textstyle") ~= "math" and
      obj:get("variant") == "undefined" then
	changes[#changes+1] = {
	  pno=pno,
	  index=i,
	}
      end
    end
  end

  if #changes == 0 then
    model:warning("No changes found", "No text objects match the criterions")
    return
  end
  local t = { label = "Localize document",
	      changes = changes,
	      variant = variant,
	      undo = function (t, doc)
		for _, change in ipairs(t.changes) do
		  doc[change.pno][change.index]:set("variant", "undefined")
		end
	      end,
	      redo = function (t, doc)
		for _, change in ipairs(t.changes) do
		  doc[change.pno][change.index]:set("variant", t.variant)
		end
	      end,
  }
  model:register(t)
end

function addLanguage(model)
  local d = ipeui.Dialog(model.ui:win(), "Add language")
  local variantL = model.doc:sheets():allNames("variant")
  if #variantL == 0 then
    model:warning("No variants known", "You need to add variants to the document first")
    return
  end
  local s = [[
I will create a copy of object in the entire document that is set to the source language
and set the copy to the target language.
]]
  d:add("label1", "label", {label=s}, 0, 1, 1, 2)
  d:add("label2", "label", {label="Source"}, 0, 1)
  d:add("source", "combo", variantL, -1, 2)
  d:add("label3", "label", {label="Target"}, 0, 1)
  d:add("target", "combo", variantL, -1, 2)
  d:addButton("ok", "&Ok", "accept")
  d:addButton("cancel", "&Cancel", "reject")
  if not d:execute() then return end
  local source = variantL[d:get("source")]
  local target = variantL[d:get("target")]
  if source == target then
    model:warning("Add language", "Source and target languages cannot be identical")
    return
  end

  local changes = {}
  for pno, page in model.doc:pages() do
    for i, obj, sel, layer in page:objects() do
      if obj:get("variant") == source then
	changes[#changes+1] = {
	  pno=pno,
	  index=i,
	}
      end
    end
  end

  if #changes == 0 then
    model:warning("No changes found", "There is no object in the source language")
    return
  end
  local t = { label = "Add language",
	      changes = changes,
	      source = source,
	      target = target,
	      undo = function (t, doc)
		for _, change in ipairs(t.changes) do
		  print("Revert: ", change.pno)
		  local p = doc[change.pno]
		  p:remove(#p)
		end
	      end,
	      redo = function (t, doc)
		for _, change in ipairs(t.changes) do
		  local p = doc[change.pno]
		  local obj = p[change.index]:clone()
		  obj:set("variant", t.target)
		  p:insert(nil, obj, nil, p:layerOf(change.index))
		end
	      end,
  }
  model:register(t)
end

methods = {
  { label = "Convert text to localized", run=localizeText },
  { label = "Add language", run=addLanguage },
}
