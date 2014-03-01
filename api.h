/*
  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  api.h

  ---------------------------------------------------------------------------*/

#ifndef __API_H
# define __API_H

# ifdef DLL     /* This source file supplies DLL-only interface code. */

#  if !defined( POCKET_UNZIP) && !defined( WINDLL)

#   include <setjmp.h>

extern jmp_buf dll_error_return;
#  endif /* ndef WINDLL */

# endif /* def DLL */

#endif /* ndef __API_H */
