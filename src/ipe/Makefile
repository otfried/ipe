# --------------------------------------------------------------------
# Makefile for Ipe
# --------------------------------------------------------------------

OBJDIR = $(BUILDDIR)/obj/ipe
include ../common.mak

TARGET = $(call exe_target,ipe)

CPPFLAGS += -I../include -I../ipecanvas -I../ipecairo -I../ipelua \
	-I../ipeui -I../spiro \
	$(LUA_CFLAGS) $(UI_CFLAGS) $(CAIRO_CFLAGS) $(ZLIB_CFLAGS)
LIBS += -L$(buildlib) -lipecanvas -lipecairo -lipelua -lipe -lipeui \
	$(LUA_LIBS) $(UI_LIBS) $(CAIRO_LIBS) $(ZLIB_LIBS) $(DL_LIBS)

MAKE_ASSETS =
WASM_LIBS =

ifdef IPEWASM
ifdef IPEWASMTARGET
TARGET = $(IPEWASMTARGET)/ipe.js
endif
# create js/wasm files suitable for a browser
LDFLAGS += -sMODULARIZE=1 -sENVIRONMENT=web -sEXPORT_ES6=1 -sALLOW_MEMORY_GROWTH=1
LDFLAGS += -sEXPORTED_RUNTIME_METHODS=ENV,FS,IDBFS,stringToNewUTF8,Emval
LDFLAGS += -lembind -lidbfs.js --embed-file opt
MAKE_ASSETS = bash mkipewebassets.sh
WASM_LIBS = $(BUILDDIR)/lib/libipeui.so $(BUILDDIR)/lib/libipecanvas.so \
	$(BUILDDIR)/lib/libipelua.so $(BUILDDIR)/lib/libipe.so \
	$(BUILDDIR)/lib/libipecairo.so \
	lua/model.lua lua/actions.lua lua/main.lua
endif

ifndef IPEBUNDLE
CPPFLAGS += -DIPELETDIR=\"$(IPELETDIR)\"
CPPFLAGS += -DIPELUADIR=\"$(IPELUADIR)\"
CPPFLAGS += -DIPEICONDIR=\"$(IPEICONDIR)\"
CPPFLAGS += -DIPEDOCDIR=\"$(IPEDOCDIR)\"
CPPFLAGS += -DIPESTYLEDIR=\"$(IPESTYLEDIR)\"
endif

all: $(TARGET)

sources	= tools.cpp appui.cpp uilua.cpp 
qt_sources = main_qt.cpp appui_qt.cpp controls_qt.cpp 
moc_headers = appui_qt.h controls_qt.h waitdlg_qt.h
win_sources = main_win.cpp appui_win.cpp pathview_win.cpp pagesorter_win.cpp
gtk_sources = main_gtk.cpp appui_gtk.cpp
cocoa_sources = main_cocoa.cpp appui_cocoa.cpp controls_cocoa.cpp \
	pagesorter_cocoa.cpp
js_sources = main_js.cpp appui_js.cpp controls_js.cpp

ifdef WIN32
CXXFLAGS += -mwindows
LDFLAGS  += -mwindows

resource = $(OBJDIR)/res.o

$(resource): ipe.rc
	$(WINDRES) -i ipe.rc -o $@ --include-dir=.

else
resource = 
endif

ifdef IPEUI_COCOA

nib = $(RESOURCEDIR)/mainmenu.nib

$(nib): mainmenu.xib
	mkdir -p $(RESOURCEDIR)
	ibtool --compile $(nib) mainmenu.xib

else
nib = 
endif

$(TARGET): $(objects) $(resource) $(nib) Makefile $(WASM_LIBS)
	$(MAKE_BINDIR)
	$(MAKE_ASSETS)
	$(CXX) $(LDFLAGS) -o $@ $(objects) $(resource) $(LIBS)

clean:
	@-rm -f $(objects) $(resource) $(TARGET) $(DEPEND)

$(DEPEND): Makefile
	$(MAKE_DEPEND)

-include $(DEPEND)

ifdef IPEAPP

app:
	$(INSTALL_DIR) $(RESOURCEDIR)/scripts
	$(INSTALL_DIR) $(RESOURCEDIR)/lua
	$(INSTALL_DIR) $(RESOURCEDIR)/styles
	$(INSTALL_DIR) $(RESOURCEDIR)/icons
	$(INSTALL_SCRIPTS) ../../scripts/* $(RESOURCEDIR)/scripts
	$(INSTALL_FILES) lua/*.lua $(RESOURCEDIR)/lua
	$(INSTALL_FILES) ../../styles/basic.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/colors.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/beamer.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/presentation.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/decorations.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/note-paper.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/tikz-shapes.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/imperial.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/right-to-left.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) ../../styles/arabic.isy $(RESOURCEDIR)/styles
	$(INSTALL_FILES) Info.plist $(BUNDLEDIR)
	$(INSTALL_FILES) $(BUILDDIR)/ipe.icns $(RESOURCEDIR)
	$(INSTALL_FILES) ../../artwork/icons.ipe $(RESOURCEDIR)/icons

endif

install: $(TARGET)
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEBINDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEMANDIR) 
#	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEDOCDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELUADIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPESTYLEDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPESCRIPTDIR) 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPEICONDIR)
	$(INSTALL_PROGRAMS) $(TARGET) $(INSTALL_ROOT)$(IPEBINDIR)
	$(INSTALL_SCRIPTS) ../../scripts/* $(INSTALL_ROOT)$(IPESCRIPTDIR)
	$(INSTALL_FILES) ../../man/*.1 $(INSTALL_ROOT)$(IPEMANDIR) 
	$(INSTALL_FILES) lua/*.lua $(INSTALL_ROOT)$(IPELUADIR)
	$(INSTALL_FILES) ../../styles/*.isy $(INSTALL_ROOT)$(IPESTYLEDIR)
	$(INSTALL_FILES) ../../artwork/icons.ipe $(INSTALL_ROOT)$(IPEICONDIR)
	$(INSTALL_FILES) ../../artwork/ipe_logo.ipe $(INSTALL_ROOT)$(IPEICONDIR)
	$(INSTALL_FILES) ../../artwork/ipe.iconset/icon_128x128.png $(INSTALL_ROOT)$(IPEICONDIR)

# --------------------------------------------------------------------
