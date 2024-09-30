# --------------------------------------------------------------------
# Makefile for ipejs - Javascript bindings
# --------------------------------------------------------------------

ifdef IPEWASM

OBJDIR = $(BUILDDIR)/obj/ipejs
include ../common.mak

# TARGET = $(call exe_target,ipejs)
TARGET = ./ipejs.js

# create js/wasm files suitable for a browser
LDFLAGS += -sMODULARIZE=1 -sENVIRONMENT=web -sEXPORT_ES6=1 -lembind

CPPFLAGS += -I../include -I../ipecairo $(CAIRO_CFLAGS) $(FREETYPE_CFLAGS)
LIBS += -L$(buildlib) -lipe -lipecairo $(CAIRO_LIBS) $(FREETYPE_LIBS)

# for use in ipejs.html, embed an Ipe document:
# (use -sSINGLE_FILE=1 to save wasm as base64 inside the js file)
LDFLAGS += -sSINGLE_FILE=1 --embed-file test.ipe
# fur use in ipecanvas.html, also add libipecanvas:
LIBS += -lipecanvas
# for use in ipelatex.html, also need file system and faked Latex run output:
LDFLAGS += -sEXPORTED_RUNTIME_METHODS=FS --embed-file ipetemp.pdf

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
