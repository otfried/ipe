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
ipe: ipecanvas ipelua
ipepresenter: ipecanvas
ipecurl: ipelib

$(subdirs):
	$(MAKE) --directory=$@ $(GOAL)

.PHONY: clean install
clean:
	$(MAKE)	GOAL=clean

install: all
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
