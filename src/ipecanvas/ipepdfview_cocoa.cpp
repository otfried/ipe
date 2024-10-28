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

// --------------------------------------------------------------------
// ipe::PdfView for Cocoa
// --------------------------------------------------------------------

#include "ipepdfview_cocoa.h"
#include "ipecairopainter.h"

#include <CoreGraphics/CoreGraphics.h>
#include <cairo-quartz.h>
#include <cairo.h>

using namespace ipe;

// --------------------------------------------------------------------

PdfView::PdfView(IpePdfView * view) { iView = view; }

PdfView::~PdfView() {
    // nothing
}

void PdfView::invalidate() {
    NSSize s = [iView bounds].size;
    iWidth = s.width;
    iHeight = s.height;
    [iView setNeedsDisplayInRect:[iView bounds]];
}

void PdfView::invalidate(int x, int y, int w, int h) {
    NSRect rect;
    rect.origin.x = x;
    rect.origin.y = iHeight - 1 - y - h;
    rect.size.width = w;
    rect.size.height = h;
    [iView setNeedsDisplayInRect:rect];
}

void PdfView::drawRect(NSRect rect) {
    NSSize s = [iView bounds].size;
    NSSize sb = [iView convertSizeToBacking:s];
    iWidth = s.width;
    iHeight = s.height;
    iBWidth = sb.width;
    iBHeight = sb.height;

    if (![iView inLiveResize]) refreshSurface();

    CGContextRef myContext = [[NSGraphicsContext currentContext] CGContext];

    CGContextTranslateCTM(myContext, 0.0, iHeight);
    CGContextScaleCTM(myContext, 1.0, -1.0);

    cairo_surface_t * surface =
	cairo_quartz_surface_create_for_cg_context(myContext, iWidth, iHeight);
    cairo_t * cr = cairo_create(surface);

    if (iSurface) {
	cairo_set_source_surface(cr, iSurface, 0.0, 0.0);
	int w = cairo_image_surface_get_width(iSurface);
	int h = cairo_image_surface_get_height(iSurface);
	if (iWidth != w || iHeight != h) {
	    cairo_matrix_t matrix;
	    cairo_matrix_init_scale(&matrix, w / iWidth, h / iHeight);
	    cairo_pattern_set_matrix(cairo_get_source(cr), &matrix);
	}
	cairo_paint(cr);
    }
    cairo_destroy(cr);
    cairo_surface_finish(surface);
    cairo_surface_destroy(surface);
}

// --------------------------------------------------------------------

@implementation IpePdfView

- (instancetype)initWithFrame:(NSRect)rect {
    if ([super initWithFrame:rect]) _pdfView = new PdfView(self);
    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)isOpaque {
    return YES;
}

- (void)drawRect:(NSRect)rect {
    self.pdfView->drawRect(rect);
}

- (void)mouseDown:(NSEvent *)event {
    id obj = self.window.delegate;
    if ([obj respondsToSelector:@selector(pdfViewMouseButton:atLocation:)]) {
	NSPoint pw = [event locationInWindow];
	NSPoint p = [self convertPoint:pw fromView:nil];
	NSSize s = [self bounds].size;
	// flip y-axis
	p.y = s.height - 1 - p.y;
	Vector q = self.pdfView->devToUser(Vector(p.x, p.y));
	NSArray<NSNumber *> * qq =
	    @[ [NSNumber numberWithDouble:q.x], [NSNumber numberWithDouble:q.y] ];
	[obj performSelector:@selector(pdfViewMouseButton:atLocation:)
		  withObject:event
		  withObject:qq];
    }
}

- (void)rightMouseDown:(NSEvent *)event {
    [self mouseDown:event];
}

- (void)otherMouseDown:(NSEvent *)event {
    [self mouseDown:event];
}

- (void)keyDown:(NSEvent *)event {
    [super keyDown:event];
}

- (void)dealloc {
    // TODO: Is this actually called?
    NSLog(@"IpePdfView:dealloc");
    delete self.pdfView;
}

@end

// --------------------------------------------------------------------
