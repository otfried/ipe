# --------------------------------------------------------------------
# Makefile for Ipe6upgrade
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipe6upgrade
include ../common.mak

TARGET = $(call exe_target,ipe6upgrade)

CPPFLAGS += -I../include
LIBS += -L$(buildlib) -lipe

ifdef IPEWASM
LDFLAGS += --pre-js $(NODEJSPRE) -lembind -lnoderawfs.js -lnodefs.js
endif

all: $(TARGET)

sources	= ipe6upgrade.cpp

$(TARGET): $(objects)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)

# --------------------------------------------------------------------
