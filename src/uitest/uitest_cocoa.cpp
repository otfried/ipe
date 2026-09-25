// -*- objc -*-
// --------------------------------------------------------------------
// uitest for Cocoa
// --------------------------------------------------------------------

#import <Cocoa/Cocoa.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

extern int luaopen_ipeui(lua_State * L);

// --------------------------------------------------------------------

static int traceback(lua_State * L) {
    if (!lua_isstring(L, 1)) /* 'message' not a string? */
	return 1;            /* keep it intact */
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_getfield(L, -1, "debug");
    if (!lua_istable(L, -1)) {
	lua_pop(L, 1);
	return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
	lua_pop(L, 2);
	return 1;
    }
    lua_pushvalue(L, 1);   // pass error message
    lua_pushinteger(L, 2); // skip this function and traceback
    lua_call(L, 2, 1);     // call debug.traceback
    return 1;
}

// --------------------------------------------------------------------

@interface IpeTestView : NSView
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
    lua_State * L;
}

@property NSWindow * window;
@property IpeTestView * content;

- (void)showPopupMenu:(NSEvent *)event;

@end

// --------------------------------------------------------------------

@implementation AppDelegate

- (id)init {
    self = [super init];
    if (self) {
	NSRect contentRect = NSMakeRect(0.0f, 0.0f, 800.0f, 600.0f);
	_window = [[NSWindow alloc]
	    initWithContentRect:contentRect
		      styleMask:NSTitledWindowMask | NSClosableWindowMask
				| NSResizableWindowMask | NSMiniaturizableWindowMask
			backing:NSBackingStoreBuffered
			  defer:YES];
	_content = [[IpeTestView alloc] initWithFrame:contentRect];
	L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_ipeui(L);
    }
    return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
    return YES;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    [self.window setContentView:self.content];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self.window makeKeyAndOrderFront:self];

    // push_winid(L, ui->windowId());
    // lua_setglobal(L, "appui");

    lua_pushcfunction(L, traceback);
    int res = luaL_loadfile(L, "uitest.lua");
    if (res != 0) {
	fprintf(stderr, "Could not load uitest.lua: %d\n", res);
	[NSApp terminate:self];
    }
    if (lua_pcall(L, 0, 0, -2)) {
	const char * errmsg = lua_tostring(L, -1);
	fprintf(stderr, "%s\n", errmsg);
	[NSApp terminate:self];
    }
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    lua_close(L);
}

- (void)showPopupMenu:(NSEvent *)event {
    NSRect rw = {[event locationInWindow], { 100.0, 100.0 }};
    NSRect rs = [[self.content window] convertRectToScreen:rw];
    lua_getglobal(L, "show_menu");
    lua_pushinteger(L, rs.origin.x);
    lua_pushinteger(L, rs.origin.y);
    lua_call(L, 2, 0);
}

@end

@implementation IpeTestView

- (void)mouseDown:(NSEvent *)event {
    [[NSApp delegate] showPopupMenu:event];
}

@end

// --------------------------------------------------------------------

int main(int argc, char * argv[]) {
    @autoreleasepool {
	NSApplication * application = [NSApplication sharedApplication];

	AppDelegate * applicationDelegate = [[AppDelegate alloc] init];

	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp activateIgnoringOtherApps:YES];

	[application setDelegate:applicationDelegate];

	[application run];
    }
    return 0;
}

// --------------------------------------------------------------------
