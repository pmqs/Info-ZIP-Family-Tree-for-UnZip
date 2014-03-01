#!/bin/sh

#==============================================================================
# unix/unsafe_prod.sh: Check PROD directory safety.     Revised: 2014-01-21
#
# Copyright (c) 2014 Info-ZIP.  All rights reserved.
#
# See the accompanying file LICENSE, version 2009-Jan-2 or later (the
# contents of which are also included in zip.h) for terms of use.  If,
# for some reason, all these files are missing, the Info-ZIP license may
# also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
#==============================================================================

if [ -n "$1" ]; then
  prod="$1"
else
  prod='.'
fi

# Return zero, if unsafe.
unsafe_prod=0

if [ "${prod}" = '.' ]; then
  unsafe_prod=1
fi

if [ ${unsafe_prod} -eq 0 ]; then
  echo "${prod}" | grep -e '^/' -e '\.\./' > /dev/null 2>&1
  if [ $? -ne 0 ]; then
    unsafe_prod=1
  fi
fi

exit ${unsafe_prod}
