/*
  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  izunzip_example.c

  Example main program illustrating how to use the libizunzip object
  library (Unix: libizunzip.a, VMS: LIBIZUNZIP.OLB).

  Basic build procedure, Unix:

    cc izunzip_example.c -IUnZip_Source_Dir -o izunzip_example \
     -LUnzip_Object_Dir -lizunzip

  (On Unix, the default Unzip_Object_Dir is the same as the
  Unzip_Source_Dir.)

  Basic build procedure, VMS:

    cc izunzip_example.c /include = Unzip_Source_Dir
    link izunzip_example.obj, Unzip_Object_Dir:libizunzip.olb /library

  If the UnZip library was built with bzip2 support, then the bzip2
  object library must be added to the link command.

  On Unix, add the appropriate -L (if needed) and -l options (plus any
  other system-specific options which may be needed).  For example:

    cc izunzip_example.c -IUnZip_Source_Dir -o izunzip_example \
     -LUnzip_Object_Dir -LBzip2_Object_Dir -lizunzip -lbz2 -lizunzip

  Additional -L and/or -l options may be needed to supply other external
  libraries, such as iconv or zlib, if these are used.

  On VMS, a link options file, LIB_IZUNZIP.OPT, is generated along with
  the object library (in the same directory), and it can be used to
  simplify the LINK command.  (LIB_IZUNZIP.OPT contains comments showing
  how to define the logical names which it uses.)

    define LIB_IZUNZIP Dir      ! See comments in LIB_IZUNZIP.OPT.
    define LIB_other Dir        ! See comments in LIB_IZUNZIP.OPT.
    link izunzip_example.obj, Unzip_Object_Dir:lib_izunzip.opt /options

  ---------------------------------------------------------------------------*/

#include "unzip.h"              /* UnZip specifics. */

#include <stdio.h>
#include <string.h>


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


/*
 * main(): Example main program.
 */

int main( int argc, char **argv)
{
    char *features;
    int sts;
#ifdef __VMS
    int vsts;
#endif
    ZCONST UzpVer *unzip_ver_p; /* Storage for program version string. */

    /* Populate the user-supplied call-back function structure.
     *
     * See unzip.h for details of the UzpCB structure, and for
     * prototypes of the various call-back functions.  The only
     * call-back function supplied here is the one which returns an
     * encryption password, and it's used only if MY_PW is defined.
     */
    UzpCB user_functions =
     { (sizeof user_functions), NULL, NULL, NULL, UZP_PW, NULL };

    /* Call the UnZip entry point function, UzpMainI(), passing it an
     * UnZip command expressed as an argument vector.
     *
     * This example emulates the normal UnZip program, so we pass the
     * existing argument vector to the UnZip entry point.  A real
     * application program would probably form an argument vector with
     * some application-generated command.
     */
    sts = UzpMainI( argc, argv, &user_functions);

    /* Display the returned status value.
     * On VMS, also get and display the VMS-format status code.
     */
    fprintf( stderr, " Raw sts = %d.\n", sts);
#ifdef __VMS
    vsts = vms_status( sts);
    fprintf( stderr, " VMS sts = %d (%%x%08x).\n", vsts, vsts);
#endif

    /* Get and display the library version. */
    unzip_ver_p = UzpVersion();
    fprintf( stderr, " UnZip version %d.%d%d%s\n",
     unzip_ver_p->unzip.major, unzip_ver_p->unzip.minor,
     unzip_ver_p->unzip.patchlevel, unzip_ver_p->betalevel);

    /* Get and display the library feature list. */
    features = UzpFeatures();
    if (features != NULL)
        fprintf( stderr, " UnZip features: %s\n", features);
}
