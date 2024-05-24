// -*- objc -*-
// --------------------------------------------------------------------
// Special widgets for Cocoa
// --------------------------------------------------------------------
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

#ifndef CONTROLS_COCOA_H
#define CONTROLS_COCOA_H

#include "ipelib.h"

#include <Cocoa/Cocoa.h>

using namespace ipe;

inline String N2I(NSString *aStr) { return String(aStr.UTF8String); }
inline const char *N2C(NSString *aStr) { return aStr.UTF8String; }
inline NSString *I2N(String s) {return [NSString stringWithUTF8String:s.z()];}
inline NSString *C2N(const char *s) {return [NSString stringWithUTF8String:s];}

@interface IpeMenuItem : NSMenuItem
@property NSString * ipeAction;

- (instancetype) initWithTitle:(NSString *) aTitle
		     ipeAction:(NSString *) anIpeAction
		 keyEquivalent:(NSString *) aKey;
@end

@protocol IpeControlsDelegate

- (void) pathViewAttributeChanged:(String) attr;
- (void) pathViewPopup:(NSPoint) p;
- (void) bookmarkSelected:(int) index;
- (void) layerMenuAt:(NSPoint) p forLayer:(NSString *) layer;
- (void) layerAction:(NSString *) actionName forLayer:(NSString *) layer;

@end

@interface IpePathView : NSView

@property (weak) id <IpeControlsDelegate> delegate;

- (void) setAttributes:(const AllAttributes *) all cascade:(Cascade *) sheet;

@end

@interface IpeLayerView : NSScrollView <NSTableViewDataSource,
					  NSTableViewDelegate>

@property (weak) id<IpeControlsDelegate> delegate;

- (void) setPage:(const Page *) page view:(int) view;

- (void) ipeLayerClicked:(int) row;
- (void) ipeLayerMenuAt:(NSPoint) p forRow:(int) row;
- (void) ipeLayerToggled:(id) sender;

@end

@interface IpeBookmarksView : NSScrollView <NSTableViewDataSource,
					      NSTableViewDelegate>

@property (weak) id<IpeControlsDelegate> delegate;

- (void) setBookmarks:(int) no fromStrings:(const String *) s;

@end

// --------------------------------------------------------------------
#endif
