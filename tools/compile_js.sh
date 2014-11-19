#!/bin/bash

set -e

MYPATH=`dirname $0`
OUTFILE=$(mktemp -dt tessel.XXXXXXXXXX)
TARGET=$1
SRC=$(basename "$TARGET")
BUILTIN_SRC="builtin/"$SRC

mkdir -p $OUTFILE/builtin
$MYPATH/../node_modules/.bin/colony-compiler -l $1 > $OUTFILE/$BUILTIN_SRC
pushd $OUTFILE > /dev/null
(echo "const \\"; xxd -i $BUILTIN_SRC) > out.c
popd > /dev/null
mv $OUTFILE/out.c $2
