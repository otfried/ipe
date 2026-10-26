Ipe is an extensible drawing editor.  Most of Ipe is written in Lua, and you
can extend it with **ipelets** (Lua files that add functions to the editor's
Ipelets menu), or process Ipe documents from the command line with **scripts**
(run with `ipescript`).  Both use the same Lua library, the table `ipe`.

## Read this first

These are the facts that most often go wrong when writing Ipe scripts and
ipelets.

1. **Script or ipelet?**  A script is run with `ipescript name args` and owns
   the document: it loads it with `ipe.Document(filename)` and saves it with
   `doc:save(filename)`.  An ipelet runs inside the Ipe editor and works on the
   document being edited, through the `model` argument of its `run` function.
2. **An ipelet must never modify the document directly.**  It changes the
   document only through an *undo item* passed to `model:register(t)`, or
   through `model:creation`, `model:transformation`, and `model:setAttribute`,
   which do this for you.  The usual pattern is: clone the page twice
   (`original`, `final`), modify `final`, and register an undo item with
   `undo = _G.revertOriginal` and `redo = _G.revertFinal`.  A script, on
   the other hand, modifies the document directly.
3. **An ipelet defines the global variables** `label` (menu text), `about`,
   and `run` (or `methods`, a table of `{ label=..., run=... }`).  `run`
   is called as `run(model, method_number)`.
4. **An ipelet runs in a restricted environment**: only `_G`, `ipe`, `ipeui`,
   `math`, `string`, `table`, `assert`, `ipairs`, `pairs`, `print`,
   `tonumber`, `tostring`, `prefs`, `config`, `shortcuts`, and `mouse` exist.
   Everything else must be reached through `_G`, for instance `_G.type(x)`,
   `_G.pcall`, `_G.io`.  Writing `type(x)` fails.
5. **Conventions**: pages, views, and objects are numbered from 1; layers are
   identified by name.  Coordinates are in points, the origin is at the
   lower left, and the y-axis points up.  `ipe.Vector`, `ipe.Matrix`, and the
   other geometric classes are immutable values (only `ipe.Rect` is mutable).
   Objects obtained from a page are the page's own objects, not copies: use
   `obj:clone()` and `page:clone()` to copy them.
6. **Errors**: functions that read files return `nil` and an error message;
   calling a function with illegal arguments raises a Lua error.
7. **Dialogs** use `ipeui.Dialog` and can only be shown inside an ipelet (or
   program) that runs inside the Ipe editor.
8. **Test ipelets without the editor** with
   `ipescript run-ipelet myipelet.lua input.ipe output.ipe [name=value ...]`.
   It runs the ipelet on a document with the real model of the Ipe editor,
   accepts dialogs with their default values (or the values given as
   `name=value`), then undoes and redoes the changes and reports if the page
   is not restored exactly.  Use `--select none|all|1,3`, `--method N`,
   `--page N`, `--view N` to control the run.  Always test ipelets this way
   before presenting them as working.
9. **Look at working examples**: the examples in this documentation, and the
   scripts (`scripts/` folder) and ipelets (`ipelets/` folder) that come with
   Ipe.

## How this documentation is organized

* **Guide**: a tutorial on writing scripts and ipelets, with complete
  examples.  It also describes the ipelet `model` object, undo items, and
  the snap settings.
* **Lua API reference**: every function of the Lua bindings (geometry,
  objects, attributes, pages, documents, stylesheets), the Ipe editor
  (`AppUi`), and dialogs and menus.
