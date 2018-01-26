/*
  Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  globals.c

  Routines to allocate and initialize globals, with or without threads.

  Contents:  registerGlobalPointer()
             deregisterGlobalPointer()
             getGlobalPointer()
             globalsCtor()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"

#ifndef FUNZIP
/* Initialization of sigs is completed ("PK" inserted) at runtime so
 * that a program executable won't look like a zip archive.
 */
uch central_digsig_sig[ 4]      = { 0, 0, 0x05, 0x05 };
uch central_hdr_sig[ 4]         = { 0, 0, 0x01, 0x02 };
uch end_central_sig[ 4]         = { 0, 0, 0x05, 0x06 };
uch end_central64_sig[ 4]       = { 0, 0, 0x06, 0x06 };
uch end_centloc64_sig[ 4]       = { 0, 0, 0x06, 0x07 };
/* uch extd_local_sig[ 4]          = { 0, 0, 0x07, 0x08 }; */ /* NOT USED YET */
uch local_hdr_sig[ 4]           = { 0, 0, 0x03, 0x04 };
#endif /* ndef FUNZIP */


# ifdef REENTRANT
#  ifdef USETHREADID
#    define THREADID_ENTRIES  0x40

     int lastScan;
     Uz_Globs  *threadPtrTable[THREADID_ENTRIES];
     ulg        threadIdTable [THREADID_ENTRIES] = {
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,    /* Make sure there are */
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,    /* THREADID_ENTRIES 0s */
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
     };

     static ZCONST char Far TooManyThreads[] =
       "error:  more than %d simultaneous threads.\n\
        Some threads are probably not calling DESTROYTHREAD()\n";
     static ZCONST char Far EntryNotFound[] =
       "error:  couldn't find global pointer in table.\n\
        Maybe somebody accidentally called DESTROYTHREAD() twice.\n";
     static ZCONST char Far GlobalPointerMismatch[] =
       "error:  global pointer in table does not match pointer passed as\
 parameter\n";

static void registerGlobalPointer OF((__GPRO));



static void registerGlobalPointer(__G)
    __GDEF
{
    int scan=0;
    ulg tid = GetThreadId();

    while (threadIdTable[scan] && scan < THREADID_ENTRIES)
        scan++;

    if (scan == THREADID_ENTRIES) {
        ZCONST char *tooMany = LoadFarString(TooManyThreads);
        Info(slide, 0x421, ((char *)slide, tooMany, THREADID_ENTRIES));
        izu_free(pG);
        EXIT(PK_MEM);   /* essentially memory error before we've started */
    }

    threadIdTable [scan] = tid;
    threadPtrTable[scan] = pG;
    lastScan = scan;
}



void deregisterGlobalPointer(__G)
    __GDEF
{
    int scan=0;
    ulg tid = GetThreadId();


    while (threadIdTable[scan] != tid && scan < THREADID_ENTRIES)
        scan++;

/*---------------------------------------------------------------------------
    There are two things we can do if we can't find the entry:  ignore it or
    scream.  The most likely reason for it not to be here is the user calling
    this routine twice.  Since this could cause BIG problems if any globals
    are accessed after the first call, we'd better scream.
  ---------------------------------------------------------------------------*/

    if (scan == THREADID_ENTRIES || threadPtrTable[scan] != pG) {
        ZCONST char *noEntry;
        if (scan == THREADID_ENTRIES)
            noEntry = LoadFarString(EntryNotFound);
        else
            noEntry = LoadFarString(GlobalPointerMismatch);
        Info(slide, 0x421, ((char *)slide, noEntry));
        EXIT(PK_WARN);   /* programming error, but after we're all done */
    }

    threadIdTable [scan] = 0;
    lastScan = scan;
    izu_free(threadPtrTable[scan]);
}



Uz_Globs *getGlobalPointer()
{
    int scan=0;
    ulg tid = GetThreadId();

    while (threadIdTable[scan] != tid && scan < THREADID_ENTRIES)
        scan++;

/*---------------------------------------------------------------------------
    There are two things we can do if we can't find the entry:  ignore it or
    scream.  The most likely reason for it not to be here is the user calling
    this routine twice.  Since this could cause BIG problems if any globals
    are accessed after the first call, we'd better scream.
  ---------------------------------------------------------------------------*/

    if (scan == THREADID_ENTRIES) {
        ZCONST char *noEntry = LoadFarString(EntryNotFound);
        fprintf(stderr, noEntry);  /* can't use Info w/o a global pointer */
        EXIT(PK_ERR);   /* programming error while still working */
    }

    return threadPtrTable[scan];
}

#  else /* def USETHREADID */
     Uz_Globs *GG;
#  endif /* def USETHREADID [else] */
# else /* def REENTRANT */
   Uz_Globs G;
#endif /* def REENTRANT [else] */



Uz_Globs *globalsCtor()
{
#ifdef REENTRANT
    Uz_Globs *pG = (Uz_Globs *)izu_malloc(sizeof(Uz_Globs));

    if (!pG)
        return (Uz_Globs *)NULL;
#endif /* REENTRANT */

    /* for REENTRANT version, G is defined as (*pG) */

    memzero(&G, sizeof(Uz_Globs));

    /* Program version string for unzip.c:UzpVersionStr(). */
    *G.prog_vers_str = '\0';

    /* Command-line option processing. */
    G.permute_opts_args = 1;    /* !0: Return "-" options first. */
    G.dashdash_ends_opts = 1;   /* !0: "--" ends options. */

#ifndef FUNZIP
# ifdef CMS_MVS
    uO.aflag=1;
    uO.C_flag=1;
# endif
# ifdef TANDEM
    uO.aflag=1;     /* default to '-a' auto create Text Files as type 101 */
# endif
# if (!defined(NO_TIMESTAMPS))
    uO.D_flag = 1;  /* Default to '-D', no restoration of dir timestamps. */
# endif
# ifdef VMS
    G.echo_orig = -1;   /* Original terminal echo state (-1: unknown). */
# endif

    uO.lflag=(-1);
    G.wildzipfn = "";
    G.pInfo = G.info;
    G.sol = TRUE;          /* at start of line */

    G.message = UzpMessagePrnt;
    G.input = UzpInput;           /* not used by anyone at the moment... */
# if defined(WINDLL) || defined(MACOS) || defined( DLL)
    G.mpause = NULL;              /* has scrollbars:  no need for pausing */
# else
    G.mpause = UzpMorePause;
# endif
# if !defined( DLL)
    G.decr_passwd = UzpPassword;
# endif /* !defined( DLL) */
    G.zipfn_sgmnt = NULL;               /* Archive segment name */
    G.zipfn_sgmnt_size = 0;             /* Archive segment size */
    G.zipfd_sgmnt = ZIPFD_INVALID;      /* Archive segment file descr/pntr */

# ifdef ENABLE_USER_PROGRESS
    /* User-progress messages. */
    G.u_p_not_first = 0;                /* First time flag. */
    G.u_p_nodename[ 0] = '\0';          /* "host::tty". */
# endif /* def ENABLE_USER_PROGRESS */

# ifdef LZMA_SUPPORT
    G.struct_lzma_p = NULL;             /* Storage pointer and flag. */
# endif /* def LZMA_SUPPORT */

# ifdef PPMD_SUPPORT
    G.struct_ppmd_p = NULL;             /* Storage pointer and flag. */
# endif /* def PPMD_SUPPORT */

#endif /* ndef FUNZIP */

    G.query_fp = stdin;         /* Change to terminal if streaming archive. */

#if (!defined(DOS_FLX_H68_NLM_OS2_W32) && !defined(AMIGA) && !defined(RISCOS))
# if (!defined(MACOS) && !defined(ATARI) && !defined(VMS))
    G.echofd = -1;
# endif /* !(MACOS || ATARI || VMS) */
#endif /* !(DOS_FLX_H68_NLM_OS2_W32 || AMIGA || RISCOS) */

#ifdef SYSTEM_SPECIFIC_CTOR
    SYSTEM_SPECIFIC_CTOR(__G);
#endif

#ifdef REENTRANT
# ifdef USETHREADID
    registerGlobalPointer(__G);
# else
    GG = &G;
# endif /* ?USETHREADID */
#endif /* REENTRANT */

    return &G;
}
