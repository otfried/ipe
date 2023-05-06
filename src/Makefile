# --------------------------------------------------------------------
# Makefile for building all components of Ipe
# --------------------------------------------------------------------

GOAL ?= all

ifdef IPECROSS
IPEQVORONOI = 1
endif

subdirs = \
	ipelib \
	ipelets/lua \
	ipetoipe \
	ipe6upgrade \
	ipeextract \
	ipescript \
	ipecairo \
	iperender \
	ipecanvas \
	ipelua \
	ipeui \
	ipe \
	ipepresenter \
	ipecurl

ifdef IPEQVORONOI
subdirs += ipelets/qvoronoi
endif

ipelogo = ../artwork/ipe_logo.ipe
ICONSET = ../build/ipe.iconset
ifdef IPECROSS
IPEICO = ../build/ipe.ico
else
IPEICO =
endif

.PHONY: all documentation $(subdirs)

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
ipe: ipecanvas ipelua $(IPEICO)
ipepresenter: ipecanvas $(IPEICO)
ipecurl: ipelib

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

$(ICONSET): $(ipelogo)
	mkdir -p $(ICONSET)
	iperender -png -resolution 41 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_16x16.png
	iperender -png -resolution 82 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_32x32.png
	iperender -png -resolution 162 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_64x64.png
	iperender -png -resolution 322 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_128x128.png
	iperender -png -resolution 644 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_256x256.png
	iperender -png -resolution 1286 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_512x512.png
	iperender -png -resolution 2570 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_512x512@2x.png

$(IPEICO): $(ICONSET)
	icotool -c $(ICONSET)/icon_16x16.png \
		$(ICONSET)/icon_32x32.png \
		$(ICONSET)/icon_64x64.png \
		$(ICONSET)/icon_128x128.png \
		$(ICONSET)/icon_256x256.png \
		$(ICONSET)/icon_512x512.png -o $(IPEICO)

.PHONY: clean install
clean:
	$(MAKE)	GOAL=clean

install: all $(ICONSET)
	$(MAKE) GOAL=install

app: all
	$(MAKE) --directory=ipe app
	$(MAKE) --directory=ipelets/lua app
ifdef IPEQVORONOI
	$(MAKE) --directory=ipelets/qvoronoi app
endif
	$(MAKE) --directory=ipepresenter app

documentation:
	$(MAKE) --directory=ipelib documentation

# --------------------------------------------------------------------
