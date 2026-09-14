#!/bin/bash

#
# The Ipe Electron Edition reuses all the HTML/CSS, Typescript, and WASM code from
# the web edition.
#
# This script copies the necessary files from ipe-web to ipe-electron.
#

IW=../ipe-web

cp $IW/index.html src/renderer
sed -e "s!/src/main.ts!./src/renderer.ts!" -i src/renderer/index.html
sed -z -e 's!<script type="text/javascript">.*</script>!!' -i src/renderer/index.html

cp ../../artwork/ipe.iconset/icon_64x64.png src/main

tag=`git log -1 HEAD --format="commit %h and was built %aD."`

cat > src/renderer/src/gitversion.ts <<EOF
export const buildInfo =
	"$tag";
EOF
