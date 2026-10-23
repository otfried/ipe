----------------------------------------------------------------------
-- Fuzzy search and quick launcher for ipelets
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

label = "Search ipelets..."

about = [[
Search all ipelets by name, category, or description, and run the selected one.

Type to filter the list, use the Up and Down keys to move the selection,
and press Return or double-click to run the ipelet.

This ipelet is part of Ipe.
]]

-- name of this ipelet (the ipelet environment's field), not to be listed itself
local self_name = name

local function fuzzy_score(query, target)
  if query == "" then return 0 end
  local q = query:lower()
  local t = target:lower()

  -- 1. Exact full match
  if q == t then return 10000 end

  -- 2. Exact substring match
  local sub_start, _ = t:find(q, 1, true)
  if sub_start then
    local prefix_bonus = (sub_start == 1) and 1000 or 0
    return 5000 + prefix_bonus - sub_start * 20 - (#t - #q)
  end

  -- 3. Multi-word match: every word in query must appear in target
  local qwords = {}
  for w in q:gmatch("%S+") do qwords[#qwords + 1] = w end
  if #qwords > 1 then
    local all_words_match = true
    local word_score = 3000
    for _, qw in ipairs(qwords) do
      local s = t:find(qw, 1, true)
      if not s then
        all_words_match = false
        break
      end
      word_score = word_score - s * 10
    end
    if all_words_match then
      return word_score - (#t - #q)
    end
  end

  -- 4. Fuzzy subsequence match
  local qidx = 1
  local qlen = #q
  local tlen = #t
  local score = 1000
  local prev_idx = -2

  for tidx = 1, tlen do
    local tc = t:sub(tidx, tidx)
    local qc = q:sub(qidx, qidx)

    local is_boundary = (tidx == 1) or (t:sub(tidx - 1, tidx - 1):match("[%s%-_%:/%.%(%)]"))

    if tc == qc then
      qidx = qidx + 1
      if is_boundary then
        score = score + 40
      elseif tidx == prev_idx + 1 then
        score = score + 25
      else
        score = score + 5
      end
      prev_idx = tidx

      if qidx > qlen then
        return score - (tlen - qlen)
      end
    end
  end

  return nil
end

local function match_item(query, item)
  if query == "" then
    return true, 0
  end

  -- 1. Match title (highest priority)
  local s1 = fuzzy_score(query, item.title)
  if s1 then
    return true, s1 + 10000
  end

  -- 2. Match group/ipelet name
  local s2 = fuzzy_score(query, item.group)
  if s2 then
    return true, s2 + 8000
  end

  -- 3. Match category (e.g. CGAL Algorithms)
  if item.category and item.category ~= "" then
    local s3 = fuzzy_score(query, item.category)
    if s3 then
      return true, s3 + 6000
    end
  end

  -- 4. Match description / about text (substring or word match)
  if item.about and #item.about > 0 then
    local qa = query:lower()
    local ta = item.about:lower()
    local s = ta:find(qa, 1, true)
    if s then
      return true, 2000 - math.min(s, 500)
    end
    local qwords = {}
    for w in qa:gmatch("%S+") do qwords[#qwords + 1] = w end
    if #qwords > 1 then
      local all_match = true
      for _, qw in ipairs(qwords) do
        if not ta:find(qw, 1, true) then
          all_match = false
          break
        end
      end
      if all_match then
        return true, 1500
      end
    end
  end

  return false, 0
end

local function collect_commands()
  local items = {}
  local seen = {}
  for _, ip in ipairs(_G.ipelets) do
    -- like the Ipelet menu, skip ipelets without a label (e.g. customization.lua)
    if ip.label and ip.name ~= self_name then
      local group = ip.label or ip.name or "Ipelet"
      local base_about = ip.about or ""
      if ip.methods and #ip.methods > 0 then
        for j, m in ipairs(ip.methods) do
          local label = m.label or ("Method " .. j)
          local action = "ipelet_" .. j .. "_" .. ip.name
          if not seen[action] then
            seen[action] = true
            local shortcut = shortcuts[action]
            local about = m.about or base_about
            local title = group .. ": " .. label
            items[#items + 1] = {
              ipelet = ip,
              name = ip.name,
              category = ip.category,
              method = j,
              action = action,
              group = group,
              label = label,
              title = title,
              about = about,
              shortcut = shortcut,
            }
          end
        end
      else
        local label = ip.label or ip.name
        local action = "ipelet_1_" .. ip.name
        if not seen[action] then
          seen[action] = true
          local shortcut = shortcuts[action]
          local about = base_about
          local title = label
          items[#items + 1] = {
            ipelet = ip,
            name = ip.name,
            category = ip.category,
            method = 1,
            action = action,
            group = group,
            label = label,
            title = title,
            about = about,
            shortcut = shortcut,
          }
        end
      end
    end
  end

  table.sort(items, function (a, b)
    return a.title:lower() < b.title:lower()
  end)

  return items
end

local function format_display(item)
  if item.shortcut and item.shortcut ~= "" then
    return string.format("%s  [%s]", item.title, item.shortcut)
  else
    return item.title
  end
end

local function format_details(item)
  if not item then
    return "No matching ipelet found."
  end
  local s = "Ipelet: " .. item.group .. "\n"
  if item.category and item.category ~= "" then
    s = s .. "Category: " .. item.category .. "\n"
  end
  s = s .. "Command: " .. item.label .. "\n"
  if item.shortcut and item.shortcut ~= "" then
    s = s .. "Shortcut: " .. item.shortcut .. "\n"
  end
  s = s .. "\n"
  if item.about and item.about:gsub("%s+", "") ~= "" then
    s = s .. item.about:match("^%s*(.-)%s*$")
  else
    s = s .. "(No description provided by ipelet)"
  end
  return s
end

function run(model)
  local all_items = collect_commands()
  if #all_items == 0 then
    model.ui:explain("No ipelets are currently loaded")
    return
  end

  local current_matches = all_items

  local d = ipeui.Dialog(model.ui:win(), "Search Ipelets")

  local function update_list_and_details(dlg)
    local q = dlg:get("search") or ""
    q = q:match("^%s*(.-)%s*$") or ""

    local scored = {}
    for _, item in ipairs(all_items) do
      local matched, score = match_item(q, item)
      if matched then
        scored[#scored + 1] = { item = item, score = score }
      end
    end

    table.sort(scored, function (a, b)
      if a.score ~= b.score then
        return a.score > b.score
      end
      return a.item.title:lower() < b.item.title:lower()
    end)

    current_matches = {}
    local display_lines = {}
    for _, entry in ipairs(scored) do
      current_matches[#current_matches + 1] = entry.item
      display_lines[#display_lines + 1] = format_display(entry.item)
    end

    if #display_lines == 0 then
      dlg:set("list", { "(No matches)" })
      dlg:set("description", "No ipelet matches the query '" .. q .. "'.")
      dlg:setEnabled("ok", false)
    else
      dlg:set("list", display_lines)
      dlg:set("description", format_details(current_matches[1]))
      dlg:setEnabled("ok", true)
    end
  end

  local function on_selection_changed(dlg)
    local idx = dlg:get("list")
    if idx and current_matches[idx] then
      dlg:set("description", format_details(current_matches[idx]))
    end
  end

  local initial_display = {}
  for _, item in ipairs(all_items) do
    initial_display[#initial_display + 1] = format_display(item)
  end
  initial_display.action = on_selection_changed

  d:add("search_label", "label", { label = "Search:" }, 1, 1)
  d:add("search", "input", { focus = true, action = update_list_and_details }, 1, 2)
  d:add("list", "list", initial_display, 2, 1, 1, 2)
  d:add("desc_label", "label", { label = "Details:" }, 3, 1, 1, 2)
  d:add("description", "text", { read_only = true }, 4, 1, 1, 2)

  d:addButton("ok", "&Run", "accept")
  d:addButton("cancel", "&Cancel", "reject")

  d:setStretch("row", 2, 4)
  d:setStretch("row", 4, 2)
  d:setStretch("column", 2, 1)

  d:set("description", format_details(all_items[1]))

  local dialog_size = prefs.ipelet_search_size or { 600, 520 }
  if not d:execute(dialog_size) then
    return
  end

  local sel = d:get("list")
  if sel and current_matches[sel] then
    local chosen = current_matches[sel]
    model:action_ipelet(chosen.action)
  end
end
