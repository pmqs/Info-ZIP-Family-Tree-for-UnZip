/*
  Copyright (c) 2015 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
    Atari ST specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __atari_h
# define __atari_h

# include <time.h>
# include <stat.h>
# include <fcntl.h>
# include <limits.h>
# define SYMLINKS
# define EXE_EXTENSION  ".tos"
# ifndef DATE_FORMAT
#  define DATE_FORMAT  DF_DMY
# endif
# define DIR_END        '/'
# define INT_SPRINTF
# define timezone      _timezone
# define lenEOL        2
# define PutNativeEOL  {*q++ = native(CR); *q++ = native(LF);}
# undef SHORT_NAMES
# if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#   define TIMESTAMP
# endif

#endif /* ndef __atari_h */
