# --------------------------------------------------------------------
# Makefile for Ipecurl
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipecurl
include ../common.mak

TARGET = $(call exe_target,ipecurl)

CPPFLAGS += -I../include $(CURL_CFLAGS)
LIBS += -L$(buildlib) -lipe $(CURL_LIBS)

all: $(TARGET)

win_sources   = ipecurl_win.cpp
cocoa_sources = ipecurl_osx.cpp

ifndef WIN32
ifndef MACOS
IPECURL_SH=1
endif
endif

ifdef IPECURL_SH

$(TARGET): ipecurl.sh
	$(MAKE_BINDIR)
	cp ipecurl.sh $(TARGET)
	chmod 0755 $(TARGET)

clean:
	@-rm -f $(TARGET)

else

$(TARGET): $(objects)
	$(MAKE_BINDIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

endif

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)

# --------------------------------------------------------------------
