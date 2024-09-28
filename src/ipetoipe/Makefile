# --------------------------------------------------------------------
# Makefile for Ipetoipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipetoipe
include ../common.mak

TARGET = $(call exe_target,ipetoipe)

CPPFLAGS += -I../include
ifdef IPENODEJS
LDFLAGS += --pre-js $(NODEJSPRE) -lnoderawfs.js -lnodefs.js
endif
LIBS += -L$(buildlib) -lipe

all: $(TARGET)

sources	= ipetoipe.cpp

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

# --------------------------------------------------------------------
