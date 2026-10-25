# --------------------------------------------------------------------
# Makefile for Lua bindings for ipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipelua
include ../common.mak

TARGET = $(call dll_target,ipelua)
MAKE_SYMLINKS = $(call dll_symlinks,ipelua)
SONAME = $(call soname,ipelua)
INSTALL_SYMLINKS = $(call install_symlinks,ipelua)

CPPFLAGS += -I../include $(LUA_CFLAGS)
CXXFLAGS += $(DLL_CFLAGS)

ifndef IPEWASM
LIBS += -L$(buildlib) -lipe $(LUA_LIBS) $(DL_LIBS)
endif

all: $(TARGET)

LUA ?= $(LUA_PACKAGE)

.PHONY: test checkdocs doctest
test: $(TARGET)
	$(LUA) test.lua "$(abspath $(TARGET))"

# run the examples of the manual chapter "Scripts and ipelets"; needs the
# built ipescript, and uses Ipe's Lua code from the source tree
IPESCRIPT ?= $(buildbin)/ipescript

doctest:
	unset IPEDEBUG; \
	LD_LIBRARY_PATH="$(abspath $(buildlib)):$$LD_LIBRARY_PATH" \
	DYLD_LIBRARY_PATH="$(abspath $(buildlib)):$$DYLD_LIBRARY_PATH" \
	IPESCRIPT="$(abspath $(IPESCRIPT))" \
	IPEMANUAL="$(abspath ../../manual)" \
	IPELUAPATH="$(abspath ../ipe/lua)/?.lua" \
	IPESCRIPTS="$(abspath .):$(abspath ../../scripts):$(abspath ../../manual/examples)" \
	"$(abspath $(IPESCRIPT))" doctest

# check that all Lua bindings (including ipeui and AppUi) are in bindings.txt
checkdocs:
	$(LUA) checkdocs.lua

sources	= \
	ipelib.cpp \
	ipeluageo.cpp \
	ipeluaobj.cpp \
	ipeluastyle.cpp \
	ipeluapage.cpp \
	ipeluaipelet.cpp

ifdef IPEWASM
$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	emar rcs $@ $^
else
$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) $(SONAME) -o $@ $^ $(LIBS)
	$(MAKE_SYMLINKS)
endif

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_SYMLINKS)

# --------------------------------------------------------------------
