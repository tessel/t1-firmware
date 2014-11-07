#!/bin/bash
set -e

OUTFILE=$(mktemp -dt tessel.XXXXXXXXXX)
mkdir -p $OUTFILE/$(dirname "$1")
$(dirname $0)/../node_modules/.bin/colony-compiler -l $1 > $OUTFILE/$1
pushd $OUTFILE > /dev/null
(echo "const \\"; xxd -i $1) > out.c
popd > /dev/null
mv $OUTFILE/out.c $2