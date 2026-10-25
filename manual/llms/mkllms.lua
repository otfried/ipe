-- --------------------------------------------------------------------
-- Generate plain-text (Markdown) documentation for AI assistants
--
-- Usage:  lua5.4 manual/llms/mkllms.lua [output-directory [base-url]]
--
-- Reads the manual chapter "Scripts and ipelets" (ReStructuredText) and
-- the Doxygen pages with the Lua bindings (src/ipelua/*.txt), and writes
--   llms.txt         index, in the format of https://llmstxt.org
--   llms-guide.txt   the tutorial on writing scripts and ipelets
--   llms-api.txt     the reference for the Lua bindings and the ipelet model
--   llms-full.txt    both, in one file
--   AGENTS-ipelets.md  template for an AGENTS.md file that users can put into
--                    their ipelet folder; it points assistants to the files above
-- to the output directory (default: the current directory).  The base-url
-- (default https://otfried.github.io/ipe/, or the environment variable
-- LLMS_BASE_URL) is where the files will be published.
--
-- This is a plain Lua program that does not need Ipe.  It only understands
-- the few ReStructuredText and Doxygen constructs used in these sources
-- and stops with an error message when it finds something else, so that
-- new markup cannot silently end up in the output.
-- --------------------------------------------------------------------

local scriptdir = arg[0]:match("^(.*)/[^/]*$") or "."
local manual = scriptdir .. "/.."
local root = manual .. "/.."
local outdir = arg[1] or "."
local baseurl = arg[2] or os.getenv("LLMS_BASE_URL") or "https://otfried.github.io/ipe/"
if baseurl:sub(-1) ~= "/" then baseurl = baseurl .. "/" end

local chapterFile = "82_ipelets.rst"
local apiSources = {
  { file = root .. "/src/ipelua/ipelets.txt", only = { ["lua-ipelets"] = true } },
  { file = root .. "/src/ipelua/bindings.txt" },
}
-- where Doxygen's \include and \verbinclude look for files
local includeDirs = { manual .. "/examples", root .. "/scripts", root .. "/test" }

local function fail(message)
  io.stderr:write("mkllms: " .. message .. "\n")
  os.exit(1)
end

local function warn(message)
  io.stderr:write("mkllms: warning: " .. message .. "\n")
end

local function readfile(name)
  local f = io.open(name, "rb")
  if not f then fail("cannot read " .. name) end
  local s = f:read("a")
  f:close()
  return s
end

local function writefile(name, data)
  local f = io.open(name, "wb")
  if not f then fail("cannot write " .. name) end
  f:write(data)
  f:close()
end

local function splitLines(text)
  local lines = {}
  for line in (text .. "\n"):gmatch("(.-)\r?\n") do lines[#lines + 1] = line end
  if lines[#lines] == "" then lines[#lines] = nil end
  return lines
end

local function trim(s) return (s:gsub("^%s+", ""):gsub("%s+$", "")) end

local function findInclude(name)
  for _, dir in ipairs(includeDirs) do
    local f = io.open(dir .. "/" .. name, "rb")
    if f then
      local s = f:read("a")
      f:close()
      return s
    end
  end
end

local function emitFence(out, lang, lines)
  out[#out + 1] = "```" .. lang
  for _, l in ipairs(lines) do out[#out + 1] = l end
  out[#out + 1] = "```"
end

----------------------------------------------------------------------
-- ReStructuredText
----------------------------------------------------------------------

-- title and label tables of all chapters of the manual
local labels = {}
local function isUnderline(line, title)
  if not line or #title == 0 or #line < #title then return false end
  local c = line:sub(1, 1)
  return ("=-\"*~^#"):find(c, 1, true) ~= nil and line == string.rep(c, #line)
end

local function scanLabels(lines)
  local pending = {}
  for i, line in ipairs(lines) do
    local label = line:match("^%.%. _([^:]+):%s*$")
    if label then pending[#pending + 1] = label end
    if #pending > 0 and line ~= "" and not line:match("^%.%. _")
       and isUnderline(lines[i + 1], line) then
      for _, p in ipairs(pending) do labels[p] = line end
      pending = {}
    end
  end
end

do
  for _, line in ipairs(splitLines(readfile(manual .. "/index.rst"))) do
    local name = line:match("^   ([%w_%-]+)%s*$")
    local f = name and io.open(manual .. "/" .. name .. ".rst", "rb")
    if f then
      scanLabels(splitLines(f:read("a")))
      f:close()
    end
  end
end

local function rstInline(s)
  -- external links
  s = s:gsub("`([^`<]-)%s*<([^>`]+)>`__?", function (text, url)
    return "[" .. trim(text) .. "](" .. url .. ")"
  end)
  -- roles
  s = s:gsub(":(%a+):`([^`]*)`", function (role, arg)
    if role == "code" or role == "file" or role == "program" or role == "kbd" then
      return "`" .. arg .. "`"
    elseif role == "guilabel" then
      return "**" .. arg .. "**"
    elseif role == "menuselection" then
      return "*" .. arg:gsub("%s*%-%->%s*", " → ") .. "*"
    elseif role == "ref" then
      local text, label = arg:match("^(.-)%s*<([^>]+)>$")
      if text then return text end
      if not labels[arg] then fail("unknown label in :ref:`" .. arg .. "`") end
      return '"' .. labels[arg] .. '"'
    elseif role == "doc" then
      return "the chapter " .. '"' .. arg .. '"'
    elseif role == "download" then
      return ""
    end
    fail("unknown role :" .. role .. ": in '" .. s .. "'")
  end)
  s = s:gsub("``(.-)``", "`%1`")
  return s
end

-- Lines of the indented block that follows line i (blank lines are
-- included), dedented; and the index of the first line after the block
local function indentedBlock(lines, i)
  local j = i + 1
  while j <= #lines and (lines[j] == "" or lines[j]:match("^%s")) do j = j + 1 end
  local block = {}
  local indent
  for k = i + 1, j - 1 do
    block[#block + 1] = lines[k]
    if lines[k] ~= "" then
      local n = #lines[k]:match("^(%s*)")
      indent = indent and math.min(indent, n) or n
    end
  end
  for k, l in ipairs(block) do block[k] = l:sub((indent or 0) + 1) end
  while #block > 0 and block[#block] == "" do block[#block] = nil end
  return block, j
end

-- split leading option lines (":name: value") off a directive block
local function directiveOptions(block)
  local options = {}
  local k = 1
  while k <= #block and (block[k] == "" or block[k]:match("^:[%w%-]+:")) do
    local name, value = block[k]:match("^:([%w%-]+):%s*(.*)$")
    if name then options[name] = value end
    k = k + 1
  end
  local body = { table.unpack(block, k) }
  return options, body
end

local function rstTable(block, out)
  local options, body = directiveOptions(block)
  local headerRows = tonumber(options["header-rows"] or "0")
  local rows = {}
  for _, line in ipairs(body) do
    local first = line:match("^%*%s+%-%s?(.*)$")
    local cell = line:match("^%s+%-%s?(.*)$")
    if first then
      rows[#rows + 1] = { first }
    elseif cell then
      table.insert(rows[#rows], cell)
    elseif line ~= "" then
      local row = rows[#rows]
      if not row then fail("unexpected line in list-table: " .. line) end
      row[#row] = row[#row] .. " " .. trim(line)
    end
  end
  local ncols = 0
  for _, row in ipairs(rows) do ncols = math.max(ncols, #row) end
  local function rowText(row)
    local cells = {}
    for k = 1, ncols do cells[k] = trim(rstInline(row[k] or "")):gsub("|", "\\|") end
    return "| " .. table.concat(cells, " | ") .. " |"
  end
  out[#out + 1] = rowText(rows[1])
  out[#out + 1] = "|" .. string.rep("---|", ncols)
  for k = 2, #rows do out[#out + 1] = rowText(rows[k]) end
end

local function rstToMarkdown(text)
  local lines = splitLines(text)
  local out = {}
  local levels = {}   -- underline character -> heading level
  local nlevels = 0
  local i = 1
  while i <= #lines do
    local line = lines[i]
    local directive, argument = line:match("^%.%. ([%w%-]+)::%s*(.*)$")
    if line == "" then
      out[#out + 1] = ""
      i = i + 1
    elseif line:match("^%.%. _[^:]+:%s*$") then
      i = i + 1
    elseif line:match("^%.%.%s*$") then
      i = i + 1
    elseif directive then
      local block, nextIndex = indentedBlock(lines, i)
      if directive == "code-block" then
        local _, body = directiveOptions(block)
        emitFence(out, argument ~= "" and argument or "lua", body)
      elseif directive == "literalinclude" then
        local options = directiveOptions(block)
        local f = io.open(manual .. "/" .. argument, "rb")
        if not f then fail("literalinclude: no such file " .. argument) end
        local content = f:read("a")
        f:close()
        local body = splitLines(content)
        emitFence(out, options.language or "text", body)
      elseif directive == "list-table" then
        rstTable(block, out)
      else
        fail("unknown directive '" .. directive .. "' at line " .. i)
      end
      out[#out + 1] = ""   -- Markdown needs a blank line after code and tables
      i = nextIndex
    elseif line:match("^%.%.") then
      fail("unknown construct at line " .. i .. ": " .. line)
    elseif isUnderline(lines[i + 1], line) and not line:match("^%s") then
      local c = lines[i + 1]:sub(1, 1)
      if not levels[c] then
	nlevels = nlevels + 1
	levels[c] = nlevels
      end
      out[#out + 1] = string.rep("#", levels[c]) .. " " .. rstInline(line)
      i = i + 2
    elseif not line:match("^%s") and not line:match("^[%*%-] ") and lines[i + 1]
	   and lines[i + 1]:match("^  %S") then
      -- definition list: term and indented definition
      local block, nextIndex = indentedBlock(lines, i)
      local parts = {}
      for _, l in ipairs(block) do
	if l == "" then
	  if #parts > 0 and parts[#parts] ~= "\n\n" then parts[#parts + 1] = "\n\n" end
	else
	  if #parts > 0 and parts[#parts] ~= "\n\n" then parts[#parts + 1] = " " end
	  parts[#parts + 1] = trim(l)
	end
      end
      -- convert the markup after joining, as it may span several lines
      out[#out + 1] = "* " .. rstInline(trim(line) .. ": " .. table.concat(parts))
      out[#out + 1] = ""
      i = nextIndex
    else
      -- inline markup (such as a role) may continue on the next line
      local joined = line
      local count = select(2, joined:gsub("`", ""))
      while count % 2 == 1 and lines[i + 1] and lines[i + 1] ~= "" do
	i = i + 1
	joined = joined .. " " .. trim(lines[i])
	count = select(2, joined:gsub("`", ""))
      end
      out[#out + 1] = rstInline(joined)
      i = i + 1
    end
  end
  return out
end

----------------------------------------------------------------------
-- Doxygen
----------------------------------------------------------------------

-- pages: list of { id, title, lines }
local function doxygenPages(text)
  local pages = {}
  local current
  for _, line in ipairs(splitLines(text)) do
    local id, title = line:match("^/?%*?!?%s*\\page%s+(%S+)%s+(.-)%s*$")
    if id then
      current = { id = id, title = title, lines = {} }
      pages[#pages + 1] = current
    elseif line:match("^/%*!%s*$") or line:match("^%*/%s*$") then
      -- comment delimiters
    elseif current then
      current.lines[#current.lines + 1] = line
    end
  end
  return pages
end

local doxTitles = {}   -- page and section ids -> title

local function doxInline(s)
  s = s:gsub("\\subpage%s+(%S+)", function (id)
    return doxTitles[id] or fail("unknown page " .. id)
  end)
  s = s:gsub('\\ref%s+(%S+)%s+"([^"]*)"', "%2")
  s = s:gsub("\\ref%s+(%S+)", function (id)
    return '"' .. (doxTitles[id] or fail("unknown reference " .. id)) .. '"'
  end)
  s = s:gsub("\\<", "<"):gsub("\\>", ">")
  s = s:gsub("<tt>(.-)</tt>", "`%1`")
  s = s:gsub("``(.-)``", "`%1`")
  s = s:gsub("\\([ecab])%s+([^%s]+)", function (cmd, word)
    local tail = ""
    local trailing = word:match("[%.,;:]+$")
    if trailing then
      word = word:sub(1, -#trailing - 1)
      tail = trailing
    end
    if word:sub(-1) == ")" and not word:find("(", 1, true) then
      word = word:sub(1, -2)
      tail = ")" .. tail
    end
    if cmd == "c" then return "`" .. word .. "`" .. tail end
    if cmd == "b" then return "**" .. word .. "**" .. tail end
    return "*" .. word .. "*" .. tail
  end)
  s = s:gsub("^\\li%s+", "* ")
  return s
end

local function doxToMarkdown(page)
  local out = { "# " .. page.title }
  local inVerbatim = false
  local verbatim
  -- a section that includes a missing example file is dropped entirely
  local sectionStart, sectionBroken = 1, false
  local function endSection()
    if sectionBroken then
      for k = #out, sectionStart, -1 do out[k] = nil end
    end
    sectionBroken = false
  end
  local n = 1
  while n <= #page.lines do
    local line = page.lines[n]
    n = n + 1
    -- a command such as \c may be at the end of a line
    while not inVerbatim and line:match("\\[ecab]%s*$") and page.lines[n] do
      line = line:gsub("%s+$", "") .. " " .. trim(page.lines[n])
      n = n + 1
    end
    if inVerbatim then
      if line:match("^%s*\\endverbatim%s*$") then
	emitFence(out, "lua", verbatim)
	inVerbatim = false
      else
	verbatim[#verbatim + 1] = line
      end
    elseif line:match("^%s*\\verbatim%s*$") then
      inVerbatim = true
      verbatim = {}
    else
      local sec, title = line:match("^\\section%s+(%S+)%s+(.-)%s*$")
      local subsec, subtitle = line:match("^\\subsection%s+(%S+)%s+(.-)%s*$")
      local incl = line:match("^\\verbinclude%s+(%S+)%s*$") or line:match("^\\include%s+(%S+)%s*$")
      if sec then
	endSection()
	sectionStart = #out + 1
	out[#out + 1] = "## " .. title
      elseif subsec then
	out[#out + 1] = "### " .. subtitle
      elseif incl then
	local content = findInclude(incl)
	if content then
	  emitFence(out, "lua", splitLines(content))
	else
	  warn("file " .. incl .. " included in the documentation does not exist, "
	       .. "leaving out the section of page " .. page.id)
	  sectionBroken = true
	end
      else
	out[#out + 1] = doxInline(line)
      end
    end
  end
  if inVerbatim then fail("unterminated \\verbatim in page " .. page.id) end
  endSection()
  return out
end

----------------------------------------------------------------------
-- Checking the result
----------------------------------------------------------------------

local function checkMarkdown(name, lines)
  local inFence = false
  for n, line in ipairs(lines) do
    if line:match("^```") then
      inFence = not inFence
    elseif not inFence then
      local bad = line:match("\\%a+") or line:match(":%a+:`") or line:match("^%.%. ")
		  or line:match("<tt>") or line:match("^%s*\\")
      if bad then fail(name .. ": markup left over at output line " .. n .. ": " .. line) end
    end
  end
  if inFence then fail(name .. ": unterminated code block") end
end

local function shiftHeadings(lines, by)
  local out = {}
  local inFence = false
  for _, line in ipairs(lines) do
    if line:match("^```") then inFence = not inFence end
    if not inFence and line:match("^#+ ") then line = string.rep("#", by) .. line end
    out[#out + 1] = line
  end
  return out
end

----------------------------------------------------------------------
-- Put it all together
----------------------------------------------------------------------

local version = (readfile(root .. "/src/config.mak"):match("\nIPEVERS%s*=%s*([%d%.]+)")) or "unknown"

local function gitCommit()
  local sha = os.getenv("GITHUB_SHA")
  if sha then return sha:sub(1, 12) end
  local p = io.popen("git -C '" .. root .. "' rev-parse --short=12 HEAD 2>/dev/null")
  if p then
    local s = p:read("l")
    p:close()
    if s and #s > 0 then return s end
  end
  return "unknown"
end

local stamp = string.format("Ipe %s, generated %s from commit %s", version,
			    os.date("!%Y-%m-%d"), gitCommit())

local function join(lines) return table.concat(lines, "\n") .. "\n" end

-- the guide
local guide = rstToMarkdown(readfile(manual .. "/" .. chapterFile))
checkMarkdown("guide", guide)

-- the reference
local pages = {}
local selected = {}
for _, source in ipairs(apiSources) do
  for _, page in ipairs(doxygenPages(readfile(source.file))) do
    doxTitles[page.id] = page.title
    if not source.only or source.only[page.id] then
      selected[#selected + 1] = page
    end
    for _, line in ipairs(page.lines) do
      local sec, title = line:match("^\\section%s+(%S+)%s+(.-)%s*$")
      if not sec then sec, title = line:match("^\\subsection%s+(%S+)%s+(.-)%s*$") end
      if sec then doxTitles[sec] = title end
    end
  end
end
local api = {}
for _, page in ipairs(selected) do
  for _, line in ipairs(doxToMarkdown(page)) do api[#api + 1] = line end
  api[#api + 1] = ""
end
checkMarkdown("api", api)

local intro = splitLines(readfile(scriptdir .. "/intro.md"))
checkMarkdown("intro", intro)

local summary = "Ipe is an extensible drawing editor with a Lua scripting interface. "
  .. "This documentation explains how to write Ipe scripts and ipelets (extensions "
  .. "of the editor) in Lua, with complete examples and a reference for all Lua "
  .. "functions."

local function document(title, parts)
  local all = { "# " .. title, "", "> " .. summary, "", "(" .. stamp .. ")", "" }
  for _, part in ipairs(parts) do
    for _, line in ipairs(part) do all[#all + 1] = line end
    all[#all + 1] = ""
  end
  -- squeeze runs of blank lines, except inside code blocks
  local out = {}
  local inFence = false
  for _, line in ipairs(all) do
    if line:match("^```") then inFence = not inFence end
    if inFence or line ~= "" or (#out > 0 and out[#out] ~= "") then out[#out + 1] = line end
  end
  while out[#out] == "" do out[#out] = nil end
  return join(out)
end

local function titled(title, lines)
  local out = { "# " .. title, "" }
  for _, l in ipairs(shiftHeadings(lines, 1)) do out[#out + 1] = l end
  return out
end

local introPart = { "## Introduction", "" }
for _, l in ipairs(shiftHeadings(intro, 1)) do introPart[#introPart + 1] = l end

local guidePart = shiftHeadings(guide, 1)
local apiPart = { "## Lua API reference", "" }
for _, l in ipairs(shiftHeadings(api, 2)) do apiPart[#apiPart + 1] = l end

writefile(outdir .. "/llms-guide.txt",
	  document("Ipe: writing scripts and ipelets (guide)", { introPart, guidePart }))
writefile(outdir .. "/llms-api.txt",
	  document("Ipe: Lua API reference", { apiPart }))
writefile(outdir .. "/llms-full.txt",
	  document("Ipe: writing scripts and ipelets (complete documentation)",
		   { introPart, guidePart, apiPart }))

writefile(outdir .. "/llms.txt", join({
  "# Ipe: writing scripts and ipelets",
  "",
  "> " .. summary,
  "",
  "Ipe scripts run from the command line (`ipescript`) and process Ipe documents.",
  "Ipelets run inside the Ipe editor and must register undo items instead of",
  "modifying the document directly.  Read the guide first: it lists the most",
  "common mistakes.  Test ipelets without the editor with `ipescript run-ipelet`.",
  "",
  "## Documentation",
  "",
  "- [Guide](" .. baseurl .. "llms-guide.txt): introduction, rules and pitfalls, "
    .. "tutorial with tested examples for scripts and ipelets",
  "- [Lua API reference](" .. baseurl .. "llms-api.txt): all Lua functions of "
    .. "Ipe (geometry, objects, pages, documents, stylesheets, the editor, "
    .. "dialogs and menus), and the ipelet model and undo items",
  "- [Complete documentation](" .. baseurl .. "llms-full.txt): guide and reference "
    .. "in one file",
  "",
  "## Optional",
  "",
  "- [AGENTS.md for ipelet folders](" .. baseurl .. "AGENTS-ipelets.md): a short "
    .. "instruction file that users can place in the folder with their ipelets; "
    .. "it points coding assistants to this documentation and tells them to test",
  "- [Ipe manual](" .. baseurl .. "): the user manual, in HTML",
  "- [Ipelib documentation](https://ipe.otfried.org/ipelib/): C++ library and "
    .. "Lua bindings, as generated by Doxygen",
  "- [Source code](https://github.com/otfried/ipe): includes the scripts, the "
    .. "ipelets, and the examples used in this documentation",
  "",
  "(" .. stamp .. ")",
}))

-- the AGENTS.md template for users' ipelet folders
do
  local template = readfile(scriptdir .. "/AGENTS-ipelets.md")
  local text = template:gsub("{{BASE_URL}}", baseurl)
  if text:find("{{", 1, true) then fail("AGENTS-ipelets.md: unknown placeholder") end
  writefile(outdir .. "/AGENTS-ipelets.md", text)
end

local function size(name)
  local s = readfile(outdir .. "/" .. name)
  return string.format("%s: %d bytes", name, #s)
end
print("mkllms: " .. size("llms.txt") .. ", " .. size("llms-guide.txt") .. ", "
      .. size("llms-api.txt") .. ", " .. size("llms-full.txt"))
