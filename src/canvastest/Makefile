# --------------------------------------------------------------------
# Makefile for canvastest
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/canvastest
include ../common.mak

TARGET = $(call exe_target,canvastest)

CXXFLAGS += -I../include -I../ipecairo -I../ipecanvas \
	$(UI_CFLAGS) $(CAIRO_CFLAGS)
LIBS += -L$(buildlib) $(UI_LIBS) $(CAIRO_LIBS) -lipecanvas -lipecairo -lipe

all: $(TARGET)

gtk_sources = canvastest_gtk.cpp

$(TARGET): $(objects) $(resource) $(nib)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $(objects) $(resource) $(LIBS)

clean:
	@-rm -f $(objects) $(resource) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

# --------------------------------------------------------------------
