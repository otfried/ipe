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
DESKTOPICONS = hicolor
IPERENDER ?= iperender

all: $(ICONSET) $(DESKTOPICONS)

$(ICONSET): $(ipelogo)
	mkdir -p $(ICONSET)
	$(IPERENDER) -png -resolution 41 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_16x16.png
	$(IPERENDER) -png -resolution 82 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_32x32.png
	$(IPERENDER) -png -resolution 162 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_64x64.png
	$(IPERENDER) -png -resolution 322 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_128x128.png
	$(IPERENDER) -png -resolution 644 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_256x256.png
	$(IPERENDER) -png -resolution 1286 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_512x512.png
	$(IPERENDER) -png -resolution 2570 -transparent -nocrop $(ipelogo) $(ICONSET)/icon_512x512@2x.png

$(DESKTOPICONS): $(ipelogo)
	mkdir -p $(DESKTOPICONS)/16x16/apps
	mkdir -p $(DESKTOPICONS)/48x48/apps
	mkdir -p $(DESKTOPICONS)/256x256/apps
	mkdir -p $(DESKTOPICONS)/scalable/apps
	$(IPERENDER) -png -resolution 41 -transparent -nocrop $(ipelogo) \
		$(DESKTOPICONS)/16x16/apps/org.otfried.Ipe.png
	$(IPERENDER) -png -resolution 122 -transparent -nocrop $(ipelogo) \
		$(DESKTOPICONS)/48x48/apps/org.otfried.Ipe.png
	$(IPERENDER) -png -resolution 644 -transparent -nocrop $(ipelogo) \
		$(DESKTOPICONS)/256x256/apps/org.otfried.Ipe.png
	$(IPERENDER) -svg -transparent -nocrop $(ipelogo) \
		$(DESKTOPICONS)/scalable/apps/org.otfried.Ipe.svg
