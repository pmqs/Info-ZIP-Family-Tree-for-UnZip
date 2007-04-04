/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2005-Feb-10 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
    OpenVMS specific configuration section (included by unzpriv.h):
  ---------------------------------------------------------------------------*/

#ifndef __vmscfg_h   /* Prevent (unlikely) multiple inclusions. */
#define __vmscfg_h

/* Accomodation for /NAMES = AS_IS with old header files. */
#define cma$tis_errno_get_addr CMA$TIS_ERRNO_GET_ADDR

/* 2007-02-22 SMS.
 * Enable symbolic links according to the available C RTL support,
 * unless prohibited by the user defining NO_SYMLINKS.
 */
#if !defined(__VAX) && defined(__CRTL_VER) && __CRTL_VER >= 70301000
#  ifndef NO_SYMLINKS
#     define SYMLINKS
#  endif
#endif
#ifdef SYMLINKS
#  include <unistd.h>
#endif

#  include <types.h>                    /* GRR:  experimenting... */
#  include <stat.h>
#  include <time.h>                     /* the usual non-BSD time functions */
#  include <file.h>                     /* same things as fcntl.h has */
#  include <unixio.h>
#  include <rms.h>
   /* Define maximum path length according to NAM member size. */
#  ifndef NAM_MAXRSS
#    define NAM_MAXRSS NAM$C_MAXRSS
#  endif
#  define _MAX_PATH (NAM_MAXRSS+1)      /* to define FILNAMSIZ below */
#  ifdef RETURN_CODES  /* VMS interprets standard PK return codes incorrectly */
#    define RETURN(ret) return_VMS(__G__ (ret))   /* verbose version */
#    define EXIT(ret)   return_VMS(__G__ (ret))
#  else
#    define RETURN      return_VMS                /* quiet version */
#    define EXIT        return_VMS
#  endif
#  ifdef VMSCLI
#    define USAGE(ret)  VMSCLI_usage(__G__ (ret))
#  endif
#  define DIR_BEG       '['
#  define DIR_END       ']'
#  define DIR_EXT       ".dir"
#  ifndef DATE_FORMAT
#    define DATE_FORMAT DF_MDY
#  endif
#  define lenEOL        1
#  define PutNativeEOL  *q++ = native(LF);
#  define SCREENSIZE(ttrows, ttcols)  screensize(ttrows, ttcols)
#  define SCREENWIDTH   80
#  define SCREENLWRAP   screenlinewrap()
#  if (defined(__VMS_VERSION) && !defined(VMS_VERSION))
#    define VMS_VERSION __VMS_VERSION
#  endif
#  if (defined(__VMS_VER) && !defined(__CRTL_VER))
#    define __CRTL_VER __VMS_VER
#  endif
#  if ((!defined(__CRTL_VER)) || (__CRTL_VER < 70000000))
#    define NO_GMTIME           /* gmtime() of earlier VMS C RTLs is broken */
#  else
#    if (!defined(NO_EF_UT_TIME) && !defined(USE_EF_UT_TIME))
#      define USE_EF_UT_TIME
#    endif
#    if (!defined(HAVE_STRNICMP) && !defined(NO_STRNICMP))
#      define HAVE_STRNICMP
#      ifdef STRNICMP
#        undef STRNICMP
#      endif
#      define STRNICMP  strncasecmp
#    endif
#  endif
#  ifndef HAVE_STRNICMP                 /* use our private zstrnicmp() */
#    define NO_STRNICMP                 /*  unless explicitly overridden */
#  endif
#  if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#    define TIMESTAMP
#  endif
#  define RESTORE_UIDGID
   /* VMS is run on little-endian processors with 4-byte ints:
    * enable the optimized CRC-32 code */
#  ifdef IZ_CRC_BE_OPTIMIZ
#    undef IZ_CRC_BE_OPTIMIZ
#  endif
#  if !defined(IZ_CRC_LE_OPTIMIZ) && !defined(NO_CRC_OPTIMIZ)
#    define IZ_CRC_LE_OPTIMIZ
#  endif
#  if !defined(IZ_CRCOPTIM_UNFOLDTBL) && !defined(NO_CRC_OPTIMIZ)
#    define IZ_CRCOPTIM_UNFOLDTBL
#  endif

#  ifdef __DECC
    /* File open callback ID values. */
#   define OPENR_ID 1
    /* File open callback ID storage. */
    extern int openr_id;
    /* File open callback function. */
    extern int acc_cb();
    /* Option macros for open().
     * General: Stream access
     *
     * Callback function (DEC C only) sets deq, mbc, mbf, rah, wbh, ...
     */
#   define OPNZIP_RMS_ARGS "ctx=stm", "acc", acc_cb, &openr_id
#  else /* !__DECC */ /* (So, GNU C, VAX C, ...)*/
#   define OPNZIP_RMS_ARGS "ctx=stm"
#  endif /* ?__DECC */

#endif /* !__vmscfg_h */
