#!/bin/bash
#
# create a variable with git commit and date to display in "About Ipe"
#

tag=`git log -1 HEAD --format="commit %h and was built %aD."`

cat > src/gitversion.ts <<EOF
export const buildInfo =
	"$tag";
EOF
