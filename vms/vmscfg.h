/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
    OpenVMS specific configuration section (included by unzpriv.h):
  ---------------------------------------------------------------------------*/

#ifndef __vmscfg_h   /* Prevent (unlikely) multiple inclusions. */
# define __vmscfg_h

/* 2011-12-26 SMS.
 * Added the whole cma$tis_* set instead of only cma$tis_errno_get_addr.
 * (Needed cma$tis_vmserrno_get_addr for the user-triggered progress
 * messages.)
 *
 * Workaround for broken header files of older DECC distributions
 * that are incompatible with the /NAMES=AS_IS qualifier.
 */
# define cma$tis_errno_get_addr         CMA$TIS_ERRNO_GET_ADDR
# define cma$tis_vmserrno_get_addr      CMA$TIS_VMSERRNO_GET_ADDR
# define cma$tis_errno_set_value        CMA$TIS_ERRNO_SET_VALUE
# define cma$tis_vmserrno_set_value     CMA$TIS_VMSERRNO_SET_VALUE

# define sys$assign     SYS$ASSIGN
# define sys$dassgn     SYS$DASSGN
# define sys$qiow       SYS$QIOW

/* LARGE FILE SUPPORT - 10/6/04 EG */
/* This needs to be set before the includes so they set the right sizes */

# ifdef NO_LARGE_FILE_SUPPORT
#  ifdef LARGE_FILE_SUPPORT
#   undef LARGE_FILE_SUPPORT
#  endif
# endif

# ifdef LARGE_FILE_SUPPORT

#  define _LARGEFILE            /* Define the pertinent macro. */

/* LARGE_FILE_SUPPORT implies ZIP64_SUPPORT,
   unless explicitly disabled by NO_ZIP64_SUPPORT.
*/
#  ifdef NO_ZIP64_SUPPORT
#    ifdef ZIP64_SUPPORT
#      undef ZIP64_SUPPORT
#    endif
#  else
#    ifndef ZIP64_SUPPORT
#      define ZIP64_SUPPORT
#    endif
#  endif

# endif /* def LARGE_FILE_SUPPORT */

/* 2007-02-22 SMS.
 * Enable symbolic links according to the available C RTL support,
 * unless prohibited by the user defining NO_SYMLINKS.
 */
# if !defined(__VAX) && defined(__CRTL_VER) && __CRTL_VER >= 70301000
#  ifndef NO_SYMLINKS
#   define SYMLINKS
#  endif
# endif

# ifdef SYMLINKS
#  include <unistd.h>
# endif

# include <limits.h>                    /* UINT_MAX */
# include <types.h>                     /* GRR:  experimenting... */
# include <stat.h>
# include <time.h>                      /* the usual non-BSD time functions */
# include <file.h>                      /* same things as fcntl.h has */
# include <unixio.h>
# include <rms.h>

/* 2012-09-05 SMS.
 * Judge availability of str[n]casecmp() in C RTL.
 * (Note: This must follow a "#include <decc$types.h>" in something to
 * ensure that __CRTL_VER is as defined as it will ever be.  DEC C on
 * VAX may not define it itself.)
 */
# if __CRTL_VER >= 70000000
#  define HAVE_STRCASECMP
# endif /* __CRTL_VER >= 70000000 */

# ifdef HAVE_STRCASECMP
#  include <strings.h>    /* str[n]casecmp() */
# else /* def HAVE_STRCASECMP */
#  define strcasecmp( s1, s2) strncasecmp( s1, s2, UINT_MAX)
extern int strncasecmp( char *, char *, size_t);
# endif /* def HAVE_STRCASECMP [else] */

/* Define maximum path length according to NAM[L] member size. */
# ifndef NAMX_MAXRSS
#  ifdef NAML$C_MAXRSS
#   define NAMX_MAXRSS NAML$C_MAXRSS
#  else
#   define NAMX_MAXRSS NAM$C_MAXRSS
#  endif
# endif

# define _MAX_PATH (NAMX_MAXRSS+1)      /* to define FILNAMSIZ below */

# ifdef DLL
     /* Return the normal (raw) PK status code. */
#  define RETURN      return
#  define EXIT        return
# else /* def DLL */
   /* Do the desired VMS-specific status code and exit processing. */
#  ifdef RETURN_CODES
    /* Display error message before exiting. */
#   define RETURN(ret) return_VMS(__G__ (ret))
#   define EXIT(ret)   return_VMS(__G__ (ret))
#  else
    /* Exit without extra error message. */
#   define RETURN      return_VMS
#   define EXIT        return_VMS
#  endif
# endif /* def DLL [else] */

# ifdef VMSCLI
#  define USAGE(ret)  VMSCLI_usage(__G__ (ret))
# endif
# define DIR_BEG       '['
# define DIR_END       ']'
# define DIR_EXT       ".dir"
# ifndef UZ_FNFILTER_REPLACECHAR
   /* We use '?' instead of the single char wildcard '%' as "unprintable
    * charcode" placeholder, because '%' is valid for ODS-5 names but '?'
    * is invalid. This choice may allow easier detection of "unprintables"
    * when reading the fnfilter() output.
    */
#  define UZ_FNFILTER_REPLACECHAR  '?'
# endif
# ifndef DATE_FORMAT
#  define DATE_FORMAT DF_MDY
# endif
# define lenEOL        1
# define PutNativeEOL  *q++ = native(LF);
# define SCREENSIZE(ttrows, ttcols)  screensize(ttrows, ttcols)
# define SCREENWIDTH   80
# define SCREENLWRAP   screenlinewrap()
# if (defined(__VMS_VERSION) && !defined(VMS_VERSION))
#  define VMS_VERSION __VMS_VERSION
# endif
# if (defined(__VMS_VER) && !defined(__CRTL_VER))
#  define __CRTL_VER __VMS_VER
# endif
# if ((!defined(__CRTL_VER)) || (__CRTL_VER < 70000000))
#  define NO_GMTIME           /* gmtime() of earlier VMS C RTLs is broken */
# else
#  if (!defined(NO_EF_UT_TIME) && !defined(USE_EF_UT_TIME))
#   define USE_EF_UT_TIME
#  endif
#  if (!defined(HAVE_STRNICMP) && !defined(NO_STRNICMP))
#   define HAVE_STRNICMP
#   ifdef STRNICMP
#    undef STRNICMP
#   endif
#   define STRNICMP  strncasecmp
#  endif
# endif
# ifndef HAVE_STRNICMP                 /* use our private zstrnicmp() */
#  define NO_STRNICMP                 /*  unless explicitly overridden */
# endif
# if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#  define TIMESTAMP
# endif
# define SET_DIR_ATTRIB
# define RESTORE_UIDGID

/* VMS runs on little-endian processors with 4-byte ints.
 * Enable the optimized CRC-32 code.
 */
# ifdef IZ_CRC_BE_OPTIMIZ
#  undef IZ_CRC_BE_OPTIMIZ
# endif
# if !defined(IZ_CRC_LE_OPTIMIZ) && !defined(NO_CRC_OPTIMIZ)
#  define IZ_CRC_LE_OPTIMIZ
# endif
# if !defined(IZ_CRCOPTIM_UNFOLDTBL) && !defined(NO_CRC_OPTIMIZ)
#  define IZ_CRCOPTIM_UNFOLDTBL
# endif

/* Enable "better" unprintable charcodes filtering in fnfilter().
 * (On VMS, the isprint() implementation seems to detect 8-bit printable
 * characters even for the default "C" locale. A previous localization
 * setup by calling setlocale() is not neccessary.)
 */
# if (!defined(NO_WORKING_ISPRINT) && !defined(HAVE_WORKING_ISPRINT))
#  define HAVE_WORKING_ISPRINT
# endif

# ifdef NO_OFF_T
  typedef long zoff_t;
# else
  typedef off_t zoff_t;
# endif
# define ZOFF_T_DEFINED

typedef struct stat z_stat;
# define Z_STAT_DEFINED

# if defined( UNICODE_SUPPORT) && defined( UNICODE_WCHAR)
#  define HAVE_CTYPE_H
#  define HAVE_LOCALE_H
#  define HAVE_WCHAR_H
#  define HAVE_WCTYPE_H
# endif /* defined( UNICODE_SUPPORT) && defined( UNICODE_WCHAR) */

/* ISO/OEM (iconv) character conversion. */
			
# ifdef ICONV_MAPPING   /* Currently defined by the user. */

#  define MAX_CP_NAME 31

#  ifdef SETLOCALE
#   undef SETLOCALE
#  endif
#  define SETLOCALE(category, locale) setlocale(category, locale)
#  include <locale.h>

#  ifdef _ISO_INTERN
#   undef _ISO_INTERN
#  endif
#  define _ISO_INTERN( string) charset_to_intern( string, G.iso_cp)

#  ifdef _OEM_INTERN
#   undef _OEM_INTERN
#  endif
#  ifndef IZ_OEM2ISO_ARRAY
#   define IZ_OEM2ISO_ARRAY
#  endif
#  define _OEM_INTERN( string) charset_to_intern( string, G.oem_cp)

/* Possible "const" type qualifier for arg 2 of iconv(). */
#  ifndef ICONV_ARG2
#   define ICONV_ARG2
#  endif /* ndef ICONV_ARG2 */

# endif /* def ICONV_MAPPING */

# ifdef __DECC

    /* File open callback ID values. */
#  define OPENR_ID 1

    /* File open callback ID storage. */
    extern int openr_id;

    /* File open callback function. */
    extern int acc_cb();

    /* Option macros for open().
     * General: Stream access
     *
     * Callback function (DEC C only) sets deq, mbc, mbf, rah, wbh, ...
     */
#  define OPNZIP_RMS_ARGS "ctx=stm", "acc", acc_cb, &openr_id

#else /* def __DECC */ /* (So, GNU C, VAX C, ...)*/

#  define OPNZIP_RMS_ARGS "ctx=stm"

# endif /* def __DECC [else] */

#endif /* ndef __vmscfg_h */
