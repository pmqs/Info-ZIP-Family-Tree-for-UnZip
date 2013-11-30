#!/bin/sh

#==============================================================================
# unix/mlz.sh: Combine loose object files and existing object libraries
#              into a new (big) object library.         Revised: 2013-11-29
#
# Copyright (c) 2012-2013 Info-ZIP.  All rights reserved.
#
# See the accompanying file LICENSE, version 2009-Jan-2 or later (the
# contents of which are also included in zip.h) for terms of use.  If,
# for some reason, all these files are missing, the Info-ZIP license may
# also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
#==============================================================================

#==============================================================================
# This script combines a set of loose object files ($2) with the object
# files in a set of object libraries ($3, ...), to make a new (big)
# object library ($1).
#
# $1        Output archive.
# $2        ".o" file list.
# $3 ...    ".a" file list.
# ${PROD}   Product file directory.
#
# Currently, each section of the optional code (AES_WG, LZMA, PPMd, ...)
# goes into its own, separate object library ($(LIB_AES_WG),
# $(LIB_LZMA), $(LIB_PPMD), ...).  It's easier to extract the object
# files from each of these object libraries than to keep their component
# object files around, and to maintain lists of what's in them.
#==============================================================================

tmpdir="` basename $0 `_$$_tmp"

trap "sts=$? ; rm -rf $tmpdir; exit $sts" 0 1 2 3 15

mkdir "$tmpdir"

aro="$1"
o_zip="$2"
shift
shift

if test -z "${PROD}" ; then
  PROD=.
fi

cd "$tmpdir"

for ar in "$@"
do
  ar x ../$ar
done

cd ..

if test -f "$aro" ; then
  rm -f "$aro"
fi
ar r "$aro" $o_zip \
 ` ( cd "$tmpdir" ; find . -name '*.o' -print | sed -e "s|^.|${PROD}|" ) `

