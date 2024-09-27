# --------------------------------------------------------------------
# Makefile for Iperender
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/iperender
include ../common.mak

TARGET = $(call exe_target,iperender)

CPPFLAGS += -I../include $(CAIRO_CFLAGS) -I../ipecairo
ifdef IPENODEJS
LDFLAGS += -lnoderawfs.js -lnodefs.js --pre-js $(NODEJSPRE)
endif
LIBS += -L$(buildlib) -lipecairo -lipe $(CAIRO_LIBS)

all: $(TARGET)

sources	= iperender.cpp

$(TARGET): $(objects) $(NODEJSPRE)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $(objects) $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_PROGRAMS) iperender-par $(INSTALL_ROOT)$(IPEBINDIR)

# --------------------------------------------------------------------
