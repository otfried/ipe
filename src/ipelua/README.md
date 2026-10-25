# Lua Binding Tests

Run from the source directory after building Ipe's libraries:

```sh
make -C ipelua test
```

The target builds the bindings first and uses `LUA_PACKAGE` as the Lua
interpreter command. Override it with `LUA=lua5.4`, for example, when the
package and interpreter names differ. The interpreter must match the Lua
version used to build the bindings. `TESTFLAGS` passes options to the suite.

To run directly:

```sh
make -C ipelua
lua5.4 ipelua/test.lua ../build/lib/libipelua.so
```

The optional first argument is the Lua binding library to load. Without it,
the script uses an existing global `ipe`, or loads the installed `ipe` module.
Use the same Lua version as the binding library. Library names and loader
environment variables differ on macOS and Windows.

The script also runs through `ipescript`:

```sh
IPESCRIPTS=ipelua ../build/bin/ipescript test
```

## Coverage

The suite checks geometry and operators; every object type; attributes and
all shape representations; embedded bitmap metadata; spline conversions;
stylesheets and cascades; page layers, views, selection, maps, transforms,
and metadata; document pages and properties; in-memory XML round-trips;
copy independence; garbage collection; and invalid arguments.

It instruments the registered functions and userdata metatables and fails
if an in-memory entry point has not been called. This is an entry-point
coverage check, not a claim of exhaustive branch coverage. Currently, 42 test
groups exercise all 224 included entry points.

Fixtures are Lua values and XML strings. Test cases do not read or write
files. An external image's path is only stored, never loaded. The script and
native library naturally must be loaded by the runner.

Excluded file-system functions are blocked during testing and listed as
`SKIP`: file operations, image loading/saving, document loading/saving/export,
the LaTeX workflow (including `howToRunLatex`, which reads cache configuration),
and native ipelet loading. File-loading constructor overloads and
`ipe.folder("latex")`, which creates a directory, are also blocked.
Other folder lookups only construct path strings.

The suite covers `libipelua`, not GUI-only `AppUi` or `ipeui` bindings. Native
ipelet methods require loading a library and an editor helper, so are excluded.
Garbage-collection metamethods are exercised automatically, never manually.

## Documentation Check

`checkdocs.lua` is a static check that every function registered in the C++
sources (`libipelua`, `ipeui`, and `AppUi`) is mentioned in `bindings.txt`:

```sh
make -C ipelua checkdocs
```

It reads the `luaL_Reg` tables and looks for each name in the part of
`bindings.txt` that documents the corresponding class (see the table at the
top of the script). A name is documented if it appears as `:name` or `.name`
there, or as `ipe.name` / `ipeui.name` for global functions. It does not check
that the documentation is correct. Add class mappings or exceptions to the
script when adding new binding classes.

## Doc-tests for the manual

`doctest.lua` runs the examples of the manual chapter "Scripts and
ipelets" (`manual/examples`) automatically, using `ipescript`, which must be
built:

```sh
make -C ipelua doctest
```

It checks that every example script and ipelet does what the chapter says,
using `run-ipelet` for the ipelets (including its undo and redo check); that
every `ipescript` command shown in the chapter works and prints exactly the
output shown there; that the Lua code blocks compile; and that the chapter
and the example files agree. When you change an example or the chapter, run this
test. A new example file must be used in the chapter and be tested in
`doctest.lua`.

## Documentation for AI assistants

`manual/llms/mkllms.lua` converts the manual chapter "Scripts and ipelets" and
the Doxygen pages with the Lua bindings (`bindings.txt`, and the ipelet part of
`ipelets.txt`) into plain-text Markdown files for AI assistants: `llms.txt` (an
index in the format of llmstxt.org), `llms-guide.txt`, `llms-api.txt`,
`llms-full.txt`, and `AGENTS-ipelets.md` (a template for an `AGENTS.md` file that
users can put into their ipelet folder):

```sh
lua5.4 manual/llms/mkllms.lua build/html
```

The GitHub action `sphinx.yml` runs this after building the manual, so the files
are published together with the manual, at `https://otfried.github.io/ipe/`.
The script only understands the few ReStructuredText and Doxygen constructs
used in these sources and fails when it finds something else, so extend it when
you introduce new markup.  `manual/llms/intro.md` is the "read this first"
text at the start of the guide: keep it in line with the manual.

## Existing Issues

Expected failures match a specific assertion message. An unrelated exception
still fails the suite. An unexpected pass (`XPASS`) also fails, signaling that
the corresponding known-issue marker should be removed.

Additionally, document property updates that omit a nonempty `variant` can
trigger a native assertion. Tests explicitly supply one; this unsafe input
is reported as skipped, rather than aborting the whole process.
