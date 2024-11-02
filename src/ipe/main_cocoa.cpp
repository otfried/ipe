// -*- objc -*-
// main.mm
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

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

*/

#include "ipebase.h"
#include "ipelua.h"

#include "controls_cocoa.h"

// for COPYRIGHT_YEAR
#include "appui.h"

using namespace ipe;
using namespace ipelua;

#include "main_common.i"

// --------------------------------------------------------------------

static void setup_globals(lua_State * L) {
    lua_getglobal(L, "package");
    const char * luapath = getenv("IPELUAPATH");
    if (luapath)
	lua_pushstring(L, luapath);
    else
	push_string(L, Platform::folder(FolderLua, "?.lua"));
    lua_setfield(L, -2, "path");

    lua_newtable(L); // config table
    lua_pushliteral(L, "apple");
    lua_setfield(L, -2, "platform");
    lua_pushliteral(L, "cocoa");
    lua_setfield(L, -2, "toolkit");

    setup_common_config(L);

    NSArray<NSString *> * args = [[NSProcessInfo processInfo] arguments];
    int argc = [args count];
    lua_createtable(L, 0, argc - 1);
    for (int i = 1; i < argc; ++i) {
	lua_pushstring(L, [args[i] UTF8String]);
	lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "argv");

    NSRect e = [[NSScreen mainScreen] frame];
    int cx = int(e.size.width);
    int cy = int(e.size.height);
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, cx);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, cy);
    lua_rawseti(L, -2, 2);
    lua_setfield(L, -2, "screen_geometry");

    lua_setglobal(L, "config");

    lua_pushcfunction(L, ipe_tonumber);
    lua_setglobal(L, "tonumber");
}

// --------------------------------------------------------------------

static bool run_mainloop = false;

// On Mac OS, the event loop is already running,
// but if mainloop isn't called from Lua, we terminate.
static int mainloop(lua_State * L) {
    run_mainloop = true;
    return 0;
}

// --------------------------------------------------------------------

// Is bug #147 still relevant?
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

// --------------------------------------------------------------------

static const char * const about_text =
    "Copyright (c) 1993-%d Otfried Cheong\n\n"
    "The extensible drawing editor Ipe creates figures in PDF format, "
    "using LaTeX to format the text in the figures.\n"
    "Ipe is released under the GNU Public License.\n"
    "See http://ipe.otfried.org for details.\n"
    "If you are an Ipe fan and want to show others, have a look at the "
    "Ipe T-shirts (www.shirtee.com/en/store/ipe).\n\n"
    "Platinum and gold sponsors\n\n"
    " * Hee-Kap Ahn\n"
    " * Günter Rote\n"
    " * SCALGO\n"
    " * Martin Ziegler\n\n"
    "If you enjoy Ipe, feel free to treat the author on a cup of coffee at "
    "https://ko-fi.com/ipe7author.\n\n"
    "You can also become a member of the exclusive community of "
    "Ipe patrons (http://patreon.com/otfried). "
    "For the price of a cup of coffee per month you can make a meaningful contribution "
    "to the continuing development of Ipe.";

// --------------------------------------------------------------------

@implementation AppDelegate {
    lua_State * L;
}

- (instancetype)init {
    self = [super init];
    if (self) {
	L = setup_lua();
	setup_globals(L);
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)decoder {
    // this is needed on OSX 10.9, but not called on 10.11
    return [self init];
}

- (void)aboutIpe:(id)sender {
    NSString * text =
	[NSString stringWithFormat:@"Ipe %d.%d.%d", IPELIB_VERSION / 10000,
				   (IPELIB_VERSION / 100) % 100, IPELIB_VERSION % 100];

    NSString * info = [NSString stringWithFormat:@(about_text), COPYRIGHT_YEAR];

    NSAlert * alert = [[NSAlert alloc] init];
    [alert setMessageText:text];
    [alert setInformativeText:info];
    [alert setAlertStyle:NSInformationalAlertStyle];
    [alert runModal];
}

- (void)ipeAlwaysAction:(id)sender {
    String method = "action_";
    method += N2I([sender ipeAction]);
    lua_getglobal(L, method.z());
    // push_string(L, N2I([sender title]));
    lua_call(L, 0, 0);
}

- (void)ipeRecentFileAction:(id)sender {
    lua_getglobal(L, "action_recent_file");
    push_string(L, N2I([sender title]));
    lua_call(L, 1, 0);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
    lua_getglobal(L, "prefs");
    lua_getfield(L, -1, "terminate_on_close");
    bool term = lua_toboolean(L, -1);
    lua_pop(L, 2);
    return term;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    lua_run_ipe(L, mainloop);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    if (!run_mainloop) [NSApp terminate:self];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSNotification *)notification {
    NSArray<NSWindow *> * wins = [NSApp windows];
    int count = 0;
    int modified = 0;
    for (NSWindow * w : wins) {
	if ([w isMemberOfClass:[NSWindow class]]) {
	    if ([w.delegate respondsToSelector:@selector(ipeIsModified:)]) {
		bool mod = [w.delegate performSelector:@selector(ipeIsModified:)
					    withObject:self];
		count++;
		if (mod) modified++;
	    }
	}
    }
    ipeDebug("%d windows, %d modified", count, modified);
    if (modified == 0) return NSTerminateNow;

    NSString * warn = [NSString
	stringWithFormat:@"%d of your %d open Ipe windows contain unsaved changes!",
			 modified, count];
    NSAlert * alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Really quit Ipe?"];
    [alert setInformativeText:warn];
    [alert setAlertStyle:NSWarningAlertStyle];
    [alert addButtonWithTitle:@"Discard all changes"];
    [alert addButtonWithTitle:@"Cancel"];
    switch ([alert runModal]) {
    case NSAlertFirstButtonReturn: return NSTerminateNow;
    default: return NSTerminateCancel;
    }
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    lua_close(L);
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
    lua_getglobal(L, "file_open_event");
    lua_pushstring(L, filename.UTF8String);
    lua_call(L, 1, 0);
    return YES;
}

@end

// --------------------------------------------------------------------

int main(int argc, const char * argv[]) {
    ipe::Platform::initLib(ipe::IPELIB_VERSION);
    return NSApplicationMain(argc, argv);
}

// --------------------------------------------------------------------
