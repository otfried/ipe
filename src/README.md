
# Implementation notes

## Asynchronous methods

The JS and GTK implementations of Ipe are fully asynchronous:
everything that takes time returns control to the event loop.  When
this happens inside a Lua function, the Lua code yields, and is
resumed on completion of the operation.

Asynchronous operations are:

- messageBox
- fileDialog
- getClipboard (JS only)
- popup menu
- all dialogs
- the wait dialog while running Latex or showing an external editor

In QT, only dialogs are asynchronous, everything else runs their own
event loop.  On Windows and Cocoa, (if I remember right) nothing is
asynchronous.


## Coroutines

Only Lua coroutines can yield, the main thread cannot yield, and it is
not possible to yield across C functions.  For debugging of coroutines,
these functions are useful:
```
coroutine.running() ➤ running thread plus true if it's the main thread
coroutine.status(co) ➤ "running", "suspended", "normal", "dead"
coroutine.isyieldable([co])
debug.debug ()
debug.traceback([thread])
```

The AppUi creates a new coroutine for every action, and so does a
dialog when it executes a callback.  We use lua_resume as the single
mechanism to enable yield/resume in actions and to catch (and report)
Lua errors without crashing Ipe.  To make sure every error is caught,
every "resume" must go through the single function resumeLuaThread (in
ipeui).

To make sure the coroutines survive when yielded, every UI component
that relies on yielding must take a ref of the thread before it
yields, and unref after resumeLuaThread.

The Lua tools do not allow yielding while responding to mouse events,
so they only protect their Lua calls.  There are even a few completely
unprotected calls.




