# -*- makefile -*-
# --------------------------------------------------------------------
#
# Ipe configuration for Unix
#
# *** This File is NOT USED on MAC OS X ***
#
# --------------------------------------------------------------------
# Include and linking options for libraries
# --------------------------------------------------------------------
#
# We just query "pkg-config" for the correct flags.  If this doesn't
# work on your system, enter the correct linker flags and directories
# directly.
#
PKG_CONFIG ?= pkg-config
#
# The name of the Lua package (it could be "lua", "lua54", or "lua5.4")
#
LUA_PACKAGE   ?= lua5.4
#
# Add spell checking? (you'll need to set SPELL_CFLAGS and SPELL_LIBS below)
#
#IPE_SPELLCHECK = 1
#
#
ZLIB_CFLAGS   ?=
ZLIB_LIBS     ?= -lz
JPEG_CFLAGS   ?=
JPEG_LIBS     ?= -ljpeg
PNG_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags libpng)
PNG_LIBS      ?= $(shell $(PKG_CONFIG) --libs libpng)
FREETYPE_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags freetype2)
FREETYPE_LIBS ?= $(shell $(PKG_CONFIG) --libs freetype2)
CAIRO_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags cairo)
CAIRO_LIBS    ?= $(shell $(PKG_CONFIG) --libs cairo)
LUA_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags $(LUA_PACKAGE))
LUA_LIBS      ?= $(shell $(PKG_CONFIG) --libs $(LUA_PACKAGE))
CURL_CFLAGS   ?= $(shell $(PKG_CONFIG) --cflags libcurl)
CURL_LIBS     ?= $(shell $(PKG_CONFIG) --libs libcurl)
SPIRO_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags libspiro)
SPIRO_LIBS    ?= $(shell $(PKG_CONFIG) --libs libspiro)
GSL_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags gsl)
GSL_LIBS      ?= $(shell $(PKG_CONFIG) --libs gsl)
#
# Until Qt 6.3.1, the pkg-config files are missing
# here is an ugly hack using the Qt5 pkg-config instead
#
QT6PKGCONFIG  := $(shell $(PKG_CONFIG) --exists Qt6Gui && echo "YES")
ifndef QT_CFLAGS
ifeq "$(QT6PKGCONFIG)" "YES"
QT_CFLAGS     ?= $(shell $(PKG_CONFIG) --cflags Qt6Gui Qt6Widgets Qt6Core)
else
QT_CFLAGS1    := $(shell $(PKG_CONFIG) --cflags Qt5Gui Qt5Widgets Qt5Core)
QT_CFLAGS     ?= $(subst qt5,qt6,$(QT_CFLAGS1))
endif
endif
ifndef QT_LIBS
ifeq "$(QT6PKGCONFIG)" "YES"
QT_LIBS	      ?= $(shell $(PKG_CONFIG) --libs Qt6Gui Qt6Widgets Qt6Core)
else
QT_LIBS1      := $(shell $(PKG_CONFIG) --libs Qt5Gui Qt5Widgets Qt5Core)
QT_LIBS	      ?= $(subst Qt5,Qt6,$(QT_LIBS1))
endif
endif
#
ifdef IPE_SPELLCHECK
SPELL_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags QtSpell-qt6)
SPELL_LIBS    ?= $(shell $(PKG_CONFIG) --libs QtSpell-qt6)
endif
#
# Library needed to use dlopen/dlsym/dlclose calls
#
DL_LIBS       ?= -ldl
#
# MOC is the Qt meta-object compiler.
# Make sure it's the right one for Qt6.
MOC	      ?= /usr/lib/qt6/libexec/moc
#
# --------------------------------------------------------------------
#
# The C++ compiler
# I'm testing with g++ and clang++.
#
CXX = g++
#
# Special compilation flags for compiling shared libraries
# 64-bit Linux requires shared libraries to be compiled as
# position independent code, that is -fpic or -fPIC
# Qt6 seems to require -fPIC
DLL_CFLAGS = -fPIC
#
# --------------------------------------------------------------------
#
# Installing Ipe:
#
IPEVERS = 7.2.30
#
# IPEPREFIX is the global prefix for the Ipe directory structure, which
# you can override individually for any of the specific directories.
# You could choose "/usr/local" or "/opt/ipe7", or
# even "/usr", or "$(HOME)/ipe7" if you have to install in your home
# directory.
#
# If you are installing Ipe in a networked environment, keep in mind
# that executables, ipelets, and Ipe library are machine-dependent,
# while the documentation and fonts can be shared.
#
#IPEPREFIX  := /usr/local
#IPEPREFIX  := /usr
#IPEPREFIX  := /opt/ipe7
#
ifeq "$(IPEPREFIX)" ""
$(error You need to specify IPEPREFIX!)
endif
#
# Where Ipe executables will be installed ('ipe', 'ipetoipe' etc)
IPEBINDIR  ?= $(IPEPREFIX)/bin
#
# Where the Ipe libraries will be installed ('libipe.so' etc.)
IPELIBDIR  ?= $(IPEPREFIX)/lib
#
# Where the header files for Ipelib will be installed:
IPEHEADERDIR ?= $(IPEPREFIX)/include
#
# Where Ipelets will be installed:
IPELETDIR ?= $(IPEPREFIX)/lib/ipe/$(IPEVERS)/ipelets
#
# Where Lua code will be installed
# (This is the part of the Ipe program written in the Lua language)
IPELUADIR ?= $(IPEPREFIX)/share/ipe/$(IPEVERS)/lua
#
# Directory where Ipe will look for scripts
# (standard scripts will also be installed here)
IPESCRIPTDIR ?= $(IPEPREFIX)/share/ipe/$(IPEVERS)/scripts
#
# Directory where Ipe will look for style files
# (standard Ipe styles will also be installed here)
IPESTYLEDIR ?= $(IPEPREFIX)/share/ipe/$(IPEVERS)/styles
#
# IPEICONDIR contains the icons used in the Ipe user interface
#
IPEICONDIR ?= $(IPEPREFIX)/share/ipe/$(IPEVERS)/icons
#
# IPEDOCDIR contains the Ipe documentation (mostly html files)
#
IPEDOCDIR ?= $(IPEPREFIX)/share/ipe/$(IPEVERS)/doc
#
# The Ipe manual pages are installed into IPEMANDIR
#
IPEMANDIR ?= $(IPEPREFIX)/share/man/man1
#
# --------------------------------------------------------------------
