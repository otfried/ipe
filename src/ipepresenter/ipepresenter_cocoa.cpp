// -*- objc -*-
// --------------------------------------------------------------------
// IpePresenter with Cocoa UI
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

#include "ipepresenter.h"
#include "ipepdfview_cocoa.h"
#include "ipeselector_cocoa.h"

#import <Cocoa/Cocoa.h>

using ipe::String;

inline String N2I(NSString *aStr) { return String(aStr.UTF8String); }
inline NSString *I2N(String s) {return [NSString stringWithUTF8String:s.z()];}

// --------------------------------------------------------------------

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSSplitViewDelegate>
@end

@interface IpePdfSelectorProvider : IpeSelectorProvider

@property ipe::PdfFile *pdf;
@property ipe::PdfThumbnail *pdfThumb;
@property NSMutableArray<NSString *> *pdfLabels;

- (int) count;
- (NSString *) title:(int) index;
- (ipe::Buffer) renderImage:(int) index;
@end

// --------------------------------------------------------------------

@implementation IpePdfSelectorProvider

- (int) count
{
  return self.pdf->countPages();
}

- (NSString *) title:(int) index
{
  return self.pdfLabels[index];
}

- (ipe::Buffer) renderImage:(int) index
{
  return self.pdfThumb->render(self.pdf->page(index));
}
@end

// --------------------------------------------------------------------

class AppUi : public Presenter {
public:
  AppUi();
  bool load(NSString *aFname);
  void setPdf();
  void setView();
  void fitBoxAll();
  void setTime();
  void timerElapsed();
  void selectPage();
  virtual void showType3Warning(const char *s) override;
  virtual void browseLaunch(bool launch, String dest) override;

public:
  int iTime;
  bool iCountDown;
  bool iCountTime;

  NSWindow *iWindow;
  NSWindow *iScreenWindow;

  NSSplitView *iContent;
  NSSplitView *iRightSide;
  NSStackView *iTopRight;

  NSTextField *iClock;
  NSTextView *iNotesView;
  NSScrollView *iNotes;

  IpePdfView *iCurrent;
  IpePdfView *iNext;
  IpePdfView *iScreen;

  IpePdfSelectorProvider *iProvider;
};

AppUi::AppUi() : iTime{0}, iCountDown{false}, iCountTime{false},
  iCurrent{nil}, iNext{nil}, iScreen{nil}, iProvider{nil}
{
  NSRect contentRect = NSMakeRect(335., 390., 800., 600.);
  NSRect mainRect = NSMakeRect(0., 0., 200., 100.);
  NSRect subRect = NSMakeRect(0., 0., 100., 100.);
  NSRect clockRect = NSMakeRect(0., 0., 100., 30.);

  iWindow = [[NSWindow alloc]
	       initWithContentRect:contentRect
			 styleMask:(NSTitledWindowMask|
				    NSClosableWindowMask|
				    NSResizableWindowMask|
				    NSMiniaturizableWindowMask)
			   backing:NSBackingStoreBuffered
			     defer:YES];

  iContent = [[NSSplitView alloc] initWithFrame:subRect];
  iContent.vertical = YES;

  iRightSide = [[NSSplitView alloc] initWithFrame:subRect];
  iRightSide.vertical = NO;

  iCurrent = [[IpePdfView alloc] initWithFrame:mainRect];
  iNext = [[IpePdfView alloc] initWithFrame:subRect];

  iClock = [[NSTextField alloc] initWithFrame:clockRect];
  iClock.bordered= NO;
  iClock.drawsBackground = NO;
  iClock.editable = NO;
  iClock.font = [NSFont labelFontOfSize:24.0];
  iClock.alignment = NSCenterTextAlignment;
  iClock.usesSingleLineMode = YES;

  iNotes = [[NSScrollView alloc] initWithFrame:subRect];
  iNotesView = [[NSTextView alloc] initWithFrame:subRect];
  iNotesView.editable = NO;
  iNotesView.richText = NO;
  [iNotesView setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
  [iNotes setDocumentView:iNotesView];
  iNotes.hasVerticalScroller = YES;

  iTopRight = [NSStackView stackViewWithViews:@[iClock, iNotes]];
  iTopRight.orientation = NSUserInterfaceLayoutOrientationVertical;

  [iContent addSubview:iCurrent];
  [iContent addSubview:iRightSide];
  [iRightSide addSubview:iTopRight];
  [iRightSide addSubview:iNext];
  [iContent adjustSubviews];
  [iRightSide adjustSubviews];
  double split = (0.6 * [iRightSide minPossiblePositionOfDividerAtIndex:0] +
		  0.4 * [iRightSide maxPossiblePositionOfDividerAtIndex:0]);
  [iRightSide setPosition:split ofDividerAtIndex:0];

  [iWindow setContentView:iContent];

  iScreenWindow = [[NSWindow alloc]
		    initWithContentRect:contentRect
			      styleMask:(NSTitledWindowMask|
					 NSResizableWindowMask|
					 NSMiniaturizableWindowMask)
				backing:NSBackingStoreBuffered
				  defer:YES];
  iScreen = [[IpePdfView alloc] initWithFrame:subRect];
  iScreen.pdfView->setBackground(ipe::Color(0, 0, 0));
  [iScreenWindow setContentView:iScreen];
}

bool AppUi::load(NSString *aFname)
{
  bool result = Presenter::load(N2I(aFname).z());
  if (result) {
    setPdf();
    setView();
    fitBoxAll();
    iProvider = nil;
  }
  return result;
}

void AppUi::setPdf()
{
  iCurrent.pdfView->setPdf(iPdf.get(), iFonts.get());
  iNext.pdfView->setPdf(iPdf.get(), iFonts.get());
  iScreen.pdfView->setPdf(iPdf.get(), iFonts.get());
}

void AppUi::setView()
{
  setViewPage(iScreen.pdfView, iPdfPageNo);
  setViewPage(iCurrent.pdfView, iPdfPageNo);
  setViewPage(iNext.pdfView, iPdfPageNo < iPdf->countPages() - 1 ? iPdfPageNo + 1 : iPdfPageNo);

  [iWindow setTitle:I2N(currentLabel())];
  NSAttributedString *n = [[NSAttributedString alloc]
			    initWithString:I2N(iAnnotations[iPdfPageNo])];
  [[iNotesView textStorage] setAttributedString:n];
  iNotesView.textColor = [NSColor textColor];
  iNotesView.font = [NSFont labelFontOfSize:14.0];
  setTime();
}

void AppUi::fitBoxAll()
{
  if (!iPdf)
    return;
  fitBox(mediaBox(-1), iCurrent.pdfView);
  fitBox(mediaBox(-2), iNext.pdfView);
  fitBox(mediaBox(-1), iScreen.pdfView);
}

void AppUi::setTime()
{
  [iClock setStringValue:[NSString stringWithFormat:@"%d:%02d:%02d",
				   iTime / 3600, (iTime / 60) % 60, iTime % 60]];
  [iRightSide adjustSubviews];
}

void AppUi::timerElapsed()
{
  if (iCountTime) {
    if (iCountDown) {
      if (iTime > 0)
	--iTime;
    } else
      ++iTime;
    setTime();
  }
}

void AppUi::selectPage()
{
  constexpr int iconWidth = 250;

  if (!iProvider) {
    iProvider = [IpePdfSelectorProvider new];
    iProvider.pdf = iPdf.get();
    iProvider.pdfThumb = new ipe::PdfThumbnail(iPdf.get(), iconWidth);

    iProvider.pdfLabels = [NSMutableArray new];
    for (int i = 0; i < iPdf->countPages(); ++i)
      [iProvider.pdfLabels addObject:I2N(pageLabel(i))];

    NSSize tnSize;
    tnSize.width = iProvider.pdfThumb->width() / 2.0;
    tnSize.height = iProvider.pdfThumb->height() / 2.0;
    iProvider.tnSize = tnSize;
  }

  const char *title = "IpePresenter: Select page";
  int width = 800;
  int height = 600;

  int sel = showPageSelectDialog(width, height, title, iProvider, iPdfPageNo);
  if (sel >= 0) {
    iPdfPageNo = sel;
    setView();
  }
}

void AppUi::showType3Warning(const char *s)
{
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = [NSString stringWithUTF8String:s];
  [alert addButtonWithTitle:@"Ok"];
  [alert runModal];
}

void AppUi::browseLaunch(bool launch, String dest)
{
  NSString *urls = I2N(dest);
  if (launch) {
    NSURL *url = [NSURL fileURLWithPath:urls isDirectory:NO];
    [[NSWorkspace sharedWorkspace] openURL:url];
  } else {
    NSURL *url = [NSURL URLWithString:urls];
    [[NSWorkspace sharedWorkspace] openURL:url];
  }
}

// --------------------------------------------------------------------

static void setItemShortcut(NSMenu *menu, int index, unichar code)
{
  NSMenuItem *item = [menu itemAtIndex:index];
  item.keyEquivalent = [NSString stringWithFormat:@"%C", code];
  item.keyEquivalentModifierMask = 0;
}

// --------------------------------------------------------------------

static const char * const about_text =
  "IpePresenter %d.%d.%d\n\n"
  "Copyright (c) 2020-2023 Otfried Cheong\n\n"
  "A presentation tool for giving PDF presentations "
  "created in Ipe or using beamer.\n"
  "Originally invented by Dmitriy Morozov, "
  "IpePresenter is now developed together with Ipe and released under the GNU Public License.\n"
  "See http://ipepresenter.otfried.org for details.\n\n"
  "If you are an IpePresenter fan and want to show others, have a look at the "
  "Ipe T-shirts (www.shirtee.com/en/store/ipe).\n\n"
  "Platinum and gold sponsors\n\n"
  " * Hee-Kap Ahn\n"
  " * Günter Rote\n"
  " * SCALGO\n"
  " * Martin Ziegler\n\n"
  "If you enjoy IpePresenter, feel free to treat the author on a cup of coffee at https://ko-fi.com/ipe7author.\n\n"
  "You can also become a member of the exclusive community of "
  "Ipe patrons (http://patreon.com/otfried). "
  "For the price of a cup of coffee per month you can make a meaningful contribution "
  "to the continuing development of IpePresenter and Ipe.";

// --------------------------------------------------------------------

@implementation AppDelegate  {
  AppUi *ui;
}

- (instancetype) init {
  self = [super init];
  if (self)
    ui = new AppUi();
  return self;
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *) app {
  return YES;
}

- (void) applicationWillFinishLaunching:(NSNotification *) notification {
  [ui->iWindow setDelegate:(id<NSWindowDelegate>) self];
  [ui->iScreenWindow setDelegate:(id<NSWindowDelegate>) self];
  [ui->iContent setDelegate:(id<NSSplitViewDelegate>) self];
  [ui->iRightSide setDelegate:(id<NSSplitViewDelegate>) self];
}

- (void) applicationDidFinishLaunching:(NSNotification *) aNotification {
  NSMenu *menu = [NSApp menu];
  int i = [menu indexOfItemWithTag:13];
  NSMenu *navi = [menu itemAtIndex:i].submenu;
  setItemShortcut(navi, 0, NSRightArrowFunctionKey);
  setItemShortcut(navi, 1, NSDownArrowFunctionKey);
  setItemShortcut(navi, 2, NSLeftArrowFunctionKey);
  setItemShortcut(navi, 3, NSUpArrowFunctionKey);

  [ui->iWindow makeKeyAndOrderFront:self];
  ui->fitBoxAll();

  [NSTimer scheduledTimerWithTimeInterval:1.0
				   target:self
				 selector:@selector(timerFired:)
				 userInfo:nil
				  repeats:YES];

  if (ui->iFileName.empty())
    [self openDocument:self];
}

- (BOOL) application:(NSApplication *) app openFile:(NSString *) filename
{
  return ui->load(filename);
}

- (void) windowDidEndLiveResize:(NSNotification *) notification
{
  ui->fitBoxAll();
}

- (void) splitViewDidResizeSubviews:(NSNotification *) notification
{
  ui->fitBoxAll();
}

- (void) windowDidExitFullScreen:(NSNotification *) notification
{
  ui->fitBoxAll();
}

- (BOOL) validateMenuItem:(NSMenuItem *) item
{
  if (item.action == @selector(countDown:)) {
    item.state = ui->iCountDown ? NSOnState : NSOffState;
  } else if (item.action == @selector(countTime:)) {
    item.state = ui->iCountTime ? NSOnState : NSOffState;
  } else if (item.action == @selector(blackout:)) {
    item.state = ui->iScreen.pdfView->blackout() ? NSOnState : NSOffState;
  }
  return YES;
}

- (void) pdfViewMouseButton:(NSEvent *) event atLocation:(NSArray<NSNumber *> *) pos
{
  ipe::Vector p([pos[0] doubleValue], [pos[1] doubleValue]);
  const ipe::PdfDict *action = ui->findLink(p);
  if (action) {
    ui->interpretAction(action);
    ui->setView();
  } else {
    int button = event.buttonNumber;
    if (button == 0)
      [self nextView:event.window];
    else if (button == 1)
      [self previousView:event.window];
  }
}

- (BOOL) windowShouldClose:(id) sender
{
  if (sender == ui->iWindow)
    [ui->iScreenWindow close]; // also close presentation window
  return true;
}

- (void) timerFired:(NSTimer *) timer
{
  ui->timerElapsed();
}

// --------------------------------------------------------------------

- (void) openDocument:(id) sender {
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel beginSheetModalForWindow:ui->iWindow completionHandler:
    ^(NSInteger result) {
      if (result == NSFileHandlingPanelOKButton) {
	NSURL *url = [[panel URLs] objectAtIndex:0];
	if ([url isFileURL])
	  ui->load([url path]);
      }
      // canceled or failed to load and we didn't have a document before
      if (ui->iFileName.empty())
	  [NSApp terminate:self];
    }
   ];
}

- (void) showPresentation:(id) sender
{
  [ui->iScreenWindow setIsVisible:true];
}

- (void) blackout:(id) sender
{
  ui->iScreen.pdfView->setBlackout(!ui->iScreen.pdfView->blackout());
  ui->iScreen.pdfView->updatePdf();
}

- (void) setTime:(id) sender
{
  NSString *input = [self input:@"Enter time in minutes:" defaultValue:@""];
  if (input) {
    ipe::Lex lex(N2I(input));
    int minutes = lex.getInt();
    ui->iTime = 60 * minutes;
    ui->setTime();
  }
}

- (void) resetTime:(id) sender
{
  ui->iTime = 0;
  ui->setTime();
}

- (void) countDown:(id) sender
{
  ui->iCountDown = !ui->iCountDown;
}

- (void) countTime:(id) sender
{
  ui->iCountTime = !ui->iCountTime;
}

- (void) nextView:(id) sender
{
  ui->nextView(+1);
  ui->setView();
}

- (void) previousView:(id) sender
{
  ui->nextView(-1);
  ui->setView();
}

- (void) nextPage:(id) sender
{
  ui->nextPage(+1);
  ui->setView();
}

- (void) previousPage:(id) sender
{
  ui->nextPage(-1);
  ui->setView();
}

- (void) jumpTo:(id) sender
{
  NSString *input = [self input:@"Enter page label:" defaultValue:@""];
  if (input) {
    ui->jumpToPage(N2I(input));
    ui->setView();
  }
}

- (void) selectPage:(id) sender
{
  ui->selectPage();
}

- (NSString *) input: (NSString *) prompt defaultValue: (NSString *)defaultValue
{
  NSAlert *alert = [[NSAlert alloc] init];
  alert.messageText = prompt;
  [alert addButtonWithTitle:@"Ok"];
  [alert addButtonWithTitle:@"Cancel"];

  NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
  input.stringValue = defaultValue;
  alert.accessoryView = input;

  NSInteger button = [alert runModal];
  if (button == NSAlertFirstButtonReturn) {
    [input validateEditing];
    return [input stringValue];
  } else
    return nil;
}

- (void) aboutIpePresenter:(id) sender
{
  NSString *info = [NSString stringWithFormat:@(about_text),
					  ipe::IPELIB_VERSION / 10000,
					 (ipe::IPELIB_VERSION / 100) % 100,
					  ipe::IPELIB_VERSION % 100];

  NSAlert *alert = [[NSAlert alloc] init];
  [alert setMessageText:@"About IpePresenter"];
  [alert setInformativeText:info];
  [alert setAlertStyle:NSInformationalAlertStyle];
  [alert runModal];
}

@end

// --------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  ipe::Platform::initLib(ipe::IPELIB_VERSION);
  return NSApplicationMain(argc, argv);
}

// --------------------------------------------------------------------
