# --------------------------------------------------------------------
# Makefile for Ipelib
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipelib
include ../common.mak

TARGET = $(call dll_target,ipe)
MAKE_SYMLINKS = $(call dll_symlinks,ipe)
SONAME = $(call soname,ipe)
INSTALL_SYMLINKS = $(call install_symlinks,ipe)

CPPFLAGS += -I../include
CPPFLAGS += -DIPE_GSL $(GSL_CFLAGS)
LIBS += $(GSL_LIBS)
ifdef IPE_NO_IPELIB_VERSION_CHECK
CPPFLAGS += -DIPE_NO_IPELIB_VERSION_CHECK
endif
CPPFLAGS += $(ZLIB_CFLAGS) $(JPEG_CFLAGS) $(PNG_CFLAGS) $(SPIRO_CFLAGS)
CXXFLAGS += $(DLL_CFLAGS)
LIBS += $(JPEG_LIBS) $(ZLIB_LIBS) $(PNG_LIBS) $(SPIRO_LIBS)

DOXYGEN ?= doxygen
DOXDIR  = $(BUILDDIR)/doxygen

ifdef WIN32
LIBS += -lole32 -luuid
endif

all: $(TARGET)

sources	= \
	ipebase.cpp \
	ipeplatform.cpp \
	ipegeo.cpp \
	ipexml.cpp \
	ipeattributes.cpp \
	ipebitmap.cpp \
	ipeshape.cpp \
	ipegroup.cpp \
	ipeimage.cpp \
	ipetext.cpp \
	ipepath.cpp \
	ipereference.cpp \
	ipeobject.cpp \
	ipefactory.cpp \
	ipestdstyles.cpp \
	ipeiml.cpp \
	ipepage.cpp \
	ipepainter.cpp \
	ipetoolbase.cpp \
	ipepdfparser.cpp \
	ipepdfwriter.cpp \
	iperesources.cpp \
	ipestyle.cpp \
	ipesnap.cpp \
	ipeutils.cpp \
	ipelatex.cpp \
	ipedoc.cpp

ifdef WIN32
sources += ipebitmap_win.cpp
else
sources += ipebitmap_unix.cpp
endif

$(TARGET): $(objects)
	$(MAKE_LIBDIR)
	$(CXX) $(LDFLAGS) $(DLL_LDFLAGS) $(SONAME) -o $@ $^ $(LIBS)
	$(MAKE_SYMLINKS)

clean:
	@-rm -f $(objects) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

documentation: 
	rm -fr $(DOXDIR)
	mkdir -p $(DOXDIR)
	$(DOXYGEN)

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELIBDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPELIBDIR)
	$(INSTALL_FILES) ../include/*.h $(INSTALL_ROOT)$(IPEHEADERDIR)
	$(INSTALL_SYMLINKS)

# --------------------------------------------------------------------
