// -*- objc -*-
// --------------------------------------------------------------------
// PageSelector for Cocoa
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

#include "ipeselector_cocoa.h"

#include "ipecanvas.h"

#include <cairo-quartz.h>
#include <cairo.h>

using namespace ipe;

inline NSString * C2N(const char * s) { return [NSString stringWithUTF8String:s]; }

// --------------------------------------------------------------------

// not thread-safe, but don't know how to do it otherwise...
// currently this is no problem because we run this as a modal dialog.
NSSize thumbnail_size;

@implementation IpeSelectorProvider

- (int)count {
    if (self.page >= 0)
	return self.doc->page(self.page)->countViews();
    else
	return self.doc->countPages();
}

- (NSString *)title:(int)index {
    if (self.page >= 0) {
	String t = self.doc->page(self.page)->viewName(index);
	if (t.empty())
	    return [NSString stringWithFormat:@"View %d", index + 1];
	else
	    return [NSString stringWithFormat:@"%d: %@", index + 1, C2N(t.z())];
    } else {
	String t = self.doc->page(index)->title();
	if (t.empty())
	    return [NSString stringWithFormat:@"Page %d", index + 1];
	else
	    return [NSString stringWithFormat:@"%d: %@", index + 1, C2N(t.z())];
    }
}

- (BOOL)marked:(int)index {
    return [self.marks[index] isEqual:@YES];
}

- (void)createMarks {
    self.marks = [NSMutableArray arrayWithCapacity:self.count];
    if (self.page >= 0) {
	Page * p = self.doc->page(self.page);
	for (int i = 0; i < self.doc->page(self.page)->countViews(); ++i)
	    [self.marks addObject:[NSNumber numberWithBool:p->markedView(i)]];
    } else {
	for (int i = 0; i < self.doc->countPages(); ++i)
	    [self.marks addObject:[NSNumber numberWithBool:self.doc->page(i)->marked()]];
    }
}

- (NSImage *)image:(int)index {
    if (!self.images) {
	self.images = [NSMutableArray arrayWithCapacity:[self count]];
	for (int i = 0; i < [self count]; ++i) [self.images addObject:[NSNull null]];
    }
    if (self.images[index] == [NSNull null]) {
	Buffer b = [self renderImage:index];
	self.images[index] = [self createImage:b];
    }
    return (NSImage *)self.images[index];
}

- (Buffer)renderImage:(int)index {
    if (self.page >= 0)
	return self.thumb->render(self.doc->page(self.page), index);
    else {
	Page * p = self.doc->page(index);
	return self.thumb->render(p, p->countViews() - 1);
    }
}

- (NSImage *)createImage:(Buffer)b {
    int w = 2 * self.tnSize.width;
    int h = 2 * self.tnSize.height;
    return [NSImage imageWithSize:self.tnSize
			  flipped:YES
		   drawingHandler:^(NSRect rect) {
		     cairo_surface_t * image = cairo_image_surface_create_for_data(
			 (uint8_t *)b.data(), CAIRO_FORMAT_ARGB32, w, h, 4 * w);
		     CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
		     cairo_surface_t * surface =
			 cairo_quartz_surface_create_for_cg_context(ctx, w, h);
		     cairo_t * cr = cairo_create(surface);
		     cairo_set_source_surface(cr, image, 0.0, 0.0);
		     cairo_matrix_t matrix;
		     cairo_matrix_init_scale(&matrix, 2.0, 2.0);
		     cairo_pattern_set_matrix(cairo_get_source(cr), &matrix);
		     cairo_paint(cr);
		     cairo_destroy(cr);
		     cairo_surface_finish(surface);
		     cairo_surface_destroy(surface);
		     cairo_surface_destroy(image);
		     return YES;
		   }];
}

@end

// --------------------------------------------------------------------

@implementation IpeSelectorItem

@end

// --------------------------------------------------------------------

@implementation IpeSelectorView

- (id)initWithFrame:(NSRect)frameRect {
    NSSize buttonSize = {thumbnail_size.width + 6, thumbnail_size.height + 30};
    NSSize itemSize = {buttonSize.width + 20, buttonSize.height + 20};
    const NSPoint buttonOrigin = {10, 10};

    self = [super initWithFrame:(NSRect){frameRect.origin, itemSize}];
    if (self) {
	NSButton * b =
	    [[NSButton alloc] initWithFrame:(NSRect){buttonOrigin, buttonSize}];
	b.imagePosition = NSImageAbove;
	b.buttonType = NSMomentaryPushInButton;
	b.action = @selector(ipePageSelected:);
	[self addSubview:b];
	self.button = b;
    }
    return self;
}

- (void)ipeSet:(IpeSelectorItem *)item {
    self.button.tag = item.index;
    self.button.title = [item.provider title:item.index];
    self.button.image = [item.provider image:item.index];
}

@end

// --------------------------------------------------------------------

@implementation IpeSelectorPrototype

- (void)loadView {
    self.view = [[IpeSelectorView alloc] initWithFrame:NSZeroRect];
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    [(IpeSelectorView *)[self view] ipeSet:representedObject];
}

@end

// --------------------------------------------------------------------

@interface IpeSelectorDelegate : NSObject <NSWindowDelegate>

- (void)ipePageSelected:(id)sender;

@end

@implementation IpeSelectorDelegate

- (void)ipePageSelected:(id)sender {
    [NSApp stopModalWithCode:[sender tag]];
    NSWindow * panel = [NSApp modalWindow];
    if (panel) [panel close];
}

- (BOOL)windowShouldClose:(id)sender {
    [NSApp stopModalWithCode:-1];
    // [NSApp abortModal];
    return YES;
}

@end

// --------------------------------------------------------------------

int showPageSelectDialog(int width, int height, const char * title,
			 IpeSelectorProvider * provider, int startIndex) {
    thumbnail_size = provider.tnSize;

    NSPanel * panel =
	[[NSPanel alloc] initWithContentRect:NSMakeRect(200., 100., width, height)
				   styleMask:NSTitledWindowMask | NSResizableWindowMask
					     | NSClosableWindowMask
				     backing:NSBackingStoreBuffered
				       defer:YES];
    panel.title = C2N(title);

    IpeSelectorDelegate * delegate = [IpeSelectorDelegate new];
    panel.delegate = delegate;

    NSMutableArray * elements = [NSMutableArray arrayWithCapacity:[provider count]];

    for (int i = 0; i < [provider count]; ++i) {
	IpeSelectorItem * item = [IpeSelectorItem new];
	item.index = i;
	item.provider = provider;
	[elements addObject:item];
    }

    NSScrollView * scroll =
	[[NSScrollView alloc] initWithFrame:[panel.contentView frame]];
    scroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.hasVerticalScroller = YES;
    panel.contentView = scroll;

    NSCollectionView * cv = [[NSCollectionView alloc] initWithFrame:NSZeroRect];
    cv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.documentView = cv;

    cv.itemPrototype = [IpeSelectorPrototype new];
    cv.content = elements;

    int result = [NSApp runModalForWindow:panel];
    return result;
}

// --------------------------------------------------------------------

int CanvasBase::selectPageOrView(Document * doc, int page, int startIndex, int pageWidth,
				 int width, int height) {
    // for retina displays, create bitmap with twice the resolution
    Thumbnail thumbs(doc, 2 * pageWidth);

    NSSize tnSize;
    tnSize.width = thumbs.width() / 2.0;
    tnSize.height = thumbs.height() / 2.0;

    const char * title = (page >= 0) ? "Ipe: Select view" : "Ipe: Select page";

    IpeSelectorProvider * provider = [IpeSelectorProvider new];
    provider.doc = doc;
    provider.thumb = &thumbs;
    provider.page = page;
    provider.tnSize = tnSize;

    return showPageSelectDialog(width, height, title, provider, startIndex);
}

// --------------------------------------------------------------------
