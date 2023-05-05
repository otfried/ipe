# --------------------------------------------------------------------
# Makefile for Ipetoipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipetoipe
include ../common.mak

TARGET = $(call exe_target,ipetoipe)

CPPFLAGS += -I../include
LIBS += -L$(buildlib) -lipe

all: $(TARGET)

sources	= ipetoipe.cpp

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
