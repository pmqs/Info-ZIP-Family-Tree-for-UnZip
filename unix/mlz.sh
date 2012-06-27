#!/bin/sh

# $1 = Output archive.
# $2 = ".o" file list.
# $3 ... = ".a" file list.

tmpdir="` basename $0 `_$$_tmp"

trap "sts=$? ; rm -rf $tmpdir; exit $sts" 0 1 2 3 15

mkdir "$tmpdir"

aro="$1"
o_zip="$2"
shift
shift

cd "$tmpdir"

for ar in "$@"
do
  ar x ../$ar
done

cd ..

if test -f "$aro" ; then
  rm -f "$aro"
fi
ar r "$aro" $o_zip ` ( cd "$tmpdir" ; ls ) `
