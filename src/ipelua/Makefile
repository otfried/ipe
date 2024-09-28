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

sources	= \
	ipelib.cpp \
	ipeluageo.cpp \
	ipeluaobj.cpp \
	ipeluastyle.cpp \
	ipeluapage.cpp \
	ipeluaipelet.cpp

$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) $(SONAME) -o $@ $^ $(LIBS)
	$(MAKE_SYMLINKS)

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
