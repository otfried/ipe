# --------------------------------------------------------------------
# Makefile for ipe::Canvas
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipecanvas
include ../common.mak

TARGET = $(call dll_target,ipecanvas)
MAKE_SYMLINKS = $(call dll_symlinks,ipecanvas)
SONAME = $(call soname,ipecanvas)
INSTALL_SYMLINKS = $(call install_symlinks,ipecanvas)

CPPFLAGS += -I../include -I../ipecairo $(CAIRO_CFLAGS) $(UI_CFLAGS)
LIBS += -L$(buildlib) -lipecairo -lipe $(CAIRO_LIBS) $(UI_LIBS)
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= ipetool.cpp ipecanvas.cpp ipepdfview.cpp
cocoa_sources = ipecanvas_cocoa.cpp ipeselector_cocoa.cpp ipepdfview_cocoa.cpp
win_sources = ipecanvas_win.cpp ipeselector_win.cpp ipepdfview_win.cpp
gtk_sources = ipecanvas_gtk.cpp ipeselector_gtk.cpp
qt_sources = ipecanvas_qt.cpp ipeselector_qt.cpp ipepdfview_qt.cpp
moc_headers = ipecanvas_qt.h ipeselector_qt.h ipepdfview_qt.h

$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) $(SONAME) -o $@ $^ $(LIBS)
	$(MAKE_SYMLINKS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_FILES) *.h $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_SYMLINKS)

# --------------------------------------------------------------------
