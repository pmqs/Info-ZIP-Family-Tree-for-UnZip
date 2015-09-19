/*
  zos/bbunzip.c - UnZip 6

  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-2 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*--------------------------------------------------------------------------

  zos/bbunzip.c

  This source file supports the compilation of the Info-ZIP UnZip utility.

  This enables the InfoZip source code to be used unmodified to build the
  utility.  A single compilation is performed, with each of the required C
  header and source files #included.

  This greatly simplifies the compilation of the z/OS MVS UNZIP executable,
  and results in optimum performance by providing a cheap form of global
  program optimization.

  ---------------------------------------------------------------------------*/
#pragma title ("BBUNZIP -- Info-ZIP UnZip Utility")

#pragma runopts(ENV(MVS),PLIST(MVS), TRAP(OFF), NOEXECOPS,             \
                STACK(64K),                                            \
                HEAP(512K,1M,ANYWHERE,FREE),                           \
                NOTEST(NONE,INSPICD,PROMPT) )

//#pragma strings( readonly )
#pragma csect(code,   "IZ_MAIN")
#pragma csect(static, "IZ_WS")
#pragma csect(test,   "IZ_TEST")

#pragma comment( copyright , "1990-2014 Info-ZIP.  All rights reserved" )

#pragma subtitle( "Load module eye-catchers" )
#pragma page()

/*------------------------------------------------------
| Program build time eye-catchers for executable.      |
------------------------------------------------------*/
static const char * EYECATCHER_EyeCatcher  = "EYE CATCHER" ;
static const char * EYECATCHER_Version     = "06.10.00" ;
static const char * EYECATCHER_Datestamp   = __DATE__ ;
static const char * EYECATCHER_Timestamp   = __TIME__ ;
static const char * EYECATCHER_Filename    = __FILE__ ;

#pragma subtitle ("Build Controls")
#pragma page ()

#define MVS
#define _POSIX_SOURCE
#define IZ_BIGBUILD             /* Single compilation build */

#define PROGRAM_ID   "UNZIP"

#pragma subtitle ("Include Files")
#pragma page ()

   /*------------------------------------------------------+
   | System include files.                                 |
   +------------------------------------------------------*/

   /*-------------------------------------------------------
   | UNZIP utility: mainline                               |
   -------------------------------------------------------*/
#include "unzip.c"

   /*-------------------------------------------------------
   | UNZIP utility: 32-bit CRC routines                    |
   -------------------------------------------------------*/
#include "crc32.c"

   /*-------------------------------------------------------
   | UNZIP utility: decryption routines                    |
   -------------------------------------------------------*/
#include "crypt.c"

   /*-------------------------------------------------------
   | UNZIP utility: Environment variable routines          |
   -------------------------------------------------------*/
#include "envargs.c"

   /*-------------------------------------------------------
   | UNZIP utility: explode impoded data routines          |
   -------------------------------------------------------*/
#include "explode.c"

   /*-------------------------------------------------------
   | UNZIP utility: extract/test ZIPfile members           |
   -------------------------------------------------------*/
#include "extract.c"

   /*-------------------------------------------------------
   | UNZIP utility: file I/O routines                      |
   -------------------------------------------------------*/
#include "fileio.c"

   /*-------------------------------------------------------
   | UNZIP utility: global variable definitions            |
   -------------------------------------------------------*/
#include "globals.c"

   /*-------------------------------------------------------
   | UNZIP utility: inflate routines                       |
   -------------------------------------------------------*/
#include "inflate.c"

   /*-------------------------------------------------------
   | UNZIP utility: list routines                          |
   -------------------------------------------------------*/
#include "list.c"

   /*-------------------------------------------------------
   | UNZIP utility: regular expression routines            |
   -------------------------------------------------------*/
#include "match.c"

   /*-------------------------------------------------------
   | UNZIP utility: multiple-ZIPfile processing routines   |
   -------------------------------------------------------*/
#include "process.c"

   /*-------------------------------------------------------
   | UNZIP utility: tty I/O routines                       |
   -------------------------------------------------------*/
#include "ttyio.c"

   /*-------------------------------------------------------
   | UNZIP utility: unreduce routines (dummy)              |
   -------------------------------------------------------*/
#include "unreduce.c"

   /*-------------------------------------------------------
   | UNZIP utility: unshrink routines                      |
   -------------------------------------------------------*/
#include "unshrink.c"

   /*-------------------------------------------------------
   | UNZIP utility: ZIP file listing routines              |
   -------------------------------------------------------*/
#include "zipinfo.c"

   /*-------------------------------------------------------
   | UNZIP utility: Routines common to VM/CMS and MVS      |
   -------------------------------------------------------*/
#include "vmmvs.c"

#pragma title ("LEQUNZIP -- InfoZip UNZIP Utility")
#pragma subtitle (" ")
