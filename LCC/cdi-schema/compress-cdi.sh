#!/bin/sh

# Compress the CDI by removing all newlines.
# Also convert all double-quotes to single
# arg1: input CDI file

input_file=$1

#XMLLINT_INDENT="" xmllint --format  $1 | tr -d '\n' | sed "s/\"/'/g"
XMLLINT_INDENT="" xmllint --format  $1 | sed "s/\"/'/g" | sed -E 's/^(.*)$/\1 \\/g'
echo ""

