/*
  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  api.c

  This module supplies an UnZip engine for use directly from C/C++
  programs.  The functions are:

    char *UzpFeatures( void)
    ZCONST UzpVer *UzpVersion(void);
    unsigned UzpVersion2(UzpVer2 *version)
    int UzpMain(int argc, char *argv[]);
    int UzpMainI( int argc, char *argv[], UzpCB *init)
    int UzpAltMain(int argc, char *argv[], UzpInit *init);
    int UzpValidate(char *archive, int AllCodes);
    void UzpFreeMemBuffer(UzpBuffer *retstr);
    int UzpUnzipToMemory(char *zip, char *file, UzpOpts *optflgs,
                         UzpCB *UsrFuncts, UzpBuffer *retstr);

  non-WINDLL only (a special WINDLL variant is defined in windll/windll.c):
    int UzpGrep(char *archive, char *file, char *pattern, int cmd, int SkipBin,
                UzpCB *UsrFuncts);

  OS/2 only (for now):
    int UzpFileTree(char *name, cbList(callBack), char *cpInclude[],
          char *cpExclude[]);

  You must define DLL in order to include the API extensions.

  ---------------------------------------------------------------------------*/


#ifdef DLL      /* This source file supplies library interface code. */

# ifdef OS2
#  define  INCL_DOSMEMMGR
#  include <os2.h>
# endif /* def OS2 */

# if defined( UNIX) && defined( __APPLE__)
#  include <unix/macosx.h>
# endif /* defined( UNIX) && defined( __APPLE__) */

# define UNZIP_INTERNAL
# include "unzip.h"
# ifdef WINDLL
#  ifdef POCKET_UNZIP
#   include "wince/intrface.h"
#  else /* def POCKET_UNZIP */
#   include "windll/windll.h"
#  endif /* def POCKET_UNZIP [else] */
# else /* def WINDLL */
#  include "api.h"
# endif /* def WINDLL [else] */
# include "unzvers.h"
# include "crypt.h"

# include <setjmp.h>

# ifndef POCKET_UNZIP           /* WinCE pUnZip defines this elsewhere. */
jmp_buf dll_error_return;
# endif /* def POCKET_UNZIP */

/*---------------------------------------------------------------------------
    Documented API entry points
  ---------------------------------------------------------------------------*/


ZCONST UzpVer * UZ_EXP UzpVersion()
{
    static ZCONST UzpVer version = {    /* Doesn't change between calls. */
        /* structure size */
        UZPVER_LEN,
        /* version flags */
# ifdef BETA
#  ifdef ZLIB_VERSION
        3,
#  else /* def ZLIB_VERSION */
        1,
#  endif /* def ZLIB_VERSION [else] */
# else /* def BETA */
#  ifdef ZLIB_VERSION
        2,
#  else /* def ZLIB_VERSION */
        0,
#  endif /* def ZLIB_VERSION [else] */
# endif /* def BETA [else] */
        /* betalevel and date strings */
        UZ_BETALEVEL, UZ_VERSION_DATE,
        /* zlib_version string */
# ifdef ZLIB_VERSION
        ZLIB_VERSION,
# else /* def ZLIB_VERSION */
        NULL,
# endif /* def ZLIB_VERSION [else] */
        /*== someday each of these may have a separate patchlevel: ==*/
        /* unzip version */
        {UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, 0},
        /* zipinfo version */
        {ZI_MAJORVER, ZI_MINORVER, UZ_PATCHLEVEL, 0},
        /* os2dll version (retained for backward compatibility) */
        {UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, 0},
        /* windll version (retained for backward compatibility)*/
        {UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, 0},
# ifdef OS2DLL
        /* os2dll API minimum compatible version*/
        {UZ_OS2API_COMP_MAJOR, UZ_OS2API_COMP_MINOR, UZ_OS2API_COMP_REVIS, 0}
# else /* def OS2DLL */
#  ifdef WINDLL
        /* windll API minimum compatible version*/
        {UZ_WINAPI_COMP_MAJOR, UZ_WINAPI_COMP_MINOR, UZ_WINAPI_COMP_REVIS, 0}
#  else /* def WINDLL */
        /* generic DLL API minimum compatible version*/
        {UZ_GENAPI_COMP_MAJOR, UZ_GENAPI_COMP_MINOR, UZ_GENAPI_COMP_REVIS, 0}
#  endif /* def WINDLL [else] */
# endif /* def OS2DLL [else] */
    };

    return &version;
}

unsigned UZ_EXP UzpVersion2( OFT( UzpVer2 *)version)
#  ifdef NO_PROTO
    UzpVer2 *version;
#  endif /* def NO_PROTO */
{

    if (version->structlen != sizeof(UzpVer2))
        return sizeof(UzpVer2);

# ifdef BETA
    version->flag = 1;
# else /* def BETA */
    version->flag = 0;
# endif /* def BETA [else] */
    strcpy(version->betalevel, UZ_BETALEVEL);
    strcpy(version->date, UZ_VERSION_DATE);

# ifdef ZLIB_VERSION
    /* Although ZLIB_VERSION is a compile-time constant, we implement an
       "overrun-safe" copy because its actual value is not under our control.
     */
    strncpy(version->zlib_version, ZLIB_VERSION,
            sizeof(version->zlib_version) - 1);
    version->zlib_version[sizeof(version->zlib_version) - 1] = '\0';
    version->flag |= 2;
# else /* def ZLIB_VERSION */
    version->zlib_version[0] = '\0';
# endif /* def ZLIB_VERSION [else] */

    /* someday each of these may have a separate patchlevel: */
    version->unzip.vmajor = UZ_MAJORVER;
    version->unzip.vminor = UZ_MINORVER;
    version->unzip.patchlevel = UZ_PATCHLEVEL;

    version->zipinfo.vmajor = ZI_MAJORVER;
    version->zipinfo.vminor = ZI_MINORVER;
    version->zipinfo.patchlevel = UZ_PATCHLEVEL;

    /* these are retained for backward compatibility only: */
    version->os2dll.vmajor = UZ_MAJORVER;
    version->os2dll.vminor = UZ_MINORVER;
    version->os2dll.patchlevel = UZ_PATCHLEVEL;

    version->windll.vmajor = UZ_MAJORVER;
    version->windll.vminor = UZ_MINORVER;
    version->windll.patchlevel = UZ_PATCHLEVEL;

# ifdef OS2DLL
    /* os2dll API minimum compatible version*/
    version->dllapimin.vmajor = UZ_OS2API_COMP_MAJOR;
    version->dllapimin.vminor = UZ_OS2API_COMP_MINOR;
    version->dllapimin.patchlevel = UZ_OS2API_COMP_REVIS;
# else /* def OS2DLL */
#  ifdef WINDLL
    /* windll API minimum compatible version*/
    version->dllapimin.vmajor = UZ_WINAPI_COMP_MAJOR;
    version->dllapimin.vminor = UZ_WINAPI_COMP_MINOR;
    version->dllapimin.patchlevel = UZ_WINAPI_COMP_REVIS;
#  else /* def WINDLL */
    /* generic DLL API minimum compatible version*/
    version->dllapimin.vmajor = UZ_GENAPI_COMP_MAJOR;
    version->dllapimin.vminor = UZ_GENAPI_COMP_MINOR;
    version->dllapimin.patchlevel = UZ_GENAPI_COMP_REVIS;
#  endif /* def WINDLL [else] */
# endif /* def OS2DLL [else] */
    return 0;
}



char *UzpFeatures()
{
    char *feats;
    char tempstring[ 100];      /* Temporary string storage. */
    char featurelist[ 1000];    /* Feature string storage. */

    /* All features start and end with a semi-colon for easy parsing. */
    strcpy( featurelist, ";");
    tempstring[ 0] = '\0';      /* Avoid "unused" warning. */

# ifdef ACORN_FTYPE_NFS
    strcat( featurelist, "apple_nfrsrc;");
# endif

# if defined( UNIX) && defined( __APPLE__)

#  ifndef APPLE_NFRSRC
     Bad code: error APPLE_NFRSRC not defined.
#  endif
#  if defined( __ppc__) || defined( __ppc64__)
#   if APPLE_NFRSRC
    strcat( featurelist, "apple_nfrsrc;");
#   endif /* APPLE_NFRSRC */
#  else /* defined( __ppc__) || defined( __ppc64__) */
#   if ! APPLE_NFRSRC
    strcat( featurelist, "apple_rsrc;");
#   endif /* ! APPLE_NFRSRC */
#  endif /* defined( __ppc__) || defined( __ppc64__) [else] */

#  ifdef APPLE_XATTR
    strcat( featurelist, "apple_xattr;");
#  endif /* def APPLE_XATTR */

# endif /* defined( UNIX) && defined( __APPLE__) */

# ifdef ASM_CRC
    strcat( featurelist, "asm_crc;");
# endif

# ifdef ASM_INFLATECODES
    strcat( featurelist, "asm_inflatecodes;");
# endif

# ifdef CHECK_VERSIONS
    strcat( featurelist, "check_versions;");
# endif

    strcat( featurelist, "compmethods:store");
# ifdef BZIP2_SUPPORT
    strcat( featurelist, ",bzip2");
# endif
# ifdef DEFLATE_SUPPORT
    strcat( featurelist, ",deflate");
# endif
# ifdef DEFLATE64_SUPPORT
    strcat( featurelist, ",deflate64");
# endif
# ifdef LZMA_SUPPORT
    strcat( featurelist, ",lzma");
# endif
# ifdef PPMD_SUPPORT
    strcat( featurelist, ",ppmd");
# endif
# if defined( USE_UNREDUCE_PUBLIC) || defined( USE_UNREDUCE_SMITH)
    strcat( featurelist, ",unreduce;");
# endif
    strcat( featurelist, ";");

# ifdef IZ_CRYPT_ANY

#  ifdef IZ_CRYPT_TRAD
    strcat( featurelist, "crypt;");
#  endif

#  ifdef IZ_CRYPT_AES_WG
    strcat( featurelist, "crypt_aes_wg;");
#  endif

#  ifdef PASSWD_FROM_STDIN
    strcat( featurelist, "passwd_from_stdin;");
#  endif

# endif /* def IZ_CRYPT_ANY */

# ifdef DEBUG
    strcat( featurelist, "debug;");
# endif

# ifdef DEBUG_TIME
    strcat( featurelist, "debug_time;");
# endif

# ifdef DLL
    strcat( featurelist, "dll;");
# endif

# ifdef DOSWILD
    strcat( featurelist, "doswild;");
# endif

# ifdef ICONV_MAPPING
    strcat( featurelist, "iconv_mapping;");
# endif

# ifdef IZ_HAVE_UXUIDGID
    strcat( featurelist, "iz_have_uxuidgid;");
# endif

# ifdef LARGE_FILE_SUPPORT
    strcat( featurelist, "large_file_support;");
# endif

# ifdef LZW_CLEAN
    strcat( featurelist, "lzw_clean;");
# endif

# ifndef MORE
    strcat( featurelist, "no_more;");
# endif

# ifdef MULT_VOLUME
    strcat( featurelist, "mult_volume;");
# endif

# ifdef NTSD_EAS
    strcat( featurelist, "ntsd_eas;");
# endif

# if defined( WIN32) && defined( NO_W32TIMES_IZFIX)
    strcat( featurelist, "no_w32times_izfix;");
# endif

# ifdef OLD_THEOS_EXTRA
    strcat( featurelist, "old_theos_extra;");
# endif

# ifdef OS2_EAS
    strcat( featurelist, "os2_eas;");
# endif

# ifdef QLZIP
    strcat( featurelist, "qlzip;");
# endif

# ifdef REENTRANT
    strcat( featurelist, "reentrant;");
# endif

# ifdef REGARGS
    strcat( featurelist, "regargs;");
# endif

# ifdef RETURN_CODES
    strcat( featurelist, "return_codes;");
# endif

# ifdef SET_DIR_ATTRIB
    strcat( featurelist, "set_dir_attrib;");
# endif

# ifdef SYMLINKS
    strcat( featurelist, "symlinks;");
# endif

# ifdef TIMESTAMP
    strcat( featurelist, "timestamp;");
# endif

# ifdef UNIXBACKUP
    strcat( featurelist, "unixbackup;");
# endif

# ifdef UNICODE_SUPPORT
    strcat( featurelist, "unicode;");
# endif

# if defined(__DJGPP__) && (__DJGPP__ >= 2)

#  ifdef USE_DJGPP_ENV
    strcat( featurelist, "use_djgpp_env;");
#  endif

#  ifdef USE_DJGPP_GLOB
    strcat( featurelist, "use_djgpp_glob;");
#  endif

# endif /* defined(__DJGPP__) && (__DJGPP__ >= 2) */

# ifdef USE_EF_UT_TIME
    strcat( featurelist, "use_ef_ut_time;");
# endif

# ifdef USE_UNREDUCE_PUBLIC
    strcat( featurelist, "use_unreduce_public;");
# endif
# ifdef USE_UNREDUCE_SMITH
    strcat( featurelist, "use_unreduce_smith;");
# endif

# ifdef USE_VFAT
    strcat( featurelist, "use_vfat;");
# endif

# ifdef USE_ZLIB
    strcat( featurelist, "zlib;");
    sprintf( tempstring, "zlib_version:%s,%s;", ZLIB_VERSION, zlibVersion());
    strcat( featurelist, tempstring);
# endif

# ifdef VMSCLI
    strcat( featurelist, "vmscli;");
# endif

# ifdef VMSWILD
    strcat( featurelist, "vmswild;");
# endif

# ifdef WILD_STOP_AT_DIR
    strcat( featurelist, "wild_stop_at_dir;");
# endif

# ifdef WIN32_WIDE
    strcat( featurelist, "win32_wide;");
# endif

# ifdef ZIP64_SUPPORT
    strcat( featurelist, "zip64;");
# endif

# ifdef NO_ZIPINFO
    strcat( featurelist, "no_zipinfo;");
# endif

    feats = malloc( strlen( featurelist) + 1);
    if (feats != NULL)
    {
        strcpy( feats, featurelist);
    }

    return feats;
}



# ifndef SFX
#  ifndef WINDLL

int UZ_EXP UzpAltMain( OFT( int)argc,
                       OFT( char **)argv,
                       OFT( UzpInit *)init)
#  ifdef NO_PROTO
    int argc;
    char **argv;
    UzpInit *init;
#  endif /* def NO_PROTO */
{
    int r, (*dummyfn)();


    CONSTRUCTGLOBALS();

    if (init->structlen >= (sizeof(ulg) + sizeof(dummyfn)) && init->msgfn)
        G.message = init->msgfn;

    if (init->structlen >= (sizeof(ulg) + 2*sizeof(dummyfn)) && init->inputfn)
        G.input = init->inputfn;

    if (init->structlen >= (sizeof(ulg) + 3*sizeof(dummyfn)) && init->pausefn)
        G.mpause = init->pausefn;

    if (init->structlen >= (sizeof(ulg) + 4*sizeof(dummyfn)) && init->userfn)
        (*init->userfn)();    /* allow void* arg? */

    r = unzip(__G__ argc, argv);
    DESTROYGLOBALS();
    RETURN(r);
}

#  endif /* ndef WINDLL */


int UZ_EXP UzpMainI( OFT( int)argc,
                     OFT( char **)argv,
                     OFT( UzpCB *)init)
#  ifdef NO_PROTO
    int argc;
    char **argv;
    UzpCB *init;
#  endif /* def NO_PROTO */
{
    int r, (*dummyfn)();

    CONSTRUCTGLOBALS();

    if (init->structlen >= (sizeof(ulg) + sizeof(dummyfn)) && init->msgfn)
        G.message = init->msgfn;

    if (init->structlen >= (sizeof(ulg) + 2*sizeof(dummyfn)) && init->inputfn)
        G.input = init->inputfn;

    if (init->structlen >= (sizeof(ulg) + 3*sizeof(dummyfn)) && init->pausefn)
        G.mpause = init->pausefn;

    if (init->structlen >= (sizeof(ulg) + 4*sizeof(dummyfn)) && init->passwdfn)
        G.decr_passwd = init->passwdfn;

    r = unzip(__G__ argc, argv);
    DESTROYGLOBALS();
    RETURN(r);
}



#  ifndef __16BIT__

void UZ_EXP UzpFreeMemBuffer( OFT( UzpBuffer *)retstr)
#  ifdef NO_PROTO
    UzpBuffer *retstr;
#  endif /* def NO_PROTO */
{
    if (retstr != NULL && retstr->strptr != NULL) {
        izu_free(retstr->strptr);
        retstr->strptr = NULL;
        retstr->strlength = 0;
    }
}


#   ifndef WINDLL

static int UzpDLL_Init( OFT( zvoid *)pG,
                        OFT( UzpCB *)UsrFuncts)
#  ifdef NO_PROTO
    zvoid *pG;
    UzpCB *UsrFuncts;
#  endif /* def NO_PROTO */
{
    int (*dummyfn)();

    if (UsrFuncts->structlen >= (sizeof(ulg) + sizeof(dummyfn)) &&
        UsrFuncts->msgfn)
        ((Uz_Globs *)pG)->message = UsrFuncts->msgfn;
    else
        return FALSE;

    if (UsrFuncts->structlen >= (sizeof(ulg) + 2*sizeof(dummyfn)) &&
        UsrFuncts->inputfn)
        ((Uz_Globs *)pG)->input = UsrFuncts->inputfn;

    if (UsrFuncts->structlen >= (sizeof(ulg) + 3*sizeof(dummyfn)) &&
        UsrFuncts->pausefn)
        ((Uz_Globs *)pG)->mpause = UsrFuncts->pausefn;

    if (UsrFuncts->structlen >= (sizeof(ulg) + 4*sizeof(dummyfn)) &&
        UsrFuncts->passwdfn)
        ((Uz_Globs *)pG)->decr_passwd = UsrFuncts->passwdfn;

    if (UsrFuncts->structlen >= (sizeof(ulg) + 5*sizeof(dummyfn)) &&
        UsrFuncts->statrepfn)
        ((Uz_Globs *)pG)->statreportcb = UsrFuncts->statrepfn;

    return TRUE;
}


int UZ_EXP UzpUnzipToMemory( OFT( char *)zip,
                             OFT( char *)file,
                             OFT( UzpOpts *)optflgs,
                             OFT( UzpCB *)UsrFuncts,
                             OFT( UzpBuffer *)retstr)
#  ifdef NO_PROTO
    char *zip;
    char *file;
    UzpOpts *optflgs;
    UzpCB *UsrFuncts;
    UzpBuffer *retstr;
#  endif /* def NO_PROTO */
{
    int r;
#    if defined(WINDLL) && !defined(CRTL_CP_IS_ISO)
    char *intern_zip, *intern_file;
#    endif /* defined(WINDLL) && !defined(CRTL_CP_IS_ISO) */

    CONSTRUCTGLOBALS();
#    if defined(WINDLL) && !defined(CRTL_CP_IS_ISO)
    intern_zip = (char *)izu_malloc(strlen(zip)+1);
    if (intern_zip == NULL) {
       DESTROYGLOBALS();
       return PK_MEM;
    }
    intern_file = (char *)izu_malloc(strlen(file)+1);
    if (intern_file == NULL) {
       DESTROYGLOBALS();
       izu_free(intern_zip);
       return PK_MEM;
    }
    ISO_TO_INTERN(zip, intern_zip);
    ISO_TO_INTERN(file, intern_file);
#     define zip intern_zip
#     define file intern_file
#    endif /* defined(WINDLL) && !defined(CRTL_CP_IS_ISO) */
    /* Copy those options that are meaningful for UzpUnzipToMemory, instead of
     * a simple "memcpy(G.UzO, optflgs, sizeof(UzpOpts));"
     */
    uO.pwdarg = optflgs->pwdarg;
    uO.aflag = optflgs->aflag;
    uO.C_flag = optflgs->C_flag;
    uO.qflag = optflgs->qflag;  /* currently,  overridden in unzipToMemory */

    if (!UzpDLL_Init((zvoid *)&G, UsrFuncts)) {
       DESTROYGLOBALS();
       return PK_BADERR;
    }
    G.redirect_data = 1;

    r = (unzipToMemory(__G__ zip, file, retstr) <= PK_WARN);

    DESTROYGLOBALS();
#    if defined(WINDLL) && !defined(CRTL_CP_IS_ISO)
#     undef file
#     undef zip
    izu_free(intern_file);
    izu_free(intern_zip);
#    endif /* defined(WINDLL) && !defined(CRTL_CP_IS_ISO) */
    if (!r && retstr->strlength) {
       izu_free(retstr->strptr);
       retstr->strptr = NULL;
    }
    return r;
}
#   endif /* ndef WINDLL */
#  endif /* ndef __16BIT__ */



#  ifdef OS2DLL

int UZ_EXP UzpFileTree( OFT( char *)name,
                        OFT( cbList( callBack)),        /* Defective? */
                        OFT( char **)cpInclude,
                        OFT( char **)cpExclude)
#  ifdef NO_PROTO
    char *name;
    cbList(callBack);
    char **cpInclude;
    char **cpExclude;
#  endif /* def NO_PROTO */
{
    int r;

    CONSTRUCTGLOBALS();
    uO.qflag = 2;
    uO.vflag = 1;
    uO.C_flag = 1;
    G.wildzipfn = (char *)izu_malloc( strlen( name)+ 1);
    strcpy( G.wildzipfn, name);
    G.process_all_files = TRUE;
    if (cpInclude) {
        char **ptr = cpInclude;

        while (*ptr != NULL) ptr++;
        G.filespecs = ptr - cpInclude;
        G.pfnames = cpInclude, G.process_all_files = FALSE;
    }
    if (cpExclude) {
        char **ptr = cpExclude;

        while (*ptr != NULL) ptr++;
        G.xfilespecs = ptr - cpExclude;
        G.pxnames = cpExclude, G.process_all_files = FALSE;
    }

    G.processExternally = callBack;
    r = process_zipfiles(__G)==0;
    DESTROYGLOBALS();
    return r;
}

#  endif /* def OS2DLL */
# endif /* ndef SFX */



/*---------------------------------------------------------------------------
    Helper functions
  ---------------------------------------------------------------------------*/


void setFileNotFound(__G)
    __GDEF
{
    G.filenotfound++;
}


# ifndef SFX

int unzipToMemory( __G__ zip, file, retstr)
    __GDEF
    char *zip;
    char *file;
    UzpBuffer *retstr;
{
    int r;
    char *incname[2];

    if ((zip == NULL) || (strlen(zip) > ((WSIZE>>2) - 160)))
        return PK_PARAM;
    if ((file == NULL) || (strlen(file) > ((WSIZE>>2) - 160)))
        return PK_PARAM;

    G.process_all_files = FALSE;
    G.extract_flag = TRUE;
    uO.qflag = 2;
    G.wildzipfn = (char *)izu_malloc( strlen( zip)+ 1);
    strcpy( G.wildzipfn, zip);

    G.pfnames = incname;
    incname[0] = file;
    incname[1] = NULL;
    G.filespecs = 1;

    r = process_zipfiles(__G);
    if (retstr) {
        retstr->strptr = (char *)G.redirect_buffer;
        retstr->strlength = G.redirect_size;
    }
    return r;                   /* returns PK_??? error values */
}

# endif /* ndef SFX */

/*
    With the advent of 64 bit support, for now I am assuming that
    if the size of the file is greater than an unsigned long, there
    will simply not be enough memory to handle it, and am returning
    FALSE.
*/
int redirect_outfile( __G)
     __GDEF
{
# ifdef ZIP64_SUPPORT
    z_uint8 check_conversion;
# endif

    if (G.redirect_size != 0 || G.redirect_buffer != NULL)
        return FALSE;

# ifndef NO_SLIDE_REDIR
    G.redirect_slide = !G.pInfo->textmode;
# endif
# if lenEOL != 1
    if (G.pInfo->textmode) {
        G.redirect_size = (ulg)(G.lrec.ucsize * lenEOL);
        if (G.redirect_size < G.lrec.ucsize)
            G.redirect_size = (ulg)((G.lrec.ucsize > (ulg)-2L) ?
                                    G.lrec.ucsize : -2L);
#  ifdef ZIP64_SUPPORT
        check_conversion = G.lrec.ucsize * lenEOL;
#  endif /* def ZIP64_SUPPORT */
    } else
# endif /* lenEOL != 1 */
    {
        G.redirect_size = (ulg)G.lrec.ucsize;
# ifdef ZIP64_SUPPORT
        check_conversion = (z_uint8)G.lrec.ucsize;
# endif /* def ZIP64_SUPPORT */
    }

# ifdef ZIP64_SUPPORT
    if ((z_uint8)G.redirect_size != check_conversion)
        return FALSE;
# endif /* def ZIP64_SUPPORT */

# ifdef __16BIT__
    if ((ulg)((extent)G.redirect_size) != G.redirect_size)
        return FALSE;
# endif /* def __16BIT__ */
# ifdef OS2
    DosAllocMem((void **)&G.redirect_buffer, G.redirect_size+1,
      PAG_READ|PAG_WRITE|PAG_COMMIT);
    G.redirect_pointer = G.redirect_buffer;
# else /* def OS2 */
    G.redirect_pointer =
      G.redirect_buffer = (uch *)izu_malloc((extent)(G.redirect_size+1));
# endif /* def OS2 [else] */
    if (!G.redirect_buffer)
        return FALSE;
    G.redirect_pointer[G.redirect_size] = '\0';
    return TRUE;
}



#  ifdef NO_PROTO
int writeToMemory( __G__ rawbuf, size)
    __GDEF
    ZCONST uch *rawbuf;
    extent size;
#  else /* def NO_PROTO */
int writeToMemory( __GPRO__ ZCONST uch *rawbuf, extent size)
#  endif /* def NO_PROTO [else] */
{
    int errflg = FALSE;

    if ((uch *)rawbuf != G.redirect_pointer) {
        extent redir_avail = (G.redirect_buffer + G.redirect_size) -
                             G.redirect_pointer;

        /* Check for output buffer overflow */
        if (size > redir_avail) {
           /* limit transfer data to available space, set error return flag */
           size = redir_avail;
           errflg = TRUE;
        }
        memcpy(G.redirect_pointer, rawbuf, size);
    }
    G.redirect_pointer += size;
    return errflg;
}



int close_redirect( __G)
     __GDEF
{
    if (G.pInfo->textmode) {
        *G.redirect_pointer = '\0';
        G.redirect_size = (ulg)(G.redirect_pointer - G.redirect_buffer);
        if ((G.redirect_buffer =
         (uch *)izu_realloc(G.redirect_buffer, G.redirect_size + 1)) == NULL) {
            G.redirect_size = 0;
            return EOF;
        }
    }
    return 0;
}



# ifndef SFX
#  ifndef __16BIT__
#   ifndef WINDLL

/* Purpose: Determine if file in archive contains the string szSearch

   Parameters: archive  = archive name
               file     = file contained in the archive. This cannot be
                          a wildcard to be meaningful
               pattern  = string to search for
               cmd      = 0 - case-insensitive search
                          1 - case-sensitve search
                          2 - case-insensitive, whole words only
                          3 - case-sensitive, whole words only
               SkipBin  = if true, skip any files that have control
                          characters other than CR, LF, or tab in the first
                          100 characters.

   Returns:    TRUE if a match is found
               FALSE if no match is found
               -1 on error

   Comments: This does not pretend to be as useful as the standard
             Unix grep, which returns the strings associated with a
             particular pattern, nor does it search past the first
             matching occurrence of the pattern.
 */

int UZ_EXP UzpGrep( OFT( char *)archive,
                    OFT( char *)file,
                    OFT( char *)pattern,
                    OFT( int) cmd,
                    OFT( int) SkipBin,
                    OFT( UzpCB *)UsrFuncts)
#  ifdef NO_PROTO
    char *archive;
    char *file;
    char *pattern;
    int cmd;
    int SkipBin;
    UzpCB *UsrFuncts;
#  endif /* def NO_PROTO */
{
    int retcode = FALSE, compare;
    ulg i, j, patternLen, buflen;
    char * sz, *p;
    UzpOpts flgopts;
    UzpBuffer retstr;

    memzero(&flgopts, sizeof(UzpOpts));         /* no special options */

    if (!UzpUnzipToMemory(archive, file, &flgopts, UsrFuncts, &retstr)) {
       return -1;   /* not enough memory, file not found, or other error */
    }

    if (SkipBin) {
        if (retstr.strlength < 100)
            buflen = retstr.strlength;
        else
            buflen = 100;
        for (i = 0; i < buflen; i++) {
            if (iscntrl(retstr.strptr[i])) {
                if ((retstr.strptr[i] != 0x0A) &&
                    (retstr.strptr[i] != 0x0D) &&
                    (retstr.strptr[i] != 0x09))
                {
                    /* OK, we now think we have a binary file of some sort */
                    izu_free(retstr.strptr);
                    return FALSE;
                }
            }
        }
    }

    patternLen = strlen(pattern);

    if (retstr.strlength < patternLen) {
        izu_free(retstr.strptr);
        return FALSE;
    }

    sz = izu_malloc(patternLen + 3); /* +2 in case doing whole words only */
    if (cmd > 1) {
        strcpy(sz, " ");
        strcat(sz, pattern);
        strcat(sz, " ");
    } else
        strcpy(sz, pattern);

    if ((cmd == 0) || (cmd == 2)) {
        for (i = 0; i < strlen(sz); i++)
            sz[i] = toupper(sz[i]);
        for (i = 0; i < retstr.strlength; i++)
            retstr.strptr[i] = toupper(retstr.strptr[i]);
    }

    for (i = 0; i < (retstr.strlength - patternLen); i++) {
        p = &retstr.strptr[i];
        compare = TRUE;
        for (j = 0; j < patternLen; j++) {
            /* We cannot do strncmp here, as we may be dealing with a
             * "binary" file, such as a word processing file, or perhaps
             * even a true executable of some sort. */
            if (p[j] != sz[j]) {
                compare = FALSE;
                break;
            }
        }
        if (compare == TRUE) {
            retcode = TRUE;
            break;
        }
    }

    izu_free(sz);
    izu_free(retstr.strptr);

    return retcode;
}
#   endif /* ndef WINDLL */
#  endif /* ndef __16BIT__ */



int UZ_EXP UzpValidate( OFT( char *)archive,
                        OFT( int) AllCodes)
#  ifdef NO_PROTO
    char *archive;
    int AllCodes;
#  endif /* def NO_PROTO */
{
    int retcode;
    CONSTRUCTGLOBALS();

    uO.jflag = 1;
    uO.tflag = 1;
    uO.overwrite_none = 0;
    G.extract_flag = (!uO.zipinfo_mode &&
                      !uO.cflag && !uO.tflag && !uO.vflag && !uO.zflag
#  ifdef TIMESTAMP
                      && !uO.T_flag
#  endif
                     );

    uO.qflag = 2;                        /* turn off all messages */
    G.fValidate = TRUE;

    if (archive == NULL) {      /* something is screwed up:  no filename */
        DESTROYGLOBALS();
        retcode = PK_NOZIP;
        goto exit_retcode;
    }

    if (strlen(archive) >= FILNAMSIZ) {
       /* length of supplied archive name exceed the system's filename limit */
       DESTROYGLOBALS();
       retcode = PK_PARAM;
       goto exit_retcode;
    }

    G.wildzipfn = (char *)izu_malloc( FILNAMSIZ);
    strcpy( G.wildzipfn, archive);
#  if defined(WINDLL) && !defined(CRTL_CP_IS_ISO)
    _ISO_INTERN(G.wildzipfn);
#  endif

#  ifdef WINDLL
    Wiz_NoPrinting(TRUE);
#  endif

    G.process_all_files = TRUE;         /* for speed */

    if (setjmp(dll_error_return) != 0) {
#  ifdef WINDLL
        Wiz_NoPrinting(FALSE);
#  endif
        izu_free(G.wildzipfn);
        DESTROYGLOBALS();
        retcode = PK_BADERR;
        goto exit_retcode;
    }

    retcode = process_zipfiles(__G);

    izu_free(G.wildzipfn);
#  ifdef WINDLL
    Wiz_NoPrinting(FALSE);
#  endif
    DESTROYGLOBALS();

    /* PK_WARN == 1 and PK_FIND == 11. When we are just looking at an
       archive, we should still be able to see the files inside it,
       even if we can't decode them for some reason.

       We also still want to be able to get at files even if there is
       something odd about the zip archive, hence allow PK_WARN,
       PK_FIND, IZ_UNSUP as well as PK_ERR
     */

exit_retcode:
    if (AllCodes)
        return retcode;

    if ((retcode == PK_OK) || (retcode == PK_WARN) || (retcode == PK_ERR) ||
        (retcode == IZ_UNSUP) || (retcode == PK_FIND))
        return TRUE;
    else
        return FALSE;
}

# endif /* ndef SFX */

#else /* def DLL */

/* Dummy declaration to quiet compilers. */
int dummy_api;

#endif /* def DLL [else] */
