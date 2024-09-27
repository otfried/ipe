# --------------------------------------------------------------------
# Makefile for ipejs - Javascript bindings to ipelib etc.
# --------------------------------------------------------------------

ifdef IPENODEJS

OBJDIR = $(BUILDDIR)/obj/ipejs
include ../common.mak

TARGET = $(call exe_target,ipejs)

CPPFLAGS += -I../include
LIBS += -L$(buildlib) -lipe
LDFLAGS += -lembind

all: $(TARGET)

sources	= ipejs.cpp

$(TARGET): $(objects)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $(objects) $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

endif

# --------------------------------------------------------------------
