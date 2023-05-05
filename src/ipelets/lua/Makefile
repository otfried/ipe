# --------------------------------------------------------------------
# Makefile for Lua ipelets (only for install)
# --------------------------------------------------------------------

IPESRCDIR = ../..
include ../../common.mak

LUASCRIPTS = move.lua goodies.lua align.lua euclid.lua \
	symbols.lua search-replace.lua selectby.lua \
	gridmaker.lua

all: 

install: 
	$(INSTALL_DIR) $(INSTALL_ROOT)$(IPELETDIR)
	$(INSTALL_FILES) $(LUASCRIPTS) $(INSTALL_ROOT)$(IPELETDIR)

ifdef IPEAPP

app:
	$(INSTALL_DIR) $(RESOURCEDIR)/ipelets
	$(INSTALL_FILES) $(LUASCRIPTS) $(RESOURCEDIR)/ipelets

endif

# --------------------------------------------------------------------
