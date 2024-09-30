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
IPEICO = ../build/ipe.ico
else
IPEICO =
endif
IPEICNS = ../build/ipe.icns

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
ipe: ipelib ipecanvas ipelua ipeui $(IPEICO)
ipepresenter: ipecanvas $(IPEICO)
ipecurl: ipelib
ipejs: ipelib ipecairo ipecanvas

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

$(IPEICO):
	mkdir -p ../build
	icotool -c $(ICONSET)/icon_16x16.png \
		$(ICONSET)/icon_32x32.png \
		$(ICONSET)/icon_64x64.png \
		$(ICONSET)/icon_128x128.png \
		$(ICONSET)/icon_256x256.png \
		$(ICONSET)/icon_512x512.png -o $(IPEICO)

$(IPEICNS):
	mkdir -p ../build
	iconutil -c icns -o $(IPEICNS) $(ICONSET)

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

manual:
	sphinx-build -M html ../manual ../build

epub:
	sphinx-build -M epub ../manual ../build

book:
	sphinx-build -M latexpdf ../manual ../build

documentation:
	$(MAKE) --directory=ipelib documentation

# --------------------------------------------------------------------
