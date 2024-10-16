#
# Prepare all files to be bundled for ipe-web
#

rm -fr opt
mkdir -p opt/ipe/icons
mkdir -p opt/ipe/styles
mkdir -p opt/ipe/lua

cp ../../artwork/icons.ipe opt/ipe/icons
cp ../../styles/* opt/ipe/styles
cp lua/*.lua opt/ipe/lua

