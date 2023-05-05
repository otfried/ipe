# --------------------------------------------------------------------
# Makefile for Ipescript
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipescript
include ../common.mak

TARGET = $(call exe_target,ipescript)

CPPFLAGS += -I../include -I../ipelua $(LUA_CFLAGS)
LIBS += -L$(buildlib) -lipelua -lipe $(LUA_LIBS) $(DL_LIBS)

ifdef IPEBUNDLE
CPPFLAGS += -DIPEBUNDLE
else
CPPFLAGS += -DIPELETDIR=\"$(IPELETDIR)\"
CPPFLAGS += -DIPESTYLEDIR=\"$(IPESTYLEDIR)\"
CPPFLAGS += -DIPESCRIPTDIR=\"$(IPESCRIPTDIR)\"
endif

all: $(TARGET)

sources	= ipescript.cpp

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
