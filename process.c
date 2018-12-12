/*
  Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  process.c

  This file contains the top-level routines for processing multiple zipfiles.

  Contains:  file_size()
             open_and_test_input_file()
             process_zip_cmmnt()
             rec_find()
             find_ecrec64()
             find_ecrec()
             extract_archive_seekable()
  (ext)      process_file_hdr()
             extract_archive()
             check_auto_dest_dir()
  (ext)      free_G_buffers()
  (ext)      process_zipfiles()
             get_cdir_ent()
  (ext)      process_cdir_file_hdr()
  (ext)      process_local_file_hdr()
  (ext)      process_cdir_digsig()
  (ext)      getZip64Data()
  (ext)      ef_scan_for_izux()
  (ext)      getRISCOSexfield()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"
#ifdef WINDLL
# ifdef POCKET_UNZIP
#  include "wince/intrface.h"
# else
#  include "windll/windll.h"
# endif
#endif
#if defined(DYNALLOC_CRCTAB) || defined(UNICODE_SUPPORT)
# include "crc32.h"
#endif

#ifdef LZMA_SUPPORT
# include "if_lzma.h"
#endif /* def LZMA_SUPPORT */

#ifdef PPMD_SUPPORT
# include "if_ppmd.h"
#endif /* def PPMD_SUPPORT */


static ZCONST char Far CannotAllocateBuffers[] =
  "error:  cannot allocate unzip buffers\n";

#ifdef SFX

   static ZCONST char Far CannotFindMyself[] =
     "unzipsfx:  cannot find myself! [%s]\n";
# ifdef CHEAP_SFX_AUTORUN
   static ZCONST char Far AutorunPrompt[] =
     "\nAuto-run command: %s\nExecute this command? [y/n] ";
   static ZCONST char Far NotAutoRunning[] =
     "Not executing auto-run command.";
# endif /* def CHEAP_SFX_AUTORUN */

#else /* def SFX */

   /* process_zipfiles() strings */
# ifdef VMS
   static ZCONST char Far DestDirPrompt[] =
    "\nDestination subdirectory name (only -- No \"[\", \"]\", \".DIR\"): ";
# else /* def VMS */
   static ZCONST char Far DestDirPrompt[] =
    "\nDestination subdirectory name: ";
#endif  /* def VMS [else] */
   static ZCONST char Far InputError[] =
    " NULL\n(EOF or read error)\n";
# if (defined(IZ_CHECK_TZ) && defined(USE_EF_UT_TIME))
     static ZCONST char Far WarnInvalidTZ[] =
       "Warning: TZ environment variable not found, cannot use UTC times!!\n";
# endif
# if !(defined(UNIX) || defined(AMIGA))
   static ZCONST char Far CannotFindWildcardMatch[] =
     "%s:  cannot find any matches for wildcard specification \"%s\".\n";
# endif /* !(defined(UNIX) || defined(AMIGA)) */
   static ZCONST char Far AutoDestDirQuery[] =
"[D]ifferent destination, [R]etry, re[U]se existing dir, or [S]kip archive? ";
   static ZCONST char Far BadAutoDestDir[] =
     "  Automatic destination directory exists: %s\n";
   static ZCONST char Far BadDestDir[] =
     "  Destination directory exists: %s\n";
   static ZCONST char Far FilesProcessOK[] =
     "%d archive%s successfully processed.\n";
   static ZCONST char Far ArchiveWarning[] =
     "%d archive%s had warnings but no fatal errors.\n";
   static ZCONST char Far ArchiveFatalError[] =
     "%d archive%s had fatal errors.\n";
   static ZCONST char Far FileHadNoZipfileDir[] =
     "%d file%s had no zipfile directory.\n";
   static ZCONST char Far ZipfileWasDir[] = "1 \"zipfile\" was a directory.\n";
   static ZCONST char Far ManyZipfilesWereDir[] =
     "%d \"zipfiles\" were directories.\n";
   static ZCONST char Far NoZipfileFound[] = "No zipfiles found.\n";

   /* extract_archive_seekable() strings. */
# ifdef ZSUFX2                          /* Three possible archive names. */
   static ZCONST char Far CannotFindCentDirMsg3[] =
     "%s:  cannot find zipfile directory in either \"%s\" or\n\
        %s\"%s%s\", and cannot find/open \"%s\".\n";
   static ZCONST char Far CannotFindZipfile3[] =
     "%s:  cannot find/open \"%s\", \"%s%s\", or \"%s\".\n";
# else /* def ZSUFX2 */
   static ZCONST char Far CannotFindCentDirMsg[] =
     "%s:  cannot find zipfile directory in \"%s\",\n\
        %sand cannot find/open \"%s\", period.\n";
#  ifdef VMS                            /* One possible archive name. */
   static ZCONST char Far CannotFindZipfileV[] =
     "%s:  cannot find/open \"%s\" (%s).\n";
#  else /* def VMS */                   /* Two possible archive names. */
   static ZCONST char Far CannotFindZipfile2[] =
     "%s:  cannot find/open either \"%s\" or \"%s\".\n";
#  endif /* def VMS [else] */
# endif /* def ZSUFX2 [else] */
# ifndef WINDLL
   static ZCONST char Far Unzip[] = "unzip";
# else
   static ZCONST char Far Unzip[] = "UnZip DLL";
# endif
# ifdef DO_SAFECHECK_2GB
   static ZCONST char Far ZipfileTooBig[] =
     "Trying to read large file (> 2 GiB) without large file support\n";
# endif /* DO_SAFECHECK_2GB */
   static ZCONST char Far MaybeExe[] =
     "note:  \"%s\" may be a plain executable, not an archive\n";
   static ZCONST char Far CentDirNotInZipMsg[] = "\
   [%s]:\n\
     Zipfile is disk %lu of a multi-disk archive, and this is not the disk on\n\
     which the central zipfile directory begins (disk %lu).\n";
   static ZCONST char Far EndCentDirBogus[] =
     "warning [%s]:  end-of-central-directory record claims this\n\
  is disk %lu but that the central directory starts on disk %lu; this is a\n\
  contradiction.  Attempting to process anyway.\n";
# ifdef NO_MULTIPART
   static ZCONST char Far NoMultiDiskArcSupport[] =
     "error [%s]:  zipfile is part of multi-disk archive\n\
  (sorry, not yet supported).\n";
   static ZCONST char Far MaybePakBug[] = "warning [%s]:\
  zipfile claims to be 2nd disk of a 2-part archive;\n\
  attempting to process anyway.  If no further errors occur, this archive\n\
  was probably created by PAK v2.51 or earlier.  This bug was reported to\n\
  NoGate in March 1991 and was supposed to have been fixed by mid-1991; as\n\
  of mid-1992 it still hadn't been.  (If further errors do occur, archive\n\
  was probably created by PKZIP 2.04c or later; UnZip does not yet support\n\
  multi-part archives.)\n";
# else
   static ZCONST char Far MaybePakBug[] = "warning [%s]:\
  zipfile claims to be last disk of a multi-part archive;\n\
  attempting to process anyway, assuming all parts have been concatenated\n\
  together in order.  Expect \"errors\" and warnings...true multi-part support\
\n  doesn't exist yet (coming soon).\n";
# endif
   static ZCONST char Far ExtraBytesAtStart[] =
     "warning [%s]:  %s extra byte%s at beginning or within zipfile\n\
  (attempting to process anyway)\n";

# ifdef ARCHIVE_STDIN
   static ZCONST char Far ZipInfoStream[] =
     "%s:  Streaming input archive not supported in ZipInfo mode.\n";
# endif /* def ARCHIVE_STDIN */

#endif /* def SFX [else] */

#if ((!defined(WINDLL) && !defined(SFX)) || !defined(NO_ZIPINFO))
   static ZCONST char Far LogInitline[] = "Archive:  %s\n";
#endif

static ZCONST char Far MissingBytes[] =
  "error [%s]:  missing %s bytes in zipfile\n\
  (attempting to process anyway)\n";
static ZCONST char Far NullCentDirOffset[] =
  "error [%s]:  NULL central directory offset\n\
  (attempting to process anyway)\n";
static ZCONST char Far ZipfileEmpty[] = "warning [%s]:  zipfile is empty\n";
static ZCONST char Far CentDirStartNotFound[] =
  "error [%s]:  start of central directory not found;\n\
  zipfile corrupt.\n%s";
static ZCONST char Far Cent64EndSigSearchErr[] =
  "fatal error: read failure while seeking for End-of-centdir-64 signature.\n\
  This zipfile is corrupt.\n";
static ZCONST char Far Cent64EndSigSearchOff[] =
  "error: End-of-centdir-64 signature not where expected (prepended bytes?)\n\
  (attempting to process anyway)\n";
#ifdef SFX
   static ZCONST char Far CentDirEndSigNotFound[] =
     "  End-of-central-directory signature not found.\n";
#else /* def SFX */
   static ZCONST char Far CentDirTooLong[] =
     "error [%s]:  reported length of central directory is\n\
  %s bytes too long (Atari STZip zipfile?  J.H.Holm ZIPSPLIT 1.1\n\
  zipfile?).  Compensating...\n";
   static ZCONST char Far CentDirEndSigNotFound[] = "\
  End-of-central-directory signature not found.  Either this file is not\n\
  a zipfile, or it constitutes one disk of a multi-part archive.  In the\n\
  latter case the central directory and zipfile comment will be found on\n\
  the last disk(s) of this archive.\n";
#endif /* def SFX [else] */
#ifdef TIMESTAMP
   static ZCONST char Far ZipTimeStampFailed[] =
     "warning:  cannot set time for: %s\n";
   static ZCONST char Far ZipTimeStampSuccess[] =
     "Updated time stamp for: %s\n";
#endif
static ZCONST char Far ZipfileCommTrunc1[] =
  "caution:  zipfile comment truncated\n";
#ifndef NO_ZIPINFO
   static ZCONST char Far NoZipfileComment[] =
     "There is no zipfile comment.\n";
   static ZCONST char Far ZipfileCommentDesc[] =
     "The zipfile comment is %u bytes long and contains the following text:\n";
   static ZCONST char Far ZipfileCommBegin[] =
     "======================== zipfile comment begins\
 ==========================\n";
   static ZCONST char Far ZipfileCommEnd[] =
     "========================= zipfile comment ends\
 ===========================\n";
   static ZCONST char Far ZipfileCommTrunc2[] =
     "  The zipfile comment is truncated.\n";
#endif /* ndef NO_ZIPINFO */
#ifdef UNICODE_SUPPORT
   static ZCONST char Far UnicodeVersionError[] =
     "warning:  Unicode Path version > 1\n";
   static ZCONST char Far UnicodeMismatchError[] =
     "warning:  Unicode Path checksum invalid\n";
#endif



/************************/
/* Function file_size() */
/************************/
/* File size determination which does not mislead for large files in a
 * small-file program.  The file must already be open.
 * Returns the file size.
 */
#ifdef USE_STRM_INPUT
static zoff_t file_size(file)
    FILE *file;
{
    int sts;
    size_t siz;
#else /* def USE_STRM_INPUT */
static zoff_t file_size(fh)
    int fh;
{
    int siz;
#endif /* def USE_STRM_INPUT [else] */
    zoff_t ofs;
    char waste[4];

#ifdef USE_STRM_INPUT
    /* Seek to actual EOF. */
    sts = zfseeko(file, (zoff_t)0, SEEK_END);
    if (sts != 0) {
        /* fseeko() failed.  (Unlikely.) */
        ofs = EOF;
    } else {
        /* Get apparent offset at EOF. */
        ofs = zftello(file);
        if (ofs < 0) {
            /* Offset negative (overflow).  File too big. */
            ofs = EOF;
        } else {
            /* Seek to apparent EOF offset.
               Won't be at actual EOF if offset was truncated.
            */
            sts = zfseeko(file, ofs, SEEK_SET);
            if (sts != 0) {
                /* fseeko() failed.  (Unlikely.) */
                ofs = EOF;
            } else {
                /* Read a byte at apparent EOF.  Should set EOF flag. */
                siz = fread(waste, 1, 1, file);
                if (feof(file) == 0) {
                    /* Not at EOF, but should be.  File too big. */
                    ofs = EOF;
                }
            }
        }
    }
#else /* def USE_STRM_INPUT */
    /* Seek to actual EOF. */
    ofs = zlseek(fh, (zoff_t)0, SEEK_END);
    if (ofs == (zoff_t)(-1)) {
        /* zlseek() failed.  (Unlikely.) */
        ofs = EOF;
    } else if (ofs < 0) {
        /* Offset negative (overflow).  File too big. */
        ofs = EOF;
    } else {
        /* Seek to apparent EOF offset.
           Won't be at actual EOF if offset was truncated.
        */
        ofs = zlseek(fh, ofs, SEEK_SET);
        if (ofs == (zoff_t)(-1)) {
            /* zlseek() failed.  (Unlikely.) */
            ofs = EOF;
        } else {
            /* Read a byte at apparent EOF.  Should set EOF flag. */
            siz = read(fh, waste, 1);
            if (siz != 0) {
                /* Not at EOF, but should be.  File too big. */
                ofs = EOF;
            }
        }
    }
#endif /* def USE_STRM_INPUT [else] */

    return ofs;
} /* file_size(). */


#ifdef SFX
# define C_MAYBE_EXE                    /* Used in open_and_test_input_file() */
# define C_A_MAYBE_EXE                  /* Used in caller(s). */
#else
# define C_MAYBE_EXE , maybe_exe
# define C_A_MAYBE_EXE , &maybe_exe
#endif


/***************************************/
/* Function open_and_test_input_file() */
/***************************************/

static int open_and_test_input_file( __G__ lastchance C_MAYBE_EXE)
  __GDEF
  int *lastchance;
#ifndef SFX
  int *maybe_exe;
#endif
{
  int error = PK_OK;          /* Return PK-type error code. */
  int sts;

#ifdef ARCHIVE_STDIN
  if (strcmp( G.zipfn, "-") == 0)
  {
    G.zipstdin = 1;
    G.zipfn = "<stdin>";
    /* Use controlling terminal (instead of stdin) for interactive queries. */
    if (G.query_fp == stdin)
    {
# ifdef WIN32
      int fd;
      HANDLE conin;

      G.query_fp = NULL;
      /* Get HANDLE for console input. */
      conin = CreateFileA( "CONIN$", GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (conin != INVALID_HANDLE_VALUE)
      {
        /* Get a C file descriptor for the console HANDLE. */
        fd = _open_osfhandle( (intptr_t)conin, _O_RDONLY);      /* Err msg? */
        if (fd > 0)
        {
          /* Get a C file pointer for the C file descriptor. */
          G.query_fp = fdopen( fd, "r");                        /* Err msg? */
        }
      }
      if (G.query_fp == NULL)
        G.query_fp = stdin;             /* Should have an error message. */
# else /* def WIN32 */
      G.query_fp = fopen( ctermid( NULL), "r");                 /* Err msg? */
      if (G.query_fp == NULL)
        G.query_fp = stdin;             /* Should have an error message. */
# endif /* def WIN32 [else]*/
    }
# ifdef USE_STRM_INPUT
    G.zipfd = stdin;
# else /* def USE_STRM_INPUT */
    G.zipfd = STDIN_FILENO;
# endif /* def USE_STRM_INPUT */

    *lastchance = 1;
  }
  else
  {
    if (G.query_fp != stdin)            /* Was using terminal for queries. */
    {
      fclose( G.query_fp);              /* Switch back to stdin. */
      G.query_fp = stdin;
    }
#else /* def ARCHIVE_STDIN */
  {
#endif /* def ARCHIVE_STDIN [else] */
    sts = SSTAT(G.zipfn, &G.statbuf);

    if (sts ||
#ifdef THEOS
     (error = S_ISLIB(G.statbuf.st_mode)) != 0 ||
#endif
     (error = S_ISDIR(G.statbuf.st_mode)) != 0)
    {
#ifndef SFX
      if (*lastchance && (uO.qflag < 3))
      {
# ifdef ZSUFX2
        if (G.no_ecrec)
        {
          Info(slide, 1, ((char *)slide,
           LoadFarString(CannotFindCentDirMsg3),
           LoadFarStringSmall((uO.zipinfo_mode ? Zipnfo : Unzip)),
           G.wildzipfn, uO.zipinfo_mode? "  " : "", G.wildzipfn, ZSUFX,
           G.zipfn));
        }
        else
        {
          Info(slide, 1, ((char *)slide,
           LoadFarString(CannotFindZipfile3),
           LoadFarStringSmall((uO.zipinfo_mode ? Zipnfo : Unzip)),
           G.wildzipfn, G.wildzipfn, ZSUFX, G.zipfn));
        }
# else /* def ZSUFX2 */
        if (G.no_ecrec)
        {
          Info(slide, 0x401, ((char *)slide,
           LoadFarString(CannotFindCentDirMsg),
           LoadFarStringSmall((uO.zipinfo_mode ? Zipnfo : Unzip)),
           G.wildzipfn, uO.zipinfo_mode? "  " : "", G.zipfn));
        }
        else
        {
#  ifdef VMS
          Info(slide, 0x401, ((char *)slide,
           LoadFarString(CannotFindZipfileV),
           LoadFarStringSmall((uO.zipinfo_mode ? Zipnfo : Unzip)),
           G.wildzipfn,
           (*G.zipfn ? G.zipfn : vms_msg_text())));
#  else /* def VMS */
          Info(slide, 0x401, ((char *)slide,
           LoadFarString(CannotFindZipfile2),
           LoadFarStringSmall((uO.zipinfo_mode ? Zipnfo : Unzip)),
           G.wildzipfn, G.zipfn));
#  endif /* def VMS [else] */
        }
# endif /* def ZSUFX2 [else] */
      }
#endif /* ndef SFX */

      return error? IZ_DIR : PK_NOZIP;
    }
    G.ziplen = G.statbuf.st_size;

#ifndef SFX
# if defined(UNIX) || defined(DOS_OS2_W32) || defined(THEOS)
    if (G.statbuf.st_mode & S_IEXEC)    /* No extension on Unix exes.  Might */
        *maybe_exe = TRUE;              /* find unzip, not unzip.zip; etc.  */
# endif
#endif /* ndef SFX */

#ifdef VMS
    if (check_format(__G))              /* Check for variable-length format. */
    {
      return PK_ERR;
    }
#endif /* def VMS */

    if (open_infile(__G__ OIF_PRIMARY))
    { /* This should never fail, given the stat() test above, but... */
      return PK_NOZIP;
    }

#ifdef DO_SAFECHECK_2GB
    /* Need more care: Do not trust the size returned by stat() but
     * determine it by reading beyond the end of the file.
     */
    G.ziplen = file_size(G.zipfd);

    if (G.ziplen == EOF)
    {
      Info(slide, 0x401, ((char *)slide, LoadFarString(ZipfileTooBig)));
      CLOSE_INFILE( &G.zipfd);
      return IZ_ERRBF;
    }
#endif /* DO_SAFECHECK_2GB */
  }

  /* Determine seekability/size. */
  if (error == PK_OK)
  {
    G.ziplen = file_size( G.zipfd);
  }

  return error;
} /* open_and_test_input_file(). */


/********************************/
/* Function process_zip_cmmnt() */
/********************************/

static int process_zip_cmmnt(__G)       /* return PK-type error code */
    __GDEF
{
    int error = PK_COOL;

/*---------------------------------------------------------------------------
    Get the zipfile comment (up to 64KB long), if any, and print it out.
  ---------------------------------------------------------------------------*/

#ifdef WINDLL
    /* for comment button: */
    if ((!G.fValidate) && (G.lpUserFunctions != NULL))
       G.lpUserFunctions->cchComment = G.ecrec.zipfile_comment_length;
#endif /* def WINDLL */

#ifndef NO_ZIPINFO
    /* ZipInfo, verbose format */
    if (uO.zipinfo_mode && uO.lflag > 9) {
        /*-------------------------------------------------------------------
            Get the zipfile comment, if any, and print it out.
            (Comment may be up to 64KB long.  May the fleas of a thousand
            camels infest the arm-pits of anyone who actually takes advantage
            of this fact.)
          -------------------------------------------------------------------*/

        if (!G.ecrec.zipfile_comment_length)
            Info(slide, 0, ((char *)slide, LoadFarString(NoZipfileComment)));
        else {
            Info(slide, 0, ((char *)slide, LoadFarString(ZipfileCommentDesc),
              G.ecrec.zipfile_comment_length));
            Info(slide, 0, ((char *)slide, LoadFarString(ZipfileCommBegin)));
            if (do_string(__G__ G.ecrec.zipfile_comment_length, DISPLAY))
                error = PK_WARN;
            Info(slide, 0, ((char *)slide, LoadFarString(ZipfileCommEnd)));
            if (error)
                Info(slide, 0, ((char *)slide,
                  LoadFarString(ZipfileCommTrunc2)));
        } /* endif (comment exists) */

    /* ZipInfo, non-verbose mode:  print zipfile comment only if requested */
    } else if (G.ecrec.zipfile_comment_length &&
               (uO.zflag > 0) && uO.zipinfo_mode) {
        if (do_string(__G__ G.ecrec.zipfile_comment_length, DISPLAY)) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ZipfileCommTrunc1)));
            error = PK_WARN;
        }
    } else
#endif /* ndef NO_ZIPINFO */
    if ( G.ecrec.zipfile_comment_length &&
         (uO.zflag > 0
#ifndef WINDLL
          || (uO.zflag == 0
# ifndef NO_ZIPINFO
              && !uO.zipinfo_mode
# endif
# ifdef TIMESTAMP
              && !uO.T_flag
# endif
              && !uO.qflag)
#endif /* ndef WINDLL */
         ) )
    {
        if (do_string(__G__ G.ecrec.zipfile_comment_length,
#if defined(SFX) && defined(CHEAP_SFX_AUTORUN)
# ifdef NO_ZIPINFO
                      CHECK_AUTORUN
# else /* def NO_ZIPINFO */
                      (oU.zipinfo_mode ? DISPLAY : CHECK_AUTORUN)
# endif /* def NO_ZIPINFO [else] */
#else /* defined(SFX) && defined(CHEAP_SFX_AUTORUN) */
                      DISPLAY
#endif /* defined(SFX) && defined(CHEAP_SFX_AUTORUN) [else] */
                     ))
        {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ZipfileCommTrunc1)));
            error = PK_WARN;
        }
    }
#if defined(SFX) && defined(CHEAP_SFX_AUTORUN)
    else if (G.ecrec.zipfile_comment_length) {
        if (do_string(__G__ G.ecrec.zipfile_comment_length, CHECK_AUTORUN_Q))
        {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ZipfileCommTrunc1)));
            error = PK_WARN;
        }
    }
#endif /* defined(SFX) && defined(CHEAP_SFX_AUTORUN) */
    return error;

} /* process_zip_cmmnt(). */


/***********************/
/* Function rec_find() */
/***********************/

static int rec_find(__G__ searchlen, signature, rec_size)
    /* return 0 when rec found, 1 when not found, 2 in case of read error */
    __GDEF
    zoff_t searchlen;
    uch* signature;
    int rec_size;
{
    int i, numblks, found=FALSE;
    zoff_t tail_len;

/*---------------------------------------------------------------------------
    Zipfile is longer than INBUFSIZ:  may need to loop.  Start with short
    block at end of zipfile (if not TOO short).
  ---------------------------------------------------------------------------*/

    if ((tail_len = G.ziplen % INBUFSIZ) > rec_size) {
#ifdef USE_STRM_INPUT
        zfseeko(G.zipfd, G.ziplen-tail_len, SEEK_SET);
        G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
        G.cur_zipfile_bufstart = zlseek(G.zipfd, G.ziplen-tail_len, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */
        if ((G.incnt = read(G.zipfd, (char *)G.inbuf,
            (unsigned int)tail_len)) != (int)tail_len)
            return 2;      /* it's expedient... */

        /* 'P' must be at least (rec_size+4) bytes from end of zipfile */
        for (G.inptr = G.inbuf+(int)tail_len-(rec_size+4);
             G.inptr >= G.inbuf;
             --G.inptr) {
            if ( (*G.inptr == (uch)0x50) &&         /* ASCII 'P' */
                 !memcmp((char *)G.inptr, signature, 4) ) {
                G.incnt -= (int)(G.inptr - G.inbuf);
                found = TRUE;
                break;
            }
        }
        /* sig may span block boundary: */
        memcpy((char *)G.hold, (char *)G.inbuf, 3);
    } else
        G.cur_zipfile_bufstart = G.ziplen - tail_len;

/*-----------------------------------------------------------------------
    Loop through blocks of zipfile data, starting at the end and going
    toward the beginning.  In general, need not check whole zipfile for
    signature, but may want to do so if testing.
  -----------------------------------------------------------------------*/

    numblks = (int)((searchlen - tail_len + (INBUFSIZ-1)) / INBUFSIZ);
    /*               ==amount=   ==done==   ==rounding==    =blksiz=  */

    for (i = 1;  !found && (i <= numblks);  ++i) {
        G.cur_zipfile_bufstart -= INBUFSIZ;
#ifdef USE_STRM_INPUT
        zfseeko(G.zipfd, G.cur_zipfile_bufstart, SEEK_SET);
#else /* def USE_STRM_INPUT */
        zlseek(G.zipfd, G.cur_zipfile_bufstart, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */
        if ((G.incnt = read(G.zipfd,(char *)G.inbuf,INBUFSIZ))
            != INBUFSIZ)
            return 2;          /* read error is fatal failure */

        for (G.inptr = G.inbuf+INBUFSIZ-1;  G.inptr >= G.inbuf; --G.inptr)
            if ( (*G.inptr == (uch)0x50) &&         /* ASCII 'P' */
                 !memcmp(G.inptr, signature, 4) ) {
                G.incnt -= (int)(G.inptr - G.inbuf);
                found = TRUE;
                break;
            }
        /* sig may span block boundary: */
        memcpy((char *)G.hold, (char *)G.inbuf, 3);
    }

    return (found ? 0 : 1);
} /* rec_find(). */


/***************************/
/* Function find_ecrec64() */
/***************************/

static int find_ecrec64(__G__ searchlen)         /* return PK-class error */
    __GDEF
    zoff_t searchlen;
{
    ec_byte_rec64 byterec;          /* buf for ecrec64 */
    ec_byte_loc64 byterecL;         /* buf for ecrec64 locator */
    zoff_t ecloc64_start_offset;    /* start offset of ecrec64 locator */
    zusz_t ecrec64_start_offset;    /* start offset of ecrec64 */
    zuvl_t ecrec64_start_disk;      /* start disk of ecrec64 */
    zuvl_t ecloc64_total_disks;     /* total disks */
    zuvl_t ecrec64_disk_cdstart;    /* disk number of central dir start */
    zucn_t ecrec64_this_entries;    /* entries on disk with ecrec64 */
    zucn_t ecrec64_tot_entries;     /* total number of entries */
    zusz_t ecrec64_cdirsize;        /* length of central dir */
    zusz_t ecrec64_offs_cdstart;    /* offset of central dir start */

    /* First, find the ecrec64 locator.  By definition, this must be before
       ecrec with nothing in between.  We back up the size of the ecrec64
       locator and check.  */

    ecloc64_start_offset = G.real_ecrec_offset - (ECLOC64_SIZE+4);
    if (ecloc64_start_offset < 0)
      /* Seeking would go past beginning, so probably empty archive */
      return PK_COOL;

#ifdef USE_STRM_INPUT
    zfseeko(G.zipfd, ecloc64_start_offset, SEEK_SET);
    G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
    G.cur_zipfile_bufstart = zlseek(G.zipfd, ecloc64_start_offset, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */

    if ((G.incnt = read(G.zipfd, (char *)byterecL, ECLOC64_SIZE+4))
        != (ECLOC64_SIZE+4)) {
      if (uO.qflag || uO.zipinfo_mode)
          Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
      Info(slide, 0x401, ((char *)slide,
        LoadFarString(Cent64EndSigSearchErr)));
      return PK_ERR;
    }

    if (memcmp((char *)byterecL, end_centloc64_sig, 4) ) {
      /* not found */
      return PK_COOL;
    }

    /* Read the locator. */
    ecrec64_start_disk = (zuvl_t)makelong(&byterecL[NUM_DISK_START_EOCDR64]);
    ecrec64_start_offset = (zusz_t)makeint64(&byterecL[OFFSET_START_EOCDR64]);
    ecloc64_total_disks = (zuvl_t)makelong(&byterecL[NUM_THIS_DISK_LOC64]);

    /* Check for consistency */
#ifdef TEST
    fprintf(stdout, "\nnumber of disks (ECR) %u, (ECLOC64) %u\n",
            G.ecrec.number_this_disk, ecloc64_total_disks); fflush(stdout);
#endif
    if ((G.ecrec.number_this_disk != 0xFFFF) &&
        (G.ecrec.number_this_disk != ecloc64_total_disks - 1) &&
        (ecloc64_total_disks != 0)) {
      /* Note: For some unknown reason, the developers at PKWARE decided to
         store the "zip64 total disks" value as a counter starting from 1,
         whereas all other "split/span volume" related fields use 0-based
         volume numbers. Sigh... */
      /* When the total number of disks as found in the traditional ecrec
         is not 0xFFFF, the disk numbers in ecrec and ecloc64 must match.
         When this is not the case, the found ecrec64 locator cannot be valid.
         -> This is not a Zip64 archive.
       */
      /* Actually the central directory can span multiple disks, and the above
         total - 1 check would fail in this case.  Should fix this.

         There are archive creators that put 0 in total disks when it should
         be 1.  We should handle this.  This is done by the added check above.
      */
      Trace((stderr,
       "\ninvalid ECLOC64, differing disk# (ECR %u, ECL64 %lu)\n",
       G.ecrec.number_this_disk, (unsigned long)(ecloc64_total_disks - 1)));
      return PK_COOL;
    }

    /* If found locator, look for ecrec64 where the locator says it is. */

    /* For now assume that ecrec64 is on the same disk as ecloc64 and ecrec,
       which is usually the case and is how Zip writes it.  To do this right,
       however, we should allow the ecrec64 to be on another disk since
       the AppNote allows it and the ecrec64 can be large, especially if
       Version 2 is used (AppNote uses 8 bytes for the size of this record). */

    /* FIX BELOW IF ADD SUPPORT FOR MULTIPLE DISKS */

    if (ecrec64_start_offset > (zusz_t)ecloc64_start_offset) {
      /* ecrec64 has to be before ecrec64 locator */
      if (uO.qflag || uO.zipinfo_mode)
          Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
      Info(slide, 0x401, ((char *)slide,
        LoadFarString(Cent64EndSigSearchErr)));
      return PK_ERR;
    }

#ifdef USE_STRM_INPUT
    zfseeko(G.zipfd, ecrec64_start_offset, SEEK_SET);
    G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
    G.cur_zipfile_bufstart = zlseek(G.zipfd, ecrec64_start_offset, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */

    if ((G.incnt = read(G.zipfd, (char *)byterec, ECREC64_SIZE+4))
        != (ECREC64_SIZE+4)) {
      if (uO.qflag || uO.zipinfo_mode)
          Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
      Info(slide, 0x401, ((char *)slide,
        LoadFarString(Cent64EndSigSearchErr)));
      return PK_ERR;
    }

    if (memcmp((char *)byterec, end_central64_sig, 4) ) {
      /* Zip64 EOCD Record not found */
      /* Since we already have seen the Zip64 EOCD Locator, it's
         possible we got here because there are bytes prepended
         to the archive, like the sfx prefix. */

      /* Make a guess as to where the Zip64 EOCD Record might be */
      ecrec64_start_offset = ecloc64_start_offset - ECREC64_SIZE - 4;

#ifdef USE_STRM_INPUT
      zfseeko(G.zipfd, ecrec64_start_offset, SEEK_SET);
      G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
      G.cur_zipfile_bufstart = zlseek(G.zipfd, ecrec64_start_offset, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */

      if ((G.incnt = read(G.zipfd, (char *)byterec, ECREC64_SIZE+4))
          != (ECREC64_SIZE+4)) {
        if (uO.qflag || uO.zipinfo_mode)
            Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
        Info(slide, 0x401, ((char *)slide,
          LoadFarString(Cent64EndSigSearchErr)));
        return PK_ERR;
      }

      if (memcmp((char *)byterec, end_central64_sig, 4) ) {
        /* Zip64 EOCD Record not found */
        /* Probably something not so easy to handle so exit */
        if (uO.qflag || uO.zipinfo_mode)
            Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
        Info(slide, 0x401, ((char *)slide,
          LoadFarString(Cent64EndSigSearchErr)));
        return PK_ERR;
      }

      if (uO.qflag || uO.zipinfo_mode)
          Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
      Info(slide, 0x401, ((char *)slide,
        LoadFarString(Cent64EndSigSearchOff)));
    }

    /* Check consistency of found ecrec64 with ecloc64 (and ecrec): */
    if ( (zuvl_t)makelong(&byterec[NUMBER_THIS_DSK_REC64])
         != ecrec64_start_disk )
        /* found ecrec64 does not match ecloc64 info -> no Zip64 archive */
        return PK_COOL;
    /* Read all relevant ecrec64 fields and compare them to the corresponding
       ecrec fields unless those are set to "all-ones".
     */
    ecrec64_disk_cdstart =
      (zuvl_t)makelong(&byterec[NUM_DISK_START_CEN_DIR64]);
    if ( (G.ecrec.num_disk_start_cdir != 0xFFFF) &&
         (G.ecrec.num_disk_start_cdir != ecrec64_disk_cdstart) )
        return PK_COOL;
    ecrec64_this_entries
      = makeint64(&byterec[NUM_ENTRIES_CEN_DIR_THS_DISK64]);
    if ( (G.ecrec.num_entries_centrl_dir_ths_disk != 0xFFFF) &&
         (G.ecrec.num_entries_centrl_dir_ths_disk != ecrec64_this_entries) )
        return PK_COOL;
    ecrec64_tot_entries
      = makeint64(&byterec[TOTAL_ENTRIES_CENTRAL_DIR64]);
    if ( (G.ecrec.total_entries_central_dir != 0xFFFF) &&
         (G.ecrec.total_entries_central_dir != ecrec64_tot_entries) )
        return PK_COOL;
    ecrec64_cdirsize
      = makeint64(&byterec[SIZE_CENTRAL_DIRECTORY64]);
    if ( (G.ecrec.size_central_directory != 0xFFFFFFFFL) &&
         (G.ecrec.size_central_directory != ecrec64_cdirsize) )
        return PK_COOL;
    ecrec64_offs_cdstart
      = makeint64(&byterec[OFFSET_START_CENTRAL_DIRECT64]);
    if ( (G.ecrec.offset_start_central_directory != 0xFFFFFFFFL) &&
         (G.ecrec.offset_start_central_directory != ecrec64_offs_cdstart) )
        return PK_COOL;

    /* Now, we are (almost) sure that we have a Zip64 archive. */
    G.ecrec.have_ecr64 = 1;

    /* Update the "end-of-central-dir offset" for later checks. */
    G.real_ecrec_offset = ecrec64_start_offset;

    /* Update all ecdir_rec data that are flagged to be invalid
       in Zip64 mode.  Set the ecrec64-mandatory flag when such a
       case is found. */
    if (G.ecrec.number_this_disk == 0xFFFF) {
      G.ecrec.number_this_disk = ecrec64_start_disk;
      if (ecrec64_start_disk != 0xFFFF) G.ecrec.is_zip64_archive = TRUE;
    }
    if (G.ecrec.num_disk_start_cdir == 0xFFFF) {
      G.ecrec.num_disk_start_cdir = ecrec64_disk_cdstart;
      if (ecrec64_disk_cdstart != 0xFFFF) G.ecrec.is_zip64_archive = TRUE;
    }
    if (G.ecrec.num_entries_centrl_dir_ths_disk == 0xFFFF) {
      G.ecrec.num_entries_centrl_dir_ths_disk = ecrec64_this_entries;
      if (ecrec64_this_entries != 0xFFFF) G.ecrec.is_zip64_archive = TRUE;
    }
    if (G.ecrec.total_entries_central_dir == 0xFFFF) {
      G.ecrec.total_entries_central_dir = ecrec64_tot_entries;
      if (ecrec64_tot_entries != 0xFFFF) G.ecrec.is_zip64_archive = TRUE;
    }
    if (G.ecrec.size_central_directory == 0xFFFFFFFFL) {
      G.ecrec.size_central_directory = ecrec64_cdirsize;
      if (ecrec64_cdirsize != 0xFFFFFFFF) G.ecrec.is_zip64_archive = TRUE;
    }
    if (G.ecrec.offset_start_central_directory == 0xFFFFFFFFL) {
      G.ecrec.offset_start_central_directory = ecrec64_offs_cdstart;
      if (ecrec64_offs_cdstart != 0xFFFFFFFF) G.ecrec.is_zip64_archive = TRUE;
    }

    return PK_COOL;
} /* find_ecrec64(). */


/*************************/
/* Function find_ecrec() */
/*************************/

static int find_ecrec(__G__ searchlen)          /* return PK-class error */
    __GDEF
    zoff_t searchlen;
{
    int found = FALSE;
    int error_in_archive;
    int result;
    ec_byte_rec byterec;

/*---------------------------------------------------------------------------
    Treat case of short zipfile separately.
  ---------------------------------------------------------------------------*/

    if (G.ziplen <= INBUFSIZ) {
#ifdef USE_STRM_INPUT
        zfseeko(G.zipfd, (zoff_t)0, SEEK_SET);
#else /* def USE_STRM_INPUT */
        zlseek(G.zipfd, (zoff_t)0, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */
        if ((G.incnt = read(G.zipfd,(char *)G.inbuf,(unsigned int)G.ziplen))
            == (int)G.ziplen)

            /* 'P' must be at least (ECREC_SIZE+4) bytes from end of zipfile */
            for (G.inptr = G.inbuf+(int)G.ziplen-(ECREC_SIZE+4);
                 G.inptr >= G.inbuf;
                 --G.inptr) {
                if ( (*G.inptr == (uch)0x50) &&         /* ASCII 'P' */
                     !memcmp((char *)G.inptr, end_central_sig, 4)) {
                    G.incnt -= (int)(G.inptr - G.inbuf);
                    found = TRUE;
                    break;
                }
            }

/*---------------------------------------------------------------------------
    Zipfile is longer than INBUFSIZ:

    MB - this next block of code moved to rec_find so that same code can be
    used to look for zip64 ec record.  No need to include code above since
    a zip64 ec record will only be looked for if it is a BIG file.
  ---------------------------------------------------------------------------*/

    } else {
        found =
          (rec_find(__G__ searchlen, end_central_sig, ECREC_SIZE) == 0
           ? TRUE : FALSE);
    } /* end if (ziplen > INBUFSIZ) */

/*---------------------------------------------------------------------------
    Searched through whole region where signature should be without finding
    it.  Print informational message and die a horrible death.
  ---------------------------------------------------------------------------*/

    if (!found) {
        if (uO.qflag || uO.zipinfo_mode)
            Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
        Info(slide, 0x401, ((char *)slide,
          LoadFarString(CentDirEndSigNotFound)));
        return PK_ERR;   /* failed */
    }

/*---------------------------------------------------------------------------
    Found the signature, so get the end-central data before returning.  Do
    any necessary machine-type conversions (byte ordering, structure padding
    compensation) by reading data into character array and copying to struct.
  ---------------------------------------------------------------------------*/

    G.real_ecrec_offset = G.cur_zipfile_bufstart + (G.inptr-G.inbuf);
#ifdef TEST
    printf("\n  found end-of-central-dir signature at offset %s (%sh)\n",
      FmZofft(G.real_ecrec_offset, NULL, NULL),
      FmZofft(G.real_ecrec_offset, FZOFFT_HEX_DOT_WID, "X"));
    printf("    from beginning of file; offset %d (%.4Xh) within block\n",
      G.inptr-G.inbuf, G.inptr-G.inbuf);
#endif

    if (readbuf(__G__ byterec, ECREC_SIZE+4) == 0)
        return PK_EOF;

    G.ecrec.number_this_disk =
      makeword(&byterec[NUMBER_THIS_DISK]);
    G.ecrec.num_disk_start_cdir =
      makeword(&byterec[NUM_DISK_WITH_START_CEN_DIR]);
    G.ecrec.num_entries_centrl_dir_ths_disk =
      makeword(&byterec[NUM_ENTRIES_CEN_DIR_THS_DISK]);
    G.ecrec.total_entries_central_dir =
      makeword(&byterec[TOTAL_ENTRIES_CENTRAL_DIR]);
    G.ecrec.size_central_directory =
      makelong(&byterec[SIZE_CENTRAL_DIRECTORY]);
    G.ecrec.offset_start_central_directory =
      makelong(&byterec[OFFSET_START_CENTRAL_DIRECTORY]);
    G.ecrec.zipfile_comment_length =
      makeword(&byterec[ZIPFILE_COMMENT_LENGTH]);

    /* Now, we have to read the archive comment, BEFORE the file pointer
       is moved away backwards to seek for a Zip64 ECLOC64 structure.
     */
    if ( (error_in_archive = process_zip_cmmnt(__G)) > PK_WARN )
        return error_in_archive;

    /* Next: Check for existence of Zip64 end-of-cent-dir locator
       ECLOC64. This structure must reside on the same volume as the
       classic ECREC, at exactly (ECLOC64_SIZE+4) bytes in front
       of the ECREC.
       The ECLOC64 structure directs to the longer ECREC64 structure
       A ECREC64 will ALWAYS exist for a proper Zip64 archive, as
       the "Version Needed To Extract" field is required to be set
       to 4.5 or higher whenever any Zip64 features are used anywhere
       in the archive, so just check for that to see if this is a
       Zip64 archive.
     */
    result = find_ecrec64(__G__ searchlen+76);
        /* 76 bytes for zip64ec & zip64 locator */
    if (result != PK_COOL) {
        if (error_in_archive < result)
            error_in_archive = result;
        return error_in_archive;
    }

    G.expect_ecrec_offset = G.ecrec.offset_start_central_directory +
                            G.ecrec.size_central_directory;

#ifndef NO_ZIPINFO
    if (uO.zipinfo_mode) {
        /* In ZipInfo mode, additional info about the data found in the
           end-of-central-directory areas is printed out.
         */
        zi_end_central(__G);
    }
#endif /* ndef NO_ZIPINFO */

    return error_in_archive;

} /* find_ecrec(). */


/***************************************/
/* Function extract_archive_seekable() */
/***************************************/

/* Return PK-type error code. */
static int extract_archive_seekable( __G__ lastchance)
    __GDEF
    int lastchance;
{
#ifndef SFX
    int maybe_exe = FALSE;
    int too_weird_to_continue = FALSE;
# ifdef TIMESTAMP
    time_t uxstamp;
    ulg nmember = 0L;
# endif
#endif
    int error = 0;
    int error_in_archive;

/*---------------------------------------------------------------------------
    Find and process the end-of-central-directory header.  UnZip need only
    check last 65557 bytes of zipfile:  comment may be up to 65535, end-of-
    central-directory record is 18 bytes, and signature itself is 4 bytes;
    add some to allow for appended garbage.  Since ZipInfo is often used as
    a debugging tool, search the whole zipfile if zipinfo_mode is true.
  ---------------------------------------------------------------------------*/

    G.cur_zipfile_bufstart = 0;
    G.inptr = G.inbuf;

#if ((!defined(WINDLL) && !defined(SFX)) || !defined(NO_ZIPINFO))
# if !defined(WINDLL) && !defined(SFX)
    if ( (!uO.zipinfo_mode && !uO.qflag
#  ifdef TIMESTAMP
          && !uO.T_flag
#  endif
         )
#  ifndef NO_ZIPINFO
         || (uO.zipinfo_mode && uO.hflag)
#  endif
       )
# else /* !defined(WINDLL) && !defined(SFX) */
    if (uO.zipinfo_mode && uO.hflag)
# endif /* !defined(WINDLL) && !defined(SFX) [else] */
# ifdef WIN32   /* Win32 console may require codepage conversion for G.zipfn */
        Info(slide, 0, ((char *)slide, LoadFarString(LogInitline),
          FnFilter1(G.zipfn)));
# else
        Info(slide, 0, ((char *)slide, LoadFarString(LogInitline), G.zipfn));
# endif
#endif /* ((!defined(WINDLL) && !defined(SFX)) || !defined(NO_ZIPINFO)) */

    if ( (error_in_archive = find_ecrec(__G__
#ifndef NO_ZIPINFO
                                        uO.zipinfo_mode ? G.ziplen :
#endif
                                        IZ_MIN(G.ziplen, 66000L)))
         > PK_WARN )
    {
        CLOSE_INFILE( &G.zipfd);

#ifdef SFX
        ++lastchance;   /* avoid picky compiler warnings */
        return error_in_archive;
#else /* def SFX */
        if (maybe_exe)
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybeExe),
            G.zipfn));
        if (lastchance)
            return error_in_archive;
        else {
            G.no_ecrec = TRUE;    /* assume we found wrong file:  e.g., */
            return PK_NOZIP;       /*  unzip instead of unzip.zip */
        }
#endif /* def SFX [else] */
    }

    if ((uO.zflag > 0) && !uO.zipinfo_mode) { /* unzip: zflag = comment ONLY */
        CLOSE_INFILE( &G.zipfd);
        return error_in_archive;
    }

/*---------------------------------------------------------------------------
    Test the end-of-central-directory info for incompatibilities (multi-disk
    archives) or inconsistencies (missing or extra bytes in zipfile).
  ---------------------------------------------------------------------------*/

    /* Apparently a few archives out there use 1 as the disk number in a
       one disk archive, incorrectly.  Need to change this to allow for that
       case.  */

#ifdef NO_MULTIPART
    error = !uO.zipinfo_mode && (G.ecrec.number_this_disk == 1) &&
            (G.ecrec.num_disk_start_cdir == 1);
#endif

#ifndef SFX
    if (uO.zipinfo_mode &&
        G.ecrec.number_this_disk != G.ecrec.num_disk_start_cdir)
    {
        if (G.ecrec.number_this_disk > G.ecrec.num_disk_start_cdir) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(CentDirNotInZipMsg), G.zipfn,
              (ulg)G.ecrec.number_this_disk,
              (ulg)G.ecrec.num_disk_start_cdir));
            error_in_archive = PK_FIND;
            too_weird_to_continue = TRUE;
        } else {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(EndCentDirBogus), G.zipfn,
              (ulg)G.ecrec.number_this_disk,
              (ulg)G.ecrec.num_disk_start_cdir));
            error_in_archive = PK_WARN;
        }
# ifdef NO_MULTIPART  /* concatenation of multiple parts works in some cases */
    } else if (!uO.zipinfo_mode && !error && G.ecrec.number_this_disk != 0) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(NoMultiDiskArcSupport),
          G.zipfn));
        error_in_archive = PK_FIND;
        too_weird_to_continue = TRUE;
# endif
    }

    if (!too_weird_to_continue) {  /* (relatively) normal zipfile:  go for it */
        if (error) {
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybePakBug),
              G.zipfn));
            error_in_archive = PK_WARN;
        }
#endif /* ndef SFX */
        if (((G.extra_bytes = G.real_ecrec_offset-G.expect_ecrec_offset) <
         (zoff_t)0) && (G.ecrec.number_this_disk == 0))
        {
            Info(slide, 0x401, ((char *)slide, LoadFarString(MissingBytes),
              G.zipfn, FmZofft((-G.extra_bytes), NULL, NULL)));
            error_in_archive = PK_ERR;
        } else if (G.extra_bytes > 0) {
            if ((G.ecrec.offset_start_central_directory == 0) &&
                (G.ecrec.size_central_directory != 0))   /* zip 1.5 -go bug */
            {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(NullCentDirOffset), G.zipfn));
                G.ecrec.offset_start_central_directory = G.extra_bytes;
                G.extra_bytes = 0;
                error_in_archive = PK_ERR;
            }
#ifndef SFX
            else {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExtraBytesAtStart), G.zipfn,
                  FmZofft(G.extra_bytes, NULL, NULL),
                  (G.extra_bytes == 1)? "":"s"));
                error_in_archive = PK_WARN;
            }
#endif /* ndef SFX */
        }

    /*-----------------------------------------------------------------------
        Check for empty zipfile and exit now if so.
      -----------------------------------------------------------------------*/

        if (G.expect_ecrec_offset==0L && G.ecrec.size_central_directory==0) {
            if (uO.zipinfo_mode)
                Info(slide, 0, ((char *)slide, "%sEmpty zipfile.\n",
                  uO.lflag>9? "\n  " : ""));
            else
                Info(slide, 0x401, ((char *)slide, LoadFarString(ZipfileEmpty),
                                    G.zipfn));
            CLOSE_INFILE( &G.zipfd);
            return (error_in_archive > PK_WARN)? error_in_archive : PK_WARN;
        }

        /* Set the segment number (needed for segmented archives). */
        G.sgmnt_nr = G.ecrec.number_this_disk;

    /*-----------------------------------------------------------------------
        Compensate for missing or extra bytes, and seek to where the start
        of central directory should be.  If header not found, uncompensate
        and try again (necessary for at least some Atari archives created
        with STZip, as well as archives created by J.H. Holm's ZIPSPLIT 1.1).
      -----------------------------------------------------------------------*/

        error = seek_zipf(__G__ G.ecrec.offset_start_central_directory);
        if (error == PK_BADERR) {
            CLOSE_INFILE( &G.zipfd);
            return PK_BADERR;
        }
#ifdef OLD_SEEK_TEST
        if (error != PK_OK || readbuf(__G__ G.sig, 4) == 0) {
            CLOSE_INFILE( &G.zipfd);
            return PK_ERR;  /* file may be locked, or possibly disk error(?) */
        }
        if (memcmp(G.sig, central_hdr_sig, 4))
#else /* def OLD_SEEK_TEST */
        if ((error != PK_OK) || (readbuf(__G__ G.sig, 4) == 0) ||
            memcmp(G.sig, central_hdr_sig, 4))
#endif /* def OLD_SEEK_TEST [else] */
        {
#ifndef SFX
            zoff_t tmp = G.extra_bytes;
#endif /* ndef SFX */

            G.extra_bytes = 0;
            error = seek_zipf(__G__ G.ecrec.offset_start_central_directory);
            if ((error != PK_OK) || (readbuf(__G__ G.sig, 4) == 0) ||
                memcmp(G.sig, central_hdr_sig, 4))
            {
                if (error != PK_BADERR)
                  Info(slide, 0x401, ((char *)slide,
                    LoadFarString(CentDirStartNotFound), G.zipfn,
                    LoadFarStringSmall(ReportMsg)));
                CLOSE_INFILE( &G.zipfd);
                return (error != PK_OK ? error : PK_BADERR);
            }
#ifndef SFX
            Info(slide, 0x401, ((char *)slide, LoadFarString(CentDirTooLong),
              G.zipfn, FmZofft((-tmp), NULL, NULL)));
#endif /* ndef SFX */
            error_in_archive = PK_ERR;
        }

    /*-----------------------------------------------------------------------
        Seek to the start of the central directory one last time, since we
        have just read the first entry's signature bytes; then list, extract
        or test member files as instructed, and close the zipfile.
      -----------------------------------------------------------------------*/

        error = seek_zipf(__G__ G.ecrec.offset_start_central_directory);
        if (error != PK_OK) {
            CLOSE_INFILE( &G.zipfd);
            return error;
        }

        Trace((stderr, "about to extract/list files (error = %d)\n",
          error_in_archive));

#ifdef DLL
        /* G.fValidate is used only to look at an archive to see if
           it appears to be a valid archive.  There is no interest
           in what the archive contains, nor in validating that the
           entries in the archive are in good condition.  This is
           currently used only in the Windows DLLs for purposes of
           checking archives within an archive to determine whether
           or not to display the inner archives.
         */
        if (!G.fValidate)
#endif /* def DLL */
        {
#ifndef NO_ZIPINFO
            if (uO.zipinfo_mode)
                error = zipinform(__G);                 /* ZIPINFO 'EM */
            else
#endif /* ndef NO_ZIPINFO */
#ifndef SFX
# ifdef TIMESTAMP
            if (uO.T_flag)
                error = get_time_stamp(__G__ &uxstamp, &nmember);
            else
# endif /* def TIMESTAMP */
            if (uO.vflag && !uO.tflag && !uO.cflag)
                error = list_files(__G);              /* LIST 'EM */
            else
#endif /* ndef SFX */
                error = extract_or_test_files(__G);   /* EXTRACT OR TEST 'EM */

            Trace((stderr, "done with extract/list files (error = %d)\n",
                   error));
        }

        if (error > error_in_archive)   /* don't overwrite stronger error */
            error_in_archive = error;   /*  with (for example) a warning */
#ifndef SFX
    } /* end if (!too_weird_to_continue) */
#endif

    CLOSE_INFILE( &G.zipfd);

#ifdef TIMESTAMP
    if (uO.T_flag && !uO.zipinfo_mode && (nmember > 0L)) {
# ifdef WIN32
        if (stamp_file(__G__ G.zipfn, uxstamp)) {       /* TIME-STAMP 'EM */
# else /* def WIN32 */
        if (stamp_file(G.zipfn, uxstamp)) {             /* TIME-STAMP 'EM */
# endif /* def WIN32 [else] */
            if (uO.qflag < 3)
                Info(slide, 0x201, ((char *)slide,
                  LoadFarString(ZipTimeStampFailed), G.zipfn));
            if (error_in_archive < PK_WARN)
                error_in_archive = PK_WARN;
        } else {
            if (!uO.qflag)
                Info(slide, 0, ((char *)slide,
                  LoadFarString(ZipTimeStampSuccess), G.zipfn));
        }
    }
#endif /* def TIMESTAMP */

    return error_in_archive;
} /* extract_archive_seekable(). */


/*******************************/
/* Function process_file_hdr() */
/*******************************/
/* Return PK-type error code. */

int process_file_hdr( __G)
    __GDEF
{
    G.pInfo->lcflag = 0;
    if (uO.L_flag == 1)       /* name conversion for monocase systems */
        switch (G.pInfo->hostnum) {
            case FS_FAT_:     /* PKZIP and zip -k store in uppercase */
            case CPM_:        /* like MS-DOS, right? */
            case VM_CMS_:     /* all caps? */
            case MVS_:        /* all caps? */
            case TANDEM_:
            case TOPS20_:
            case VMS_:        /* our Zip uses lowercase, but ASi's doesn't */
        /*  case Z_SYSTEM_:   ? */
        /*  case QDOS_:       ? */
                G.pInfo->lcflag = 1;   /* convert filename to lowercase */
                break;

            default:     /* AMIGA_, FS_HPFS_, FS_NTFS_, MAC_, UNIX_, ATARI_, */
                break;   /*  FS_VFAT_, ATHEOS_, BEOS_ (Z_SYSTEM_), THEOS_: */
                         /*  no conversion */
        }
    else if (uO.L_flag > 1)   /* let -LL force lower case for all names */
        G.pInfo->lcflag = 1;

    /* Handle the PKWare verification bit, bit 2 (0x0004) of internal
     * attributes.  If this is set, then a verification checksum is in
     * the first 3 bytes of the external attributes.  In this case all
     * we can use for setting file attributes is the last external
     * attributes byte.
     *
     * Use of this bit is incompatible with symlinks, which require use
     * of the symlink bit in the high end of the external attributes.
     */
    if (G.crec.internal_file_attributes & 0x0004)
    {
        G.crec.external_file_attributes &= (ulg)0xff;
    }

    /* do Amigas (AMIGA_) also have volume labels? */
    if (IS_VOLID( G.crec.external_file_attributes) &&
        (G.pInfo->hostnum == FS_FAT_ || G.pInfo->hostnum == FS_HPFS_ ||
         G.pInfo->hostnum == FS_NTFS_ || G.pInfo->hostnum == ATARI_ ||
         G.pInfo->hostnum == VMS_))
    {
        G.pInfo->vollabel = TRUE;
        G.pInfo->lcflag = 0;        /* preserve case of volume labels */
    } else
        G.pInfo->vollabel = FALSE;

    /* This flag is needed to detect archives made by "PKZIP for Unix"
     * when deciding which kind of codepage conversion should be applied
     * to strings (see do_string() function in fileio.c)
     */
    G.pInfo->HasUxAtt = (G.crec.external_file_attributes & 0xffff0000L) != 0L;

#ifdef UNICODE_SUPPORT
    /* remember the state of GPB11 (General Purpose Bit 11) which indicates
       that the standard path and comment are UTF-8. */
    G.pInfo->GPFIsUTF8 = ((G.crec.general_purpose_bit_flag & UTF8_BIT) != 0);
#endif

#ifdef SYMLINKS
    /* Initialize the symlink flag.  The system-specific mapattr() (or
     * other) function should set it properly.
     */
    G.pInfo->symlink = 0;
#endif

    return PK_COOL;
} /* process_file_hdr(). */


/******************************/
/* Function extract_archive() */
/******************************/

static int extract_archive( __G__ lastchance)   /* Return PK-type error code. */
  __GDEF
  int lastchance;
{
  int error;
#ifndef SFX
  int maybe_exe = FALSE;
#endif

  error = open_and_test_input_file( __G__ &lastchance C_A_MAYBE_EXE);
  if (error == 0)
  {
#if defined( ARCHIVE_STDIN) && !defined( SFX)
    if (G.ziplen < 0)
    {
      if (uO.zipinfo_mode)
      {
        /* ZipInfo currently does not support a streamed archive. */
        error = PK_PARAM;
        Info(slide, 1, ((char *)slide,
         LoadFarString(ZipInfoStream),
         LoadFarStringSmall(Zipnfo)));
      }
      else
      {
        error = extract_or_test_stream( __G);
      }
    }
    else
#endif /* defined( ARCHIVE_STDIN) && !defined( SFX) */
    {
      error = extract_archive_seekable( __G__ lastchance);
    }
  }

  return error;
} /* extract_archive() */


/**********************************/
/* Function check_auto_dest_dir() */
/**********************************/

#ifndef SFX

# define ANS ((char *)(slide + (extent)(WSIZE>> 1)))

static int check_auto_dest_dir( __G)
  __GDEF
{
  int error_auto_dest = 0;

  /* Automatic destination subdirectory? */
  if (uO.auto_exdir > 0)
  {
    int auto_exdir_state;
    int temp_reuse;
    char *auto_dest_dir;

# ifdef VMS
#  define AUTO_DEST_DIR_NAME auto_dest_dir_name
    char *auto_dest_dir_name = NULL;
# else /* def VMS */
#  define AUTO_DEST_DIR_NAME auto_dest_dir
# endif /* def VMS [else] */

    auto_exdir_state = 1;                       /* Initial state. */
    temp_reuse = 0;
    while (auto_exdir_state > 0)  /* Still ok? */
    {
      auto_exdir_state &= 3;      /* Clear transient error flag. */
      if ((auto_exdir_state& 3) == 1)
      {
        /* Get subdirectory name from archive name. */
        auto_dest_dir = name_only( G.zipfn);
      }
      else if ((auto_exdir_state& 3) == 2)
      {
        /* Get subdirectory name from user. */
        Info( slide, 0x81, ((char *)slide,
         LoadFarString( DestDirPrompt)));
        if (fgets( ANS, (WSIZE>> 1), stdin) == (char *)NULL)
        {
          /* fgets() failed (Null, EOF?). */
          Info(slide, 1, ((char *)slide,
           LoadFarString( InputError)));
          auto_exdir_state = -1;                  /* Fatal error. */
        }
        else
        {
          /* Trim trailing newline. */
          if ((strlen( ANS) > 0) && (ANS[ strlen( ANS)- 1] == '\n'))
          {
            ANS[ strlen( ANS)- 1] = '\0';
          }
          if (strlen( ANS) == 0)
          {
            /* Bad (null) response from user. */
            auto_exdir_state |= 4;                /* Transient error. */
          }
          else
          {
            /* Store the user-specified name. */
            auto_dest_dir = izu_malloc( strlen( ANS)+ 1);
            if (auto_dest_dir == NULL)
            {
              auto_exdir_state = -1;              /* Fatal error. */
            }
            else
            {
              strcpy( auto_dest_dir, ANS);
            }
          }
        }
      } /* (Else, retry the same name.) */

      if ((auto_exdir_state > 0) && (auto_exdir_state < 4))
      {
#  ifdef VMS
        /* For the existence test, VMS wants "directory_name.DIR". */
        if (auto_dest_dir != NULL)
        {
#   define DIR_TYPE ".DIR"
          auto_dest_dir_name =
          izu_malloc( (strlen( auto_dest_dir)+ sizeof( DIR_TYPE)));
          if (auto_dest_dir_name != NULL)
          {
            strcpy( auto_dest_dir_name, auto_dest_dir);
            strcpy( (auto_dest_dir_name+ strlen( auto_dest_dir)),
             DIR_TYPE);
          }
        }
#  endif /* def VMS */
        if (AUTO_DEST_DIR_NAME != NULL)
        {
          /* Name not found, or it's a directory, and reuse is ok? */
          if ((SSTAT( AUTO_DEST_DIR_NAME, &G.statbuf) != 0) ||
           ((S_ISDIR( G.statbuf.st_mode) &&
           ((uO.auto_exdir > 1) || (temp_reuse != 0)))))
          {
#  ifdef VMS
            /* For use, VMS wants "[.directory_name]", so form it
             * (in already allocated storage).
             */
            strcpy( AUTO_DEST_DIR_NAME, "[.");
            strcat( AUTO_DEST_DIR_NAME, auto_dest_dir);
            strcat( AUTO_DEST_DIR_NAME, "]");
            izu_free( auto_dest_dir);     /* Free the raw name storage. */
#  endif /* def VMS */				
            if (uO.exdir != NULL)         /* Free any old storage. */
              izu_free( uO.exdir);
            uO.exdir = AUTO_DEST_DIR_NAME;    /* Set the new dir name. */
            auto_exdir_state = 0;                 /* Happy. */
          }
          else
          {
            auto_exdir_state |= 4;                /* Transient error. */
          }
        }
        else
        {
          auto_exdir_state |= 4;                  /* Transient error. */
        }
      }
      if (auto_exdir_state >= 4)
      {
        /* Show the name of the archive being skipped. */
#  ifdef WIN32  /* Win32 console may require codepage conversion for G.zipfn. */
        Info( slide, 0, ((char *)slide,
         LoadFarString( LogInitline), FnFilter1( G.zipfn)));
#  else
        Info( slide, 0, ((char *)slide,
         LoadFarString( LogInitline), G.zipfn));
#  endif
        /* Show the name of the bad destination directory. */
        if ((auto_exdir_state& 1) != 0)
        {
          Info( slide, 1, ((char *)slide,
           LoadFarString( BadAutoDestDir), AUTO_DEST_DIR_NAME));
          auto_exdir_state = 2;                   /* Next, ask user. */
        }
        else
        {
          Info( slide, 1, ((char *)slide,
           LoadFarString( BadDestDir), AUTO_DEST_DIR_NAME));
        }

        /* Ask Dest, Retry, reUse, Skip? */
        *ANS = ' ';
        while (*ANS == ' ')
        {
          temp_reuse = 0;         /* Reset temp reuse flag. */
          Info( slide, 0x81, ((char *)slide,
           LoadFarString( AutoDestDirQuery)));
          if (fgets( ANS, (WSIZE>> 1), stdin) == (char *)NULL)
          {
            /* fgets() failed (Null, EOF?). */
            Info(slide, 1, ((char *)slide,
             LoadFarString( InputError)));
            auto_exdir_state = -1;                /* Fatal error. */
            *ANS = '\0';
          }
          else
          {
            /* Parse user input (first character). */
            switch (*ANS)
            {
              case 'D':
              case 'd':
                auto_exdir_state = 2;             /* Ask user. */
                break;
              case 'U':
              case 'u':           /* Same as 'R', except... */
                temp_reuse = 1;   /* Allow dest dir reuse (once). */
              case 'R':
              case 'r':
                auto_exdir_state = 3;             /* Retry. */
                break;
              case 'S':
              case 's':
                auto_exdir_state = -1;            /* Fatal error. */
                break;
              default:
                *ANS = ' ';       /* Ask again. */
            } /* switch */
          }
        } /* while (*ANS == ' ')  (Dest, Retry, reUse, Skip?) */
      }
    } /* while (auto_exdir_state > 0)  (Destination query.) */

    if (auto_exdir_state != 0)
    {
      error_auto_dest = 1;
    }
  } /* if (uO.auto_exdir > 0) */

  return error_auto_dest;
} /* check_auto_dest_dir() */

#endif /* ndef SFX */


#ifdef REENTRANT

/*****************************/
/* Function free_G_buffers() */
/*****************************/

void free_G_buffers(__G)     /* releases all memory allocated in global vars */
    __GDEF
{
# ifndef SFX
    unsigned i;
# endif

# ifdef SYSTEM_SPECIFIC_DTOR
    SYSTEM_SPECIFIC_DTOR(__G);
# endif

# ifdef DEFLATE_SUPPORT
    inflate_free(__G);
# endif

# ifdef UNICODE_SUPPORT
    if (G.filename_full)
    {
      izu_free(G.filename_full);
      G.filename_full = (char *)NULL;
      G.fnfull_bufsize = 0;
    }

    if ((G.unipath_filename != NULL) &&
     (G.unipath_filename != G.filename_full))
    {
      izu_free( G.unipath_filename);
    }
    G.unipath_filename = (char *)NULL;

#  ifdef WIN32_WIDE
#   ifdef DYNAMIC_WIDE_NAME
    if (G.unipath_widefilename)
    {
      izu_free( G.unipath_widefilename);
      G.unipath_widefilename = (wchar_t *)NULL;
    }
#   else /* def DYNAMIC_WIDE_NAME */
    *G.unipath_widefilename = L'\0';
#   endif /* def DYNAMIC_WIDE_NAME [else] */

    if (G.has_win32_wide)
      checkdirw(__G__ (wchar_t *)NULL, END);
    else
      checkdir(__G__ (char *)NULL, END);
#  else /* def WIN32_WIDE */
    checkdir(__G__ (char *)NULL, END);
#  endif /* def WIN32_WIDE [else] */
# endif /* def UNICODE_SUPPORT */

# ifdef DYNALLOC_CRCTAB
    if (CRC_32_TAB) {
        free_crc_table();
        CRC_32_TAB = NULL;
    }
# endif

   if (G.key != (char *)NULL) {
        izu_free(G.key);
        G.key = (char *)NULL;
   }

   if (G.extra_field != (uch *)NULL) {
        izu_free(G.extra_field);
        G.extra_field = (uch *)NULL;
   }

# if (!defined(VMS) && !defined(SMALL_MEM))
    /* VMS uses its own buffer scheme for textmode flush() */
    if (G.outbuf2) {
        izu_free(G.outbuf2);   /* malloc'd ONLY if unshrink and -a */
        G.outbuf2 = (uch *)NULL;
    }
# endif

    if (G.outbuf)
        izu_free(G.outbuf);
    if (G.inbuf)
        izu_free(G.inbuf);
    G.inbuf = G.outbuf = (uch *)NULL;

# ifdef LZMA_SUPPORT
    /* Free any LZMA-related dynamic storage. */
    if (G.struct_lzma_p != NULL)
    {
        g_lzma_free( __G);
        izu_free( G.struct_lzma_p);
        G.struct_lzma_p = NULL;
    }
# endif /* def LZMA_SUPPORT */

# ifdef PPMD_SUPPORT
    /* Free any PPMd-related dynamic storage. */
    if (G.struct_ppmd_p != NULL)
    {
        g_ppmd8_free( __G);
        izu_free( G.struct_ppmd_p);
        G.struct_ppmd_p = NULL;
    }
# endif /* def PPMD_SUPPORT */

# ifndef SFX
    for (i = 0; i < DIR_BLKSIZ; i++) {
        if (G.info[i].cfilname != (char Far *)NULL) {
            zffree(G.info[i].cfilname);
            G.info[i].cfilname = (char Far *)NULL;
        }
    }
# endif

# ifdef MALLOC_WORK
    if (G.area.Slide) {
        izu_free(G.area.Slide);
        G.area.Slide = (uch *)NULL;
    }
# endif

    /* 2012-12-11 SMS.
     * Free any command-line-related storage.
     */
# ifndef WINDLL
    /* Include and exclude name lists.  (free_args() checks for NULL.) */
    free_args( G.pfnames);
    free_args( G.pxnames);
# endif /* ndef WINDLL */
    /* Extraction root directory (-d/--extract-dir). */
    if (uO.exdir != NULL)
        izu_free( uO.exdir);
    /* Encryption password (-P/--password). */
    if (uO.pwdarg != NULL)
        izu_free( uO.pwdarg);
    /* Archive path name. */
    if (G.wildzipfn != NULL)
        izu_free( G.wildzipfn);

# if defined( UNIX) && defined( __APPLE__)
    /* AppleDouble header buffer.  (G.apl_dbl_hdr_alloc > 0?) */
    if (G.apl_dbl_hdr != NULL)
    {
        izu_free( G.apl_dbl_hdr);
        G.apl_dbl_hdr_alloc = 0;
    }
# endif /* defined( UNIX) && defined( __APPLE__) */

} /* free_G_buffers(). */

#endif /* def REENTRANT */


/*******************************/
/* Function process_zipfiles() */
/*******************************/

int process_zipfiles(__G)    /* return PK-type error code */
    __GDEF
{
#ifndef SFX
    char *zipfn_prev = (char *)NULL;
    int NumLoseFiles;
    int NumMissDirs;
    int NumMissFiles;
    int NumWarnFiles;
    int NumWinFiles;
    int want_blank = 0;
# ifndef VMS
#  ifdef ZSUFX2
    char *zipfn_sufx_p;
#  endif
# endif
#endif /* ndef SFX */

    int error = 0;
    int error_in_archive = 0;

/*---------------------------------------------------------------------------
    Start by allocating buffers and (re)constructing the various PK signature
    strings.
  ---------------------------------------------------------------------------*/

    G.inbuf = (uch *)izu_malloc(INBUFSIZ + 4);    /* +4 for hold[] (below) */
    G.outbuf = (uch *)izu_malloc(OUTBUFSIZ + 1);  /* +1 for string term. */

    if ((G.inbuf == (uch *)NULL) || (G.outbuf == (uch *)NULL)) {
        Info(slide, 0x401, ((char *)slide,
          LoadFarString(CannotAllocateBuffers)));
        return(PK_MEM);
    }
    G.hold = G.inbuf + INBUFSIZ;     /* to check for boundary-spanning sigs */
#ifndef VMS     /* VMS uses its own buffer scheme for textmode flush(). */
# ifdef SMALL_MEM
    G.outbuf2 = G.outbuf+RAWBUFSIZ;  /* never changes */
# endif
#endif /* ndef VMS */

#if 0 /* CRC_32_TAB has been NULLified by CONSTRUCTGLOBALS !!!! */
    /* allocate the CRC table later when we know we can read zipfile data */
    CRC_32_TAB = NULL;
#endif /* 0 */

    /* Finish initialization of magic signature strings.
     * Insert (ASCII, not EBCDIC) 'P' and 'K'.
     */
    central_digsig_sig[ 0] = central_hdr_sig[ 0] =
     end_centloc64_sig[ 0] = end_central_sig[ 0] =
     end_central64_sig[ 0] = /* extd_local_sig[ 0] = */
     local_hdr_sig[ 0] = 0x50;                          /* 'P'. */

    central_digsig_sig[ 1] = central_hdr_sig[ 1] =
     end_centloc64_sig[ 1] = end_central_sig[ 1] =
     end_central64_sig[ 1] = /* extd_local_sig[ 1] = */
     local_hdr_sig[ 1] = 0x4b;                          /* 'K'. */

/*---------------------------------------------------------------------------
    Make sure timezone info is set correctly; localtime() returns GMT on some
    OSes (e.g., Solaris 2.x) if this isn't done first.  The ifdefs around
    tzset() were initially copied from dos_to_unix_time() in fileio.c.  They
    may still be too strict; any listed OS that supplies tzset(), regardless
    of whether the function does anything, should be removed from the ifdefs.
  ---------------------------------------------------------------------------*/

#if (defined(WIN32) && defined(USE_EF_UT_TIME))
    /* For the Win32 environment, we may have to "prepare" the environment
       prior to the tzset() call, to work around tzset() implementation bugs.
     */
    iz_w32_prepareTZenv();
#endif

#if defined(IZ_CHECK_TZ) && defined(USE_EF_UT_TIME)
# ifndef VALID_TIMEZONE
#  define VALID_TIMEZONE(tmp) \
    (((tmp = getenv("TZ")) != NULL) && (*tmp != '\0'))
# endif
    {
        char *p;
        G.tz_is_valid = VALID_TIMEZONE(p);
# ifndef SFX
        if (!G.tz_is_valid) {
            Info(slide, 0x401, ((char *)slide, LoadFarString(WarnInvalidTZ)));
            error_in_archive = error = PK_WARN;
        }
# endif /* ndef SFX */
    }
#endif /* defined(IZ_CHECK_TZ) && defined(USE_EF_UT_TIME) */

/* For systems that do not have tzset() but supply this function using another
   name (_tzset() or something similar), an appropiate "#define tzset ..."
   should be added to the system specifc configuration section.  */
#if (!defined(T20_VMS) && !defined(MACOS) && !defined(RISCOS) && !defined(QDOS))
# if (!defined(BSD) && !defined(MTS) && !defined(CMS_MVS) && !defined(TANDEM))
    tzset();
# endif
#endif

/* Initialize UnZip's built-in pseudo hard-coded "ISO <--> OEM" translation,
   depending on the detected codepage setup.  */
#ifdef NEED_ISO_OEM_INIT
    prepare_ISO_OEM_translat(__G);
#endif

/*---------------------------------------------------------------------------
    Initialize the internal flag holding the mode of processing "overwrite
    existing file" cases.  We do not use the calling interface flags directly
    because the overwrite mode may be changed by user interaction while
    processing archive files.  Such a change should not affect the option
    settings as passed through the DLL calling interface.
    In case of conflicting options, the 'safer' flag uO.overwrite_none takes
    precedence.
  ---------------------------------------------------------------------------*/
    G.overwrite_mode = (uO.overwrite_none ? OVERWRT_NEVER :
                        (uO.overwrite_all ? OVERWRT_ALWAYS : OVERWRT_QUERY));

/*---------------------------------------------------------------------------
    Match (possible) wildcard zipfile specification with existing files and
    attempt to process each.  If no hits, try again after appending ".zip"
    suffix.  If still no luck, give up.
  ---------------------------------------------------------------------------*/

#ifdef SFX
    /* A self-extracting archive processes itself (only). */
    if ((error = extract_archive(__G__ 0)) == PK_NOZIP) {
# ifdef EXE_EXTENSION
        int len = strlen(G.argv0);

        /* append .exe if appropriate; also .sfx? */
        if ( (G.zipfn = (char *)izu_malloc(len+sizeof(EXE_EXTENSION))) !=
             (char *)NULL ) {
            strcpy(G.zipfn, G.argv0);
            strcpy(G.zipfn+len, EXE_EXTENSION);
            error = extract_archive(__G__ 0);
            izu_free(G.zipfn);
            G.zipfn = G.argv0;  /* for "cannot find myself" message only */
        }
# endif /* def EXE_EXTENSION */
# ifdef WIN32
        G.zipfn = G.argv0;  /* for "cannot find myself" message only */
# endif
    }
    if (error) {
        if (error == IZ_DIR)
            error_in_archive = PK_NOZIP;
        else
            error_in_archive = error;
        if (error == PK_NOZIP)
            Info(slide, 1, ((char *)slide, LoadFarString(CannotFindMyself),
              G.zipfn));
    }
# ifdef CHEAP_SFX_AUTORUN
    if (G.autorun_command[0] && !uO.qflag) { /* NO autorun without prompt! */
        Info(slide, 0x81, ((char *)slide, LoadFarString(AutorunPrompt),
                      FnFilter1(G.autorun_command)));
        if ((fgets_ans( __G) >= 0) && (toupper( *G.answerbuf) == 'Y'))
            system(G.autorun_command);
        else
            Info(slide, 1, ((char *)slide, LoadFarString(NotAutoRunning)));
    }
# endif /* CHEAP_SFX_AUTORUN */

#else /* def SFX */

    /* Non-SFX archive names may be wildcards, or missing ".zip" suffix. */
    NumWinFiles = NumLoseFiles = NumWarnFiles = 0;
    NumMissDirs = NumMissFiles = 0;

    while ((G.zipfn = do_wild(__G__ G.wildzipfn)) != (char *)NULL)
    {
        Trace((stderr, "do_wild( %s ) returns %s\n", G.wildzipfn, G.zipfn));

        zipfn_prev = G.zipfn;

        /* Print a blank line between the output of different zipfiles. */
        if (!uO.qflag && (want_blank != 0)
# ifdef TIMESTAMP
         && (!uO.T_flag || uO.zipinfo_mode)
# endif
        )
        {
            (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0);
        }
        else
            want_blank = 1;

        /* Check auto destination directory. */
        if (check_auto_dest_dir( __G) != 0)
        {
            /* Bad auto dest dir.  Don't process archive. */
            error = IZ_BADDEST;
            ++NumLoseFiles;
        }
        else
        {
            /* No auto destination dir problem.  Process normally. */
            error = extract_archive(__G__ 0);
            Trace((stderr, "extract_archive(0) returns %d\n", error));

            if (error == PK_WARN)
                ++NumWarnFiles;
            else if (error == IZ_DIR)
                ++NumMissDirs;
            else if (error == PK_NOZIP)
                ++NumMissFiles;
            else if (error != PK_OK)
                ++NumLoseFiles;
            else
                ++NumWinFiles;
        }

        if ((error != IZ_DIR) && (error > error_in_archive))
            error_in_archive = error;
# ifdef WINDLL
        if (error == IZ_CTRLC) {
            free_G_buffers(__G);
            return error;
        }
# endif
        if (uO.auto_exdir > 0)
        {
            /* Reset the root directory storage indicator to make ready
             * for the next wildcard archive name (and its corresponding
             * destination directory).
             */
            checkdir( __G__ (char *)NULL, END);
        }
    } /* end while-loop (wildcard zipfiles) */

    if (((NumWinFiles + NumWarnFiles + NumLoseFiles) == 0) &&
        ((NumMissDirs + NumMissFiles) == 1) && (zipfn_prev != (char *)NULL))
    {
# if !defined(UNIX) && !defined(AMIGA)  /* Wildcard archive name possible. */
        if (iswild(G.wildzipfn))
        {
            /* Wildcard archive name.  Don't try adding SUFX[2]. */
            if (iswild(zipfn_prev))
            {
                NumMissDirs = NumMissFiles = 0;
                error_in_archive = PK_COOL;
                if (uO.qflag < 3)
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(CannotFindWildcardMatch),
                      LoadFarStringSmall((uO.zipinfo_mode ? Zipnfo : Unzip)),
                      G.wildzipfn));
            }
        }
        else
# endif /* !defined(UNIX) && !defined(AMIGA) */
        {
            /* Non-wildcard archive name.  Try adding SUFX[2], except on
             * VMS, which uses a default file spec instead.
             */
# ifdef VMS
            /* Last chance try, only to generate the error report. */
            G.zipfn = zipfn_prev;
            error = extract_archive(__G__ 1);
# else /* def VMS */

# ifdef ARCHIVE_STDIN
            if (G.zipstdin == 0)    /* Don't play with archive name "-". */
# endif /* def ARCHIVE_STDIN */
            {
                NumMissDirs = NumMissFiles = 0;
                error_in_archive = PK_COOL;

                /* 2005-08-14 Chr. Spieler
                 * Although we already "know" the failure result, we call
                 * extract_archive() again with the same zipfile name (and
                 * the lastchance flag set), just to trigger the error report.
                 */
                /* Append ZSUFX to the archive name, and try again. */
#  ifdef ZSUFX2
                zipfn_sufx_p =          /* Save pointer for ZSUFX2, below. */
#  endif
                 strcpy( zipfn_prev+ strlen( zipfn_prev), ZSUFX);

                G.zipfn = zipfn_prev;

#  ifdef ZSUFX2
                if (((error = extract_archive(__G__ 0)) == PK_NOZIP) ||
                 (error == IZ_DIR))
                {
                    if (error == IZ_DIR)
                        ++NumMissDirs;

                    /* Append ZSUFX2 to the archive name, and try again. */
                    strcpy( zipfn_sufx_p, ZSUFX2);
                    error = extract_archive(__G__ 1);
                }
#  else /* def ZSUFX2 */
                error = extract_archive(__G__ 1);
#  endif /* def ZSUFX2 [else] */
            } /* if (G.zipstdin == 0) */
# endif /* def VMS [else] */

            Trace((stderr, "extract_archive(1) returns %d\n", error));
            switch (error) {
              case PK_WARN:
                ++NumWarnFiles;
                break;
              case IZ_DIR:
                ++NumMissDirs;
                error = PK_NOZIP;
                break;
              case PK_NOZIP:
                /* increment again => bug:
                   "1 file had no zipfile directory." */
                /* ++NumMissFiles */ ;
                break;
              default:
                if (error)
                    ++NumLoseFiles;
                else
                    ++NumWinFiles;
                break;
            }

            if (error > error_in_archive)
                error_in_archive = error;
# ifdef WINDLL
            if (error == IZ_CTRLC) {
                free_G_buffers(__G);
                return error;
            }
# endif /* def WINDLL */
        }
    }
#endif /* def SFX [else] */

/*---------------------------------------------------------------------------
    Print summary of all zipfiles, assuming zipfile spec was a wildcard (no
    need for a summary if just one zipfile).
  ---------------------------------------------------------------------------*/

#ifndef SFX
    if (iswild(G.wildzipfn) && (uO.qflag < 3)
# ifdef TIMESTAMP
        && !(uO.T_flag && !uO.zipinfo_mode && (uO.qflag > 1))
# endif
                                                    )
    {
        if (((NumMissFiles+ NumLoseFiles+ NumWarnFiles > 0) ||
         (NumWinFiles != 1))
# ifdef TIMESTAMP
            && !(uO.T_flag && !uO.zipinfo_mode && uO.qflag)
# endif
            && !(uO.tflag && uO.qflag > 1))
             (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0x401);

        if ((NumWinFiles > 1) ||
            (NumWinFiles == 1 &&
             NumMissDirs + NumMissFiles + NumLoseFiles + NumWarnFiles > 0))
            Info(slide, 0x401, ((char *)slide, LoadFarString(FilesProcessOK),
              NumWinFiles, (NumWinFiles == 1)? " was" : "s were"));
        if (NumWarnFiles > 0)
            Info(slide, 0x401, ((char *)slide, LoadFarString(ArchiveWarning),
              NumWarnFiles, (NumWarnFiles == 1)? "" : "s"));
        if (NumLoseFiles > 0)
            Info(slide, 0x401, ((char *)slide, LoadFarString(ArchiveFatalError),
              NumLoseFiles, (NumLoseFiles == 1)? "" : "s"));
        if (NumMissFiles > 0)
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(FileHadNoZipfileDir), NumMissFiles,
              (NumMissFiles == 1)? "" : "s"));
        if (NumMissDirs == 1)
            Info(slide, 0x401, ((char *)slide, LoadFarString(ZipfileWasDir)));
        else if (NumMissDirs > 0)
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ManyZipfilesWereDir), NumMissDirs));
        if (NumWinFiles + NumLoseFiles + NumWarnFiles == 0)
            Info(slide, 0x401, ((char *)slide, LoadFarString(NoZipfileFound)));
    }
#endif /* ndef SFX */

    /* 2012-12-11 SMS.
     * Freeing dynamic storage is now done by globals.h:DESTROYGLOBALS()
     * (using free_G_buffers()), but only if REENTRANT.
     */
#if 0 /* Disabled. */
    /* free allocated memory */
    free_G_buffers(__G);
#endif /* 0 */

    return error_in_archive;

} /* process_zipfiles(). */


#if 0 /* currently unused */
/********************************/
/* Function check_ecrec_zip64() */
/********************************/

static int check_ecrec_zip64(__G)
    __GDEF
{
    return G.ecrec.offset_start_central_directory  == 0xFFFFFFFFL
        || G.ecrec.size_central_directory          == 0xFFFFFFFFL
        || G.ecrec.total_entries_central_dir       == 0xFFFF
        || G.ecrec.num_entries_centrl_dir_ths_disk == 0xFFFF
        || G.ecrec.num_disk_start_cdir             == 0xFFFF
        || G.ecrec.number_this_disk                == 0xFFFF;
} /* check_ecrec_zip64(). */
#endif /* 0 */


/***************************/
/* Function get_cdir_ent() */
/***************************/

static int get_cdir_ent(__G)    /* return PK-type error code */
    __GDEF
{
    cdir_byte_hdr byterec;

/*---------------------------------------------------------------------------
    Read the next central directory entry and do any necessary machine-type
    conversions (byte ordering, structure padding compensation--do so by
    copying the data from the array into which it was read (byterec) to the
    usable struct (crec)).
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ byterec, CREC_SIZE) == 0)
        return PK_EOF;

    G.crec.version_made_by[0] = byterec[C_VERSION_MADE_BY_0];
    G.crec.version_made_by[1] = byterec[C_VERSION_MADE_BY_1];
    G.crec.version_needed_to_extract[0] =
      byterec[C_VERSION_NEEDED_TO_EXTRACT_0];
    G.crec.version_needed_to_extract[1] =
      byterec[C_VERSION_NEEDED_TO_EXTRACT_1];

    G.crec.general_purpose_bit_flag =
      makeword(&byterec[C_GENERAL_PURPOSE_BIT_FLAG]);
    G.crec.compression_method =
      makeword(&byterec[C_COMPRESSION_METHOD]);
    G.crec.last_mod_dos_datetime =
      makelong(&byterec[C_LAST_MOD_DOS_DATETIME]);
    G.crec.crc32 =
      makelong(&byterec[C_CRC32]);
    G.crec.csize =
      makelong(&byterec[C_COMPRESSED_SIZE]);
    G.crec.ucsize =
      makelong(&byterec[C_UNCOMPRESSED_SIZE]);
    G.crec.filename_length =
      makeword(&byterec[C_FILENAME_LENGTH]);
    G.crec.extra_field_length =
      makeword(&byterec[C_EXTRA_FIELD_LENGTH]);
    G.crec.file_comment_length =
      makeword(&byterec[C_FILE_COMMENT_LENGTH]);
    G.crec.disk_number_start =
      makeword(&byterec[C_DISK_NUMBER_START]);
    G.crec.internal_file_attributes =
      makeword(&byterec[C_INTERNAL_FILE_ATTRIBUTES]);
    G.crec.external_file_attributes =
      makelong(&byterec[C_EXTERNAL_FILE_ATTRIBUTES]);  /* LONG, not word! */
    G.crec.relative_offset_local_header =
      makelong(&byterec[C_RELATIVE_OFFSET_LOCAL_HEADER]);

    return PK_COOL;
} /* get_cdir_ent(). */


/************************************/
/* Function process_cdir_file_hdr() */
/************************************/

int process_cdir_file_hdr(__G)    /* return PK-type error code */
    __GDEF
{
    int error;

/*---------------------------------------------------------------------------
    Get central directory info, save host and method numbers, and set flag
    for lowercase conversion of filename, depending on the OS from which the
    file is coming.
  ---------------------------------------------------------------------------*/

    if ((error = get_cdir_ent(__G)) != 0)
        return error;

    G.pInfo->hostver = G.crec.version_made_by[0];
    G.pInfo->hostnum = IZ_MIN(G.crec.version_made_by[1], NUM_HOSTS);
/*  extnum = IZ_MIN(crec.version_needed_to_extract[1], NUM_HOSTS); */

    error = process_file_hdr( __G);

    return error;

} /* process_cdir_file_hdr(). */


/*************************************/
/* Function process_local_file_hdr() */
/*************************************/

int process_local_file_hdr(__G)    /* return PK-type error code */
    __GDEF
{
    local_byte_hdr byterec;

/*---------------------------------------------------------------------------
    Read the next local file header and do any necessary machine-type con-
    versions (byte ordering, structure padding compensation--do so by copy-
    ing the data from the array into which it was read (byterec) to the
    usable struct (lrec)).
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ byterec, LREC_SIZE) == 0)
        return PK_EOF;

    G.lrec.version_needed_to_extract[0] =
      byterec[L_VERSION_NEEDED_TO_EXTRACT_0];
    G.lrec.version_needed_to_extract[1] =
      byterec[L_VERSION_NEEDED_TO_EXTRACT_1];

    G.lrec.general_purpose_bit_flag =
      makeword(&byterec[L_GENERAL_PURPOSE_BIT_FLAG]);
    G.lrec.compression_method = makeword(&byterec[L_COMPRESSION_METHOD]);
    G.lrec.last_mod_dos_datetime = makelong(&byterec[L_LAST_MOD_DOS_DATETIME]);
    G.lrec.crc32 = makelong(&byterec[L_CRC32]);
    G.lrec.csize = makelong(&byterec[L_COMPRESSED_SIZE]);
    G.lrec.ucsize = makelong(&byterec[L_UNCOMPRESSED_SIZE]);
    G.lrec.filename_length = makeword(&byterec[L_FILENAME_LENGTH]);
    G.lrec.extra_field_length = makeword(&byterec[L_EXTRA_FIELD_LENGTH]);

    if ((G.lrec.general_purpose_bit_flag & 8) != 0) {
        /* can't trust local header, use central directory: */
        G.lrec.crc32 = G.pInfo->crc;
        G.lrec.csize = G.pInfo->compr_size;
        G.lrec.ucsize = G.pInfo->uncompr_size;
    }

    G.csize = G.lrec.csize;

    return PK_COOL;
} /* process_local_file_hdr(). */


/**********************************/
/* Function process_cdir_digsig() */
/**********************************/

int process_cdir_digsig(__G__ enddigsig_len_p)
    __GDEF
    long *enddigsig_len_p;
{
  uch *digsig_buf;
  uch digsig_len[ 2];
  long enddigsig_len;
  int error = PK_OK;            /* Return PK-type status.  Assume success. */

  /* Read the two-byte byte count. */
  if (readbuf(__G__ digsig_len, 2) < 2)
  {
    Info(slide, 0x221, ((char *)slide,
     LoadFarString( ErrorUnexpectedEOF), 2, G.zipfn));
    error = PK_EOF;
  }
  else
  { /* Good byte count data.  Assemble the value.  Return it, if requested. */
    enddigsig_len = makeword( digsig_len);
    if (enddigsig_len_p != NULL)
    {
      *enddigsig_len_p = enddigsig_len;
    }
    /* Allocate storage for data. */
    digsig_buf = (uch *)izu_malloc( enddigsig_len);
    if (digsig_buf == (uch *)NULL)
    {
      Info( slide, 0x401, ((char *)slide,
       "Not enough memory for Central Dir digital signature"));
      error = PK_MEM;
    }
    else
    { /* Have storage.  Read the data. */
      if ((long)readbuf(__G__ digsig_buf, enddigsig_len) < enddigsig_len)
      {
        Info(slide, 0x221, ((char *)slide,
         LoadFarString( ErrorUnexpectedEOF), 21, G.zipfn));
        error = PK_EOF;
      }
      else
      {
        /* Process digital signature here.  (Currently not done.) */
        /* Then free its storage. */
        izu_free( digsig_buf);

        /* Read what should be the EOCD[64] signature. */
        if (readbuf(__G__ G.sig, 4) < 4)
        {
          Info(slide, 0x221, ((char *)slide,
           LoadFarString( ErrorUnexpectedEOF), 22, G.zipfn));
          error = PK_EOF;
        }
      }
    }
  }

  return error;

} /* process_cdir_digsig(). */


/*******************************/
/* Function getZip64Data() */
/*******************************/

int getZip64Data(__G__ ef_buf, ef_len)
    __GDEF
    ZCONST uch *ef_buf; /* buffer containing extra field */
    long ef_len;        /* total length of extra field */
{
    unsigned eb_id;
    long eb_len;

/*---------------------------------------------------------------------------
    This function scans the extra field for zip64 information, ie 8-byte
    versions of compressed file size, uncompressed file size, relative offset
    and a 4-byte version of disk start number.
    Sets both local header and central header fields.  Not terribly clever,
    but it means that this procedure is only called in one place.
  ---------------------------------------------------------------------------*/

/* 2012-11-25 SMS.  (OUSPG report.)
 * Changed eb_len and ef_len from unsigned to signed, to catch underflow
 * of ef_len caused by corrupt/malicious data.  (32-bit is adequate.
 * Used "long" to accommodate any systems with 16-bit "int".)
 *
 * 2014-12-17 SMS.  (oCERT.org report.)
 * Added checks to ensure that enough data are available before calling
 * makeint64() or makelong().  Replaced various sizeof() values with
 * simple ("4" or "8") constants.  (The Zip64 structures do not depend
 * on our variable sizes.)  Error handling is crude, but we should now
 * stay within the buffer.
 */

#define Z64FLGS 0xffff
#define Z64FLGL 0xffffffff

    if (ef_len == 0 || ef_buf == NULL)
        return PK_COOL;

    Trace((stderr, "\ngetZip64Data: scanning extra field of length %ld\n",
      ef_len));

    while (ef_len >= EB_HEADSIZE)
    {
        eb_id = makeword( EB_ID+ ef_buf);
        eb_len = makeword( EB_LEN+ ef_buf);

        if (eb_len > (ef_len- EB_HEADSIZE))
        {
            /* Extra block length exceeds remaining extra field length. */
            Trace((stderr,
             "getZip64Data: block length %u > rest ef_size %u\n",
             eb_len, (ef_len- EB_HEADSIZE)));
            break;
        }

        if (eb_id == EF_PKSZ64)
        {
          int offset = EB_HEADSIZE;

          if ((G.crec.ucsize == Z64FLGL) || (G.lrec.ucsize == Z64FLGL))
          {
            if (offset+ 8 > ef_len)
              return PK_ERR;

            G.crec.ucsize = G.lrec.ucsize = makeint64( offset+ ef_buf);
            offset += 8;
          }

          if ((G.crec.csize == Z64FLGL) || (G.lrec.csize == Z64FLGL))
          {
            if (offset+ 8 > ef_len)
              return PK_ERR;

            G.csize = G.crec.csize = G.lrec.csize = makeint64( offset+ ef_buf);
            offset += 8;
          }

          if (G.crec.relative_offset_local_header == Z64FLGL)
          {
            if (offset+ 8 > ef_len)
              return PK_ERR;

            G.crec.relative_offset_local_header = makeint64( offset+ ef_buf);
            offset += 8;
          }

          if (G.crec.disk_number_start == Z64FLGS)
          {
            if (offset+ 4 > ef_len)
              return PK_ERR;

            G.crec.disk_number_start = (zuvl_t)makelong( offset+ ef_buf);
            offset += 4;
          }

          break;                /* Expect only one EF_PKSZ64 block. */
        }

        /* Advance to the next extra field block. */
        ef_buf += (eb_len+ EB_HEADSIZE);
        ef_len -= (eb_len+ EB_HEADSIZE);
    }

    return PK_COOL;
} /* getZip64Data(). */


#ifdef UNICODE_SUPPORT

/*******************************/
/* Function getUnicodeData() */
/*******************************/

/* 2012-11-25 SMS.  (OUSPG report.)
 * See note at getZip64Data().
 */

int getUnicodeData(__G__ ef_buf, ef_len)
    __GDEF
    ZCONST uch *ef_buf; /* buffer containing extra field */
    long ef_len;        /* total length of extra field */
{
    unsigned eb_id;
    long eb_len;

/*---------------------------------------------------------------------------
    This function scans the extra field for Unicode information, ie UTF-8
    path extra fields.

    Caller should first set G.unipath_filename to NULL (after freeing
    old storage, as needed.)
    On return, G.unipath_filename =
        <unchanged> (NULL), if error or no Unicode path extra field
        "", if the standard path is UTF-8
        NUL-terminated UTF-8 path from extra field
    Any non-NULL G.unipath_filename (including "") was malloc()'d here,
    and the caller is resposible for free()-ing it.
    Return PK_COOL if no error.
  ---------------------------------------------------------------------------*/

    if (ef_len == 0 || ef_buf == NULL)
        return PK_COOL;

    Trace((stderr, "\ngetUnicodeData: scanning extra field of length %ld\n",
      ef_len));

    while (ef_len >= EB_HEADSIZE) {
        eb_id = makeword(EB_ID + ef_buf);
        eb_len = makeword(EB_LEN + ef_buf);

        if (eb_len > (ef_len - EB_HEADSIZE)) {
            /* discovered some extra field inconsistency! */
            Trace((stderr,
             "getUnicodeData: block length %ld > rest ef_size %ld\n",
             eb_len, ef_len - EB_HEADSIZE));
            break;
        }
        if (eb_id == EF_UNIPATH) {

          int offset = EB_HEADSIZE;
          ush ULen = (ush)(eb_len - 5);
          ulg chksum = CRCVAL_INITIAL;

          /* version */
          G.unipath_version = (uch) *(offset + ef_buf);
          offset += 1;
          if (G.unipath_version > 1) {
            /* can do only version 1 */
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(UnicodeVersionError)));
            return PK_ERR;
          }

          /* filename CRC */
          G.unipath_checksum = makelong(offset + ef_buf);
          offset += 4;

          /*
           * Compute 32-bit crc
           */

          chksum = crc32(chksum, (uch *)(G.filename_full),
                         strlen(G.filename_full));

          /* If the checksums's don't match then likely filename has been
           * modified and the Unicode Path is no longer valid.
           */
          if (chksum != G.unipath_checksum) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(UnicodeMismatchError)));
            if (G.unicode_mismatch == 1) {
              /* warn and continue */
            } else if (G.unicode_mismatch == 2) {
              /* ignore and continue */
            } else if (G.unicode_mismatch == 0) {
            }
            return PK_ERR;
          }

          /* UTF-8 Path */
          if ((G.unipath_filename = izu_malloc(ULen + 1)) == NULL) {
            return PK_ERR;
          }
          if (ULen == 0) {
            /* standard path is UTF-8 so use that */
            G.unipath_filename[0] = '\0';
          } else {
            /* UTF-8 path */
            strncpy(G.unipath_filename,
                    (ZCONST char *)(offset + ef_buf), ULen);
            G.unipath_filename[ULen] = '\0';
          }
        }

        /* Skip this extra field block */
        ef_buf += (eb_len + EB_HEADSIZE);
        ef_len -= (eb_len + EB_HEADSIZE);
    }

    return PK_COOL;
} /* getUnicodeData(). */


# ifdef UNICODE_WCHAR
/*---------------------------------------------
 * Unicode conversion functions
 *
 * Based on functions provided by Paul Kienitz
 *
 *---------------------------------------------
 */

/*
   NOTES APPLICABLE TO ALL STRING FUNCTIONS:

   All of the x_to_y functions take parameters for an output buffer and
   its available length, and return an int.  The value returned is the
   length of the string that the input produces, which may be larger than
   the provided buffer length.  If the returned value is less than the
   buffer length, then the contents of the buffer will be null-terminated;
   otherwise, it will not be terminated and may be invalid, possibly
   stopping in the middle of a multibyte sequence.

   In all cases you may pass NULL as the buffer and/or 0 as the length, if
   you just want to learn how much space the string is going to require.

   The functions will return -1 if the input is invalid UTF-8 or cannot be
   encoded as UTF-8.
*/

static int utf8_char_bytes OF((ZCONST char *utf8));
static ulg ucs4_char_from_utf8 OF((ZCONST char **utf8));
static int utf8_to_ucs4_string OF((ZCONST char *utf8, ulg *ucs4buf,
                                   int buflen));

/* utility functions for managing UTF-8 and UCS-4 strings */


/* utf8_char_bytes
 *
 * Returns the number of bytes used by the first character in a UTF-8
 * string, or -1 if the UTF-8 is invalid or null.
 */
static int utf8_char_bytes(utf8)
  ZCONST char *utf8;
{
  int      t, r;
  unsigned lead;

  if (!utf8)
    return -1;          /* no input */
  lead = (unsigned char) *utf8;
  if (lead < 0x80)
    r = 1;              /* an ascii-7 character */
  else if (lead < 0xC0)
    return -1;          /* error: trailing byte without lead byte */
  else if (lead < 0xE0)
    r = 2;              /* an 11 bit character */
  else if (lead < 0xF0)
    r = 3;              /* a 16 bit character */
  else if (lead < 0xF8)
    r = 4;              /* a 21 bit character (the most currently used) */
  else if (lead < 0xFC)
    r = 5;              /* a 26 bit character (shouldn't happen) */
  else if (lead < 0xFE)
    r = 6;              /* a 31 bit character (shouldn't happen) */
  else
    return -1;          /* error: invalid lead byte */
  for (t = 1; t < r; t++)
    if ((unsigned char) utf8[t] < 0x80 || (unsigned char) utf8[t] >= 0xC0)
      return -1;        /* error: not enough valid trailing bytes */
  return r;
}


/* ucs4_char_from_utf8
 *
 * Given a reference to a pointer into a UTF-8 string, returns the next
 * UCS-4 character and advances the pointer to the next character sequence.
 * Returns ~0 (= -1 in twos-complement notation) and does not advance the
 * pointer when input is ill-formed.
 */
static ulg ucs4_char_from_utf8(utf8)
  ZCONST char **utf8;
{
  ulg  ret;
  int  t, bytes;

  if (!utf8)
    return ~0L;                         /* no input */
  bytes = utf8_char_bytes(*utf8);
  if (bytes <= 0)
    return ~0L;                         /* invalid input */
  if (bytes == 1)
    ret = **utf8;                       /* ascii-7 */
  else
    ret = **utf8 & (0x7F >> bytes);     /* lead byte of a multibyte sequence */
  (*utf8)++;
  for (t = 1; t < bytes; t++)           /* consume trailing bytes */
    ret = (ret << 6) | (*((*utf8)++) & 0x3F);
  return (zwchar) ret;
}


#if 0 /* currently unused */
/* utf8_from_ucs4_char - Convert UCS char to UTF-8
 *
 * Returns the number of bytes put into utf8buf to represent ch, from 1 to 6,
 * or -1 if ch is too large to represent.  utf8buf must have room for 6 bytes.
 */
static int utf8_from_ucs4_char(utf8buf, ch)
  char *utf8buf;
  ulg ch;
{
  int trailing = 0;
  int leadmask = 0x80;
  int leadbits = 0x3F;
  int tch = ch;
  int ret;

  if (ch > 0x7FFFFFFFL)
    return -1;                /* UTF-8 can represent 31 bits */
  if (ch < 0x7F)
  {
    *utf8buf++ = (char) ch;   /* ascii-7 */
    return 1;
  }
  do {
    trailing++;
    leadmask = (leadmask >> 1) | 0x80;
    leadbits >>= 1;
    tch >>= 6;
  } while (tch & ~leadbits);
  ret = trailing + 1;
  /* produce lead byte */
  *utf8buf++ = (char) (leadmask | (ch >> (6 * trailing)));
  while (--trailing >= 0)
    /* produce trailing bytes */
    *utf8buf++ = (char) (0x80 | ((ch >> (6 * trailing)) & 0x3F));
  return ret;
}
#endif /* 0 */


/*===================================================================*/

/* utf8_to_ucs4_string - convert UTF-8 string to UCS string
 *
 * Return UCS count.  Now returns int so can return -1.
 */
static int utf8_to_ucs4_string(utf8, ucs4buf, buflen)
  ZCONST char *utf8;
  ulg *ucs4buf;
  int buflen;
{
  int count = 0;

  for (;;)
  {
    ulg ch = ucs4_char_from_utf8(&utf8);
    if (ch == (ulg)~0L)
      return -1;
    else
    {
      if (ucs4buf && count < buflen)
        ucs4buf[count] = ch;
      if (ch == 0)
        return count;
      count++;
    }
  }
}


#if 0 /* currently unused */
/* ucs4_string_to_utf8
 *
 *
 */
static int ucs4_string_to_utf8(ucs4, utf8buf, buflen)
  ZCONST ulg *ucs4;
  char *utf8buf;
  int buflen;
{
  char mb[6];
  int  count = 0;

  if (!ucs4)
    return -1;
  for (;;)
  {
    int mbl = utf8_from_ucs4_char(mb, *ucs4++);
    int c;
    if (mbl <= 0)
      return -1;
    /* We could optimize this a bit by passing utf8buf + count */
    /* directly to utf8_from_ucs4_char when buflen >= count + 6... */
    c = buflen - count;
    if (mbl < c)
      c = mbl;
    if (utf8buf && count < buflen)
      strncpy(utf8buf + count, mb, c);
    if (mbl == 1 && !mb[0])
      return count;           /* terminating nul */
    count += mbl;
  }
}


/* utf8_chars
 *
 * Wrapper: counts the actual unicode characters in a UTF-8 string.
 */
static int utf8_chars(utf8)
  ZCONST char *utf8;
{
  return utf8_to_ucs4_string(utf8, NULL, 0);
}
#endif /* 0 */

/* --------------------------------------------------- */
/* Unicode Support
 *
 * These functions common for all Unicode ports.
 *
 * These functions should allocate and return strings that can be
 * freed with free().
 *
 * 8/27/05 EG
 *
 * Use zwchar for wide char which is unsigned long
 * in zip.h and 32 bits.  This avoids problems with
 * different sizes of wchar_t.
 */

#if 0 /* currently unused */
/* is_ascii_string
 * Checks if a string is all ascii
 */
int is_ascii_string(mbstring)
  ZCONST char *mbstring;
{
  char *p;
  uch c;

  for (p = mbstring; c = (uch)*p; p++) {
    if (c > 0x7F) {
      return 0;
    }
  }
  return 1;
}

/* local to UTF-8 */
char *local_to_utf8_string(local_string)
  ZCONST char *local_string;
{
  return wide_to_utf8_string(local_to_wide_string(local_string));
}
# endif /* 0 */

/* wide_to_escape_string
   provides a string that represents a wide char not in local char set

   An initial try at an algorithm.  Suggestions welcome.

   According to the standard, Unicode character points are restricted to
   the number range from 0 to 0x10FFFF, respective 21 bits.
   For a hexadecimal notation, 2 octets are sufficient for the mostly
   used characters from the "Basic Multilingual Plane", all other
   Unicode characters can be represented by 3 octets (= 6 hex digits).
   The Unicode standard suggests to write Unicode character points
   as 4 resp. 6 hex digits, preprended by "U+".
   (e.g.: U+10FFFF for the highest character point, or U+0030 for the ASCII
   digit "0")

   However, for the purpose of escaping non-ASCII chars in an ASCII character
   stream, the "U" is not a very good escape initializer. Therefore, we
   use the following convention within our Info-ZIP code:

   If not an ASCII char probably need 2 bytes at least.  So if
   a 2-byte wide encode it as 4 hex digits with a leading #U.  If
   needs 3 bytes then prefix the string with #L.  So
   #U1234
   is a 2-byte wide character with bytes 0x12 and 0x34 while
   #L123456
   is a 3-byte wide character with bytes 0x12, 0x34, 0x56.
   On Windows, wide that need two wide characters need to be converted
   to a single number.
  */

 /* set this to the max bytes an escape can be */
#  define MAX_ESCAPE_BYTES 8

char *wide_to_escape_string(wide_char)
  zwchar wide_char;
{
  int i;
  zwchar w = wide_char;
  uch b[sizeof(zwchar)];
  char d[3];
  char e[11];
  int len;
  char *r;

  /* fill byte array with zeros */
  memzero(b, sizeof(zwchar));
  /* get bytes in right to left order */
  for (len = 0; w; len++) {
    b[len] = (char)(w % 0x100);
    w /= 0x100;
  }
  strcpy(e, "#");
  /* either 2 bytes or 3 bytes */
  if (len <= 2) {
    len = 2;
    strcat(e, "U");
  } else {
    strcat(e, "L");
  }
  for (i = len - 1; i >= 0; i--) {
    sprintf(d, "%02x", b[i]);
    strcat(e, d);
  }
  if ((r = izu_malloc(strlen(e) + 1)) == NULL) {
    return NULL;
  }
  strcpy(r, e);
  return r;
}

#if 0 /* currently unused */
/* returns the wide character represented by the escape string */
zwchar escape_string_to_wide(escape_string)
  ZCONST char *escape_string;
{
  int i;
  zwchar w;
  char c;
  int len;
  ZCONST char *e = escape_string;

  if (e == NULL) {
    return 0;
  }
  if (e[0] != '#') {
    /* no leading # */
    return 0;
  }
  len = strlen(e);
  /* either #U1234 or #L123456 format */
  if (len != 6 && len != 8) {
    return 0;
  }
  w = 0;
  if (e[1] == 'L') {
    if (len != 8) {
      return 0;
    }
    /* 3 bytes */
    for (i = 2; i < 8; i++) {
      c = e[i];
      if (c < '0' || c > '9') {
        return 0;
      }
      w = w * 0x10 + (zwchar)(c - '0');
    }
  } else if (e[1] == 'U') {
    /* 2 bytes */
    for (i = 2; i < 6; i++) {
      c = e[i];
      if (c < '0' || c > '9') {
        return 0;
      }
      w = w * 0x10 + (zwchar)(c - '0');
    }
  }
  return w;
}
#endif /* 0 */

#  ifndef WIN32  /* WIN32 supplies a special variant of this function */
/* convert wide character string to multi-byte character string */
char *wide_to_local_string(wide_string, escape_all)
  ZCONST zwchar *wide_string;
  int escape_all;
{
  int i;
  wchar_t wc;
  int b;
  int state_dependent;
  int wsize;
  int max_bytes = MB_CUR_MAX;
  char buf[9];
  char *buffer = NULL;
  char *local_string = NULL;

  if (wide_string == NULL)
    return NULL;

  for (wsize = 0; wide_string[wsize]; wsize++) ;

  if (max_bytes < MAX_ESCAPE_BYTES)
    max_bytes = MAX_ESCAPE_BYTES;

  if ((buffer = (char *)izu_malloc(wsize * max_bytes + 1)) == NULL) {
    return NULL;
  }

  /* convert it */
  buffer[0] = '\0';
  /* set initial state if state-dependent encoding */
  wc = (wchar_t)'a';
  b = wctomb(NULL, wc);
  if (b == 0)
    state_dependent = 0;
  else
    state_dependent = 1;
  for (i = 0; i < wsize; i++) {
    if (sizeof(wchar_t) < 4 && wide_string[i] > 0xFFFF) {
      /* wchar_t probably 2 bytes */
      /* could do surrogates if state_dependent and wctomb can do */
      wc = zwchar_to_wchar_t_default_char;
    } else {
      wc = (wchar_t)wide_string[i];
    }
    b = wctomb(buf, wc);
    if (escape_all) {
      if (b == 1 && (uch)buf[0] <= 0x7f) {
        /* ASCII */
        strncat(buffer, buf, b);
      } else {
        /* use escape for wide character */
        char *escape_string = wide_to_escape_string(wide_string[i]);
        strcat(buffer, escape_string);
        izu_free(escape_string);
      }
    } else if (b > 0) {
      /* multi-byte char */
      strncat(buffer, buf, b);
    } else {
      /* no MB for this wide */
        /* use escape for wide character */
        char *escape_string = wide_to_escape_string(wide_string[i]);
        strcat(buffer, escape_string);
        izu_free(escape_string);
    }
  }
  if ((local_string = (char *)izu_malloc(strlen(buffer) + 1)) != NULL) {
    strcpy(local_string, buffer);
  }
  izu_free(buffer);

  return local_string;
}
#  endif /* ndef WIN32 */

#if 0 /* currently unused */
/* convert local string to display character set string */
char *local_to_display_string(local_string)
  ZCONST char *local_string;
{
  char *display_string;

  /* For Windows, OEM string should never be bigger than ANSI string, says
     CharToOem description.
     For all other ports, just make a copy of local_string.
  */
  if ((display_string = (char *)izu_malloc(
   strlen(local_string) + 1)) == NULL) {
    return NULL;
  }

  strcpy(display_string, local_string);

#  ifdef EBCDIC
  {
    char *ebc;

    if ((ebc = izu_malloc(strlen(display_string) + 1)) ==  NULL) {
      return NULL;
    }
    strtoebc(ebc, display_string);
    izu_free(display_string);
    display_string = ebc;
  }
#  endif

  return display_string;
}
#endif /* 0 */

/* UTF-8 to local */
char *utf8_to_local_string(utf8_string, escape_all)
  ZCONST char *utf8_string;
  int escape_all;
{
  zwchar *wide = utf8_to_wide_string(utf8_string);
  char *loc = wide_to_local_string(wide, escape_all);
  izu_free(wide);
  return loc;
}

wchar_t *wide_to_wchar_string(wide_string)
  zwchar *wide_string;
{
  wchar_t *wstring;
  int i;
  int zwlen;

  for (zwlen = 0; wide_string[zwlen]; zwlen++) ;

  if ((wstring = izu_malloc((zwlen + 1) * sizeof(wchar_t))) == NULL) {
    return NULL;
  }

  for (i = 0; wide_string[i]; i++) {
    wstring[i] = (wchar_t)wide_string[i];
  }
  wstring[i] = (wchar_t)0;

  return wstring;
}

zwchar *wchar_to_wide_string(wchar_string)
  wchar_t *wchar_string;
{
  zwchar *zwstring;
  int i;
  int wlen;

  for (wlen = 0; wchar_string[wlen]; wlen++) ;

  if ((zwstring = izu_malloc((wlen + 1) * sizeof(zwchar))) == NULL) {
    return NULL;
  }

  for (i = 0; wchar_string[i]; i++) {
    zwstring[i] = (zwchar)wchar_string[i];
  }
  zwstring[i] = (zwchar)0;

  return zwstring;
}


/* convert multi-byte character string to wide character string */
zwchar *local_to_wide_string(local_string)
  ZCONST char *local_string;
{
  size_t wsize;
  wchar_t *wc_string;
  zwchar *wide_string;

  /* for now try to convert as string - fails if a bad char in string */
  wsize = mbstowcs(NULL, local_string, strlen(local_string) + 1);
  if (wsize == (size_t)-1) {
    /* could not convert */
    return NULL;
  }

  /* convert it */
  if ((wc_string = (wchar_t *)izu_malloc(
   (wsize + 1) * sizeof(wchar_t))) == NULL) {
    return NULL;
  }
  wsize = mbstowcs(wc_string, local_string, strlen(local_string) + 1);
  wc_string[wsize] = (wchar_t) 0;

  /* in case wchar_t is not zwchar */
  if ((wide_string = (zwchar *)izu_malloc(
   (wsize + 1) * sizeof(zwchar))) == NULL) {
    izu_free( wc_string);
    return NULL;
  }
  for (wsize = 0; (wide_string[wsize] = (zwchar)wc_string[wsize]); wsize++) ;
  wide_string[wsize] = (zwchar) 0;
  izu_free(wc_string);

  return wide_string;
}

#if 0 /* currently unused */

/* convert wide string to UTF-8 */
char *wide_to_utf8_string(wide_string)
  ZCONST zwchar *wide_string;
{
  int mbcount;
  char *utf8_string;

  /* get size of utf8 string */
  mbcount = ucs4_string_to_utf8(wide_string, NULL, 0);
  if (mbcount == -1)
    return NULL;
  if ((utf8_string = (char *) izu_malloc(mbcount + 1)) == NULL) {
    return NULL;
  }
  mbcount = ucs4_string_to_utf8(wide_string, utf8_string, mbcount + 1);
  if (mbcount == -1)
    return NULL;

  return utf8_string;
}

zwchar *wchar_to_wide_string(wchar_string)
  wchar_t *wchar_string;
{
  int i;
  int wchar_len;
  zwchar *wide_string;

  wchar_len = wcslen(wchar_string);

  if ((wide_string = izu_malloc((wchar_len + 1) * sizeof(zwchar))) == NULL) {
    return NULL;
  }
  for (i = 0; i <= wchar_len; i++) {
    wide_string[i] = wchar_string[i];
  }

  return wide_string;
}

#endif /* 0 */


#  if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)

char *wchar_to_local_string(wchar_string, escape_all)
  wchar_t *wchar_string;
  int escape_all;
{
  zwchar *wide_string = wchar_to_wide_string(wchar_string);
  char *local_string = wide_to_local_string(wide_string, escape_all);

  izu_free(wide_string);

  return local_string;
}

#  endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */


/* convert UTF-8 string to wide string */
zwchar *utf8_to_wide_string(utf8_string)
  ZCONST char *utf8_string;
{
  int wcount;
  zwchar *wide_string;

  wcount = utf8_to_ucs4_string(utf8_string, NULL, 0);
  if (wcount == -1)
    return NULL;
  if ((wide_string = (zwchar *) izu_malloc((wcount + 1) * sizeof(zwchar)))
      == NULL) {
    return NULL;
  }
  wcount = utf8_to_ucs4_string(utf8_string, wide_string, wcount + 1);

  return wide_string;
}

# endif /* def UNICODE_WCHAR */
#endif /* def UNICODE_SUPPORT */



#ifdef USE_EF_UT_TIME

# ifdef IZ_HAVE_UXUIDGID
static int read_ux3_value(dbuf, uidgid_sz, p_uidgid)
    ZCONST uch *dbuf;   /* buffer a uid or gid value */
    unsigned uidgid_sz; /* size of uid/gid value */
    ulg *p_uidgid;      /* return storage: uid or gid value */
{
    zusz_t uidgid64;

    switch (uidgid_sz) {
      case 2:
        *p_uidgid = (ulg)makeword(dbuf);
        break;
      case 4:
        *p_uidgid = (ulg)makelong(dbuf);
        break;
      case 8:
        uidgid64 = makeint64(dbuf);
#  ifndef LARGE_FILE_SUPPORT
        if (uidgid64 == (zusz_t)0xffffffffL)
            return FALSE;
#  endif
        *p_uidgid = (ulg)uidgid64;
        if ((zusz_t)(*p_uidgid) != uidgid64)
            return FALSE;
        break;
    }
    return TRUE;
}
# endif /* IZ_HAVE_UXUIDGID */


/*******************************/
/* Function ef_scan_for_izux() */
/*******************************/

/* 2012-11-25 SMS.  (OUSPG report.)
 * See note at getZip64Data().
 */

unsigned ef_scan_for_izux(ef_buf, ef_len, ef_is_c, dos_mdatetime,
                          z_utim, z_uidgid)
    ZCONST uch *ef_buf; /* buffer containing extra field */
    long ef_len;        /* total length of extra field */
    int ef_is_c;        /* flag indicating "is central extra field" */
    ulg dos_mdatetime;  /* last_mod_file_date_time in DOS format */
    iztimes *z_utim;    /* return storage: atime, mtime, ctime */
    ulg *z_uidgid;      /* return storage: uid and gid */
{
    unsigned flags = 0;
    unsigned eb_id;
    long eb_len;
    int have_new_type_eb = 0;
    long i_time;        /* buffer for Unix style 32-bit integer time value */
# ifdef TIME_T_TYPE_DOUBLE
    int ut_in_archive_sgn = 0;
# else
    int ut_zip_unzip_compatible = FALSE;
# endif

/*---------------------------------------------------------------------------
 *    This function scans the extra field for the following blocks,
 * containing Unix-compatible (time_t, UTC) file times and/or UID/GID.
 *
 * EF_PKUNIX  0x000d)     : 32-bit atime, mtime; 16-bit UID, GID;
 *                          other, optional data (which we ignore?).
 * EF_IZUNIX  0x5855 "UX"): 32-bit atime, mtime; 16-bit UID, GID.
 *                          (Same as fixed part of EF_PKUNIX.)
 * EF_IZUNIX2 0x7855 "Ux"): 16-bit UID, GID.  (No data in cent. dir.)
 * EF_IZUNIX3 0x7875 "ux"): Variable-length UID, GID.
 * EF_TIME    0x5455 "UT"): flags, 32-bit mtime?, atime?, ctime?.
 *                          (Only mtime in central directory.)
 *
 *    If a valid block is found (and the z_utim pointer is not NULL),
 * then the time stamps are copied to the iztimes structure.
 *    If an IZUNIX2 ("Ux") block is found or the IZUNIX ("UX") or PKUNIX
 * block contains UID/GID fields, and the z_uidgid array pointer is not
 * NULL, then the UID/GID data are also transfered.
 *    The presence of an EF_TIME ("UT"), EF_IZUNIX2 ("Ux"), or
 * EF_IZUNIX3 ("ux") block results in ignoring all data from probably
 * present, obsolete EF_IZUNIX ("UX") blocks.
 *    If multiple blocks of the same type are found, then only the
 * information from the last block is used.
 *    The return value is a combination of the EF_TIME ("UT") Flags
 * field with an additional flag bit (EB_UX2_VALID) indicating the
 * presence of valid UID/GID info (or 0 in case of failure).
  ---------------------------------------------------------------------------*/

    if (ef_len == 0 || ef_buf == NULL || (z_utim == 0 && z_uidgid == NULL))
        return 0;

    TTrace((stderr,
     "\nef_scan_for_izux: scanning extra field of length %ld\n", ef_len));

    while (ef_len >= EB_HEADSIZE) {
        eb_id = makeword(EB_ID + ef_buf);
        eb_len = makeword(EB_LEN + ef_buf);

        if (eb_len > (ef_len - EB_HEADSIZE)) {
            /* discovered some extra field inconsistency! */
            TTrace((stderr,
             "ef_scan_for_izux: block length %ld > rest ef_size %ld\n",
             eb_len, ef_len - EB_HEADSIZE));
            break;
        }

        switch (eb_id) {
          case EF_TIME:         /* "UT" */
            flags &= ~EB_UT_FL_TIMES;   /* Ignore any previous times. */
            have_new_type_eb = 1;
            if ( eb_len >= EB_UT_MINLEN && z_utim != NULL) {
                unsigned eb_idx = EB_UT_TIME1;
                TTrace((stderr, "ef_scan_for_izux: found TIME extra field\n"));
                flags |= (ef_buf[EB_HEADSIZE+EB_UT_FLAGS] & EB_UT_FL_TIMES);
                if ((flags & EB_UT_FL_MTIME)) {
                    if ((long)(eb_idx+4) <= eb_len) {
                        i_time = (long)makelong((EB_HEADSIZE+eb_idx) + ef_buf);
                        eb_idx += 4;
                        TTrace((stderr, "  UT e.f. modification time = %ld\n",
                         i_time));

# ifdef TIME_T_TYPE_DOUBLE
                        if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                            if (dos_mdatetime == DOSTIME_MINIMUM) {
                              ut_in_archive_sgn = -1;
                              z_utim->mtime =
                                (time_t)((long)i_time | (~(long)0x7fffffffL));
                            } else if (dos_mdatetime >= DOSTIME_2038_01_18) {
                              ut_in_archive_sgn = 1;
                              z_utim->mtime =
                                (time_t)((ulg)i_time & (ulg)0xffffffffL);
                            } else {
                              ut_in_archive_sgn = 0;
                              /* cannot determine sign of mtime;
                                 without modtime: ignore complete UT field */
                              flags &= ~EB_UT_FL_TIMES;  /* No times. */
                              TTrace((stderr,
                               "  UT modtime range error; ignore e.f.!\n"));
                              break;            /* stop scanning this field */
                            }
                        } else {
                            /* cannot determine, safe assumption is FALSE */
                            ut_in_archive_sgn = 0;
                            z_utim->mtime = (time_t)i_time;
                        }
# else /* def TIME_T_TYPE_DOUBLE */
                        if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                            ut_zip_unzip_compatible =
                              ((time_t)0x80000000L < (time_t)0L)
                              ? (dos_mdatetime == DOSTIME_MINIMUM)
                              : (dos_mdatetime >= DOSTIME_2038_01_18);
                            if (!ut_zip_unzip_compatible) {
                              /* UnZip interprets mtime differently than Zip;
                                 without modtime: ignore complete UT field */
                              flags &= ~EB_UT_FL_TIMES;  /* No times. */
                              TTrace((stderr,
                               "  UT modtime range error; ignore e.f.!\n"));
                              break;            /* stop scanning this field */
                            }
                        } else {
                            /* cannot determine, safe assumption is FALSE */
                            ut_zip_unzip_compatible = FALSE;
                        }
                        z_utim->mtime = (time_t)i_time;
# endif /* def TIME_T_TYPE_DOUBLE [else] */
                    } else {
                        flags &= ~EB_UT_FL_MTIME;
                        TTrace((stderr, "  UT e.f. truncated; no modtime\n"));
                    }
                }
                if (ef_is_c) {
                    break;      /* central version of TIME field ends here */
                }

                if (flags & EB_UT_FL_ATIME) {
                    if ((long)(eb_idx+4) <= eb_len) {
                        i_time = (long)makelong((EB_HEADSIZE+eb_idx) + ef_buf);
                        eb_idx += 4;
                        TTrace((stderr, "  UT e.f. access time = %ld\n",
                         i_time));
# ifdef TIME_T_TYPE_DOUBLE
                        if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                            if (ut_in_archive_sgn == -1)
                              z_utim->atime =
                                (time_t)((long)i_time | (~(long)0x7fffffffL));
                            } else if (ut_in_archive_sgn == 1) {
                              z_utim->atime =
                                (time_t)((ulg)i_time & (ulg)0xffffffffL);
                            } else {
                              /* sign of 32-bit time is unknown -> ignore it */
                              flags &= ~EB_UT_FL_ATIME;
                              TTrace((stderr,
                               "  UT access time range error: skip time!\n"));
                            }
                        } else {
                            z_utim->atime = (time_t)i_time;
                        }
# else /* def TIME_T_TYPE_DOUBLE */
                        if (((ulg)(i_time) & (ulg)(0x80000000L)) &&
                            !ut_zip_unzip_compatible) {
                            flags &= ~EB_UT_FL_ATIME;
                            TTrace((stderr,
                             "  UT access time range error: skip time!\n"));
                        } else {
                            z_utim->atime = (time_t)i_time;
                        }
# endif /* def TIME_T_TYPE_DOUBLE [else] */
                    } else {
                        flags &= ~EB_UT_FL_ATIME;
                    }
                }
                if (flags & EB_UT_FL_CTIME) {
                    if ((long)(eb_idx+4) <= eb_len) {
                        i_time = (long)makelong((EB_HEADSIZE+eb_idx) + ef_buf);
                        TTrace((stderr, "  UT e.f. creation time = %ld\n",
                         i_time));
# ifdef TIME_T_TYPE_DOUBLE
                        if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                            if (ut_in_archive_sgn == -1)
                              z_utim->ctime =
                                (time_t)((long)i_time | (~(long)0x7fffffffL));
                            } else if (ut_in_archive_sgn == 1) {
                              z_utim->ctime =
                                (time_t)((ulg)i_time & (ulg)0xffffffffL);
                            } else {
                              /* sign of 32-bit time is unknown -> ignore it */
                              flags &= ~EB_UT_FL_CTIME;
                              TTrace((stderr,
                               "  UT creation time range error: skip time!\n"));
                            }
                        } else {
                            z_utim->ctime = (time_t)i_time;
                        }
# else /* def TIME_T_TYPE_DOUBLE */
                        if (((ulg)(i_time) & (ulg)(0x80000000L)) &&
                            !ut_zip_unzip_compatible) {
                            flags &= ~EB_UT_FL_CTIME;
                            TTrace((stderr,
                             "  UT creation time range error: skip time!\n"));
                        } else {
                            z_utim->ctime = (time_t)i_time;
                        }
# endif /* def TIME_T_TYPE_DOUBLE [else] */
                    } else {
                        flags &= ~EB_UT_FL_CTIME;
                    }
                }
            }
            break;

          case EF_IZUNIX2:      /* "Ux", 16-bit UID, GID. */
            if (have_new_type_eb == 0) {        /* (< 1) */
                have_new_type_eb = 1;
            }
            if (have_new_type_eb <= 1) {
                /* Ignore any prior (EF_IZUNIX/EF_PKUNIX) UID/GID. */
                flags &= EB_UT_FL_TIMES;
            }
# ifdef IZ_HAVE_UXUIDGID
            if (have_new_type_eb > 1)
                break;          /* IZUNIX3 overrides IZUNIX2 e.f. block ! */
            if (eb_len == EB_UX2_MINLEN && z_uidgid != NULL) {
                z_uidgid[0] = (ulg)makeword((EB_HEADSIZE+EB_UX2_UID) + ef_buf);
                z_uidgid[1] = (ulg)makeword((EB_HEADSIZE+EB_UX2_GID) + ef_buf);
                flags |= EB_UX2_VALID;   /* signal success */
            }
# endif
            break;

          case EF_IZUNIX3:      /* "ux", Variable-length UID, GID. */
            /* new 3rd generation Unix ef */
            have_new_type_eb = 2;       /* (Maximum newness, so no tests.) */
            /* Ignore any prior EF_IZUNIX/EF_PKUNIX/EF_IZUNIX2 UID/GID. */
            flags &= EB_UT_FL_TIMES;
        /*
          Version       1 byte      version of this extra field, currently 1
          UIDSize       1 byte      Size of UID field (min = 2)
          UID           Variable    UID for this entry
          GIDSize       1 byte      Size of GID field (min = 2)
          GID           Variable    GID for this entry
        */

# ifdef IZ_HAVE_UXUIDGID
            /* Check for a legitimate extra block length, a non-NULL
             * destination pointer, and "ux" version 1 (which is all we
             * understand).
             */
            if ((eb_len >= EB_UX3_MINLEN) &&
             (z_uidgid != NULL) &&
             (*((EB_HEADSIZE + 0) + ef_buf) == 1))
            {
                /* 2012-12-07 SMS.  (OUSPG report.)
                 * First, clear "flags".  Then, check the validity of
                 * uid_size before using it to find gid_size.
                 * Made Xid_size bigger than "uch" for safer arithmetic.
                 */
                unsigned uid_size;
                unsigned gid_size;

                uid_size = *((EB_HEADSIZE + 1) + ef_buf);

                /* Valid: 1 (Version) + 1 (UIDSize) + UIDSize +
                 *        1 (GIDSize) + 2 (min GIDSize) <= eb_len.
                 */
                if (5+ uid_size <= eb_len)
                {
                    gid_size = *((EB_HEADSIZE + uid_size + 2) + ef_buf);

                    /* Last, check total claimed xID sizes against eb_len. */
                    if (3+ uid_size+ gid_size == eb_len)
                    {
                        if (read_ux3_value( (EB_HEADSIZE + 2) + ef_buf,
                         uid_size, &z_uidgid[0]) &&
                         read_ux3_value( (EB_HEADSIZE + uid_size + 3) + ef_buf,
                         gid_size, &z_uidgid[1]))
                        {
                        flags |= EB_UX2_VALID;   /* signal success */
                        }
                    }
                }
            }
# endif /* def IZ_HAVE_UXUIDGID */
            break;

          case EF_IZUNIX:       /* "UX", atime, mtime; 16-bit UID, GID. */
          case EF_PKUNIX:       /* 0x000d (Same layout as IZUNIX) */
            if (eb_len >= EB_UX_MINLEN) {
                TTrace((stderr, "ef_scan_for_izux: found %s extra field\n",
                 (eb_id == EF_IZUNIX ? "IZUNIX" : "PKUNIX")));
                if (have_new_type_eb > 0) {
                    break;      /* Ignore IZUNIX extra field block ! */
                }
                if (z_utim != NULL) {
                    flags |= (EB_UT_FL_MTIME | EB_UT_FL_ATIME);
                    i_time = (long)makelong((EB_HEADSIZE+EB_UX_MTIME)+ef_buf);
                    TTrace((stderr, "  Unix EF modtime = %ld\n", i_time));
# ifdef TIME_T_TYPE_DOUBLE
                    if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                        if (dos_mdatetime == DOSTIME_MINIMUM) {
                            ut_in_archive_sgn = -1;
                            z_utim->mtime =
                              (time_t)((long)i_time | (~(long)0x7fffffffL));
                        } else if (dos_mdatetime >= DOSTIME_2038_01_18) {
                            ut_in_archive_sgn = 1;
                            z_utim->mtime =
                              (time_t)((ulg)i_time & (ulg)0xffffffffL);
                        } else {
                            ut_in_archive_sgn = 0;
                            /* cannot determine sign of mtime;
                               without modtime: ignore complete UT field */
                            flags &= ~EB_UT_FL_TIMES;   /* No times. */
                            TTrace((stderr,
                             "  UX modtime range error: ignore e.f.!\n"));
                        }
                    } else {
                        /* cannot determine, safe assumption is FALSE */
                        ut_in_archive_sgn = 0;
                        z_utim->mtime = (time_t)i_time;
                    }
# else /* def TIME_T_TYPE_DOUBLE */
                    if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                        ut_zip_unzip_compatible =
                          ((time_t)0x80000000L < (time_t)0L)
                          ? (dos_mdatetime == DOSTIME_MINIMUM)
                          : (dos_mdatetime >= DOSTIME_2038_01_18);
                        if (!ut_zip_unzip_compatible) {
                            /* UnZip interpretes mtime differently than Zip;
                               without modtime: ignore complete UT field */
                            flags &= ~EB_UT_FL_TIMES;   /* No times. */
                            TTrace((stderr,
                             "  UX modtime range error: ignore e.f.!\n"));
                        }
                    } else {
                        /* cannot determine, safe assumption is FALSE */
                        ut_zip_unzip_compatible = FALSE;
                    }
                    z_utim->mtime = (time_t)i_time;
# endif /* def TIME_T_TYPE_DOUBLE [else] */
                    i_time = (long)makelong((EB_HEADSIZE+EB_UX_ATIME)+ef_buf);
                    TTrace((stderr, "  Unix EF actime = %ld\n", i_time));
# ifdef TIME_T_TYPE_DOUBLE
                    if ((ulg)(i_time) & (ulg)(0x80000000L)) {
                        if (ut_in_archive_sgn == -1)
                            z_utim->atime =
                              (time_t)((long)i_time | (~(long)0x7fffffffL));
                        } else if (ut_in_archive_sgn == 1) {
                            z_utim->atime =
                              (time_t)((ulg)i_time & (ulg)0xffffffffL);
                        } else if (flags & EB_UT_FL_TIMES) {
                            /* sign of 32-bit time is unknown -> ignore it */
                            flags &= ~EB_UT_FL_ATIME;
                            TTrace((stderr,
                             "  UX access time range error: skip time!\n"));
                        }
                    } else {
                        z_utim->atime = (time_t)i_time;
                    }
# else /* def TIME_T_TYPE_DOUBLE */
                    if (((ulg)(i_time) & (ulg)(0x80000000L)) &&
                        !ut_zip_unzip_compatible && (flags & EB_UT_FL_TIMES)) {
                        /* atime not in range of UnZip's time_t */
                        flags &= ~EB_UT_FL_ATIME;
                        TTrace((stderr,
                         "  UX access time range error: skip time!\n"));
                    } else {
                        z_utim->atime = (time_t)i_time;
                    }
# endif /* def TIME_T_TYPE_DOUBLE [else] */
                }
# ifdef IZ_HAVE_UXUIDGID
                if (eb_len >= EB_UX_FULLSIZE && z_uidgid != NULL) {
                    z_uidgid[0] = makeword((EB_HEADSIZE+EB_UX_UID) + ef_buf);
                    z_uidgid[1] = makeword((EB_HEADSIZE+EB_UX_GID) + ef_buf);
                    flags |= EB_UX2_VALID;
                }
# endif /* IZ_HAVE_UXUIDGID */
            }
            break;

          default:
            break;
        }

        /* Skip this extra field block */
        ef_buf += (eb_len + EB_HEADSIZE);
        ef_len -= (eb_len + EB_HEADSIZE);
    }

    return flags;
} /* ef_scan_for_izux(). */

#endif /* USE_EF_UT_TIME */


#if (defined(RISCOS) || defined(ACORN_FTYPE_NFS))

# define SPARKID_2 0x30435241   /* = "ARC0" */

/*******************************/
/* Function getRISCOSexfield() */
/*******************************/

/* 2012-11-25 SMS.  (OUSPG report.)
 * See note at getZip64Data().
 */

zvoid *getRISCOSexfield(ef_buf, ef_len)
    ZCONST uch *ef_buf; /* buffer containing extra field */
    long ef_len;        /* total length of extra field */
{
    unsigned eb_id;
    long eb_len;

/*---------------------------------------------------------------------------
    This function scans the extra field for a Acorn SPARK filetype ef-block.
    If a valid block is found, the function returns a pointer to the start
    of the SPARK_EF block in the extra field buffer.  Otherwise, a NULL
    pointer is returned.
  ---------------------------------------------------------------------------*/

    if (ef_len == 0 || ef_buf == NULL)
        return NULL;

    Trace((stderr,
     "\ngetRISCOSexfield: scanning extra field of length %ld\n", ef_len));

    while (ef_len >= EB_HEADSIZE) {
        eb_id = makeword(EB_ID + ef_buf);
        eb_len = makeword(EB_LEN + ef_buf);

        if (eb_len > (ef_len - EB_HEADSIZE)) {
            /* discovered some extra field inconsistency! */
            Trace((stderr,
             "getRISCOSexfield: block length %ld > rest ef_size %ld\n",
             eb_len, ef_len - EB_HEADSIZE));
            break;
        }

        if (eb_id == EF_SPARK && (eb_len == 24 || eb_len == 20)) {
            if (makelong(EB_HEADSIZE + ef_buf) == SPARKID_2) {
                /* Return a pointer to the valid SPARK filetype ef block */
                return (zvoid *)ef_buf;
            }
        }

        /* Skip this extra field block */
        ef_buf += (eb_len + EB_HEADSIZE);
        ef_len -= (eb_len + EB_HEADSIZE);
    }

    return NULL;
} /* getRISCOSexfield(). */

#endif /* (RISCOS || ACORN_FTYPE_NFS) */
