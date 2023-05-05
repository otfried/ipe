# -*- makefile -*-
# --------------------------------------------------------------------
#
# Ipe configuration for Mac OS X
#
# --------------------------------------------------------------------
#
# Where are the dependencies?
#
# Setting to use Macports libraries:
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
LUA_CFLAGS      ?= -I$(IPEDEPS)/include
LUA_LIBS        ?= -L$(IPEDEPS)/lib -llua.5.3 -lm
SPIRO_CFLAGS    ?=
SPIRO_LIBS      ?= -L$(IPEDEPS)/lib -lspiro
GSL_CFLAGS      ?= $(shell gsl-config --cflags)
GSL_LIBS        ?= $(shell gsl-config --libs)
#
IPEVERS = 7.2.27
#
CXX = clang++
#
# --------------------------------------------------------------------
