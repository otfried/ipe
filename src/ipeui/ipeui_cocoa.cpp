// -*- objc -*-
// --------------------------------------------------------------------
// Lua bindings for Cocoa dialogs
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

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

#include "ipeui_common.h"

#import <Cocoa/Cocoa.h>

#include "ipeuilayout_cocoa.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>

#define COLORICONSIZE 12

inline const char * N2C(NSString * aStr) { return aStr.UTF8String; }
inline NSString * C2N(const char * s) { return [NSString stringWithUTF8String:s]; }
inline NSString * S2N(const std::string & s) { return C2N(s.c_str()); }

// --------------------------------------------------------------------

class PDialog;

@interface IpeDialogDelegate : NSObject <NSWindowDelegate, NSTableViewDataSource,
					 NSTableViewDelegate, NSTextFieldDelegate>

@property PDialog * dialog;

- (void)ipeControl:(id)sender;

@end

// --------------------------------------------------------------------

void addToLayout(NSView * view, NSView * subview) {
    [view addSubview:subview];
    [subview setTranslatesAutoresizingMaskIntoConstraints:NO];
}

id layoutGuide(NSView * owner) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101100
    if (@available(macOS 10.11, *)) {
	NSLayoutGuide * g = [[NSLayoutGuide alloc] init];
	[owner addLayoutGuide:g];
	return g;
    }
#endif
    NSView * g = [[NSView alloc] initWithFrame:NSZeroRect];
    addToLayout(owner, g);
    return g;
}

static NSLayoutAttribute layoutAttribute(char ch) {
    switch (ch) {
    case 'l': return NSLayoutAttributeLeft;
    case 'r': return NSLayoutAttributeRight;
    case 't': return NSLayoutAttributeTop;
    case 'b': return NSLayoutAttributeBottom;
    case 'w': return NSLayoutAttributeWidth;
    case 'h': return NSLayoutAttributeHeight;
    case 'x': return NSLayoutAttributeCenterX;
    case 'y': return NSLayoutAttributeCenterY;
    default: return NSLayoutAttributeNotAnAttribute;
    }
}

static NSLayoutRelation layoutRelation(char ch) {
    switch (ch) {
    case '<': return NSLayoutRelationLessThanOrEqual;
    case '=':
    default: return NSLayoutRelationEqual;
    case '>': return NSLayoutRelationGreaterThanOrEqual;
    }
}

NSLayoutConstraint * layout(id a, id b, const char * rel, double gap, double multiplier,
			    BOOL activate) {
    assert(strlen(rel) == 3);
    NSLayoutAttribute a1 = layoutAttribute(rel[0]);
    NSLayoutAttribute b1 = layoutAttribute(rel[2]);
    NSLayoutConstraint * c = [NSLayoutConstraint constraintWithItem:a
							  attribute:a1
							  relatedBy:layoutRelation(rel[1])
							     toItem:b
							  attribute:b1
							 multiplier:multiplier
							   constant:gap];
    c.active = activate;
    return c;
}

// --------------------------------------------------------------------

// Exported from ipeui!
NSString * ipeui_set_mnemonic(NSString * title, NSButton * button) {
    unichar mnemonic = 0;
    NSMutableString * result = [NSMutableString stringWithCapacity:[title length]];
    size_t i = 0;
    while (i < [title length]) {
	unichar ch = [title characterAtIndex:i];
	if (ch != '&') {
	    [result appendFormat:@"%C", ch];
	    ++i;
	} else if (i + 1 < [title length]) {
	    ch = [title characterAtIndex:i + 1];
	    if (!mnemonic && ch != '&') mnemonic = ch;
	    [result appendFormat:@"%C", ch];
	    i += 2;
	} else
	    ++i; // discard trailing &
    }
    if (button) {
	[button setTitle:result];
	if (mnemonic) {
	    [button setKeyEquivalent:[NSString stringWithCharacters:&mnemonic length:1]];
	    [button setKeyEquivalentModifierMask:NSAlternateKeyMask | NSCommandKeyMask];
	}
    }
    return result;
}

// --------------------------------------------------------------------

class PDialog : public Dialog {
public:
    PDialog(lua_State * L0, WINID parent, const char * caption, const char * language);
    virtual ~PDialog();

    void itemAction(int idx);
    bool moveListSelection(int delta);
    int numberOfRows(int idx);
    NSString * row(int idx, int row);

    virtual void setMapped(lua_State * L, int idx);
    virtual Result buildAndRun(int w, int h);
    virtual void retrieveValues();
    virtual void enableItem(int idx, bool value);
    virtual void acceptDialog(lua_State * L);

private:
    void fillComboBox(NSPopUpButton * cb, int idx);
    void setTextView(NSTextView * tv, const std::string & s);
    std::string getTextView(NSTextView * tv);
    void layoutControls();

private:
    NSPanel * iPanel;
    IpeDialogDelegate * iDelegate;
    NSMutableArray<NSView *> * iViews;
};

// --------------------------------------------------------------------

@implementation IpeDialogDelegate

- (void)ipeControl:(id)sender {
    self.dialog->itemAction([sender tag]);
}

// sent only for edits by the user, not for setStringValue
- (void)controlTextDidChange:(NSNotification *)notification {
    self.dialog->itemAction([[notification object] tag]);
}

// Up/Down in an input move the selection of the dialog's list
- (BOOL)control:(NSControl *)control
	       textView:(NSTextView *)textView
    doCommandBySelector:(SEL)command {
    if (command == @selector(moveUp:)) return self.dialog->moveListSelection(-1);
    if (command == @selector(moveDown:)) return self.dialog->moveListSelection(1);
    return NO;
}

- (BOOL)windowShouldClose:(id)sender {
    [NSApp stopModalWithCode:0];
    return YES;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    self.dialog->itemAction([[notification object] tag]);
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tv {
    return self.dialog->numberOfRows([tv tag]);
}

- (id)tableView:(NSTableView *)tv
    objectValueForTableColumn:(NSTableColumn *)col
			  row:(NSInteger)row {
    return self.dialog->row([tv tag], row);
}

- (NSView *)tableView:(NSTableView *)tv
    viewForTableColumn:(NSTableColumn *)col
		   row:(NSInteger)row {
    NSTextField * result = [tv makeViewWithIdentifier:@"DialogList" owner:self];
    if (result == nil) {
	result = [[NSTextField alloc] initWithFrame:NSMakeRect(0., 0., 200., 20.)];
	result.identifier = @"DialogList";
	result.editable = NO;
	result.bordered = NO;
	result.drawsBackground = NO;
    }
    [result setStringValue:self.dialog->row([tv tag], row)];
    return result;
}

@end

// --------------------------------------------------------------------

// An image from a bitmap in memory (RGBA, not premultiplied); nil for other kinds.
static NSImage * bitmapImage(const ImageSpec & spec) {
    if (spec.kind != ImageSpec::EBitmap) return nil;
    NSBitmapImageRep * rep = [[NSBitmapImageRep alloc]
	initWithBitmapDataPlanes:NULL
		      pixelsWide:spec.width
		      pixelsHigh:spec.height
		   bitsPerSample:8
		 samplesPerPixel:4
			hasAlpha:YES
			isPlanar:NO
		  colorSpaceName:NSDeviceRGBColorSpace
		    bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
		     bytesPerRow:4 * spec.width
		    bitsPerPixel:32];
    if (!rep) return nil;
    memcpy([rep bitmapData], spec.pixels.data(), spec.pixels.size());
    NSImage * image = [[NSImage alloc] initWithSize:NSMakeSize(spec.width, spec.height)];
    [image addRepresentation:rep];
    return image;
}

@interface IpeDialogImage : NSView

- (instancetype)initWithWidth:(int)width height:(int)height spec:(const ImageSpec &)spec;
- (void)setSpec:(const ImageSpec &)spec;

@end

@implementation IpeDialogImage {
    ImageSpec iSpec;
    int iWidth;
    int iHeight;
}

- (instancetype)initWithWidth:(int)width height:(int)height spec:(const ImageSpec &)spec {
    self = [super initWithFrame:NSMakeRect(0., 0., width, height)];
    if (self) {
	iWidth = width;
	iHeight = height;
	iSpec = spec;
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (NSSize)intrinsicContentSize {
    return NSMakeSize(iWidth, iHeight);
}

- (void)setSpec:(const ImageSpec &)spec {
    iSpec = spec;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    NSRect bounds = [self bounds];
    [[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:0.86 alpha:1.0] setFill];
    NSRectFill(bounds);
    [[NSColor colorWithCalibratedRed:0.62 green:0.62 blue:0.50 alpha:1.0] setStroke];
    NSFrameRect(bounds);

    double zoom = iSpec.zoom;
    NSRect body = NSInsetRect(bounds, 18., 16.);

    NSImage * image = nil;
    if (iSpec.kind == ImageSpec::EFile) {
	image = [[NSImage alloc] initWithContentsOfFile:S2N(iSpec.text)];
    } else if (iSpec.kind == ImageSpec::EBitmap) {
	image = bitmapImage(iSpec);
    }
    if (image) {
	NSSize size = image.size;
	size.width /= zoom;
	size.height /= zoom;
	double scale =
	    std::min(body.size.width / size.width, body.size.height / size.height);
	scale = std::min(1.0, scale);
	NSSize scaled = NSMakeSize(size.width * scale, size.height * scale);
	NSRect target =
	    NSMakeRect(NSMidX(body) - scaled.width / 2.,
		       NSMidY(body) - scaled.height / 2., scaled.width, scaled.height);
	[[NSGraphicsContext currentContext]
	    setImageInterpolation:NSImageInterpolationHigh];
	[image drawInRect:target
		  fromRect:NSZeroRect
		 operation:NSCompositingOperationSourceOver
		  fraction:1.0
	    respectFlipped:YES
		     hints:nil];
	return;
    }
    NSDictionary * attrs = @{
	NSFontAttributeName : [NSFont systemFontOfSize:12.],
	NSForegroundColorAttributeName : [NSColor secondaryLabelColor]
    };
    NSString * message = (iSpec.kind == ImageSpec::EText && !iSpec.text.empty())
			     ? S2N(iSpec.text)
			     : @"Preview unavailable";
    [message drawInRect:body withAttributes:attrs];
}

@end

// --------------------------------------------------------------------
// The sorter element: a collection view (the same kind of legacy,
// prototype-based collection view that the page sorter uses) showing items
// with an icon, an optional label, and an optional checkbox.

static NSString * const IpeSorterDragType = @"ipeSorterDragIndexes";

@interface IpeSorterEntry : NSObject
@property NSInteger number; // number of the item in the table given to Lua (from 1)
@property BOOL hasMark;
@property BOOL marked;
@property BOOL readOnly;
@property NSString * label;
@property NSImage * image;
@end

@implementation IpeSorterEntry
@end

@interface IpeSorterItemView : NSBox
@property(assign) NSImageView * image;
@property(assign) NSTextField * ititle;
@property(assign) NSButton * imarked;
@property IpeSorterEntry * entry;

- (void)ipeSet:(IpeSorterEntry *)entry;
@end

@implementation IpeSorterItemView

- (id)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
	self.titlePosition = NSNoTitle;
	self.boxType = NSBoxCustom;
	self.cornerRadius = 8.0;
	self.borderType = NSLineBorder;
	self.contentViewMargins = NSZeroSize;
	self.fillColor = [NSColor controlBackgroundColor];
	self.borderColor = [NSColor controlBackgroundColor];
	NSImageView * v = [[NSImageView alloc] initWithFrame:NSZeroRect];
	v.imageAlignment = NSImageAlignCenter;
	v.imageScaling = NSImageScaleProportionallyDown;
	[self addSubview:v];
	self.image = v;
	NSButton * b = [[NSButton alloc] initWithFrame:NSMakeRect(0., 0., 16., 16.)];
	[b setButtonType:NSSwitchButton];
	b.title = @"";
	b.target = self;
	b.action = @selector(ipeToggle:);
	[self addSubview:b];
	self.imarked = b;
	NSTextField * t = [[NSTextField alloc] initWithFrame:NSZeroRect];
	t.editable = NO;
	t.selectable = NO;
	t.bordered = NO;
	t.drawsBackground = NO;
	t.alignment = NSCenterTextAlignment;
	t.lineBreakMode = NSLineBreakByTruncatingTail;
	[self addSubview:t];
	self.ititle = t;
	[self ipeLayout];
    }
    return self;
}

// icon above, checkbox and label in a line below
- (void)ipeLayout {
    NSSize s = [self bounds].size;
    double textX = self.imarked.hidden ? 4. : 22.;
    self.image.frame =
	NSMakeRect(4., 24., std::max(s.width - 8., 0.), std::max(s.height - 28., 0.));
    self.imarked.frame = NSMakeRect(4., 4., 16., 16.);
    self.ititle.frame = NSMakeRect(textX, 3., std::max(s.width - textX - 4., 0.), 18.);
}

- (void)setFrameSize:(NSSize)size {
    [super setFrameSize:size];
    [self ipeLayout];
}

- (void)ipeSet:(IpeSorterEntry *)entry {
    if (!entry) return;
    self.entry = entry;
    self.image.image = entry.image;
    self.ititle.stringValue = entry.label ? entry.label : @"";
    self.imarked.hidden = !entry.hasMark;
    self.imarked.state = entry.marked ? NSOnState : NSOffState;
    self.imarked.enabled = !entry.readOnly;
    [self ipeLayout];
}

- (void)ipeToggle:(id)sender {
    self.entry.marked = ([(NSButton *)sender state] == NSOnState);
}

// the context menu is created by the collection view
- (NSMenu *)menuForEvent:(NSEvent *)event {
    return [[self superview] menuForEvent:event];
}

@end

@interface IpeSorterPrototype : NSCollectionViewItem
@end

@implementation IpeSorterPrototype

- (void)loadView {
    self.view = [[IpeSorterItemView alloc] initWithFrame:NSMakeRect(0., 0., 100., 100.)];
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    [(IpeSorterItemView *)[self view] ipeSet:representedObject];
}

- (void)setSelected:(BOOL)flag {
    [super setSelected:flag];

    NSBox * view = (NSBox *)[self view];
    if (flag) {
	view.fillColor = [NSColor selectedControlColor];
	view.borderColor = [NSColor blackColor];
    } else {
	view.fillColor = [NSColor controlBackgroundColor];
	view.borderColor = [NSColor controlBackgroundColor];
    }
}

@end

@interface IpeSorterView : NSCollectionView <NSCollectionViewDelegate>
@property BOOL ipeReadOnly;

- (instancetype)initWithReadOnly:(BOOL)readOnly;
- (void)ipeSetItems:(const std::vector<SorterItem> &)items;
- (void)ipeFillOrder:(std::vector<int> &)order;
@end

@implementation IpeSorterView {
    NSMutableArray<IpeSorterEntry *> * iEntries; // current items, in order
    NSMutableArray<IpeSorterEntry *> * iCut;     // items removed by "Cut"
    NSIndexSet * iDragIndexes;
    NSInteger iMenuHit; // item under the pointer when the menu was shown, or -1
}

- (instancetype)initWithReadOnly:(BOOL)readOnly {
    self = [super initWithFrame:NSZeroRect];
    if (self) {
	self.ipeReadOnly = readOnly;
	iEntries = [NSMutableArray array];
	iCut = [NSMutableArray array];
	iMenuHit = -1;
	self.itemPrototype = [IpeSorterPrototype new];
	self.selectable = YES;
	self.allowsMultipleSelection = YES;
	self.delegate = self;
	if (!readOnly) [self registerForDraggedTypes:@[ IpeSorterDragType ]];
    }
    return self;
}

- (void)ipeReload {
    self.content = [NSArray arrayWithArray:iEntries];
    self.selectionIndexes = [NSIndexSet indexSet];
}

- (void)ipeSetItems:(const std::vector<SorterItem> &)items {
    int iconWidth = 32, iconHeight = 32;
    [iEntries removeAllObjects];
    [iCut removeAllObjects];
    for (size_t i = 0; i < items.size(); ++i) {
	const SorterItem & it = items[i];
	IpeSorterEntry * e = [IpeSorterEntry new];
	e.number = static_cast<NSInteger>(i) + 1;
	e.hasMark = it.hasMark;
	e.marked = it.hasMark && it.mark;
	e.readOnly = self.ipeReadOnly;
	e.label = S2N(it.label);
	e.image = bitmapImage(it.image);
	if (it.image.kind == ImageSpec::EBitmap) {
	    iconWidth = std::max(iconWidth, it.image.width);
	    iconHeight = std::max(iconHeight, it.image.height);
	}
	[iEntries addObject:e];
    }
    // all items have the size of the largest icon (and room for a short label)
    NSSize size = NSMakeSize(std::max(iconWidth + 8, 90), iconHeight + 28);
    self.minItemSize = size;
    self.maxItemSize = size;
    [self ipeReload];
}

// The numbers of the items in their current order, negated if marked.
// For a readonly sorter, the numbers of the selected items.
- (void)ipeFillOrder:(std::vector<int> &)order {
    order.clear();
    if (self.ipeReadOnly) {
	NSIndexSet * sel = [self selectionIndexes];
	for (NSUInteger i = [sel firstIndex]; i != NSNotFound;
	     i = [sel indexGreaterThanIndex:i]) {
	    if (i < [iEntries count]) order.push_back(int(iEntries[i].number));
	}
    } else {
	for (IpeSorterEntry * e in iEntries)
	    order.push_back(int(e.marked ? -e.number : e.number));
    }
}

- (NSMenu *)menuForEvent:(NSEvent *)event {
    if (self.ipeReadOnly) return nil;
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    iMenuHit = -1;
    for (NSUInteger i = 0; i < [iEntries count]; ++i) {
	NSCollectionViewItem * item = [self itemAtIndex:i];
	if (item && NSPointInRect(p, [[item view] frame])) {
	    iMenuHit = i;
	    break;
	}
    }
    // the item under the pointer is selected, unless it was already
    if (iMenuHit >= 0 && ![[self selectionIndexes] containsIndex:iMenuHit])
	self.selectionIndexes = [NSIndexSet indexSetWithIndex:iMenuHit];

    NSIndexSet * sel = [self selectionIndexes];
    BOOL canMark = NO, canUnmark = NO;
    for (NSUInteger i = [sel firstIndex]; i != NSNotFound;
	 i = [sel indexGreaterThanIndex:i]) {
	if (i >= [iEntries count] || !iEntries[i].hasMark) continue;
	if (iEntries[i].marked)
	    canUnmark = YES;
	else
	    canMark = YES;
    }
    NSMenu * menu = [[NSMenu alloc] initWithTitle:@""];
    if ([sel count] > 0) {
	[self ipeAddMenuItem:@"Delete" action:@selector(ipeDelete:) to:menu];
	[self ipeAddMenuItem:@"Cut" action:@selector(ipeCut:) to:menu];
	if (canMark) [self ipeAddMenuItem:@"Mark" action:@selector(ipeMark:) to:menu];
	if (canUnmark)
	    [self ipeAddMenuItem:@"Unmark" action:@selector(ipeUnmark:) to:menu];
    }
    if ([iCut count] > 0)
	[self ipeAddMenuItem:@"Insert" action:@selector(ipeInsert:) to:menu];
    return [menu numberOfItems] > 0 ? menu : nil;
}

- (void)ipeAddMenuItem:(NSString *)title action:(SEL)action to:(NSMenu *)menu {
    NSMenuItem * item = [menu addItemWithTitle:title action:action keyEquivalent:@""];
    item.target = self;
}

- (void)ipeDelete:(id)sender {
    [iEntries removeObjectsAtIndexes:[self selectionIndexes]];
    [self ipeReload];
}

- (void)ipeCut:(id)sender {
    NSIndexSet * sel = [self selectionIndexes];
    [iCut setArray:[iEntries objectsAtIndexes:sel]];
    [iEntries removeObjectsAtIndexes:sel];
    [self ipeReload];
}

// the cut items go before the item that was clicked, or to the end
- (void)ipeInsert:(id)sender {
    NSUInteger pos = (iMenuHit >= 0) ? NSUInteger(iMenuHit) : [iEntries count];
    NSIndexSet * at =
	[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(pos, [iCut count])];
    [iEntries insertObjects:iCut atIndexes:at];
    [iCut removeAllObjects];
    [self ipeReload];
    self.selectionIndexes = at;
}

- (void)ipeSetMarks:(BOOL)mark {
    NSIndexSet * sel = [self selectionIndexes];
    for (NSUInteger i = [sel firstIndex]; i != NSNotFound;
	 i = [sel indexGreaterThanIndex:i]) {
	if (i >= [iEntries count] || !iEntries[i].hasMark) continue;
	iEntries[i].marked = mark;
	[[self itemAtIndex:i] setRepresentedObject:iEntries[i]];
    }
}

- (void)ipeMark:(id)sender {
    [self ipeSetMarks:YES];
}

- (void)ipeUnmark:(id)sender {
    [self ipeSetMarks:NO];
}

- (BOOL)collectionView:(NSCollectionView *)cv
    canDragItemsAtIndexes:(NSIndexSet *)indexes
		withEvent:(NSEvent *)event {
    return !self.ipeReadOnly;
}

- (BOOL)collectionView:(NSCollectionView *)cv
    writeItemsAtIndexes:(NSIndexSet *)indexes
	   toPasteboard:(NSPasteboard *)pasteboard {
    iDragIndexes = [indexes copy];
    [pasteboard setData:[@"ipe" dataUsingEncoding:NSUTF8StringEncoding]
		forType:IpeSorterDragType];
    return YES;
}

- (NSDragOperation)collectionView:(NSCollectionView *)cv
		     validateDrop:(id<NSDraggingInfo>)draggingInfo
		    proposedIndex:(NSInteger *)proposedDropIndex
		    dropOperation:(NSCollectionViewDropOperation *)proposedDropOperation {
    if (self.ipeReadOnly || !iDragIndexes) return NSDragOperationNone;
    *proposedDropOperation = NSCollectionViewDropBefore;
    return NSDragOperationMove;
}

- (BOOL)collectionView:(NSCollectionView *)cv
	    acceptDrop:(id<NSDraggingInfo>)draggingInfo
		 index:(NSInteger)index
	 dropOperation:(NSCollectionViewDropOperation)dropOperation {
    if (self.ipeReadOnly || !iDragIndexes) return NO;
    NSIndexSet * moved = iDragIndexes;
    iDragIndexes = nil;
    NSUInteger dest = index - [moved countOfIndexesInRange:NSMakeRange(0, index)];
    NSArray * move = [iEntries objectsAtIndexes:moved];
    [iEntries removeObjectsAtIndexes:moved];
    NSIndexSet * at =
	[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(dest, [move count])];
    [iEntries insertObjects:move atIndexes:at];
    [self ipeReload];
    self.selectionIndexes = at;
    return YES;
}

@end

// --------------------------------------------------------------------

PDialog::PDialog(lua_State * L0, WINID parent, const char * caption,
		 const char * language)
    : Dialog(L0, parent, caption, language) {
    //
}

PDialog::~PDialog() {
    //
}

void PDialog::acceptDialog(lua_State * L) {
    int accept = lua_toboolean(L, 2);
    [NSApp stopModalWithCode:accept];
    [iPanel close];
}

void PDialog::itemAction(int idx) {
    SElement & m = iElements[idx];
    if (m.flags & EAccept) {
	[NSApp stopModalWithCode:true];
	[iPanel close];
    } else if (m.flags & EReject) {
	[NSApp stopModalWithCode:false];
	[iPanel close];
    } else if (m.lua_method != LUA_NOREF)
	callLua(m.lua_method);
}

// Selecting the row triggers tableViewSelectionDidChange, which calls Lua.
bool PDialog::moveListSelection(int delta) {
    for (int i = 0; i < int(iElements.size()); ++i) {
	if (iElements[i].type != EList || iElements[i].row < 0) continue;
	NSTableView * tv = (NSTableView *)iViews[i];
	NSInteger n = [tv numberOfRows];
	if (n <= 0) return false;
	NSInteger cur = std::max<NSInteger>([tv selectedRow], 0);
	NSInteger next = std::max<NSInteger>(0, std::min(n - 1, cur + delta));
	if (next != cur) {
	    [tv selectRowIndexes:[NSIndexSet indexSetWithIndex:next]
		byExtendingSelection:NO];
	    [tv scrollRowToVisible:next];
	}
	return true;
    }
    return false;
}

int PDialog::numberOfRows(int idx) { return iElements[idx].items.size(); }

NSString * PDialog::row(int idx, int row) { return S2N(iElements[idx].items[row]); }

void PDialog::setMapped(lua_State * L, int idx) {
    SElement & m = iElements[idx];
    NSView * ctrl = iViews[idx];
    switch (m.type) {
    case ELabel:
    case EInput: [((NSTextField *)ctrl) setStringValue:S2N(m.text)]; break;
    case ETextEdit: setTextView((NSTextView *)ctrl, m.text); break;
    case EImage: [(IpeDialogImage *)ctrl setSpec:m.image]; break;
    case ESorter: [(IpeSorterView *)ctrl ipeSetItems:m.sorter]; break;
    case ECheckBox: [((NSButton *)ctrl) setState:m.value]; break;
    case EList:
	// listbox gets items directly from items array
	[((NSTableView *)ctrl) reloadData];
	[((NSTableView *)ctrl) selectRowIndexes:[NSIndexSet indexSetWithIndex:m.value]
			   byExtendingSelection:NO];
	break;
    case ECombo: {
	NSPopUpButton * b = (NSPopUpButton *)ctrl;
	if (lua_istable(L, 3)) fillComboBox(b, idx);
	[b selectItemAtIndex:m.value];
    } break;
    default: break; // EButton
    }
}

void PDialog::retrieveValues() {
    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	NSView * ctrl = iViews[i];
	switch (m.type) {
	case EInput: m.text = N2C([((NSTextField *)ctrl) stringValue]); break;
	case ETextEdit: m.text = getTextView((NSTextView *)ctrl); break;
	case EList: m.value = [((NSTableView *)ctrl) selectedRow]; break;
	case ECombo: m.value = [((NSPopUpButton *)ctrl) indexOfSelectedItem]; break;
	case ECheckBox: m.value = [((NSButton *)ctrl) intValue]; break;
	case ESorter: [(IpeSorterView *)ctrl ipeFillOrder:m.order]; break;
	default: break; // label and button - nothing to do
	}
    }
}

void PDialog::enableItem(int idx, bool value) {
    SElement & m = iElements[idx];
    // images and sorters are not controls, and cannot be disabled
    if (m.type != ETextEdit && m.type != EImage && m.type != ESorter)
	[((NSControl *)iViews[idx]) setEnabled:value];
}

void PDialog::fillComboBox(NSPopUpButton * cb, int idx) {
    SElement & m = iElements[idx];
    [cb removeAllItems];
    for (int k = 0; k < int(m.items.size()); ++k) [cb addItemWithTitle:S2N(m.items[k])];
}

void PDialog::setTextView(NSTextView * tv, const std::string & s) {
    NSAttributedString * n = [[NSAttributedString alloc] initWithString:S2N(s)];
    [[tv textStorage] setAttributedString:n];
    tv.textColor = [NSColor textColor];
}

std::string PDialog::getTextView(NSTextView * tv) {
    return std::string(N2C([[tv textStorage] string]));
}

// --------------------------------------------------------------------

void PDialog::layoutControls() {
    double gap = 12.0;
    int buttonGap = 12.0;

    NSView * content = [iPanel contentView];

    // create row guides
    NSMutableArray * rows = [NSMutableArray arrayWithCapacity:iNoRows];

    for (int i = 0; i < iNoRows; ++i) {
	id g = layoutGuide(content);
	layout(content, g, "l=l");
	layout(g, content, "r=r");
	if (i > 0) layout(g, rows[i - 1], "t=b", gap);
	[rows addObject:g];
    }
    layout(rows[0], content, "t=t", gap);

    // create column guides
    NSMutableArray * cols = [NSMutableArray arrayWithCapacity:iNoCols];

    for (int i = 0; i < iNoCols; ++i) {
	id g = layoutGuide(content);
	layout(content, g, "t=t");
	layout(g, rows[iNoRows - 1], "b=b");
	if (i > 0) layout(g, cols[i - 1], "l=r", gap);
	[cols addObject:g];
    }
    layout(cols[0], content, "l=l", gap);
    layout(content, cols[iNoCols - 1], "r=r", gap);

    // layout buttons
    NSView * lastButton = nil;
    int bcount = 0;
    for (size_t i = 0; i < iElements.size(); ++i) {
	SElement & m = iElements[i];
	if (m.row >= 0) continue;
	NSView * w = iViews[i];
	++bcount;
	layout(w, rows[iNoRows - 1], "t=b", buttonGap);
	layout(content, w, "b=b", buttonGap);
	if (lastButton) {
	    if (bcount == 3)
		layout(lastButton, w, "l>r", buttonGap);
	    else
		layout(lastButton, w, "l=r", buttonGap);
	    layout(w, lastButton, "w=w");
	} else
	    layout(content, w, "r=r", buttonGap);
	lastButton = w;
    }
    if (bcount > 2)
	layout(lastButton, content, "l=l", buttonGap);
    else if (bcount)
	layout(lastButton, content, "l>l", buttonGap);
    else
	// no button added using "addButton".  Probably an old ipelet.
	layout(content, rows[iNoRows - 1], "b=b", gap);

    while (int(iColStretch.size()) < iNoCols) iColStretch.push_back(0);
    while (int(iRowStretch.size()) < iNoRows) iRowStretch.push_back(0);

    // layout rows and columns
    for (size_t i = 0; i < iElements.size(); ++i) {
	SElement & m = iElements[i];
	if (m.row < 0) continue;
	NSView * w = iViews[i];
	if (m.type == EList || m.type == ETextEdit || m.type == ESorter)
	    w = [[w superview] superview];
	layout(w, rows[m.row], "t=t");
	if (m.type == ECombo || m.type == ECheckBox)
	    layout(rows[m.row + m.rowspan - 1], w, "b>b");
	else
	    layout(rows[m.row + m.rowspan - 1], w, "b=b");
	layout(w, cols[m.col], "l=l");
	layout(w, cols[m.col + m.colspan - 1], "r=r");
	if (m.type == EInput || m.type == ETextEdit) layout(w, nil, "w>0", 100);
	if (m.type == EImage || m.type == ESorter) {
	    layout(w, nil, "w>0", m.minWidth);
	    layout(w, nil, "h>0", m.minHeight);
	}
	// does it have stretch?
	BOOL rowStretch = NO;
	for (int r = m.row; r < m.row + m.rowspan; ++r)
	    if (iRowStretch[r] > 0) rowStretch = YES;
	BOOL colStretch = NO;
	for (int c = m.col; c < m.col + m.colspan; ++c)
	    if (iColStretch[c] > 0) colStretch = YES;
	NSLayoutPriority rowpri = rowStretch ? 250.0 : 750.0;
	NSLayoutPriority colpri = colStretch ? 250.0 : 550.0;
	[w setContentHuggingPriority:rowpri
		      forOrientation:NSLayoutConstraintOrientationVertical];
	[w setContentHuggingPriority:colpri
		      forOrientation:NSLayoutConstraintOrientationHorizontal];
    }
    // make columns with stretch 1 equally wide
    NSView * g1 = nil;
    for (int i = 0; i < iNoCols; ++i) {
	if (iColStretch[i] == 1) {
	    if (g1 == nil)
		g1 = cols[i];
	    else
		layout(g1, cols[i], "w=w");
	}
    }
}

// --------------------------------------------------------------------

Dialog::Result PDialog::buildAndRun(int w, int h) {
    NSUInteger style = NSTitledWindowMask | NSResizableWindowMask;
    if (iIgnoreEscapeField >= 0) style |= NSClosableWindowMask;

    iPanel = [[NSPanel alloc] initWithContentRect:NSMakeRect(400., 800., w, h)
					styleMask:style
					  backing:NSBackingStoreBuffered
					    defer:YES];
    [iPanel setTitle:S2N(iCaption)];
    hDialog = iPanel;
    iDelegate = [[IpeDialogDelegate alloc] init];
    [iDelegate setDialog:this];
    [iPanel setDelegate:iDelegate];

    iViews = [NSMutableArray arrayWithCapacity:iElements.size()];

    NSView * content = [iPanel contentView];
    NSView * focusCtrl = nil;

    for (int i = 0; i < int(iElements.size()); ++i) {
	SElement & m = iElements[i];
	NSControl * ctrl = nil;
	NSView * view = nil;
	NSScrollView * scroll = nil;
	if (m.row < 0) {
	    NSButton * b = [[NSButton alloc] initWithFrame:NSZeroRect];
	    [b setButtonType:NSMomentaryPushInButton];
	    ipeui_set_mnemonic(S2N(m.text), b);
	    [b setImagePosition:NSNoImage];
	    [b setBezelStyle:NSRoundedBezelStyle];
	    ctrl = b;
	    if (m.flags & EAccept) {
		[b setKeyEquivalent:@"\r"];
		[b setKeyEquivalentModifierMask:NSCommandKeyMask];
	    }
	} else {
	    switch (m.type) {
	    case ELabel: {
		NSTextField * t = [[NSTextField alloc] initWithFrame:NSZeroRect];
		t.stringValue = S2N(m.text);
		t.bordered = NO;
		t.drawsBackground = NO;
		t.editable = NO;
		ctrl = t;
	    } break;
	    case EButton: {
		NSButton * b = [[NSButton alloc] initWithFrame:NSZeroRect];
		[b setButtonType:NSMomentaryPushInButton];
		ipeui_set_mnemonic(S2N(m.text), b);
		[b setImagePosition:NSNoImage];
		[b setBezelStyle:NSRoundedBezelStyle];
		ctrl = b;
	    } break;
	    case ECheckBox: {
		NSButton * b = [[NSButton alloc] initWithFrame:NSZeroRect];
		[b setButtonType:NSSwitchButton];
		ipeui_set_mnemonic(S2N(m.text), b);
		[b setState:m.value ? NSOnState : NSOffState];
		ctrl = b;
	    } break;
	    case EInput: {
		NSTextField * t = [[NSTextField alloc] initWithFrame:NSZeroRect];
		[t setStringValue:S2N(m.text)];
		if (m.flags & ESelectAll) [t selectText:content];
		[t setDelegate:iDelegate];
		ctrl = t;
	    } break;
	    case EImage: {
		view = [[IpeDialogImage alloc] initWithWidth:m.minWidth
						      height:m.minHeight
							spec:m.image];
	    } break;
	    case ESorter: {
		scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
		IpeSorterView * sv =
		    [[IpeSorterView alloc] initWithReadOnly:!!(m.flags & EReadOnly)];
		[sv ipeSetItems:m.sorter];
		view = sv;
	    } break;
	    case ETextEdit: {
		scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
		NSTextView * tv = [[NSTextView alloc] initWithFrame:NSZeroRect];
		tv.editable = !(m.flags & (EReadOnly | EDisabled));
		tv.richText = NO;
		tv.allowsUndo = YES;
		tv.continuousSpellCheckingEnabled = !!(m.flags & ESpellCheck);
		tv.automaticSpellingCorrectionEnabled = NO;
		tv.automaticQuoteSubstitutionEnabled = NO;
		tv.automaticTextReplacementEnabled = NO;
		tv.automaticDataDetectionEnabled = NO;
		tv.automaticDashSubstitutionEnabled = NO;
		setTextView(tv, m.text);
		view = tv;
	    } break;
	    case ECombo: {
		NSPopUpButton * b =
		    [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
		fillComboBox(b, i);
		[b selectItemAtIndex:m.value];
		ctrl = b;
	    } break;
	    case EList: {
		scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
		NSTableView * tv = [[NSTableView alloc] initWithFrame:NSZeroRect];
		NSTableColumn * column =
		    [[NSTableColumn alloc] initWithIdentifier:@"col1"];
		[tv addTableColumn:column];
		[tv setTag:i]; // needed before adding data source
		[tv setDataSource:iDelegate];
		[tv setHeaderView:nil];
		[tv selectRowIndexes:[NSIndexSet indexSetWithIndex:m.value]
		    byExtendingSelection:NO];
		[tv setDelegate:iDelegate];
		ctrl = tv;
	    } break;
	    default: break;
	    }
	}
	if (ctrl) {
	    ctrl.enabled = !(m.flags & EDisabled);
	    ctrl.tag = i;
	    // inputs report changes through controlTextDidChange, not on Enter
	    if (m.type != EList && m.type != EInput) {
		ctrl.action = @selector(ipeControl:);
		ctrl.target = iDelegate;
	    }
	    view = ctrl;
	}
	if (m.flags & EFocused) focusCtrl = view;
	if (scroll) {
	    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	    [scroll setDocumentView:view];
	    scroll.hasVerticalScroller = YES;
	    // NSScrollView has no border by default, unlike other elements
	    scroll.borderType = NSBezelBorder;
	    layout(scroll, nil, "h>0", 100.0);
	    layout(scroll, nil, "w>0", 160.0);
	}
	[view setContentCompressionResistancePriority:NSLayoutPriorityRequired
				       forOrientation:
					   NSLayoutConstraintOrientationVertical];
	[iViews addObject:view];
	addToLayout(content, scroll ? scroll : view);
    }

    layoutControls();
    // set keyboard focus
    if (focusCtrl) [iPanel makeFirstResponder:focusCtrl];
    // this is such a hack, but it seems to work for Ipe
    NSMenu * editMenu = [[[NSApp mainMenu] itemAtIndex:2] submenu];
    NSMenuItem * undoItem = [editMenu itemAtIndex:0];
    NSMenuItem * redoItem = [editMenu itemAtIndex:1];
    undoItem.action = @selector(undo:);
    redoItem.action = @selector(redo:);
    int result = [NSApp runModalForWindow:iPanel];
    retrieveValues(); // for future reference
    undoItem.action = @selector(ipeMenuAction:);
    redoItem.action = @selector(ipeMenuAction:);
    iPanel = nil;
    return result ? Result::ACCEPTED : Result::CLOSED;
}

// --------------------------------------------------------------------

static int dialog_constructor(lua_State * L) {
    WINID parent = check_winid(L, 1);
    const char * s = luaL_checkstring(L, 2);
    const char * language = "";
    if (lua_isstring(L, 3)) language = luaL_checkstring(L, 3);

    Dialog ** dlg = (Dialog **)lua_newuserdata(L, sizeof(Dialog *));
    *dlg = nullptr;
    luaL_getmetatable(L, "Ipe.dialog");
    lua_setmetatable(L, -2);
    *dlg = new PDialog(L, parent, s, language);
    return 1;
}

// --------------------------------------------------------------------

class PMenu;

@interface IpePopupMenuItem : NSMenuItem

@property NSString * ipeName;        // name of single item
@property int ipeSubmenuIndex;       // index of item in submenu
@property NSString * ipeSubmenuName; // name of item in submenu
@property PMenu * ipeMenu;

- (void)ipePopupAction:(id)sender;

@end

class PMenu : public Menu {
public:
    PMenu();
    virtual int add(lua_State * L);
    virtual int execute(lua_State * L);
    void itemSelected(IpePopupMenuItem * item);

private:
    NSMenu * iMenu;
    IpePopupMenuItem __unsafe_unretained * iSelected;
};

// --------------------------------------------------------------------

@implementation IpePopupMenuItem

- (void)ipePopupAction:(id)sender {
    IpePopupMenuItem * item = (IpePopupMenuItem *)sender;
    self.ipeMenu->itemSelected(item);
}

@end

// --------------------------------------------------------------------

PMenu::PMenu() { iMenu = [[NSMenu alloc] init]; }

void PMenu::itemSelected(IpePopupMenuItem * item) { iSelected = item; }

int PMenu::execute(lua_State * L) {
    NSPoint p = {luaL_checknumber(L, 2), luaL_checknumber(L, 3)};
    iSelected = nil;
    BOOL result = [iMenu popUpMenuPositioningItem:nil atLocation:p inView:nil];
    if (result && iSelected) {
	lua_pushstring(L, N2C(iSelected.ipeName));
	if (iSelected.ipeSubmenuName)
	    lua_pushstring(L, N2C(iSelected.ipeSubmenuName));
	else
	    lua_pushliteral(L, "");
	return 2;
    } else
	return 0;
}

NSImage * colorIcon(double red, double green, double blue, int pixels) {
    NSImage * icon =
	[NSImage imageWithSize:NSMakeSize(pixels, pixels)
		       flipped:NO
		drawingHandler:^(NSRect rect) {
		  CGContextRef myContext = [[NSGraphicsContext currentContext] CGContext];
		  CGContextSetRGBFillColor(myContext, red, green, blue, 1.0);
		  CGContextFillRect(myContext, CGRectMake(0, 0, pixels, pixels));
		  return YES;
		}];
    return icon;
}

int PMenu::add(lua_State * L) {
    const char * name = luaL_checkstring(L, 2);
    const char * title = luaL_checkstring(L, 3);
    if (lua_gettop(L) == 3) {
	IpePopupMenuItem * item =
	    [[IpePopupMenuItem alloc] initWithTitle:ipeui_set_mnemonic(C2N(title), nil)
					     action:@selector(ipePopupAction:)
				      keyEquivalent:@""];
	[item setIpeName:C2N(name)];
	[item setIpeSubmenuIndex:0];
	[item setTarget:item];
	[item setIpeMenu:this];
	[iMenu addItem:item];
    } else {
	luaL_argcheck(L, lua_istable(L, 4), 4, "argument is not a table");
	bool hasmap = !lua_isnoneornil(L, 5) && lua_isfunction(L, 5);
	bool hastable = !hasmap && !lua_isnoneornil(L, 5);
	bool hascolor = !lua_isnoneornil(L, 6) && lua_isfunction(L, 6);
	bool hascheck = !hascolor && !lua_isnoneornil(L, 6);
	if (hastable)
	    luaL_argcheck(L, lua_istable(L, 5), 5, "argument is not a function or table");
	const char * current = nullptr;
	if (hascheck) {
	    luaL_argcheck(L, lua_isstring(L, 6), 6,
			  "argument is not a function or string");
	    current = luaL_checkstring(L, 6);
	}

	NSMenu * sm = [[NSMenu alloc] initWithTitle:C2N(title)];
	if (hascolor) [sm setShowsStateColumn:NO];

	int no = lua_rawlen(L, 4);
	for (int i = 1; i <= no; ++i) {
	    lua_rawgeti(L, 4, i);
	    luaL_argcheck(L, lua_isstring(L, -1), 4, "items must be strings");
	    const char * label = lua_tostring(L, -1);
	    if (hastable) {
		lua_rawgeti(L, 5, i);
		luaL_argcheck(L, lua_isstring(L, -1), 5, "labels must be strings");
	    } else if (hasmap) {
		lua_pushvalue(L, 5);  // function
		lua_pushnumber(L, i); // index
		lua_pushvalue(L, -3); // name
		lua_call(L, 2, 1);    // function returns label
		luaL_argcheck(L, lua_isstring(L, -1), 5,
			      "function does not return string");
	    } else
		lua_pushvalue(L, -1);

	    const char * text = lua_tostring(L, -1);

	    IpePopupMenuItem * item =
		[[IpePopupMenuItem alloc] initWithTitle:ipeui_set_mnemonic(C2N(text), nil)
						 action:@selector(ipePopupAction:)
					  keyEquivalent:@""];
	    [item setIpeName:C2N(name)];
	    [item setIpeSubmenuIndex:i];
	    [item setIpeSubmenuName:C2N(label)];
	    [item setTarget:item];
	    [item setIpeMenu:this];
	    if (hascheck && !strcmp(label, current)) [item setState:NSOnState];
	    [sm addItem:item];
	    if (hascolor) {
		lua_pushvalue(L, 6);  // function
		lua_pushnumber(L, i); // index
		lua_pushvalue(L, -4); // name
		lua_call(L, 2, 3);    // function returns red, green, blue
		double red = luaL_checknumber(L, -3);
		double green = luaL_checknumber(L, -2);
		double blue = luaL_checknumber(L, -1);
		lua_pop(L, 3); // pop result
		NSImage * im = colorIcon(red, green, blue, COLORICONSIZE);
		[item setImage:im];
	    }
	    lua_pop(L, 2); // item, text
	}
	NSMenuItem * mitem =
	    [[NSMenuItem alloc] initWithTitle:ipeui_set_mnemonic(C2N(title), nil)
				       action:nullptr
				keyEquivalent:@""];
	[mitem setSubmenu:sm];
	[iMenu addItem:mitem];
    }
    return 0;
}

// --------------------------------------------------------------------

static int menu_constructor(lua_State * L) {
    // check_winid(L, 1);
    Menu ** m = (Menu **)lua_newuserdata(L, sizeof(Menu *));
    *m = nullptr;
    luaL_getmetatable(L, "Ipe.menu");
    lua_setmetatable(L, -2);
    *m = new PMenu();
    return 1;
}

// --------------------------------------------------------------------

class PTimer;

@interface IpeTimerDelegate : NSObject
@property PTimer * ptimer;

- (void)fired:(NSTimer *)timer;
@end

class PTimer : public Timer {
public:
    PTimer(lua_State * L0, int lua_object, const char * method);
    virtual int setInterval(lua_State * L);
    virtual int active(lua_State * L);
    virtual int start(lua_State * L);
    virtual int stop(lua_State * L);
    void fired();

private:
    int iInterval;
    NSTimer * iTimer;
    IpeTimerDelegate * iDelegate;
};

// --------------------------------------------------------------------

@implementation IpeTimerDelegate
- (void)fired:(NSTimer *)timer {
    self.ptimer->fired();
}
@end

PTimer::PTimer(lua_State * L0, int lua_object, const char * method)
    : Timer(L0, lua_object, method) {
    iTimer = nil;
    iInterval = 0;
    iDelegate = [[IpeTimerDelegate alloc] init];
    iDelegate.ptimer = this;
}

void PTimer::fired() { callLua(); }

int PTimer::setInterval(lua_State * L) {
    iInterval = int(luaL_checkinteger(L, 2));
    return 0;
}

int PTimer::active(lua_State * L) {
    lua_pushboolean(L, iTimer && [iTimer isValid]);
    return 1;
}

int PTimer::start(lua_State * L) {
    if (iTimer) luaL_argerror(L, 1, "timer is already started");
    iTimer = [NSTimer scheduledTimerWithTimeInterval:iInterval / 1000.0
					      target:iDelegate
					    selector:@selector(fired:)
					    userInfo:nil
					     repeats:!iSingleShot];
    return 0;
}

int PTimer::stop(lua_State * L) {
    if (iTimer) [iTimer invalidate];
    iTimer = nil;
    return 0;
}

// --------------------------------------------------------------------

static int timer_constructor(lua_State * L) {
    luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
    const char * method = luaL_checkstring(L, 2);

    Timer ** t = (Timer **)lua_newuserdata(L, sizeof(Timer *));
    *t = nullptr;
    luaL_getmetatable(L, "Ipe.timer");
    lua_setmetatable(L, -2);

    // create a table with weak reference to Lua object
    lua_createtable(L, 1, 1);
    lua_pushliteral(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1);
    int lua_object = luaL_ref(L, LUA_REGISTRYINDEX);
    *t = new PTimer(L, lua_object, method);
    return 1;
}

// --------------------------------------------------------------------

static NSArray<NSString *> * make_filters(const char * s) {
    NSMutableArray<NSString *> * exts = [NSMutableArray arrayWithCapacity:10];
    const char * p = s;
    while (*p) {
	p += 2; // skip *.
	const char * q = p;
	while (*q && *q != ';') ++q;
	NSString * ext =
	    [[NSString alloc] initWithBytes:p length:q - p encoding:NSUTF8StringEncoding];
	[exts addObject:ext];
	if (*q == ';') ++q;
	p = q;
    }
    return exts;
}

@interface IpeFileDialogHelper : NSObject

@property(weak) NSSavePanel * panel;
@property NSPopUpButton * fileType;
@property NSMutableArray * filters;

- (instancetype)initFor:(NSSavePanel *)panel;
- (void)setupWithLua:(lua_State *)L;
- (void)changeFileType:(id)sender;
@end

@implementation IpeFileDialogHelper

- (instancetype)initFor:(NSSavePanel *)panel {
    self = [super init];
    if (self) { _panel = panel; }
    return self;
}

- (void)changeFileType:(id)sender {
    [self setFilter:[self.fileType indexOfSelectedItem]];
}

- (void)setupWithLua:(lua_State *)L {
    self.panel.message = C2N(luaL_checkstring(L, 3));
    if (!lua_isnoneornil(L, 5)) {
	const char * dir = luaL_checkstring(L, 5);
	self.panel.directoryURL = [NSURL fileURLWithPath:C2N(dir) isDirectory:YES];
    }
    if (!lua_isnoneornil(L, 6)) {
	auto url = [NSURL fileURLWithPath:C2N(luaL_checkstring(L, 6))];
	self.panel.nameFieldStringValue = [url lastPathComponent];
    }

    self.panel.canCreateDirectories = YES;
    self.panel.showsTagField = NO;
    self.panel.canSelectHiddenExtension = YES;

    self.fileType = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0., 0., 200.0, 40.0)
					       pullsDown:NO];

    self.fileType.target = self;
    self.fileType.action = @selector(changeFileType:);
    self.panel.accessoryView = self.fileType;

    if (!lua_istable(L, 4)) luaL_argerror(L, 4, "table expected for filters");
    int nFilters = lua_rawlen(L, 4);
    self.filters = [NSMutableArray arrayWithCapacity:nFilters / 2];
    for (int i = 1; i <= nFilters; ++i) {
	lua_rawgeti(L, 4, i);
	luaL_argcheck(L, lua_isstring(L, -1), 4, "filter entry is not a string");
	const char * s = lua_tostring(L, -1);
	if (i % 2 == 1)
	    [self.fileType addItemWithTitle:C2N(s)];
	else
	    [self.filters addObject:make_filters(s)];
	lua_pop(L, 1); // element i
    }

    int selected = 0;
    if (!lua_isnoneornil(L, 7)) selected = luaL_checkinteger(L, 7) - 1;
    [self.fileType selectItemAtIndex:selected];
    [self setFilter:selected];
}

- (void)setFilter:(int)filterIndex {
    if ([self.filters[filterIndex][0] isEqualToString:@"*"])
	self.panel.allowedFileTypes = nil;
    else
	self.panel.allowedFileTypes = self.filters[filterIndex];
}

@end

static int ipeui_fileDialog(lua_State * L) {
    static const char * const typenames[] = {"open", "save", nullptr};
    // not needed: NSWindow *win = check_winid(L, 1);
    int type = luaL_checkoption(L, 2, nullptr, typenames);
    if (type == 0) { // open file
	NSOpenPanel * panel = [NSOpenPanel openPanel];
	IpeFileDialogHelper * helper = [[IpeFileDialogHelper alloc] initFor:panel];
	[helper setupWithLua:L];
	if ([panel runModal] == NSFileHandlingPanelOKButton) {
	    lua_pushstring(L, N2C([[panel URLs][0] path]));
	    lua_pushinteger(L, [helper.fileType indexOfSelectedItem] + 1);
	    return 2;
	}
    } else { // save file
	NSSavePanel * panel = [NSSavePanel savePanel];
	IpeFileDialogHelper * helper = [[IpeFileDialogHelper alloc] initFor:panel];
	[helper setupWithLua:L];
	if ([panel runModal] == NSFileHandlingPanelOKButton) {
	    lua_pushstring(L, N2C([[panel URL] path]));
	    lua_pushinteger(L, [helper.fileType indexOfSelectedItem] + 1);
	    return 2;
	}
    }
    return 0;
}

// --------------------------------------------------------------------

static int ipeui_getColor(lua_State * L) {
    NSWindow * win = check_winid(L, 1);
    NSColorPanel * panel = [NSColorPanel sharedColorPanel];
    if ([panel isVisible]) {
	NSColor * rgb = [panel color];
	lua_pushnumber(L, rgb.redComponent);
	lua_pushnumber(L, rgb.greenComponent);
	lua_pushnumber(L, rgb.blueComponent);
	return 3;
    } else {
	const char * title = luaL_checkstring(L, 2);
	double r = luaL_checknumber(L, 3);
	double g = luaL_checknumber(L, 4);
	double b = luaL_checknumber(L, 5);

	NSColor * rgb = [NSColor colorWithRed:r green:g blue:b alpha:1.0];
	[panel setColor:rgb];
	[panel setTitle:C2N(title)];
	[panel orderFront:win];
	return 0;
    }
}

// --------------------------------------------------------------------

static int ipeui_messageBox(lua_State * L) {
    static const char * const options[] = {"none",     "warning",  "information",
					   "question", "critical", nullptr};
    static const char * const buttontype[] = {
	"ok", "okcancel", "yesnocancel", "discardcancel", "savediscardcancel", nullptr};

    NSWindow * win = check_winid(L, 1);
    int type = luaL_checkoption(L, 2, "none", options);
    const char * text = luaL_checkstring(L, 3);
    const char * details = nullptr;
    if (!lua_isnoneornil(L, 4)) details = luaL_checkstring(L, 4);
    int buttons = 0;
    if (lua_isnumber(L, 5))
	buttons = (int)luaL_checkinteger(L, 5);
    else if (!lua_isnoneornil(L, 5))
	buttons = luaL_checkoption(L, 5, nullptr, buttontype);

    (void)win;
    NSAlert * alert = [[NSAlert alloc] init];
    [alert setMessageText:C2N(text)];
    if (details) [alert setInformativeText:C2N(details)];

    NSAlertStyle astyle = NSInformationalAlertStyle;
    switch (type) {
    case 0:
    default: break;
    case 1: astyle = NSWarningAlertStyle; break;
    case 2: break;
    case 3:
	// Question doesn't seem to exist on Cocoa
	break;
    case 4: astyle = NSCriticalAlertStyle; break;
    }

    [alert setAlertStyle:astyle];

    switch (buttons) {
    case 0:
    default: break;
    case 1:
	[alert addButtonWithTitle:@"Ok"];
	[alert addButtonWithTitle:@"Cancel"];
	break;
    case 2:
	[alert addButtonWithTitle:@"Yes"];
	[alert addButtonWithTitle:@"No"];
	[alert addButtonWithTitle:@"Cancel"];
	break;
    case 3:
	[alert addButtonWithTitle:@"Discard"];
	[alert addButtonWithTitle:@"Cancel"];
	break;
    case 4:
	[alert addButtonWithTitle:@"Save"];
	[alert addButtonWithTitle:@"Discard"];
	[alert addButtonWithTitle:@"Cancel"];
	break;
    }
    switch ([alert runModal]) {
    case NSAlertFirstButtonReturn: lua_pushnumber(L, 1); break;
    case NSAlertSecondButtonReturn:
	if (buttons == 2 || buttons == 4)
	    lua_pushnumber(L, 0);
	else
	    lua_pushnumber(L, -1);
	break;
    case NSAlertThirdButtonReturn:
    default: lua_pushnumber(L, -1); break;
    }
    return 1;
}

// --------------------------------------------------------------------

static int ipeui_currentDateTime(lua_State * L) {
    NSDate * now = [NSDate date];
    NSCalendar * gregorian =
	[[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
    unsigned unitFlags = NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay
			 | NSCalendarUnitHour | NSCalendarUnitMinute
			 | NSCalendarUnitSecond;
    NSDateComponents * st = [gregorian components:unitFlags fromDate:now];
    char buf[16];
    sprintf(buf, "%04ld%02ld%02ld%02ld%02ld%02ld", long(st.year), long(st.month),
	    long(st.day), long(st.hour), long(st.minute), long(st.second));
    lua_pushstring(L, buf);
    return 1;
}

static int ipeui_startBrowser(lua_State * L) {
    NSString * urls = C2N(luaL_checkstring(L, 1));
    NSURL * url = nil;
    if ([urls hasPrefix:@"file:///"]) {
	url = [NSURL fileURLWithPath:[urls substringFromIndex:8] isDirectory:NO];
    } else {
	// TODO: percent-escape chars not allowed in URLS (such as spaces)
	// (not urgent since we only use this for the manual on the file system
	url = [NSURL URLWithString:urls];
    }
    int res = [[NSWorkspace sharedWorkspace] openURL:url];
    lua_pushboolean(L, res);
    return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipeui_functions[] = {
    {"Dialog", dialog_constructor},
    {"Timer", timer_constructor},
    {"Menu", menu_constructor},
    {"fileDialog", ipeui_fileDialog},
    {"getColor", ipeui_getColor},
    {"messageBox", ipeui_messageBox},
    {"currentDateTime", ipeui_currentDateTime},
    {"startBrowser", ipeui_startBrowser},
    {nullptr, nullptr}};

// --------------------------------------------------------------------

int luaopen_ipeui(lua_State * L) {
    luaL_newlib(L, ipeui_functions);
    lua_setglobal(L, "ipeui");
    luaopen_ipeui_common(L);
    return 0;
}

// --------------------------------------------------------------------
