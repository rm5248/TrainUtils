#!/bin/bash

# Check if there are two arguments
if [ $# -eq 2 ]; then
    # Check if the input files actually exists.
    if ! [[ -f "$1" ]]; then
     echo "The input file $1 does not exist."
     exit 1
    elif ! [[ -f "$2" ]]; then
     echo "The input file $2 does not exist."
     exit 1
    fi 
else
    echo "Usage: $0 [inputfile] [outputfile]"
    exit 1
fi


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Create a temporary directory where the "normalized" XML files are stored to
# BTW: Keep this tmpdir creation OS X and GNU/Linux compatible
tmpdir=`mktemp -d 2>/dev/null || mktemp -d -t 'xmlcompare'`
trap 'rm -rf "$tmpdir"' EXIT INT TERM HUP
xsltproc -o $tmpdir/$1 "$SCRIPT_DIR"/indent-and-sort-attr.xsl $1
xsltproc -o $tmpdir/$2 "$SCRIPT_DIR"/indent-and-sort-attr.xsl $2

# for debugging
#cat $tmpdir/$1
#echo "------------------------------"
#cat $tmpdir/$2

# use a simple diff, replace by kdiff3 or whatever you like more
diff $tmpdir/$1 $tmpdir/$2
ret=$?
if [ $ret -eq 0 ]
then
   echo "No logical difference in XML files :)"
fi
rm -r $tmpdir
exit $ret
