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

## Existing Issues

Expected failures match a specific assertion message. An unrelated exception
still fails the suite. An unexpected pass (`XPASS`) also fails, signaling that
the corresponding known-issue marker should be removed.

Additionally, document property updates that omit a nonempty `variant` can
trigger a native assertion. Tests explicitly supply one; this unsafe input
is reported as skipped, rather than aborting the whole process.
