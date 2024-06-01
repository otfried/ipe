# -*- makefile -*-
# --------------------------------------------------------------------
#
# Ipe configuration for Mac OS with manual settings
#
# --------------------------------------------------------------------
#
# Where are the dependencies?
#
IPEDEPS	 ?= /opt/local
#
# --------------------------------------------------------------------
#
# We build as an application bundle (a directory "Ipe.app" that
# contains Ipe and all its files).
# If you don't want this, you'll need to also set IPEPREFIX and
# all the variables in the "config.mak" file.
#
IPEBUNDLE  = 1
#
# --------------------------------------------------------------------
#
PNG_CFLAGS      ?= -I$(IPEDEPS)/include/libpng16
PNG_LIBS        ?= -L$(IPEDEPS)/lib -lpng16
FREETYPE_CFLAGS ?= -I$(IPEDEPS)/include/freetype2 -I$(IPEDEPS)/include
FREETYPE_LIBS   ?= -L$(IPEDEPS)/lib -lfreetype
CAIRO_CFLAGS    ?= -I$(IPEDEPS)/include/cairo
CAIRO_LIBS      ?= -L$(IPEDEPS)/lib -lcairo
LUA_CFLAGS      ?= -I$(IPEDEPS)/include/lua
LUA_LIBS        ?= -L$(IPEDEPS)/lib -llua54 -lm
SPIRO_CFLAGS    ?= -I$(IPEDEPS)/include
SPIRO_LIBS      ?= -L$(IPEDEPS)/lib -lspiro
GSL_CFLAGS      ?= -I$(IPEDEPS)/include
GSL_LIBS        ?= -L$(IPEDEPS)/lib -lgsl -lgslcblas -lm
#
IPEVERS = 7.2.30
#
CXX = clang++
#
# --------------------------------------------------------------------
