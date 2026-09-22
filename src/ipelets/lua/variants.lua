
label = "Document variants"

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
