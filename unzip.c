/*
  Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  unzip.c

  UnZip - a zipfile extraction utility.  See below for make
  instructions, or read the comments in Makefile and the various
  Contents files for more detailed explanations.  To report a bug,
  submit a *complete* description via //www.info-zip.org/zip-bug.html;
  include machine type, operating system and version, compiler and
  version, and reasonably detailed error messages or problem report.
  To join Info-ZIP, see the instructions in README.

  UnZip 5.x was a greatly expanded and partially rewritten successor to
  4.x, which in turn was almost a complete rewrite of version 3.x.  For
  a detailed revision history, see UnzpHist.zip at quest.jpl.nasa.gov.
  For a list of the many (near infinite) contributors, see "CONTRIBS" in
  the UnZip source distribution.  (Some of this information is outdated
  and needs to be updated.)

  UnZip 6.0 added support for archives larger than 4 GiB using the Zip64
  extensions as well as support for Unicode information embedded per the
  latest zip standard additions.

  ---------------------------------------------------------------------------

  [from original zipinfo.c]

  This program reads great gobs of totally nifty information, including
  the central directory stuff, from ZIP archives ("zipfiles" for short).
  It started as just a testbed for fooling with zipfiles, but at this
  point it is actually a useful utility.  It also became the basis for
  the rewrite of UnZip (3.16 -> 4.0), using the central directory for
  processing rather than the individual (local) file headers.

  As of ZipInfo v2.0 and UnZip v5.1, the two programs are combined into
  one.  If the executable is named "unzip" (or "unzip.exe", depending),
  it behaves like UnZip by default; if it is named "zipinfo" or "ii", it
  behaves like ZipInfo.  The ZipInfo behavior may also be triggered by
  use of unzip's -Z option; for example:
    unzip -Z [zipinfo_options] archive.zip

  Another dandy product from your buddies at Newtware!

  Author:  Greg Roelofs, newt@pobox.com, http://pobox.com/~newt/
           23 August 1990 -> April 1997

  ---------------------------------------------------------------------------

  Copyrights:  see accompanying file "LICENSE" in UnZip source distribution.
               (This software is free but NOT IN THE PUBLIC DOMAIN.)

  ---------------------------------------------------------------------------*/


#define __UNZIP_C       /* identifies this source module */
#define UNZIP_INTERNAL
#include "unzip.h"      /* includes, typedefs, macros, prototypes, etc. */
#include "crypt.h"
#include "unzvers.h"

#if defined( UNIX) && defined( __APPLE__)
# include "unix/macosx.h"
#endif /* defined( UNIX) && defined( __APPLE__) */

#ifdef IZ_CRYPT_AES_WG
# include "aes_wg/aesopt.h"
# include "aes_wg/iz_aes_wg.h"
#endif /* def IZ_CRYPT_AES_WG */

#if defined( LZMA_SUPPORT) || defined( PPMD_SUPPORT)
# include "szip/SzVersion.h"
#endif /* defined( LZMA_SUPPORT) || defined( PPMD_SUPPORT) */

#ifndef WINDLL_OLD      /* The WINDLL port uses windll/windll.c instead... */

# ifdef DLL
#  ifndef NO_EXCEPT_SIGNALS
#   define NO_EXCEPT_SIGNALS
#  endif /* ndef NO_EXCEPT_SIGNALS */
# endif /* def DLL */

/***************************/
/* Local type declarations */
/***************************/

# if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
typedef struct _sign_info
    {
        struct _sign_info *previous;
        void (*sighandler)(int);
        int sigtype;
    } savsigs_info;
# endif /* (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS)) */

/*******************/
/* Local Functions */
/*******************/

# if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
static int setsignalhandler OF((__GPRO__ savsigs_info **p_savedhandler_chain,
                                int signal_type, void (*newhandler)(int)));
# endif

# ifndef SFX
static void  help_extended      OF((__GPRO));
# endif /* ndef SFX */

# ifdef ENABLE_USER_PROGRESS
#  ifdef VMS
#   define USER_PROGRESS_CLASS extern
#  else /* def VMS */
#   define USER_PROGRESS_CLASS static
#  endif /* def VMS [else] */
USER_PROGRESS_CLASS void user_progress OF((int));
# endif /* def ENABLE_USER_PROGRESS */


/*************/
/* Constants */
/*************/

# include "consts.h" /* All constant global variables are in here. */
                     /* (Non-constant globals were moved to globals.c.) */

/* Constant local variables: */

# if !defined( SFX) || defined( DIAG_SFX)
#  ifndef _WIN32_WCE /* Win CE does not support environment variables */
static ZCONST char Far EnvUnZip[] = ENV_UNZIP;
static ZCONST char Far EnvUnZip2[] = ENV_UNZIP2;
static ZCONST char Far EnvZipInfo[] = ENV_ZIPINFO;
static ZCONST char Far EnvZipInfo2[] = ENV_ZIPINFO2;
#   ifdef RISCOS
static ZCONST char Far EnvUnZipExts[] = ENV_UNZIPEXTS;
#   endif /* RISCOS */
static ZCONST char Far NoMemEnvArguments[] =
 "envargs:  cannot get memory for arguments";
#  endif /* !_WIN32_WCE */
static ZCONST char Far CmdLineParamTooLong[] =
 "error:  command line parameter #%d exceeds internal size limit\n";
# endif /* !defined( SFX) || defined( DIAG_SFX) */

static ZCONST char Far NoMemArgsList[] =
 "error:  no memory for arguments list";

# if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
static ZCONST char Far CantSaveSigHandler[] =
 "error:  cannot save signal handler settings\n";
# endif

# if !defined( SFX) || defined( DIAG_SFX) || defined( SFX_EXDIR)
static ZCONST char Far NotExtracting[] =
 "caution:  not extracting; -d ignored\n";
static ZCONST char Far MustGiveExdir[] =
 "error:  must specify directory to which to extract with -d option\n";
static ZCONST char Far OnlyOneExdir[] =
 "error:  -d option used more than once (only one exdir allowed)\n";
# endif /* !defined( SFX) || defined( DIAG_SFX) || defined( SFX_EXDIR) */

# if (defined(UNICODE_SUPPORT) && !defined(UNICODE_WCHAR))
static ZCONST char Far UTF8EscapeUnSupp[] =
 "warning:  -U \"escape all non-ASCII UTF-8 chars\" is not supported\n";
# endif

# ifdef IZ_CRYPT_ANY
static ZCONST char Far MustGivePasswd[] =
 "error:  must give decryption password with -P option\n";
# endif

# ifndef SFX
static ZCONST char Far Zfirst[] =
 "error:  -Z must be first option for ZipInfo mode (check UNZIP variable?)\n";
static ZCONST char Far BadAutoDestValue[] =
 "error:  option -da/auto-extract-dir value must be 'reuse'\n";
# endif /* ndef SFX */

static ZCONST char Far InvalidOptionsMsg[] = "error:\
  -fn or any combination of -c, -l, -p, -t, -u and -v options invalid\n";
static ZCONST char Far IgnoreOOptionMsg[] =
 "caution:  both -n and -o specified; ignoring -o\n";
static ZCONST char Far BadJunkDirsValue[] =
 "error:  option -j/--junk-dirs value must be numeric\n";

/* usage() strings */
# ifndef SFX
#  ifdef VMS
static ZCONST char Far ExampleCmnt[] = "!";
static ZCONST char Far Example3[] = "vms.c";
static ZCONST char Far Example2[] = "  unzip \"-V\" foo \"Bar\"\
 ! Use quotes to preserve case, unless SET PROC/PARS=EXTE\n";
#  else /* def VMS */
static ZCONST char Far ExampleCmnt[] = "#";
static ZCONST char Far Example3[] = "ReadMe";
#   ifdef RISCOS
static ZCONST char Far Example2[] =
 "  unzip foo -d RAM:$   # Extract all files from foo into RAMDisc\n";
#   else /* def RISCOS */
#    if (defined(OS2) || (defined(DOS_FLX_OS2_W32) && defined(MORE)))
static ZCONST char Far Example2[] =
 "";                /* no room:  too many local3[] items */
#    else /* (defined(OS2) || (defined(DOS_FLX_OS2_W32) && defined(MORE))) */
#     ifdef MACOS
static ZCONST char Far Example2[] = ""; /* not needed */
#     else /* def MACOS */
static ZCONST char Far Example2[] = " \
 unzip -p foo | more  # Pipe contents of foo.zip into program \"more\"\n";
#     endif /* def MACOS [else] */
#    endif /* (defined(OS2) || (defined(DOS_FLX_OS2_W32) && defined(MORE))) [else] */
#   endif /* def RISCOS [else] */
#  endif /* def VMS [else] */

/* local1[]:  command options */
#  if defined(TIMESTAMP)
static ZCONST char Far local1[] =
 "  -T  timestamp archive to latest";
#  else /* !TIMESTAMP */
static ZCONST char Far local1[] = "";
#  endif /* ?TIMESTAMP */

/* local2[] and local3[]:  modifier options */
#  ifdef DOS_FLX_H68_OS2_W32
#   ifdef FLEXOS
static ZCONST char Far local2[] = "";
#   else
static ZCONST char Far local2[] =
 " -$  label removables (-$$ => fixed disks)";
#   endif
#   ifdef OS2
#    ifdef MORE
static ZCONST char Far local3[] = "\
  -X  restore ACLs if supported              -s  spaces in filenames => '_'\n\
                                             -M  pipe through \"more\" pager\n";
#    else
static ZCONST char Far local3[] = " \
 -X  restore ACLs if supported              -s  spaces in filenames => '_'\n\n";
#    endif /* ?MORE */
#   else /* !OS2 */
#    ifdef WIN32
#     ifdef NTSD_EAS
#      ifdef MORE
static ZCONST char Far local3[] = "\
  -X  restore ACLs (-XX => use privileges)   -s  spaces in filenames => '_'\n\
                                             -M  pipe through \"more\" pager\n";
#      else
static ZCONST char Far local3[] = " \
 -X  restore ACLs (-XX => use privileges)   -s  spaces in filenames => '_'\n\n";
#      endif /* ?MORE */
#     else /* !NTSD_EAS */
#      ifdef MORE
static ZCONST char Far local3[] = "\
  -M  pipe through \"more\" pager            \
  -s  spaces in filenames => '_'\n\n";
#      else
static ZCONST char Far local3[] = " \
                                            -s  spaces in filenames => '_'\n\n";
#      endif /* ?MORE */
#     endif /* ?NTSD_EAS */
#    else /* !WIN32 */
#     ifdef MORE
static ZCONST char Far local3[] = "  -\
M  pipe through \"more\" pager              -s  spaces in filenames => '_'\n\n";
#     else /* def MORE */
static ZCONST char Far local3[] = "\
                                             -s  spaces in filenames => '_'\n";
#     endif /* def MORE [else] */
#    endif /* ?WIN32 */
#   endif /* ?OS2 || ?WIN32 */
#  else /* !DOS_FLX_OS2_W32 */
#   ifdef VMS
#    ifdef KFLAG
static ZCONST char Far local2[] = " -X | -k | -ka  restore UIC | prot | ACL";
#    else /* def KFLAG */
static ZCONST char Far local2[] = " -X | -ka  restore UIC | ACL";
#    endif /* def KFLAG [else] */
#    ifdef MORE
static ZCONST char Far local3[] = "\
  -Y  treat \".nnn\" as \";nnn\" version         -2  force ODS2 names\n\
  -D- restore dir (-D: no) times             -M  pipe through \"more\" pager\n\
  (Must quote upper-case options, like \"-V\", unless SET PROC/PARSE=EXTEND.)\
\n";
#    else /* def MORE */
static ZCONST char Far local3[] = "\n\
  -Y  treat \".nnn\" as \";nnn\" version         -2  force ODS2 names\n\
  -D- restore dir (-D: no) times\n\
  (Must quote upper-case options, like \"-V\", unless SET PROC/PARSE=EXTEND.)\
\n\n";
#    endif /* def MORE [else] */
#   else /* !VMS */
#    ifdef ATH_BEO_UNX
#     ifdef KFLAG
static ZCONST char Far local2[] = " -X | -k  restore UID/GID | permissions";
#     else /* def KFLAG */
static ZCONST char Far local2[] = " -X  restore UID/GID info";
#     endif /* def KFLAG [else] */
#     ifdef __APPLE__
#      ifdef MORE
static ZCONST char Far local3[] = "\
  -K  keep setuid/setgid/tacky permissions   -D- restore dir (-D: no) times\n\
  -J  No special AppleDouble file handling\n\
  -Je/-Jf/-Jq/-Jr  ignore extended attrs/Finder info/quarantine/resource fork\
\n\
  -M  pipe through \"more\" pager\n";
#      else /* def MORE */
static ZCONST char Far local3[] = "\
  -K  keep setuid/setgid/tacky permissions   -D- restore dir (-D: no) times\n\
  -J  No special AppleDouble file handling\n\
  -Je/-Jf/-Jq/-Jr  ignore extended attrs/Finder info/quarantine/resource fork\
\n";
#      endif /* def MORE [else] */
#     else /* def __APPLE__ */
#      ifdef MORE
static ZCONST char Far local3[] = "\
  -K  keep setuid/setgid/tacky permissions   -D- restore dir (-D: no) times\n\
  -M  pipe through \"more\" pager\n";
#      else /* def MORE */
static ZCONST char Far local3[] = "\
  -K  keep setuid/setgid/tacky permissions\n";
#      endif /* def MORE [else] */
#     endif /* def __APPLE__ [else] */
#    else /* !ATH_BEO_UNX */
#     ifdef TANDEM
static ZCONST char Far local2[] = "\
 -X  restore Tandem User ID                 -r  remove file extensions\n\
  -b  create 'C' (180) text files          ";
#      ifdef MORE
static ZCONST char Far local3[] = " \
                                            -M  pipe through \"more\" pager\n";
#      else /* def MORE */
static ZCONST char Far local3[] = "\n";
#      endif /* def MORE [else] */
#     else /* !TANDEM */
#      ifdef AMIGA
static ZCONST char Far local2[] = " -N  restore comments as filenotes";
#       ifdef MORE
static ZCONST char Far local3[] = " \
                                            -M  pipe through \"more\" pager\n";
#       else /* def MORE */
static ZCONST char Far local3[] = "\n";
#       endif /* def MORE [else] */
#      else /* !AMIGA */
#       ifdef MACOS
static ZCONST char Far local2[] = " -E  show Mac info during extraction";
static ZCONST char Far local3[] = " \
 -i  ignore filenames in mac extra info     -J  junk (ignore) Mac extra info\n\
\n";
#       else /* !MACOS */
#        ifdef MORE
static ZCONST char Far local2[] = " -M  pipe through \"more\" pager";
static ZCONST char Far local3[] = "\n";
#        else /* def MORE */
static ZCONST char Far local2[] = "";   /* Atari, Mac, CMS/MVS etc. */
static ZCONST char Far local3[] = "";
#        endif /* def MORE [else] */
#       endif /* ?MACOS */
#      endif /* ?AMIGA */
#     endif /* ?TANDEM */
#    endif /* ?ATH_BEO_UNX */
#   endif /* ?VMS */
#  endif /* ?DOS_FLX_OS2_W32 */
# endif /* ndef SFX */

/* UnZip Usage DCL suggestion strings. */
# ifdef VMS
#  ifdef VMSCLI	
#   define USAGE_DCL_U "  DCL: unzip == \"$ dev:[dir]unzip_cli\""
#  else /* def VMSCLI */
#   define USAGE_DCL_U "  DCL: unzip == \"$ dev:[dir]unzip.exe\""
#  endif /* def VMSCLI [else] */
# else /* def VMS */
#  define USAGE_DCL_U  ""
# endif /* def VMS [else] */

/* ZipInfo Usage DCL suggestion and example strings. */
# ifndef NO_ZIPINFO
#  ifdef VMS
#   ifdef VMSCLI	
#    define USAGE_DCL_Z "  DCL: zipinfo == \"$ dv:[dr]unzip_cli /zipi\""
#   else /* def VMSCLI */
#    define USAGE_DCL_Z "  DCL: zipinfo == \"$ dv:[dr]unzip \"\"-Z\"\"\""
#   endif /* def VMSCLI [else] */
static ZCONST char Far ZipInfoExample[] = "* or % (e.g., \"*font-%.zip\")";
#  else /* def VMS */
#   define USAGE_DCL_Z  ""
static ZCONST char Far ZipInfoExample[] = "*, ?, [] (e.g., \"[a-j]*.zip\")";
#  endif /* def VMS [else] */

/* ZipInfo Usage text strings.
 *    Maintain consistency between these strings and the corresponding
 *    VMS CLI strings in vms/cmdline.c.
 */
#  ifdef VMSCLI
/* Used in vms/cmdline.c, so not static in VMS CLI.  "/lic" v. "--lic". */
ZCONST char Far ZipInfoUsageLine1[] = "\
Info-ZIP ZipInfo %s (%s)%s\n\
 Copyright (c) 1990-2017 Info-ZIP.  License: unzip /license\n";
#  else /* def VMSCLI */
static ZCONST char Far ZipInfoUsageLine1[] = "\
Info-ZIP ZipInfo %s (%s)%s\n\
 Copyright (c) 1990-2017 Info-ZIP.  License: unzip --license\n";
#  endif /* def VMSCLI [else] */

static ZCONST char Far ZipInfoUsageLine2[] = "\
Usage: zipinfo [-12smlvChMtTz] file[.zip] [list...] [-x xlist...]\n\
   or: unzip %s-Z%s [-12smlvChMtTz] file[.zip] [list...] [-x xlist...]\n\
\n\
Action: List name, date/time, attribute, size, compression method, etc., for\n\
files in list (excluding those in xlist) from the specified .zip archive(s).\n\
\"file[.zip]\" may be a wildcard name containing %s.\n";

static ZCONST char Far ZipInfoUsageLine3[] = "\n\
Options (main listing format)            -s  short Unix \"ls -l\" format (def.)\
\n\
  -1  filenames ONLY, one per line       -m  medium Unix \"ls -l\" format\n\
  -2  like -1, but allowing -h/-t/-z     -l  long Unix \"ls -l\" format\n\
                                         -v  verbose, multi-page format\n";

static ZCONST char Far ZipInfoUsageLine4[] = "\
Options (modifiers):\n\
  -h  print header line       -t  print totals for listed files or for all\n\
  -z  print zipfile comment   -T  print file times in sortable decimal format\
\n\
  -C  be case-insensitive%s\
  -x xlist  exclude from the listing the filenames in xlist\n";

#  ifdef MORE
static ZCONST char Far ZipInfoUsageLine4m[] =
     "     -M  page output through built-in \"more\"\n";
#  else /*  defMORE */
static ZCONST char Far ZipInfoUsageLine4m[] = "";
#  endif /* def MORE [else] */
# endif /* !NO_ZIPINFO */

# ifdef BETA
#  ifndef VMSCLI
static                  /* Used in vms/cmdline.c, so not static in VMS CLI. */
#  endif /* ndef VMSCLI */
ZCONST char Far BetaVersion[] = "%s\
        THIS IS A BETA VERSION OF UNZIP%s -- NOT FOR GENERAL DISTRIBUTION.\n";
# endif

# ifndef VMSCLI
static                  /* Used in vms/cmdline.c, so not static in VMS CLI. */
# endif /* ndef VMSCLI */
ZCONST char Far UnzipBanner[] =
 "%s %s (%s)  (c) %4.4s Info-ZIP  http://info-zip.org\n";

/* UnZipSFX Usage text strings.
 *    Maintain consistency between these strings and the corresponding
 *    VMS CLI strings in vms/cmdline.c.
 */
# ifdef SFX
static ZCONST char Far UnzipSFXUsage[] = "\n\
Usage: %s%s %s\n\
        [-opts[modifiers]] [members]\n\n";
#  ifdef SFX_EXDIR
static ZCONST char Far UnzipSFXOpts[] =
   " Options (primary mode): -cfptuz  Modifiers: -abCdjLnoPq%sV%s\n";
#  else
static ZCONST char Far UnzipSFXOpts[] =
     " Options (primary mode): -cfptuz  Modifiers: -abCjLnoPq%sV%s\n";
#  endif
#  ifdef VMS
static ZCONST char Far UnzipSFXOptsV[] = "\
 (Must quote upper-case options, like \"-V\", unless SET PROC/PARSE=EXTEND.)\n";
#  endif /* def VMS */
static ZCONST char Far UnzipSFXOpts2[] =
     " License: --license  More info: See UnZip documentation.\n";
# endif /* def SFX */

# if !defined( SFX) || defined( DIAG_SFX)
static ZCONST char Far CompileOptions[] =
 "UnZip special compilation options/features:\n";
static ZCONST char Far CompileOptFormat[] = "        %s\n";
#  ifndef _WIN32_WCE /* Win CE does not support environment variables */
static ZCONST char Far EnvOptions[] =
 "\nUnZip and ZipInfo environment options:\n";
static ZCONST char Far EnvOptFormat[] = "%16s:  %.1024s\n";
#  endif
static ZCONST char Far None[] = "[none]";
#  ifdef ACORN_FTYPE_NFS
static ZCONST char Far AcornFtypeNFS[] = "ACORN_FTYPE_NFS";
#  endif

#  if defined( UNIX) && defined( __APPLE__)
#   ifndef APPLE_NFRSRC
 /* Next "#" indented to accommodate K&R (#error-ignorant) compilers. */
 #   error APPLE_NFRSRC not defined.
#   endif
#   if defined( __ppc__) || defined( __ppc64__)
#    if APPLE_NFRSRC
#     define APPLE_NFRSRC_MSG
static ZCONST char Far AppleNFRSRC[] =
 "APPLE_NFRSRC         (\"/..namedfork/rsrc\" suffix for resource fork)";
#    endif /* APPLE_NFRSRC */
#   else /* defined( __ppc__) || defined( __ppc64__) */
#    if ! APPLE_NFRSRC
#     define APPLE_NFRSRC_MSG
static ZCONST char Far AppleNFRSRC[] =
 "APPLE_NFRSRC         (NOT!  \"/rsrc\" suffix for resource fork)";
#    endif /* ! APPLE_NFRSRC */
#   endif /* defined( __ppc__) || defined( __ppc64__) [else] */
#   ifdef APPLE_XATTR
static ZCONST char Far AppleXATTR[] =
 "APPLE_XATTR          (Apple extended attributes supported)";
#   endif /* def APPLE_XATTR */
#  endif /* defined( UNIX) && defined( __APPLE__) */

#  ifdef ARCHIVE_STDIN
static ZCONST char Far ArchiveStdin[] =
 "ARCHIVE_STDIN        (Allow streaming archive from stdin)";
#  endif
#  ifdef ASM_CRC
static ZCONST char Far AsmCRC[] =
 "ASM_CRC              (Assembly code used for CRC calculation)";
#  endif
#  ifdef ASM_INFLATECODES
static ZCONST char Far AsmInflateCodes[] =
 "ASM_INFLATECODES     (Assembly code used for Inflate)";
#  endif
#  ifdef CHECK_VERSIONS
static ZCONST char Far Check_Versions[] =
 "CHECK_VERSIONS       (Check VMS versions, build v. run-time)";
#  endif
#  if defined( _MSC_VER) && defined( _DEBUG)
static ZCONST char Far WinDebug[] =
 "_DEBUG               (Windows Debug configuration)";
#  endif
#  ifdef DEBUG
static ZCONST char Far UDebug[] = "DEBUG";
#  endif
#  ifdef DEBUG_TIME
static ZCONST char Far DebugTime[] = "DEBUG_TIME";
#  endif
#  ifdef DEFLATE64_SUPPORT
static ZCONST char Far Deflate64_Sup[] =
 "DEFLATE64_SUPPORT    (PKZIP 4.x Deflate64(tm) compression)";
#  endif
#  ifdef DLL
static ZCONST char Far Dll[] =
 "DLL                  (UnZip built as DLL or object library)";
#  endif
#  ifdef DOSWILD
static ZCONST char Far DosWild[] = "DOSWILD";
#  endif
#  ifdef EXDIR_RENAME
static ZCONST char Far ExdirRename[] =
 "EXDIR_RENAME         (\"-d exdir\" applied to user-spec'd rename)";
#  endif
#  ifdef ICONV_MAPPING
static ZCONST char Far Iconv[] =
 "ICONV_MAPPING        (ISO/OEM (iconv, -I/-O) conversion supported)";
#  endif
#  ifdef IZ_HAVE_UXUIDGID
static ZCONST char Far ux_Uid_Gid[] =
 "IZ_HAVE_UXUIDGID     (UID, GID > 16-bit (\"ux\" extra block) supported)";
#  endif
#  ifdef LZW_CLEAN
static ZCONST char Far LZW_Clean[] =
 "LZW_CLEAN            (PKZIP/Zip 1.x Shrink compression not supported)";
#  endif
#  ifndef MORE
static ZCONST char Far No_More[] =
 "NO_MORE              (Built-in -M pager (\"more\") disabled)";
#  endif
#  ifdef NO_ZIPINFO
static ZCONST char Far No_ZipInfo[] =
#   ifdef VMS
 "NO_ZIPINFO           (ZipInfo -Z (/ZIPINFO) mode disabled)";
#   else /* def VMS */
 "NO_ZIPINFO           (ZipInfo -Z mode disabled)";
#   endif /* def VMS */
#  endif
#  ifdef NTSD_EAS
static ZCONST char Far NTSDExtAttrib[] =
 "NTSD_EAS             (Windows NT extended attributes supported)";
#  endif
#  if defined(WIN32) && defined(NO_W32TIMES_IZFIX)
static ZCONST char Far W32NoIZTimeFix[] = "NO_W32TIMES_IZFIX";
#  endif
#  ifdef OLD_THEOS_EXTRA
static ZCONST char Far OldTheosExtra[] =
 "OLD_THEOS_EXTRA      (Handle also old Theos port extra field)";
#  endif
#  ifdef OS2_EAS
static ZCONST char Far OS2ExtAttrib[] =
 "OS2_EAS              (OS/2 extended attributes supported)";
#  endif
#  ifdef QLZIP
static ZCONST char Far SMSExFldOnUnix[] = "QLZIP";
#  endif
#  ifdef REENTRANT
static ZCONST char Far Reentrant[] =
 "REENTRANT            (UnZip built for reentrancy)";
#  endif
#  ifdef REGARGS
static ZCONST char Far RegArgs[] = "REGARGS";
#  endif
#  ifdef RETURN_CODES
static ZCONST char Far Return_Codes[] =
 "RETURN_CODES         (Display error message at exit)";
#  endif
#  ifdef SET_DIR_ATTRIB
static ZCONST char Far SetDirAttrib[] =
 "SET_DIR_ATTRIB       (Setting directory attributes supported)";
#  endif
#  ifdef SYMLINKS
static ZCONST char Far SymLinkSupport[] =
 "SYMLINKS             (Symbolic links supported, if RTL and file sys do)";
#  endif
#  ifdef TIMESTAMP
static ZCONST char Far TimeStamp[] =
 "TIMESTAMP            (Restoring file timestamps supported)";
#  endif
#  ifdef UNIXBACKUP
static ZCONST char Far UnixBackup[] =
 "UNIXBACKUP           (-B creates backup files)";
#  endif
#  ifdef USE_EF_UT_TIME
static ZCONST char Far Use_EF_UT_time[] =
 "USE_EF_UT_TIME       (Use Universal Time, if available)";
#  endif
#  ifndef LZW_CLEAN
static ZCONST char Far Unshrink_Sup[] =
 "UNSHRINK_SUPPORT     (PKZIP/Zip 1.x Shrink compression)";
#  endif
#  ifdef USE_UNREDUCE_PUBLIC
static ZCONST char Far Use_Unreduce[] =
 "USE_UNREDUCE_PUBLIC  (PKZIP 0.9x Reduce compression, public-domain)";
#  else
#   ifdef USE_UNREDUCE_SMITH
static ZCONST char Far Use_Unreduce[] =
 "USE_UNREDUCE_SMITH   (PKZIP 0.9x Reduce compression, proprietary)";
static ZCONST char Far Use_UnreduceSc[] =
 "                     (UnReduce copyright (c) 1989 by S. H. Smith.)";
#   endif
#  endif
#  ifdef UNICODE_SUPPORT
#   ifdef UTF8_MAYBE_NATIVE
#    ifdef UNICODE_WCHAR
       /* direct native UTF-8 check AND charset transform via wchar_t */
static ZCONST char Far Unicode_Sup[] =
 "UNICODE_SUPPORT [wide-chars, char coding: %s] (handle UTF-8 paths)";
#    else
       /* direct native UTF-8 check, only */
static ZCONST char Far Unicode_Sup[] =
 "UNICODE_SUPPORT [char coding: %s] (handle UTF-8 paths)";
#    endif
static ZCONST char Far SysChUTF8[] = "UTF-8";
static ZCONST char Far SysChOther[] = "other";
#   else /* !UTF8_MAYBE_NATIVE */
       /* charset transform via wchar_t, no native UTF-8 support */
static ZCONST char Far Unicode_Sup[] =
 "UNICODE_SUPPORT [wide-chars] (handle UTF-8 paths)";
#   endif /* ?UTF8_MAYBE_NATIVE */
#  endif /* UNICODE_SUPPORT */
#  ifdef WIN32_WIDE
static ZCONST char Far Win32_Wide_Sup[] =
 "WIN32_WIDE           (Wide characters supported)";
#  endif
#  ifdef _MBCS
static ZCONST char Far Have_MBCS_Support[] =
 "MBCS-support         (Multibyte character support, MB_CUR_MAX = %u)";
#  endif
#  ifdef MULT_VOLUME
static ZCONST char Far MultiVol_Sup[] =
 "MULT_VOLUME          (Multi-volume archives supported)";
#  endif
#  ifdef LARGE_FILE_SUPPORT
static ZCONST char Far LFS_Sup[] =
 "LARGE_FILE_SUPPORT   (Large files over 2 GiB supported)";
#  endif
#  ifdef ZIP64_SUPPORT
static ZCONST char Far Zip64_Sup[] =
 "ZIP64_SUPPORT        (Archives using Zip64 for large files supported)";
#  endif
#  if (defined(__DJGPP__) && (__DJGPP__ >= 2))
#   ifdef USE_DJGPP_ENV
static ZCONST char Far Use_DJGPP_Env[] = "USE_DJGPP_ENV";
#   endif
#   ifdef USE_DJGPP_GLOB
static ZCONST char Far Use_DJGPP_Glob[] = "USE_DJGPP_GLOB";
#   endif
#  endif /* __DJGPP__ && (__DJGPP__ >= 2) */
#  ifdef USE_VFAT
static ZCONST char Far Use_VFAT_support[] = "USE_VFAT";
#  endif
#  ifdef USE_ZLIB
static ZCONST char Far UseZlib[] =
 "USE_ZLIB             (Using ZLIB, build ver %s, run-time %s)";
#  endif /* def USE_ZLIB */
#  ifdef BZIP2_SUPPORT
static ZCONST char Far BZip2_Sup[] =
 "BZIP2_SUPPORT        (PKZIP 4.6+, bzip2 lib ver %s)";
#  endif /* def BZIP2_SUPPORT */
#  ifdef LZMA_SUPPORT
static ZCONST char Far LZMA_Sup[] =
 "LZMA_SUPPORT         (PKZIP 6.3+, LZMA compression, ver %s)";
#  endif /* def LZMA_SUPPORT */
#  ifdef PPMD_SUPPORT
static ZCONST char Far PPMD_Sup[] =
 "PPMD_SUPPORT         (PKZIP 6.3+, PPMd compression, ver %s)";
#  endif /* def PPMD_SUPPORT */
#  ifdef VMS_TEXT_CONV
static ZCONST char Far VmsTextConv[] =
 "VMS_TEXT_CONV        (Conversion of VMS var-len rec fmt text supported)";
#  endif
#  ifdef VMSCLI
static ZCONST char Far VmsCLI[] =
 "VMSCLI               (Use VMS command-line interface)";
#  endif
#  ifdef VMSWILD
static ZCONST char Far VmsWild[] =
 "VMSWILD              (Use VMS-style wildcard characters)";
#  endif
#  ifdef WILD_STOP_AT_DIR
static ZCONST char Far WildStopAtDir[] =
 "WILD_STOP_AT_DIR     (Wildcard \"*\" doesn't span \"/\" dir delimiter)";
#  endif
#  ifdef IZ_CRYPT_AES_WG
static ZCONST char Far AesWgEncryptionNotice1[] =
 "\nAES Strong Encryption notice:\n";
static ZCONST char Far AesWgEncryptionNotice2[] =
 "\
        This executable includes 256-bit AES strong encryption and may be\n\
        subject to export restrictions in many countries, including the USA.\n";

    static ZCONST char Far AesWgDecryption[] = "      \
  IZ_CRYPT_AES_WG      (AES encryption (IZ WinZip/Gladman), ver %d.%d%s)\n";
#  endif
#  ifdef IZ_CRYPT_ANY
#   ifdef IZ_CRYPT_TRAD
static ZCONST char Far TraditionalEncryptionNotice1[] =
 "\nTraditional Zip Encryption notice:\n";
static ZCONST char Far TraditionalEncryptionNotice2[] =
 "\
        The traditional zip encryption code of this program is not\n\
        copyrighted, and is put in the public domain.  It was originally\n\
        written in Europe, and, to the best of our knowledge, can be freely\n\
        distributed in both source and object forms from any country,\n\
        including the USA under License Exception TSU of the U.S. Export\n\
        Administration Regulations (section 740.13(e)) of 6 June 2002.\n";
static ZCONST char Far Decryption[] =
 "        IZ_CRYPT_TRAD        (Traditional (weak) encryption, ver %d.%d%s)\n";
static ZCONST char Far CryptDate[] = CR_VERSION_DATE;
#   endif /* def IZ_CRYPT_TRAD */
#   ifdef PASSWD_FROM_STDIN
static ZCONST char Far PasswdStdin[] = "PASSWD_FROM_STDIN";
#   endif
#  endif /* def IZ_CRYPT_ANY */
#  ifndef __RSXNT__
#   ifdef __EMX__
static ZCONST char Far EnvEMX[] = "EMX";
static ZCONST char Far EnvEMXOPT[] = "EMXOPT";
#   endif
#   if (defined(__GO32__) && (!defined(__DJGPP__) || (__DJGPP__ < 2)))
static ZCONST char Far EnvGO32[] = "GO32";
static ZCONST char Far EnvGO32TMP[] = "GO32TMP";
#   endif
#  endif /* !__RSXNT__ */
# endif /* !defined( SFX) || defined( DIAG_SFX) */

/* UnZip Usage text strings.
 *    Maintain consistency between these strings and the corresponding
 *    VMS CLI strings in vms/cmdline.c.
 */
# ifndef SFX
#  ifdef VMSCLI
/* Used in vms/cmdline.c, so not static in VMS CLI.  "/lic" v. "--lic". */
ZCONST char Far UnzipUsageLine1[] = "\
Info-ZIP UnZip %s (%s)%s\n\
 Copyright (c) 1990-2017 Info-ZIP.  License: unzip /license\n";
#  else /* def VMSCLI */
static ZCONST char Far UnzipUsageLine1[] = "\
Info-ZIP UnZip %s (%s)%s\n\
 Copyright (c) 1990-2017 Info-ZIP.  License: unzip --license\n";
#  endif /* def VMSCLI [else] */

static ZCONST char Far UnzipVersionLine[] = "\
 More info: http://info-zip.org  http://info-zip.org/UnZip.html\n\
 Bugs: http://www.info-zip.org/zip-bug.html  See README for details.\n\n";

#  ifdef MACOS
static ZCONST char Far UnzipUsageLine2[] = "\
Usage: unzip %s[-opts[modifiers]] file[.zip] [list] [-d exdir]\n\
 Default action: Extract files in list, to exdir;\n\
  file[.zip] may be a wildcard.  %s\n";
#  else /* !MACOS */
#   ifdef VM_CMS
static ZCONST char Far UnzipUsageLine2[] = "\
Usage: unzip %s[-opts[modifiers]] file[.zip] [list] [-x xlist] [-d fm]\n\
 Default action: Extract files in list, except those in xlist, to disk fm;\
\n  file[.zip] may be a wildcard.  %s\n";
#   else /* !VM_CMS */
static ZCONST char Far UnzipUsageLine2[] = "\
Usage: unzip %s[-opts[modifiers]] file[.zip] [list] [-x xlist] [-d exdir]\n\
 Default action: Extract files in list, except those in xlist, to exdir;\n\
 file[.zip] may be a wildcard.  %s\n";
#   endif /* ?VM_CMS */
#  endif /* ?MACOS */

#  ifdef NO_ZIPINFO
#  define ZIPINFO_MODE_OPTION  ""
static ZCONST char Far ZipInfoMode[] =
 "(ZipInfo mode is disabled in this build.)";
#  else
#  define ZIPINFO_MODE_OPTION  "[-Z] "
static ZCONST char Far ZipInfoMode[] =
 "-Z => ZipInfo mode (\"unzip -Z\" for usage).";
#  endif /* ?NO_ZIPINFO */

#  ifdef MACOS
static ZCONST char Far UnzipUsageLine3[] = "\
Options (primary mode):\n\
  -d  extract files into exdir               -l  list files (short format)\n\
  -f  freshen existing files, create none    -t  test compressed archive data\n\
  -u  update files, create if necessary      -z  display archive comment only\n\
  -v  list verbosely/show version info     %s\n";
#  else /* def MACOS */
#   ifdef VM_CMS
static ZCONST char Far UnzipUsageLine3[] = "\
Options (primary mode):\n\
  -p  extract files to pipe, no messages     -l  list files (short format)\n\
  -f  freshen existing files, create none    -t  test compressed archive data\n\
  -u  update files, create if necessary      -z  display archive comment only\n\
  -v  list verbosely/show version info     %s\n";
#   else /* def VM_CMS */
static ZCONST char Far UnzipUsageLine3[] = "\
Options (primary mode):\n\
  -p  extract files to pipe, no messages     -l  list files (short format)\n\
  -f  freshen existing files, create none    -t  test compressed archive data\n\
  -u  update files, create if necessary      -z  display archive comment only\n\
  -v  list verbosely/show version info     %s\n";
#   endif /* def VM_CMS [else] */
#  endif /* def MACOS [else] */

/* There is not enough space on a standard 80x25 Windows console screen for
 * the additional line advertising the UTF-8 debugging options. This may
 * eventually also be the case for other ports. Probably, the -U option need
 * not be shown on the introductory screen at all. [Chr. Spieler, 2008-02-09]
 *
 * Likely, other advanced options should be moved to an extended help page and
 * the option to list that page put here.  [E. Gordon, 2008-3-16]
 */
#  if (defined(UNICODE_SUPPORT) && !defined(WIN32))
#   ifdef VMS
static ZCONST char Far UnzipUsageLine4[] = "\
Options (modifiers):\n\
  -x  exclude files that follow (in xlist)   -d  extract files into exdir\n\
  -n  never overwrite or make a new version of an existing file\n\
  -o  always make a new version (-oo: overwrite original) of an existing file\n\
  -q  quiet mode (-qq => quieter)            -a  auto-convert any text files\n\
  -j[=N] junk paths (strip all/top-N dirs)   -aa treat ALL files as text\n\
  -U  use escapes for all non-ASCII Unicode  -UU ignore any Unicode fields\n\
  -C  match filenames case-insensitively     -L  make (some) names \
lowercase\n %-42s  -V  retain VMS version numbers\n%s";
#   else /* def VMS */
static ZCONST char Far UnzipUsageLine4[] = "\
Options (modifiers):\n\
  -n  never overwrite existing files         -q  quiet mode (-qq => quieter)\n\
  -o  overwrite files WITHOUT prompting      -a  auto-convert any text files\n\
  -j[=N] junk paths (strip all/top-N dirs)   -aa treat ALL files as text\n\
  -U  use escapes for all non-ASCII Unicode  -UU ignore any Unicode fields\n\
  -C  match filenames case-insensitively     -L  make (some) names \
lowercase\n %-42s  -V  retain VMS version numbers\n%s";
#   endif /* def VMS [else] */
#  else /* (defined(UNICODE_SUPPORT) && !defined(WIN32)) */
#   ifdef VMS
static ZCONST char Far UnzipUsageLine4[] = "\
Options (modifiers):\n\
  -n  never overwrite or make a new version of an existing file\n\
  -o  always make a new version (-oo: overwrite original) of an existing file\n\
  -q  quiet mode (-qq => quieter)            -a  auto-convert any text files\n\
  -j[=N] junk paths (strip all/top-N dirs)   -aa treat ALL files as text\n\
  -C  match filenames case-insensitively     -L  make (some) names \
lowercase\n %-42s  -V  retain VMS version numbers\n%s";
#   else /* def VMS */
static ZCONST char Far UnzipUsageLine4[] = "\
Options (modifiers):\n\
  -n  never overwrite existing files         -q  quiet mode (-qq => quieter)\n\
  -o  overwrite files WITHOUT prompting      -a  auto-convert any text files\n\
  -j[=N] junk paths (strip all/top-N dirs)   -aa treat ALL files as text\n\
  -C  match filenames case-insensitively     -L  make (some) names \
lowercase\n %-42s  -V  retain VMS version numbers\n%s";
#   endif /* def VMS [else] */
#  endif /* (defined(UNICODE_SUPPORT) && !defined(WIN32)) [else] */

static ZCONST char Far UnzipUsageLine5[] = "\
More help: unzip -hh   Examples:\n\
  unzip data1 -x joe   %s Extract all files except joe from archive data1.zip\n\
%s\
  unzip -fo foo %-6s %s Replace quietly existing %s if archive file newer\n";

# endif /* ndef SFX */



/*
 * -------------------------------------------------------
 * Command Line Options
 * -------------------------------------------------------
 *
 * Valid command line options.
 *
 * get_option() uses one of these tables to check if an option is valid,
 * and if it takes a value (also called an option parameter).
 * To add an option to UnZip or ZipInfo, add it to the appropriate
 * table, and add a case in the main switch to handle it.
 *
 *  The fields:
 *      shortopt     - short option name (1 or 2 chars)
 *      longopt      - long option name
 *      value_type   - see unzpriv.h for constants
 *      negatable    - option is negatable with trailing -
 *      ID           - unsigned long int returned for option
 *      name         - short description of option which is
 *                       returned on some errors and when options
 *                       are listed with -so option, can be NULL
 *
 * If shortopt or longopt is not used, then set it to "".
 *
 * Single-character short options use the shortopt character as an ID.
 * Two-character short options use a non-ASCII code (o_XX), defined in
 * unzpriv.h.  (Look for "Option ID".)
 */

/* The tables below are based on the old main command line code, with
 * some changes.
 */

static ZCONST struct option_struct far options_unzip[] =
{
/* UnZip options */

/*   short longopt            value_type        negatable
 *     ID    description
 */
    {"0",  "no-char-set",     o_NO_VALUE,       o_NEGATABLE,
       '0',  "don't map FAT/NTFS names"},
# ifdef VMS
    {"2",  "force-ods2",      o_NO_VALUE,       o_NEGATABLE,
       '2',  "Force ODS2-compliant names."},
# endif
    {"a",  "ascii",           o_NO_VALUE,       o_NEGATABLE,
       'a',  "text convert (EOL char, ASCII->EBCDIC)"},
# if (defined(DLL) && defined(API_DOC))
    {"A",  "api-help",        o_OPTIONAL_VALUE, o_NOT_NEGATABLE,
       'A',  "extended help for API"},
# endif
    {"b",  "binary",          o_NO_VALUE,       o_NEGATABLE,
       'b',  "binary, no ASCII conversions"},
# ifdef UNIXBACKUP
    {"B",  "backup",          o_NO_VALUE,       o_NEGATABLE,
       'B',  "back up existing files"},
# endif
# ifdef CMS_MVS
    {"B",  "cms-mvs-binary",  o_NO_VALUE,       o_NEGATABLE,
       'b',  "CMS/MVS binary"},
# endif
    {"c",  "to-stdout",       o_NO_VALUE,       o_NEGATABLE,
       'c',  "output to stdout"},
# ifdef CMS_MVS
    /* for CMS_MVS map to lower case */
    {"C",  "cms-mvs-lower",   o_NO_VALUE,       o_NEGATABLE,
       'C',  "CMS/MVS lower case"},
# else /* ifdef CMS_MVS */
    {"C",  "ignore-case",     o_NO_VALUE,       o_NEGATABLE,
       'C',  "ignore case"},
# endif /* ifdef CMS_MVS [else] */
# if (!defined(SFX) || defined(SFX_EXDIR))
    {"d",  "extract-dir",     o_REQUIRED_VALUE, o_NEGATABLE,
       'd',  "extraction root directory"},
# endif
# ifndef SFX
    {"da", "auto-extract-dir", o_OPT_EQ_VALUE,  o_NEGATABLE,
       o_da, "automatic extraction root directory"},
# endif /* ndef SFX */
# if (!defined(NO_TIMESTAMPS))
       /* seems the best long option name I can think of */
    {"D",  "dir-timestamps",  o_NO_VALUE,       o_NEGATABLE,
       'D',  "restore no times (-D- = dir and file)"},
# endif
    {"e",  "extract",         o_NO_VALUE,       o_NEGATABLE,
       'e',  "extract (not used?)"},
# ifdef MACOS
    {"E",  "mac-efs",         o_NO_VALUE,       o_NEGATABLE,
       'E',  "show Mac e.f. when restoring"},
# endif
    {"f",  "freshen",         o_NO_VALUE,       o_NEGATABLE,
       'f',  "freshen (extract only newer files)"},
# if (defined(RISCOS) || defined(ACORN_FTYPE_NFS))
    {"F",  "keep-nfs",        o_NO_VALUE,       o_NEGATABLE,
       'F',  "Acorn filetype & NFS extension handling"},
# endif
    {"h",  "help",            o_NO_VALUE,       o_NOT_NEGATABLE,
       'h',  "help"},
    {"hh", "long-help",       o_NO_VALUE,       o_NOT_NEGATABLE,
       o_hh, "long help"},
# ifdef MACOS
    {"i",  "no-mac-ef-names", o_NO_VALUE,       o_NEGATABLE,
       'i',  "ignore filenames stored in Mac ef"},
# endif
# ifdef ICONV_MAPPING
#  ifdef UNIX
    {"I",  "iso-char-set",    o_REQUIRED_VALUE, o_NOT_NEGATABLE,
       'I',  "ISO char set to use"},
#  endif /* def ICONV_MAPPING */
# endif
    {"j",  "junk-dirs",       o_OPT_EQ_VALUE,   o_NEGATABLE,
       'j',  "junk directories, extract names only"},
# ifdef J_FLAG
    {"J",  "junk-attrs",      o_NO_VALUE,       o_NEGATABLE,
       'J',  "Junk AtheOS, BeOS, or MacOS file attrs"},
# endif
# if defined( UNIX) && defined( __APPLE__)
    {"Je", "junk-extattrs",   o_NO_VALUE,       o_NEGATABLE,
       o_Je, "Junk Mac OS X extended attributes"},
    {"Jf", "junk-finder",     o_NO_VALUE,       o_NEGATABLE,
       o_Jf, "Junk Mac OS X Finder info"},
    {"Jq", "junk-qtn",        o_NO_VALUE,       o_NEGATABLE,
       o_Jq, "Junk Mac OS X quarantine"},
    {"Jr", "junk-rsrc",       o_NO_VALUE,       o_NEGATABLE,
       o_Jr, "Junk Mac OS X resource fork"},
# endif /* defined( UNIX) && defined( __APPLE__) */
    {"",   "jar",             o_NO_VALUE,       o_NEGATABLE,
       o_ja, "Treat archive(s) as Java JAR (UTF-8)"},
# ifdef ATH_BEO_UNX
    {"K",  "keep-s-attrs",    o_NO_VALUE,       o_NEGATABLE,
       'K',  "retain SUID/SGID/Tacky attrs"},
# endif
# ifdef KFLAG
    {"k",  "keep-permissions", o_NO_VALUE,      o_NEGATABLE,
       'k',  "retain permissions"},
# endif
# ifdef VMS
    {"ka", "keep-acl",        o_NO_VALUE,       o_NEGATABLE,
       o_ka, "restore (VMS) ACL"},
# endif
# ifndef SFX
    {"l",  "list",            o_NO_VALUE,       o_NEGATABLE,
        'l', "list archive members"},
# endif
# ifndef CMS_MVS
    {"L",  "lowercase-names", o_NO_VALUE,       o_NEGATABLE,
       'L',  "convert (some) names to lower"},
# endif
    {"",   "license",         o_NO_VALUE,   o_NOT_NEGATABLE,
       o_LI, "Info-ZIP license"},
# ifdef MORE
#  ifdef CMS_MVS
    {"m",  "more",            o_NO_VALUE,       o_NEGATABLE,
       'm',  "pipe output through more"},
#  endif
    {"M",  "more",            o_NO_VALUE,       o_NEGATABLE,
       'M',  "pipe output through more"},
# endif /* MORE */
    {"n",  "never-overwrite", o_NO_VALUE,       o_NEGATABLE,
       'n',  "never overwrite files (no prompting)"},
# ifdef AMIGA
    {"N",  "comment-to-note", o_NO_VALUE,       o_NEGATABLE,
       'N',  "restore comments as filenotes"},
# endif
# ifdef ICONV_MAPPING
#  ifdef UNIX
    {"O",  "oem-char-set",    o_REQUIRED_VALUE, o_NOT_NEGATABLE,
       'O',  "OEM char set to use"},
#  endif /* def ICONV_MAPPING */
# endif
    {"o",  "overwrite",       o_NO_VALUE,       o_NEGATABLE,
       'o',  "overwrite files without prompting"},
    {"p",  "pipe-to-stdout",  o_NO_VALUE,       o_NEGATABLE,
       'p',  "pipe extraction to stdout, no messages"},
# ifdef IZ_CRYPT_ANY
    {"P",  "password",        o_REQUIRED_VALUE, o_NEGATABLE,
       'P',  "password"},
# endif
    {"q",  "quiet",           o_NO_VALUE,       o_NEGATABLE,
       'q',  "quiet mode (additional q's => more quiet)"},
# ifdef QDOS
    {"Q",  "QDOS",            o_NO_VALUE,       o_NEGATABLE,
       'Q',  "QDOS flags"},
# endif
# ifdef TANDEM
    {"r",  "remove-exts",     o_NO_VALUE,       o_NEGATABLE,
       'r',  "remove file extensions"},
# endif
    {"s",  "space-to-uscore", o_NO_VALUE,       o_NEGATABLE,
       's',  "spaces to underscores"},
# ifdef VMS
    {"S",  "streamlf",        o_NO_VALUE,       o_NEGATABLE,
       'S',  "VMS extract text as Stream_LF"},
# endif
# ifndef SFX
    {"sc", "show-command",    o_NO_VALUE,       o_NEGATABLE,
       o_sc, "show processed command line and exit"},
#  if !defined( VMS) && defined( ENABLE_USER_PROGRESS)
    {"si", "show-pid",        o_NO_VALUE,       o_NEGATABLE,
       o_si, "show process ID"},
#  endif /* !defined( VMS) && defined( ENABLE_USER_PROGRESS) */
    {"so", "show-options",    o_NO_VALUE,       o_NEGATABLE,
       o_so, "show available options on this system"},
# endif /* ndef SFX */
    {"t",  "test",            o_NO_VALUE,       o_NEGATABLE,
       't',  "test archive"},
# ifdef TIMESTAMP
    {"T",  "timestamp-new",   o_NO_VALUE,       o_NEGATABLE,
       'T',  "timestamp archive same as newest file"},
# endif
    {"u",  "update",          o_NO_VALUE,       o_NEGATABLE,
       'u',  "update (extract only new/newer files)"},
# ifdef UNICODE_SUPPORT
    {"U",  "unicode",         o_NO_VALUE,       o_NEGATABLE,
       'U',  "escape non-ASCII Unicode, disable Unicode"},
# endif /* ?UNICODE_SUPPORT */
# if !defined( SFX) || defined( DIAG_SFX)
    {"v",  "verbose",         o_NO_VALUE,       o_NEGATABLE,
       'v',  "verbose"},
    {"",   "version",         o_NO_VALUE,       o_NEGATABLE,
       o_ve, "version"},
    {"",   "version",         o_NO_VALUE,       o_NEGATABLE,
       o_ve, "version"},
    {"vq", "quick-version",   o_NO_VALUE,       o_NOT_NEGATABLE,
       o_vq, "show brief/quick version"},
# endif
# ifndef CMS_MVS
    {"V",  "keep-versions",  o_NO_VALUE,       o_NEGATABLE,
       'V',  "don't strip VMS version numbers"},
# endif
# ifdef WILD_STOP_AT_DIR
    {"W",  "wild-no-span",    o_NO_VALUE,       o_NEGATABLE,
       'W',  "wildcard * doesn't span /"},
# endif
    {"x",  "exclude",         o_VALUE_LIST,     o_NOT_NEGATABLE,
       'x',  "exclude this list of files"},
# if (defined(RESTORE_UIDGID) || defined(RESTORE_ACL))
    {"X",  "restore-owner",   o_NO_VALUE,       o_NEGATABLE,
       'X',  "restore owner/group (UID/GID, UIC, ...)"},
# endif
# ifdef VMS
    {"Y",  "dot-version",     o_NO_VALUE,       o_NEGATABLE,
       'Y',  "VMS treat .nnn as ;nnn version"},
# endif
    {"z",  "zipfile-comment", o_NO_VALUE,       o_NEGATABLE,
       'z',  "show zipfile comment"},
# if !defined( SFX) && !defined( NO_ZIPINFO)
    {"Z",  "zipinfo-mode",    o_NO_VALUE,       o_NOT_NEGATABLE,
       'Z',  "ZipInfo mode (must be first option)"},
# endif
# ifdef RISCOS
    {"/",  "extensions",      o_REQUIRED_VALUE, o_NEGATABLE,
       '/',  "override Unzip$Exts"},
# endif
# ifdef VOLFLAG
    {"$",  "volume-labels",   o_NO_VALUE,       o_NEGATABLE,
       '$',  "extract volume labels"},
# endif
# if (!defined(RISCOS) && !defined(CMS_MVS) && !defined(TANDEM))
    {":",  "do-double-dots",  o_NO_VALUE,       o_NEGATABLE,
       ':',  "don't skip ../ path elements"},
# endif
# ifdef UNIX
    {"^",  "control-in-name", o_NO_VALUE,       o_NEGATABLE,
       '^',  "allow control chars in filenames"},
# endif
    /* Array terminator. */
    {NULL, NULL,              o_NO_VALUE,       o_NOT_NEGATABLE,
       0,    NULL} /* end has option_ID = 0 */
};

# ifndef NO_ZIPINFO

/* ZipInfo options */

static ZCONST struct option_struct far options_zipinfo[] =
{
/*   short longopt            value_type        negatable
 *     ID    description
 */
    {"1",  "names-only",      o_NO_VALUE,       o_NEGATABLE,
       '1',  "names-only list"},
    {"2",  "names-mostly",    o_NO_VALUE,       o_NEGATABLE,
       '2',  "names-mostly list"},
#  ifndef CMS_MVS
    {"C",  "ignore-case",     o_NO_VALUE,       o_NEGATABLE,
       'C',  "ignore case"},
#  endif
    {"h",  "header",          o_NO_VALUE,       o_NEGATABLE,
       'h',  "header line"},
#  ifdef ICONV_MAPPING
#   ifdef UNIX
    {"I",  "iso-char-set",    o_REQUIRED_VALUE, o_NOT_NEGATABLE,
       'I',  "ISO charset to use"},
#   endif /* def ICONV_MAPPING */
#  endif
    {"l",  "long-list",       o_NO_VALUE,       o_NEGATABLE,
       'l',  "long list"},
    {"",   "license",         o_NO_VALUE,   o_NOT_NEGATABLE,
       o_LI, "Info-ZIP license"},
    {"m",  "medium-list",     o_NO_VALUE,       o_NEGATABLE,
       'm',  "medium list"},
#  ifdef MORE
    {"M",  "more",            o_NO_VALUE,       o_NEGATABLE,
       'M',  "output like more"},
# endif
    {"mc", "member-counts",   o_NO_VALUE,       o_NEGATABLE,
       o_mc, "show separate dir/file/link member counts"},
# ifdef ICONV_MAPPING
#  ifdef UNIX
    {"O",  "oem-char-set",    o_REQUIRED_VALUE, o_NOT_NEGATABLE,
       'O',  "OEM charset to use"},
#  endif /* def ICONV_MAPPING */
# endif
    {"s",  "short-list",      o_NO_VALUE,       o_NEGATABLE,
       's',  "short list"},
# ifndef SFX
    {"sc", "show-command",    o_NO_VALUE,       o_NEGATABLE,
       o_sc, "show processed command line and exit"},
    {"so", "show-options",    o_NO_VALUE,       o_NEGATABLE,
       o_so, "show available options on this system"},
# endif /* ndef SFX */
    {"t",  "totals",          o_NO_VALUE,       o_NEGATABLE,
       't',  "totals line"},
    {"T",  "decimal-time",    o_NO_VALUE,       o_NEGATABLE,
       'T',  "decimal time format"},
#  ifdef UNICODE_SUPPORT
    {"U",  "unicode",         o_NO_VALUE,       o_NEGATABLE,
       'U',  "escape non-ASCII Unicode, disable Unicode"},
#  endif
    {"v",  "verbose",         o_NO_VALUE,       o_NEGATABLE,
       'v',  "very detailed list"},
    {"",   "version",         o_NO_VALUE,       o_NEGATABLE,
       o_ve, "version"},
    {"vq", "quick-version",   o_NO_VALUE,       o_NOT_NEGATABLE,
       o_vq, "show brief/quick version"},
#  ifdef WILD_STOP_AT_DIR
    {"W",  "wild-no-span",    o_NO_VALUE,       o_NEGATABLE,
       'W',  "wildcard * doesn't span /"},
#  endif
    {"x",  "exclude",         o_VALUE_LIST,     o_NOT_NEGATABLE,
       'x',  "exclude this list of files"},
    {"z",  "zipfile-comment", o_NO_VALUE,       o_NEGATABLE,
       'z',  "print zipfile comment"},
    {"Z",  "zipinfo-mode",    o_NO_VALUE,       o_NEGATABLE,
       'Z',  "ZipInfo mode"},

    /* Array terminator. */
    {NULL, NULL,              o_NO_VALUE,       o_NOT_NEGATABLE,
       0,    NULL} /* end has option_ID = 0 */
};
# endif /* ndef NO_ZIPINFO */


/*****************************/
/*  main() / UzpMain() stub  */
/*****************************/

int MAIN(argc, argv)   /* return PK-type error code (except under VMS) */
    int argc;
    char *argv[];
{
    int r;

    CONSTRUCTGLOBALS();

/* Microsoft memory allocation debug.
 * Enable dump of memory leaks on program exit.
 */
# if defined(_MSC_VER) && defined(_DEBUG) && !defined( NO_IZ_DEBUG_ALLOC)
    _CrtSetDbgFlag(
     _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF);
# endif

    r = unzip(__G__ argc, argv);
    DESTROYGLOBALS();
    RETURN(r);
}


/*******************************/
/*  Primary UnZip entry point  */
/*******************************/

int unzip(__G__ argc, argv)
    __GDEF
    int argc;
    char *argv[];
{
/* Ignore argv[0] for DLL or object library.
 * (Must use "-Z" for ZipInfo mode.)
 */
#  ifdef DLL
#   ifndef IGNORE_ARGV0
#    define IGNORE_ARGV0
#   endif
#  endif

# if !defined( NO_ZIPINFO) && !defined( IGNORE_ARGV0)
    char *p;            /* Temp character pointer for argv[0]. */
# endif

# if (defined(DOS_FLX_H68_NLM_OS2_W32) || !defined(SFX))
    int i;
# endif
    int retcode;
    int error = FALSE;

# ifndef NO_EXCEPT_SIGNALS
#  ifdef REENTRANT
    savsigs_info *oldsighandlers = NULL;
#   define SET_SIGHANDLER(sigtype, newsighandler) \
      if ((retcode = setsignalhandler(__G__ &oldsighandlers, (sigtype), \
                                      (newsighandler))) > PK_WARN) \
          goto cleanup_and_exit
#  else
#   define SET_SIGHANDLER(sigtype, newsighandler) \
      signal((sigtype), (newsighandler))
#  endif
# endif /* NO_EXCEPT_SIGNALS */

# ifdef DLL
  /* Verify NULL termination of (user-supplied) argv[]. */
  if (argv[ argc] != NULL)
  {
    retcode = PK_PARAM;
    goto cleanup_and_exit;
  }
# endif /* def DLL */

# ifdef ENABLE_USER_PROGRESS
#  ifdef VMS
  establish_ctrl_t( user_progress);
#  else /* def VMS */
#   ifdef SIGUSR1
  signal( SIGUSR1, user_progress);
#   endif /* def SIGUSR1 */
#  endif /* def VMS [else] */
# endif /* def ENABLE_USER_PROGRESS */

    /* initialize international char support to the current environment */
    SETLOCALE(LC_CTYPE, "");

# ifdef UNICODE_SUPPORT
    /* see if can use UTF-8 Unicode locale */
#  ifdef UTF8_MAYBE_NATIVE
    {
        char *codeset;
#   if !(defined(NO_NL_LANGINFO) || defined(NO_LANGINFO_H))
        /* get the codeset (character set encoding) currently used */
#       include <langinfo.h>

        codeset = nl_langinfo(CODESET);
#   else /* NO_NL_LANGINFO || NO_LANGINFO_H */
        /* query the current locale setting for character classification */
        codeset = setlocale(LC_CTYPE, NULL);
        if (codeset != NULL) {
            /* extract the codeset portion of the locale name */
            codeset = strchr(codeset, '.');
            if (codeset != NULL) ++codeset;
        }
#   endif /* ?(NO_NL_LANGINFO || NO_LANGINFO_H) */
        /* is the current codeset UTF-8 ? */
        if ((codeset != NULL) && (strcmp(codeset, "UTF-8") == 0)) {
            /* successfully found UTF-8 char coding */
            G.native_is_utf8 = TRUE;
        } else {
            /* Current codeset is not UTF-8 or cannot be determined. */
            G.native_is_utf8 = FALSE;
        }
        /* Note: At least for UnZip, trying to change the process codeset to
         *       UTF-8 does not work.  For the example Linux setup of the
         *       UnZip maintainer, a successful switch to "en-US.UTF-8"
         *       resulted in garbage display of all non-basic ASCII characters.
         */
        /* On openSUSE 11.3 it appears that the console is UTF-8 aware by
         * default.  Makes using UTF-8 aware console applications, like
         * UnZip, easy.  Supplying UTF-8 arguments seems to work correctly,
         * allowing input patterns to include Japanese directly, for instance.
         */
    }
#  endif /* UTF8_MAYBE_NATIVE */

    /* initialize Unicode */
    G.unicode_escape_all = 0;
    G.unicode_mismatch = 0;

    G.unipath_version = 0;
    G.unipath_checksum = 0;
    G.unipath_filename = NULL;

#  ifdef WIN32_WIDE
#   ifdef DYNAMIC_WIDE_NAME
    G.unipath_widefilename = NULL;
#   else /* def DYNAMIC_WIDE_NAME */
    *G.unipath_widefilename = L'\0';
#   endif /* def DYNAMIC_WIDE_NAME [else] */

    G.has_win32_wide = has_win32_wide();
#  endif

# endif /* UNICODE_SUPPORT */


# ifdef ICONV_MAPPING
#  ifdef UNIX
    init_conversion_charsets( __G);
#  endif
# endif /* def ICONV_MAPPING */

# if (defined(__IBMC__) && defined(__DEBUG_ALLOC__))
    extern void DebugMalloc(void);

    atexit(DebugMalloc);
# endif

# ifdef MALLOC_WORK
    /* The following (rather complex) expression determines the allocation
       size of the decompression work area.  It simulates what the
       combined "union" and "struct" declaration of the "static" work
       area reservation achieves automatically at compile time.
       Any decent compiler should evaluate this expression completely at
       compile time and provide constants to the zcalloc() call.
       (For better readability, some subexpressions are encapsulated
       in temporarly defined macros.)
     */
#   define UZ_SLIDE_CHUNK (sizeof(shrint)+sizeof(uch)+sizeof(uch))
#   define UZ_NUMOF_CHUNKS \
      (unsigned)(((WSIZE+UZ_SLIDE_CHUNK-1)/UZ_SLIDE_CHUNK > HSIZE) ? \
                 (WSIZE+UZ_SLIDE_CHUNK-1)/UZ_SLIDE_CHUNK : HSIZE)
    G.area.Slide = (uch *)zcalloc(UZ_NUMOF_CHUNKS, UZ_SLIDE_CHUNK);
#   undef UZ_SLIDE_CHUNK
#   undef UZ_NUMOF_CHUNKS
    G.area.shrink.Parent = (shrint *)G.area.Slide;
    G.area.shrink.value = G.area.Slide + (sizeof(shrint)*(HSIZE));
    G.area.shrink.Stack = G.area.Slide +
                           (sizeof(shrint) + sizeof(uch))*(HSIZE);
# endif

/*---------------------------------------------------------------------------
    Set signal handler for restoring echo, warn of zipfile corruption, etc.
  ---------------------------------------------------------------------------*/
# ifndef NO_EXCEPT_SIGNALS
#  ifdef SIGINT
    SET_SIGHANDLER(SIGINT, handler);
#  endif
#  ifdef SIGTERM                 /* some systems really have no SIGTERM */
    SET_SIGHANDLER(SIGTERM, handler);
#  endif
#  if defined(SIGABRT) && !(defined(AMIGA) && defined(__SASC))
    SET_SIGHANDLER(SIGABRT, handler);
#  endif
#  ifdef SIGBREAK
    SET_SIGHANDLER(SIGBREAK, handler);
#  endif
#  ifdef SIGBUS
    SET_SIGHANDLER(SIGBUS, handler);
#  endif
#  ifdef SIGILL
    SET_SIGHANDLER(SIGILL, handler);
#  endif
#  ifdef SIGSEGV
    SET_SIGHANDLER(SIGSEGV, handler);
#  endif
# endif /* NO_EXCEPT_SIGNALS */

# if (defined(WIN32) && defined(__RSXNT__))
    for (i = 0 ; i < argc; i++) {
        _ISO_INTERN(argv[i]);
    }
# endif

/*---------------------------------------------------------------------------
    Macintosh initialization code.
  ---------------------------------------------------------------------------*/

# ifdef MACOS
    {
        int a;

        for (a = 0;  a < 4;  ++a)
            G.rghCursor[a] = GetCursor(a+128);
        G.giCursor = 0;
    }
# endif

/*---------------------------------------------------------------------------
    NetWare initialization code.
  ---------------------------------------------------------------------------*/

# ifdef NLM
    InitUnZipConsole();
# endif

/*---------------------------------------------------------------------------
    Acorn RISC OS initialization code.
  ---------------------------------------------------------------------------*/

# ifdef RISCOS
    set_prefix();
# endif

/*---------------------------------------------------------------------------
    Theos initialization code.
  ---------------------------------------------------------------------------*/

# ifdef THEOS
    /* The easiest way found to force creation of libraries when selected
     * members are to be unzipped. Explicitly add libraries names to the
     * arguments list before the first member of the library.
     */
    if (! _setargv(&argc, &argv)) {
        Info(slide, 0x401, ((char *)slide, "cannot process argv\n"));
        retcode = PK_MEM;
        goto cleanup_and_exit;
    }
# endif

/*---------------------------------------------------------------------------
    VMS initialization code.
  ---------------------------------------------------------------------------*/

# ifdef VMS
    /* Get the RMS default protections for mapattr() and VMS-specific
     * file and directory protection-setting.
     */
    get_rms_fileprot();
# endif

/*---------------------------------------------------------------------------
    Sanity checks.  Commentary by Otis B. Driftwood and Fiorello:

    D:  It's all right.  That's in every contract.  That's what they
        call a sanity clause.

    F:  Ha-ha-ha-ha-ha.  You can't fool me.  There ain't no Sanity
        Claus.
  ---------------------------------------------------------------------------*/

# ifdef LARGE_FILE_SUPPORT
  /* test if we can support large files - 10/6/04 EG */
    if (sizeof(zoff_t) < 8) {
        Info(slide, 0x401, ((char *)slide,
         "LARGE_FILE_SUPPORT set but not supported\n"));
        retcode = PK_COMPERR;
        goto cleanup_and_exit;
    }
    /* test if we can show 64-bit values */
    {
        zoff_t z = ~(zoff_t)0;  /* z should be all 1s now */
        char *sz;

        sz = FmZofft(z, FZOFFT_HEX_DOT_WID, "X");
        if ((sz[0] != 'F') || (strlen(sz) != 16))
        {
            z = 0;
        }

        /* shift z so only MSB is set */
        z <<= 63;
        sz = FmZofft(z, FZOFFT_HEX_DOT_WID, "X");
        if ((sz[0] != '8') || (strlen(sz) != 16))
        {
            Info(slide, 0x401, ((char *)slide,
              "Can't show 64-bit values correctly\n"));
            retcode = PK_COMPERR;
            goto cleanup_and_exit;
        }
    }
# endif /* LARGE_FILE_SUPPORT */

# ifdef IZ_CRYPT_AES_WG
    /* Verify the AES compile-time endian decision. */
    {
        union {
            unsigned int i;
            unsigned char b[ 4];
        } bi;

#  ifndef PLATFORM_BYTE_ORDER
#   define ENDI_BYTE 0x00
#   define ENDI_PROB "(Undefined)"
#  else
#   if PLATFORM_BYTE_ORDER == AES_LITTLE_ENDIAN
#    define ENDI_BYTE 0x78
#    define ENDI_PROB "Little"
#   else
#    if PLATFORM_BYTE_ORDER == AES_BIG_ENDIAN
#     define ENDI_BYTE 0x12
#     define ENDI_PROB "Big"
#    else
#     define ENDI_BYTE 0xff
#     define ENDI_PROB "(Unknown)"
#    endif
#   endif
#  endif

        bi.i = 0x12345678;
        if (bi.b[ 0] != ENDI_BYTE)
        {
            Info(slide, 0x401, ((char *)slide,
             "Bad AES_WG compile-time endian: %s\n", ENDI_PROB));
            retcode = PK_COMPERR;
            goto cleanup_and_exit;
        }
    }
# endif /* def IZ_CRYPT_AES_WG */

/*---------------------------------------------------------------------------
    First figure out if we're running in UnZip mode or ZipInfo mode, and put
    the appropriate environment-variable options into the queue.  Then rip
    through any command-line options lurking about...
  ---------------------------------------------------------------------------*/

# ifdef SFX
    G.argv0 = argv[0];
#  if (defined(OS2) || defined(WIN32))
    G.zipfn = GetLoadPath(__G);/* non-MSC NT puts path into G.filename[] */
#  else
    G.zipfn = G.argv0;
#  endif

#  ifdef VMSCLI
    {
        ulg status = vms_unzip_cmdline(&argc, &argv);
        if (!(status & 1)) {
            retcode = (int)status;
            goto cleanup_and_exit;
        }
    }
#  endif /* def VMSCLI */

    uO.zipinfo_mode = FALSE;
    error = uz_opts(__G__ options_unzip, &argc, &argv);   /* UnZipSFX. */
    if (error)
    {
         /* Parsing error message already emitted.
          * Leave a blank line, and add the (brief) SFX usage message.
          */
        Info(slide, 0x401, ((char *)slide, "\n"));
        USAGE( error);
    }

# else /* def SFX */

#  ifdef RISCOS
    /* get the extensions to swap from environment */
    getRISCOSexts(ENV_UNZIPEXTS);
#  endif

#  ifdef MSDOS
    /* extract MKS extended argument list from environment (before envargs!) */
    mksargs(&argc, &argv);
#  endif

#  ifdef VMSCLI
    {
        ulg status = vms_unzip_cmdline(&argc, &argv);
        if (!(status & 1)) {
            retcode = (int)status;
            goto cleanup_and_exit;
        }
    }
#  endif /* def VMSCLI */

    G.noargs = (argc == 1);   /* no options, no zipfile, no anything */

#  ifndef NO_ZIPINFO
#   ifndef IGNORE_ARGV0
    for (p = argv[0] + strlen(argv[0]); p >= argv[0]; --p) {
        if (*p == DIR_END
#    ifdef DIR_END2
            || *p == DIR_END2
#    endif
           )
            break;
    }
    ++p;
#   endif /* ndef IGNORE_ARGV0 */

#   ifdef IGNORE_ARGV0
    if (
#   else /* def IGNORE_ARGV0 */
#    ifdef THEOS
    if (strncmp(p, "ZIPINFO.",8) == 0 || strstr(p, ".ZIPINFO:") != NULL ||
        strncmp(p, "II.",3) == 0 || strstr(p, ".II:") != NULL ||
#    else /* def THEOS */
    if (STRNICMP(p, LoadFarStringSmall(Zipnfo), 7) == 0 ||
        STRNICMP(p, "ii", 2) == 0 ||
#    endif /* def THEOS [else] */
#   endif /* def IGNORE_ARGV0 [else] */
        /* Check first arg for "-Z" or "--zipinfo-mode"
         * (but not "--zipfile-comment").
         */
        ((argc > 1) &&
         ((strncmp( argv[ 1], "-Z", 2) == 0) ||
          ((strlen(  argv[ 1]) > 5) &&
           (strncmp( argv[ 1], "--zipinfo-mode", strlen(  argv[ 1])) == 0)
        )))
    )
    {
        uO.zipinfo_mode = TRUE;
#   ifndef _WIN32_WCE /* Win CE does not support environment variables */
        if ((error = envargs(&argc, &argv, LoadFarStringSmall(EnvZipInfo),
                             LoadFarStringSmall2(EnvZipInfo2))) != PK_OK)
            perror(LoadFarString(NoMemEnvArguments));
#   endif
    } else
#  endif /* ndef NO_ZIPINFO */
    {
        uO.zipinfo_mode = FALSE;
#  ifndef _WIN32_WCE /* Win CE does not support environment variables */
        if ((error = envargs(&argc, &argv, LoadFarStringSmall(EnvUnZip),
                             LoadFarStringSmall2(EnvUnZip2))) != PK_OK)
            perror(LoadFarString(NoMemEnvArguments));
#  endif
    }

    if (!error) {
        /* Check the length of all passed command line parameters.
         * Command arguments might get sent through the Info() message
         * system, which uses the sliding window area as string buffer.
         * As arguments may additionally get fed through one of the FnFilter
         * macros, we require all command line arguments to be shorter than
         * WSIZE/4 (and ca. 2 standard line widths for fixed message text).
         */
        for (i = 1 ; i < argc; i++) {
           if (strlen(argv[i]) > ((WSIZE>>2) - 160)) {
               Info(slide, 0x401, ((char *)slide,
                 LoadFarString(CmdLineParamTooLong), i));
               retcode = PK_PARAM;
               goto cleanup_and_exit;
           }
        }
#  ifndef NO_ZIPINFO
        if (uO.zipinfo_mode)
        {
            error = zi_opts(__G__ options_zipinfo, &argc, &argv);
        }
        else
#  endif /* ndef NO_ZIPINFO */
        {
            error = uz_opts(__G__ options_unzip, &argc, &argv);
        }
    }

# endif /* def SFX [else] */

    if ((argc < 0) || error)
    {
        retcode = error;
        if ((argc < -1) || error)
        {
            if (error)
            {   /* Leave a blank line after an error message. */
                Info(slide, 0x401, ((char *)slide, "\n"));
            }
            USAGE( error);
        }
        goto cleanup_and_exit;
    }

/*---------------------------------------------------------------------------
    Now get the zipfile name from the command line and then process any re-
    maining options and file specifications.
  ---------------------------------------------------------------------------*/

# ifdef DOS_FLX_H68_NLM_OS2_W32
    /* convert MSDOS-style 'backward slash' directory separators to Unix-style
     * 'forward slashes' for user's convenience (include zipfile name itself)
     */
    {
        /* 2012-06-23 SMS.
         * Use a local variable here to avoid damage to the global
         * G.pfnames, which may be used later by UnZipSFX (at least).
         */
        char **G_pfnames;

#  ifdef SFX
        for (G_pfnames = argv, i = argc;  i > 0;  --i)
#  else
    /* argc does not include the zipfile specification */
        for (G_pfnames = argv, i = argc;  i > 0;  --i)
#  endif
        {
#  ifdef __human68k__
            extern char *_toslash(char *);
            _toslash(*G_pfnames);
#  else /* !__human68k__ */
            char *q = *G_pfnames;

            while (*q != '\0') {
                if (*q == '\\')
                    *q = '/';
                INCSTR(q);
            }
#  endif /* ?__human68k__ */
            ++G_pfnames;
        }
    }
# endif /* DOS_FLX_H68_NLM_OS2_W32 */

# if !defined( SFX) || defined( SFX_EXDIR)
    if (uO.exdir != (char *)NULL && !G.extract_flag)
        /* Have "-d exdir", but not actually extracting, so -d is ignored. */
        Info(slide, 0x401, ((char *)slide, LoadFarString(NotExtracting)));
# endif /* !defined( SFX) || defined( SFX_EXDIR) */

# ifdef UNICODE_SUPPORT
    /* set Unicode-escape-all if option -U used */
    if (uO.U_flag == 1)
#  ifdef UNICODE_WCHAR
        G.unicode_escape_all = TRUE;
#  else
        Info(slide, 0x401, ((char *)slide, LoadFarString(UTF8EscapeUnSupp)));
#  endif
# endif

# if defined( UNIX) && defined( __APPLE__)
    /* Set flag according to the capabilities of the destination volume. */
    G.exdir_attr_ok = vol_attr_ok( (uO.exdir == NULL) ? "." : uO.exdir);
# endif /* defined( UNIX) && defined( __APPLE__) */

# ifdef KFLAG
    /* Get Unix umask value.  (Already have VMS default protection value.) */
#  if defined( __ATHEOS__) || defined( __BEOS__) || defined( UNIX)
    umask( G.umask_val = umask( 0));
#  endif
# endif /* def KFLAG */

/*---------------------------------------------------------------------------
    Okey dokey, we have everything we need to get started.  Let's roll.
  ---------------------------------------------------------------------------*/

    retcode = process_zipfiles(__G);

cleanup_and_exit:
# if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
    /* restore all signal handlers back to their state at function entry */
    while (oldsighandlers != NULL) {
        savsigs_info *thissigsav = oldsighandlers;

        signal(thissigsav->sigtype, thissigsav->sighandler);
        oldsighandlers = thissigsav->previous;
        izu_free(thissigsav);
    }
# endif

# ifdef REENTRANT
    free_args( argv);
# endif /* def REENTRANT */

# if (defined(MSDOS) && !defined(SFX) && !defined(WINDLL))
    if (retcode != PK_OK)
        check_for_windows("UnZip");
# endif
    return(retcode);

} /* end main()/unzip() */



# if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
/*******************************/
/* Function setsignalhandler() */
/*******************************/

static int setsignalhandler(__G__ p_savedhandler_chain, signal_type,
                            newhandler)
    __GDEF
    savsigs_info **p_savedhandler_chain;
    int signal_type;
    void (*newhandler)(int);
{
    savsigs_info *savsig;

    savsig = izu_malloc(sizeof(savsigs_info));
    if (savsig == NULL) {
        /* error message and break */
        Info(slide, 0x401, ((char *)slide, LoadFarString(CantSaveSigHandler)));
        return PK_MEM;
    }
    savsig->sigtype = signal_type;
    savsig->sighandler = signal(SIGINT, newhandler);
    if (savsig->sighandler == SIG_ERR) {
        izu_free(savsig);
    } else {
        savsig->previous = *p_savedhandler_chain;
        *p_savedhandler_chain = savsig;
    }
    return PK_OK;

} /* end function setsignalhandler() */

# endif /* REENTRANT && !NO_EXCEPT_SIGNALS */


/* 2012-12-12 SMS.
 * Free some storage, if it was allocated, and we care.
 * (Use before a fatal error exit.)
 */
# ifdef REENTRANT
#  define FREE_NON_NULL( x) if ((x) != NULL) izu_free( x)
#  define UPDATE_PARGV *pargv = args
# else
#  define FREE_NON_NULL( x)
#  define UPDATE_PARGV
# endif


/**********************/
/* Function uz_opts() */
/**********************/

int uz_opts(__G__ opts, pargc, pargv)
    __GDEF
    ZCONST struct option_struct *opts;
    int *pargc;
    char ***pargv;
{
    char **args;
    int argc;
    int uzo_err = FALSE;
    int showhelp = 0;
# ifdef ENABLE_USER_PROGRESS
    int show_pid = 0;
# endif /* def ENABLE_USER_PROGRESS */

    /* used by get_option */
    unsigned long option; /* option ID returned by get_option */
    int argcnt = 0;       /* current argcnt in args */
    int argnum = 0;       /* arg number */
    int optchar = 0;      /* option state */
    char *value = NULL;   /* non-option arg, option value or NULL */
    int negative = 0;     /* 1 = option negated */
    int fna = 0;          /* current first non-opt arg */
    int optnum = 0;       /* index in table */
    int dashdash = 0;     /* Have seen "--". */


    /* Because get_option() returns xfiles and files one at a time,
     * store them in linked lists until we have them all.
     */

    int file_count = 0;
    struct file_list *next_file;

    /* files to extract */
    int in_files_count = 0;
    struct file_list *in_files = NULL;
    struct file_list *next_in_files = NULL;

    /* files to exclude in -x list */
    int in_xfiles_count = 0;
    struct file_list *in_xfiles = NULL;
    struct file_list *next_in_xfiles = NULL;

    G.wildzipfn = NULL;

    /* make copy of args that can use with insert_arg() used by get_option() */
    args = copy_args(__G__ *pargv, 0);

    /* 2013-01-17 SMS.
     * Note that before any early exit, we must inform the caller of our
     * new argv[], because he will want to izu_free() ours, not the
     * original:
     *         UPDATE_PARGV;            (*pargv = args;)
     *         return PK_xxx;
     * In principle, this could be done here, and by anyone who changes
     * "args", but that looked like more work.
     */

    /* Initialize lists */
    G.filespecs = 0;
    G.xfilespecs = 0;

    /*
    -------------------------------------------
    Process command line using get_option
    -------------------------------------------

    Each call to get_option() returns either a command
    line option and possible value or a non-option argument.
    Arguments are permuted so that all options (-r, -b temp)
    are returned before non-option arguments (zipfile).
    Returns 0 when nothing left to read.
    */

    /* set argnum = 0 on first call to init get_option */
    argnum = 0;

    /* get_option returns the option ID and updates parameters:
           args     - usually same as argv if no argument file support
           argcnt   - current argc for args
           value    - char* to value (free() when done with it) or NULL if none
           negative - option was negated with trailing -

       See the comments for get_option() for the other parameters.
    */

    /* 2012-12-12 SMS.
     * get_option() may allocate storage for "value".  If exiting early
     * (typically because of an error condition), then use
     * "FREE_NON_NULL( value)" to free this storage.  (See comments
     * above, where FREE_NON_NULL() is defined.)  If saving "value"
     * in some persistent location, then set "value = NULL" to prevent
     * this storage from being free()'d.  Otherwise, at the bottom of
     * the "while" loop, this storage will be free()'d.
     */

    while ((option = get_option(__G__ opts, &args, &argcnt, &argnum,
                                &optchar, &value, &negative,
                                &fna, &optnum, 0)))
    {
        if (option == o_BAD_ERR)
        {
          FREE_NON_NULL( value);        /* Leaving early.  Free it. */
          UPDATE_PARGV;                 /* See note 2013-01-17 SMS. */
          return(PK_PARAM);
        }

        switch (option)
        {
# ifdef RISCOS
            case ('/'):
                if (negative) {   /* negative not allowed with -/ swap */
                    Info(slide, 0x401, ((char *)slide,
                      "error:  must give extensions list"));
                    FREE_NON_NULL( value);      /* Leaving early.  Free it. */
                    UPDATE_PARGV;               /* See note 2013-01-17 SMS. */
                    return(PK_PARAM);   /* don't extract here by accident */
                }
                /* 2012-12-11 SMS.
                 * Note that this Acorn-RISC-OS-specific variable,
                 * exts2swap, is getting dynamic memory which will not
                 * be free()'d.  Most likely, it really should be moved
                 * into the general globals atructure, and dealt with
                 * properly, so someone who cares might wish to do that.
                 */
                exts2swap = value; /* override Unzip$Exts */
                value = NULL;           /* In use.  Don't free it. */
                break;
# endif /* def RISCOS */
            case ('0'):
                if (negative) {
                    uO.zero_flag = IZ_MAX( uO.zero_flag- negative, 0);
                } else
                    ++uO.zero_flag;
                break;
            case ('a'):
                if (negative) {
                    uO.aflag = IZ_MAX(uO.aflag-negative,0);
                } else
                    ++uO.aflag;
                break;
# if defined(DLL) && defined(API_DOC)
            case ('A'):    /* extended help for API */
                APIhelp(__G__ value);
                *pargc = -1;  /* signal to exit successfully */
                FREE_NON_NULL( value);          /* Leaving early.  Free it. */
                UPDATE_PARGV;                   /* See note 2013-01-17 SMS. */
                return PK_OK;
# endif /* defined(DLL) && defined(API_DOC) */
            case ('b'):
                if (negative) {
# if defined(TANDEM) || defined(VMS)
                    /* AS negative IS ALWAYS 1, IS THIS RIGHT? */
                    uO.bflag = IZ_MAX(uO.bflag-negative,0);
# endif /* defined(TANDEM) || defined(VMS) */
                    /* do nothing:  "-b" is default */
                } else {
# ifdef VMS
                    if (uO.aflag == 0)
                       ++uO.bflag;
# endif /* def VMS */
# ifdef TANDEM
                    ++uO.bflag;
# endif /* def TANDEM */
                    uO.aflag = 0;
                }
                break;
# ifdef UNIXBACKUP
            case ('B'): /* -B: back up existing files */
                if (negative)
                    uO.B_flag = FALSE;
                else
                    uO.B_flag = TRUE;
                break;
# endif /* def UNIXBACKUP */
            case ('c'):
                if (negative) {
                    uO.cflag = FALSE;
# ifdef NATIVE
                    uO.aflag = 0;
# endif /* def NATIVE */
                } else {
                    uO.cflag = TRUE;
# ifdef NATIVE
                    uO.aflag = 2;   /* so you can read it on the screen */
# endif /* def NATIVE */
# ifdef DLL
                    if (G.redirect_text)
                        G.redirect_data = 2;
# endif /* def DLL */
                }
                break;
# ifndef CMS_MVS
            case ('C'):    /* -C:  match filenames case-insensitively */
                if (negative)
                    uO.C_flag = FALSE, negative = 0;
                else
                    uO.C_flag = TRUE;
                break;
# endif /* ndef CMS_MVS */
# if !defined(SFX) || defined(SFX_EXDIR)
            case ('d'):
                /* 2014-02-19 SMS.  Negative is allowed why? */
                if (negative) {   /* negative not allowed with -d exdir */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(MustGiveExdir)));
                    FREE_NON_NULL( value);      /* Leaving early.  Free it. */
                    UPDATE_PARGV;               /* See note 2013-01-17 SMS. */
                    return(PK_PARAM);   /* don't extract here by accident */
                }
#  ifndef SFX
                uO.auto_exdir = 0;              /* -d overrides -da. */
#  endif /* ndef SFX */
                if (uO.exdir != (char *)NULL) {
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(OnlyOneExdir)));
                    FREE_NON_NULL( value);      /* Leaving early.  Free it. */
                    UPDATE_PARGV;               /* See note 2013-01-17 SMS. */
                    return(PK_PARAM);   /* GRR:  stupid restriction? */
                } else {
                    /* first check for "-dexdir", then for "-d exdir" */
                    uO.exdir = value;
                    if (uO.exdir == NULL || *uO.exdir == '\0') {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(MustGiveExdir)));
                        FREE_NON_NULL( value);  /* Leaving early.  Free it. */
                        UPDATE_PARGV;           /* See note 2013-01-17 SMS. */
                        return(PK_PARAM);  /* don't extract here by accident */
                    }
                    /* else uO.exdir points at extraction dir */
                }
                value = NULL;           /* In use.  Don't free it. */
                break;
            case (o_da):
#  ifndef SFX                           /* SFX ignores -da. */
                if (negative)
                {
                    uO.auto_exdir = IZ_MAX( (uO.auto_exdir- 1), 0);
                    negative = 0;
                }
                else if (value == NULL)
                {
                    uO.auto_exdir = 1;  /* Create new dest dir. */
                }
                else if (STRNICMP( value, "reuse", strlen( value)) == 0)
                {
                    uO.auto_exdir = 2;  /* Create new or reuse old dest dir. */
                }
                else
                {
                    /* Some invalid (non-"reuse") value found. */
                    Info( slide, 0x401, ((char *)slide,
                     LoadFarString( BadAutoDestValue)));
                    /* Leaving early.  Free it. */
                    FREE_NON_NULL( value);
                    UPDATE_PARGV;       /* See note 2013-01-17 SMS. */
                    return PK_PARAM;
                }
#  endif /* ndef SFX */
                break;
# endif /* !defined(SFX) || defined(SFX_EXDIR) */
# if !defined(NO_TIMESTAMPS)
            case ('D'):    /* -D: Skip restoring dir (or any) timestamp. */
                if (negative) {
                    uO.D_flag = IZ_MAX(uO.D_flag-negative,0);
                    negative = 0;
                } else
                    uO.D_flag++;
                break;
# endif /* !defined(NO_TIMESTAMPS) */
            case ('e'):    /* just ignore -e, -x options (extract) */
                break;
# ifdef MACOS
            case ('E'): /* -E [MacOS] display Mac e.f. when restoring */
                if (negative) {
                    uO.E_flag = FALSE, negative = 0;
                } else {
                    uO.E_flag = TRUE;
                }
                break;
# endif /* def MACOS */
            case ('f'):    /* "freshen" (extract only newer files) */
                if (negative)
                    uO.fflag = uO.uflag = FALSE, negative = 0;
                else
                    uO.fflag = uO.uflag = TRUE;
                break;
# if defined(RISCOS) || defined(ACORN_FTYPE_NFS)
            case ('F'):    /* Acorn filetype & NFS extension handling */
                if (negative)
                    uO.acorn_nfs_ext = FALSE, negative = 0;
                else
                    uO.acorn_nfs_ext = TRUE;
                break;
# endif /* defined(RISCOS) || defined(ACORN_FTYPE_NFS) */
            case ('h'):    /* just print help message and quit */
                if (showhelp == 0) {
                    showhelp = 1;
                }
                break;
# ifndef SFX
            case (o_hh):    /* just print long help message and quit */
                if (showhelp == 0) {
                    showhelp = 2;
                }
                break;
# endif /* ndef SFX */
# ifdef MACOS
            case ('i'): /* -i [MacOS] ignore filenames stored in Mac ef */
                if (negative) {
                    uO.i_flag = FALSE;
                } else {
                    uO.i_flag = TRUE;
                }
                break;
# endif  /* def MACOS */
# if defined( UNICODE_SUPPORT) && defined( ICONV_MAPPING)
#  ifdef UNIX
            case ('I'): /* -I [UNIX] ISO char set of input entries */
                strncpy( G.iso_cp, value, sizeof( G.iso_cp));
                break;
#  endif /* def UNIX */
# endif /* defined( UNICODE_SUPPORT) && defined( ICONV_MAPPING) */
            case ('j'):    /* junk pathnames/directory structure */
                if (negative)
                {
                    /* "-j-".  Junk nothing.  Keep all directories. */
                    uO.jflag = 0;
                }
                else
                {
                    /* Analyze any option value (junk depth). */
                    if (value == NULL)
                    {
                        /* No value specified.  Junk all directories. */
                        uO.jflag = -1;  /* "-j", or equivalent. */
                    }
                    else
                    {
                        /* Some value specified.  Decode it. */
                        long val;
                        char *ep;

                        val = strtol( value, &ep, 10);
                        if (ep < value+ strlen( value))
                        {
                            /* Some non-numeric character found. */
                            Info( slide, 0x401, ((char *)slide,
                             LoadFarString( BadJunkDirsValue)));
                            /* Leaving early.  Free it. */
                            FREE_NON_NULL( value);
                            UPDATE_PARGV;       /* See note 2013-01-17 SMS. */
                            return PK_PARAM;
                        }
                        else
                        {
                            /* "-j=N".  Junk specified number of dirs. */
                            uO.jflag = val;
                        }
                    }
                }
                break;
# ifdef J_FLAG
            case ('J'):    /* Junk AtheOS, BeOS or MacOS[X] file attributes */
                if (negative) {
                    uO.J_flag = FALSE;
                } else {
                    uO.J_flag = TRUE;
                }
                break;
# endif /* def J_FLAG */
# if defined( UNIX) && defined( __APPLE__)
            case (o_Je):   /* Junk (all) extended attributes. */
                if (negative) {
                    uO.Je_flag = FALSE;
                } else {
                    uO.Je_flag = TRUE;
                }
                break;
            case (o_Jf):   /* Junk Finder info. */
                if (negative) {
                    uO.Jf_flag = FALSE;
                } else {
                    uO.Jf_flag = TRUE;
                }
                break;
            case (o_Jq):   /* Junk quarantine ("com.apple.quarantine") */
                if (negative) {
                    uO.Jq_flag = FALSE;
                } else {
                    uO.Jq_flag = TRUE;
                }
                break;
            case (o_Jr):   /* Junk Resource fork. */
                if (negative) {
                    uO.Jr_flag = FALSE;
                } else {
                    uO.Jr_flag = TRUE;
                }
                break;
# endif /* defined( UNIX) && defined( __APPLE__) */
            case (o_ja):        /* --java-cafe. */
                if (negative) {
                    --uO.java_cafe;
                    negative = 0;
                } else
                    ++uO.java_cafe;
                break;
# ifdef ATH_BEO_UNX
            case ('K'):
                if (negative) {
                    uO.K_flag = FALSE;
                } else {
                    uO.K_flag = TRUE;
                }
                break;
# endif /* ATH_BEO_UNX */
# ifdef KFLAG
            case ('k'):
                if (negative) {
                    uO.kflag = IZ_MAX( -1, (uO.kflag- 1));
                } else {
                    uO.kflag = IZ_MIN( 1, (uO.kflag+ 1));
                }
                break;
# endif /* def KFLAG */
# ifdef VMS
            case (o_ka):
                if (negative) {
                    uO.ka_flag = FALSE;
                } else {
                    uO.ka_flag = TRUE;
                }
                break;
# endif /* def VMS */
# ifndef SFX
            case ('l'):
                if (negative) {
                    uO.vflag = IZ_MAX( (uO.vflag- negative), 0);
                    negative = 0;
                } else
                    ++uO.vflag;
                break;
# endif /* ndef SFX */
# ifndef CMS_MVS
            case ('L'):    /* convert (some) filenames to lowercase */
                if (negative) {
                    uO.L_flag = IZ_MAX(uO.L_flag-1,0);
                } else
                    ++uO.L_flag;
                break;
# endif /* ndef CMS_MVS */
            case (o_LI):    /* show license */
                showhelp = -1;
                break;
# ifdef MORE
#  ifdef CMS_MVS
            case ('m'):
#  endif /* def CMS_MVS */
            case ('M'):    /* send all screen output through "more" fn. */
/* GRR:  eventually check for numerical argument => height */
                if (negative)
                    G.M_flag = FALSE;
                else
                    G.M_flag = TRUE;
                break;
# endif /* def MORE */
            case ('n'):    /* don't overwrite any files */
                if (negative)
                    uO.overwrite_none = FALSE;
                else
                    uO.overwrite_none = TRUE;
                break;
# ifdef AMIGA
            case ('N'):    /* restore comments as filenotes */
                if (negative)
                    uO.N_flag = FALSE;
                else
                    uO.N_flag = TRUE;
                break;
# endif /* def AMIGA */
            case ('o'):    /* OK to overwrite files without prompting */
                if (negative) {
                    uO.overwrite_all = IZ_MAX(uO.overwrite_all-negative,0);
                    negative = 0;
                } else
                    ++uO.overwrite_all;
                break;
# if defined( UNICODE_SUPPORT) && defined( ICONV_MAPPING)
#  ifdef UNIX
            case ('O'): /* -O [UNIX] OEM char set of input entries */
                strncpy( G.oem_cp, value, sizeof( G.oem_cp));
                break;
#  endif /* def UNIX */
# endif /* defined( UNICODE_SUPPORT) && defined( ICONV_MAPPING) */
            case ('p'):    /* pipes:  extract to stdout, no messages */
                if (negative) {
                    uO.cflag = FALSE;
                    uO.qflag = IZ_MAX(uO.qflag-999,0);
                    negative = 0;
                } else {
                    uO.cflag = TRUE;
                    uO.qflag += 999;
                }
                break;
# ifdef IZ_CRYPT_ANY
            /* GRR:  yes, this is highly insecure, but dozens of people
             * have pestered us for this, so here we go... */
            case ('P'):
                if (negative) {   /* negative not allowed with -P passwd */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(MustGivePasswd)));
                    FREE_NON_NULL( value);      /* Leaving early.  Free it. */
                    UPDATE_PARGV;               /* See note 2013-01-17 SMS. */
                    return(PK_PARAM);   /* don't extract here by accident */
                }
                if (uO.pwdarg != (char *)NULL) {
#if 0
                    GRR:  eventually support multiple passwords?
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(OnlyOnePasswd)));
                    FREE_NON_NULL( value);      /* Leaving early.  Free it. */
                    UPDATE_PARGV;               /* See note 2013-01-17 SMS. */
                    return(PK_PARAM);
#endif /* 0 */
                } else {
                    /* first check for "-Ppasswd", then for "-P passwd" */
                    uO.pwdarg = value;
                    if (uO.pwdarg == NULL || *uO.pwdarg == '\0') {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(MustGivePasswd)));
                        FREE_NON_NULL( value);  /* Leaving early.  Free it. */
                        UPDATE_PARGV;           /* See note 2013-01-17 SMS. */
                        return(PK_PARAM);
                    }
                    /* else pwdarg points at decryption password */
                }
                value = NULL;           /* In use.  Don't free it. */
                break;
# endif /* def IZ_CRYPT_ANY */
            case ('q'):    /* quiet:  fewer comments/messages */
                if (negative) {
                    uO.qflag = IZ_MAX(uO.qflag-negative,0);
                    negative = 0;
                } else
                    ++uO.qflag;
                break;
# ifdef QDOS
            case ('Q'):   /* QDOS flags */
                qlflag ^= strtol(value, &value, 10);
                break;    /* we XOR this as we can config qlflags */
# endif /* def QDOS */
# ifdef TANDEM
            case ('r'):    /* remove file extensions */
                if (negative)
                    uO.rflag = FALSE;
                else
                    uO.rflag = TRUE;
                break;
# endif /* def TANDEM */
            case ('s'):    /* spaces in filenames:  allow by default */
                if (negative)
                    uO.sflag = FALSE;
                else
                    uO.sflag = TRUE;
                break;
# ifndef SFX
            case (o_sc):   /* show processed command line and exit */
                showhelp = -3;
                break;
            case (o_so):   /* show available options on this system */
                showhelp = -2;
                break;
# endif /* ndef SFX */

# if !defined( VMS) && defined( ENABLE_USER_PROGRESS)
            case (o_si):   /* Show process ID. */
                show_pid = 1;
                break;
# endif /* !defined( VMS) && defined( ENABLE_USER_PROGRESS) */

# ifdef VMS
            /* VMS:  extract "text" files in Stream_LF format (-a[a]) */
            case ('S'):
                if (negative)
                    uO.S_flag = FALSE;
                else
                    uO.S_flag = TRUE;
                break;
# endif /* def VMS */
            case ('t'):
                if (negative)
                    uO.tflag = FALSE;
                else
                    uO.tflag = TRUE;
                break;
# ifdef TIMESTAMP
            case ('T'):
                if (negative)
                    uO.T_flag = FALSE;
                else
                    uO.T_flag = TRUE;
                break;
# endif /* def TIMESTAMP */
            case ('u'):    /* update (extract only new and newer files) */
                if (negative)
                    uO.uflag = FALSE;
                else
                    uO.uflag = TRUE;
                break;
# ifdef UNICODE_SUPPORT
            case ('U'):    /* escape UTF-8, or disable UTF-8 support */
                if (negative)
                    uO.U_flag = IZ_MAX(uO.U_flag - 1, 0);
                else
                    uO.U_flag++;
                break;
# endif /* def UNICODE_SUPPORT */
# if !defined( SFX) || defined( DIAG_SFX)
            case ('v'):    /* verbose */
            case (o_ve):   /* version */
                if (negative)
                {
                    uO.vflag = IZ_MAX( (uO.vflag- negative), 0);
                    negative = 0;
                }
                else if (uO.vflag)
                {
                    ++uO.vflag;
                }
                else
                {
                    uO.vflag = 2;
                }
                break;

            case (o_vq):   /* brief version */
                uO.vflag = 3;
                uO.qflag = 4;
                break;
# endif /* !defined( SFX) || defined( DIAG_SFX) */
# ifndef CMS_MVS
            case ('V'):    /* Version (retain VMS/DEC-20 file versions) */
                if (negative)
                    uO.V_flag = IZ_MAX( (uO.V_flag- 1), -1);
                else
                    uO.V_flag = IZ_MIN( (uO.V_flag+ 1), 1);
                break;
# endif /* ndef CMS_MVS */
# ifdef WILD_STOP_AT_DIR
            case ('W'):    /* Wildcard interpretation (stop at '/'?) */
                if (negative)
                    uO.W_flag = FALSE;
                else
                    uO.W_flag = TRUE;
                break;
# endif /* def WILD_STOP_AT_DIR */
            case ('x'):    /* Exclude.  Add -x file to linked list. */
                if (in_xfiles_count == 0) {
                    /* first entry */
                    if ((in_xfiles = (struct file_list *)
                                     izu_malloc(sizeof(struct file_list))
                        ) == NULL) {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(NoMemArgsList)));
                        FREE_NON_NULL( value);  /* Leaving early.  Free it. */
                        UPDATE_PARGV;           /* See note 2013-01-17 SMS. */
                        return PK_MEM;
                    }
                    in_xfiles->name = value;
                    in_xfiles->next = NULL;
                    next_in_xfiles = in_xfiles;
                } else {
                    /* add next entry */
                    if ((next_file = (struct file_list *)
                                     izu_malloc(sizeof(struct file_list))
                        ) == NULL) {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(NoMemArgsList)));
                        FREE_NON_NULL( value);  /* Leaving early.  Free it. */
                        UPDATE_PARGV;           /* See note 2013-01-17 SMS. */
                        return PK_MEM;
                    }
                    next_in_xfiles->next = next_file;
                    next_file->name = value;
                    next_file->next = NULL;
                    next_in_xfiles = next_file;
                }
                in_xfiles_count++;
                value = NULL;           /* In use.  Don't free it. */
                break;
# if defined(RESTORE_UIDGID) || defined(RESTORE_ACL)
            case ('X'):   /* restore owner/group (more?) info (need privs?) */
                if (negative) {
                    uO.X_flag = IZ_MAX(uO.X_flag-negative, -1);
                    negative = 0;
                } else
                    ++uO.X_flag;
                break;
# endif /* defined(RESTORE_UIDGID) || defined(RESTORE_ACL) */
# ifdef VMS
            case ('Y'):    /* Treat ".nnn" as ";nnn" version. */
                if (negative)
                    uO.Y_flag = FALSE;
                else
                    uO.Y_flag = TRUE;
                break;
# endif /* def VMS */
            case ('z'):    /* display only the archive comment */
                if (negative) {
                    uO.zflag = IZ_MAX(uO.zflag-negative,0);
                    negative = 0;
                } else
                    ++uO.zflag;
                break;
# ifndef SFX
            case ('Z'):    /* should have been first option (ZipInfo) */
                Info(slide, 0x401, ((char *)slide, LoadFarString(Zfirst)));
                uzo_err = TRUE;
                break;
# endif /* ndef SFX */
# ifdef VMS
            case ('2'):    /* Force ODS2-compliant names. */
                if (negative)
                    uO.ods2_flag = FALSE, negative = 0;
                else
                    uO.ods2_flag = TRUE;
                break;
# endif /* def VMS */
# ifdef VOLFLAG
            case ('$'):
                if (negative) {
                    uO.volflag = IZ_MAX(uO.volflag-negative,0);
                    negative = 0;
                } else
                    ++uO.volflag;
                break;
# endif /* def VOLFLAG */
# if !defined(RISCOS) && !defined(CMS_MVS) && !defined(TANDEM)
            case (':'):    /* allow "parent dir" path components */
                if (negative) {
                    uO.ddotflag = IZ_MAX(uO.ddotflag-negative,0);
                    negative = 0;
                } else
                    ++uO.ddotflag;
                break;
# endif /* !defined(RISCOS) && !defined(CMS_MVS) && !defined(TANDEM) */
# ifdef UNIX
            case ('^'):    /* allow control chars in filenames */
                if (negative) {
                    uO.cflxflag = IZ_MAX(uO.cflxflag-negative,0);
                    negative = 0;
                } else
                    ++uO.cflxflag;
                break;
# endif /* def UNIX */
            case o_NON_OPTION_ARG:
                /* Not an option.  (Because of permutation, no more
                 * "-" options are expected henceforth.)
                 * "--" also appears here.
                 */

                /* First "--" is ignored (and stops arg processing for
                 * remaining args).
                 */
                if ((strcmp( value, "--") == 0) && (dashdash == 0))
                {
                  dashdash = 1;
                }
                else

# ifndef SFX
                /* For non-SFX (only), the first non-option argument is
                 * the archive name.  (For SFX, every non-option argument
                 * is an archive member name.)
                 */
                if (G.wildzipfn == NULL)
                {
                    /* first non-option argument is zip file */
                    G.wildzipfn = value;
                }
                else
# endif /* ndef SFX */
                {
                    /* add include file to list */
                    if (in_files_count == 0)
                    {
                        /* first entry */
                        if ((next_file = (struct file_list *)
                         izu_malloc(sizeof(struct file_list)) ) == NULL)
                        {
                            Info(slide, 0x401, ((char *)slide,
                              LoadFarString(NoMemArgsList)));
                            /* Leaving early.  Free it. */
                            FREE_NON_NULL( value);
                            UPDATE_PARGV;       /* See note 2013-01-17 SMS. */
                            return PK_MEM;
                        }
                        next_file->name = value;
                        next_file->next = NULL;
                        in_files = next_file;
                        next_in_files = next_file;
                    }
                    else
                    {
                        /* add next entry */
                        if ((next_file = (struct file_list *)
                         izu_malloc(sizeof(struct file_list))) == NULL)
                        {
                            Info(slide, 0x401, ((char *)slide,
                              LoadFarString(NoMemArgsList)));
                            /* Leaving early.  Free it. */
                            FREE_NON_NULL( value);
                            UPDATE_PARGV;       /* See note 2013-01-17 SMS. */
                            return PK_MEM;
                        }
                        next_in_files->next = next_file;
                        next_file->name = value;
                        next_file->next = NULL;
                        next_in_files = next_file;
                    }
                    in_files_count++;
                }
                value = NULL;           /* In use.  Don't free it. */
                break;
            default:
                uzo_err = TRUE;
                break;

        } /* switch (option) */

        FREE_NON_NULL( value);          /* Free it now, if it's not in use. */

    } /* while (get_option()) */


    /* convert files and xfiles lists to arrays */

    /* convert files list to array */
    if (in_files_count)
    {
      if ((G.pfnames = (char **)
       izu_malloc( (in_files_count + 1) * sizeof(char *))) == NULL)
      {
          Info(slide, 0x401, ((char *)slide, LoadFarString(NoMemArgsList)));
          UPDATE_PARGV;                 /* See note 2013-01-17 SMS. */
          return PK_MEM;
      }
      file_count = 0;
      for (next_file = in_files; next_file;)
      {
          G.pfnames[file_count] = next_file->name;
          in_files = next_file;
          next_file = next_file->next;
          izu_free(in_files);
          file_count++;
      }
      G.pfnames[file_count] = NULL;
      G.filespecs = in_files_count;
    }

    /* convert xfiles list to array */
    if (in_xfiles_count)
    {
      if ((G.pxnames = (char **)
       izu_malloc( (in_xfiles_count + 1) * sizeof(char *))) == NULL)
      {
          Info(slide, 0x401, ((char *)slide, LoadFarString(NoMemArgsList)));
          UPDATE_PARGV;                 /* See note 2013-01-17 SMS. */
          return PK_MEM;
      }
      file_count = 0;
      for (next_file = in_xfiles; next_file;)
      {
          G.pxnames[file_count] = next_file->name;
          in_xfiles = next_file;
          next_file = next_file->next;
          izu_free(in_xfiles);
          file_count++;
      }
      G.pxnames[file_count] = NULL;
      G.xfilespecs = in_xfiles_count;
    }

    /* For speed, set process_all_files flag if no include or exclude list. */
    G.process_all_files = (in_files_count == 0) && (in_xfiles_count == 0);

    /* get_option() could have changed the arg count, so re-evaluate it. */
    argc = arg_count(__G__ args);


/*---------------------------------------------------------------------------
    Check for nonsensical combinations of options.
  ---------------------------------------------------------------------------*/

    if ((uO.cflag && (uO.tflag || uO.uflag)) ||
        (uO.tflag && uO.uflag) || (uO.fflag && uO.overwrite_none))
    {
        Info(slide, 0x401, ((char *)slide, LoadFarString(InvalidOptionsMsg)));
        uzo_err = TRUE;
    }
    if (uO.aflag > 2)
        uO.aflag = 2;
# ifdef VMS
    if (uO.bflag > 2)
        uO.bflag = 2;
    /* Clear -S flag when converting text files. */
    if (uO.aflag <= 0)
        uO.S_flag = 0;
# endif /* def VMS */
    if (uO.overwrite_all && uO.overwrite_none) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(IgnoreOOptionMsg)));
        uO.overwrite_all = FALSE;
    }
# ifdef MORE
    if (G.M_flag && !isatty(1))  /* stdout redirected: "more" func. useless */
        G.M_flag = 0;
# endif /* def MORE */

# ifdef SFX
#  ifdef DIAG_SFX
    if ((showhelp == 0) || uzo_err)
#  else /* def DIAG_SFX */
    if (uzo_err)
#  endif /* def DIAG_SFX [else] */
# else /* def SFX */
    if ((showhelp == 0) && ((G.wildzipfn == NULL) || uzo_err))
# endif /* def SFX [else] */
    {
# if defined( SFX) && defined( DIAG_SFX)
        int argc_orig;

        argc_orig = argc;
# endif /* defined( SFX) && defined( DIAG_SFX) */

        /* tell caller to exit */
        if (argc <= 2)
            argc = -1;

        *pargc = argc;
        *pargv = args;

        /* 2012-07-27 SMS.
         * Allow (only) "-v" report in SFX, if DIAG_SFX is defined.
         */
# if !defined( SFX) || defined( DIAG_SFX)
        if (uO.vflag >= 2 && argc == -1) {              /* "unzip -v" */
            show_version_info(__G);
            return PK_OK;
        }
#  ifndef SFX
        if (!G.noargs && !uzo_err)
            uzo_err = TRUE;     /* had options (not -h or -v) but no zipfile */
#  endif /* ndef SFX */
#  if defined( SFX) && !defined( DIAG_SFX)
        return USAGE(uzo_err);
#  endif /* defined( SFX) && !defined( DIAG_SFX) */
# endif /* !defined( SFX) || defined( DIAG_SFX) */

# if defined( SFX) && defined( DIAG_SFX)
        argc = argc_orig;
# endif /* defined( SFX) && defined( DIAG_SFX) */
    }

    if (uO.cflag || uO.tflag || uO.vflag || uO.zflag
# ifdef TIMESTAMP
                                                     || uO.T_flag
# endif /* def TIMESTAMP */
                                                                 )
        G.extract_flag = FALSE;
    else
        G.extract_flag = TRUE;

    if (showhelp > 0) {         /* just print help message and quit */
        *pargc = -1;
# ifndef SFX
        if (showhelp == 2) {
            help_extended(__G);
            return PK_OK;
        } else
# endif /* ndef SFX */
        {
            return USAGE(PK_OK);
        }
    } else if (showhelp == -1) {
      *pargc = -1;
      show_license(__G);
      return PK_OK;
# ifndef SFX
    } else if (showhelp == -2) {
      /* show available options */
      *pargc = -1;
      show_options(__G);
      return PK_OK;
    } else if (showhelp == -3) {
      /* show command line args */
      *pargc = -1;
      show_commandline( args);
      return PK_OK;
# endif /* ndef SFX */
    }

    if ((uO.cflag && (uO.tflag || uO.uflag)) ||
        (uO.tflag && uO.uflag) || (uO.fflag && uO.overwrite_none))
    {
        Info(slide, 0x401, ((char *)slide, LoadFarString(InvalidOptionsMsg)));
        uzo_err = TRUE;
    }
    if (uO.aflag > 2)
        uO.aflag = 2;
# ifdef VMS
    if (uO.bflag > 2)
        uO.bflag = 2;
    /* Clear -S flag when converting text files. */
    if (uO.aflag <= 0)
        uO.S_flag = 0;
# endif /* def VMS */
    if (uO.overwrite_all && uO.overwrite_none) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(IgnoreOOptionMsg)));
        uO.overwrite_all = FALSE;
    }
# ifdef MORE
    if (G.M_flag && !isatty(1))  /* stdout redirected: "more" func. useless */
        G.M_flag = 0;
# endif /* def MORE */

# if defined( SFX) && !defined( DIAG_SFX)
    if (uzo_err)
# else /* defined( SFX) && !defined( DIAG_SFX) */
    if ((argc-- == 0) || uzo_err)
# endif /* defined( SFX) && !defined( DIAG_SFX) [else] */
    {
        *pargc = argc;
        *pargv = args;
# ifndef SFX
        if (uO.vflag >= 2 && argc == -1) {              /* "unzip -v" */
            show_version_info(__G);
            return PK_OK;
        }
        if (!G.noargs && !uzo_err)
            uzo_err = TRUE;     /* had options (not -h or -v) but no zipfile */
# endif /* ndef SFX */
        return (uzo_err ? PK_PARAM : PK_COOL);
    }

# ifdef SFX
/* # if defined( SFX) && !defined( DIAG_SFX) */
    /* print our banner unless we're being fairly quiet */
    if (uO.qflag < 2)
        Info(slide, (uzo_err ? 1 : 0),
         ((char *)slide, LoadFarString( UnzipBanner),
         "UnZipSFX", UzpVersionStr(), UZ_VERSION_DATE, UZ_VERSION_DATE));
#  ifdef BETA
    /* always print the beta warning:  no unauthorized distribution!! */
    Info(slide, (uzo_err ? 1 : 0),
     ((char *)slide, LoadFarString(BetaVersion), "\n", "SFX"));
#  endif /* def BETA */
# endif /* def SFX */

    if (uO.cflag || uO.tflag || uO.vflag || uO.zflag
# ifdef TIMESTAMP
                                                     || uO.T_flag
# endif /* def TIMESTAMP */
                                                                 )
        G.extract_flag = FALSE;
    else
        G.extract_flag = TRUE;

  /* Show process ID. */
# if !defined( VMS) && defined( ENABLE_USER_PROGRESS)
    if (show_pid)
    {
      fprintf( stderr, "PID = %d \n", getpid());
    }
# endif /* !defined( VMS) && defined( ENABLE_USER_PROGRESS) */

    *pargc = argc;
    *pargv = args;
    return PK_OK;

} /* end function uz_opts() */


# if !defined( SFX) || defined( DIAG_SFX)
#  ifndef _WIN32_WCE    /* Win CE does not support environment variables */

/*
 * show_env(): Display option environment variables.
 */

static void show_env_heading( __G__ heading)
 __GDEF
 int *heading;
{
    /* Display the heading once. */
    if (*heading == 0)
    {
        *heading = 1;
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptions)));
    }
} /* show_env_heading() */

#   ifndef VMSCLI
static                  /* Used in vms/cmdline.c, so not static in VMS CLI. */
#   endif /* ndef VMSCLI */
void show_env( __G__ non_null_only)
 __GDEF
 int non_null_only;
{
    int heading = 0;
    char *envptr;

    envptr = getenv(LoadFarStringSmall(EnvUnZip));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvUnZip),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
    envptr = getenv(LoadFarStringSmall(EnvUnZip2));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvUnZip2),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
    envptr = getenv(LoadFarStringSmall(EnvZipInfo));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvZipInfo),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
    envptr = getenv(LoadFarStringSmall(EnvZipInfo2));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvZipInfo2),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
#   ifndef __RSXNT__
#    ifdef __EMX__
    envptr = getenv(LoadFarStringSmall(EnvEMX));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvEMX),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
    envptr = getenv(LoadFarStringSmall(EnvEMXOPT));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvEMXOPT),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
#    endif /* __EMX__ */
#    if (defined(__GO32__) && (!defined(__DJGPP__) || (__DJGPP__ < 2)))
    envptr = getenv(LoadFarStringSmall(EnvGO32));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvGO32),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
    envptr = getenv(LoadFarStringSmall(EnvGO32TMP));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvGO32TMP),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
#    endif /* __GO32__ && !(__DJGPP__ >= 2) */
#   endif /* !__RSXNT__ */
#   ifdef RISCOS
    envptr = getenv(LoadFarStringSmall(EnvUnZipExts));
    if ((non_null_only == 0) || (envptr != (char *)NULL))
    {
        show_env_heading( __G__ &heading);
        Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
         LoadFarStringSmall(EnvUnZipExts),
         (envptr == (char *)NULL || *envptr == 0)?
         LoadFarStringSmall2(None) : envptr));
    }
#   endif /* RISCOS */

} /* show_env() */

#  endif /* ndef _WIN32_WCE */
# endif /* !defined( SFX) || defined( DIAG_SFX) */



/********************/
/* Function usage() */
/********************/

# ifdef SFX
#  ifdef VMS
#    define LOCAL "X"
#  endif
#  ifdef UNIX
#    define LOCAL "X"
#  endif
#  ifdef DOS_OS2_W32
#    define LOCAL "s$"
#  endif
#  if (defined(FLEXOS) || defined(NLM))
#    define LOCAL "s"
#  endif
#  ifdef AMIGA
#    define LOCAL "N"
#  endif
   /* Default for all other systems: */
#  ifndef LOCAL
#    define LOCAL ""
#  endif

#  ifndef NO_TIMESTAMP
#   ifdef MORE
#      define SFXOPT1 "DM"
#   else
#      define SFXOPT1 "D"
#   endif
#  else
#   ifdef MORE
#      define SFXOPT1 "M"
#   else
#      define SFXOPT1 ""
#   endif
#  endif

/* SFX Usage guide. */

int usage(__G__ u_err)   /* return PK-type error code */
    __GDEF
    int u_err;
{
    int flag = (u_err? 1 : 0);

    Info( slide, flag, ((char *)slide, LoadFarString( UnzipBanner),
     "UnZipSFX", UzpVersionStr(), UZ_VERSION_DATE, UZ_VERSION_DATE));
#  ifdef BETA
    Info( slide, flag, ((char *)slide, LoadFarString( BetaVersion), "\n",
     "SFX"));
#  endif

#  ifdef VMS
#   define CMD_PREFIX "MCR "
#   define CMD_CONTIN "-"
#  else /* def VMS */
#   define CMD_PREFIX ""
#   define CMD_CONTIN "\\"
#  endif /* def VMS [else] */

    Info( slide, flag, ((char *)slide, LoadFarString( UnzipSFXUsage),
     CMD_PREFIX, G.zipfn, CMD_CONTIN));

    Info( slide, flag, ((char *)slide, LoadFarString( UnzipSFXOpts),
     SFXOPT1, LOCAL));
#  ifdef VMS
    Info( slide, flag, ((char *)slide, LoadFarString( UnzipSFXOptsV)));
#  endif /* def VMS */
    Info(slide, flag, ((char *)slide, LoadFarString( UnzipSFXOpts2)));

    if (u_err)
        return PK_PARAM;
    else
        return PK_COOL;         /* No error, but wanted usage guide. */

} /* end function usage() */

# else /* def SFX */

#  ifdef VMS
#    define QUOT '\"'
#    define QUOTS "\""
#  else /* def VMS */
#    define QUOT ' '
#    define QUOTS ""
#  endif /* def VMS [else] */

/* Normal (Non-SFX) Usage guide. */

int usage(__G__ u_err)   /* return PK-type error code */
    __GDEF
    int u_err;
{
    int flag = (u_err? 1 : 0);

/*---------------------------------------------------------------------------
    Print either ZipInfo usage or UnZip usage, depending on incantation.
    (Strings must be no longer than 512 bytes for Turbo C, apparently.)
  ---------------------------------------------------------------------------*/

    if (uO.zipinfo_mode)
    {
#  ifndef NO_ZIPINFO

        Info( slide, flag, ((char *)slide, LoadFarString( ZipInfoUsageLine1),
         UzpVersionStr(), UZ_VERSION_DATE, USAGE_DCL_Z));
#  ifdef BETA
        Info( slide, flag, ((char *)slide, LoadFarString( BetaVersion), "",
         ""));
#  endif
        Info( slide, flag, ((char *)slide, LoadFarString( ZipInfoUsageLine2),
         QUOTS, QUOTS, LoadFarStringSmall2( ZipInfoExample)));
        Info( slide, flag, ((char *)slide, LoadFarString( ZipInfoUsageLine3)));
        Info( slide, flag, ((char *)slide, LoadFarString( ZipInfoUsageLine4),
         LoadFarString( ZipInfoUsageLine4m)));
#   ifdef VMS
        Info(slide, flag, ((char *)slide, "\n\
  (Must quote upper-case options and names, unless SET PROC/PARSE=EXTEND.)\
\n"));
#   endif /* def VMS */
#  endif /* ndef NO_ZIPINFO */
    }
    else
    {   /* UnZip mode */
        Info(slide, flag, ((char *)slide, LoadFarString( UnzipUsageLine1),
         UzpVersionStr(), UZ_VERSION_DATE, USAGE_DCL_U));
#  ifdef BETA
        Info(slide, flag, ((char *)slide, LoadFarString(BetaVersion), "", ""));
#  endif

        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine2),
          ZIPINFO_MODE_OPTION, LoadFarStringSmall(ZipInfoMode)));

        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine3),
          LoadFarStringSmall(local1)));

        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine4),
          LoadFarStringSmall(local2), LoadFarStringSmall2(local3)));

        /* This is extra work for SMALL_MEM, but it will work since
         * LoadFarStringSmall2 uses the same buffer.  Remember, this
         * is a hack. */
        Info(slide, flag, ((char *)slide,
         LoadFarString( UnzipUsageLine5), LoadFarStringSmall( ExampleCmnt),
          LoadFarStringSmall( Example2),
          LoadFarStringSmall2( Example3), LoadFarStringSmall( ExampleCmnt),
          LoadFarStringSmall2( Example3)));

    } /* end if (uO.zipinfo_mode) */

    if (u_err)
    {
        show_env( __G__ 1);     /* Show env vars, if option error. */
        return PK_PARAM;
    }
    else
        return PK_COOL;         /* No error, but wanted usage guide. */

} /* end function usage() */

# endif /* def SFX [else] */


/* Print license to stdout. */
void show_license(__G)
    __GDEF
{
    extent i;             /* counter for license array */

    /* license array */
    static ZCONST char *text[] = {
  "Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.",
  "",
  "This is version 2009-Jan-02 of the Info-ZIP license.",
  "",
  "For the purposes of this copyright and license, \"Info-ZIP\" is defined as",
  "the following set of individuals:",
  "",
  "   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,",
  "   Jean-loup Gailly, Hunter Goatley, Ed Gordon, Ian Gorman, Chris Herborth,",
  "   Dirk Haase, Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz,",
  "   David Kirschbaum, Johnny Lee, Onno van der Linden, Igor Mandrichenko,",
  "   Steve P. Miller, Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs,",
  "   Kai Uwe Rommel, Steve Salisbury, Dave Smith, Steven M. Schweda,",
  "   Christian Spieler, Cosmin Truta, Antoine Verheijen, Paul von Behren,",
  "   Rich Wales, Mike White",
  "",
  "This software is provided \"as is,\" without warranty of any kind, express",
  "or implied.  In no event shall Info-ZIP or its contributors be held liable",
  "for any direct, indirect, incidental, special or consequential damages",
  "arising out of the use of or inability to use this software.",
  "",
  "Permission is granted to anyone to use this software for any purpose,",
  "including commercial applications, and to alter it and redistribute it",
  "freely, subject to the above disclaimer and the following restrictions:",
  "",
  "    1. Redistributions of source code (in whole or in part) must retain",
  "       the above copyright notice, definition, disclaimer, and this list",
  "       of conditions.",
  "",
  "    2. Redistributions in binary form (compiled executables and libraries)",
  "       must reproduce the above copyright notice, definition, disclaimer,",
  "       and this list of conditions in documentation and/or other materials",
  "       provided with the distribution.  Additional documentation is not needed",
  "       for executables where a command line license option provides these and",
  "       a note regarding this option is in the executable's startup banner.  The",
  "       sole exception to this condition is redistribution of a standard",
  "       UnZipSFX binary (including SFXWiz) as part of a self-extracting archive;",
  "       that is permitted without inclusion of this license, as long as the",
  "       normal SFX banner has not been removed from the binary or disabled.",
  "",
  "    3. Altered versions--including, but not limited to, ports to new operating",
  "       systems, existing ports with new graphical interfaces, versions with",
  "       modified or added functionality, and dynamic, shared, or static library",
  "       versions not from Info-ZIP--must be plainly marked as such and must not",
  "       be misrepresented as being the original source or, if binaries,",
  "       compiled from the original source.  Such altered versions also must not",
  "       be misrepresented as being Info-ZIP releases--including, but not",
  "       limited to, labeling of the altered versions with the names \"Info-ZIP\"",
  "       (or any variation thereof, including, but not limited to, different",
  "       capitalizations), \"Pocket UnZip,\" \"WiZ\" or \"MacZip\" without the",
  "       explicit permission of Info-ZIP.  Such altered versions are further",
  "       prohibited from misrepresentative use of the Zip-Bugs or Info-ZIP",
  "       e-mail addresses or the Info-ZIP URL(s), such as to imply Info-ZIP",
  "       will provide support for the altered versions.",
  "",
  "    4. Info-ZIP retains the right to use the names \"Info-ZIP,\" \"Zip,\" \"UnZip,\"",
  "       \"UnZipSFX,\" \"WiZ,\" \"Pocket UnZip,\" \"Pocket Zip,\" and \"MacZip\" for its",
  "       own source and binary releases.",
  ""
    };

    for (i = 0; i < sizeof(text)/sizeof(char *); i++)
    {
        Info(slide, 0, ((char *)slide, "%s\n", text[i]));
    }
} /* end function show_license() */


# ifndef SFX

/* Print extended help to stdout. */
static void help_extended(__G)
    __GDEF
{
    extent i;             /* counter for help array */

    /* help array */
    static ZCONST char *text[] = {
  "",
  "Extended Help for UnZip",
  "",
  "See the UnZip Manual for more detailed information.",
  "",
  "",
  "UnZip lists and extracts files in zip archives.  The default action is to",
  "extract archive members to the current directory, creating directories as",
  "needed.  With appropriate options, UnZip lists the contents of archives",
  "instead.",
  "",
  "Basic UnZip command line:",
  "  unzip [-Z] [options] archive[.zip] [file ...] [-x xfile ...]",
  "",
  "Some examples:",
  "  unzip -l foo.zip        List files in short format in archive foo.zip.",
  "",
  "  unzip -t foo            Test the files in archive foo.",
  "",
  "  unzip -Z foo            List files using more detailed ZipInfo format.",
  "",
  "  unzip foo               Unzip the contents of foo in current dir.",
  "",
  "  unzip -a foo            Unzip foo and convert text files to local OS.",
  "",
  "If UnZip is run in ZipInfo mode, a more detailed list of archive contents",
  "is provided.  The -Z option sets ZipInfo mode and changes the available",
  "options.",
  "",
  "Basic ZipInfo command line:",
  "  zipinfo [options] archive[.zip] [file ...] [-x xfile ...]",
  "  unzip -Z [options] archive[.zip] [file ...] [-x xfile ...]",
  "",
  "Below, MacOS refers to Mac OS before Mac OS X.  Mac OS X is a Unix-based",
  "port and is referred to as MacOSX.",
  "",
  "UnZip 6.1 uses a new command parser which supports long options.  Short",
  "options begin with a single dash (-h), while long options start with two",
  "(--help).  Long options can be abbreviated until uniqueness is lost.",
  "",
  "-------------------------------------------------------------------------",
  "WARNING:",
  "Option negation now uses a TRAILING dash.  So -B turns on backup and -B-",
  "turns it off.  For long options, --backup turns on backup and --backup-",
  "turns it off.  Options later in command line override those earlier.",
  "\"unzip --options\" will show all options and which can be negated.",
  "-------------------------------------------------------------------------",
  "",
  "UnZip now allows most options to appear anywhere in the command line.",
  "An exception is \"-x file_list\", which still must be last unless list is",
  "terminated by \"@\" argument as in \"-x f1 f2 f3 @ -B\".",
  "",
  "",
  "UnZip options:",
  "  -Z   Switch to ZipInfo mode.  Must be the first option.",
  "  -hh  Display extended help.",
  "  -A   [OS/2, Unix DLL] Print extended help for DLL.",
  "  -c   Extract files to stdout/screen.  Like -p, but include names.  Also,",
  "         -a is allowed, and ASCII-EBCDIC conversions are done if needed.",
  "  -f   Freshen by extracting only if an older file is on disk.",
  "  -l   List files using short form.",
  "  -p   Extract files to pipe (stdout).  Only file data are put out, and all",
  "         files are extracted as archived (without conversions).",
  "  -t   Test archive files.",
  "  -T   Set timestamp on archive(s) to that of newest file.  Similar to",
  "       zip -o, but faster.",
  "  -u   Update existing older files on disk as -f, and extract new files.",
  "  -v   Use verbose list format (with -l).  Alone (unzip -v), show version",
  "         and build option information.  Also can be added to other list",
  "         commands for more verbose output.",
  "  -z   Display only the archive comment.",
  "",
  "UnZip modifiers:",
  "  -a   Convert text files to local OS format.  Convert line ends, EOF",
  "         marker, and from or to EBCDIC character set as needed.",
  "  -B   [UNIXBACKUP compile option enabled] Save a backup copy of each",
  "         overwritten file in foo~ or foo~99999 format.",
  "  -b   Treat all files as binary.  [Tandem] Force filecode 180 ('C').",
  "         [VMS] Autoconvert binary files.  -bb forces convert of all files.",
  "  -C   Use case-insensitive matching.",
  "  -D   Skip restoration of timestamps on all files and directories.",
  "  -D-  Restore timestamps on directories as well as on files.",
  "       DEFAULT IS NOW to restore timestamps on files, but not directories.",
  "  -E   [MacOS (not MacOSX)]  Display contents of MacOS extra field during",
  "         restore.",
  "  -F   [Acorn] Suppress removal of NFS filetype extension.  [Non-Acorn if",
  "         ACORN_FTYPE_NFS] Translate filetype and append to name.",
  "  -I   [Unix, with ICONV_MAPPING] ISO code page to use.",
  "  -i   [MacOS] Ignore filenames in MacOS extra field.  Instead, use name in",
  "         standard header.",
  "  -J   [BeOS] Junk file attributes.",
  "       [MacOS] Ignore MacOS specific info.",
  "       [MacOSX] No special AppleDouble file handling.",
  "  -Je  [MacOSX] Ignore AppleDouble extended attributes.",
  "  -Jf  [MacOSX] Ignore AppleDouble Finder info.",
  "  -Jq  [MacOSX] Ignore AppleDouble quarantine (an extended attribute).",
  "  -Jr  [MacOSX] Ignore AppleDouble resource fork.",
  "  -j[=N] Junk paths.  Strip all (or top N) directories from extracted files.",
  "  --jar Treat archive(s) as Java JAR (UTF-8 names).",
  "  -K   [AtheOS, BeOS, Unix] Restore SUID/SGID/Tacky file attributes.",
  "  -k   [AtheOS, BeOS, Unix, VMS] Ignore umask (VMS: default protection)",
  "         when restoring permissions/protections.",
  "  -k-    Ignore archive permissions/protections.  Use umask (VMS: dflt prot).",
  "         Default: Apply umask (VMS: dflt prot) to archive perms/prots.",
  "  -ka  [VMS] Restore (VMS) ACL.",
  "  -L   Convert to lowercase any names from uppercase-only file system.",
  "  -LL  Convert all files to lowercase.",
  "  -M   Pipe all output through internal pager similar to Unix more(1).",
  "  -N   [Amiga] Extract file comments as Amiga filenotes.",
  "  -n   Never overwrite existing files.  Skip extracting that file, no prompt.",
  "  -O   [Unix, with ICONV_MAPPING] OEM code page to use.  If -O is not used,",
  "         UnZip tries to set automatically the OEM code page, based on",
  "         the current environment language setting.",
  "  -o   Overwrite existing files without prompting.  Useful with -f.  Use with",
  "         care.",
  "  -P pw Use password pw to decrypt files.  THIS IS INSECURE!  Some OS show",
  "         command line to other users.",
  "  -q   Perform operations quietly.  The more q (as in -qq) the quieter.",
  "  -S   [VMS] Convert text files (-a, -aa) into Stream_LF format.",
  "  -s   Convert spaces in filenames to underscores.",
  "  -U   [UNICODE enabled] Show non-local characters as #Uxxxx or #Lxxxxxx ASCII",
  "         text escapes where x is hex digit.  [Old] -U used to leave names",
  "         uppercase if created on MS-DOS, VMS, etc.  See -L.",
  "  -UU  [UNICODE enabled] Disable use of stored UTF-8 paths.  Note that UTF-8",
  "         paths stored as native local paths are still processed as Unicode.",
  "  -V   Retain VMS file version numbers.",
  "  -W   [Only if WILD_STOP_AT_DIR] Modify pattern matching so ? and * do not",
  "         match directory separator /, but ** does.  Allows matching at specific",
  "         directory levels.",
  "  -X   [Unix, VMS, OS/2, NT, Tandem] Restore UID/GID on Unix, UIC on VMS,",
  "         ACL on certain network-enabled versions of OS/2, or security ACL",
  "         on Windows NT.  Can require user privileges.",
  "  -XX  [NT] Extract NT security ACLs after trying to enable additional",
  "         system privileges.",
  "  -Y   [VMS] Treat archived name endings of .nnn as VMS version numbers.",
  "  -$   [MS-DOS, OS/2, NT] Restore volume label if extraction medium is",
  "         removable.  -$$ allows fixed media (hard drives) to be labeled.",
  "  -/ e [Acorn] Use e as extension list.",
  "  -:   [All but Acorn, VM/CMS, MVS, Tandem] Allow extract archive members into",
  "         locations outside of current extraction root folder.  This allows",
  "         paths such as ../foo to be extracted above the current extraction",
  "         directory, which can be a security problem.",
  "  -^   [Unix] Allow control characters in names of extracted entries.  Usually",
  "         this is not a good thing and should be avoided.",
  "  -2   [VMS] Force unconditional conversion of names to ODS-compatible names.",
  "         Default is to exploit destination file system, preserving cases and",
  "         extended name characters on ODS5 and applying ODS2 filtering on ODS2.",
  "",
  "  --commandline  Show the processed command line and exit.",
  "",
  "Wildcards:",
  "  Internally UnZip supports the following wildcards:",
  "    ?       (or %% or #, depending on OS) matches any single character",
  "    *       matches any number of characters, including zero",
  "    [list]  matches char in list (regex), can do range [ac-f], all but [!bf]",
  "  If port supports [], must escape [ as [[]",
  "  For shells that expand wildcards, escape (\\* or \"*\") so UnZip can recurse.",
  "",
  "Include and Exclude archive members:",
  "  pattern pattern ...      include members that match a pattern",
  "  -x pattern pattern ...   exclude members that match a pattern",
  "  Patterns are paths with optional wildcards and match paths as stored in",
  "  archive.  Exclude and include lists end at next option or end of line.",
  "    unzip archive -x pattern pattern ...",
  "",
  "Segmented (split) archives (archives created as a set of split files):",
  "  Currently, all archive segments must be files on a single volume, with",
  "  name extensions \".z01\", \".z02\", and so on, with the last segment",
  "  \".zip\".",
  "",
  "Streaming (piping into UnZip):",
  "  Currently UnZip does not support streaming.  The FUnZip utility can be",
  "  used to process the first entry in a stream.",
  "    cat archive | funzip",
  "",
  "Testing archives:",
  "  -t        test contents of archive",
  "  This can be modified using -q for quieter operation, and -qq for even",
  "  quieter operation.",
  "",
  "Unicode and character set conversions:",
  "  If compiled with Unicode support, UnZip automatically handles archives",
  "  with Unicode entries.  On ports where UTF-8 is not the native character",
  "  set, characters not in current encoding are shown as ASCII escapes in",
  "  form #Uxxxx or #Lxxxxxx where x is ASCII character for hex digit.",
  "  Though modern UNIX consoles support full UTF-8, some older console",
  "  displays may be limited to a specific code page.  Either way full UTF-8",
  "  paths are generally restored on extraction where OS supports.  Use -U",
  "  to force use of escapes in extracted names.  Use -UU to totally ignore",
  "  Unicode.  Unicode comments are only supported currently if UTF-8 is the",
  "  native character set.  Full comment support is expected shortly.",
  "",
  "  On Unix, with ICONV_MAPPING defined at build time, options -I and -O are",
  "  used to specify the ISO and OEM code pages used for name conversions.",
  "  By default, UnZip will try to select a code page based on the user's",
  "  environment (language settings).  -I and -O will override this.",
  "",
  "",
  "ZipInfo options (these are used in ZipInfo mode (unzip -Z ...)):",
  "  -1  List names only, one per line.  No headers/trailers.  Good for scripts.",
  "  -2  List names only as -1, but allow headers, trailers, and comments.",
  "  -s  List archive entries in short Unix ls -l format.  Default list format.",
  "  -m  List in long Unix ls -l format.  Like -s, but includes compression %.",
  "  -l  List in long Unix ls -l format.  Like -m, but compression in bytes.",
  "  -v  List zipfile information in verbose, multi-page format.",
  "  -h  List header line.  Includes archive name, actual size, total files.",
  "  -M  Pipe all output through internal pager similar to Unix more(1) command.",
  "  -t  List totals for files listed or for all files.  Includes uncompressed",
  "        and compressed sizes, and compression factors.",
  "  -T  Print file dates and times in a sortable decimal format (yymmdd.hhmmss)",
  "        Default date and time format is a more human-readable version.",
  "  -U  [UNICODE] If entry has a UTF-8 Unicode path, display any characters",
  "        not in current character set as text #Uxxxx and #Lxxxxxx escapes",
  "        representing the Unicode character number of the character in hex.",
  "  -UU [UNICODE]  Disable use of any UTF-8 path information.",
  "  -W   [Only if WILD_STOP_AT_DIR] Modify pattern matching so ? and * do not",
  "         match directory separator /, but ** does.  Allows matching at specific",
  "         directory levels.",
  "  -z  Include archive comment if any in listing.",
  "",
  "",
  "FUnZip stream extractor:",
  "  FUnZip extracts the first member in an archive to stdout.  Typically",
  "  used to unzip the first member of a stream or pipe.  If a file argument",
  "  is given, read from that file instead of stdin.",
  "",
  "FUnZip command line:",
  "  funzip [-password] [input[.zip|.gz]]",
  "",
  "",
  "UnZipSFX self extractor:",
  "  UnZipSFX is a special UnZip program which can be attached to a normal",
  "  Zip archive, forming a self-extracting archive.  On a system which is",
  "  compatible with the UnZipSFX executable used, the resulting",
  "  UnZipSFX+archive bundle can be executed to extract the contents of the",
  "  archive in the bundle, without using a separate UnZip program.",
  "  (A separate, normal UnZip program on any system can also be used to",
  "  extract files from a self-extracting archive, ignoring the UnZipSFX",
  "  program in the SFX bundle.)",
  "",
  "UnZipSFX command line:",
  "  <unzipsfx+archive_filename> [options] [file ...] [-x xfile ...]",
  "",
  "UnZipSFX options:",
  "  -c, -p   Output to stdout/pipe.  (See UnZip, above.)",
  "  -f, -u   Freshen and Update, as for UnZip.  (See UnZip, above.)",
  "  -t       Test embedded archive.  (Can be used to list contents.)",
  "  -z       Print archive comment.  (See UnZip, above.)",
  "",
  "UnZipSFX modifiers:",
  "  Most UnZip modifiers are supported.  These include",
  "  -a       Convert text files.",
  "  -n       Never overwrite.",
  "  -o       Overwrite without prompting.",
  "  -q       Quiet operation.",
  "  -C       Match names case-insensitively.",
  "  -j[=N]   Junk paths.  Strip all (or top N) directories from extracted files.",
  "  -V       Retain VMS file version numbers (like: \";123\").",
  "  -s       Convert spaces to underscores.",
  "  -$       Restore volume label.",
  "",
  "If UnZipSFX is built with SFX_EXDIR defined, the -d option also available:",
  "  -d exd   Extract to directory exd.",
  "By default, all files are extracted to the current directory.  This option",
  "forces extraction to specified directory.",
  "",
  "See unzipsfx manual page for more information.",
  ""
    };

    for (i = 0; i < sizeof(text)/sizeof(char *); i++)
    {
        Info(slide, 0, ((char *)slide, "%s\n", text[i]));
    }
} /* end function help_extended() */


/* Print available options. */
void show_options(__G)
    __GDEF
{
    int i;
    size_t lolen;
    char sh[7];
    char lo[80];
    char *val_type;
    char *neg;
    ZCONST struct option_struct *opts;

#  ifdef NO_ZIPINFO
#   define CMD_NAME "UnZip"
#  else /* def NO_ZIPINFO */
#   define CMD_NAME (uO.zipinfo_mode ? "ZipInfo" : "UnZip")
#  endif /* def NO_ZIPINFO [else] */

    Info(slide, 0, ((char *)slide, "\
Available %s options:\n", CMD_NAME));

    Info(slide, 0, ((char *)slide, "\
 sh  long               val  neg description\n"));

    Info(slide, 0, ((char *)slide, "\
 --  ----               ---  --- -----------\n"));

#  ifndef NO_ZIPINFO
    if (uO.zipinfo_mode)
    {
      opts = options_zipinfo;
    }
    else
#  endif /* ndef NO_ZIPINFO */
    {
      opts = options_unzip;
    }

    for (i = 0; opts[i].option_ID != 0; i++)
    {
        strcpy(sh, opts[i].shortopt);
        strcat(sh, "  ");
        sh[2] = '\0';

        strcpy(lo, opts[i].longopt);
        lolen = strlen(lo);
        strcat(lo, "                      ");
        if (lolen < 17)
          lolen = 17;
        lo[lolen] = '\0';

        switch (opts[i].value_type)
        {
            case o_NO_VALUE:
                val_type = "    "; break;
            case o_REQUIRED_VALUE:
                val_type = "requ"; break;
            case o_OPTIONAL_VALUE:
                val_type = "optn"; break;
            case o_VALUE_LIST:
                val_type = "list"; break;
            case o_ONE_CHAR_VALUE:
                val_type = "char"; break;
            case o_NUMBER_VALUE:
                val_type = "numb"; break;
            case o_OPT_EQ_VALUE:
                val_type = "=val"; break;
            default:
                val_type = "????";
        }

        if (opts[i].negatable == 0)
            neg = "   ";
        else if (opts[i].negatable == 1)
            neg = "neg";
        else
            neg = "???";

        Info(slide, 0, ((char *)slide, "\
 %s  %s  %s %s %s\n",
         sh, lo, val_type, neg, opts[i].name));
    }
} /* end function show_options() */


/* Print processed command line. */
void show_commandline( args)
    char *args[];
{
#  define MAX_CARG_LEN (WSIZE>>2)

    extent i;
    char argtext[MAX_CARG_LEN + 1];
    GETGLOBALS();

    Info(slide, 0, ((char *)slide, "%s\n\n", "Processed command line:"));

    for (i = 0; args[i]; i++)
    {
        if (strlen(args[i]) > MAX_CARG_LEN - 6) {
            /* If arg too big, truncate it and add ... to end.
               We are just displaying the args and exiting. */
            args[i][MAX_CARG_LEN - 6] = '\0';
            strcat(args[i], " ...");
        }
        sprintf(argtext, "\"%s\"", args[i]);
        Info(slide, 0, ((char *)slide, "%s  ", argtext));
    }
    Info(slide, 0, ((char *)slide, "%s", "\n"));
} /* end function show_commandline() */


#  ifndef _WIN32_WCE /* Win CE does not support environment variables */
#   if (!defined(MODERN) || defined(NO_STDLIB_H))
/* Declare getenv() to be sure (might be missing in some environments) */
extern char *getenv();
#   endif
#  endif
# endif /* ndef SFX */


char *UZ_EXP UzpVersionStr( OFT( void))
{
  char pl_sufx[ 8];
  char *bt_sufx = "";

  GETGLOBALS();

  if (*G.prog_vers_str == '\0')
  {
    *pl_sufx = '\0';
    if (UZ_PATCHLEVEL != 0)
    {
      sprintf( pl_sufx, ".%d", UZ_PATCHLEVEL);
    }
    if ((UZ_BETALEVEL != NULL) && (strcmp( UZ_BETALEVEL, "")))
    {
      bt_sufx = UZ_BETALEVEL;
    }
    sprintf( G.prog_vers_str, "%d.%d%s%s",
     UZ_MAJORVER, UZ_MINORVER, pl_sufx, bt_sufx);
  }

  return G.prog_vers_str;
}


# ifndef SFX
#  ifdef VMS
char *UZ_EXP UzpDclStr( OFT( void))
{
  return USAGE_DCL_U;
}

#   ifndef NO_ZIPINFO
char *UZ_EXP ZiDclStr( OFT( void))
{
  return USAGE_DCL_Z;
}
#   endif /* ndef NO_ZIPINFO */
#  endif /* def VMS */
# endif /* ifndef SFX */


# if !defined( SFX) || defined( DIAG_SFX)

/********************************/
/* Function show_version_info() */
/********************************/

void show_version_info(__G)
    __GDEF
{
    if (uO.qflag > 3)                           /* "-qqqqvv" or "-vq" */
    {
        Info(slide, 0, ((char *)slide, LoadFarString( UnzipBanner),
         "UnZip", UzpVersionStr(), UZ_VERSION_DATE, UZ_VERSION_DATE));
    }
    else
    {
        int numopts = 0;

#  ifndef SFX
        Info(slide, 0, ((char *)slide, LoadFarString( UnzipUsageLine1),
         UzpVersionStr(), UZ_VERSION_DATE,
         "  Maintainer: Steven M. Schweda"));
        Info(slide, 0, ((char *)slide,
          LoadFarString(UnzipVersionLine)));
#  ifdef BETA
        Info( slide, 0, ((char *)slide, LoadFarString( BetaVersion),
         "", ""));
        Info( slide, 0, ((char *)slide, "%s", "\n"));
#  endif
        version(__G);
#  endif /* ndef SFX */

        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptions)));
#  ifdef ACORN_FTYPE_NFS
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(AcornFtypeNFS)));
        ++numopts;
#  endif
#  ifdef APPLE_NFRSRC_MSG
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(AppleNFRSRC)));
        ++numopts;
#  endif /* def APPLE_NFRSRC */
#  ifdef APPLE_XATTR
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(AppleXATTR)));
        ++numopts;
#  endif /* def APPLE_XATTR */
#  ifdef ASM_CRC
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(AsmCRC)));
        ++numopts;
#  endif
#  ifdef ARCHIVE_STDIN
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(ArchiveStdin)));
        ++numopts;
#  endif
#  ifdef ASM_INFLATECODES
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(AsmInflateCodes)));
        ++numopts;
#  endif
#  ifdef CHECK_VERSIONS
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Check_Versions)));
        ++numopts;
#  endif
#  if defined( _MSC_VER) && defined( _DEBUG)
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(WinDebug)));
        ++numopts;
#  endif
#  ifdef DEBUG
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(UDebug)));
        ++numopts;
#  endif
#  ifdef DEBUG_TIME
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(DebugTime)));
        ++numopts;
#  endif
#  ifdef DLL
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Dll)));
        ++numopts;
#  endif
#  ifdef DOSWILD
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(DosWild)));
        ++numopts;
#  endif
#  ifdef EXDIR_RENAME
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(ExdirRename)));
        ++numopts;
#  endif
#  ifdef ICONV_MAPPING
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Iconv)));
        ++numopts;
#  endif /* def ICONV_MAPPING */
#  ifdef IZ_HAVE_UXUIDGID
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(ux_Uid_Gid)));
        ++numopts;
#  endif
#  ifdef LZW_CLEAN
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(LZW_Clean)));
        ++numopts;
#  endif
#  ifndef MORE
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(No_More)));
        ++numopts;
#  endif
#  ifdef NO_ZIPINFO
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(No_ZipInfo)));
        ++numopts;
#  endif
#  ifdef NTSD_EAS
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(NTSDExtAttrib)));
        ++numopts;
#  endif
#  if defined(WIN32) && defined(NO_W32TIMES_IZFIX)
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(W32NoIZTimeFix)));
        ++numopts;
#  endif
#  ifdef OLD_THEOS_EXTRA
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(OldTheosExtra)));
        ++numopts;
#  endif
#  ifdef OS2_EAS
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(OS2ExtAttrib)));
        ++numopts;
#  endif
#  ifdef QLZIP
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(SMSExFldOnUnix)));
        ++numopts;
#  endif
#  ifdef REENTRANT
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Reentrant)));
        ++numopts;
#  endif
#  ifdef REGARGS
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(RegArgs)));
        ++numopts;
#  endif
#  ifdef RETURN_CODES
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Return_Codes)));
        ++numopts;
#  endif
#  ifdef SET_DIR_ATTRIB
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(SetDirAttrib)));
        ++numopts;
#  endif
#  ifdef SYMLINKS
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(SymLinkSupport)));
        ++numopts;
#  endif
#  ifdef TIMESTAMP
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(TimeStamp)));
        ++numopts;
#  endif
#  ifdef UNIXBACKUP
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(UnixBackup)));
        ++numopts;
#  endif
#  ifdef USE_EF_UT_TIME
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Use_EF_UT_time)));
        ++numopts;
#  endif
#  ifndef LZW_CLEAN
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Unshrink_Sup)));
        ++numopts;
#  endif
#  ifdef DEFLATE64_SUPPORT
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Deflate64_Sup)));
        ++numopts;
#  endif /* def DEFLATE64_SUPPORT */
#  ifdef UNICODE_SUPPORT
#   ifdef UTF8_MAYBE_NATIVE
        sprintf((char *)(slide+256), LoadFarStringSmall(Unicode_Sup),
          LoadFarStringSmall2(G.native_is_utf8 ? SysChUTF8 : SysChOther));
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          (char *)(slide+256)));
#   else
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Unicode_Sup)));
#   endif
        ++numopts;
#  endif
#  ifdef WIN32_WIDE
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Win32_Wide_Sup)));
        ++numopts;
#  endif
#  ifdef _MBCS
        sprintf((char *)(slide+256), LoadFarStringSmall(Have_MBCS_Support),
          (unsigned int)MB_CUR_MAX);
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          (char *)(slide+256)));
        ++numopts;
#  endif
#  ifdef MULT_VOLUME
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(MultiVol_Sup)));
        ++numopts;
#  endif
#  ifdef LARGE_FILE_SUPPORT
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(LFS_Sup)));
        ++numopts;
#  endif
#  ifdef ZIP64_SUPPORT
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Zip64_Sup)));
        ++numopts;
#  endif
#  if (defined(__DJGPP__) && (__DJGPP__ >= 2))
#   ifdef USE_DJGPP_ENV
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Use_DJGPP_Env)));
        ++numopts;
#   endif
#   ifdef USE_DJGPP_GLOB
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Use_DJGPP_Glob)));
        ++numopts;
#   endif
#  endif /* __DJGPP__ && (__DJGPP__ >= 2) */
#  ifdef USE_VFAT
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Use_VFAT_support)));
        ++numopts;
#  endif
#  ifdef USE_ZLIB
        sprintf((char *)(slide+256), LoadFarStringSmall(UseZlib),
          ZLIB_VERSION, zlibVersion());
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          (char *)(slide+256)));
        ++numopts;
#  endif
#  ifdef BZIP2_SUPPORT
        sprintf((char *)(slide+256), LoadFarStringSmall(BZip2_Sup),
          BZ2_bzlibVersion());
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          (char *)(slide+256)));
        ++numopts;
#  endif /* def BZIP2_SUPPORT */
#  ifdef LZMA_SUPPORT
        sprintf((char *)(slide+256), LoadFarStringSmall(LZMA_Sup),
         MY_VERSION);
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
         (char *)(slide+256)));
        ++numopts;
#  endif /* def LZMA_SUPPORT */
#  ifdef PPMD_SUPPORT
        sprintf((char *)(slide+256), LoadFarStringSmall(PPMD_Sup),
         MY_VERSION);
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
         (char *)(slide+256)));
        ++numopts;
#  endif /* def PPMD_SUPPORT */
#  if defined( USE_UNREDUCE_PUBLIC) || defined( USE_UNREDUCE_SMITH)
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Use_Unreduce)));
#   ifdef USE_UNREDUCE_SMITH
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(Use_UnreduceSc)));
#   endif
        ++numopts;
#  endif
#  ifdef VMS_TEXT_CONV
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(VmsTextConv)));
        ++numopts;
#  endif
#  ifdef VMSCLI
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(VmsCLI)));
        ++numopts;
#  endif
#  ifdef VMSWILD
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(VmsWild)));
        ++numopts;
#  endif
#  ifdef WILD_STOP_AT_DIR
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(WildStopAtDir)));
        ++numopts;
#  endif
#  ifdef IZ_CRYPT_ANY
#   ifdef PASSWD_FROM_STDIN
        Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
          LoadFarStringSmall(PasswdStdin)));
        ++numopts;
#   endif /* def IZ_CRYPT_ANY */
#   ifdef IZ_CRYPT_TRAD
        Info(slide, 0, ((char *)slide, LoadFarString(Decryption),
          CR_MAJORVER, CR_MINORVER, CR_BETA_VER));
        ++numopts;
#   endif /* def IZ_CRYPT_TRAD */
#   ifdef IZ_CRYPT_AES_WG
        Info(slide, 0, ((char *)slide, LoadFarStringSmall(AesWgDecryption),
          IZ_AES_WG_MAJORVER, IZ_AES_WG_MINORVER, IZ_AES_WG_BETA_VER));
        ++numopts;
#   endif /* def IZ_CRYPT_AES_WG */
#  endif /* def IZ_CRYPT_ANY */
        if (numopts == 0)
            Info(slide, 0, ((char *)slide,
              LoadFarString(CompileOptFormat),
              LoadFarStringSmall(None)));

#  ifdef IZ_CRYPT_TRAD
        Info(slide, 0, ((char *)slide,
         LoadFarString(TraditionalEncryptionNotice1)));
        Info(slide, 0, ((char *)slide,
         LoadFarString(TraditionalEncryptionNotice2)));
#  endif /* def IZ_CRYPT_TRAD */

#  ifdef IZ_CRYPT_AES_WG
        Info(slide, 0, ((char *)slide,
         LoadFarString(AesWgEncryptionNotice1)));
        Info(slide, 0, ((char *)slide,
         LoadFarString(AesWgEncryptionNotice2)));
#  endif /* def IZ_CRYPT_AES_WG */

#  ifndef _WIN32_WCE /* Win CE does not support environment variables */
        show_env( __G__ 0);
#  endif /* !_WIN32_WCE */
    }
} /* end function show_version() */

# endif /* !defined( SFX) || defined( DIAG_SFX) */
#endif /* ndef WINDLL_OLD */






/*---------------------------------------------------------------
 *  Long option support
 *  8/23/2003
 *  Updated 3/1/2008 to support UnZip
 *
 *  Defines function get_option() to get and process the command
 *  line options and arguments from argv[].  The caller calls
 *  get_option() in a loop to get either one option and possible
 *  value or a non-option argument each loop.
 *
 *  This version has been modified to work with UnZip and ZipInfo.
 *  the major changes are removing the error returns, instead
 *  passing back error codes for errors, and supporting separate
 *  groups of options for UnZip and ZipInfo and selecting the option
 *  group by an argument.
 *
 *  This version does not include argument file support and can
 *  work directly on argv.  The argument file code complicates things and
 *  it seemed best to leave it out for now.  If argument file support
 *  (reading in command line arguments stored in a file and inserting into
 *  command line where @filename is found) is added later the arguments
 *  can change and a freeable copy of argv will be needed and can be
 *  created using copy_args in the left out code.
 *
 *  Supports short and long options as defined in the array options[]
 *  in zip.c, multiple short options in an argument (like -jlv), long
 *  option abbreviation (like --te for --temp-file if --te unique),
 *  short and long option values (like -b filename or --temp-file filename
 *  or --temp-file=filename), optional and required values, option negation
 *  by trailing - (like -S- to not include hidden and system files in MSDOS),
 *  value lists (like -x a b c), argument permuting (returning all options
 *  and values before any non-option arguments), and argument files (where
 *  any non-option non-value argument in form @path gets substituted with
 *  the white space separated arguments in the text file at path).  In this
 *  version argument file support has been removed to simplify development
 *  but may be added later.
 *
 *  E. Gordon
 */


/* message output - char casts are needed to handle constants */
#define oERR(err, message) Info(slide, 0x401, ((char *)slide, (char *)message))
#define oWARN(message) Info(slide, 0x401, ((char *)slide, (char *)message))



/* Although the below provides some support for multibyte characters
   the proper thing to do may be to use wide characters and support
   Unicode.  May get to it soon.  Wide support would likely require
   the ability to convert the command line to wide strings, which most
   modern OS should support now.  EG
 */

/* For now stay with multi-byte characters.  May support wide characters
   in Zip 3.1 and UnZip 6.1.
 */

/* multibyte character set support
   Multibyte characters use typically two or more sequential bytes
   to represent additional characters than can fit in a single byte
   character set.  The code used here is based on the ANSI mblen function. */
#define MB_CLEN(ptr) CLEN(ptr)
#define MB_NEXTCHAR(ptr) PREINCSTR(ptr)


/* constants */

/* function get_args_from_arg_file() can return this in depth parameter */
#define ARG_FILE_ERR -1

/* Symbolic values for optchar. */
#define SKIP_VALUE_ARG           -1
#define SKIP_VALUE_ARG2          -2     /* 2012-03-08 SMS. */
#define THIS_ARG_DONE            -3
#define START_VALUE_LIST         -4
#define IN_VALUE_LIST            -5
#define NON_OPTION_ARG           -6
#define STOP_VALUE_LIST          -7
#define READ_REST_ARGS_VERBATIM  -8     /* 7/25/04 EG */


/* buffer for error messages (this sizing is a guess but must hold 2 paths) */
#define OPTIONERR_BUF_SIZE (80+ 2* PATH_MAX)
char optionerrbuf[OPTIONERR_BUF_SIZE + 1];

/* error messages */
static ZCONST char Far op_not_neg_err[] =
   "option %s not negatable\n";
static ZCONST char Far op_req_val_err[] =
   "option %s requires a value\n";
static ZCONST char Far op_no_allow_val_err[] =
   "option %s does not allow a value\n";
static ZCONST char Far sh_op_not_sup_err[] =
   "short option '%c' not supported\n";
static ZCONST char Far oco_req_val_err[] =
   "option %s requires one character value\n";
static ZCONST char Far oco_no_mbc_err[] =
   "option %s does not support multibyte values\n";
static ZCONST char Far num_req_val_err[] =
   "option %s requires number value\n";
static ZCONST char Far long_op_ambig_err[] =
   "long option '%s' ambiguous\n";
static ZCONST char Far long_op_not_sup_err[] =
   "long option '%s' not supported\n";

static ZCONST char Far no_arg_files_err[] = "argument files not enabled\n";


/* below removed as only used for processing argument files */

/* get_nextarg */
/* get_args_from_string */
/* get_args_from_arg_file */


/* copy error, option name, and option description if any to buf */
static int optionerr( opts, buf, err, optind, islong)
  ZCONST struct option_struct *opts;
  char *buf;
  ZCONST char Far *err;
  int optind;
  int islong;
{
  char optname[50];

  if (opts[optind].name && opts[optind].name[0] != '\0') {
    sprintf(optname, "'%s' (%s)",
            LoadFarStringSmall2(islong ? opts[optind].longopt
                                       : opts[optind].shortopt),
            LoadFarStringSmall(opts[optind].name));
  } else {
    sprintf(optname, "'%s'",
            LoadFarStringSmall2(islong ? opts[optind].longopt
                                       : opts[optind].shortopt));
  }
  sprintf(buf, LoadFarStringSmall(err), optname);
  return 0;
}


/* copy_args
 *
 * Copy arguments in args, allocating storage with malloc.
 * Copies until a NULL argument is found or until max_args args
 * including args[0] are copied.  Set max_args to 0 to copy
 * until NULL.  Always terminates returned args[] with NULL arg.
 *
 * Any argument in the returned args can be freed with free().  Any
 * freed argument should be replaced with either another string
 * allocated with malloc or by NULL if last argument so that free_args
 * will properly work.
 */
char **copy_args(__G__ args, max_args)
  __GDEF
  char **args;
  int max_args;
{
  int j;
  char **new_args;

  if (args == NULL) {
    return NULL;
  }

  /* Count non-NULL args.  Stop at max_args, if not zero. */
  for (j = 0; args[ j] && (max_args == 0 || j < max_args); j++);
  max_args = j;

  if ((new_args = (char **) izu_malloc(
   (max_args + 1) * sizeof(char *))) == NULL)
  {
    oWARN("memory - ca.1");
    return NULL;
  }

  /* Transfer (non-NULL) original args[] to new_args[]. */
  for (j = 0; j < max_args; j++)
  {
    if ((new_args[ j] = izu_malloc( strlen( args[ j])+ 1)) == NULL)
    {
      free_args( new_args);
      oWARN("memory - ca.2");
      return NULL;
    }
    strcpy( new_args[ j], args[ j]);
  }

  /* NULL_terminate new_args[]. */
  new_args[ max_args] = NULL;

  return new_args;
}


/* count args - count args in argv like array */
int arg_count(__G__ args)
  __GDEF
  char **args;
{
  int i;

  if (args == NULL) {
    return 0;
  }

  for (i = 0; args[i]; i++) {
  }
  return i;
}


/* free args - free args created with one of these functions */
int free_args( args)
  char **args;
{
  int i;

  if (args == NULL) {
    return 0;
  }

  for (i = 0; args[i]; i++) {
    izu_free(args[i]);
  }
  izu_free(args);
  return i;
}


/* insert_arg
 *
 * Insert the argument arg into the array *pargs before argument at_arg.
 * If at_arg = -1 then append to end.
 * Return the new count of arguments (argc).
 *
 * If free_args is true, this function frees the old args array
 * (but not the component strings).  DO NOT set free_args on original
 * argv but only on args allocated with malloc.
 */

int insert_arg(__G__ pargs, arg, at_arg, free_args)
  __GDEF
  char ***pargs;
  ZCONST char *arg;
  int at_arg;
  int free_args;
{
  char *newarg = NULL;
  char **args;
  char **newargs = NULL;
  int argnum;
  int newargnum;
  int argcnt;
  int newargcnt;

  if (pargs == NULL) {
    return 0;
  }
  args = *pargs;

  /* count args */
  if (args == NULL) {
    argcnt = 0;
  } else {
    for (argcnt = 0; args[argcnt]; argcnt++) ;
  }
  if (arg == NULL) {
    /* done */
    return argcnt;
  }
  if (at_arg == -1) {
    at_arg = argcnt;
  }
  newargcnt = argcnt + 1;

  /* get storage for new args */
  if ((newargs = (char **) izu_malloc(
   (newargcnt + 1) * sizeof(char *))) == NULL)
  {
    oWARN("memory - ia.1");
    return 0;
  }

  /* copy argument pointers from args to position at_arg, copy the new arg,
     then copy the rest of the args */
  argnum = 0;
  newargnum = 0;
  if (args) {
    for (; args[argnum] && argnum < at_arg; argnum++) {
      newargs[newargnum++] = args[argnum];
    }
  }
  /* copy new arg */
  if ((newarg = (char *) izu_malloc(strlen(arg) + 1)) == NULL) {
    oWARN("memory - ia.2");
    return 0;
  }
  strcpy(newarg, arg);

  newargs[newargnum++] = newarg;
  if (args) {
    for ( ; args[argnum]; argnum++) {
      newargs[newargnum++] = args[argnum];
    }
  }
  newargs[newargnum] = NULL;

  /* free old args array but not component strings - this assumes that
   * args was allocated with malloc as copy_args does.  DO NOT DO THIS
   * on the original argv.
   */
  if (free_args)
    izu_free(args);

  *pargs = newargs;

  return newargnum;
}

/* ------------------------------------- */

/* get_shortopt
 *
 * Get next short option from arg.  The state is stored in argnum, optchar, and
 * option_num so no static storage is used.  Returns the option_ID.
 *
 * parameters:
 *    args         - argv array of arguments
 *    argnum       - index of current arg in args
 *    optchar      - pointer to index of next char to process.  Can be 0 or
 *                   const defined at top of this file like THIS_ARG_DONE
 *    negated      - on return pointer to int set to 1 if option negated
 *                   or 0 otherwise
 *    value        - on return pointer to string set to value of option if any
 *                   or NULL if none.  If value is returned then the caller
 *                   should free() it when not needed anymore.
 *    option_num   - pointer to index in options[] of returned option or
 *                   o_NO_OPTION_MATCH if none.  Do not change as used by
 *                   value lists.
 *    depth        - recursion depth (0 at top level, 1 or more in arg files)
 */
static unsigned long get_shortopt(__G__ opts, args, argnum, optchar,
                                  negated, value, option_num, depth)
  __GDEF
  ZCONST struct option_struct *opts;
  ZCONST char **args;
  int argnum;
  int *optchar;
  int *negated;
  char **value;
  int *option_num;
  int depth;
{
  ZCONST char *shortopt;
  size_t clen;
  ZCONST char *nextchar;
  ZCONST char *s;
  ZCONST char *start;
  int op;
  ZCONST char *arg;
  int match = -1;


  /* get arg */
  arg = args[argnum];
  /* current char in arg */
  nextchar = arg + (*optchar);
  clen = MB_CLEN(nextchar);
  /* next char in arg */
  (*optchar) +=  (int)clen;
  /* get first char of short option */
  shortopt = arg + (*optchar);
  /* no value */
  *value = NULL;

  if (*shortopt == '\0') {
    /* no more options in arg */
    *optchar = 0;
    *option_num = o_NO_OPTION_MATCH;
    return 0;
  }

  /* look for match in options */
  clen = MB_CLEN(shortopt);
  for (op = 0; opts[op].option_ID; op++) {
      s = opts[op].shortopt;
      if (s && s[0] == shortopt[0]) {
        if (s[1] == '\0' && clen == 1) {
          /* single char match */
          match = op;
        } else {
          /* 2 wide short opt.  Could support more chars but should use long opts instead */
          if (s[1] == shortopt[1]) {
            /* match 2 char short opt or 2 byte char */
            match = op;
            if (clen == 1) (*optchar)++;
            break;
          }
        }
      }
  }

  if (match > -1) {
    /* match */
    clen = MB_CLEN(shortopt);
    nextchar = arg + (*optchar) + clen;
    /* check for trailing dash negating option */
    if (*nextchar == '-') {
      /* negated */
      if (opts[match].negatable == o_NOT_NEGATABLE) {
        if (opts[match].value_type == o_NO_VALUE) {
          optionerr( opts, optionerrbuf, op_not_neg_err, match, 0);
          if (depth > 0) {
            /* unwind */
            oWARN(optionerrbuf);
            return o_ARG_FILE_ERR;
          } else {
            oWARN(optionerrbuf);
            return o_BAD_ERR;
          }
        }
      } else {
        *negated = 1;
        /* set up to skip negating dash */
        (*optchar) += (int)clen;
        clen = 1;
      }
    }

    /* value */
    clen = MB_CLEN(arg + (*optchar));
    /* optional value, one char value, and number value must follow option */
    if (opts[match].value_type == o_ONE_CHAR_VALUE) {
      /* one char value */
      if (arg[(*optchar) + clen]) {
        /* has value */
        if (MB_CLEN(arg + (*optchar) + clen) > 1) {
          /* multibyte value not allowed for now */
          optionerr( opts, optionerrbuf, oco_no_mbc_err, match, 0);
          if (depth > 0) {
            /* unwind */
            oWARN(optionerrbuf);
            return o_ARG_FILE_ERR;
          } else {
            oWARN(optionerrbuf);
            return o_BAD_ERR;
          }
        }
        if ((*value = (char *) izu_malloc(2)) == NULL) {
          oWARN("memory - gso.1");
          return o_BAD_ERR;
        }
        (*value)[0] = *(arg + (*optchar) + clen);
        (*value)[1] = '\0';
        *optchar += (int)clen;
        clen = 1;
      } else {
        /* one char values require a value */
        optionerr( opts, optionerrbuf, oco_req_val_err, match, 0);
        if (depth > 0) {
          oWARN(optionerrbuf);
          return o_ARG_FILE_ERR;
        } else {
          oWARN(optionerrbuf);
          return o_BAD_ERR;
        }
      }
    } else if (opts[match].value_type == o_NUMBER_VALUE) {
      /* read chars until end of number */
      start = arg + (*optchar) + clen;
      if (*start == '+' || *start == '-') {
        start++;
      }
      s = start;
      for (; isdigit(*s); MB_NEXTCHAR(s)) ;
      if (s == start) {
        /* no digits */
        optionerr( opts, optionerrbuf, num_req_val_err, match, 0);
        if (depth > 0) {
          oWARN(optionerrbuf);
          return o_ARG_FILE_ERR;
        } else {
          oWARN(optionerrbuf);
          return o_BAD_ERR;
        }
      }
      start = arg + (*optchar) + clen;
      if ((*value = (char *) izu_malloc((int)(s - start) + 1)) == NULL) {
        oWARN("memory - gso.2");
        return o_BAD_ERR;
      }
      *optchar += (int)(s - start);
      strncpy(*value, start, (int)(s - start));
      (*value)[(int)(s - start)] = '\0';
      clen = MB_CLEN(s);
    } else if (opts[match].value_type == o_OPTIONAL_VALUE) {
      /* optional value */
      /* This seemed inconsistent so now if no value attached to argument look
         to the next argument if that argument is not an option for option
         value - 11/12/04 EG */
      if (arg[(*optchar) + clen]) {
        /* has value */
        /* add support for optional = - 2/6/05 EG */
        if (arg[(*optchar) + clen] == '=') {
          /* skip = */
          clen++;
        }
        if (arg[(*optchar) + clen]) {
          if ((*value = (char *)izu_malloc(
           strlen(arg + (*optchar) + clen) + 1)) == NULL) {
            oWARN("memory - gso.3");
            return o_BAD_ERR;
          }
          strcpy(*value, arg + (*optchar) + clen);
        }
        *optchar = THIS_ARG_DONE;
      } else if (args[argnum + 1] && args[argnum + 1][0] != '-') {
        /* use next arg for value */
        if ((*value = (char *)izu_malloc(
         strlen(args[argnum + 1]) + 1)) == NULL) {
          oWARN("memory - gso.4");
          return o_BAD_ERR;
        }
        /* using next arg as value */
        strcpy(*value, args[argnum + 1]);
        *optchar = SKIP_VALUE_ARG;
      }
    } else if (opts[match].value_type == o_OPT_EQ_VALUE) {
      /* Optional value, but "=" required with detached value. */
      int have_eq = 0;

      if (arg[(*optchar) + clen]) {
        /* "-optXXXX".  May have attached value. */
        if (arg[(*optchar) + clen] == '=') {
          /* Skip '=' (but remember it). */
          clen++;
          have_eq = 1;
        }
        if (arg[(*optchar) + clen]) {
          /* "-opt{=|X}XXX".  Have more chars.  Attached value? */
          if (have_eq) {
            /* "-opt=value".  Have attached value. */
            if ((*value = (char *)izu_malloc(
             strlen(arg + (*optchar) + clen) + 1)) == NULL) {
              oERR(ZE_MEM, "gso.5");
            }
            strcpy(*value, arg + (*optchar) + clen);
            *optchar = THIS_ARG_DONE;
          }
            /* else
             * "-opt{^=}XXX".  Have more chars, but no attached value.
             * Should be more short options.
             */
        }
        else if (have_eq && args[argnum + 1] && args[argnum + 1][0] != '-') {
          /* "-opt= value".  Have detached value. */
          if ((*value = (char *)izu_malloc(
           strlen(args[argnum + 2])+ 1)) == NULL) {
            oWARN("memory - gso.6");
            return o_BAD_ERR;
          }
          /* Set value.  Skip value arg. */
          strcpy(*value, args[argnum + 1]);
          *optchar = SKIP_VALUE_ARG;
        } else {
          /* "-opt[=] -opt".  No "=" or no (non-option) value specified. */
          *optchar = THIS_ARG_DONE;
        }
      } else if (args[argnum + 1] && (strcmp( args[argnum + 1], "=") == 0)) {
        /* "-opt = ".  Loose "=" token.  Look for detached value. */
        if (args[argnum + 2] && args[argnum + 2][0] != '-') {
          /* "-opt = value".  Have detached value. */
          if ((*value = (char *)izu_malloc(
           strlen(args[argnum + 2])+ 1)) == NULL) {
            oWARN("memory - gso.7");
            return o_BAD_ERR;
          }
          /* Set value.  Skip "=" and value args. */
          strcpy(*value, args[argnum + 2]);
          *optchar = SKIP_VALUE_ARG2;
        } else {
          /* "-opt =", but no (non-option) value.  Skip "=" arg. */
          *optchar = SKIP_VALUE_ARG;
        }
      } else if (args[argnum + 1] && args[argnum + 1][0] == '=') {
        /* "-opt =[XXXX]".  Have detached "=value".  Skip '='. */
        if ((*value = (char *)izu_malloc(strlen(args[argnum + 1]))) == NULL)
        {
          oWARN("memory - gso.8");
          return o_BAD_ERR;
        }
        /* Using next arg (less '=') as value. */
        strcpy(*value, args[argnum + 1]+ 1);
        *optchar = SKIP_VALUE_ARG;
      } else {
        /* No "=", therefore no value. */
        *optchar = THIS_ARG_DONE;
      }
    } else if (opts[match].value_type == o_REQUIRED_VALUE ||
               opts[match].value_type == o_VALUE_LIST) {
      /* see if follows option */
      if (arg[(*optchar) + clen]) {
        /* has value following option as -ovalue */
        /* add support for optional = - 6/5/05 EG */
        if (arg[(*optchar) + clen] == '=') {
          /* skip = */
          clen++;
        }
        if ((*value = (char *)izu_malloc(strlen(arg + (*optchar) + clen) + 1))
            == NULL) {
          oWARN("memory - gso.9");
          return o_BAD_ERR;
        }
        strcpy(*value, arg + (*optchar) + clen);
        *optchar = THIS_ARG_DONE;
      } else {
        /* use next arg for value */
        if (args[argnum + 1]) {
          if ((*value = (char *)izu_malloc(strlen(args[argnum + 1]) + 1))
              == NULL) {
            oWARN("memory - gso.10");
            return o_BAD_ERR;
          }
          strcpy(*value, args[argnum + 1]);
          if (opts[match].value_type == o_VALUE_LIST) {
            *optchar = START_VALUE_LIST;
          } else {
            *optchar = SKIP_VALUE_ARG;
          }
        } else {
          /* no value found */
          optionerr( opts, optionerrbuf, op_req_val_err, match, 0);
          if (depth > 0) {
            oWARN(optionerrbuf);
            return o_ARG_FILE_ERR;
          } else {
            oWARN(optionerrbuf);
            return o_BAD_ERR;
          }
        }
      }
    }

    *option_num = match;
    return opts[match].option_ID;
  }
  sprintf(optionerrbuf, LoadFarStringSmall(sh_op_not_sup_err), *shortopt);
  if (depth > 0) {
    /* unwind */
    oWARN(optionerrbuf);
    return o_ARG_FILE_ERR;
  } else {
    oWARN(optionerrbuf);
    return o_BAD_ERR;
  }
}


/* get_longopt
 *
 * Get the long option in args array at argnum.
 * Parameters same as for get_shortopt.
 */

static unsigned long get_longopt(__G__ opts, args, argnum, optchar,
                                 negated, value, option_num, depth)
  __GDEF
  ZCONST struct option_struct *opts;
  ZCONST char **args;
  int argnum;
  int *optchar;
  int *negated;
  char **value;
  int *option_num;
  int depth;
{
  char *longopt;
  char *lastchr;
  char *valuestart;
  int op;
  char *arg;
  int match = -1;
  *value = NULL;

  if (args == NULL) {
    *option_num = o_NO_OPTION_MATCH;
    return 0;
  }
  if (args[argnum] == NULL) {
    *option_num = o_NO_OPTION_MATCH;
    return 0;
  }
  /* copy arg so can chop end if value */
  if ((arg = (char *)izu_malloc(strlen(args[argnum]) + 1)) == NULL) {
    oWARN("memory - glo-.1");
    return o_BAD_ERR;
  }
  strcpy(arg, args[argnum]);

  /* get option */
  longopt = arg + 2;
  /* no value */
  *value = NULL;

  /* find = */
  for (lastchr = longopt, valuestart = longopt;
       *valuestart && *valuestart != '=';
       lastchr = valuestart, MB_NEXTCHAR(valuestart)) ;
  if (*valuestart) {
    /* found =value */
    *valuestart = '\0';
    valuestart++;
  } else {
    valuestart = NULL;
  }

  if (*lastchr == '-') {
    /* option negated */
    *negated = 1;
    *lastchr = '\0';
  } else {
    *negated = 0;
  }

  /* look for long option match */
  for (op = 0; opts[op].option_ID; op++) {
      if (opts[op].longopt &&
          strcmp(LoadFarStringSmall(opts[op].longopt), longopt) == 0) {
        /* exact match */
        match = op;
        break;
      }
      if (opts[op].longopt &&
          strncmp(LoadFarStringSmall(opts[op].longopt),
                  longopt, strlen(longopt)) == 0) {
        if (match > -1) {
          sprintf(optionerrbuf, LoadFarStringSmall(long_op_ambig_err),
                  longopt);
          izu_free(arg);
          if (depth > 0) {
            /* unwind */
            oWARN(optionerrbuf);
            return o_ARG_FILE_ERR;
          } else {
            oWARN(optionerrbuf);
            return o_BAD_ERR;
          }
        }
        match = op;
      }

  }

  if (match == -1) {
    sprintf(optionerrbuf, LoadFarStringSmall(long_op_not_sup_err), longopt);
    izu_free(arg);
    if (depth > 0) {
      oWARN(optionerrbuf);
      return o_ARG_FILE_ERR;
    } else {
      oWARN(optionerrbuf);
      return o_BAD_ERR;
    }
  }

  /* one long option an arg */
  *optchar = THIS_ARG_DONE;

  /* if negated then see if allowed */
  if (*negated && opts[match].negatable == o_NOT_NEGATABLE) {
    optionerr( opts, optionerrbuf, op_not_neg_err, match, 1);
    izu_free(arg);
    if (depth > 0) {
      /* unwind */
      oWARN(optionerrbuf);
      return o_ARG_FILE_ERR;
    } else {
      oWARN(optionerrbuf);
      return o_BAD_ERR;
    }
  }
  /* get value */
  if (opts[match].value_type == o_OPT_EQ_VALUE) {
    /* Optional value, but "=" required with detached value. */
    if (valuestart == NULL) {
      /* "--opt ="? */
      if (args[ argnum+ 1] != NULL) {
        /* Next arg exists. */
        if (*args[ argnum+ 1] == '=') {
          /* Next arg is "=" or "=val". */
          if (*(args[ ++argnum]+ 1) == '\0') {
            /* No attached value.  Use next arg, if any. */
            if (args[ argnum+ 1] != NULL) {
              valuestart = (char *)args[ ++argnum];
              *optchar = SKIP_VALUE_ARG2;
            }
          } else {
            /* "=val". */
            valuestart = (char *)args[ argnum]+ 1;
            *optchar = SKIP_VALUE_ARG;
          }
        }
      }
    } else if (*valuestart == '\0') {
      /* "--opt= val"? */
      if (args[ argnum+ 1] != NULL) {
        valuestart = (char *)args[ ++argnum];
        *optchar = SKIP_VALUE_ARG;
      }
    }
    if (valuestart) {
      /* A value was specified somehow.  Save it. */
      if ((*value = (char *)izu_malloc(strlen(valuestart) + 1)) == NULL) {
        izu_free(arg);
        oWARN("memory - glo.2");
        return o_BAD_ERR;
      }
      strcpy(*value, valuestart);
    }
  } else if (opts[match].value_type == o_OPTIONAL_VALUE) {
    /* optional value in form option=value */
    if (valuestart) {
      /* option=value */
      if ((*value = (char *)izu_malloc(strlen(valuestart) + 1)) == NULL) {
        izu_free(arg);
        oWARN("memory - glo.3");
        return o_BAD_ERR;
      }
      strcpy(*value, valuestart);
    }
  } else if (opts[match].value_type == o_REQUIRED_VALUE ||
             opts[match].value_type == o_NUMBER_VALUE ||
             opts[match].value_type == o_ONE_CHAR_VALUE ||
             opts[match].value_type == o_VALUE_LIST) {
    /* handle long option one char and number value as required value */
    if (valuestart) {
      /* option=value */
      if ((*value = (char *)izu_malloc(strlen(valuestart) + 1)) == NULL) {
        izu_free(arg);
        oWARN("memory - glo.4");
        return o_BAD_ERR;
      }
      strcpy(*value, valuestart);
    } else {
      /* use next arg */
      if (args[argnum + 1]) {
        if ((*value = (char *)izu_malloc(
         strlen(args[argnum + 1]) + 1)) == NULL) {
          izu_free(arg);
          oWARN("memory - glo.5");
          return o_BAD_ERR;
        }
        /* using next arg as value */
        strcpy(*value, args[argnum + 1]);
        if (opts[match].value_type == o_VALUE_LIST) {
          *optchar = START_VALUE_LIST;
        } else {
          *optchar = SKIP_VALUE_ARG;
        }
      } else {
        /* no value found */
        optionerr( opts, optionerrbuf, op_req_val_err, match, 1);
        izu_free(arg);
        if (depth > 0) {
          /* unwind */
          oWARN(optionerrbuf);
          return o_ARG_FILE_ERR;
        } else {
          oWARN(optionerrbuf);
          return o_BAD_ERR;
        }
      }
    }
  } else if (opts[match].value_type == o_NO_VALUE) {
    /* this option does not accept a value */
    if (valuestart) {
      /* --option=value */
      optionerr( opts, optionerrbuf, op_no_allow_val_err, match, 1);
      izu_free(arg);
      if (depth > 0) {
        oWARN(optionerrbuf);
        return o_ARG_FILE_ERR;
      } else {
        oWARN(optionerrbuf);
        return o_BAD_ERR;
      }
    }
  }
  izu_free(arg);

  *option_num = match;
  return opts[match].option_ID;
}



/* get_option
 *
 * Main interface for user.  Use this function to get options, values and
 * non-option arguments from a command line provided in argv form.
 *
 * To use get_option() first define valid options by setting
 * the global variable options[] to an array of option_struct.  Also
 * either change defaults below or make variables global and set elsewhere.
 * Zip uses below defaults.
 *
 * Call get_option() to get an option (like -b or --temp-file) and any
 * value for that option (like filename for -b) or a non-option argument
 * (like archive name) each call.  If *value* is not NULL after calling
 * get_option() it is a returned value and the caller should either store
 * the char pointer or free() it before calling get_option() again to avoid
 * leaking memory.  If a non-option non-value argument is returned get_option()
 * returns o_NON_OPTION_ARG and value is set to the entire argument.
 * When there are no more arguments get_option() returns 0.
 *
 * The parameters argnum (after set to 0 on initial call),
 * optchar, first_nonopt_arg, option_num, and depth (after initial
 * call) are set and maintained by get_option() and should not be
 * changed.  The parameters argc, negated, and value are outputs and
 * can be used by the calling program.  get_option() returns either the
 * option_ID for the current option, a special value defined in
 * zip.h, or 0 when no more arguments.
 *
 * The value returned by get_option() is the ID value in the options
 * table.  This value can be duplicated in the table if different
 * options are really the same option.  The index into the options[]
 * table is given by option_num, though the ID should be used as
 * option numbers change when the table is changed.  The ID must
 * not be 0 for any option as this ends the table.  If get_option()
 * finds an option not in the table it calls oERR to post an
 * error and exit.  Errors also result if the option requires a
 * value that is missing, a value is present but the option does
 * not take one, and an option is negated but is not
 * negatable.  Non-option arguments return o_NON_OPTION_ARG
 * with the entire argument in value.
 *
 * For Zip and UnZip, permuting is on and all options and their values
 * are returned before any non-option arguments like archive name.
 *
 * The arguments "-" alone and "--" alone return as non-option arguments.
 * Note that "-" should not be used as part of a short option
 * entry in the table but can be used in the middle of long
 * options such as in the long option "a-long-option".  Now "--" alone
 * stops option processing, returning any arguments following "--" as
 * non-option arguments instead of options.
 *
 * Argument file support is removed from this version. It may be added later.
 *
 * After each call:
 *   argc       is set to the current size of args[] but should not change
 *                with argument file support removed,
 *   argnum     is the index of the current arg,
 *   value      is either the value of the returned option or non-option
 *                argument or NULL if option with no value,
 *   negated    is set if the option was negated by a trailing dash (-)
 *   option_num is set to either the index in options[] for the option or
 *                o_NO_OPTION_MATCH if no match.
 * Negation is checked before the value is read if the option is negatable so
 * that the - is not included in the value.  If the option is not negatable
 * but takes a value then the - will start the value.  If permuting then
 * argnum and first_nonopt_arg are unreliable and should not be used.
 *
 * Command line is read from left to right.  As get_option() finds non-option
 * arguments (arguments not starting with - and that are not values to options)
 * it moves later options and values in front of the non-option arguments.
 * This permuting is turned off by setting G.permute_opts_args to 0.  Then
 * get_option() will return options and non-option arguments in the order
 * found.  Currently permuting is only done after an argument is completely
 * processed so that any value can be moved with options they go with.  All
 * state information is stored in the parameters argnum, optchar,
 * first_nonopt_arg and option_num.  You should not change these after the
 * first call to get_option().  If you need to back up to a previous arg then
 * set argnum to that arg (remembering that args may have been permuted) and
 * set optchar = 0 and first_nonopt_arg to the first non-option argument if
 * permuting.  After all arguments are returned the next call to get_option()
 * returns 0.  The caller can then call free_args(args) if appropriate.
 *
 * get_option() accepts arguments in the following forms:
 *  short options
 *       of 1 and 2 characters, e.g. a, b, cc, d, and ba, after a single
 *       leading -, as in -abccdba.  In this example if 'b' is followed by 'a'
 *       it matches short option 'ba' else it is interpreted as short option
 *       b followed by another option.  The character - is not legal as a
 *       short option or as part of a 2 character short option.
 *
 *       If a short option has a value it immediately follows the option or
 *       if that option is the end of the arg then the next arg is used as
 *       the value.  So if short option e has a value, it can be given as
 *             -evalue
 *       or
 *             -e value
 *       and now
 *             -e=value
 *       but now that = is optional a leading = is stripped for the first.
 *       This change allows optional short option values to be defaulted as
 *             -e=
 *       Either optional or required values can be specified.  Optional values
 *       now use both forms as ignoring the later got confusing.  Any
 *       non-value short options can preceed a valued short option as in
 *             -abevalue
 *       Some value types (one_char and number) allow options after the value
 *       so if oc is an option that takes a character and n takes a number
 *       then
 *             -abocVccn42evalue
 *       returns value V for oc and value 42 for n.  All values are strings
 *       so programs may have to convert the "42" to a number.  See long
 *       options below for how value lists are handled.
 *
 *       Any short option can be negated by following it with -.  Any - is
 *       handled and skipped over before any value is read unless the option
 *       is not negatable but takes a value and then - starts the value.
 *
 *       If the value for an optional value is just =, then treated as no
 *       value.
 *
 *  long options
 *       of arbitrary length are assumed if an arg starts with -- but is not
 *       exactly --.  Long options are given one per arg and can be abbreviated
 *       if the abbreviation uniquely matches one of the long options.
 *       Exact matches always match before partial matches.  If ambiguous an
 *       error is generated.
 *
 *       Values are specified either in the form
 *             --longoption=value
 *       or can be the following arg if the value is required as in
 *             --longoption value
 *       Optional values to long options must be in the first form.
 *
 *       Value lists are specified by o_VALUE_LIST and consist of an option
 *       that takes a value followed by one or more value arguments.
 *       The two forms are
 *             --option=value
 *       or
 *             -ovalue
 *       for a single value or
 *             --option value1 value2 value3 ... --option2
 *       or
 *             -o value1 value2 value3 ...
 *       for a list of values.  The list ends at the next option, the
 *       end of the command line, or at a single "@" argument.
 *       Each value is treated as if it was preceeded by the option, so
 *             --option1 val1 val2
 *       with option1 value_type set to o_VALUE_LIST is the same as
 *             --option1=val1 --option1=val2
 *
 *       Long options can be negated by following the option with - as in
 *             --longoption-
 *       Long options with values can also be negated if this makes sense for
 *       the caller as:
 *             --longoption-=value
 *       If = is not followed by anything it is treated as no value.
 *
 *  @path
 *       Argument files support removed from this version.  It may be added
 *       back later.
 *
 *  non-option argument
 *       is any argument not given above.  If G.permute_opts_args, then
 *       these are returned after all options, otherwise all options and
 *       args are returned in order.  Returns option ID o_NON_OPTION_ARG
 *       and sets value to the argument.
 *
 *
 * Arguments to get_option:
 *  char ***pargs          - pointer to arg array in the argv form
 *  int *argc              - returns the current argc for args incl. args[0]
 *  int *argnum            - the index of the current argument (caller
 *                            should set = 0 on first call and not change
 *                            after that)
 *  int *optchar           - index of next short opt in arg or special
 *  int *first_nonopt_arg  - used by get_option to permute args
 *  int *negated           - option was negated (had trailing -)
 *  char *value            - value of option if any (free when done with it)
 *                            or NULL
 *  int *option_num        - the index in options of the last option returned
 *                            (can be o_NO_OPTION_MATCH)
 *  int recursion_depth    - current depth of recursion
 *                            (always set to 0 by caller)
 *                            (always 0 with argument files support removed)
 *
 *  Caller should only read the returned option ID and the value, negated,
 *  and option_num (if required) parameters after each call.
 *
 *  Ed Gordon
 *  8/24/2003 (last updated 3/1/2008 EG)
 *
 */

unsigned long get_option(__G__ opts, pargs, argc, argnum, optchar, value,
                         negated, first_nonopt_arg, option_num, recursion_depth)
  __GDEF
  ZCONST struct option_struct *opts;
  char ***pargs;
  int *argc;
  int *argnum;
  int *optchar;
  char **value;
  int *negated;
  int *first_nonopt_arg;
  int *option_num;
  int recursion_depth;
{
  char **args;
  unsigned long option_ID;

  int argcnt;
  int first_nonoption_arg;
  char *arg = NULL;
  int h;
  int optc;
  int argn;
  int j;
  int v;
  int read_rest_args_verbatim = 0;  /* 7/25/04 - ignore options and arg files for rest args */

  /* caller should free value or assign it to another
     variable before calling get_option again. */
  *value = NULL;

  /* if args is NULL then done */
  if (pargs == NULL) {
    *argc = 0;
    return 0;
  }
  args = *pargs;
  if (args == NULL) {
    *argc = 0;
    return 0;
  }

  /* count args */
  for (argcnt = 0; args[argcnt]; argcnt++) ;

  /* if no provided args then nothing to do */
  if (argcnt < 1 || (recursion_depth == 0 && argcnt < 2)) {
    *argc = argcnt;
    /* return 0 to note that no args are left */
    return 0;
  }

  *negated = 0;
  first_nonoption_arg = *first_nonopt_arg;
  argn = *argnum;
  optc = *optchar;

  if (optc == READ_REST_ARGS_VERBATIM) {
    read_rest_args_verbatim = 1;
  }

  if (argn == -1 || (recursion_depth == 0 && argn == 0)) {
    /* first call */
    /* if depth = 0 then args[0] is argv[0] so skip */
    *option_num = o_NO_OPTION_MATCH;
    optc = THIS_ARG_DONE;
    first_nonoption_arg = -1;
  }

  /* if option_num is set then restore last option_ID in case continuing
     value list */
  option_ID = 0;
  if (*option_num != o_NO_OPTION_MATCH) {
    option_ID = opts[*option_num].option_ID;
  }

  /* get next option if any */
  for (;;)  {
    if (read_rest_args_verbatim) {
      /* Args after "--" are non-option args, if G.dashdash_ends_opts. */
      argn++;
      if (argn > argcnt || args[argn] == NULL) {
        /* done */
        option_ID = 0;
        break;
      }
      arg = args[argn];
      if ((*value = (char *)izu_malloc(strlen(arg) + 1)) == NULL) {
        oWARN("memory - go.1");
        return o_BAD_ERR;
      }
      strcpy(*value, arg);
      *option_num = o_NO_OPTION_MATCH;
      option_ID = o_NON_OPTION_ARG;
      break;

    /* permute non-option args after option args so options are returned
       first */
    } else if (G.permute_opts_args) {
      if (optc == SKIP_VALUE_ARG || optc == SKIP_VALUE_ARG2 ||
          optc == THIS_ARG_DONE ||
          optc == START_VALUE_LIST || optc == IN_VALUE_LIST ||
          optc == STOP_VALUE_LIST) {
        /* moved to new arg */
        if (first_nonoption_arg > -1 && args[first_nonoption_arg]) {
          /* do the permuting - move non-options after this option */
          /* if option and value separate args or starting list skip option */
          if (optc == SKIP_VALUE_ARG2) {
            v = 2;
          } else if (optc == SKIP_VALUE_ARG || optc == START_VALUE_LIST) {
            v = 1;
          } else {
            v = 0;
          }
          for (h = first_nonoption_arg; h < argn; h++) {
            arg = args[first_nonoption_arg];
            for (j = first_nonoption_arg; j < argn + v; j++) {
              args[j] = args[j + 1];
            }
            args[j] = arg;
          }
          first_nonoption_arg += 1 + v;
        }
      }
    } else if (optc == NON_OPTION_ARG) {
      /* if not permuting then already returned arg */
      optc = THIS_ARG_DONE;
    }

    /* value lists */
    if (optc == STOP_VALUE_LIST) {
      optc = THIS_ARG_DONE;
    }

    if (optc == START_VALUE_LIST || optc == IN_VALUE_LIST) {
      if (optc == START_VALUE_LIST) {
        /* already returned first value */
        argn++;
        optc = IN_VALUE_LIST;
      }
      argn++;
      arg = args[argn];
      /* if end of args and still in list and there are non-option args then
         terminate list */
      if (arg == NULL && (optc == START_VALUE_LIST || optc == IN_VALUE_LIST)
          && first_nonoption_arg > -1) {
        /* terminate value list with @ */
        /* this is only needed for argument files */
        /* but is also good for show command line so command lines with lists
           can always be read back in */
        argcnt = insert_arg(__G__ &args, "@", first_nonoption_arg, 1);
        argn++;
        if (first_nonoption_arg > -1) {
          first_nonoption_arg++;
        }
      }

      arg = args[argn];
      if (arg && arg[0] == '@' && arg[1] == '\0') {
          /* inserted arguments terminator */
          optc = STOP_VALUE_LIST;
          continue;
      } else if (arg && arg[0] != '-') {  /* not option */
        /* - and -- are not allowed in value lists unless escaped */
        /* another value in value list */
        if ((*value = (char *)izu_malloc(strlen(args[argn]) + 1)) == NULL) {
          oWARN("memory - go.2");
          return o_BAD_ERR;
        }
        strcpy(*value, args[argn]);
        break;

      } else {
        argn--;
        optc = THIS_ARG_DONE;
      }
    }

    /* move to next arg */
    if (optc == SKIP_VALUE_ARG2) {
      argn += 3;
      optc = 0;
    } else if (optc == SKIP_VALUE_ARG) {
      argn += 2;
      optc = 0;
    } else if (optc == THIS_ARG_DONE) {
      argn++;
      optc = 0;
    }
    if (argn > argcnt) {
      break;
    }
    if (args[argn] == NULL) {
      /* done unless permuting and non-option args */
      if (first_nonoption_arg > -1 && args[first_nonoption_arg]) {
        /* return non-option arguments at end */
        if (optc == NON_OPTION_ARG) {
          first_nonoption_arg++;
        }
        /* after first pass args are permuted but skipped over non-option
           args */
        /* swap so argn points to first non-option arg */
        j = argn;
        argn = first_nonoption_arg;
        first_nonoption_arg = j;
      }
      if (argn > argcnt || args[argn] == NULL) {
        /* done */
        option_ID = 0;
        break;
      }
    }

    /* after swap first_nonoption_arg points to end which is NULL */
    if (first_nonoption_arg > -1 && (args[first_nonoption_arg] == NULL)) {
      /* only non-option args left */
      if (optc == NON_OPTION_ARG) {
        argn++;
      }
      if (argn > argcnt || args[argn] == NULL) {
        /* done */
        option_ID = 0;
        break;
      }
      if ((*value = (char *)izu_malloc(strlen(args[argn]) + 1)) == NULL) {
        oWARN("memory - go.3");
        return o_BAD_ERR;
      }
      strcpy(*value, args[argn]);
      optc = NON_OPTION_ARG;
      option_ID = o_NON_OPTION_ARG;
      break;
    }

    arg = args[argn];

    /* is it an option */
    if (arg[0] == '-') {
      /* option */
      if (arg[1] == '\0') {
        /* arg = - */
        /* treat like non-option arg */
        *option_num = o_NO_OPTION_MATCH;
        if (G.permute_opts_args) {
          /* permute args to move all non-option args to end */
          if (first_nonoption_arg < 0) {
            first_nonoption_arg = argn;
          }
          argn++;
        } else {
          /* not permute args so return non-option args when found */
          if ((*value = (char *)izu_malloc(strlen(arg) + 1)) == NULL) {
            oWARN("memory - go.4");
            return o_BAD_ERR;
          }
          strcpy(*value, arg);
          optc = NON_OPTION_ARG;
          option_ID = o_NON_OPTION_ARG;
          break;
        }

      } else if (arg[1] == '-') {
        /* long option */
        if (arg[2] == '\0') {
          /* arg = -- */
          if (G.dashdash_ends_opts) {
            /* Now -- stops permuting and forces the rest of
               the command line to be read verbatim - 7/25/04 EG */

            /* never permute args after -- and return as non-option args */
            if (first_nonoption_arg < 1) {
              /* -- is first non-option argument - 8/7/04 EG */
              argn--;
            } else {
              /* go back to start of non-option args - 8/7/04 EG */
              argn = first_nonoption_arg - 1;
            }

            /* disable permuting and treat remaining arguments as not
               options */
            read_rest_args_verbatim = 1;
            optc = READ_REST_ARGS_VERBATIM;

          } else {
            /* treat like non-option arg */
            *option_num = o_NO_OPTION_MATCH;
            if (G.permute_opts_args) {
              /* permute args to move all non-option args to end */
              if (first_nonoption_arg < 0) {
                first_nonoption_arg = argn;
              }
              argn++;
            } else {
              /* not permute args so return non-option args when found */
              if ((*value = (char *)izu_malloc(strlen(arg) + 1)) == NULL) {
                oWARN("memory - go.5");
                return o_BAD_ERR;
              }
              strcpy(*value, arg);
              optc = NON_OPTION_ARG;
              option_ID = o_NON_OPTION_ARG;
              break;
            }
          }

        } else {
          option_ID = get_longopt(__G__ opts, (ZCONST char **)args, argn,
                                  &optc, negated,
                                  value, option_num, recursion_depth);
          if (option_ID == o_BAD_ERR) {
            return o_BAD_ERR;
          } else if (option_ID == o_ARG_FILE_ERR) {
            /* unwind as only get this if recursion_depth > 0 */
            return option_ID;
          }
          break;
        }

      } else {
        /* short option */
        option_ID = get_shortopt(__G__ opts, (ZCONST char **)args, argn,
                                 &optc, negated,
                                 value, option_num, recursion_depth);

        if (option_ID == o_BAD_ERR) {
          return o_BAD_ERR;
        } else if (option_ID == o_ARG_FILE_ERR) {
          /* unwind as only get this if recursion_depth > 0 */
          return option_ID;
        }

        if (optc == 0) {
          /* if optc = 0 then ran out of short opts this arg */
          optc = THIS_ARG_DONE;
        } else {
          break;
        }
      }
    } else {
      /* non-option */
      if (G.permute_opts_args) {
        /* permute args to move all non-option args to end */
        if (first_nonoption_arg < 0) {
          first_nonoption_arg = argn;
        }
        argn++;
      } else {
        /* no permute args so return non-option args when found */
        if ((*value = (char *)izu_malloc(strlen(arg) + 1)) == NULL) {
          oWARN("memory - go.6");
          return o_BAD_ERR;
        }
        strcpy(*value, arg);
        *option_num = o_NO_OPTION_MATCH;
        optc = NON_OPTION_ARG;
        option_ID = o_NON_OPTION_ARG;
        break;
      }

    }
  }

  *pargs = args;
  *argc = argcnt;
  *first_nonopt_arg = first_nonoption_arg;
  *argnum = argn;
  *optchar = optc;

  return option_ID;
}


/* Ctrl/T (VMS) AST or SIGUSR1 handler for user-triggered progress
 * message.
 * UNIX: arg = signal number.
 * VMS:  arg = Out-of-band character mask.
 */
#ifdef ENABLE_USER_PROGRESS

# ifndef VMS
#  include <limits.h>
#  include <time.h>
#  include <sys/times.h>
#  include <sys/utsname.h>
#  include <unistd.h>
# endif /* ndef VMS */

USER_PROGRESS_CLASS void user_progress( arg)
int arg;
{
  /* VMS Ctrl/T automatically puts out a line like:
   * ALP::_FTA24: 07:59:43 ZIP       CPU=00:00:59.08 PF=2320 IO=52406 MEM=333
   * (host::tty local_time program cpu_time page_faults I/O_ops phys_mem)
   * We do something vaguely similar on non-VMS systems.
   */
# ifdef VMS

  GETGLOBALS();

# else /* def VMS */

#  ifdef CLK_TCK
#   define clk_tck CLK_TCK
#  else /* def CLK_TCK */
  long clk_tck;
#  endif /* def CLK_TCK [else] */
#  define U_P_NODENAME_LEN 32

  static char u_p_prog_name[] = "UnZip";                /* Program name. */

  struct utsname u_p_utsname;
  struct tm u_p_loc_tm;
  struct tms u_p_tms;
  char *cp;
  char *tty_name;
  time_t u_p_time;
  float stime_f;
  float utime_f;

  GETGLOBALS();

  /* On the first time through, get the host name and tty name, and form
   * the "host::tty" string (in G.u_p_nodename) for the intro line.
   */
  if (G.u_p_not_first == 0)
  {
    G.u_p_not_first = 1;
    /* Host name.  (Trim off any domain info.  (Needed on Tru64.)) */
    uname( &u_p_utsname);
    strncpy( G.u_p_nodename, u_p_utsname.nodename, (U_P_NODENAME_LEN- 8));
    G.u_p_nodename[ 24] = '\0';
    cp = strchr( G.u_p_nodename, '.');
    if (cp != NULL)
      *cp = '\0';

    /* Terminal name.  (Trim off any leading "/dev/"). */
    tty_name = ttyname( 0);
    if (tty_name != NULL)
    {
      cp = strstr( tty_name, "/dev/");
      if (cp != NULL)
        tty_name += 5;

      strcat( G.u_p_nodename, "::");
      strncat( G.u_p_nodename, tty_name,
       (U_P_NODENAME_LEN- strlen( G.u_p_nodename)));
    }
  }

  /* Local time.  (Use reentrant localtime_r().) */
  u_p_time = time( NULL);
  localtime_r( &u_p_time, &u_p_loc_tm);

  /* CPU time. */
  times( &u_p_tms);
#  ifndef CLK_TCK
  clk_tck = sysconf( _SC_CLK_TCK);
#  endif /* ndef CLK_TCK */
  utime_f = ((float)u_p_tms.tms_utime)/ clk_tck;
  stime_f = ((float)u_p_tms.tms_stime)/ clk_tck;

  /* Put out intro line. */
  fprintf( stderr, "%s %02d:%02d:%02d %s CPU=%.2f\n",
   G.u_p_nodename,
   u_p_loc_tm.tm_hour, u_p_loc_tm.tm_min, u_p_loc_tm.tm_sec,
   u_p_prog_name,
   (stime_f+ utime_f));

# endif /* def VMS [else] */

  if (G.zipfn != NULL)
  {
    fprintf( stderr, "   Archive: %s\n", G.zipfn);
  }

  if (*G.filename != '\0')
  { /* Repeat the extraction message (to the extent possible). */
    fprintf( stderr, ActionMsg,
     ((G.action_msg_str == NULL) ? "????" : G.action_msg_str),
     FnFilter1( G.filename), "", "");
  }

# ifndef VMS
  /* Re-establish this SIGUSR1 handler.
   * (On VMS, the Ctrl/T handler persists.)
   */
  signal( SIGUSR1, user_progress);
# endif /* ndef VMS */
}

#endif /* def ENABLE_USER_PROGRESS */


#ifdef MEMDIAG
# include "memdiag.c"
#endif /* def MEMDIAG */

