# Working on the Ipe source code

Ipe is an extensible drawing editor (C++ libraries, Qt/GTK/Cocoa/Windows/web
user interfaces, and a large part of the editor written in Lua).  This file is
for AI coding assistants and new contributors.  The user documentation is in
`manual/` (Sphinx), the C++ and Lua API documentation is in `src/ipelua/*.txt`
and the headers (Doxygen).

If you are asked to *write an ipelet or script for a user* rather than to work
on Ipe itself, read `manual/82_ipelets.rst` (the chapter "Scripts and ipelets")
instead, or the plain-text version at <https://otfried.github.io/ipe/llms.txt>.

## Layout

| Directory | Contents |
|---|---|
| `src/ipelib`, `src/include` | core C++ library: geometry, objects, pages, documents, PDF/XML I/O |
| `src/ipecairo`, `src/ipecanvas` | rendering with Cairo, the canvas widget |
| `src/ipelua` | Lua bindings of the library (`libipelua`); `bindings.txt` is their documentation; tests in `test.lua` |
| `src/ipeui` | dialogs, menus, timers for Lua, one backend per toolkit: `ipeui_qt`, `ipeui_gtk`, `ipeui_win`, `ipeui_cocoa`, `ipeui_js`, and the shared `ipeui_common` |
| `src/ipe` | the editor: C++ per-toolkit `appui_*`, `uilua.cpp` (the `AppUi` Lua object), and `lua/` with the editor's Lua code (`model.lua`, `tools.lua`, `actions.lua`, `prefs.lua`, ...) |
| `src/ipelets/lua` | the ipelets that come with Ipe |
| `scripts` | scripts run with `ipescript` (including `run-ipelet`, a test runner for ipelets) |
| `src/ipescript`, `ipetoipe`, `iperender`, ... | command line programs |
| `src/ipejs`, `ipe-web`, `ipe-electron`, `ipe-vscode`, `ipe-cli` | WebAssembly build, and the web, Electron, VS Code, and command-line editions built on it |
| `manual` | the user manual (Sphinx), including tested examples in `manual/examples` |

Do not edit generated or ignored files (see `.gitignore`), such as
`src/ipe-web/src/ipe/ipe.js`.

## Building

Dependencies and options are described in `doc/install-qt.txt`,
`doc/install-gtk.txt`, and `doc/macos.txt`.  On Linux:

```sh
cd src
make IPEUI=QT          # output in build/;  IPEUI=GTK builds into gtkbuild/
```

`IPESTRICT=1` (used by CI) turns warnings into errors.  A change to a file
that exists in several toolkit versions (for instance `ipeui_qt.cpp`,
`ipeui_gtk.cpp`, `ipeui_win.cpp`, `ipeui_cocoa.cpp`, `ipeui_js.cpp`, and the
`ipe-web` TypeScript that implements the JS dialogs) must be made in all of them,
even if you can only build some: Windows, macOS, and the web edition are not
built on a normal Linux machine, so write that code with extra care and
say that it is untested.

## Tests and checks

Run the checks that cover what you changed.  CI (`.github/workflows`) runs them
all.

```sh
# Lua bindings (needs the built libraries; use your build dir for LD_LIBRARY_PATH)
LD_LIBRARY_PATH=build/lib make -C src/ipelua test

# every registered Lua binding must be documented in bindings.txt
make -C src/ipelua checkdocs

# run the examples of the manual chapter "Scripts and ipelets" (needs ipescript)
LD_LIBRARY_PATH=build/lib make -C src/ipelua doctest

# the manual must build without warnings
sphinx-build -b html manual /tmp/manual-html

# files for AI assistants (llms.txt) are generated from the manual
lua5.4 manual/llms/mkllms.lua /tmp/llms

# web edition
cd src/ipe-web && pnpm run check
```

For the `uitest` program (manual test of the dialog code), see `src/uitest`.

## Conventions

* **New or changed Lua bindings** (in `src/ipelua`, `src/ipeui`, `src/ipe/uilua.cpp`)
  need an entry in `src/ipelua/bindings.txt`, a test in `src/ipelua/test.lua` if
  they are in `libipelua`, and `make -C src/ipelua checkdocs` must stay clean.
  Names ending in `Async` are internal and exempt.
* **Documentation is code**: examples in `manual/examples` are run by
  `make doctest`, which also compares the output shown in the manual
  with the real output.  Update the manual text and the examples together.
* **C++**: format with `clang-format` (`src/.clang-format`: 4 spaces, tabs for
  continuation, 90 columns).  Every source file has the GPL license header.
* **Lua**: follow the style of the neighboring file.  Code that runs inside
  ipelets sees only a restricted environment (see the manual), so use `_G.` for
  everything else.
* Keep changes surgical: this code base is used on five platforms.
