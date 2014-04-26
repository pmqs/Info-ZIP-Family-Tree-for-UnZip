/*
  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  unzipstb.c

  Example main program illustrating how to use the UnZip DLL.
  This example acts like the normal UnZip program, but includes some
  DLL version checking and additional diagnostic messages.

  ---------------------------------------------------------------------------*/

#include "unzip.h"
#include "unzvers.h"

#include <stdio.h>
#if defined(MODERN) && !defined(NO_STDDEF_H)
# include <stddef.h>
#endif
#include <io.h>


/* Call-back functions. */

/*
 * MyUzpMsg(): Message output call-back function.
 */

int UZ_EXP MyUzpMsg( zvoid *pG,            /* Ignore (globals pointer). */
                     uch *buf,             /* Text buffer. */
                     ulg size,             /* Text buffer size. */
                     int flag)             /* Flag. */
{
    return write( fileno( stdout), buf, size);
}


#ifdef MY_PW
# define UZP_PW MyUzpPassword
/*
 * MyUzpPassword(): Encryption password call-back function.
 */

int UZ_EXP MyUzpPassword( zvoid *pG,            /* Ignore (globals pointer). */
                          int *rcnt,            /* Retry count. */
                          char *pwbuf,          /* Password buffer. */
                          int size,             /* Password buffer size. */
                          ZCONST char *zfn,     /* Archive name. */
                          ZCONST char *efn)     /* Archive member name. */
{
    fprintf( stderr, "MyUP.  size = %d, zfn: >%s<\n", size, zfn);
    fprintf( stderr, "MyUP.  efn: >%s<\n", efn);
    fprintf( stderr, "MyUP.  *rcnt = %d\n", *rcnt);
    strncpy( pwbuf, "password", size);
    return IZ_PW_ENTERED;
}
#else /* def MY_PW */
# define UZP_PW NULL
#endif /* def MY_PW [else] */


/* Main program entry. */

int main(int argc, char *argv[])
{
    static ZCONST UzpVer *pVersion;     /* no pervert jokes, please... */

    /* Populate the user-supplied call-back function structure. */
    UzpCB user_functions =
     { (sizeof user_functions), MyUzpMsg, NULL, NULL, UZP_PW, NULL };

    pVersion = UzpVersion();

    printf("UnZip DLL example: checking version numbers (DLL is dated %s)\n",
      pVersion->date);
    printf("   UnZip versions:    expecting %u.%u%u, using %u.%u%u%s\n",
      UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, pVersion->unzip.vmajor,
      pVersion->unzip.vminor, pVersion->unzip.patchlevel, pVersion->betalevel);
    printf("   ZipInfo versions:  expecting %u.%u%u, using %u.%u%u\n",
      ZI_MAJORVER, ZI_MINORVER, UZ_PATCHLEVEL, pVersion->zipinfo.vmajor,
      pVersion->zipinfo.vminor, pVersion->zipinfo.patchlevel);

/*
    D2_M*VER and os2dll.* are obsolete, though retained for compatibility:

    printf("   OS2 DLL versions:  expecting %u.%u%u, using %u.%u%u\n",
      D2_MAJORVER, D2_MINORVER, D2_PATCHLEVEL, pVersion->os2dll.vmajor,
      pVersion->os2dll.vminor, pVersion->os2dll.patchlevel);
 */

    if (pVersion->flag & 2)
        printf("   using zlib version %s\n", pVersion->zlib_version);

    /* This example code only uses the dll calls UzpVersion() and
     * UzpMain().  The APIs for these two calls have maintained backward
     * compatibility since at least the UnZip release 5.3 !
     */

#define UZDLL_MINVERS_MAJOR             6
#define UZDLL_MINVERS_MINOR             1
#define UZDLL_MINVERS_PATCHLEVEL        0
    /* This UnZip DLL stub requires a DLL version of at least: */
    if ( (pVersion->unzip.vmajor < UZDLL_MINVERS_MAJOR) ||
         ((pVersion->unzip.vmajor == UZDLL_MINVERS_MAJOR) &&
          ((pVersion->unzip.vminor < UZDLL_MINVERS_MINOR) ||
           ((pVersion->unzip.vminor == UZDLL_MINVERS_MINOR) &&
            (pVersion->unzip.patchlevel < UZDLL_MINVERS_PATCHLEVEL)
           )
          )
         ) )
    {
        printf("  aborting because of too old UnZip DLL version!\n");
        return -1;
    }

    /* In case the offsetof() macro is not supported by some C compiler
       environment, it might be replaced by something like:
         ((extent)(void *)&(((UzpVer *)0)->dllapimin))
     */
    if (pVersion->structlen >=
#if defined(MODERN) && !defined(NO_STDDEF_H)
        ( offsetof(UzpVer, dllapimin)
#else
          ((unsigned)&(((UzpVer *)0)->dllapimin)
#endif
         + sizeof(_version_type) ))
    {
#ifdef OS2DLL
# define UZ_API_COMP_MAJOR              UZ_OS2API_COMP_MAJOR
# define UZ_API_COMP_MINOR              UZ_OS2API_COMP_MINOR
# define UZ_API_COMP_REVIS              UZ_OS2API_COMP_REVIS
#else /* def OS2DLL */
# ifdef WINDLL
#  define UZ_API_COMP_MAJOR             UZ_WINAPI_COMP_MAJOR
#  define UZ_API_COMP_MINOR             UZ_WINAPI_COMP_MINOR
#  define UZ_API_COMP_REVIS             UZ_WINAPI_COMP_REVIS
# else /* def WINDLL */
#  define UZ_API_COMP_MAJOR             UZ_GENAPI_COMP_MAJOR
#  define UZ_API_COMP_MINOR             UZ_GENAPI_COMP_MINOR
#  define UZ_API_COMP_REVIS             UZ_GENAPI_COMP_REVIS
# endif /* def WINDLL [else] */
#endif /* def OS2DLL [else] */
        printf(
          "   UnZip API version: can handle <= %u.%u%u, DLL supplies %u.%u%u\n",
          UZ_API_COMP_MAJOR, UZ_API_COMP_MINOR, UZ_API_COMP_REVIS,
          pVersion->dllapimin.vmajor, pVersion->dllapimin.vminor,
          pVersion->dllapimin.patchlevel);
        if ( (pVersion->dllapimin.vmajor > UZ_API_COMP_MAJOR) ||
             ((pVersion->dllapimin.vmajor == UZ_API_COMP_MAJOR) &&
              ((pVersion->dllapimin.vminor > UZ_API_COMP_MINOR) ||
               ((pVersion->dllapimin.vminor == UZ_API_COMP_MINOR) &&
                (pVersion->dllapimin.patchlevel > UZ_API_COMP_REVIS)
               )
              )
             ) )
        {
            printf("  aborting because of unsupported dll api version!\n");
            return -1;
        }
    }
    printf("\n");

    /* Call the UnZip entry point function, UzpMainI(), passing it an
     * UnZip command expressed as an argument vector.
     *
     * This example emulates the normal UnZip program, so we pass the
     * existing argument vector to the UnZip entry point.  A real
     * application program would probably form an argument vector with
     * some application-generated command.
     */
    return UzpMainI( argc, argv, &user_functions);
}
