#!/bin/bash

#
# The Ipe Electron Edition reuses all the HTML/CSS, Typescript, and WASM code from
# the web edition.
#
# IpeUi recognizes that it runs inside Electron from the presence of window.ipc.
#
# This script copies the necessary files from ipe-web to ipe-electron.
#

IW=../ipe-web

cp $IW/index.html src/renderer
cp $IW/src/* src/renderer/src

rm src/renderer/src/main.ts
rm src/renderer/src/vite-env.d.ts

sed -e "s!/src/main.ts!./src/renderer.ts!" -i src/renderer/index.html

sed -z -e 's!<script type="text/javascript">.*</script>!!' -i src/renderer/index.html

cp ../../artwork/ipe.iconset/icon_64x64.png src/main
