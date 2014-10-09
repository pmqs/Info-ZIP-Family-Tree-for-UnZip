/*---------------------------------------------------------------------------

  unzip.h (new)

  Copyright (c) 1990-2014 Info-ZIP.  All rights reserved.

  This header file contains the public macros and typedefs required by
  both the UnZip sources and by any application using the UnZip API.  If
  UNZIP_INTERNAL is defined, it includes unzpriv.h (containing includes,
  prototypes and extern variables used by the actual UnZip sources).

  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
This is version 2009-Jan-02 of the Info-ZIP license.
The definitive version of this document should be available at
ftp://ftp.info-zip.org/pub/infozip/license.html indefinitely and
a copy at http://www.info-zip.org/pub/infozip/license.html.


Copyright (c) 1990-2012 Info-ZIP.  All rights reserved.

For the purposes of this copyright and license, "Info-ZIP" is defined as
the following set of individuals:

   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
   Jean-loup Gailly, Hunter Goatley, Ed Gordon, Ian Gorman, Chris Herborth,
   Dirk Haase, Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz,
   David Kirschbaum, Johnny Lee, Onno van der Linden, Igor Mandrichenko,
   Steve P. Miller, Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs,
   Kai Uwe Rommel, Steve Salisbury, Dave Smith, Steven M. Schweda,
   Christian Spieler, Cosmin Truta, Antoine Verheijen, Paul von Behren,
   Rich Wales, Mike White.

This software is provided "as is," without warranty of any kind, express
or implied.  In no event shall Info-ZIP or its contributors be held liable
for any direct, indirect, incidental, special or consequential damages
arising out of the use of or inability to use this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the above disclaimer and the following restrictions:

    1. Redistributions of source code (in whole or in part) must retain
       the above copyright notice, definition, disclaimer, and this list
       of conditions.

    2. Redistributions in binary form (compiled executables and libraries)
       must reproduce the above copyright notice, definition, disclaimer,
       and this list of conditions in documentation and/or other materials
       provided with the distribution.  Additional documentation is not needed
       for executables where a command line license option provides these and
       a note regarding this option is in the executable's startup banner.  The
       sole exception to this condition is redistribution of a standard
       UnZipSFX binary (including SFXWiz) as part of a self-extracting archive;
       that is permitted without inclusion of this license, as long as the
       normal SFX banner has not been removed from the binary or disabled.

    3. Altered versions--including, but not limited to, ports to new operating
       systems, existing ports with new graphical interfaces, versions with
       modified or added functionality, and dynamic, shared, or static library
       versions not from Info-ZIP--must be plainly marked as such and must not
       be misrepresented as being the original source or, if binaries,
       compiled from the original source.  Such altered versions also must not
       be misrepresented as being Info-ZIP releases--including, but not
       limited to, labeling of the altered versions with the names "Info-ZIP"
       (or any variation thereof, including, but not limited to, different
       capitalizations), "Pocket UnZip," "WiZ" or "MacZip" without the
       explicit permission of Info-ZIP.  Such altered versions are further
       prohibited from misrepresentative use of the Zip-Bugs or Info-ZIP
       e-mail addresses or the Info-ZIP URL(s), such as to imply Info-ZIP
       will provide support for the altered versions.

    4. Info-ZIP retains the right to use the names "Info-ZIP," "Zip," "UnZip,"
       "UnZipSFX," "WiZ," "Pocket UnZip," "Pocket Zip," and "MacZip" for its
       own source and binary releases.
  ---------------------------------------------------------------------------*/

#ifndef __unzip_h   /* prevent multiple inclusions */
# define __unzip_h

/*---------------------------------------------------------------------------
    Predefined, machine-specific macros.
  ---------------------------------------------------------------------------*/

# ifdef __GO32__                /* MS-DOS extender:  NOT Unix */
#  ifdef unix
#   undef unix
#  endif
#  ifdef _unix
#   undef _unix
#  endif
#  ifdef __unix
#   undef __unix
#  endif
#  ifdef __unix__
#   undef __unix__
#  endif
# endif

# if ((defined(__convex__) || defined(__convexc__)) && !defined(CONVEX))
#  define CONVEX
# endif

# if (defined(unix) || defined(_unix) || defined(__unix) || defined(__unix__))
#  ifndef UNIX
#   define UNIX
#  endif
# endif /* unix || _unix || __unix || __unix__ */
# if (defined(M_XENIX) || defined(COHERENT) || defined(__hpux))
#  ifndef UNIX
#   define UNIX
#  endif
# endif /* M_XENIX || COHERENT || __hpux */
# if (defined(__NetBSD__) || defined(__FreeBSD__))
#  ifndef UNIX
#   define UNIX
#  endif
# endif /* __NetBSD__ || __FreeBSD__ */
# if (defined(CONVEX) || defined(MINIX) || defined(_AIX) || defined(__QNX__))
#  ifndef UNIX
#   define UNIX
#  endif
# endif /* CONVEX || MINIX || _AIX || __QNX__ */

# if (defined(VM_CMS) || defined(MVS))
#  define CMS_MVS
# endif

# if (defined(__OS2__) && !defined(OS2))
#  define OS2
# endif

# if (defined(__TANDEM) && !defined(TANDEM))
#  define TANDEM
# endif

# if (defined(__VMS) && !defined(VMS))
#  define VMS
# endif

# if ((defined(__WIN32__) || defined(_WIN32)) && !defined(WIN32))
#  define WIN32
# endif
# if ((defined(__WINNT__) || defined(__WINNT)) && !defined(WIN32))
#  define WIN32
# endif

# if defined(_WIN32_WCE)
#  ifndef WIN32         /* WinCE is treated as a variant of the Win32 API */
#   define WIN32
#  endif
#  ifndef UNICODE       /* WinCE requires UNICODE wide character support */
#   define UNICODE
#  endif
# endif

# ifdef __COMPILER_KCC__
#  include <c-env.h>
#  ifdef SYS_T20
#   define TOPS20
#  endif
# endif /* __COMPILER_KCC__ */

/* Borland C does not define __TURBOC__ if compiling for a 32-bit platform */
# ifdef __BORLANDC__
#  ifndef __TURBOC__
#   define __TURBOC__
#  endif
#  if (!defined(__MSDOS__) && !defined(OS2) && !defined(WIN32))
#   define __MSDOS__
#  endif
# endif

/* define MSDOS for Turbo C (unless OS/2) and Power C as well as Microsoft C */
# ifdef __POWERC
#  define __TURBOC__
#  define MSDOS
# endif /* __POWERC */

# if (defined(__MSDOS__) && !defined(MSDOS))   /* just to make sure */
#  define MSDOS
# endif

/* RSXNTDJ (at least up to v1.3) compiles for WIN32 (RSXNT) using a derivative
   of the EMX environment, but defines MSDOS and __GO32__. ARG !!! */
# if (defined(MSDOS) && defined(WIN32))
#  undef MSDOS                  /* WIN32 is >>>not<<< MSDOS */
# endif
# if (defined(__GO32__) && defined(__EMX__) && defined(__RSXNT__))
#  undef __GO32__
# endif

# if (defined(linux) && !defined(LINUX))
#  define LINUX
# endif

# ifdef __riscos
#  define RISCOS
# endif

# if (defined(THINK_C) || defined(MPW))
#  define MACOS
# endif
# if (defined(__MWERKS__) && defined(macintosh))
#  define MACOS
# endif

/* use prototypes and ANSI libraries if __STDC__, or MS-DOS, or OS/2, or Win32,
 * or IBM C Set/2, or Borland C, or Watcom C, or GNU gcc (emx or Cygwin),
 * or Macintosh, or Sequent, or Atari, or IBM RS/6000, or Silicon Graphics,
 * or Convex?, or AtheOS, or BeOS.
 */
# if (defined(__STDC__) || defined(MSDOS) || defined(OS2) || defined(WIN32))
#  ifndef PROTO
#   define PROTO
#  endif
#  ifndef MODERN
#   define MODERN
#  endif
# endif
# if (defined(__IBMC__) || defined(__BORLANDC__) || defined(__WATCOMC__))
#  ifndef PROTO
#   define PROTO
#  endif
#  ifndef MODERN
#   define MODERN
#  endif
# endif
# if (defined(__EMX__) || defined(__CYGWIN__))
#  ifndef PROTO
#   define PROTO
#  endif
#  ifndef MODERN
#   define MODERN
#  endif
# endif
# if (defined(MACOS) || defined(ATARI_ST) || defined(RISCOS) || defined(THEOS))
#  ifndef PROTO
#   define PROTO
#  endif
#  ifndef MODERN
#   define MODERN
#  endif
# endif
/* Sequent running Dynix/ptx:  non-modern compiler */
# if (defined(_AIX) || defined(sgi) || (defined(_SEQUENT_) && !defined(PTX)))
#  ifndef PROTO
#   define PROTO
#  endif
#  ifndef MODERN
#   define MODERN
#  endif
# endif
# if (defined(CMS_MVS) || defined(__ATHEOS__) || defined(__BEOS__))
/* || defined(CONVEX) ? */
#  ifndef PROTO
#   define PROTO
#  endif
#  ifndef MODERN
#   define MODERN
#  endif
# endif
/* Bundled C compiler on HP-UX needs this.  Others shouldn't care. */
# if (defined(__hpux))
#  ifndef MODERN
#   define MODERN
#  endif
# endif

/* turn off prototypes if requested */
# if (defined(NOPROTO) && defined(PROTO))
#  undef PROTO
# endif

/* Function prototype control.  (See also globals.h.) */
# ifdef PROTO
#  define OF(a) a
#  define OFT(a) a
# else
#  define OF(a) ()
#  define OFT(a)
# endif

/* enable the "const" keyword only if MODERN and if not otherwise instructed */
# ifdef MODERN
#  if (!defined(ZCONST) && (defined(USE_CONST) || !defined(NO_CONST)))
#   define ZCONST const
#  endif
# endif

# ifndef ZCONST
#  define ZCONST
# endif

/* Tell Microsoft Visual C++ 2005 (and newer) to leave us alone
 * and let us use standard C functions the way we're supposed to.
 * (These preprocessor symbols must appear before the first system
 *  header include. They are located here, because for WINDLL the
 *  first system header includes follow just below.)
 */
# ifdef _MSC_VER
#  if _MSC_VER >= 1400
#   ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#   endif
#   ifndef _CRT_NONSTDC_NO_WARNINGS
#    define _CRT_NONSTDC_NO_WARNINGS
#   endif
#   if defined(POCKET_UNZIP) && !defined(_CRT_NON_CONFORMING_SWPRINTFS)
#    define _CRT_NON_CONFORMING_SWPRINTFS
#   endif
#  endif
   /* Memory allocation debug.  Map malloc() onto malloc_dbg().
    * Look for _CrtSetDbgFlag in unzip.c:MAIN().
    */
#  if defined(_DEBUG) && !defined( NO_IZ_DEBUG_ALLOC)
#   define _CRTDBG_MAP_ALLOC
#   include "crtdbg.h"
#  endif
# endif /* def _MSC_VER */

/* Enable UNIXBACKUP (-B) option as default on specific systems, to
 * allow backing up files that would be overwritten.
 * NO_UNIXBACKUP overrides UNIXBACKUP
 */
# if (!defined( NO_UNIXBACKUP) && !defined( UNIXBACKUP))
#  if defined( UNIX) || defined( OS2) || defined( WIN32)
#   define UNIXBACKUP
#  endif
# endif

/*---------------------------------------------------------------------------
    Grab system-specific public include headers.
  ---------------------------------------------------------------------------*/

# ifdef POCKET_UNZIP             /* WinCE port */
#  include "wince/punzip.h"     /* must appear before windows.h */
# endif

# ifdef WINDLL
   /* for UnZip, the "basic" part of the win32 api is sufficient */
#  ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#   define IZ_HASDEFINED_WIN32LEAN
#  endif
#  include <windows.h>
#  include "windll/structs.h"
#  ifdef IZ_HASDEFINED_WIN32LEAN
#   undef WIN32_LEAN_AND_MEAN
#   undef IZ_HASDEFINED_WIN32LEAN
#  endif
# endif /* def WINDLL */


/*---------------------------------------------------------------------------
    Grab system-dependent definition of EXPENTRY for prototypes below.
  ---------------------------------------------------------------------------*/

#if 0
# if (defined(OS2) && !defined(FUNZIP))
#  ifdef UNZIP_INTERNAL
#   define INCL_NOPM
#   define INCL_DOSNLS
#   define INCL_DOSPROCESS
#   define INCL_DOSDEVICES
#   define INCL_DOSDEVIOCTL
#   define INCL_DOSERRORS
#   define INCL_DOSMISC
#   ifdef OS2DLL
#    define INCL_REXXSAA
#    include <rexxsaa.h>
#   endif
#  endif /* UNZIP_INTERNAL */
#  include <os2.h>
#  define UZ_EXP EXPENTRY
# endif /* OS2 && !FUNZIP */
#endif /* 0 */

# if (defined(OS2) && !defined(FUNZIP))
#  if (defined(__IBMC__) || defined(__WATCOMC__))
#   define UZ_EXP  _System    /* compiler keyword */
#  else
#   define UZ_EXP
#  endif
# endif /* OS2 && !FUNZIP */

# if (defined(WINDLL) || defined(USE_UNZIP_LIB))
#  ifndef EXPENTRY
#   define UZ_EXP WINAPI
#  else
#   define UZ_EXP EXPENTRY
#  endif
# endif

# ifndef UZ_EXP
#  define UZ_EXP
# endif

# ifdef __cplusplus
extern "C" {
# endif

/*---------------------------------------------------------------------------
    Public typedefs.
  ---------------------------------------------------------------------------*/

# ifndef _IZ_TYPES_DEFINED
#  ifdef MODERN
typedef void zvoid;
#  else /* def MODERN */
#   ifndef AOS_VS       /* mostly modern? */
#    ifndef VAXC        /* not fully modern, but has knows 'void' */
#     define void int
#    endif /* !VAXC */
#   endif /* !AOS_VS */
typedef char zvoid;
#  endif /* def MODERN [else] */
typedef unsigned char   uch;    /* code assumes unsigned bytes; these type-  */
typedef unsigned short  ush;    /*  defs replace byte/UWORD/ULONG (which are */
typedef unsigned long   ulg;    /*  predefined on some systems) & match zip  */
#  define _IZ_TYPES_DEFINED
# endif /* !_IZ_TYPES_DEFINED */

/* InputFn is not yet used and is likely to change: */
# ifdef PROTO
typedef int     (UZ_EXP MsgFn)      (zvoid *pG, uch *buf, ulg size, int flag);
typedef int     (UZ_EXP InputFn)    (zvoid *pG, uch *buf, int *size, int flag);
typedef void    (UZ_EXP PauseFn)    (zvoid *pG, ZCONST char *prompt, int flag);
typedef int     (UZ_EXP PasswdFn)   (zvoid *pG, int *rcnt, char *pwbuf,
                                     int size, ZCONST char *zfn,
                                     ZCONST char *efn);
 typedef int    (UZ_EXP StatCBFn)   (zvoid *pG, int fnflag, ZCONST char *zfn,
                                     ZCONST char *efn, ZCONST zvoid *details);
typedef void    (UZ_EXP UsrIniFn)   (void);
#else /* def PROTO */
typedef int     (UZ_EXP MsgFn)      ();
typedef int     (UZ_EXP InputFn)    ();
typedef void    (UZ_EXP PauseFn)    ();
typedef int     (UZ_EXP PasswdFn)   ();
typedef int     (UZ_EXP StatCBFn)   ();
typedef void    (UZ_EXP UsrIniFn)   ();
#endif /* def PROTO [else] */

typedef struct _UzpBuffer {    /* rxstr */
    ulg   strlength;           /* length of string */
    char  *strptr;             /* pointer to string */
} UzpBuffer;

typedef struct _UzpInit {
    ulg structlen;             /* length of the struct being passed */

    /* GRR: can we assume that each of these is a 32-bit pointer?  if not,
     * does it matter? add "far" keyword to make sure? */
    MsgFn *msgfn;
    InputFn *inputfn;
    PauseFn *pausefn;
    UsrIniFn *userfn;          /* user init function to be called after */
                               /*  globals constructed and initialized */

    /* pointer to program's environment area or something? */
    /* hooks for performance testing? */
    /* hooks for extra unzip -v output? (detect CPU or other hardware?) */
    /* anything else?  let me (Greg) know... */
} UzpInit;

typedef struct _UzpCB {
    ulg structlen;             /* length of the struct being passed */
    /* GRR: can we assume that each of these is a 32-bit pointer?  if not,
     * does it matter? add "far" keyword to make sure? */
    MsgFn *msgfn;
    InputFn *inputfn;
    PauseFn *pausefn;
    PasswdFn *passwdfn;
    StatCBFn *statrepfn;
} UzpCB;

/* the collection of general UnZip option flags and option arguments */
typedef struct _UzpOpts {
# ifndef FUNZIP
    int zero_flag;      /* -0: No mapping of FAT/NTFS names (in do_string). */
    int aflag;          /* -a: ASCII-EBCDIC and/or end-of-line translation. */
#  ifdef VMS
    int bflag;          /* -b: Force fixed record format for binary files. */
#  endif /* def VMS */
#  ifdef TANDEM
    int bflag;          /* -b: Create text files in 'C' format (180). */
#  endif /* def TANDEM */
#  ifdef UNIXBACKUP
    int B_flag;         /* -B: Back up existing files, renaming to *~#####. */
#  endif /* def UNIXBACKUP */
    int cflag;          /* -c: Output to stdout. */
    int C_flag;         /* -C: Match filenames case-insensitively. */
    int D_flag;         /* -D: Don't restore directory (-DD: any) timestamps. */
    char *exdir;        /* -d: Extraction destination (root) directory. */
#  ifndef SFX
    int auto_exdir;     /* -da: Automatic extraction destination (root) dir. */
#  endif /* ndef SFX */
#  ifdef MACOS
    int E_flag;         /* -E: Show Mac extra field during restoring. */
#  endif /* def MACOS */
    int fflag;          /* -f: "Freshen" (extract only newer files). */
#  if (defined(RISCOS) || defined(ACORN_FTYPE_NFS))
    int acorn_nfs_ext;  /* -F: RISC OS types & NFS filetype extensions */
#  endif
    int hflag;          /* -h: Header line (ZipInfo). */
#  ifdef MACOS
    int i_flag;         /* -i: Ignore filenames stored in Mac extra field. */
#  endif /* def MACOS */
#  ifdef RISCOS
    int scanimage;      /* -I: Scan image files. */
#  endif /* def RISCOS */
    int jflag;          /* -j: Junk pathnames. */
/*
 * VAX C V3.1-051 loves "\" in #define, but hates it in #if.
 * HP C V7.3-009 dislikes "defined" in macro in #if (%CC-I-EXPANDEDDEFINED).
 * It seems safest to avoid any continuation lines in either.
 */
#  if defined(__ATHEOS__) || defined(__BEOS__) || defined(__HAIKU__)
#  define J_FLAG 1
#  else
#   if defined(MACOS) || (defined( UNIX) && defined( __APPLE__))
#    define J_FLAG 1
#   endif
#  endif
#  ifdef J_FLAG
    int J_flag;         /* -J: Ignore AtheOS/BeOS/MacOS extra field. info. */
#  endif
#  if defined( UNIX) && defined( __APPLE__)
    int Je_flag;        /* -Je: Ignore (all) extended attributes. */
    int Jf_flag;        /* -Jf: Ignore Finder info. */
    int Jq_flag;        /* -Jq: Ignore quarantine ("com.apple.quarantine") */
    int Jr_flag;        /* -Jr: Ignore Resource fork. */
#  endif /* defined( UNIX) && defined( __APPLE__) */
    int java_cafe;      /* --jar: Java CAFE extra block assumed/detected. */
#  if (defined(__ATHEOS__) || defined(__BEOS__) || defined(UNIX))
    int K_flag;         /* -K: Keep setuid/setgid/tacky permissions. */
#  endif
#  if !defined( NO_KFLAG) && !defined( KFLAG)
#   if defined( __ATHEOS__) || defined( __BEOS__) || defined( UNIX)
#    define KFLAG
#   else
#    if defined( VMS)
#     define KFLAG
#    endif
#   endif
#  endif /* !defined( NO_KFLAG) && !defined( KFLAG) */
#  ifdef KFLAG
    int kflag;          /* -k: Owner+group restoration control. */
#  endif
#  ifdef VMS
    int ka_flag;        /* -ka: VMS ACL restoration control. */
#  endif
    int lflag;          /* -12slmv: Listing format (ZipInfo) */
    int L_flag;         /* -L: Convert filenames (some OS's) to lowercase. */
    int member_counts;  /* -mc: Show separate dir/file/link member counts. */
    int overwrite_none; /* -n: Never overwrite files (no prompting). */
#  ifdef AMIGA
    int N_flag;         /* -N: Restore comments as AmigaDOS filenotes. */
#  endif
    int overwrite_all;  /* -o: OK to overwrite files without prompting. */
# endif /* ndef FUNZIP */
    char *pwdarg;       /* -P: Command-line password. */
    int qflag;          /* -q: Quiet.  Produce much less output. */
# ifdef TANDEM
    int rflag;          /* -r: Remove file extensions. */
# endif
# ifndef FUNZIP
    int sflag;          /* -s: Convert spaces in filenames to underscores. */
#  ifdef VMS
    int S_flag;         /* -S: Use Stream_LF for text files (-a[a]). */
#  endif /* def VMS */
#  if (defined(MSDOS) || defined(__human68k__) || defined(OS2) || defined(WIN32))
#   define VOLFLAG
#  else
#   if defined( VMS)
#    define VOLFLAG
#   endif /* def VMS */
#  endif
#  ifdef VOLFLAG
    int volflag;        /* -$: Extract volume labels. */
#  endif /* def VOLFLAG */
    int tflag;          /* -t: Test (UnZip) or totals line (ZipInfo). */
    int T_flag;         /* -T: timestamps (UnZip) or dec. time fmt (ZipInfo). */
    int uflag;          /* -u: "Update": Extract only newer/brand-new files). */
#  if defined(UNIX) || defined(VMS) || defined(WIN32)
    int U_flag;         /* -U: Escape non-ASCII, -UU No Unicode paths. */
#  endif
    int vflag;          /* -v: Verbose/version.  (ZipInfo: Verbose listing.) */
    int V_flag;         /* -V: Retain VMS version numbers. */
    int W_flag;         /* -W: Wildcard '*' won't match '/' dir separator. */
#  if (defined (__ATHEOS__) || defined(__BEOS__) || defined(UNIX))
    int X_flag;         /* -X: Restore owner/protection or UID/GID. */
#  else
#   if (defined(TANDEM) || defined(THEOS))
    int X_flag;         /* -X: Restore owner/protection or UID/GID or ACLs. */
#   else
#    if (defined(OS2) || defined(VMS) || defined(WIN32))
    int X_flag;         /* -X: restore owner/protection or UID/GID. */
#    endif
#   endif
#  endif
#  ifdef VMS
    int Y_flag;         /* -Y: Treat ".nnn" as ";nnn" version. */
#  endif
    int zflag;          /* -z: Display the zipfile comment (only, for UnZip). */
    int zipinfo_mode;   /* -Z: ZipInfo mode (instead of normal UnZip). */
#  ifdef VMS
    int ods2_flag;      /* -2: Force names to conform to ODS2. */
#  endif
#  if (!defined(RISCOS) && !defined(CMS_MVS) && !defined(TANDEM))
    int ddotflag;       /* -:: Don't skip over "../" path elements */
#  endif
#  ifdef UNIX
    int cflxflag;       /* -^: Allow control chars in extracted filenames. */
#  endif
# endif /* ndef FUNZIP */
} UzpOpts;

/* intended to be a private struct: */
typedef struct _ver {
    uch vmajor;             /* e.g., integer 5 */
    uch vminor;             /* e.g., 2 */
    uch patchlevel;         /* e.g., 0 */
    uch not_used;
} _version_type;

typedef struct _UzpVer {
    ulg structlen;            /* length of the struct being passed */
    ulg flag;                 /* bit 0: is_beta   bit 1: uses_zlib */
    ZCONST char *betalevel;   /* e.g. "g BETA" or "" */
    ZCONST char *date;        /* e.g. "9 Oct 08" (beta) or "9 October 2008" */
    ZCONST char *zlib_version;/* e.g. "1.2.3" or NULL */
    _version_type unzip;      /* current UnZip version */
    _version_type zipinfo;    /* current ZipInfo version */
    _version_type os2dll;     /* OS2DLL version (retained for compatibility */
    _version_type windll;     /* WinDLL version (retained for compatibility */
    _version_type dllapimin;  /* last incompatible change of library API */
} UzpVer;

/* for Visual BASIC access to Windows DLLs: */
typedef struct _UzpVer2 {
    ulg structlen;            /* length of the struct being passed */
    ulg flag;                 /* bit 0: is_beta   bit 1: uses_zlib */
    char betalevel[10];       /* e.g. "g BETA" or "" */
    char date[20];            /* e.g. "9 Oct 08" (beta) or "9 October 2008" */
    char zlib_version[10];    /* e.g. "1.2.3" or NULL */
    _version_type unzip;      /* current UnZip version */
    _version_type zipinfo;    /* current ZipInfo version */
    _version_type os2dll;     /* OS2DLL version (retained for compatibility */
    _version_type windll;     /* WinDLL version (retained for compatibility */
    _version_type dllapimin;  /* last incompatible change of library API */
} UzpVer2;


typedef struct _Uzp_Siz64 {
    unsigned long lo32;
    unsigned long hi32;
} Uzp_Siz64;

typedef struct _Uzp_cdir_Rec {
    uch version_made_by[2];
    uch version_needed_to_extract[2];
    ush general_purpose_bit_flag;
    ush compression_method;
    ulg last_mod_dos_datetime;
    ulg crc32;
    Uzp_Siz64 csize;
    Uzp_Siz64 ucsize;
    ush filename_length;
    ush extra_field_length;
    ush file_comment_length;
    ush disk_number_start;
    ush internal_file_attributes;
    ulg external_file_attributes;
    Uzp_Siz64 relative_offset_local_header;
} Uzp_cdir_Rec;


# define UZPINIT_LEN    sizeof(UzpInit)
# define UZPVER_LEN     sizeof(UzpVer)
# define cbList(func)   int (* UZ_EXP func)(char *filename, Uzp_cdir_Rec *crec)


/*---------------------------------------------------------------------------
    Return (and exit) values of the public UnZip API functions.
  ---------------------------------------------------------------------------*/

/* External return codes */
# define PK_OK              0   /* no error */
# define PK_COOL            0   /* no error */
# define PK_WARN            1   /* warning error */
# define PK_ERR             2   /* error in zipfile */
# define PK_BADERR          3   /* severe error in zipfile */
# define PK_MEM             4   /* insufficient memory (during initialization) */
# define PK_MEM2            5   /* insufficient memory (password failure) */
# define PK_MEM3            6   /* insufficient memory (file decompression) */
# define PK_MEM4            7   /* insufficient memory (memory decompression) */
# define PK_MEM5            8   /* insufficient memory (not yet used) */
# define PK_NOZIP           9   /* zipfile not found */
# define PK_PARAM          10   /* bad or illegal parameters specified */
# define PK_FIND           11   /* no files found */
# define PK_COMPERR        19   /* error in compilation options */
# define PK_DISK           50   /* disk full */
# define PK_EOF            51   /* unexpected EOF */

# define IZ_CTRLC          80   /* user hit ^C to terminate */
# define IZ_UNSUP          81   /* no files found: all unsup. compr/encrypt. */
# define IZ_BADPWD         82   /* no files found: all had bad password */
# define IZ_ERRBF          83   /* big-file archive, small-file program */
# define IZ_BADDEST        84   /* Bad automatic destination directory. */

/* return codes of password fetches (negative = user abort; positive = error) */
# define IZ_PW_ENTERED      0   /* got some password string; use/try it */
# define IZ_PW_CANCEL      -1   /* no password available (for this entry) */
# define IZ_PW_CANCELALL   -2   /* no password, skip any further pwd. request */
# define IZ_PW_ERROR        5   /* = PK_MEM2 : failure (no mem, no tty, ...) */

/* flag values for status callback function */
# define UZ_ST_START_EXTRACT    1       /* no details */
# define UZ_ST_IN_PROGRESS      2       /* no details */
# define UZ_ST_FINISH_MEMBER    3       /* 'details': extracted size */

/* return values of status callback function */
# define UZ_ST_CONTINUE         0
# define UZ_ST_BREAK            1


/*---------------------------------------------------------------------------
    Prototypes for public UnZip API (DLL) functions.
  ---------------------------------------------------------------------------*/

# define  UzpMatch match

int      UZ_EXP UzpMain            OF((int argc, char **argv));
int      UZ_EXP UzpAltMain         OF((int argc, char **argv, UzpInit *init));
int      UZ_EXP UzpMainI           OF((int argc, char **argv, UzpCB *init));
ZCONST UzpVer * UZ_EXP UzpVersion  OF((void));
char    *UzpFeatures               OF((void));
void     UZ_EXP UzpFreeMemBuffer   OF((UzpBuffer *retstr));
# ifndef WINDLL
int      UZ_EXP UzpUnzipToMemory   OF((char *zip, char *file, UzpOpts *optflgs,
                                       UzpCB *UsrFunc, UzpBuffer *retstr));
int      UZ_EXP UzpGrep            OF((char *archive, char *file,
                                       char *pattern, int cmd, int SkipBin,
                                       UzpCB *UsrFunc));
# endif /* ndef WINDLL */
# ifdef OS2
int      UZ_EXP UzpFileTree        OF((char *name, cbList(callBack),
                                       char *cpInclude[], char *cpExclude[]));
# endif /* def OS2 */

unsigned UZ_EXP UzpVersion2        OF((UzpVer2 *version));
int      UZ_EXP UzpValidate        OF((char *archive, int AllCodes));

void     show_commandline          OF((char *args[]));


/* Default I/O functions (can be swapped out via UzpAltMain() entry point): */

int      UZ_EXP UzpMessagePrnt   OF((zvoid *pG, uch *buf, ulg size, int flag));
int      UZ_EXP UzpMessageNull   OF((zvoid *pG, uch *buf, ulg size, int flag));
int      UZ_EXP UzpInput         OF((zvoid *pG, uch *buf, int *size, int flag));
void     UZ_EXP UzpMorePause     OF((zvoid *pG, ZCONST char *prompt, int flag));
int      UZ_EXP UzpPassword      OF((zvoid *pG, int *rcnt, char *pwbuf,
                                     int size, ZCONST char *zfn,
                                     ZCONST char *efn));

# ifdef VMS
int      vms_status              OF((int err));                 /* vms.c */
void     decc_init               OF((void));                    /* vms.c */
# endif

/* General purpose flag: UTF-8 member path and comment. */
# ifndef UTF8_BIT               /* Avoid conflict with Zip:zip.h. */
#  define UTF8_BIT (1<<11)
# endif /* ndef UTF8_BIT */

# ifdef __cplusplus
}
# endif


/*---------------------------------------------------------------------------
    Remaining private stuff for UnZip compilation.
  ---------------------------------------------------------------------------*/

# ifdef UNZIP_INTERNAL
#  include "unzpriv.h"
# endif

#endif /* !__unzip_h */

