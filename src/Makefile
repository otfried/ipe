# --------------------------------------------------------------------
# Makefile for building all components of Ipe
# --------------------------------------------------------------------

GOAL ?= all

ifdef IPECROSS
IPEQVORONOI = 1
endif

subdirs_libraries = ipelib ipecairo ipelua ipecanvas ipeui
subdirs_cli = ipetoipe iperender ipescript ipe6upgrade ipeextract

subdirs = $(subdirs_libraries) $(subdirs_cli) ipelets/lua ipe

ifdef IPEWASM
undefine IPEQVORONOI
subdirs += ipejs
else
subdirs += ipepresenter ipecurl
endif

ifdef IPEQVORONOI
subdirs += ipelets/qvoronoi
endif

ICONSET = ../artwork/ipe.iconset
ifdef IPECROSS
IPEICNS = ../build/ipe.ico
else
ifdef IPEUI_COCOA
IPEICNS = ../build/ipe.icns
else
IPEICNS =
endif
endif

.PHONY: all manual epub book documentation $(subdirs)

all: $(subdirs)

ipetoipe: ipelib
ipelets/kgon: ipelib
ipelets/qvoronoi: ipelib
ipe6upgrade: ipelib
ipeextract: ipelib
ipecairo: ipelib
iperender: ipelib ipecairo
ipecanvas: ipelib ipecairo
ipeview: ipecanvas
ipelua: ipelib
ipescript: ipelib ipelua
ipe: ipelib ipecanvas ipelua ipeui $(IPEICNS)
ipepresenter: ipecanvas $(IPEICNS)
ipecurl: ipelib
ipejs: ipelib ipecairo ipecanvas

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

ifdef IPECROSS
$(IPEICNS):
	mkdir -p ../build
	icotool -c $(ICONSET)/icon_16x16.png \
		$(ICONSET)/icon_32x32.png \
		$(ICONSET)/icon_64x64.png \
		$(ICONSET)/icon_128x128.png \
		$(ICONSET)/icon_256x256.png \
		$(ICONSET)/icon_512x512.png -o $(IPEICNS)
else
$(IPEICNS):
	mkdir -p ../build
	iconutil -c icns -o $(IPEICNS) $(ICONSET)
endif

.PHONY: clean install
clean:
	$(MAKE)	GOAL=clean

install: all
	$(MAKE) GOAL=install

app: all $(IPEICNS)
	$(MAKE) --directory=ipe app
	$(MAKE) --directory=ipelets/lua app
ifdef IPEQVORONOI
	$(MAKE) --directory=ipelets/qvoronoi app
endif
	$(MAKE) --directory=ipepresenter app

dependencies: app
	$(MAKE) --directory=ipe dependencies
	$(MAKE) --directory=ipepresenter dependencies

manual:
	sphinx-build -M html ../manual ../build

documentation:
	$(MAKE) --directory=ipelib documentation

book:
	sphinx-build -M latexpdf ../manual ../build

epub:
	sphinx-build -M epub ../manual ../build

format:
	clang-format -i */*.h */*.cpp

# --------------------------------------------------------------------
