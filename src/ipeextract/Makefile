# --------------------------------------------------------------------
# Makefile for Ipeextract
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipeextract
include ../common.mak

TARGET = $(call exe_target,ipeextract)

CPPFLAGS += -I../include
LIBS += -L$(buildlib) -lipe

all: $(TARGET)

sources	= ipeextract.cpp

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
