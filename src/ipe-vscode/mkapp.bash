#!/bin/bash

#
# The Ipe VS Code extension reuses all the HTML/CSS, Typescript, and
# WASM code from the web edition.
#
# This script copies the necessary files from ipe-web to vscode.
#

IW=../ipe-web

{
  printf 'export const rootDocument = `\n'
  sed -n '/<body>/,/<script/{ /<body>/d; /<script/q; p; }' $IW/index.html
  printf '`;\n'
} > src/root.ts

mkdir -p images
cp ../../artwork/ipe.iconset/icon_128x128.png images/ipe.png

mkdir -p out/webview
cp webview/ipe/ipe.js out/webview
cp webview/ipe/ipe.wasm out/webview

tag=`git log -1 HEAD --format="commit %h and was built %aD."`

cat > webview/gitversion.ts <<EOF
export const buildInfo =
	"$tag";
EOF
