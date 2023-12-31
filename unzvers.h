/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
 * unzvers.h (for UnZip) by Info-ZIP.
 */

#ifndef __UNZVERS_H
# define __UNZVERS_H

# ifdef BETA
#  undef BETA                   /* Swap blocks.  Last of define/undef wins. */
#  undef BETA_MSG
# endif

/* Define BETA_MSG to restore the unzip.c:BetaVersion[] message:
 *     [...] NOT FOR GENERAL DISTRIBUTION
 */
# ifndef BETA
#  define BETA                  /* Undefine BETA for public releases. */
#  define BETA_MSG /*_xxx*/     /* Remove "_xxx" to activate the message. */
# endif

/**************************************************/
/****  Also check copyright dates in unzip.c.  ****/
/**************************************************/

# ifdef BETA
#  define UZ_BETALEVEL      "c25-BETA"
#  define UZ_VERSION_DATE   "2018-12-20"        /* Internal beta version. */
# else
#  define UZ_BETALEVEL      ""
#  define UZ_VERSION_DATE   "2018-XX-XX"        /* Official release version. */
# endif

# define UZ_MAJORVER    6               /* UnZip */
# define UZ_MINORVER    1

# define ZI_MAJORVER    UZ_MAJORVER     /* ZipInfo */
# define ZI_MINORVER    UZ_MINORVER

# define UZ_PATCHLEVEL  0

# define UZ_VER_STRING  "6.1c25"        /* Sync with Version numbers! */

# ifndef IZ_COMPANY_NAME
#  define IZ_COMPANY_NAME "Info-ZIP"
# endif

/* The following are obsolete but remain for backward compatibility. */
# if (defined(OS2) || defined(__OS2__))
#  define D2_MAJORVER    UZ_MAJORVER    /* DLL for OS/2 */
#  define D2_MINORVER    UZ_MINORVER
#  define D2_PATCHLEVEL  UZ_PATCHLEVEL
# endif

# define DW_MAJORVER    UZ_MAJORVER     /* DLL for MS Windows */
# define DW_MINORVER    UZ_MINORVER
# define DW_PATCHLEVEL  UZ_PATCHLEVEL

# define WIN_VERSION_DATE  UZ_VERSION_DATE

# define UNZ_DLL_VERSION   UZ_VER_STRING

/* The following version constants specify the UnZip version that introduced
 * the most recent incompatible change (means: change that breaks backward
 * compatibility) of a DLL/Library binary API definition.
 *
 * Currently, UnZip supports three distinct DLL/Library APIs, which each
 * carry their own "compatibility level":
 * a) The "generic" (console-mode oriented) API has been used on UNIX,
 *    for example. This API provides a "callable" interface similar to the
 *    interactive command line of the normal program executables.
 * b) The OS/2-only API provides (additional) functions specially tailored
 *    for interfacing with the REXX shell.
 * c) The Win32 DLL API with a pure binary interface which can be used to
 *    build GUI mode as well as Console mode applications.
 *
 * Whenever a change that breaks backward compatibility gets applied to
 * any of the DLL/Library APIs, the corresponding compatibility level should
 * be synchronized with the current UnZip version numbers.
 */
/* generic DLL API minimum compatible version */
# define UZ_GENAPI_COMP_MAJOR   6
# define UZ_GENAPI_COMP_MINOR   1
# define UZ_GENAPI_COMP_REVIS   0
/* os2dll API minimum compatible version */
# define UZ_OS2API_COMP_MAJOR   6
# define UZ_OS2API_COMP_MINOR   1
# define UZ_OS2API_COMP_REVIS   0
/* windll API minimum compatible version */
# define UZ_WINAPI_COMP_MAJOR   6
# define UZ_WINAPI_COMP_MINOR   1
# define UZ_WINAPI_COMP_REVIS   0

#endif /* ndef __UNZVERS_H */
