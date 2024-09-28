# --------------------------------------------------------------------
# Makefile for ipejs - Javascript bindings
# --------------------------------------------------------------------

ifdef IPEWASM

OBJDIR = $(BUILDDIR)/obj/ipejs
include ../common.mak

TARGET = $(call exe_target,ipejs)

# create js/wasm files suitable for a browser
# use -sSINGLE_FILE=1 to save wasm as base64 inside the js file
LDFLAGS += -sMODULARIZE=1 -sENVIRONMENT=web -sEXPORT_ES6=1 -lembind
# for testing, embed an Ipe document
# LDFLAGS += --embed-file tiling1.ipe

CPPFLAGS += -I../include -I../ipecairo $(CAIRO_CFLAGS) $(FREETYPE_CFLAGS)
LIBS += -L$(buildlib) -lipe -lipecairo -lipecanvas $(CAIRO_LIBS) $(FREETYPE_LIBS)

all: $(TARGET)

sources	= ipejs.cpp ipecairojs.cpp

$(TARGET): $(objects) Makefile
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $(objects) $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

endif

# --------------------------------------------------------------------
