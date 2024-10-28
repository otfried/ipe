// -*- objc -*-
// controls_cocoa.cpp
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

#include "controls_cocoa.h"

#include "ipecairopainter.h"

#include "ipeuilayout_cocoa.h"

#include <CoreGraphics/CoreGraphics.h>
#include <cairo-quartz.h>
#include <cairo.h>

@implementation IpePathView {
    Cascade * iCascade;
    AllAttributes iAll;
}

- (instancetype)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    if (self) { iCascade = nullptr; }
    return self;
}

- (void)setAttributes:(const AllAttributes *)all cascade:(Cascade *)sheet {
    iCascade = sheet;
    iAll = *all;
    [self setNeedsDisplayInRect:[self bounds]];
}

- (void)drawRect:(NSRect)rect {
    NSRect bounds = [self bounds];
    int w = bounds.size.width;
    int h = bounds.size.height;

    CGContextRef myContext = [[NSGraphicsContext currentContext] CGContext];
    cairo_surface_t * sf = cairo_quartz_surface_create_for_cg_context(myContext, w, h);
    cairo_t * cc = cairo_create(sf);
    cairo_set_source_rgb(cc, 1, 1, 0.8);
    cairo_rectangle(cc, 0, 0, w, h);
    cairo_fill(cc);

    if (iCascade) {
	cairo_translate(cc, 0, h);
	double zoom = 2.0;
	cairo_scale(cc, zoom, -zoom);
	Vector v0 = (1.0 / zoom) * Vector(0.1 * w, 0.5 * h);
	Vector v1 = (1.0 / zoom) * Vector(0.7 * w, 0.5 * h);
	Vector u1 = (1.0 / zoom) * Vector(0.88 * w, 0.8 * h);
	Vector u2 = (1.0 / zoom) * Vector(0.80 * w, 0.5 * h);
	Vector u3 = (1.0 / zoom) * Vector(0.88 * w, 0.2 * h);
	Vector u4 = (1.0 / zoom) * Vector(0.96 * w, 0.5 * h);
	Vector mid = 0.5 * (v0 + v1);
	Vector vf = iAll.iFArrowShape.isMidArrow() ? mid : v1;
	Vector vr = iAll.iRArrowShape.isMidArrow() ? mid : v0;

	CairoPainter painter(iCascade, nullptr, cc, 3.0, false, false);
	painter.setPen(iAll.iPen);
	painter.setDashStyle(iAll.iDashStyle);
	painter.setStroke(iAll.iStroke);
	painter.setFill(iAll.iFill);
	painter.pushMatrix();
	painter.newPath();
	painter.moveTo(v0);
	painter.lineTo(v1);
	painter.drawPath(EStrokedOnly);
	if (iAll.iFArrow)
	    Path::drawArrow(painter, vf, Angle(0), iAll.iFArrowShape, iAll.iFArrowSize,
			    100.0);
	if (iAll.iRArrow)
	    Path::drawArrow(painter, vr, Angle(IpePi), iAll.iRArrowShape,
			    iAll.iRArrowSize, 100.0);
	painter.setDashStyle(Attribute::NORMAL());
	painter.setTiling(iAll.iTiling);
	painter.newPath();
	painter.moveTo(u1);
	painter.lineTo(u2);
	painter.lineTo(u3);
	painter.lineTo(u4);
	painter.closePath();
	painter.drawPath(iAll.iPathMode);
	painter.popMatrix();
    }
    cairo_destroy(cc);
    cairo_surface_finish(sf);
    cairo_surface_destroy(sf);
}

- (void)mouseDown:(NSEvent *)event {
    if ([event modifierFlags] & NSControlKeyMask) {
	[self rightMouseDown:event];
	return;
    }

    double w = [self bounds].size.width;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];

    if (p.x < w * 3 / 10) {
	[self.delegate
	    pathViewAttributeChanged:(iAll.iRArrow ? "rarrow|false" : "rarrow|true")];
    } else if (p.x > w * 4 / 10 && p.x < w * 72 / 100) {
	[self.delegate
	    pathViewAttributeChanged:(iAll.iFArrow ? "farrow|false" : "farrow|true")];
    } else if (p.x > w * 78 / 100) {
	switch (iAll.iPathMode) {
	case EStrokedOnly:
	    [self.delegate pathViewAttributeChanged:"pathmode|strokedfilled"];
	    break;
	case EStrokedAndFilled:
	    [self.delegate pathViewAttributeChanged:"pathmode|filled"];
	    break;
	case EFilledOnly:
	    [self.delegate pathViewAttributeChanged:"pathmode|stroked"];
	    break;
	}
    }
}

- (void)rightMouseDown:(NSEvent *)event {
    NSRect rw = {[event locationInWindow], { 100.0, 100.0 }};
    NSRect rs = [[self window] convertRectToScreen:rw];
    [self.delegate pathViewPopup:rs.origin];
}

@end

// --------------------------------------------------------------------

@interface IpeLayerItem : NSObject
@property(copy) NSString * name;
@property(copy) NSString * text;
@property BOOL checked;
@property BOOL active;
@property BOOL locked;
@property Page::SnapMode snapMode;
@end

@implementation IpeLayerItem
@end

@interface IpeTableView : NSTableView
@end

@implementation IpeTableView
- (BOOL)acceptsFirstResponder {
    return NO;
}
@end

@interface IpeLayerField : NSTextField

- (void)setItem:(IpeLayerItem *)item inRow:(int)row;

@end

@implementation IpeLayerField

- (void)setItem:(IpeLayerItem *)item inRow:(int)row {
    self.tag = row;
    self.editable = NO;
    self.bordered = NO;
    self.stringValue = item.text;
    self.drawsBackground = (item.active || item.locked);
    if (item.active)
	self.backgroundColor = [NSColor colorWithRed:1.0 green:1.0 blue:0.0 alpha:1.0];
    else if (item.locked)
	self.backgroundColor = [NSColor colorWithRed:1.0 green:0.85 blue:0.85 alpha:1.0];
    switch (item.snapMode) {
    case Page::SnapMode::Never:
	self.textColor = [NSColor colorWithRed:0.0 green:0.0 blue:0.7 alpha:1.0];
	break;
    case Page::SnapMode::Always:
	self.textColor = [NSColor colorWithRed:0.0 green:0.7 blue:0.0 alpha:1.0];
	break;
    default:
	if (item.active || item.locked)
	    self.textColor = [NSColor colorWithRed:0 green:0 blue:0 alpha:1.0];
	else
	    self.textColor = [NSColor textColor];
	break;
    }
}

- (void)mouseDown:(NSEvent *)event {
    [self.target ipeLayerClicked:self.tag];
}

- (void)rightMouseDown:(NSEvent *)event {
    NSRect rw = {[event locationInWindow], { 100.0, 100.0 }};
    NSRect rs = [[self window] convertRectToScreen:rw];
    [self.target ipeLayerMenuAt:rs.origin forRow:self.tag];
}
@end

// --------------------------------------------------------------------

@implementation IpeLayerView {
    NSTableView * iTV;
    NSMutableArray<IpeLayerItem *> * iLayers;
}

- (instancetype)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    if (self) {
	[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	iLayers = [NSMutableArray arrayWithCapacity:20];
	iTV = [[IpeTableView alloc] initWithFrame:rect];
	NSTableColumn * column1 = [[NSTableColumn alloc] initWithIdentifier:@"checks"];
	[iTV addTableColumn:column1];
	NSTableColumn * column2 = [[NSTableColumn alloc] initWithIdentifier:@"names"];
	[iTV addTableColumn:column2];
	[iTV setHeaderView:nil];
	[iTV setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleNone];
	[iTV setDataSource:self];
	[iTV setDelegate:self];
	[iTV setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[self setDocumentView:iTV];
	[self setHasVerticalScroller:YES];
	layout(self, nil, "h>0", 80.0);
	layout(self, nil, "w>0", 80.0);
    }
    return self;
}

- (void)setPage:(const Page *)page view:(int)view {
    std::vector<int> objCounts;
    page->objectsPerLayer(objCounts);
    [iLayers removeAllObjects];
    int active = -1;
    for (int i = 0; i < page->countLayers(); ++i) {
	IpeLayerItem * item = [[IpeLayerItem alloc] init];
	item.name = I2N(page->layer(i));
	item.text =
	    [NSString stringWithFormat:@"%@ (%d)", I2N(page->layer(i)), objCounts[i]];
	item.checked = page->visible(view, i);
	item.active = (page->layer(i) == page->active(view));
	item.locked = page->isLocked(i);
	item.snapMode = page->snapping(i);
	if (page->layer(i) == page->active(view)) active = i;
	[iLayers addObject:item];
    }
    [iTV reloadData];
    [[iTV tableColumnWithIdentifier:@"checks"] sizeToFit];
    if (active >= 0)
	[iTV selectRowIndexes:[NSIndexSet indexSetWithIndex:active]
	    byExtendingSelection:NO];
}

- (void)ipeLayerToggled:(id)sender {
    int row = [sender tag];
    if (iLayers[row].checked)
	[self.delegate layerAction:@"selectoff" forLayer:iLayers[row].name];
    else
	[self.delegate layerAction:@"selecton" forLayer:iLayers[row].name];
}

- (void)ipeLayerClicked:(int)row {
    [self.delegate layerAction:@"active" forLayer:iLayers[row].name];
}

- (void)ipeLayerMenuAt:(NSPoint)p forRow:(int)row {
    [self.delegate layerMenuAt:p forLayer:iLayers[row].name];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tv {
    return [iLayers count];
}

- (id)tableView:(NSTableView *)tv
    objectValueForTableColumn:(NSTableColumn *)col
			  row:(NSInteger)row {
    if ([[col identifier] isEqualToString:@"checks"])
	return [NSNumber numberWithBool:iLayers[row].checked];
    else
	return iLayers[row].text;
}

- (NSView *)tableView:(NSTableView *)tv
    viewForTableColumn:(NSTableColumn *)col
		   row:(NSInteger)row {
    if ([[col identifier] isEqualToString:@"checks"]) {
	NSButton * result = [tv makeViewWithIdentifier:@"LayerCheck" owner:self];
	if (result == nil) {
	    result = [[NSButton alloc] initWithFrame:NSMakeRect(0., 0., 20., 20.)];
	    [result setButtonType:NSSwitchButton];
	    [result setImagePosition:NSImageOnly];
	}
	[result setAction:@selector(ipeLayerToggled:)];
	[result setTarget:self];
	[result setTag:row];
	[result setState:iLayers[row].checked];
	return result;
    } else {
	IpeLayerField * result = [tv makeViewWithIdentifier:@"LayerName" owner:self];
	if (result == nil) {
	    result = [[IpeLayerField alloc] initWithFrame:NSMakeRect(0., 0., 200., 20.)];
	    result.identifier = @"LayerName";
	    auto cell = [result cell];
	    [cell setLineBreakMode:NSLineBreakByCharWrapping];
	    [cell setTruncatesLastVisibleLine:YES];
	}
	[result setItem:iLayers[row] inRow:row];
	[result setTarget:self];
	return result;
    }
}

@end

// --------------------------------------------------------------------

@implementation IpeBookmarksView {
    NSTableView * iTV;
    NSMutableArray<NSString *> * iBookmarks;
}

- (instancetype)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    if (self) {
	[self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	iBookmarks = [NSMutableArray arrayWithCapacity:20];
	iTV = [[IpeTableView alloc] initWithFrame:rect];
	NSTableColumn * column = [[NSTableColumn alloc] initWithIdentifier:@"bookmarks"];
	[iTV addTableColumn:column];
	[iTV setHeaderView:nil];
	[iTV setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleNone];
	[iTV setDataSource:self];
	[iTV setDelegate:self];
	[iTV setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[iTV setUsesAlternatingRowBackgroundColors:YES];
	[iTV setAction:@selector(ipeSelectedBookmark:)];
	[iTV setTarget:self];
	[self setDocumentView:iTV];
	[self setHasVerticalScroller:YES];
	layout(self, nil, "h>0", 100.0);
	layout(self, nil, "w>0", 160.0);
    }
    return self;
}

- (void)ipeSelectedBookmark:(id)sender {
    int row = [sender clickedRow];
    if (self.delegate && row >= 0) [self.delegate bookmarkSelected:row];
}

- (void)setBookmarks:(int)no fromStrings:(const String *)s {
    [iBookmarks removeAllObjects];
    for (int i = 0; i < no; ++i) [iBookmarks addObject:I2N(s[i].z())];
    [iTV reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tv {
    return [iBookmarks count];
}

- (id)tableView:(NSTableView *)tv
    objectValueForTableColumn:(NSTableColumn *)col
			  row:(NSInteger)row {
    return iBookmarks[row];
}

- (NSView *)tableView:(NSTableView *)tv
    viewForTableColumn:(NSTableColumn *)col
		   row:(NSInteger)row {
    NSTextField * result = [tv makeViewWithIdentifier:@"Bookmarks" owner:self];
    if (result == nil) {
	result = [[NSTextField alloc] initWithFrame:NSMakeRect(0., 0., 200., 20.)];
	result.identifier = @"Bookmarks";
	result.editable = NO;
	result.bordered = NO;
	result.drawsBackground = NO;
    }
    [result setStringValue:iBookmarks[row]];
    return result;
}

@end

// --------------------------------------------------------------------
