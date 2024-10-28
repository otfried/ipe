// -*- objc -*-
// ipeselector_cocoa.h
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

#ifndef IPESELECTOR_COCOA_H
#define IPESELECTOR_COCOA_H

#include "ipedoc.h"
#include "ipethumbs.h"

#include <Cocoa/Cocoa.h>

// --------------------------------------------------------------------

@interface IpeSelectorProvider : NSObject

@property NSMutableArray * images;
@property ipe::Document * doc;
@property ipe::Thumbnail * thumb;
@property int page;
@property NSSize tnSize;
@property NSMutableArray<NSNumber *> * marks;

- (int)count;
- (NSString *)title:(int)index;
- (NSImage *)image:(int)index;
- (BOOL)marked:(int)index;

- (void)createMarks;
- (ipe::Buffer)renderImage:(int)index;
- (NSImage *)createImage:(ipe::Buffer)b;

@end

@interface IpeSelectorItem : NSObject

@property int index;
@property(assign) IpeSelectorProvider * provider;

@end

@interface IpeSelectorView : NSView
@property(assign) NSButton * button;

- (void)ipeSet:(IpeSelectorItem *)item;
@end

@interface IpeSelectorPrototype : NSCollectionViewItem
@end

extern int showPageSelectDialog(int width, int height, const char * title,
				IpeSelectorProvider * provider, int startIndex);

// --------------------------------------------------------------------
#endif
