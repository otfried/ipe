#!/bin/bash
#
# Ipe specific version of "latexonline" script by A. Slushnikov:
#  _          _                  ___        _ _
# | |    __ _| |_ _____  __     / _ \ _ __ | (_)_ __   ___
# | |   / _` | __/ _ \ \/ /____| | | | '_ \| | | '_ \ / _ \
# | |___ (_| | |_  __/>  <_____| |_| | | | | | | | | |  __/
# |_____\__,_|\__\___/_/\_\     \___/|_| |_|_|_|_| |_|\___|
#
# WEB Site: latex.aslushnikov.com
# Issues/bugs: https://github.com/aslushnikov/latex-online
#
# Command-line interface for latex-online service
#

function cleanup {
    rm $dumpHeaders 2>/dev/null
    rm $outputFile 2>/dev/null
    rm $tarball 2>/dev/null
}

function usage {
   echo "Usage: ipecurl ( pdflatex | xelatex | lualatex )
Run Latex compilation for Ipe using the Latex-Online service.
">&2
}

trap cleanup EXIT

if (( $# != 1 )); then
    usage
    exit 1
fi

if  [[ ($1 == '-h') || ($1 == '--help') ]]; then
    usage
    exit 1;
fi

host=`cat url1.txt`
command=$1

tarball=`mktemp latexTarball-XXXXXX`
tar -cj ipetemp.tex > $tarball
rm -f ipetemp.log ipetemp.pdf

# create tmp file for headers
dumpHeaders=`mktemp latexCurlHeaders-XXXXXX`
outputFile=`mktemp latexCurlOutput-XXXXXX`
curl -L --post301 --post302 --post303 -D $dumpHeaders -F file=@$tarball "$host/data?target=ipetemp.tex&command=$command" > $outputFile

httpResponse=`cat $dumpHeaders | grep ^HTTP | tail -1 | cut -f 2 -d ' '`

echo "entering extended mode: using latexonline at '$host'" > ipetemp.log

if [[ $httpResponse != 2* ]];
then
    # if so then output is not pdfFile but plain text one with errors
    cat $outputFile >> ipetemp.log
else
    cp $outputFile ipetemp.pdf
fi
