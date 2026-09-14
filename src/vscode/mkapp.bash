#!/bin/bash

#
# The Ipe VSCode extension reuses all the HTML/CSS, Typescript, and
# WASM code from the web edition.
#
# IpeUi recognizes that it runs inside VSCode from the presence of ???
#
# This script copies the necessary files from ipe-web to vscode.
#

IW=../ipe-web

{
  printf 'export const rootDocument = `\n'
  sed -n '/<body>/,/<script/{ /<body>/d; /<script/q; p; }' $IW/index.html
  printf '`;\n'
} > src/root.ts

mkdir -p webview/ipe

cp $IW/src/* webview/ipe/

rm webview/ipe/main.ts
rm webview/ipe/vite-env.d.ts

# cp ../../artwork/ipe.iconset/icon_64x64.png webview/ipe/

mkdir -p out/webview
cp webview/ipe/ipe.js out/webview
mv webview/ipe/ipe.wasm out/webview

tag=`git log -1 HEAD --format="commit %h and was built %aD."`

cat > webview/ipe/gitversion.ts <<EOF
export const buildInfo =
	"$tag";
EOF
