/*
  Copyright (c) 1990-2015 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  unreduce.c

  Unreduce wrapper module.

  Define USE_UNREDUCE_SMITH to enable copyrighted Unreduce code (in
  unreduce_full.c) from Samuel H. Smith.

  Define USE_UNREDUCE_PUBLIC to enable public-domain Unreduce code (in
  expand.c) from Peter Backes.

  ---------------------------------------------------------------------------*/

#define __UNREDUCE_C_   /* identifies this source module */

#ifdef USE_UNREDUCE_PUBLIC
# include "expand.c"
#else
# ifdef USE_UNREDUCE_SMITH
#  include "unreduce_full.c"
# else
int dummy_unreduce;     /* Dummy declaration to quiet compilers. */
# endif
#endif
