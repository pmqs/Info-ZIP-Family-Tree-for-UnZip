/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
    BeOS specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __beocfg_h
# define __beocfg_h

# include <sys/types.h>          /* [cjh]:  This is pretty much a generic  */
# include <sys/stat.h>           /* POSIX 1003.1 system; see beos/ for     */
# include <fcntl.h>              /* extra code to deal with our extra file */
# include <sys/param.h>          /* attributes. */
# include <unistd.h>
# include <utime.h>
# define GOT_UTIMBUF
# define DIRENT
# include <time.h>
# ifndef DATE_FORMAT
#  define DATE_FORMAT DF_MDY    /* GRR:  customize with locale.h somehow? */
# endif
# define lenEOL          1
# define PutNativeEOL    *q++ = native(LF);
# define SCREENSIZE(ttrows, ttcols)  screensize(ttrows, ttcols)
# define SCREENWIDTH     80
# if (!defined(NO_EF_UT_TIME) && !defined(USE_EF_UT_TIME))
#  define USE_EF_UT_TIME
# endif
# define SET_SYMLINK_ATTRIBS
# define SET_DIR_ATTRIB
 #if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#  define TIMESTAMP
# endif
# define RESTORE_UIDGID
# define NO_STRNICMP             /* not in the x86 headers at least */
# define INT_SPRINTF
# define SYMLINKS
# define MAIN main_stub          /* now that we're using a wrapper... */

/* Static variables that we have to add to Uz_Globs. */

# ifdef KFLAG
#  define UMASK_VAL mode_t umask_val;
# else /* def KFLAG */
#  define UMASK_VAL
# endif /* def KFLAG [else] */

# define SYSTEM_SPECIFIC_GLOBALS \
    int created_dir;\
    int exdir_renamed;\
    char *rootpath, *buildpath, *end;\
    ZCONST char *wildname;\
    char *dirname, matchname[FILNAMSIZ];\
    int rootlen, have_dirname, dirnamelen, notfirstcall;\
    zvoid *wild_dir; \
    UMASK_VAL

/* created_dir, and exdir_renamed are used by both mapname() and       */
/*    checkdir().                                                      */
/* rootlen, rootpath, buildpath and end are used by checkdir().        */
/* wild_dir, dirname, wildname, matchname[], dirnamelen, have_dirname, */
/*    and notfirstcall are used by do_wild().                          */

/* ISO/OEM (iconv) character conversion. */

# ifndef NO_ICONV_MAPPING
#  define ICONV_MAPPING         /* Available by default in Haiku. */
# endif /* def NO_ICONV_MAPPING */

# ifdef ICONV_MAPPING

#  define MAX_CP_NAME 25

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

#endif /* ndef __beocfg_h */
