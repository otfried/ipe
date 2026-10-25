# Ipe scripts and ipelets

This folder contains Lua code for the Ipe drawing editor: **ipelets**
(extensions that appear in Ipe's Ipelets menu) and/or **scripts** (programs run
from the command line with `ipescript`).

## Documentation: read before writing code

Ipe's Lua API is specialized and not part of your training data, so do not
guess function names.  Fetch the documentation first:

- Complete documentation in one file: https://otfried.github.io/ipe/llms-full.txt
- Index of all parts: https://otfried.github.io/ipe/llms.txt

It contains a guide with working examples and a reference of every Lua function.
(It describes the development version of Ipe; if a function is missing in the
user's installed Ipe, check `config.version` or ask which version they use.)

## The most important rules

1. A **script** owns its document (`ipe.Document(file)`, change it directly,
   `doc:save(file)`).  An **ipelet** works on the document in the running editor
   and **must never modify it directly**: it registers an undo item with
   `model:register(t)` (or uses `model:creation`, `model:transformation`,
   `model:setAttribute`).
2. An ipelet defines the globals `label`, `about`, and `run(model, n)` (or
   `methods`).  It runs in a restricted environment: use `_G.type`, `_G.pcall`,
   `_G.io`, etc. for anything beyond `ipe`, `ipeui`, `math`, `string`, `table`,
   `assert`, `ipairs`, `pairs`, `print`, `tonumber`, `tostring`.
3. Numbering starts at 1 (pages, views, objects); layers are named.  Units
   are points, y points up.  Objects from a page are the page's own objects;
   copy with `:clone()`.

## Workflow

1. Say whether you are writing a script or an ipelet.
2. Look at a similar example in the documentation first.
3. Write the code in a file with a descriptive name in this folder.
4. **Test it.**  Create a small test document (a script using `ipe.Document()`,
   `ipe.Path`, `ipe.Text`, and `doc:save` is enough), then run

   ```sh
   ipescript run-ipelet --select all my-ipelet.lua test.ipe result.ipe [dialogfield=value ...]
   ```

   It runs the ipelet with the model of the real editor, then undoes and
   redoes its changes and reports if the page is not restored exactly.  Read
   its output, inspect `result.ipe`, and fix problems.  For scripts, run them
   with `ipescript name args` on the test document.
5. Tell the user exactly what you tested, and what cannot be tested without the
   Ipe editor (dialogs showing real controls, LaTeX text rendering, the canvas).
