# --------------------------------------------------------------------
# Makefile for IpePresenter
# --------------------------------------------------------------------

BUNDLEDIR = $(BUILDDIR)/IpePresenter.app/Contents
OBJDIR = $(BUILDDIR)/obj/ipepresenter
include ../common.mak

TARGET = $(call exe_target,ipepresenter)

CPPFLAGS += -I../include -I../ipecanvas -I../ipecairo \
	$(UI_CFLAGS) $(CAIRO_CFLAGS) $(ZLIB_CFLAGS)
LIBS += -L$(buildlib) -lipecanvas -lipecairo -lipe \
	$(UI_LIBS) $(CAIRO_LIBS) $(ZLIB_LIBS)

all: $(TARGET)

sources = ipepresenter.cpp
win_sources = ipepresenter_win.cpp
cocoa_sources = ipepresenter_cocoa.cpp
qt_sources = ipepresenter_qt.cpp timelabel_qt.cpp
moc_headers = ipepresenter_qt.h timelabel_qt.h
gtk_sources = ipepresenter_gtk.cpp

ifdef WIN32
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows

resource = $(OBJDIR)/res.o

$(resource): ipepresenter.rc
	$(WINDRES) -i ipepresenter.rc -o $@ --include-dir=.

else
resource =
endif

ifdef IPEUI_COCOA
nib = $(RESOURCEDIR)/mainmenu.nib

$(nib): mainmenu.xib
	ibtool --compile $(nib) mainmenu.xib

# libraries to link against are here
buildlib = $(BUILDDIR)/Ipe.app/Contents/Frameworks

else
nib = 
endif

$(TARGET): $(objects) $(resource) $(nib)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $(objects) $(resource) $(LIBS)

clean:
	@-rm -f $(objects) $(resource) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)

ifdef IPEUI_COCOA
IPELIBS = $(BUILDDIR)/Ipe.app/Contents/Frameworks
APPLIBS = $(BUILDDIR)/IpePresenter.app/Contents/Frameworks
dll_symlinks = ln -sf lib$1.$(IPEVERS).dylib $(APPLIBS)/lib$1.dylib

app:
	$(INSTALL_DIR) $(RESOURCEDIR)
	$(INSTALL_DIR) $(APPLIBS)
	$(INSTALL_FILES) Info.plist $(BUNDLEDIR)
	$(INSTALL_FILES) $(BUILDDIR)/ipe.icns $(RESOURCEDIR)
	$(INSTALL_PROGRAMS) $(IPELIBS)/libipe.$(IPEVERS).dylib $(APPLIBS)
	$(INSTALL_PROGRAMS) $(IPELIBS)/libipecairo.$(IPEVERS).dylib $(APPLIBS)
	$(INSTALL_PROGRAMS) $(IPELIBS)/libipecanvas.$(IPEVERS).dylib $(APPLIBS)
	$(call dll_symlinks,ipe)
	$(call dll_symlinks,ipecairo)
	$(call dll_symlinks,ipecanvas)

dependencies:
	$(INSTALL_PROGRAMS) $(IPEDEPS)/lib/libpng16.16.dylib $(APPLIBS)
	$(INSTALL_PROGRAMS) $(IPEDEPS)/lib/libpixman-1.0.dylib $(APPLIBS)
	$(INSTALL_PROGRAMS) $(IPEDEPS)/lib/libfreetype.6.dylib $(APPLIBS)
	$(INSTALL_PROGRAMS) $(IPEDEPS)/lib/libcairo.2.dylib $(APPLIBS)

endif

# --------------------------------------------------------------------
