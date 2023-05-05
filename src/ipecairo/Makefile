# --------------------------------------------------------------------
# Makefile for Ipecairo
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipecairo
include ../common.mak

TARGET = $(call dll_target,ipecairo)
MAKE_SYMLINKS = $(call dll_symlinks,ipecairo)
SONAME = $(call soname,ipecairo)
INSTALL_SYMLINKS = $(call install_symlinks,ipecairo)

CPPFLAGS += -I../include $(CAIRO_CFLAGS) $(FREETYPE_CFLAGS)
LIBS += -L$(buildlib) -lipe $(CAIRO_LIBS) $(FREETYPE_LIBS)
ifdef WIN32
LIBS += -lgdiplus
endif
CXXFLAGS += $(DLL_CFLAGS)

all: $(TARGET)

sources	= \
	ipecairopainter.cpp \
	ipefonts.cpp	\
	ipethumbs.cpp

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
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_SYMLINKS)

# --------------------------------------------------------------------
