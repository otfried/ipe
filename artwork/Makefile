# 
# Makefile for building icons
#
# The Ipe icons are actually produced from ipe_logo.ipe
# using iperender at different resolutions.
#
# We keep the rendered icons in git anyway, so that Ipe can be built
# and packaged on machines that do not have Latex installed.
# 

ipelogo = ipe_logo.ipe
ICONSET = ipe.iconset
IPERENDER ?= iperender

GOAL ?= $(ICONSET)

$(ICONSET): $(ipelogo)
	mkdir -p $(ICONSET)
	$(IPERENDER) -png -resolution 41 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_16x16.png
	$(IPERENDER) -png -resolution 82 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_32x32.png
	$(IPERENDER) -png -resolution 162 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_64x64.png
	$(IPERENDER) -png -resolution 322 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_128x128.png
	$(IPERENDER) -png -resolution 644 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_256x256.png
	$(IPERENDER) -png -resolution 1286 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_512x512.png
	$(IPERENDER) -png -resolution 2570 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_512x512@2x.png

