# --------------------------------------------------------------------
# Makefile for uitest
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/uitest
include ../common.mak

TARGET = $(call exe_target,uitest)

CXXFLAGS += $(LUA_CFLAGS) $(UI_CFLAGS)
LIBS += -L$(buildlib) -lipeui $(LUA_LIBS) $(UI_LIBS)

all: $(TARGET)

win_sources = uitest_win.cpp
gtk_sources = uitest_gtk.cpp
cocoa_sources = uitest_cocoa.cpp

ifdef WIN32
# To show Lua console output, do not compile as GUI only
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows
endif

$(TARGET): $(objects)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@-rm -f $(objects) $(resource) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

# --------------------------------------------------------------------
