# The web-assembly based editions of Ipe

In `src`, you can say
```
source /sw/emsdk/emsdk_env.sh
export IPEWASM=1
make -j
```

This will cause all the C++ libraries in Ipe to be compiled to web
assembly.  The necessary libraries can be found at
https://github.com/otfried/emscripten-ipe.


## Command line programs

These are automatically compiled by `make` and linked with Node.
Through Node, they have direct access to the real file system, so they
can be used exactly like the C++ counterparts:

```
node ipetoipe.js --help
```

I intend to package this so one can install using `npx`.


## Ipe Web

Running `make` in wasm mode in `src/ipe` creates
`src/ipe-web/src/ipe/ipe.{js,wasm}`.  All the style files, lua code,
and ipelets are already baked into this bundle, and directly available
in the (simulated) file system under `/opt/ipe`.

The user interface is written entirely in Typescript in
`src/ipe-web/src/ipe`.

The Ipe Web edition is started from `src/ipe-web/index.html`, which
loads `src/ipe-web/src/main.ts`, which is responsible for loading the
web assembly and setting up the IpeUI.

The Web edition uses a browser-resident permanent file system (IDBFS)
to persist Ipe documents, ipelets, and style sheets.  The IpeUi
contains a small file manager to look at these files, download them to
the local file system, and to upload new files.

The web assembly code is not aware of the existence of IDBFS at all,
it simply works with files available to it under `/home/ipe`.

One difficulty with the web-based UI is that normally Ipe runs in a
Lua-aware event loop.  Actions are triggered from the event loop in
protected calls.  One can't just call arbitrary operations on the
`IpeUi` from browser events - they should always go through Lua
protection.  The standard way of doing this is to call `IpeUi.action`.
There is an interface between Typescript `async` and Lua coroutines,
see `MODEL:wrapCall` and `MODEL:resumeLua`.



## Ipe VS Code extension

A VS Code extension consists of two parts, with the UI running inside
a sandboxed webview.  It is this webview that loads the web assembly
and presents the Ipe UI.  The startup code is in
`src/ipe-vscode/webview/startipe.ts`, it reuses the user interface
`src/ipe-vscode/webview/ipe` is a symbolic link to the ipe-web code).

Communication between the extension
(`src/ipe-vscode/src/extension.ts`) and the webview is through message
passing.

The host, that is, VS Code, is responsible for maintaining documents,
including dirty state, saving, loading, reverting, and backups.

The webview code works on a document `/home/ipe/document.ipe` or
`/home/ipe/document.pdf`.  It has no view of the actual file system
and does not even know the actual filename.

Ipelets are copied from the file system into the webview when it is
created, and used from `/opt/ipe`.

The built-in style sheets are baked in at `/opt/ipe/styles`.  When
style sheets are added, the webview requests the current list of
additionally available style sheets from the host, and loads it
through the host.

The considerations about the event loop and Lua protected calls from
the web edition apply here as well.


## Ipe Electron

Electron is a technology that packages a browser together with a web
application, so it appears like a desktop application.  Quite similar
to the VS Code extension (well, VS Code itself is an Electron
application), the UI and the web assembly runs in a sandboxed webview,
that needs to communicate with a Node-based host that has access to
the file system and can run child processes.

For the Electron edition, I made a different design choice: the web
assembly code actually believes it sees the entire file system and
uses actual file names.  This was done to make the file names
displayed inside Ipe correct.

So Ipe and Lua code can actually call
`fopen('/home/random-user/long-path/document.ipe')`, but behind the
scenes in Ipelib, a file `/tmp/XXXXXX` will actually be opened.  The
association between real filenames and these internal names is stored
in the `preloadCache`, and the methods `preloadFile` and `persistFile`
are used to synchronize internal and real files.

The two different mechanisms for the VS Code and the Electron edition
were based on the fact that in Electron, it is the `IpeUi` that
controls all file operations.  I'd rather not have to maintain these
two mechanisms in the future.

I haven't decided if I want to continue maintaining the Electron
edition (it most likely depends on how well the VS Code extension
works and if we get Live Sharing to work there).  If we keep this
edition, I might rewrite it to make it more similar to the VS Code
version, and get rid of preload/persist.

