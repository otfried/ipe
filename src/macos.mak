# -*- makefile -*-
# --------------------------------------------------------------------
#
# Ipe configuration for Mac OS with pkg-config (e.g. with homebrew)
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
PKG_CONFIG   ?= pkg-config
#
PNG_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags libpng)
PNG_LIBS      ?= $(shell $(PKG_CONFIG) --libs libpng)
FREETYPE_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags freetype2)
FREETYPE_LIBS ?= $(shell $(PKG_CONFIG) --libs freetype2)
CAIRO_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags cairo)
CAIRO_LIBS    ?= $(shell $(PKG_CONFIG) --libs cairo)
LUA_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags lua)
LUA_LIBS      ?= $(shell $(PKG_CONFIG) --libs lua)
SPIRO_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags libspiro)
SPIRO_LIBS    ?= $(shell $(PKG_CONFIG) --libs libspiro)
GSL_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags gsl)
GSL_LIBS      ?= $(shell $(PKG_CONFIG) --libs gsl)
#
IPEVERS = 7.3.1
#
CXX = clang++
#
# --------------------------------------------------------------------
