#
# Prepare all files to be bundled for ipe-web
#

IPELETS="move.lua goodies.lua align.lua euclid.lua symbols.lua search-replace.lua selectby.lua gridmaker.lua"

rm -fr opt
mkdir -p opt/ipe/icons
mkdir -p opt/ipe/styles
mkdir -p opt/ipe/lua
mkdir -p opt/ipe/ipelets

cp ../../artwork/icons.ipe opt/ipe/icons
cp ../../styles/* opt/ipe/styles
cp lua/*.lua opt/ipe/lua

for f in $IPELETS; do
    cp ../ipelets/lua/$f opt/ipe/ipelets
done
