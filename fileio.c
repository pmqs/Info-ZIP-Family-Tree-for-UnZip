/*
  Copyright (c) 1990-2015 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  fileio.c

  This file contains routines for doing direct but relatively generic input/
  output, file-related sorts of things, plus some miscellaneous stuff.  Most
  of the stuff has to do with opening, closing, reading and/or writing files.

  Contains:  open_input_file()
             open_outfile()           (not: VMS, AOS/VS, CMSMVS, MACOS, TANDEM)
             undefer_input()
             defer_leftover_input()
             readbuf()
             readbyte()
             fillinbuf()
             seek_zipf()
             fgets_ans()
             flush()                  (non-VMS)
             is_vms_varlen_txt()      (non-VMS, VMS_TEXT_CONV only)
             disk_error()             (non-VMS)
             UzpMessagePrnt()
             UzpMessageNull()         (DLL only)
             UzpInput()
             UzpMorePause()
             UzpPassword()            (non-WINDLL)
             handler()
             dos_to_unix_time()       (non-VMS, non-VM/CMS, non-MVS)
             check_for_newer()        (non-VMS, non-OS/2, non-VM/CMS, non-MVS)
             do_string()
             name_only()              (non-VMS, ...?)
             makeword()
             makelong()
             makeint64()
             fzofft()
             str2iso()                (CRYPT && NEED_STR2ISO, only)
             str2oem()                (CRYPT && NEED_STR2OEM, only)
             memset()                 (ZMEM only)
             memcpy()                 (ZMEM only)
             zstrnicmp()              (NO_STRNICMP only)
             zstat()                  (REGULUS only)
             plastchar()              (_MBCS only)
             uzmbclen()               (_MBCS && NEED_UZMBCLEN, only)
             uzmbschr()               (_MBCS && NEED_UZMBSCHR, only)
             uzmbsrchr()              (_MBCS && NEED_UZMBSRCHR, only)
             fLoadFarString()         (SMALL_MEM only)
             fLoadFarStringSmall()    (SMALL_MEM only)
             fLoadFarStringSmall2()   (SMALL_MEM only)
             zfstrcpy()               (SMALL_MEM only)
             zfstrcmp()               (SMALL_MEM && !(SFX || FUNZIP) only)

  ---------------------------------------------------------------------------*/


#define __FILEIO_C      /* identifies this source module */
#define UNZIP_INTERNAL
#include "unzip.h"
#ifdef DLL
# include "api.h"
#endif /* def DLL */
#ifdef WINDLL
# ifdef POCKET_UNZIP
#  include "wince/intrface.h"
# else
#  include "windll/windll.h"
# endif
# include <setjmp.h>
#endif
#include "crc32.h"
#include "crypt.h"
#include "ttyio.h"

#if defined( UNIX) && defined( __APPLE__)
# include "unix/macosx.h"
#endif /* defined( UNIX) && defined( __APPLE__) */

/* setup of codepage conversion for decryption passwords */
#ifdef IZ_CRYPT_ANY
# if (defined(CRYP_USES_ISO2OEM) && !defined(IZ_ISO2OEM_ARRAY))
#  define IZ_ISO2OEM_ARRAY            /* pull in iso2oem[] table */
# endif
# if (defined(CRYP_USES_OEM2ISO) && !defined(IZ_OEM2ISO_ARRAY))
#  define IZ_OEM2ISO_ARRAY            /* pull in oem2iso[] table */
# endif
#endif /* def IZ_CRYPT_ANY */
#include "ebcdic.h"   /* definition/initialization of ebcdic[] */


/*
   Note: Under Windows, the maximum size of the buffer that can be used
   with any of the *printf calls is 16,384, so win_fprintf was used to
   feed the fprintf clone no more than 16K chunks at a time. This should
   be valid for anything up to 64K (and probably beyond, assuming your
   buffers are that big).
*/
#ifdef WINDLL
# define WriteError(buf,len,strm) \
   (win_fprintf(pG, (strm), (int)(len), (char far *)(buf)) != (len))
#else /* !WINDLL */
# ifdef USE_FWRITE
#  define WriteError(buf,len,strm) \
    (fwrite((char *)(buf), 1, (len), (strm)) != (len))
# else
#  define WriteError(buf,len,strm) \
    (write(fileno(strm), (char *)(buf), (int)(len)) != (len))
# endif
#endif /* ?WINDLL */

/*
   2005-09-16 SMS.
   On VMS, when output is redirected to a file, as in a command like
   "PIPE UNZIP -v > X.OUT", the output file is created with VFC record
   format, and multiple calls to write() or fwrite() will produce multiple
   records, even when there's no newline terminator in the buffer.
   The result is unsightly output with spurious newlines.  Using fprintf()
   instead of write() here, and disabling a fflush(stdout) in UzpMessagePrnt()
   below, together seem to solve the problem.

   According to the C RTL manual, "The write and decc$record_write
   functions always generate at least one record."  Also, "[T]he fwrite
   function always generates at least <number_items> records."  So,
   "fwrite(buf, len, 1, strm)" is much better ("1" record) than
   "fwrite(buf, 1, len, strm)" ("len" (1-character) records, _really_
   ugly), but neither is better than write().  Similarly, "The fflush
   function always generates a record if there is unwritten data in the
   buffer."  Apparently fprintf() buffers the stuff somewhere, and puts
   out a record (only) when it sees a newline.
*/
#ifdef VMS
# define WriteTxtErr(buf,len,strm) \
   ((extent)fprintf(strm, "%.*s", len, buf) != (extent)(len))
#else
# define WriteTxtErr(buf,len,strm)  WriteError(buf,len,strm)
#endif

#if (defined(DEFLATE64_SUPPORT) && defined(__16BIT__))
static int partflush OF((__GPRO__ uch *rawbuf, ulg size, int unshrink));
#endif
#ifdef VMS_TEXT_CONV
static int is_vms_varlen_txt OF((__GPRO__ uch *ef_buf, long ef_len));
#endif
static int disk_error OF((__GPRO));


/****************************/
/* Strings used in fileio.c */
/****************************/

static ZCONST char Far CannotOpenZipfile[] =
  "error:  cannot open zipfile [ %s ]\n        %s\n";

static ZCONST char Far NoSuchSegment[] =
 "Bad archive segment value (%d > %d)";

#if !defined(VMS) && !defined(AOS_VS) && !defined(CMS_MVS) && !defined(MACOS)
# ifndef TANDEM
#  if defined(ATH_BEO_THS_UNX) || defined(DOS_FLX_NLM_OS2_W32)
static ZCONST char Far CannotDeleteOldFile[] =
 "error:  cannot delete old %s\n        %s\n";
#   ifdef UNIXBACKUP
static ZCONST char Far CannotRenameOldFile[] =
 "error:  cannot rename old %s\n        %s\n";
static ZCONST char Far BackupSuffix[] = "~";
#   endif /* def UNIXBACKUP */
#  endif /* defined(ATH_BEO_THS_UNX) || defined(DOS_FLX_NLM_OS2_W32) */
#  ifdef NOVELL_BUG_FAILSAFE
static ZCONST char Far NovellBug[] =
 "error:  %s: stat() says does not exist, but fopen() found anyway\n";
#  endif /* def NOVELL_BUG_FAILSAFE */
static ZCONST char Far CannotCreateFile[] =
 "error:  cannot create %s\n        %s\n";
# endif /* ndef TANDEM */
#endif /* !defined(VMS) && !defined(AOS_VS) && !defined(CMS_MVS) && !defined(MACOS) */

static ZCONST char Far ReadError[] = "error:  zipfile read error\n";
static ZCONST char Far FilenameTooLongTrunc[] =
  "warning:  filename too long--truncating.\n";

#ifdef UNICODE_SUPPORT
static ZCONST char Far UFilenameTooLongTrunc[] =
 "warning:  Converted unicode filename too long--truncating.\n";
#endif /* def UNICODE_SUPPORT */

static ZCONST char Far ExtraFieldCorrupt[] =
 "warning:  extra field (type: 0x%04x) corrupt.  Continuing...\n";
static ZCONST char Far ExtraFieldTooLong[] =
 "warning:  extra field too long (%d).  Ignoring...\n";
static ZCONST char Far DiskFullMsg[] =
 "%s:  write error (disk full?).\n";

#ifdef SYMLINKS
static ZCONST char Far FileIsSymLink[] =
 "%s exists and is a symbolic link%s.\n";
#endif /* def SYMLINKS */

#if !defined( WINDLL) && defined( I_O_ERROR_QUERY) && defined( STDIN_ISATTY)
# define USE_I_O_ERROR_QUERY
#endif

#ifdef USE_I_O_ERROR_QUERY
static ZCONST char Far DiskFullQuery[] =
 "%s:  write error (disk full?).  Continue? (y/n/^C) ";
#endif /* def USE_I_O_ERROR_QUERY */

#ifndef WINDLL
static ZCONST char Far ZipfileCorrupt[] =
 "error:  zipfile probably corrupt (%s)\n";
# ifdef MORE
static ZCONST char Far MorePrompt[] = "--More--(%lu)";
# endif /* def MORE */
static ZCONST char Far QuitPrompt[] =
 "--- Press `Q' to quit, or any other key to continue ---";
static ZCONST char Far HidePrompt[] = /* (Match max prompt length.) */
 "\r                                                         \r";
# ifdef IZ_CRYPT_ANY
#  ifdef MACOS
       /* SPC: are names on MacOS REALLY so much longer than elsewhere ??? */
static ZCONST char Far PasswPrompt[] = "[%s]\n %s password: ";
#  else /* def MACOS */
static ZCONST char Far PasswPrompt[] = "[%s] %s password: ";
#  endif /* def MACOS [else] */
static ZCONST char Far PasswPrompt2[] = "Enter password: ";
static ZCONST char Far PasswRetry[] = "password incorrect--reenter: ";
# endif /* def IZ_CRYPT_ANY */
#endif /* ndef WINDLL */

#if defined( UNIX) && defined( __APPLE__)
static ZCONST char MemAllocFailed[] =
 "error:  cannot allocate AplDbl attrib memory (loc: %d, bytes: %d).\n";
static ZCONST char SetattrlistFailed[] =
 "\nsetattrlist(fndr) failure (errno = %d): %s";
# ifdef APPLE_XATTR
static ZCONST char SetxattrFailed[] =
 "\nsetxattr() failure (errno = %d): %s";
# endif /* def APPLE_XATTR */
#endif /* defined( UNIX) && defined( __APPLE__) */




/**************************/
/* Function open_infile() */
/**************************/
int open_infile(__G__ which)
  __GDEF
  int which;            /* 0: Primary archive; 1: Segment archive. */
{
  /* Open an archive (zipfile) for reading and in BINARY mode to
   * prevent CR/LF translation, which would corrupt the data.
   * Return 0: success; 1: failure.
   */

  char *fn;
  zipfd_t *pfd;

  if (which == OIF_PRIMARY)
  {
    fn = G.zipfn;               /* Primary archive file name (".zip"). */
    pfd = &G.zipfd;             /* Primary archive file descr/pointer. */
  }
  else
  {
    fn = G.zipfn_sgmnt;         /* Segment archive file name (".zXX"). */
    pfd = &G.zipfd_sgmnt;       /* Segment archive file descr/pointer. */
  }

#ifdef VMS
    *pfd = open( fn, O_RDONLY, 0, OPNZIP_RMS_ARGS);
#else /* def VMS */
# ifdef MACOS
    *pfd = open( fn, 0);
# else /* def MACOS */
#  ifdef CMS_MVS
    *pfd = vmmvs_open_infile(__G fn, pfd);
#  else /* def CMS_MVS */
    if (G.zipstdin)
    {
        /* Archive is stdin.  No need to open it. */
#   ifdef USE_STRM_INPUT
        *pfd = stdin;
#   else /* def USE_STRM_INPUT */
        *pfd = STDIN_FILENO;
#   endif /* def USE_STRM_INPUT */
    }
    else
    {
        /* Archive is plain file (we hope). */
#   ifdef USE_STRM_INPUT
        *pfd = fopen( fn, FOPR);
#   else /* def USE_STRM_INPUT */
        *pfd = open( fn, O_RDONLY | O_BINARY);
#   endif /* def USE_STRM_INPUT */
    }
#  endif /* def CMS_MVS [else] */
# endif /* def MACOS [else] */
#endif /* def VMS [else] */

    if (!fd_is_valid( *pfd))
    {
        Info(slide, 0x401, ((char *)slide, LoadFarString(CannotOpenZipfile),
          fn, strerror(errno)));
        return 1;
    }
    return 0;

} /* open_infile(). */


/***************************/
/* Function close_infile() */
/***************************/
int close_infile( __G__ pfd)
  __GDEF
  zipfd_t *pfd;
{
  int sts;

  if (fd_is_valid( *pfd))
  {
#ifdef USE_STRM_INPUT
    sts = fclose( *pfd);
    *pfd = NULL;
#else /* def USE_STRM_INPUT */
    sts = close( *pfd);
    *pfd = -1;
#endif /* def USE_STRM_INPUT [else] */
  }

  return sts;
} /* close_infile(). */


/***********************************/
/* Function set_zipfn_sgmnt_name() */
/***********************************/
int set_zipfn_sgmnt_name( __G__ sgmnt_nr)
  __GDEF
  zuvl_t sgmnt_nr;
{
  char *suffix;
  int sufx_len;

/* sizeof( ".z65535") == 8 should be safe. */
#define SGMNT_NAME_BOOST 8

  if (sgmnt_nr > G.ecrec.number_this_disk)
  {
    /* Segment number greater than (max?) central-dir disk number. */
    Info(slide, 1, ((char *)slide, LoadFarString(NoSuchSegment),
     sgmnt_nr, G.ecrec.number_this_disk));
    return 1;
  }

  if (G.zipfn_sgmnt == NULL)
  {
    G.zipfn_sgmnt_size = strlen(G.zipfn)+ SGMNT_NAME_BOOST;
    if ((G.zipfn_sgmnt = izu_malloc(G.zipfn_sgmnt_size)) == NULL)
    {
      G.zipfn_sgmnt_size = -1;
      return 1;
    }
  }
  else
  {
    if (G.zipfn_sgmnt_size < (int)strlen(G.zipfn)+ SGMNT_NAME_BOOST)
    {
      G.zipfn_sgmnt_size = strlen(G.zipfn)+ SGMNT_NAME_BOOST;
      izu_free(G.zipfn_sgmnt);
      if ((G.zipfn_sgmnt = izu_malloc(G.zipfn_sgmnt_size)) == NULL)
      {
        G.zipfn_sgmnt_size = -1;
        return 1;
      }
    }
  }


  if (sgmnt_nr == G.ecrec.number_this_disk)
  {
    zfstrcpy(G.zipfn_sgmnt, G.zipfn);
    return 0;           /* Last segment.  Name already ".zip." */
  }

#ifdef VMS

  /* A VMS archive file spec may include a version number (";nnn"),
   * confusing any simple scheme (like the one below).  $PARSE can
   * easily replace one file type with another (and null out the version
   * number), so use it, instead.
   */
  vms_sgmnt_name( G.zipfn_sgmnt, G.zipfn, (sgmnt_nr+ 1)); 

#else /* def VMS */

  zfstrcpy(G.zipfn_sgmnt, G.zipfn);
  /* Expect to find ".zXX" at the end of the segment file name. */
  sufx_len = IZ_MAX( 0, ((int)strlen(G.zipfn_sgmnt)- 4));
  suffix = G.zipfn_sgmnt+ sufx_len;

  /* try find filename extension and set right position for add number */
  if (zfstrcmp(suffix, ZSUFX) == 0)
  {
    suffix += 2;        /* Point to digits after ".z". */
# ifdef ZSUFX2
  }
  else if (zfstrcmp(suffix, ZSUFX2) == 0)       /* Check alternate suffix. */
  {
    suffix[1] = 'z';    /* Should be always lowercase??? */
    suffix += 2;        /* Point to digits after ".z". */
# endif
  }
  else
  {
    zfstrcpy( (suffix+ sufx_len), ZSUFX);
    suffix += sufx_len+ 2;
  }
  /* Insert the next segment number into the file name (G.zipfn_sgmnt). */
  sprintf(suffix, "%02d", (sgmnt_nr+ 1));

#endif /* def VMS [else] */

  return 0;
} /* set_zipfn_sgmnt_name(). */


/********************************/
/* Function open_infile_sgmnt() */
/********************************/
int open_infile_sgmnt(__G__ movement)
  __GDEF
  int movement;
{
  zipfd_t zipfd;
  zipfd_t zipfd_sgmnt;

  if (movement == 0)            /* Nothing to do. */
    return 0;

   zipfd = G.zipfd;
   zipfd_sgmnt = G.zipfd_sgmnt;

  /* Set the new segment file name. */
  if (set_zipfn_sgmnt_name(G.sgmnt_nr+ movement))
    return 1;

  if (open_infile( __G OIF_SEGMENT))
  {
    /* TODO: ask for input and try it again */
    /* error, load back old zipfn (it shouldn't be frequently) */
    if (fd_is_valid(zipfd_sgmnt))
    {
      set_zipfn_sgmnt_name(G.sgmnt_nr);
    }
    else
    {
      /* Loading of central directory record probably??? */
      izu_free(G.zipfn_sgmnt);
      G.zipfn_sgmnt = NULL;
    }

    G.zipfd_sgmnt = zipfd_sgmnt;
    return 1;
  }

  G.sgmnt_nr += movement;

  /* close old file - yes, that's nasty solution */
  /* Switch I/O to the new segment. */
  G.zipfd = G.zipfd_sgmnt;
  G.zipfd_sgmnt = zipfd;                /* Old G.zipfd. */
  CLOSE_INFILE( &G.zipfd_sgmnt);
  if (fd_is_valid(zipfd_sgmnt))         /* Old G.zipfd_sgmnt. */
  {
    G.zipfd_sgmnt = G.zipfd;
  }
  else
  {
    izu_free(G.zipfn_sgmnt);            /* We must clean it now. */
    G.zipfn_sgmnt = NULL;
  }

  return 0;
}


#if !defined(VMS) && !defined(AOS_VS) && !defined(CMS_MVS) && !defined(MACOS)
# if !defined(TANDEM)

/***************************/
/* Function open_outfile() */
/***************************/

int open_outfile(__G)           /* return 1 if fail */
    __GDEF
{
  int r;

#  ifdef DLL
    if (G.redirect_data)
        return (redirect_outfile(__G) == FALSE);
#  endif
#  ifdef QDOS
    QFilename(__G__ G.filename);
#  endif

#  if defined( UNIX) && defined( __APPLE__)
    if (G.apple_double)
    {
        /* Allocate AppleDouble header buffer, if needed. */
        if (G.apl_dbl_hdr_alloc < APL_DBL_SIZE_HDR)
        {
            G.apl_dbl_hdr = izu_realloc( G.apl_dbl_hdr, APL_DBL_SIZE_HDR);
            if (G.apl_dbl_hdr == NULL)
            {
                Info(slide, 1, ((char *)slide, MemAllocFailed,
                 1, APL_DBL_SIZE_HDR));
                return 1;
            }
            G.apl_dbl_hdr_alloc = APL_DBL_SIZE_HDR;
        }

        /* Set flags and byte counts for the AppleDouble header. */
        G.apl_dbl_hdr_len = 0;                  /* Bytes in header. */
        G.apl_dbl_hdr_bytes = APL_DBL_SIZE_HDR; /* Header bytes sought. */

        /* 2013-08-05 SMS.
         * It might be more efficient not to append a "/rsrc" suffix
         * here, and then use fsetxattr( fileno( G.outfile), ...)
         * instead of setxattr( G.filename, ...) to set the extended
         * attributes below.  (And then do something different for the
         * resource fork.)  Or append the "/rsrc" suffix later, after
         * setting the attributes.  Note that XATTR_NOFOLLOW is not used
         * with fsetxattr().
         */
        /* Excise the "._" name prefix from the post-mapname()
         * AppleDouble file name.
         */
        revert_apl_dbl_path( G.filename, G.filename);
        /* Append "/rsrc" suffix to the AppleDouble file name. */
        strcat( G.filename, APL_DBL_SUFX);
    }
    else
    {
        /* Set byte count to bypass AppleDouble processing. */
        G.apl_dbl_hdr_bytes = 0;
    }
#  endif /* defined( UNIX) && defined( __APPLE__) */

#  if defined(DOS_FLX_NLM_OS2_W32) || defined(ATH_BEO_THS_UNX)
#   ifdef BORLAND_STAT_BUG
    /* Borland 5.0's stat() barfs if the filename has no extension and the
     * file doesn't exist. */
    if (access(G.filename, 0) == -1) {
        FILE *tmp = fopen(G.filename, "wb+");

        /* file doesn't exist, so create a dummy file to keep stat() from
         * failing (will be over-written anyway) */
        fputc('0', tmp);  /* just to have something in the file */
        fclose(tmp);
    }
#   endif /* def BORLAND_STAT_BUG */

/* AppleDouble resource fork is expected to exist, so evade the test. */
#   if defined( UNIX) && defined( __APPLE__)
#    define TEST_EXIST (G.apple_double == 0)
#   else /* defined( UNIX) && defined( __APPLE__) */
#    define TEST_EXIST 1
#   endif /* defined( UNIX) && defined( __APPLE__) [else] */

#   if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
    r = ((G.has_win32_wide
          ? SSTATW(G.unipath_widefilename, &G.statbuf)
          : SSTAT(G.filename, &G.statbuf)
         ) == 0);
#   else /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
    r = (SSTAT(G.filename, &G.statbuf) == 0);
#   endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */

#   if defined( SYMLINKS) && !defined( WIN32)
    if (TEST_EXIST && (r || lstat(G.filename, &G.statbuf) == 0))
#   else /* defined( SYMLINKS) && !defined( WIN32) */
    if (TEST_EXIST && r)
#   endif /* defined( SYMLINKS) && !defined( WIN32) [else] */
    {
        Trace((stderr, "open_outfile:  stat(%s) returns 0:  file exists\n",
          FnFilter1(G.filename)));
#   ifdef UNIXBACKUP
        if (uO.B_flag) {    /* do backup */
            char *tname;
            z_stat tmpstat;
            size_t blen;
	    size_t flen;
	    size_t tlen;

            blen = strlen(BackupSuffix);
            flen = strlen(G.filename);
            tlen = flen + blen + 6;    /* includes space for 5 digits */
            if (tlen >= FILNAMSIZ) {   /* in case name is too long, truncate */
                tname = (char *)izu_malloc(FILNAMSIZ);
                if (tname == NULL)
                    return 1;                 /* in case we run out of space */
                tlen = FILNAMSIZ - 1 - blen;
                strcpy(tname, G.filename);    /* make backup name */
                tname[tlen] = '\0';
                if (flen > tlen) flen = tlen;
                tlen = FILNAMSIZ;
            } else {
                tname = (char *)malloc(tlen);
                if (tname == NULL)
                    return 1;                 /* in case we run out of space */
                strcpy(tname, G.filename);    /* make backup name */
            }
            strcpy(tname+flen, BackupSuffix);

            if (IS_OVERWRT_ALL) {
                /* If there is a previous backup file, delete it,
                 * otherwise the following rename operation may fail.
                 */
                if (SSTAT(tname, &tmpstat) == 0)
                    unlink(tname);
            } else {
                /* Check if backupname exists, and, if it's true, try
                 * appending numbers of up to 5 digits (or the maximum
                 * "unsigned int" number on 16-bit systems) to the
                 * BackupSuffix, until an unused name is found.
                 */
                unsigned maxtail, i;
                char *numtail = tname + flen + blen;

                /* take account of the "unsigned" limit on 16-bit systems: */
                maxtail = ( ((~0) >= 99999L) ? 99999 : (~0) );
                switch (tlen - flen - blen - 1) {
                    case 4: maxtail = 9999; break;
                    case 3: maxtail = 999; break;
                    case 2: maxtail = 99; break;
                    case 1: maxtail = 9; break;
                    case 0: maxtail = 0; break;
                }
                /* while filename exists */
                for (i = 0; (i < maxtail) && (SSTAT(tname, &tmpstat) == 0);)
                    sprintf(numtail,"%u", ++i);
            }

            if (rename(G.filename, tname) != 0) {   /* move file */
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(CannotRenameOldFile),
                  FnFilter1(G.filename), strerror(errno)));
                izu_free(tname);
                return 1;
            }
            Trace((stderr, "open_outfile:  %s now renamed into %s\n",
              FnFilter1(G.filename), FnFilter2(tname)));
            izu_free(tname);
        } else
#   endif /* def UNIXBACKUP */
        {
#   ifdef DOS_FLX_OS2_W32
            if (!(G.statbuf.st_mode & S_IWRITE)) {
                Trace((stderr,
                  "open_outfile:  existing file %s is read-only\n",
                  FnFilter1(G.filename)));
                chmod(G.filename, S_IREAD | S_IWRITE);
                Trace((stderr, "open_outfile:  %s now writable\n",
                  FnFilter1(G.filename)));
            }
#   endif /* def DOS_FLX_OS2_W32 */
#   ifdef NLM
            /* Give the file read/write permission (non-POSIX shortcut) */
            chmod(G.filename, 0);
#   endif /* def NLM */
#   if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
            if ((G.has_win32_wide
                 ? _wunlink(G.unipath_widefilename)
                 : unlink(G.filename)
                ) != 0)
#   else /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
            if (unlink(G.filename) != 0)
#   endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */
            {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(CannotDeleteOldFile),
                  FnFilter1(G.filename), strerror(errno)));
                return 1;
            }
            Trace((stderr, "open_outfile:  %s now deleted\n",
              FnFilter1(G.filename)));
        }
    }
#  endif /* defined(DOS_FLX_NLM_OS2_W32) || defined(ATH_BEO_THS_UNX) */
#  ifdef RISCOS
    if (SWI_OS_File_7(G.filename,0xDEADDEAD,0xDEADDEAD,G.lrec.ucsize)!=NULL) {
        Info(slide, 1, ((char *)slide, LoadFarString(CannotCreateFile),
          FnFilter1(G.filename), strerror(errno)));
        return 1;
    }
#  endif /* def RISCOS */
#  ifdef TOPS20
    char *tfilnam;

    if ((tfilnam = (char *)malloc(2*strlen(G.filename)+1)) == (char *)NULL)
        return 1;
    strcpy(tfilnam, G.filename);
    upper(tfilnam);
    enquote(tfilnam);
    if ((G.outfile = fopen(tfilnam, FOPW)) == (FILE *)NULL) {
        Info(slide, 1, ((char *)slide, LoadFarString(CannotCreateFile),
          tfilnam, strerror(errno)));
        izu_free(tfilnam);
        return 1;
    }
    izu_free(tfilnam);
#  else /* def TOPS20 */
#   ifdef MTS
    if (uO.aflag)
        G.outfile = zfopen(G.filename, FOPWT);
    else
        G.outfile = zfopen(G.filename, FOPW);
    if (G.outfile == (FILE *)NULL) {
        Info(slide, 1, ((char *)slide, LoadFarString(CannotCreateFile),
          FnFilter1(G.filename), strerror(errno)));
        return 1;
    }
#   else /* def MTS */
#    ifdef DEBUG
    Info(slide, 1, ((char *)slide,
      "open_outfile:  doing fopen(%s) for reading\n", FnFilter1(G.filename)));
    if ((G.outfile = zfopen(G.filename, FOPR)) == (FILE *)NULL)
        Info(slide, 1, ((char *)slide,
          "open_outfile:  fopen(%s) for reading failed:  does not exist\n",
          FnFilter1(G.filename)));
    else {
        Info(slide, 1, ((char *)slide,
          "open_outfile:  fopen(%s) for reading succeeded:  file exists\n",
          FnFilter1(G.filename)));
        fclose(G.outfile);
    }
#    endif /* def DEBUG */
#    ifdef NOVELL_BUG_FAILSAFE
    if (G.dne && ((G.outfile = zfopen(G.filename, FOPR)) != (FILE *)NULL)) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(NovellBug),
          FnFilter1(G.filename)));
        fclose(G.outfile);
        return 1;   /* with "./" fix in checkdir(), should never reach here */
    }
#    endif /* def NOVELL_BUG_FAILSAFE */
    Trace((stderr, "open_outfile:  doing fopen(%s) for writing\n",
      FnFilter1(G.filename)));
    {
#    if defined(ATH_BE_UNX) || defined(AOS_VS) || defined(QDOS) || defined(TANDEM)
        mode_t umask_sav = umask(0077);
#    endif

#    if defined(SYMLINKS) && defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
        G.outfile = (G.has_win32_wide
                    ? zfopenw(G.unipath_widefilename, FOPWR_W)
                    : zfopen(G.filename, FOPWR)
                    );
#    else /* defined(SYMLINKS) && defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
#     if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
        G.outfile = (G.has_win32_wide
                    ? zfopenw(G.unipath_widefilename, FOPW_W)
                    : zfopen(G.filename, FOPW)
                    );
#     else /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
#      if defined(SYMLINKS) || defined(QLZIP)
        /* These features require the ability to re-read extracted data from
           the output files. Output files are created with Read&Write access.
         */
        G.outfile = zfopen(G.filename, FOPWR);
#      else
        G.outfile = zfopen(G.filename, FOPW);
#      endif
#     endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */
#    endif /* defined(SYMLINKS) && defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */

#    if defined(ATH_BE_UNX) || defined(AOS_VS) || defined(QDOS) || defined(TANDEM)
        umask(umask_sav);
#    endif
    }
    if (G.outfile == (FILE *)NULL) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(CannotCreateFile),
          FnFilter1(G.filename), strerror(errno)));
        return 1;
    }
    Trace((stderr, "open_outfile:  fopen(%s) for writing succeeded\n",
      FnFilter1(G.filename)));
#   endif /* def MTS */
#  endif /* def TOPS20 [else] */

#  ifdef USE_FWRITE
#   ifdef DOS_NLM_OS2_W32
    /* 16-bit MSC: buffer size must be strictly LESS than 32K (WSIZE):  bogus */
    setbuf(G.outfile, (char *)NULL);   /* make output unbuffered */
#   else /* def DOS_NLM_OS2_W32 */
#    ifndef RISCOS
#     ifdef _IOFBF  /* make output fully buffered (works just about like write()) */
    setvbuf(G.outfile, (char *)slide, _IOFBF, WSIZE);
#     else /* def _IOFBF */
    setbuf(G.outfile, (char *)slide);
#     endif /* def _IOFBF [else] */
#    endif /* ndef RISCOS */
#   endif /* def DOS_NLM_OS2_W32 [else] */
#  endif /* def USE_FWRITE */
#  ifdef OS2_W32
    /* preallocate the final file size to prevent file fragmentation */
    SetFileSize(G.outfile, G.lrec.ucsize);
#  endif /* def OS2_W32 */
    return 0;

} /* end function open_outfile() */

# endif /* !defined(TANDEM) */
#endif /* !defined(VMS) && !defined(AOS_VS) && !defined(CMS_MVS) && !defined(MACOS) */




/*
 * These functions allow NEXTBYTE to function without needing two bounds
 * checks.  Call defer_leftover_input() if you ever have filled G.inbuf
 * by some means other than readbyte(), and you then want to start using
 * NEXTBYTE.  When going back to processing bytes without NEXTBYTE, call
 * undefer_input().  For example, extract_or_test_member brackets its
 * central section that does the decompression with these two functions.
 * If you need to check the number of bytes remaining in the current
 * file while using NEXTBYTE, check (G.csize + G.incnt), not G.csize.
 */

/****************************/
/* function undefer_input() */
/****************************/

void undefer_input(__G)
    __GDEF
{
    if (G.incnt > 0)
        G.csize += G.incnt;
    if (G.incnt_leftover > 0) {
        /* We know that "(G.csize < MAXINT)" so we can cast G.csize to int:
         * This condition was checked when G.incnt_leftover was set > 0 in
         * defer_leftover_input(), and it is NOT allowed to touch G.csize
         * before calling undefer_input() when (G.incnt_leftover > 0)
         * (single exception: see read_byte()'s  "G.csize <= 0" handling) !!
         */
        G.incnt = G.incnt_leftover + (int)G.csize;
        G.inptr = G.inptr_leftover - (int)G.csize;
        G.incnt_leftover = 0;
    } else if (G.incnt < 0)
        G.incnt = 0;
} /* end function undefer_input() */




/***********************************/
/* function defer_leftover_input() */
/***********************************/

void defer_leftover_input(__G)
    __GDEF
{
    if ((zoff_t)G.incnt > G.csize) {
        /* (G.csize < MAXINT), we can safely cast it to int !! */
        if (G.csize < 0L)
            G.csize = 0L;
        G.inptr_leftover = G.inptr + (int)G.csize;
        G.incnt_leftover = G.incnt - (int)G.csize;
        G.incnt = (int)G.csize;
    } else
        G.incnt_leftover = 0;
    G.csize -= G.incnt;
} /* end function defer_leftover_input() */




/**********************/
/* Function readbuf() */
/**********************/

unsigned readbuf(__G__ buf, size)   /* return number of bytes read into buf */
    __GDEF
    char *buf;
    register unsigned size;
{
    register unsigned count;
    unsigned n;

    n = size;
    while (size)
    {
        if (G.incnt <= 0)
        {
            if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0)
            {
              /* read() got no data.  If the srchive is segmented, then
               * try again with the next segment file.
               */
              /*if(fd_is_valid(G.zipfd_sgmnt)) {*/
              if (G.ecrec.number_this_disk > 0)
              {
                if ((open_infile_sgmnt( 1) != 0) ||
                 (G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0)
                    return (n-size);    /* Return short retry size. */
              } else
                 return (n-size);       /* Return short size. */
            }

            if (G.incnt < 0)
            {
                /* another hack, but no real harm copying same thing twice */
                (*G.message)((zvoid *)&G,
                  (uch *)LoadFarString(ReadError),  /* CANNOT use slide */
                  (ulg)strlen(LoadFarString(ReadError)), 0x401);
                return 0;  /* discarding some data; better than lock-up */
            }
            /* buffer ALWAYS starts on a block boundary:  */
            G.cur_zipfile_bufstart += INBUFSIZ;
            G.inptr = G.inbuf;
        }
        count = IZ_MIN(size, (unsigned)G.incnt);
        memcpy(buf, G.inptr, count);
        buf += count;
        G.inptr += count;
        G.incnt -= count;
        size -= count;
    }
    return n;

} /* end function readbuf() */




/***********************/
/* Function readbyte() */
/***********************/

int readbyte(__G)   /* refill inbuf and return a byte if available, else EOF */
    __GDEF
{
    if (G.mem_mode)
        return EOF;
    if (G.csize <= 0)
    {
        G.csize--;             /* for tests done after exploding */
        G.incnt = 0;
        return EOF;
    }
    if (G.incnt <= 0)
    {
        if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0)
        {
            /* read() got no data.  If the srchive is segmented, then
             * try again with the next segment file.
             */
            /* if(fd_is_valid(G.zipfd_sgmnt)) { */
            if (G.ecrec.number_this_disk > 0)
            {
              if ((open_infile_sgmnt( 1) != 0) ||
               (G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0)
                return EOF;
            } else
              return EOF;
        }

        if (G.incnt < 0)
        {   /* "fail" (abort, retry, ...) returns this.
             * another hack, but no real harm copying same thing twice.
             */
            (*G.message)((zvoid *)&G,
              (uch *)LoadFarString(ReadError),
              (ulg)strlen(LoadFarString(ReadError)), 0x401);
            echon();
#if defined( WINDLL) || defined( DLL)
            longjmp(dll_error_return, 1);
#else
            DESTROYGLOBALS();
            EXIT(PK_BADERR);    /* totally bailing; better than lock-up */
#endif
        }
        G.cur_zipfile_bufstart += INBUFSIZ; /* always starts on block bndry */
        G.inptr = G.inbuf;
        defer_leftover_input(__G);           /* decrements G.csize */
    }

#ifdef IZ_CRYPT_ANY
    if (G.pInfo->encrypted)
    {
# ifdef IZ_CRYPT_AES_WG
        if (G.lrec.compression_method == AESENCRED)
        {
            int n;

            n = (int)(IZ_MIN( G.incnt, G.ucsize_aes));
            fcrypt_decrypt( G.inptr, n, G.zcx);
            G.ucsize_aes -= n;
        }
        else
# endif /* def IZ_CRYPT_AES_WG */
        {
# ifdef IZ_CRYPT_TRAD
            uch *p;
            int n;

        /* This was previously set to decrypt one byte beyond G.csize, when
         * incnt reached that far.  GRR said, "but it's required:  why?"  This
         * was a bug in fillinbuf() -- was it also a bug here?
         */
            for (n = G.incnt, p = G.inptr;  n--;  p++)
                zdecode(*p);
# endif /* def IZ_CRYPT_TRAD */
        }
    }
#endif /* def IZ_CRYPT_ANY */

    --G.incnt;
    return *G.inptr++;

} /* end function readbyte() */




#if defined(USE_ZLIB) || defined(BZIP2_SUPPORT) || defined(LZMA_SUPPORT)

/************************/
/* Function fillinbuf() */
/************************/

int fillinbuf(__G) /* like readbyte() except returns number of bytes in inbuf */
    __GDEF
{
    if (G.mem_mode ||
                  (G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) <= 0)
        return 0;
    G.cur_zipfile_bufstart += INBUFSIZ;  /* always starts on a block boundary */
    G.inptr = G.inbuf;
    defer_leftover_input(__G);           /* decrements G.csize */

# ifdef IZ_CRYPT_ANY
    if (G.pInfo->encrypted)
    {
#  ifdef IZ_CRYPT_AES_WG
        if (G.lrec.compression_method == AESENCRED)
        {
            int n;

            n = IZ_MIN( G.incnt, (long)G.ucsize_aes);
            fcrypt_decrypt( G.inptr, n, G.zcx);
            G.ucsize_aes -= n;
        }
        else
#  endif /* def IZ_CRYPT_AES_WG */
        {
#  ifdef IZ_CRYPT_TRAD
            uch *p;
            int n;

            for (n = G.incnt, p = G.inptr;  n--;  p++)
                zdecode(*p);
#  endif /* def IZ_CRYPT_TRAD */
        }
    }
# endif /* def IZ_CRYPT_ANY */

    return G.incnt;

} /* end function fillinbuf() */

#endif /* defined(USE_ZLIB) || defined(BZIP2_SUPPORT) || defined(LZMA_SUPPORT) */




/************************/
/* Function seek_zipf() */
/************************/

int seek_zipf(__G__ abs_offset)
    __GDEF
    zoff_t abs_offset;
{
/*
 *  Seek to the block boundary of the block which includes abs_offset,
 *  then read block into input buffer and set pointers appropriately.
 *  If block is already in the buffer, just set the pointers.  This function
 *  is used by do_seekable (process.c), extract_or_test_entrylist (extract.c)
 *  and do_string (fileio.c).  Also, a slightly modified version is embedded
 *  within extract_or_test_entrylist (extract.c).  readbyte() and readbuf()
 *  (fileio.c) are compatible.  NOTE THAT abs_offset is intended to be the
 *  "proper offset" (i.e., if there were no extra bytes prepended);
 *  cur_zipfile_bufstart contains the corrected offset.
 *
 *  Because seek_zipf() is never used during decompression, it is safe
 *  to use the slide[] buffer for the error message.
 *
 * returns PK error codes:
 *  PK_BADERR if effective offset in zipfile is negative
 *  PK_EOF if seeking past end of zipfile
 *  PK_OK when seek was successful
 */

/*     zoff_t request = abs_offset + G.extra_bytes; */
/*     zoff_t inbuf_offset = request % INBUFSIZ; */
/*     zoff_t bufstart = request - inbuf_offset; */

  zoff_t request;
  zoff_t inbuf_offset;
  zoff_t bufstart;

  request = abs_offset + G.extra_bytes;

#if 0 /* Pre-segment-support. */
  if (request < 0)
  {
    Info(slide, 1, ((char *)slide, LoadFarStringSmall(SeekMsg),
     2, G.zipfn, LoadFarString(ReportMsg)));
    return PK_BADERR;
  }
#endif /* 0 */ /* Pre-segment-support. */

  while (request < 0)
  {
    if (G.sgmnt_size == 0)
    {
      if ((G.sgmnt_nr == 0) || open_infile_sgmnt(-1))
      {
        Info(slide, 1, ((char *)slide, LoadFarStringSmall(SeekMsg),
         3, G.zipfn, LoadFarString(ReportMsg)));
        return PK_BADERR;
      }
      /* Get the new segment size, and calculate the new offset.
       * This is where G.sgmnt_size gets a real (non-zero) value.
       */
#ifdef USE_STRM_INPUT
      zfseeko(G.zipfd, 0, SEEK_END);
      G.sgmnt_size = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
      G.sgmnt_size = zlseek(G.zipfd, 0, SEEK_END);
#endif /* USE_STRM_INPUT */
      request += G.sgmnt_size;
    }
    else
    {
      /* SMSd.  Can we trust that all segments have the same size? */
      /* We know segment size(s?), so we can calculate against
       * abs_offset and sgmnt_nr, and open the segment file which we
       * actually need.
       */
      unsigned int tmp_disk = G.ecrec.number_this_disk;

      while (request < 0)
      {
        tmp_disk--;
        request += G.sgmnt_size;
      }
      /* for same as actual disk (movement == 0) return 0  */
      if (open_infile_sgmnt(tmp_disk - G.sgmnt_nr))
      {
          Info(slide, 1, ((char *)slide, LoadFarStringSmall(SeekMsg),
           4, G.zipfn, LoadFarString(ReportMsg)));
          return PK_BADERR;
      }
    }
    /* In both cases we need to refill buffer, so set
     * G.cur_zipfile_bufstart negative (so that bufstart !=
     * G.cur_zipfile_bufstart, below).
     */
    G.cur_zipfile_bufstart = -1;
  }

  inbuf_offset = request % INBUFSIZ;
  bufstart = request - inbuf_offset;

  if (bufstart != G.cur_zipfile_bufstart)
  {
    Trace((stderr,
     "fpos_zip: abs_offset = %s, G.extra_bytes = %s\n",
     FmZofft(abs_offset, NULL, NULL),
     FmZofft(G.extra_bytes, NULL, NULL)));

#ifdef USE_STRM_INPUT
    zfseeko(G.zipfd, bufstart, SEEK_SET);
    G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
    G.cur_zipfile_bufstart = zlseek(G.zipfd, bufstart, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */

    Trace((stderr,
     "       request = %s, (abs+extra) = %s, inbuf_offset = %s\n",
     FmZofft(request, NULL, NULL),
     FmZofft((abs_offset+G.extra_bytes), NULL, NULL),
     FmZofft(inbuf_offset, NULL, NULL)));
    Trace((stderr, "       bufstart = %s, cur_zipfile_bufstart = %s\n",
     FmZofft(bufstart, NULL, NULL),
     FmZofft(G.cur_zipfile_bufstart, NULL, NULL)));

    if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) < INBUFSIZ)
    {
      /* If we're not at the end of file, then we need move to the
       * next segment file.
       * TODO: Check if EOF, instead?
       */ 
      if (G.ecrec.number_this_disk != G.sgmnt_nr)
      {
        int tmp;

        if (open_infile_sgmnt(1))
          return PK_EOF; /*TODO: Add some new return code? */

        /* append rest of data to buffer - it's important when we are only
         * few bytes before EOF and want read CDR! Now it's more safe.
         * Only some disk error could be wrong for us in that case.
         */
        tmp = read(G.zipfd, (char *)(G.inbuf+ G.incnt), (INBUFSIZ- G.incnt));
        if (tmp <= 0)
          return PK_EOF;

        G.incnt += tmp;
      }
      else if (G.incnt <= 0)
        return PK_EOF;
    }
    G.incnt -= (int)inbuf_offset;
    G.inptr = G.inbuf + (int)inbuf_offset;
  }
  else
  {
    G.incnt += (int)(G.inptr- G.inbuf) - (int)inbuf_offset;
    G.inptr = G.inbuf + (int)inbuf_offset;
  }
    return PK_OK;
} /* seek_zipf(). */


/************************/
/* Function fgets_ans() */
/************************/

int fgets_ans( __G)
    __GDEF
{
    char *ans;
    int ret;
    char waste[ 8];

    ans = fgets( G.answerbuf, sizeof( G.answerbuf), G.query_fp);
    if (ans == NULL)
    {   /* Error or end-of-file. */
        ret = -1;                       /* Failure code. */
        *G.answerbuf = '\0';            /* Null the answer buffer. */
    }
    else
    {
        ret = 0;                        /* Success code. */
        /* Read any remaining chars on line, leaving G.answerbuf intact. */
        while ((ans != NULL) && (ans[ strlen( ans)- 1] != '\n'))
        {
            ans = fgets( waste, sizeof( waste), G.query_fp);
        }
    }
    return ret;
}




#ifndef VMS  /* For VMS, use code in vms.c. */

/********************/
/* Function flush() */   /* returns PK error codes: */
/********************/   /* if tflag => always 0; PK_DISK if write error */

int flush(__G__ rawbuf, size, unshrink)
    __GDEF
    uch *rawbuf;
    ulg size;
    int unshrink;
# if defined(DEFLATE64_SUPPORT) && defined(__16BIT__)
{
    int ret;

    /* On 16-bit systems (MSDOS, OS/2 1.x), the standard C library functions
     * cannot handle writes of 64k blocks at once.  For these systems, the
     * blocks to flush are split into pieces of 32k or less.
     */
    while (size > 0x8000L) {
        ret = partflush(__G__ rawbuf, 0x8000L, unshrink);
        if (ret != PK_OK)
            return ret;
        size -= 0x8000L;
        rawbuf += (extent)0x8000;
    }
    return partflush(__G__ rawbuf, size, unshrink);
} /* end function flush() */




/************************/
/* Function partflush() */  /* returns PK error codes: */
/************************/  /* if tflag => always 0; PK_DISK if write error */

static int partflush(__G__ rawbuf, size, unshrink)
    __GDEF
    uch *rawbuf;        /* cannot be ZCONST, gets passed to (*G.message)() */
    ulg size;
    int unshrink;
# endif /* defined(DEFLATE64_SUPPORT) && defined(__16BIT__) */
{
    register uch *p;
    register uch *q;
    uch *transbuf;
# if (defined(SMALL_MEM) || defined(MED_MEM) || defined(VMS_TEXT_CONV))
    ulg transbufsiz;
# endif
    /* static int didCRlast = FALSE;    moved to globals.h */


/*---------------------------------------------------------------------------
    Compute the CRC first; if testing or if disk is full, that's it.
  ---------------------------------------------------------------------------*/

    G.crc32val = crc32(G.crc32val, rawbuf, (extent)size);

# ifdef DLL
    if ((G.statreportcb != NULL) &&
        (*G.statreportcb)(__G__ UZ_ST_IN_PROGRESS, G.zipfn, G.filename, NULL))
        return IZ_CTRLC;        /* cancel operation by user request */
# endif

    if (uO.tflag || size == 0L)  /* testing or nothing to write:  all done */
        return PK_OK;

    if (G.disk_full)
        return PK_DISK;         /* disk already full:  ignore rest of file */

/*---------------------------------------------------------------------------
    Write the bytes rawbuf[0..size-1] to the output device, first converting
    end-of-lines and ASCII/EBCDIC as needed.  If SMALL_MEM or MED_MEM are NOT
    defined, outbuf is assumed to be at least as large as rawbuf and is not
    necessarily checked for overflow.
  ---------------------------------------------------------------------------*/

    if (!G.pInfo->textmode)
    {   /* write raw binary data */
        /* GRR:  note that for standard MS-DOS compilers, size argument to
         * fwrite() can never be more than 65534, so WriteError macro will
         * have to be rewritten if size can ever be that large.  For now,
         * never more than 32K.  Also note that write() returns an int, which
         * doesn't necessarily limit size to 32767 bytes if write() is used
         * on 16-bit systems but does make it more of a pain; however, because
         * at least MSC 5.1 has a lousy implementation of fwrite() (as does
         * DEC Ultrix cc), write() is used anyway.
         */
# ifdef DLL
        if (G.redirect_data) {
#  ifdef NO_SLIDE_REDIR
            if (writeToMemory(__G__ rawbuf, (extent)size)) return PK_ERR;
#  else
            writeToMemory(__G__ rawbuf, (extent)size);
#  endif
        } else
# endif

# if defined( UNIX) && defined( __APPLE__)
        /* If expecting AppleDouble header bytes, process them.
         * Note that any extended attributes are analyzed, whether or
         * not setxattr() is available to apply them, so that we can
         * correctly locate the resource fork data which follow them.
         */
        if (G.apl_dbl_hdr_bytes > 0)
        {
            if (size < G.apl_dbl_hdr_bytes)
            {
                /* Fewer bytes than needed to complete the AppleDouble
                 * header.  Move available data to the AppleDouble
                 * header buffer, adjust the byte counts, and resume
                 * extraction.
                 */
                memcpy( &G.apl_dbl_hdr[ G.apl_dbl_hdr_len], rawbuf, size);
                G.apl_dbl_hdr_len += size;      /* Bytes in header. */
                G.apl_dbl_hdr_bytes -= size;    /* Hdr bytes still sought. */
                size = 0;                       /* Bytes left in rawbuf. */
            }
            else
            {
                /* Enough bytes to complete the AppleDouble header
                 * (short and/or extended).  Move expected data to the
                 * AppleDouble header buffer, and adjust the byte counts
                 * and pointer.  If (only) the first part (through
                 * Finder info) is complete, then get the offset for the
                 * resource fork, and adjust the expected header length
                 * accordingly, to include the attributes.
                 */
                int res_frk_offs;

                memcpy( &G.apl_dbl_hdr[ G.apl_dbl_hdr_len], rawbuf,
                 G.apl_dbl_hdr_bytes);
                G.apl_dbl_hdr_len += G.apl_dbl_hdr_bytes;   /* Bytes in hdr. */
                size -= G.apl_dbl_hdr_bytes;    /* Bytes left in rawbuf. */
                rawbuf += G.apl_dbl_hdr_bytes;  /* Pointer to remaining data. */
                G.apl_dbl_hdr_bytes = 0;        /* Hdr bytes still sought. */

                if (G.apl_dbl_hdr_len == APL_DBL_SIZE_HDR)
                {
                    /* Have complete basic header.  Get resource fork offset. */
                    res_frk_offs = BIGC_TO_HOST32(
                     &G.apl_dbl_hdr[ APL_DBL_OFS_ENT_DSCR_OFS1]);

                    if (res_frk_offs > APL_DBL_SIZE_HDR)
                    {
                        /* Have attributes.  Revise remaining header
                         * size accordingly.
                         */
                        G.apl_dbl_hdr_bytes =
                         res_frk_offs- APL_DBL_SIZE_HDR;

                        /* Allocate more AplDbl header storage, if needed. */
                        if (G.apl_dbl_hdr_alloc < res_frk_offs)
                        {
                            G.apl_dbl_hdr = izu_realloc( G.apl_dbl_hdr,
                             res_frk_offs);
                            if (G.apl_dbl_hdr == NULL)
                            {
                                Info(slide, 1, ((char *)slide, MemAllocFailed,
                                 2, res_frk_offs));
                                return 1;
                            }
                            G.apl_dbl_hdr_alloc = res_frk_offs;
                        }

                        if (size < G.apl_dbl_hdr_bytes)
                        {
                            /* Fewer bytes than needed to complete the
                             * AppleDouble header.  Move available data
                             * to the AppleDouble header buffer, adjust
                             * the byte counts, and resume extraction.
                             */
                            memcpy( &G.apl_dbl_hdr[ G.apl_dbl_hdr_len],
                             rawbuf, size);
                            G.apl_dbl_hdr_len += size;  /* Bytes in header. */
                            G.apl_dbl_hdr_bytes -= size;/* Hdr byts stl sght. */
                            size = 0;                   /* Bs left in rawbuf. */
                        }
                        else
                        {
                            /* Enough bytes to complete the AppleDouble
                             * header (extended).  Move remaining data
                             * to the AppleDouble header buffer, and
                             * adjust the byte counts and pointer.
                             */
                            memcpy( &G.apl_dbl_hdr[ G.apl_dbl_hdr_len],
                             rawbuf, G.apl_dbl_hdr_bytes);
                            G.apl_dbl_hdr_len = res_frk_offs; /* Byts in hdr. */
                            size -= G.apl_dbl_hdr_bytes;/* Bs left in rawbuf. */
                            rawbuf += G.apl_dbl_hdr_bytes;  /* Ptr to r data. */
                            G.apl_dbl_hdr_bytes = 0;    /* Hdr byts stl sght. */
                        }
                    }
                }

                if (G.apl_dbl_hdr_bytes == 0)
                {
                    /* Set the Finder info and other attributes (for the
                     * plain-name) file.
                     */
                    char btrbslash; /* Saved character had better be a slash. */
                    int sts;
                    struct attrlist attr_list_fndr;

                    /* Truncate name at "/rsrc" for setattrlist(). */
                    btrbslash =
                     G.filename[ strlen( G.filename)- strlen( APL_DBL_SUFX)];
                    G.filename[ strlen( G.filename)- strlen( APL_DBL_SUFX)] =
                     '\0';

                    /* Clear attribute list structure. */
                    memset( &attr_list_fndr, 0, sizeof( attr_list_fndr));
                    /* Set attribute list bits for Finder info. */
                    attr_list_fndr.bitmapcount = ATTR_BIT_MAP_COUNT;
                    attr_list_fndr.commonattr = ATTR_CMN_FNDRINFO;

                    if (!uO.Jf_flag)
                    {
                        /* Set Finder info for main file. */
                        sts = setattrlist(
                         G.filename,                /* Path. */
                         &attr_list_fndr,           /* Attrib list. */
                         &G.apl_dbl_hdr[ APL_DBL_OFS_FNDR_INFO], /* Src bufr. */
                         APL_DBL_SIZE_FNDR_INFO,    /* Src buffer size. */
                         0);                        /* Options. */

                        if (sts != 0)
                        {
                            Info(slide, 0x12, ((char *)slide,
                             SetattrlistFailed, errno, G.filename));
                        }
                    }

#  ifdef APPLE_XATTR
                    if ((!uO.Je_flag) &&
                     (G.apl_dbl_hdr_len > APL_DBL_OFS_ATTR))
                    {
                        /* int attr_offs;               (Unused.) */
                        int attr_count;
                        int attr_ndx;
                        char *attr_ptr;
                        int attr_size;
                        int ndx;
                        /* unsigned short flags;        (Unused.) */
                        int val_offs;
                        int val_size;

/* AppleDouble extended attribute data layout:
 *
 *  A+  Size  Description
 *   0    4   Attribute magic ("ATTR").
 *   4    4   FileID (for debug).
 *   8    4   Total size (pre-resource-fork).
 *  12    4   Attribute value first offset.
 *  16    4   Attribute value total size.
 *  20   12   Reserved.
 *  32    2   Flags.
 *  34    2   Attribute count.
 *  36    4   Value offset [0].  (Align:4.)                     -+
 *  40    4   Value size [0].                                    |
 *  44    2   Flags.                                             |
 *  46    1   Attribute name size [0].                           |
 *  47  var   Attribute name [0]  var = size + pad to align:4.   |
 *            Next offset = (name_offset+ size+ 3)& 0xfffffffc. -+
 *  O1    4   Value offset [1].
 *  O1+4  4   Value size [1].
 *  O1+8  2   Flags.
 *  O1+10 1   Attribute name size [1].
 *  O1+11 1   Attribute name  [1].
 * [...]
 */
                        ndx = APL_DBL_OFS_ATTR+ 12;
                        /* attr_offs = BIGC_TO_HOST32( &G.apl_dbl_hdr[ ndx]); */
                        ndx += 22;
                        attr_count = BIGC_TO_HOST16( &G.apl_dbl_hdr[ ndx]);
                        ndx += 2;

                        /* Loop through and set the extended attributes. */
                        for (attr_ndx = 0; attr_ndx < attr_count; attr_ndx++)
                        {
                            val_offs = BIGC_TO_HOST32( &G.apl_dbl_hdr[ ndx]);
                            ndx += 4;
                            val_size = BIGC_TO_HOST32( &G.apl_dbl_hdr[ ndx]);
                            ndx += 4;
                            /* flags = BIGC_TO_HOST16( &G.apl_dbl_hdr[ ndx]); */
                            ndx += 2;
                            attr_size = G.apl_dbl_hdr[ ndx];
                            ndx += 1;
                            attr_ptr = (char *)(&(G.apl_dbl_hdr[ ndx]));

                            /* 2013-07-05 SMS.
                             * Add more code here to inhibit setting
                             * selected extended attributes.
                             */
#   define XATTR_QTR "com.apple.quarantine"

                            if ((!uO.Jq_flag) ||
                             (strcmp( attr_ptr, XATTR_QTR) != 0))
                            {
                                sts = setxattr(
                                 G.filename,            /* Real file name. */
                                 attr_ptr,              /* Attr name. */
                                 &(G.apl_dbl_hdr[ val_offs]),   /* Attr val. */
                                 val_size,              /* Attr value size. */
                                 0,                     /* Position. */
                                 XATTR_NOFOLLOW);       /* Options. */

                                if (sts != 0)
                                {
                                    Info(slide, 0x12, ((char *)slide,
                                     SetxattrFailed, errno, G.filename));
                                }
                            }

                            /* Advance index to the next align:4 value. */
                            ndx = (ndx+ attr_size+ 3)& 0xfffffffc;
                        }
                    } 
#  endif /* def APPLE_XATTR */

                    /* Restore name suffix ("/rsrc"). */
                    G.filename[ strlen( G.filename)] = btrbslash;
                 }
            }

            /* Quit now if there are no resource fork data, or if the
             * user inhibited the resource fork.
             */
            if ((size == 0L) || uO.Jr_flag)
                return PK_OK;
        }
# endif /* defined( UNIX) && defined( __APPLE__) */

        if (!uO.cflag && WriteError(rawbuf, size, G.outfile))
            return disk_error(__G);
        else if (uO.cflag && (*G.message)((zvoid *)&G, rawbuf, size, 0x2000))
            return PK_OK;
    } else {   /* textmode:  aflag is true */
        if (unshrink) {
            /* rawbuf = outbuf */
            transbuf = G.outbuf2;
# if (defined(SMALL_MEM) || defined(MED_MEM) || defined(VMS_TEXT_CONV))
            transbufsiz = TRANSBUFSIZ;
# endif
        } else {
            /* rawbuf = slide */
            transbuf = G.outbuf;
# if (defined(SMALL_MEM) || defined(MED_MEM) || defined(VMS_TEXT_CONV))
            transbufsiz = OUTBUFSIZ;
            Trace((stderr, "\ntransbufsiz = OUTBUFSIZ = %u\n",
                   (unsigned)OUTBUFSIZ));
# endif
        }
        if (G.newfile) {
# ifdef VMS_TEXT_CONV
            if (G.pInfo->hostnum == VMS_ && G.extra_field &&
                is_vms_varlen_txt(__G__ G.extra_field,
                                  G.lrec.extra_field_length))
                G.VMS_line_state = 0;    /* 0: ready to read line length */
            else
                G.VMS_line_state = -1;   /* -1: don't treat as VMS text */
# endif
            G.didCRlast = FALSE;         /* no previous buffers written */
            G.newfile = FALSE;
        }

# ifdef VMS_TEXT_CONV
        if (G.VMS_line_state >= 0)
        {
            p = rawbuf;
            q = transbuf;
            while ((extent)(p-rawbuf) < (extent)size) {
                switch (G.VMS_line_state) {

                    /* 0: ready to read line length */
                    case 0:
                        G.VMS_line_length = 0;
                        if ((extent)(p-rawbuf) == (extent)size-1) {
                            /* last char */
                            G.VMS_line_length = (unsigned)(*p++);
                            G.VMS_line_state = 1;
                        } else {
                            G.VMS_line_length = makeword(p);
                            p += 2;
                            G.VMS_line_state = 2;
                        }
                        G.VMS_line_pad =
                               ((G.VMS_line_length & 1) != 0); /* odd */
                        break;

                    /* 1: read one byte of length, need second */
                    case 1:
                        G.VMS_line_length += ((unsigned)(*p++) << 8);
                        G.VMS_line_state = 2;
                        break;

                    /* 2: ready to read VMS_line_length chars */
                    case 2:
                        {
                            extent remaining = (extent)size+(rawbuf-p);
                            extent outroom;

                            if (G.VMS_line_length < remaining) {
                                remaining = G.VMS_line_length;
                                G.VMS_line_state = 3;
                            }

                            outroom = transbuf+(extent)transbufsiz-q;
                            if (remaining >= outroom) {
                                remaining -= outroom;
                                for (;outroom > 0; p++, outroom--)
                                    *q++ = native(*p);
# ifdef DLL
                                if (G.redirect_data) {
                                    if (writeToMemory(__G__ transbuf,
                                          (extent)(q-transbuf))) return PK_ERR;
                                } else
# endif
                                if (!uO.cflag && WriteError(transbuf,
                                    (q- transbuf), G.outfile))
                                    return disk_error(__G);
                                else if (uO.cflag && (*G.message)((zvoid *)&G,
                                         transbuf, (ulg)(q-transbuf), 0x2000))
                                    return PK_OK;
                                q = transbuf;
                                /* fall through to normal case */
                            }
                            G.VMS_line_length -= remaining;
                            for (;remaining > 0; p++, remaining--)
                                *q++ = native(*p);
                        }
                        break;

                    /* 3: ready to PutNativeEOL */
                    case 3:
                        if (q > transbuf+(extent)transbufsiz-lenEOL) {
# ifdef DLL
                            if (G.redirect_data) {
                                if (writeToMemory(__G__ transbuf,
                                      (extent)(q-transbuf))) return PK_ERR;
                            } else
# endif
                            if (!uO.cflag &&
                                WriteError(transbuf, (q- transbuf),
                                  G.outfile))
                                return disk_error(__G);
                            else if (uO.cflag && (*G.message)((zvoid *)&G,
                                     transbuf, (ulg)(q-transbuf), 0x2000))
                                return PK_OK;
                            q = transbuf;
                        }
                        PutNativeEOL
                        G.VMS_line_state = G.VMS_line_pad ? 4 : 0;
                        break;

                    /* 4: ready to read pad byte */
                    case 4:
                        ++p;
                        G.VMS_line_state = 0;
                        break;
                }
            } /* end while */

        } else
# endif /* def VMS_TEXT_CONV */

    /*-----------------------------------------------------------------------
        Algorithm:  CR/LF => native; lone CR => native; lone LF => native.
        This routine is only for non-raw-VMS, non-raw-VM/CMS files (i.e.,
        stream-oriented files, not record-oriented).
      -----------------------------------------------------------------------*/

        /* else not VMS text */ {
            p = rawbuf;
            if (*p == LF && G.didCRlast)
                ++p;
            G.didCRlast = FALSE;
            for (q = transbuf;  (extent)(p-rawbuf) < (extent)size;  ++p) {
                if (*p == CR) {           /* lone CR or CR/LF: treat as EOL  */
                    PutNativeEOL
                    if ((extent)(p-rawbuf) == (extent)size-1)
                        /* last char in buffer */
                        G.didCRlast = TRUE;
                    else if (p[1] == LF)  /* get rid of accompanying LF */
                        ++p;
                } else if (*p == LF)      /* lone LF */
                    PutNativeEOL
                else
# ifndef DOS_FLX_OS2_W32
                if (*p != CTRLZ)          /* lose all ^Z's */
# endif
                    *q++ = native(*p);

# if defined(SMALL_MEM) || defined(MED_MEM)
#  if (lenEOL == 1) /* don't check unshrink:  both buffers small but equal */
                if (!unshrink)
#  endif
                    /* check for danger of buffer overflow and flush */
                    if (q > transbuf+(extent)transbufsiz-lenEOL) {
                        Trace((stderr,
                          "p - rawbuf = %u   q-transbuf = %u   size = %lu\n",
                          (unsigned)(p-rawbuf), (unsigned)(q-transbuf), size));
                        if (!uO.cflag && WriteError(transbuf,
                            (extent)(q-transbuf), G.outfile))
                            return disk_error(__G);
                        else if (uO.cflag && (*G.message)((zvoid *)&G,
                                 transbuf, (ulg)(q-transbuf), 0x2000))
                            return PK_OK;
                        q = transbuf;
                        continue;
                    }
# endif /* defined(SMALL_MEM) || defined(MED_MEM) */
            }
        }

    /*-----------------------------------------------------------------------
        Done translating:  write whatever we've got to file (or screen).
      -----------------------------------------------------------------------*/

        Trace((stderr, "p - rawbuf = %u   q-transbuf = %u   size = %lu\n",
          (unsigned)(p-rawbuf), (unsigned)(q-transbuf), size));
        if (q > transbuf) {
# ifdef DLL
            if (G.redirect_data) {
                if (writeToMemory(__G__ transbuf, (q- transbuf)))
                    return PK_ERR;
            } else
# endif
            if (!uO.cflag && WriteError(transbuf, (q- transbuf),
                G.outfile))
                return disk_error(__G);
            else if (uO.cflag && (*G.message)((zvoid *)&G, transbuf,
                (ulg)(q-transbuf), 0x2000))
                return PK_OK;
        }
    }

    return PK_OK;

} /* end function flush() [resp. partflush() for 16-bit Deflate64 support] */




# ifdef VMS_TEXT_CONV

/********************************/
/* Function is_vms_varlen_txt() */
/********************************/

/* 2012-11-25 SMS.  (OUSPG report.)
 * Changed eb_len and ef_len from unsigned to signed, to catch underflow
 * of ef_len caused by corrupt/malicious data.  (32-bit is adequate. 
 * Used "long" to accommodate any systems with 16-bit "int".)
 */

static int is_vms_varlen_txt(__G__ ef_buf, ef_len)
    __GDEF
    uch *ef_buf;        /* buffer containing extra field */
    long ef_len;        /* total length of extra field */
{
    unsigned eb_id;
    long eb_len;
    uch *eb_data;
    unsigned eb_datlen;
#  define VMSREC_C_UNDEF        0
#  define VMSREC_C_VAR          2
    uch vms_rectype = VMSREC_C_UNDEF;
 /* uch vms_fileorg = 0; */ /* currently, fileorg is not used... */

#  define VMSPK_ITEMID          0
#  define VMSPK_ITEMLEN         2
#  define VMSPK_ITEMHEADSZ      4

#  define VMSATR_C_RECATTR      4
#  define VMS_FABSIG            0x42414656      /* "VFAB" */
/* offsets of interesting fields in VMS fabdef structure */
#  define VMSFAB_B_RFM          31      /* record format byte */
#  define VMSFAB_B_ORG          29      /* file organization byte */

    if (ef_len == 0 || ef_buf == NULL)
        return FALSE;

    while (ef_len >= EB_HEADSIZE) {
        eb_id = makeword(EB_ID + ef_buf);
        eb_len = makeword(EB_LEN + ef_buf);

        if (eb_len > (ef_len - EB_HEADSIZE)) {
            /* discovered some extra field inconsistency! */
            Trace((stderr,
             "is_vms_varlen_txt: block length %ld > rest ef_size %ld\n",
             eb_len, ef_len - EB_HEADSIZE));
            break;
        }

        switch (eb_id) {
          case EF_PKVMS:
            /* The PKVMS e.f. raw data part consists of:
             * a) 4 bytes CRC checksum
             * b) list of uncompressed variable-length data items
             * Each data item is introduced by a fixed header
             *  - 2 bytes data type ID
             *  - 2 bytes <size> of data
             *  - <size> bytes of actual attribute data
             */

            /* get pointer to start of data and its total length */
            eb_data = ef_buf+(EB_HEADSIZE+4);
            eb_datlen = eb_len-4;

            /* test the CRC checksum */
            if (makelong(ef_buf+EB_HEADSIZE) !=
                crc32(CRCVAL_INITIAL, eb_data, (extent)eb_datlen))
            {
                Info(slide, 1, ((char *)slide,
                  "[Warning: CRC error, discarding PKWARE extra field]\n"));
                /* skip over the data analysis code */
                break;
            }

            /* scan through the attribute data items */
            while (eb_datlen > 4)
            {
                unsigned fldsize = makeword(&eb_data[VMSPK_ITEMLEN]);

                /* check the item type word */
                switch (makeword(&eb_data[VMSPK_ITEMID])) {
                  case VMSATR_C_RECATTR:
                    /* we have found the (currently only) interesting
                     * data item */
                    if (fldsize >= 1) {
                        vms_rectype = eb_data[VMSPK_ITEMHEADSZ] & 15;
                     /* vms_fileorg = eb_data[VMSPK_ITEMHEADSZ] >> 4; */
                    }
                    break;
                  default:
                    break;
                }
                /* skip to next data item */
                eb_datlen -= fldsize + VMSPK_ITEMHEADSZ;
                eb_data += fldsize + VMSPK_ITEMHEADSZ;
            }
            break;

          case EF_IZVMS:
            if (makelong(ef_buf+EB_HEADSIZE) == VMS_FABSIG) {
                if ((eb_data = extract_izvms_block(__G__
                                                   ef_buf+EB_HEADSIZE, eb_len,
                                                   &eb_datlen, NULL, 0))
                    != NULL)
                {
                    if (eb_datlen >= VMSFAB_B_RFM+1) {
                        vms_rectype = eb_data[VMSFAB_B_RFM] & 15;
                     /* vms_fileorg = eb_data[VMSFAB_B_ORG] >> 4; */
                    }
                    izu_free(eb_data);
                }
            }
            break;

          default:
            break;
        }

        /* Skip this extra field block */
        ef_buf += (eb_len + EB_HEADSIZE);
        ef_len -= (eb_len + EB_HEADSIZE);
    }

    return (vms_rectype == VMSREC_C_VAR);

} /* end function is_vms_varlen_txtfile() */

# endif /* def VMS_TEXT_CONV */




/*************************/
/* Function disk_error() */
/*************************/

static int disk_error(__G)
    __GDEF
{
    /* OK to use slide[] here because this file is finished regardless */
    /* 2013-02-04 SMS.
     * By default (I_O_ERROR_QUERY not defined), don't query the user.
     * If I_O_ERROR_QUERY is defined, then query the user only if stdin
     * is a terminal.  STDIN_ISATTY is defined in unzpriv.h (unless some
     * OS-specific sub-header file does it first).
     */
    G.disk_full = 2;            /* Default: No.  Exit program. */
# ifdef USE_I_O_ERROR_QUERY
    if (STDIN_ISATTY)
    {
        Info(slide, 0x4a1, ((char *)slide, LoadFarString(DiskFullQuery),
         FnFilter1(G.filename)));
        fgets_ans( __G);
        if (toupper( *G.answerbuf) == 'Y')      /* Error = "no". */
            /* Stop writing to this file. */
            G.disk_full = 1;            /*  (outfile bad?), but new OK. */
    }
    else
    {
        /* stdin is not a terminal, so don't ask. */
        Info(slide, 0x4a1, ((char *)slide, LoadFarString(DiskFullMsg),
         FnFilter1(G.filename)));
    }
# else /* def USE_I_O_ERROR_QUERY */
    /* Default behavior: Never ask. */
    Info(slide, 0x4a1, ((char *)slide, LoadFarString(DiskFullMsg),
     FnFilter1(G.filename)));
# endif /* def USE_I_O_ERROR_QUERY [else] */

    return PK_DISK;

} /* end function disk_error() */

#endif /* ndef VMS */




/*****************************/
/* Function UzpMessagePrnt() */
/*****************************/

int UZ_EXP UzpMessagePrnt(pG, buf, size, flag)
    zvoid *pG;   /* globals struct:  always passed */
    uch *buf;    /* preformatted string to be printed */
    ulg size;    /* length of string (may include nulls) */
    int flag;    /* flag bits */
{
    /* IMPORTANT NOTE:
     *    The name of the first parameter of UzpMessagePrnt(), which passes
     *    the "Uz_Globs" address, >>> MUST <<< be identical to the string
     *    expansion of the __G__ macro in the REENTRANT case (see globals.h).
     *    This name identity is mandatory for the LoadFarString() macro
     *    (in the SMALL_MEM case) !!!
     */
    int error;
    uch *q=buf, *endbuf=buf+(unsigned)size;
#ifdef MORE
    uch *p=buf;
# if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
    int islinefeed = FALSE;
# endif
#endif /* def MORE */
    FILE *outfp;


/*---------------------------------------------------------------------------
    These tests are here to allow fine-tuning of UnZip's output messages,
    but none of them will do anything without setting the appropriate bit
    in the flag argument of every Info() statement which is to be turned
    *off*.  That is, all messages are currently turned on for all ports.
    To turn off *all* messages, use the UzpMessageNull() function instead
    of this one.
  ---------------------------------------------------------------------------*/

#if (defined(OS2) && defined(DLL))
    if (MSG_NO_DLL2(flag))  /* if OS/2 DLL bit is set, do NOT print this msg */
        return 0;
#endif
#ifdef WINDLL
    if (MSG_NO_WDLL(flag))
        return 0;
    if (MSG_NO_WGUI(flag))
        return 0;
#endif /* def WINDLL */
/*
#ifdef ACORN_GUI
    if (MSG_NO_AGUI(flag))
        return 0;
#endif
 */
#ifdef DLL                 /* don't display message if data is redirected */
    if (((Uz_Globs *)pG)->redirect_data &&
        !((Uz_Globs *)pG)->redirect_text)
        return 0;
#endif /* def DLL */

    /* 2012-10-09 SMS
     * Changed to use stderr for messages, if extracting to stdout
     * (-c, -p).
     */
    if ((MSG_STDERR(flag) && !((Uz_Globs *)pG)->UzO.tflag) ||
     (!MSG_STDOUT(flag) && ((Uz_Globs *)pG)->UzO.cflag))
        outfp = (FILE *)stderr;
    else
        outfp = (FILE *)stdout;

#ifdef VMS
    /* Save the output file for the prompt function. */
    ((Uz_Globs *)pG)->msgfp = outfp;

    /* 2011-05-07 SMS.
     * VMS needs to handle toggling between stdout and stderr in its own
     * way.  We were putting out a message like "inflating <file_name>"
     * to stdout (with no terminating "\n"), and then putting out an
     * error message with a leading "\n" to stderr, to get the error
     * message onto its own line.  This may look ok on UNIX, but it
     * makes a mess on VMS with spurious blank lines, and the error
     * message getting overwritten by the next message to stdout.  Now,
     * we save the outfp value, and, when it changes, and we're not at
     * start-of-line, we put out an explicit "\n" to the previous outfp.
     * This closes the old stdout line properly, allowing the new error
     * message to appear on its own line, with no spurious blank lines
     * added.  This method seems to work on UNIX, too.
     *
     * 2014-12-31 SMS.
     * Actually, on UNIX, it can add an extra (excessive) blank line, so
     * back to VMS-only.  For example, after:
     *       replace xxx.xxx? [y]es, [n]o, [A]ll, [N]one, [r]ename: r
     *       new name: yyy.yyy
     *
     *         inflating: yyy.yyy
     */

    /* When outfp changes, "\n"-terminate any pending line. */
    if (outfp != ((Uz_Globs *)pG)->outfp_prev)
    {
        if (!((Uz_Globs *)pG)->sol)
        {
            (void)WriteTxtErr( "\n", 1, ((Uz_Globs *)pG)->outfp_prev);
            ((Uz_Globs *)pG)->sol = TRUE;
        }
        ((Uz_Globs *)pG)->outfp_prev = outfp;
    }
#endif /* def VMS */

#ifdef QUERY_TRNEWLN
    /* some systems require termination of query prompts with '\n' to force
     * immediate display */
    if (MSG_MNEWLN(flag)) {   /* assumes writable buffer (e.g., slide[]) */
        *endbuf++ = '\n';     /*  with room for one more char at end of buf */
        ++size;               /*  (safe assumption:  only used for four */
    }                         /*  short queries in extract.c and fileio.c) */
#endif /* def QUERY_TRNEWLN */

    if (MSG_TNEWLN(flag)) {   /* again assumes writable buffer:  fragile... */
        if ((!size && !((Uz_Globs *)pG)->sol) ||
            (size && (endbuf[-1] != '\n')))
        {
            *endbuf++ = '\n';
            ++size;
        }
    }

#ifdef MORE
# ifdef SCREENSIZE
    /* room for --More-- and one line of overlap: */
#  if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
    SCREENSIZE(&((Uz_Globs *)pG)->height, &((Uz_Globs *)pG)->width);
#  else
    SCREENSIZE(&((Uz_Globs *)pG)->height, (int *)NULL);
#  endif
    ((Uz_Globs *)pG)->height -= 2;
# else /* def SCREENSIZE */
    /* room for --More-- and one line of overlap: */
    ((Uz_Globs *)pG)->height = SCREENLINES - 2;
#  if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
    ((Uz_Globs *)pG)->width = SCREENWIDTH;
#  endif
# endif /* def SCREENSIZE [else] */
#endif /* def MORE */

    if (MSG_LNEWLN(flag) && !((Uz_Globs *)pG)->sol) {
        /* not at start of line:  want newline */
#ifdef OS2DLL
        if (!((Uz_Globs *)pG)->redirect_text) {
#endif
            putc('\n', outfp);
            fflush(outfp);
#ifdef MORE
            if (((Uz_Globs *)pG)->M_flag)
            {
# if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
                ((Uz_Globs *)pG)->chars = 0;
# endif
                ++((Uz_Globs *)pG)->numlines;
                ++((Uz_Globs *)pG)->lines;
                if (((Uz_Globs *)pG)->lines >= ((Uz_Globs *)pG)->height)
                    (*((Uz_Globs *)pG)->mpause)((zvoid *)pG,
                      LoadFarString(MorePrompt), 1);
            }
#endif /* def MORE */
            if (MSG_STDERR(flag) && ((Uz_Globs *)pG)->UzO.tflag &&
                !isatty(1) && isatty(2))
            {
                /* error output from testing redirected:  also send to stderr */
                putc('\n', stderr);
                fflush(stderr);
            }
#ifdef OS2DLL
        } else
           REDIRECTC('\n');
#endif
        ((Uz_Globs *)pG)->sol = TRUE;
    }

    /* put zipfile name, filename and/or error/warning keywords here */

#ifdef MORE
    if (((Uz_Globs *)pG)->M_flag
# ifdef OS2DLL
         && !((Uz_Globs *)pG)->redirect_text
# endif
                                                 )
    {
        while (p < endbuf) {
            if (*p == '\n') {
# if defined(SCREENWIDTH) && defined(SCREENLWRAP)
                islinefeed = TRUE;
            } else if (SCREENLWRAP) {
                if (*p == '\r') {
                    ((Uz_Globs *)pG)->chars = 0;
                } else {
#  ifdef TABSIZE
                    if (*p == '\t')
                        ((Uz_Globs *)pG)->chars +=
                            (TABSIZE - (((Uz_Globs *)pG)->chars % TABSIZE));
                    else
#  endif /* def TABSIZE */
                        ++((Uz_Globs *)pG)->chars;

                    if (((Uz_Globs *)pG)->chars >= ((Uz_Globs *)pG)->width)
                        islinefeed = TRUE;
                }
            }
            if (islinefeed) {
                islinefeed = FALSE;
                ((Uz_Globs *)pG)->chars = 0;
# endif /* defined(SCREENWIDTH) && defined(SCREENLWRAP) */
                ++((Uz_Globs *)pG)->numlines;
                ++((Uz_Globs *)pG)->lines;
                if (((Uz_Globs *)pG)->lines >= ((Uz_Globs *)pG)->height)
                {
                    if ((error = WriteTxtErr(q, p-q+1, outfp)) != 0)
                        return error;
                    fflush(outfp);
                    ((Uz_Globs *)pG)->sol = TRUE;
                    q = p + 1;
                    (*((Uz_Globs *)pG)->mpause)((zvoid *)pG,
                      LoadFarString(MorePrompt), 1);
                }
            }
            INCSTR(p);
        } /* end while */
        size = (ulg)(p - q);   /* remaining text */
    }
#endif /* def MORE */

    if (size) {
#ifdef OS2DLL
        if (!((Uz_Globs *)pG)->redirect_text) {
#endif
            if ((error = WriteTxtErr(q, size, outfp)) != 0)
                return error;
#ifndef VMS     /* 2005-09-16 SMS.  See note at "WriteTxtErr()", above. */
            fflush(outfp);
#endif
            if (MSG_STDERR(flag) && ((Uz_Globs *)pG)->UzO.tflag &&
                !isatty(1) && isatty(2))
            {
                /* error output from testing redirected:  also send to stderr */
                if ((error = WriteTxtErr(q, size, stderr)) != 0)
                    return error;
                fflush(stderr);
            }
#ifdef OS2DLL
        } else {                /* GRR:  this is ugly:  hide with macro */
            if ((error = REDIRECTPRINT(q, size)) != 0)
                return error;
        }
#endif /* def OS2DLL */
        ((Uz_Globs *)pG)->sol = (endbuf[-1] == '\n');
    }
    return 0;

} /* end function UzpMessagePrnt() */




#ifdef DLL

/*****************************/
/* Function UzpMessageNull() */  /* convenience routine for no output at all */
/*****************************/

int UZ_EXP UzpMessageNull(pG, buf, size, flag)
    zvoid *pG;    /* globals struct:  always passed */
    uch *buf;     /* preformatted string to be printed */
    ulg size;     /* length of string (may include nulls) */
    int flag;     /* flag bits */
{
    return 0;

} /* end function UzpMessageNull() */

#endif /* def DLL */




/***********************/
/* Function UzpInput() */   /* GRR:  this is a placeholder for now */
/***********************/

int UZ_EXP UzpInput(pG, buf, size, flag)
    zvoid *pG;    /* globals struct:  always passed */
    uch *buf;     /* preformatted string to be printed */
    int *size;    /* (address of) size of buf and of returned string */
    int flag;     /* flag bits (bit 0: no echo) */
{
    /* tell picky compilers to shut up about "unused variable" warnings */
    pG = pG; buf = buf; flag = flag;

    *size = 0;
    return 0;

} /* end function UzpInput() */




#if !defined(WINDLL) && !defined(MACOS) && !defined( DLL)

/***************************/
/* Function UzpMorePause() */
/***************************/

void UZ_EXP UzpMorePause(pG, prompt, flag)
    zvoid *pG;            /* globals struct:  always passed */
    ZCONST char *prompt;  /* "--More--" prompt */
    int flag;             /* 0 = any char OK; 1 = accept only '\n', ' ', q */
{
    uch c;
    FILE *outfp;

/* 2012-11-17 SMS.
 * On VMS, match the message output file because, as usual, carriage
 * control differs on VMS, and mixing stderr and stdout causes spurious
 * blank lines and similar problems.  If anyone else cares, then this
 * scheme could be used elsewhere, too.  Note that VMS does not need to
 * use fflush() for terminal output.
 */
# ifdef VMS
    outfp = ((Uz_Globs *)pG)->msgfp;
    if (outfp == NULL)
        outfp = stderr; /* (For emergency use only.) */
# else /* def VMS */
    outfp = stderr;
# endif /* def VMS [else] */

/*---------------------------------------------------------------------------
    Print a prompt and wait for the user to press a key, then erase prompt
    if possible.
  ---------------------------------------------------------------------------*/

    if (!((Uz_Globs *)pG)->sol)
        fprintf( outfp, "\n");
    /* numlines may or may not be used: */
    fprintf( outfp, prompt, ((Uz_Globs *)pG)->numlines);
# ifndef VMS
    fflush( outfp);
# endif /* ndef VMS */
    if (flag & 1) {
        do {
            c = (uch)FGETCH(0);
        } while (
# ifdef THEOS
                 c != 17 &&     /* standard QUIT key */
# endif
                 c != '\r' && c != '\n' && c != ' ' && c != 'q' && c != 'Q');
    } else
        c = (uch)FGETCH(0);

    /* newline was not echoed, so cover up prompt line */
    fprintf( outfp, LoadFarString(HidePrompt));
# ifndef VMS
    fflush( outfp);
# endif /* ndef VMS */

    if (
# ifdef THEOS
        (c == 17) ||            /* standard QUIT key */
# endif
        (ToLower(c) == 'q')) {
        DESTROYGLOBALS();
        EXIT(PK_COOL);
    }

    ((Uz_Globs *)pG)->sol = TRUE;

# ifdef MORE
    /* space for another screen, enter for another line. */
    if ((flag & 1) && c == ' ')
        ((Uz_Globs *)pG)->lines = 0;
# endif /* def MORE */

} /* end function UzpMorePause() */

#endif /* !defined(WINDLL) && !defined(MACOS) && !defined( DLL) */




#ifndef WINDLL

/**************************/
/* Function UzpPassword() */
/**************************/

int UZ_EXP UzpPassword (pG, rcnt, pwbuf, size, zfn, efn)
    zvoid *pG;         /* pointer to UnZip's internal global vars */
    int *rcnt;         /* retry counter */
    char *pwbuf;       /* buffer for password */
    int size;          /* Usable size of password buffer */
    ZCONST char *zfn;  /* name of zip archive */
    ZCONST char *efn;  /* name of archive entry being processed */
{
# ifdef IZ_CRYPT_ANY
    int r = IZ_PW_ENTERED;
    char *m;
    char *prompt;

#  ifndef REENTRANT
    /* tell picky compilers to shut up about "unused variable" warnings */
    pG = pG;
#  endif

    if (*rcnt == 0) {           /* First call for current entry */
        *rcnt = 2;
        if ((prompt = (char *)malloc(2*FILNAMSIZ + 15)) != (char *)NULL) {
            sprintf(prompt, LoadFarString(PasswPrompt),
                    FnFilter1(zfn), FnFilter2(efn));
            m = prompt;
        } else
            m = (char *)LoadFarString(PasswPrompt2);
    } else {                    /* Retry call, previous password was wrong */
        (*rcnt)--;
        prompt = NULL;
        m = (char *)LoadFarString(PasswRetry);
    }

    m = getp(__G__ m, pwbuf, size);
    if (prompt != (char *)NULL) {
        izu_free(prompt);
    }
    if (m == (char *)NULL) {
        r = IZ_PW_ERROR;
    }
    else if (*pwbuf == '\0') {
        r = IZ_PW_CANCELALL;
    }
    return r;

# else /* def IZ_CRYPT_ANY */
    /* tell picky compilers to shut up about "unused variable" warnings */
    pG = pG; rcnt = rcnt; pwbuf = pwbuf; size = size; zfn = zfn; efn = efn;

    return IZ_PW_ERROR;  /* internal error; function should never get called */
# endif /* def IZ_CRYPT_ANY [else] */

} /* end function UzpPassword() */

#endif /* ndef WINDLL */




#if !defined( WINDLL) && !defined( DLL)

/**********************/
/* Function handler() */
/**********************/

void handler(signal)   /* upon interrupt, turn on echo and exit cleanly */
    int signal;
{
    GETGLOBALS();

# if !(defined(SIGBUS) || defined(SIGSEGV))     /* add a newline if not at */
    (*G.message)((zvoid *)&G, slide, 0L, 0x41); /*  start of line (to stderr; */
# endif                                         /*  slide[] should be safe) */

    /* Restore the original terminal echo setting. */
    echorig();

# ifdef SIGBUS
    if (signal == SIGBUS) {
        Info(slide, 0x421, ((char *)slide, LoadFarString(ZipfileCorrupt),
          "bus error"));
        DESTROYGLOBALS();
        EXIT(PK_BADERR);
    }
# endif /* SIGBUS */

# ifdef SIGILL
    if (signal == SIGILL) {
        Info(slide, 0x421, ((char *)slide, LoadFarString(ZipfileCorrupt),
          "illegal instruction"));
        DESTROYGLOBALS();
        EXIT(PK_BADERR);
    }
# endif /* SIGILL */

# ifdef SIGSEGV
    if (signal == SIGSEGV) {
        Info(slide, 0x421, ((char *)slide, LoadFarString(ZipfileCorrupt),
          "segmentation violation"));
        DESTROYGLOBALS();
        EXIT(PK_BADERR);
    }
# endif /* SIGSEGV */

    /* probably ctrl-C */
    DESTROYGLOBALS();
# if defined(AMIGA) && defined(__SASC)
    _abort();
# endif
    EXIT(IZ_CTRLC);       /* was EXIT(0), then EXIT(PK_ERR) */
}

#endif /* !defined( WINDLL) && !defined( DLL) */




#if !defined(VMS) && !defined(CMS_MVS)
# if !defined(OS2) || defined(TIMESTAMP)

#  if !defined(HAVE_MKTIME) || defined(WIN32)
/* also used in amiga/filedate.c and win32/win32.c */
ZCONST ush ydays[] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
#  endif




/*******************************/
/* Function dos_to_unix_time() */ /* used for freshening/updating/timestamps */
/*******************************/

time_t dos_to_unix_time(dosdatetime)
    ulg dosdatetime;
{
    time_t m_time;

#  ifdef HAVE_MKTIME

    ZCONST time_t now = time(NULL);
    struct tm *tm;
#   define YRBASE  1900

    tm = localtime(&now);
    tm->tm_isdst = -1;          /* let mktime determine if DST is in effect */

    /* MS-DOS date-time format:
     * Bits:    31:25     24:21   20:16   15:11  10:05  04:00
     * Value: year-1980   month    day     hour   min   sec/2
     *                   (1-12)  (1-31)
     */

    /* dissect date */
    tm->tm_year = ((int)(dosdatetime >> 25) & 0x7f) + (1980 - YRBASE);
    tm->tm_mon  = ((int)(dosdatetime >> 21) & 0x0f) - 1;
    tm->tm_mday = ((int)(dosdatetime >> 16) & 0x1f);

    /* dissect time */
    tm->tm_hour = (int)((unsigned)dosdatetime >> 11) & 0x1f;
    tm->tm_min  = (int)((unsigned)dosdatetime >> 5) & 0x3f;
    tm->tm_sec  = (int)((unsigned)dosdatetime << 1) & 0x3e;

    m_time = mktime(tm);
    NATIVE_TO_TIMET(m_time)     /* NOP unless MSC 7.0 or Macintosh */
    TTrace((stderr, "  final m_time  =       %lu\n", (ulg)m_time));

#  else /* def HAVE_MKTIME */

    int yr, mo, dy, hh, mm, ss;
#   ifdef TOPS20
#    define YRBASE  1900
    struct tmx *tmx;
    char temp[20];
#   else /* def TOPS20 */
#    define YRBASE  1970
    int leap;
    unsigned days;
    struct tm *tm;
#    if !defined(MACOS) && !defined(RISCOS) && !defined(QDOS) && !defined(TANDEM)
#     ifdef WIN32
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res;
#     else /* def WIN32 */
#      ifndef BSD4_4   /* GRR:  change to !defined(MODERN) ? */
#       if defined(BSD) || defined(MTS) || defined(__GO32__)
    struct timeb tbp;
#       else /* defined(BSD) || defined(MTS) || defined(__GO32__) */
#        ifdef DECLARE_TIMEZONE
    extern time_t timezone;
#        endif /* def DECLARE_TIMEZONE */
#       endif /* defined(BSD) || defined(MTS) || defined(__GO32__) [else] */
#      endif /* ndef BSD4_4 */
#     endif /* def WIN32 [else] */
#    endif /* !defined(MACOS) && !defined(RISCOS) && !defined(QDOS) && !defined(TANDEM) */
#   endif /* def TOPS20 [else] */

    /* MS-DOS date-time format:
     * Bits:    31:25     24:21   20:16   15:11  10:05  04:00
     * Value: year-1980   month    day     hour   min   sec/2
     *                   (1-12)  (1-31)
     */

    /* dissect date */
    yr = ((int)(dosdatetime >> 25) & 0x7f) + (1980 - YRBASE);
    mo = ((int)(dosdatetime >> 21) & 0x0f) - 1;
    dy = ((int)(dosdatetime >> 16) & 0x1f) - 1;

    /* dissect time */
    hh = (int)((unsigned)dosdatetime >> 11) & 0x1f;
    mm = (int)((unsigned)dosdatetime >> 5) & 0x3f;
    ss = (int)((unsigned)dosdatetime & 0x1f) * 2;

    /* 2012-11-26 SMS.  (OUSPG report.)
     * Return the mktime() error status for obviously invalid values.
     * (This avoids the use of "ydays[mo]" for an invalid "mo".  Other
     * invalid values should cause less trouble.)
     */
    if ((mo < 0) || (mo > 11) || (dy < 0) || (ss > 59))
        return (time_t)-1;

#   ifdef TOPS20
    tmx = (struct tmx *)malloc(sizeof(struct tmx));
    sprintf (temp, "%02d/%02d/%02d %02d:%02d:%02d", mo+1, dy+1, yr, hh, mm, ss);
    time_parse(temp, tmx, (char *)0);
    m_time = time_make(tmx);
    izu_free(tmx);

#   else /* def TOPS20 */

/*---------------------------------------------------------------------------
    Calculate the number of seconds since the epoch, usually 1 January 1970.
  ---------------------------------------------------------------------------*/

    /* leap = # of leap yrs from YRBASE up to but not including current year */
    leap = ((yr + YRBASE - 1) / 4);   /* leap year base factor */

    /* calculate days from BASE to this year and add expired days this year */
    days = (yr * 365) + (leap - 492) + ydays[mo];

    /* if year is a leap year and month is after February, add another day */
    if ((mo > 1) && ((yr+YRBASE)%4 == 0) && ((yr+YRBASE) != 2100))
        ++days;                 /* OK through 2199 */

    /* convert date & time to seconds relative to 00:00:00, 01/01/YRBASE */
    m_time = (time_t)((unsigned long)(days + dy) * 86400L +
                      (unsigned long)hh * 3600L +
                      (unsigned long)(mm * 60 + ss));
      /* - 1;   MS-DOS times always rounded up to nearest even second */
    TTrace((stderr, "dos_to_unix_time:\n"));
    TTrace((stderr, "  m_time before timezone = %lu\n", (ulg)m_time));

/*---------------------------------------------------------------------------
    Adjust for local standard timezone offset.
  ---------------------------------------------------------------------------*/

#    if !defined(MACOS) && !defined(RISCOS) && !defined(QDOS) && !defined(TANDEM)
#     ifdef WIN32
    /* account for timezone differences */
    res = GetTimeZoneInformation(&tzinfo);
    if (res != TIME_ZONE_ID_INVALID)
    {
    m_time += 60*(tzinfo.Bias);
#     else /* def WIN32 */
#      if defined(BSD) || defined(MTS) || defined(__GO32__)
#       ifdef BSD4_4
    if ( (dosdatetime >= DOSTIME_2038_01_18) &&
         (m_time < (time_t)0x70000000L) )
        m_time = U_TIME_T_MAX;  /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t)0L)    /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX;  /*  -> saturate at max signed time_t value */
    if ((tm = localtime(&m_time)) != (struct tm *)NULL)
        m_time -= tm->tm_gmtoff;                /* sec. EAST of GMT: subtr. */
#       else /* def BSD4_4 */
    ftime(&tbp);                                /* get `timezone' */
    m_time += tbp.timezone * 60L;               /* seconds WEST of GMT:  add */
#       endif /* def BSD4_4 */
#      else /* defined(BSD) || defined(MTS) || defined(__GO32__) */
    /* tzset was already called at start of process_zipfiles() */
    /* tzset(); */              /* set `timezone' variable */
#       ifndef __BEOS__                /* BeOS DR8 has no timezones... */
    m_time += timezone;         /* seconds WEST of GMT:  add */
#       endif /* ndef __BEOS__ */
#      endif /* defined(BSD) || defined(MTS) || defined(__GO32__) [else] */
#     endif /* def WIN32 [else] */
    TTrace((stderr, "  m_time after timezone =  %lu\n", (ulg)m_time));

/*---------------------------------------------------------------------------
    Adjust for local daylight savings (summer) time.
  ---------------------------------------------------------------------------*/

#     ifndef BSD4_4  /* (DST already added to tm_gmtoff, so skip tm_isdst) */
    if ( (dosdatetime >= DOSTIME_2038_01_18) &&
         (m_time < (time_t)0x70000000L) )
        m_time = U_TIME_T_MAX;  /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t)0L)    /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX;  /*  -> saturate at max signed time_t value */
    TIMET_TO_NATIVE(m_time)     /* NOP unless MSC 7.0 or Macintosh */
    if (((tm = localtime((time_t *)&m_time)) != NULL) && tm->tm_isdst)
#      ifdef WIN32
        m_time += 60L * tzinfo.DaylightBias;    /* adjust with DST bias */
    else
        m_time += 60L * tzinfo.StandardBias;    /* add StdBias (normally 0) */
#      else /* def WIN32 */
        m_time -= 60L * 60L;    /* adjust for daylight savings time */
#      endif /* def WIN32 [else] */
    NATIVE_TO_TIMET(m_time)     /* NOP unless MSC 7.0 or Macintosh */
    TTrace((stderr, "  m_time after DST =       %lu\n", (ulg)m_time));
#     endif /* ndef BSD4_4 */
#     ifdef WIN32
    }
#     endif /* def WIN32 */
#    endif /* !defined(MACOS) && !defined(RISCOS) && !defined(QDOS) && !defined(TANDEM) */
#   endif /* def TOPS20 [else] */

#  endif /* def HAVE_MKTIME [else] */

    if ( (dosdatetime >= DOSTIME_2038_01_18) &&
         (m_time < (time_t)0x70000000L) )
        m_time = U_TIME_T_MAX;  /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t)0L)    /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX;  /*  -> saturate at max signed time_t value */

    return m_time;

} /* end function dos_to_unix_time() */

# endif /* !defined(OS2) || defined(TIMESTAMP) */
#endif /* !defined(VMS) && !defined(CMS_MVS) */




#if !defined(VMS) && !defined(OS2) && !defined(CMS_MVS)

/******************************/
/* Function check_for_newer() */  /* used for overwriting/freshening/updating */
/******************************/

int check_for_newer(__G__ filename)  /* return 1 if existing file is newer */
    __GDEF                           /*  or equal; 0 if older; -1 if doesn't */
    char *filename;                  /*  exist yet */
{
    time_t existing, archive;
# ifdef USE_EF_UT_TIME
    iztimes z_utime;
# endif
# ifdef AOS_VS
    long    dyy, dmm, ddd, dhh, dmin, dss;


    dyy = (lrec.last_mod_dos_datetime >> 25) + 1980;
    dmm = (lrec.last_mod_dos_datetime >> 21) & 0x0f;
    ddd = (lrec.last_mod_dos_datetime >> 16) & 0x1f;
    dhh = (lrec.last_mod_dos_datetime >> 11) & 0x1f;
    dmin = (lrec.last_mod_dos_datetime >> 5) & 0x3f;
    dss = (lrec.last_mod_dos_datetime & 0x1f) * 2;

    /* under AOS/VS, file times can only be set at creation time,
     * with the info in a special DG format.  Make sure we can create
     * it here - we delete it later & re-create it, whether or not
     * it exists now.
     */
    if (!zvs_create(filename, (((ulg)dgdate(dmm, ddd, dyy)) << 16) |
        (dhh*1800L + dmin*30L + dss/2L), -1L, -1L, (char *) -1, -1, -1, -1))
        return DOES_NOT_EXIST;
# endif /* AOS_VS */

    Trace((stderr, "check_for_newer:  doing stat(%s)\n", FnFilter1(filename)));
    if (SSTAT(filename, &G.statbuf)) {
        Trace((stderr,
          "check_for_newer:  stat(%s) returns %d:  file does not exist\n",
          FnFilter1(filename), SSTAT(filename, &G.statbuf)));
# ifdef SYMLINKS
        Trace((stderr, "check_for_newer:  doing lstat(%s)\n",
          FnFilter1(filename)));
        /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
        if (lstat(filename, &G.statbuf) == 0) {
            Trace((stderr,
              "check_for_newer:  lstat(%s) returns 0:  symlink does exist\n",
              FnFilter1(filename)));
            if (QCOND2 && !IS_OVERWRT_ALL)
                Info(slide, 0, ((char *)slide, LoadFarString(FileIsSymLink),
                  FnFilter1(filename), " with no real file"));
            return EXISTS_AND_OLDER;   /* symlink dates are meaningless */
        }
# endif /* def SYMLINKS */
        return DOES_NOT_EXIST;
    }
    Trace((stderr, "check_for_newer:  stat(%s) returns 0:  file exists\n",
      FnFilter1(filename)));

# ifdef SYMLINKS
    /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
    if (lstat(filename, &G.statbuf) == 0 && S_ISLNK(G.statbuf.st_mode)) {
        Trace((stderr, "check_for_newer:  %s is a symbolic link\n",
          FnFilter1(filename)));
        if (QCOND2 && !IS_OVERWRT_ALL)
            Info(slide, 0, ((char *)slide, LoadFarString(FileIsSymLink),
              FnFilter1(filename), ""));
        return EXISTS_AND_OLDER;   /* symlink dates are meaningless */
    }
# endif /* def SYMLINKS */

    NATIVE_TO_TIMET(G.statbuf.st_mtime)   /* NOP unless MSC 7.0 or Macintosh */

# ifdef USE_EF_UT_TIME
    /* The `Unix extra field mtime' should be used for comparison with the
     * time stamp of the existing file >>>ONLY<<< when the EF info is also
     * used to set the modification time of the extracted file.
     */
    if (G.extra_field &&
#  ifdef IZ_CHECK_TZ
        G.tz_is_valid &&
#  endif
        (ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length, 0,
                          G.lrec.last_mod_dos_datetime, &z_utime, NULL)
         & EB_UT_FL_MTIME))
    {
        TTrace((stderr, "check_for_newer:  using Unix extra field mtime\n"));
        existing = G.statbuf.st_mtime;
        archive  = z_utime.mtime;
    } else {
        /* round up existing filetime to nearest 2 seconds for comparison,
         * but saturate in case of arithmetic overflow
         */
        existing = ((G.statbuf.st_mtime & 1) &&
                    (G.statbuf.st_mtime + 1 > G.statbuf.st_mtime)) ?
                   G.statbuf.st_mtime + 1 : G.statbuf.st_mtime;
        archive  = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
    }
# else /* def USE_EF_UT_TIME */
    /* round up existing filetime to nearest 2 seconds for comparison,
     * but saturate in case of arithmetic overflow
     */
    existing = ((G.statbuf.st_mtime & 1) &&
                (G.statbuf.st_mtime + 1 > G.statbuf.st_mtime)) ?
               G.statbuf.st_mtime + 1 : G.statbuf.st_mtime;
    archive  = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
# endif /* def USE_EF_UT_TIME [else] */

    TTrace((stderr, "check_for_newer:  existing %lu, archive %lu, e-a %ld\n",
      (ulg)existing, (ulg)archive, (long)(existing-archive)));

    return (existing >= archive);

} /* end function check_for_newer() */

#endif /* !defined(VMS) && !defined(OS2) && !defined(CMS_MVS) */

#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
int check_for_newerw(__G__ filenamew)  /* return 1 if existing file is newer */
    __GDEF                           /*  or equal; 0 if older; -1 if doesn't */
    wchar_t *filenamew;                  /*  exist yet */
{
    time_t existing, archive;
# ifdef USE_EF_UT_TIME
    iztimes z_utime;
# endif

    Trace((stderr, "check_for_newer:  doing stat(%s)\n", FnFilter1(filename)));
    if (SSTATW(filenamew, &G.statbuf)) {
        Trace((stderr,
          "check_for_newer:  stat(%s) returns %d:  file does not exist\n",
          FnFilter1(filename), SSTAT(filename, &G.statbuf)));
# ifdef SYMLINKS
        Trace((stderr, "check_for_newer:  doing lstat(%s)\n",
          FnFilter1(filename)));
        /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
        if (lstatw(filenamew, &G.statbuf) == 0) {
            Trace((stderr,
              "check_for_newer:  lstat(%s) returns 0:  symlink does exist\n",
              FnFilter1(filename)));
            if (QCOND2 && !IS_OVERWRT_ALL)
                Info(slide, 0, ((char *)slide, LoadFarString(FileIsSymLink),
                  FnFilterW1(filenamew), " with no real file"));
            return EXISTS_AND_OLDER;   /* symlink dates are meaningless */
        }
# endif /* def SYMLINKS */
        return DOES_NOT_EXIST;
    }
    Trace((stderr, "check_for_newer:  stat(%s) returns 0:  file exists\n",
      FnFilter1(filename)));

# ifdef SYMLINKS
    /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
    if (lstatw(filenamew, &G.statbuf) == 0 && S_ISLNK(G.statbuf.st_mode)) {
        Trace((stderr, "check_for_newer:  %s is a symbolic link\n",
          FnFilter1(filename)));
        if (QCOND2 && !IS_OVERWRT_ALL)
            Info(slide, 0, ((char *)slide, LoadFarString(FileIsSymLink),
              FnFilterW1(filenamew), ""));
        return EXISTS_AND_OLDER;   /* symlink dates are meaningless */
    }
# endif /* def SYMLINKS */

    NATIVE_TO_TIMET(G.statbuf.st_mtime)   /* NOP unless MSC 7.0 or Macintosh */

# ifdef USE_EF_UT_TIME
    /* The `Unix extra field mtime' should be used for comparison with the
     * time stamp of the existing file >>>ONLY<<< when the EF info is also
     * used to set the modification time of the extracted file.
     */
    if (G.extra_field &&
#  ifdef IZ_CHECK_TZ
        G.tz_is_valid &&
#  endif
        (ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length, 0,
                          G.lrec.last_mod_dos_datetime, &z_utime, NULL)
         & EB_UT_FL_MTIME))
    {
        TTrace((stderr, "check_for_newer:  using Unix extra field mtime\n"));
        existing = G.statbuf.st_mtime;
        archive  = z_utime.mtime;
    } else {
        /* round up existing filetime to nearest 2 seconds for comparison,
         * but saturate in case of arithmetic overflow
         */
        existing = ((G.statbuf.st_mtime & 1) &&
                    (G.statbuf.st_mtime + 1 > G.statbuf.st_mtime)) ?
                   G.statbuf.st_mtime + 1 : G.statbuf.st_mtime;
        archive  = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
    }
# else /* def USE_EF_UT_TIME */
    /* round up existing filetime to nearest 2 seconds for comparison,
     * but saturate in case of arithmetic overflow
     */
    existing = ((G.statbuf.st_mtime & 1) &&
                (G.statbuf.st_mtime + 1 > G.statbuf.st_mtime)) ?
               G.statbuf.st_mtime + 1 : G.statbuf.st_mtime;
    archive  = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
# endif /* def USE_EF_UT_TIME [else] */

    TTrace((stderr, "check_for_newer:  existing %lu, archive %lu, e-a %ld\n",
      (ulg)existing, (ulg)archive, (long)(existing-archive)));

    return (existing >= archive);

} /* end function check_for_newerw() */
#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */




/************************/
/* Function do_string() */
/************************/

int do_string(__G__ length, option)   /* return PK-type error code */
    __GDEF
    unsigned int length;        /* without prototype, ush converted to this */
    int option;
{
    unsigned comment_bytes_left;
    unsigned int block_len;
    int error=PK_OK;
#ifdef AMIGA
    char tmp_fnote[2 * AMIGA_FILENOTELEN];   /* extra room for squozen chars */
#endif
    ush flag;


/*---------------------------------------------------------------------------
    This function processes arbitrary-length (well, usually) strings.  Four
    major options are allowed:  SKIP, wherein the string is skipped (pretty
    logical, eh?); DISPLAY, wherein the string is printed to standard output
    after undergoing any necessary or unnecessary character conversions;
    DS_FN, wherein the string is put into the filename[] array after under-
    going appropriate conversions (including case-conversion, if that is
    indicated: see the global variable pInfo->lcflag); and EXTRA_FIELD,
    wherein the `string' is assumed to be an extra field and is copied to
    the (freshly malloced) buffer G.extra_field.  The third option should
    be OK since filename is dimensioned at 1025, but we check anyway.

    The string, by the way, is assumed to start at the current file-pointer
    position; its length is given by 'length'.  So start off by checking the
    length of the string:  if zero, we're already done.
  ---------------------------------------------------------------------------*/

    /* 2012-05-01 SMS.
     *
     *     if (!length)
     *         return PK_COOL;
     *
     * Well, no, if "length" is zero, then we're _not_ "already done".
     * Exiting immediately would leave the destination buffer
     * unmodified, so it would retain its previous value, instead of
     * what should be a null string.  (The APPNOTE says, "If input came
     * from standard input, the file name length is set to zero."  So,
     * we need to handle that case accordingly.)  Cases DS_FN and DS_FNL
     * now substitute NULL_NAME_REPL (unzpriv.h) for a null archive
     * member name.  Other cases must handle a null name appropriately.
     */

    switch (option) {

#if defined(SFX) && defined(CHEAP_SFX_AUTORUN)
    /*
     * Special case: See if the comment begins with an autorun command line.
     * Save that and display (or skip) the remainder.
     */

    case CHECK_AUTORUN:
    case CHECK_AUTORUN_Q:
        comment_bytes_left = length;
        if (length >= 10)
        {
            block_len = readbuf(__G__ (char *)G.outbuf, 10);
            if (block_len == 0)
                return PK_EOF;
            comment_bytes_left -= block_len;
            G.outbuf[block_len] = '\0';
            if (!strcmp((char *)G.outbuf, "$AUTORUN$>")) {
                char *eol;
                length -= 10;
                block_len = readbuf(__G__ G.autorun_command,
                             IZ_MIN(length, sizeof(G.autorun_command)-1));
                if (block_len == 0)
                    return PK_EOF;
                comment_bytes_left -= block_len;
                G.autorun_command[block_len] = '\0';
                A_TO_N(G.autorun_command);
                eol = strchr(G.autorun_command, '\n');
                if (!eol)
                    eol = G.autorun_command + strlen(G.autorun_command) - 1;
                length -= eol + 1 - G.autorun_command;
                while (eol >= G.autorun_command && isspace(*eol))
                    *eol-- = '\0';
# if defined(WIN32) && !defined(_WIN32_WCE)
                /* Win9x console always uses OEM character coding, and
                   WinNT console is set to OEM charset by default, too */
                INTERN_TO_OEM(G.autorun_command, G.autorun_command);
# endif /* defined(WIN32) && !defined(_WIN32_WCE) */
            }
        }
        if (option == CHECK_AUTORUN_Q)  /* don't display the remainder */
            length = 0;
        /* seek to beginning of remaining part of comment -- rewind if */
        /* displaying entire comment, or skip to end if discarding it  */
        seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes +
                  (G.inptr - G.inbuf) + comment_bytes_left - length);
        if (!length)
            break;
        /*  FALL THROUGH...  */
#endif /* defined(SFX) && defined(CHEAP_SFX_AUTORUN) */

    /*
     * First normal case:  print string on standard output.  First set loop
     * variables, then loop through the comment in chunks of OUTBUFSIZ bytes,
     * converting formats and printing as we go.  The second half of the
     * loop conditional was added because the file might be truncated, in
     * which case comment_bytes_left will remain at some non-zero value for
     * all time.  outbuf and slide are used as scratch buffers because they
     * are available (we should be either before or in between any file pro-
     * cessing).
     */

    case DISPLAY:
    case DISPL_8:
        comment_bytes_left = length;
        block_len = OUTBUFSIZ;       /* for the while statement, first time */
        while (comment_bytes_left > 0 && block_len > 0) {
            register uch *p = G.outbuf;
            register uch *q = G.outbuf;

            if ((block_len = readbuf(__G__ (char *)G.outbuf,
                   IZ_MIN((unsigned)OUTBUFSIZ, comment_bytes_left))) == 0)
                return PK_EOF;
            comment_bytes_left -= block_len;

            /* this is why we allocated an extra byte for outbuf:  terminate
             *  with zero (ASCIIZ) */
            G.outbuf[block_len] = '\0';

            /* remove all ASCII carriage returns from comment before printing
             * (since used before A_TO_N(), check for CR instead of '\r')
             */
            while (*p) {
                while (*p == CR)
                    ++p;
                *q++ = *p++;
            }
            /* could check whether (p - outbuf) == block_len here */
            *q = '\0';

            if (option == DISPL_8) {
                /* translate the text coded in the entry's host-dependent
                   "extended ASCII" charset into the compiler's (system's)
                   internal text code page */
                Ext_ASCII_TO_Native((char *)G.outbuf, G.pInfo->hostnum,
                                    G.pInfo->hostver, G.pInfo->HasUxAtt,
                                    FALSE);
#ifdef WINDLL
                /* translate to ANSI (RTL internal codepage may be OEM) */
                INTERN_TO_ISO((char *)G.outbuf, (char *)G.outbuf);
#else /* def WINDLL */
# if defined(WIN32) && !defined(_WIN32_WCE)
                /* Win9x console always uses OEM character coding, and
                   WinNT console is set to OEM charset by default, too */
                INTERN_TO_OEM((char *)G.outbuf, (char *)G.outbuf);
# endif /* defined(WIN32) && !defined(_WIN32_WCE) */
#endif /* def WINDLL [else] */
            } else {
                A_TO_N(G.outbuf);   /* translate string to native */
            }

#ifdef WINDLL
            /* ran out of local mem -- had to cheat */
            win_fprintf((zvoid *)&G, stdout, (int)(q-G.outbuf),
                        (char *)G.outbuf);
            win_fprintf((zvoid *)&G, stdout, 2, (char *)"\n\n");
#else /* def WINDLL */
# ifdef NOANSIFILT       /* GRR:  can ANSI be used with EBCDIC? */
            (*G.message)((zvoid *)&G, G.outbuf, (ulg)(q-G.outbuf), 0);
# else /* def NOANSIFILT */
            /* ASCII, filter out ANSI escape sequences and handle ^S (pause) */
            p = G.outbuf - 1;
            q = slide;
            while (*++p) {
                int pause = FALSE;

                if (*p == 0x1B) {          /* ASCII escape char */
                    *q++ = '^';
                    *q++ = '[';
                } else if (*p == 0x13) {   /* ASCII ^S (pause) */
                    pause = TRUE;
                    if (p[1] == LF)        /* ASCII LF */
                        *q++ = *++p;
                    else if (p[1] == CR && p[2] == LF) {  /* ASCII CR LF */
                        *q++ = *++p;
                        *q++ = *++p;
                    }
                } else
                    *q++ = *p;
                if ((unsigned)(q-slide) > WSIZE-3 || pause) {   /* flush */
                    (*G.message)((zvoid *)&G, slide, (ulg)(q-slide), 0);
                    q = slide;
                    if (pause && G.extract_flag) /* don't pause for list/test */
                        (*G.mpause)((zvoid *)&G, LoadFarString(QuitPrompt), 0);
                }
            }
            (*G.message)((zvoid *)&G, slide, (ulg)(q-slide), 0);
# endif /* def NOANSIFILT [else] */
#endif /* def WINDLL [else] */
        }
        /* add '\n' if not at start of line */
        (*G.message)((zvoid *)&G, slide, 0L, 0x40);
        break;

    /*
     * Second case:  read string into filename[] array.  The filename should
     * never ever be longer than FILNAMSIZ-1 (1024), but for now we'll check,
     * just to be sure.
     */

    case DS_FN:
    case DS_FN_L:
#ifdef UNICODE_SUPPORT
        if (length == 0)
        {
            /* Replace a null name. */
            extent fnbufsiz = sizeof( NULL_NAME_REPL);

            if (G.fnfull_bufsize < fnbufsiz)
            {
                if (G.filename_full)
                    izu_free(G.filename_full);
                G.filename_full = malloc( fnbufsiz);
                if (G.filename_full == NULL)
                    return PK_MEM;
                G.fnfull_bufsize = fnbufsiz;
            }
            strcpy( G.filename_full, NULL_NAME_REPL);
            return PK_COOL;
        }
        /* get the whole filename as need it for Unicode checksum */
        if (G.fnfull_bufsize <= length) {
            extent fnbufsiz = FILNAMSIZ;

            if (fnbufsiz <= length)
                fnbufsiz = length + 1;
            if (G.filename_full)
                izu_free(G.filename_full);
            G.filename_full = malloc(fnbufsiz);
            if (G.filename_full == NULL)
                return PK_MEM;
            G.fnfull_bufsize = fnbufsiz;
        }
        if (readbuf(__G__ G.filename_full, length) == 0)
            return PK_EOF;
        G.filename_full[length] = '\0';      /* terminate w/zero:  ASCIIZ */

        /* if needed, chop off end so standard filename is a valid length */
        if (length >= FILNAMSIZ) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(FilenameTooLongTrunc)));
            error = PK_WARN;
            length = FILNAMSIZ - 1;
        }
        /* no excess size */
        block_len = 0;
        strncpy(G.filename, G.filename_full, length);
        G.filename[length] = '\0';      /* terminate w/zero:  ASCIIZ */
#else /* def UNICODE_SUPPORT */
        if (length == 0)
        {
            strcpy( G.filename, NULL_NAME_REPL);
            return PK_COOL;
        }
        if (length >= FILNAMSIZ) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(FilenameTooLongTrunc)));
            error = PK_WARN;
            /* remember excess length in block_len */
            block_len = length - (FILNAMSIZ - 1);
            length = FILNAMSIZ - 1;
        } else
            /* no excess size */
            block_len = 0;
        if (readbuf(__G__ G.filename, length) == 0)
            return PK_EOF;
        G.filename[length] = '\0';      /* terminate w/zero:  ASCIIZ */
#endif /* def UNICODE_SUPPORT [else] */

        flag = (option==DS_FN_L) ? G.lrec.general_purpose_bit_flag :
         G.crec.general_purpose_bit_flag;

        /* Skip ISO/OEM translation if stored name is UTF-8.
         *
         * 2012-05-20 SMS.
         * Added check for Java "CAFE" extra block, to avoid mistaking
         * Java's mostly-zero header info for an MS-DOS origin.  A more
         * rigorous hostnum test might be easier, but might break other
         * stuff.
         *
         * 2014-09-19 SMS.
         * Also skip translation if user so requests (-0).
         */
        if ((uO.zero_flag == 0) &&
         ((flag & UTF8_BIT) == 0) && (uO.java_cafe <= 0))
        {
          /* translate the Zip entry filename coded in host-dependent "extended
             ASCII" into the compiler's (system's) internal text code page */
          Ext_ASCII_TO_Native(G.filename, G.pInfo->hostnum, G.pInfo->hostver,
                              G.pInfo->HasUxAtt, (option == DS_FN_L));
        }

        if (G.pInfo->lcflag)      /* replace with lowercase filename */
            STRLOWER(G.filename, G.filename);

        if (G.pInfo->vollabel && length > 8 && G.filename[8] == '.') {
            char *p = G.filename+8;
            while (*p++)
                p[-1] = *p;  /* disk label, and 8th char is dot:  remove dot */
        }

        if (!block_len)         /* no overflow, we're done here */
            break;

        /*
         * We truncated the filename, so print what's left and then fall
         * through to the SKIP routine.
         */
        Info(slide, 0x401, ((char *)slide, "[ %s ]\n", FnFilter1(G.filename)));
        length = block_len;     /* SKIP the excess bytes... */
        /*  FALL THROUGH...  */

    /*
     * Third case:  skip string, adjusting readbuf's internal variables
     * as necessary (and possibly skipping to and reading a new block of
     * data).
     */

    case SKIP:
        if (length == 0)
        {
            return PK_COOL;
        }

        /* cur_zipfile_bufstart already takes account of extra_bytes, so don't
         * correct for it twice: */
        seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes +
                  (G.inptr-G.inbuf) + length);
        break;

    /*
     * Fourth case:  assume we're at the start of an "extra field"; malloc
     * storage for it and read data into the allocated space.
     */

    case EXTRA_FIELD:
        /* 2012-12-05 SMS.  Changed to free old storage always, and to
         * null G.extra_field if the length is zero.
         */
        /* Free any previous extra field storage. */
        if (G.extra_field != (uch *)NULL)
            izu_free(G.extra_field);

        /* Return early, if no data are expected. */
        if (length == 0)
        {
            G.extra_field = (uch *)NULL;
            return PK_COOL;
        }
        /* Allocate new extra field storage, and fill it. */
        if ((G.extra_field = (uch *)malloc(length)) == (uch *)NULL) {
            Info(slide, 0x401, ((char *)slide, LoadFarString(ExtraFieldTooLong),
              length));
            /* cur_zipfile_bufstart already takes account of extra_bytes,
             * so don't correct for it twice: */
            seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes +
                      (G.inptr-G.inbuf) + length);
        } else {
            /* 2014-11-13 pstodulk, SMS.
             * http://www.info-zip.org/phpBB3/viewtopic.php?f=7&t=282
             * Faulty archive data may cause use of unset data in the
             * G.extra_field buffer.
             */
            unsigned int len_rb;

            if ((len_rb = readbuf(__G__ (char *)G.extra_field, length)) == 0)
                return PK_EOF;
            if (len_rb < length)
            {
                /* Clear any unfilled bytes in the buffer,
                 * and pretend that we got all that we requested.
                 */
                memset( (char *)G.extra_field+ len_rb, 0, (length- len_rb));
                length = len_rb;
            }
            /* Looks like here is where extra fields are read. */
            /* 2014-12-17 SMS.  (oCERT.org report.)
             * Added test to detect (and message to notify user of)
             * bad Zip64 (EF_PKSZ64) extra field data.
             */
            if (getZip64Data(__G__ G.extra_field, length) != PK_COOL)
            {
                Info(slide, 0x401, ((char *)slide,
                 LoadFarString( ExtraFieldCorrupt), EF_PKSZ64));
                error = PK_WARN;
            }
#ifdef UNICODE_SUPPORT
            /* 2013-02-11 SMS.
             * Free old G.unipath_filename storage, if not already
             * done (G.unipath_filename == G.filename_full).
             */
            if ((G.unipath_filename != NULL) &&
             (G.unipath_filename != G.filename_full))
            {
                izu_free( G.unipath_filename);
            }
            G.unipath_filename = NULL;
            if (G.UzO.U_flag < 2) {
              /* check if GPB11 (General Purpuse Bit 11) is set indicating
                 the standard path and comment are UTF-8 */
              if (G.pInfo->GPFIsUTF8) {
                /* if GPB11 set then filename_full is untruncated UTF-8 */
                if (!(G.unipath_filename = malloc(strlen(G.filename_full)+1)))
                  return PK_MEM;
                strcpy(G.unipath_filename, G.filename_full);
              } else {
                /* Get the Unicode fields if exist */
                getUnicodeData(__G__ G.extra_field, length);
                if (G.unipath_filename && (*G.unipath_filename == '\0')) {
                  /* the standard filename field is UTF-8 */
                  izu_free(G.unipath_filename);
                  G.unipath_filename = G.filename_full;
                }
              }
              if (G.unipath_filename) {
# ifdef UTF8_MAYBE_NATIVE
                if (G.native_is_utf8
#  ifdef UNICODE_WCHAR
                    && (!G.unicode_escape_all)
#  endif /* def UNICODE_WCHAR */
                   ) {
                  strncpy(G.filename, G.unipath_filename, FILNAMSIZ - 1);
                  /* make sure filename is short enough */
                  if (strlen(G.unipath_filename) >= FILNAMSIZ) {
                    G.filename[FILNAMSIZ - 1] = '\0';
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(UFilenameTooLongTrunc)));
                    error = PK_WARN;
                  }
                }
#  ifdef UNICODE_WCHAR
                else
#  endif /* def UNICODE_WCHAR */
# endif /* def UTF8_MAYBE_NATIVE */
# ifdef UNICODE_WCHAR
                {
                  char *fn;

                  /* convert UTF-8 to local character set */
                  fn = utf8_to_local_string(G.unipath_filename,
                                            G.unicode_escape_all);
                  /* make sure filename is short enough */
                  if (strlen(fn) >= FILNAMSIZ) {
                    fn[FILNAMSIZ - 1] = '\0';
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(UFilenameTooLongTrunc)));
                    error = PK_WARN;
                  }
                  /* replace filename with converted UTF-8 */
                  strcpy(G.filename, fn);
                  izu_free(fn);
                }
# endif /* def UNICODE_WCHAR */

# if 0
                /* 2013-02-10 SMS.
                 * This disabled code section does not catch all the
                 * cases.  The new pre-use free() code, above, should.
                 */
                if (G.unipath_filename != G.filename_full)
                  izu_free(G.unipath_full);
                G.unipath_full = NULL;
# endif /* 0 */

              }
# ifdef WIN32_WIDE
#  ifdef DYNAMIC_WIDE_NAME
            /* 2013-02-12 SMS.
             * Free old G.unipath_widefilename storage.
             */
              if (G.unipath_widefilename != NULL)
              {
                izu_free( G.unipath_widefilename);
                G.unipath_widefilename = NULL;
              }
#  else /* def DYNAMIC_WIDE_NAME */
              *G.unipath_widefilename = L'\0';
#  endif /* def DYNAMIC_WIDE_NAME [else] */
              if (G.has_win32_wide) {
                if (G.unipath_filename)
#  ifdef DYNAMIC_WIDE_NAME
                  /* Get wide path from UTF-8 */
                  G.unipath_widefilename = utf8_to_wchar_string(G.unipath_filename);
                else
                  G.unipath_widefilename = utf8_to_wchar_string(G.filename);
#  else /* def DYNAMIC_WIDE_NAME */
                  /* Get wide path from UTF-8 */
                  utf8_to_wchar_string( G.unipath_widefilename,
                   G.unipath_filename);
                else
                  utf8_to_wchar_string( G.unipath_widefilename, G.filename);
#  endif /* def DYNAMIC_WIDE_NAME [else] */

                if (G.pInfo->lcflag)      /* replace with lowercase filename */
                    wcslwr(G.unipath_widefilename);

                if (G.pInfo->vollabel && length > 8 && G.unipath_widefilename[8] == '.') {
                    wchar_t *p = G.unipath_widefilename+8;
                    while (*p++)
                        p[-1] = *p;  /* disk label, and 8th char is dot:  remove dot */
                }
              }
# endif /* def WIN32_WIDE */
            }
#endif /* def UNICODE_SUPPORT */
        }
        break;

#ifdef AMIGA
    /*
     * Fifth case, for the Amiga only:  take the comment that would ordinarily
     * be skipped over, and turn it into a 79 character string that will be
     * attached to the file as a "filenote" after it is extracted.
     */

    case FILENOTE:
        if ((block_len = readbuf(__G__ tmp_fnote, (unsigned)
                          IZ_MIN(length, 2 * AMIGA_FILENOTELEN - 1))) == 0)
            return PK_EOF;
        if ((length -= block_len) > 0)  /* treat remainder as in case SKIP: */
            seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes
                      + (G.inptr - G.inbuf) + length);
        /* convert multi-line text into single line with no ctl-chars: */
        tmp_fnote[block_len] = '\0';
        while ((short int) --block_len >= 0)
            if ((unsigned) tmp_fnote[block_len] < ' ')
                if (tmp_fnote[block_len+1] == ' ')     /* no excess */
                    strcpy(tmp_fnote+block_len, tmp_fnote+block_len+1);
                else
                    tmp_fnote[block_len] = ' ';
        tmp_fnote[AMIGA_FILENOTELEN - 1] = '\0';
        if (G.filenotes[G.filenote_slot])
            izu_free(G.filenotes[G.filenote_slot]);     /* should not happen */
        G.filenotes[G.filenote_slot] = NULL;
        if (tmp_fnote[0]) {
            if (!(G.filenotes[G.filenote_slot] = malloc(strlen(tmp_fnote)+1)))
                return PK_MEM;
            strcpy(G.filenotes[G.filenote_slot], tmp_fnote);
        }
        break;
#endif /* def AMIGA */

    } /* end switch (option) */

    return error;

} /* end function do_string() */




#ifndef VMS

/************************/
/* Function name_only() */
/************************/

char *name_only( path)
    char *path;
{
    /* Extract the name component from a path specification.
     * Returns pointer to allocated storage; NULL, if error.
     */

    char *result = NULL;
    char *dot;
    char *name;
    size_t len;

    name = strrchr( path, '/');
    if (name == NULL)
    {
        name = path;            /* No slash found. */
    }
    else
    {
        name++;                 /* Name begins after slash. */
    }

# ifdef WIN32
    {
        /* On Windows, look for "\", too. */
        char *nameb;

        nameb = strrchr( name, '\\');
        if (nameb != NULL)
        {
            name = nameb+ 1;    /* Name begins after backslash. */
        }
    }
# endif /* def WIN32 */

    dot = strchr( name, '.');
    if (dot == NULL)
    {
        len = strlen( name);    /* No dot found. */
    }
    else
    {
        len = dot- name;        /* Name ends before dot. */
    }

    if (len > 0)
    {
        result = izu_malloc( len+ 1);
        if (result != NULL)
        {
            strncpy( result, name, len);
            result[ len] = '\0';
        }
    }
    return result;
}

#endif /* ndef VMS */




/***********************/
/* Function makeword() */
/***********************/

ush makeword(b)
    ZCONST uch *b;
{
    /*
     * Convert Intel style 'short' integer to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (ush)((b[1] << 8) | b[0]);
}




/***********************/
/* Function makelong() */
/***********************/

ulg makelong(sig)
    ZCONST uch *sig;
{
    /*
     * Convert intel style 'long' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((ulg)sig[3]) << 24)
         + (((ulg)sig[2]) << 16)
         + (ulg)((((unsigned)sig[1]) << 8)
               + ((unsigned)sig[0]));
}




/************************/
/* Function makeint64() */
/************************/

zusz_t makeint64(sig)
    ZCONST uch *sig;
{
#ifdef LARGE_FILE_SUPPORT
    /*
     * Convert intel style 'int64' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((zusz_t)sig[7]) << 56)
        + (((zusz_t)sig[6]) << 48)
        + (((zusz_t)sig[4]) << 32)
        + (zusz_t)((((ulg)sig[3]) << 24)
                 + (((ulg)sig[2]) << 16)
                 + (((unsigned)sig[1]) << 8)
                 + (sig[0]));

#else /* def LARGE_FILE_SUPPORT */

    if ((sig[7] | sig[6] | sig[5] | sig[4]) != 0)
        return (zusz_t)0xffffffffL;
    else
        return (zusz_t)((((ulg)sig[3]) << 24)
                      + (((ulg)sig[2]) << 16)
                      + (((unsigned)sig[1]) << 8)
                      + (sig[0]));

#endif /* def LARGE_FILE_SUPPORT [else] */
}




/*********************/
/* Function fzofft() */
/*********************/

/* Format a zoff_t value in a cylindrical buffer set. */
char *fzofft(__G__ val, pre, post)
    __GDEF
    zoff_t val;
    ZCONST char *pre;
    ZCONST char *post;
{
    /* Storage cylinder. (now in globals.h) */
    /*static char fzofft_buf[FZOFFT_NUM][FZOFFT_LEN];*/
    /*static int fzofft_index = 0;*/

    /* Temporary format string storage. */
    char fmt[16];

    /* Assemble the format string. */
    fmt[0] = '%';
    fmt[1] = '\0';             /* Start after initial "%". */
    if (pre == FZOFFT_HEX_WID)  /* Special hex width. */
    {
        strcat(fmt, FZOFFT_HEX_WID_VALUE);
    }
    else if (pre == FZOFFT_HEX_DOT_WID) /* Special hex ".width". */
    {
        strcat(fmt, ".");
        strcat(fmt, FZOFFT_HEX_WID_VALUE);
    }
    else if (pre != NULL)       /* Caller's prefix (width). */
    {
        strcat(fmt, pre);
    }

    strcat(fmt, FZOFFT_FMT);   /* Long or long-long or whatever. */

    if (post == NULL)
        strcat(fmt, "d");      /* Default radix = decimal. */
    else
        strcat(fmt, post);     /* Caller's radix. */

    /* Advance the cylinder. */
    G.fzofft_index = (G.fzofft_index + 1) % FZOFFT_NUM;

    /* Write into the current chamber. */
    sprintf(G.fzofft_buf[G.fzofft_index], fmt, val);

    /* Return a pointer to this chamber. */
    return G.fzofft_buf[G.fzofft_index];
}




#ifdef IZ_CRYPT_ANY

# ifdef NEED_STR2ISO
/**********************/
/* Function str2iso() */
/**********************/

char *str2iso(dst, src)
    char *dst;                          /* destination buffer */
    register ZCONST char *src;          /* source string */
{
# ifdef INTERN_TO_ISO
    INTERN_TO_ISO(src, dst);
# else /* def INTERN_TO_ISO */
    register uch c;
    register char *dstp = dst;

    do {
        c = (uch)foreign(*src++);
        *dstp++ = (char)ASCII2ISO(c);
    } while (c != '\0');
# endif /* def INTERN_TO_ISO [else] */

    return dst;
}
# endif /* def NEED_STR2ISO */




# ifdef NEED_STR2OEM
/**********************/
/* Function str2oem() */
/**********************/

char *str2oem(dst, src)
    char *dst;                          /* destination buffer */
    register ZCONST char *src;          /* source string */
{
#  ifdef INTERN_TO_OEM
    INTERN_TO_OEM(src, dst);
#  else /* def INTERN_TO_OEM */
    register uch c;
    register char *dstp = dst;

    do {
        c = (uch)foreign(*src++);
        *dstp++ = (char)ASCII2OEM(c);
    } while (c != '\0');
#  endif /* def INTERN_TO_OEM [else] */

    return dst;
}
# endif /* def NEED_STR2OEM */

#endif /* def IZ_CRYPT_ANY */




#ifdef ZMEM  /* memset/memcmp/memcpy for systems without either them or */
             /* bzero/bcmp/bcopy */
             /* (no known systems as of 960211) */

/*********************/
/* Function memset() */
/*********************/

zvoid *memset(buf, init, len)
    register zvoid *buf;        /* buffer location */
    register int init;          /* initializer character */
    register unsigned int len;  /* length of the buffer */
{
    zvoid *start;

    start = buf;
    while (len--)
        *((char *)buf++) = (char)init;
    return start;
}




/*********************/
/* Function memcmp() */
/*********************/

int memcmp(b1, b2, len)
    register ZCONST zvoid *b1;
    register ZCONST zvoid *b2;
    register unsigned int len;
{
    register int c;

    if (len > 0) do {
        if ((c = (int)(*((ZCONST unsigned char *)b1)++) -
                 (int)(*((ZCONST unsigned char *)b2)++)) != 0)
           return c;
    } while (--len > 0)
    return 0;
}




/*********************/
/* Function memcpy() */
/*********************/

zvoid *memcpy(dst, src, len)
    register zvoid *dst;
    register ZCONST zvoid *src;
    register unsigned int len;
{
    zvoid *start;

    start = dst;
    while (len-- > 0)
        *((char *)dst)++ = *((ZCONST char *)src)++;
    return start;
}

#endif /* def ZMEM */




#ifdef NEED_LABS

/*******************/
/* Function labs() */
/*******************/

long int labs( l)
 long l;
{
    if (l >= 0)
        return l;
    else
        return -l;
}

#endif /* def NEED_LABS */




#ifdef NO_STRNICMP

/************************/
/* Function zstrnicmp() */
/************************/

int zstrnicmp(s1, s2, n)
    register ZCONST char *s1, *s2;
    register unsigned n;
{
    for (; n > 0;  --n, ++s1, ++s2) {

        if (ToLower(*s1) != ToLower(*s2))
            /* test includes early termination of one string */
            return ((uch)ToLower(*s1) < (uch)ToLower(*s2))? -1 : 1;

        if (*s1 == '\0')   /* both strings terminate early */
            return 0;
    }
    return 0;
}

#endif /* def NO_STRNICMP */




#ifdef REGULUS  /* returns the inode number on success(!)...argh argh argh */
#  undef stat

/********************/
/* Function zstat() */
/********************/

int zstat(p, s)
    ZCONST char *p;
    struct stat *s;
{
    return (stat((char *)p,s) >= 0? 0 : (-1));
}

#endif /* def REGULUS */




#ifdef _MBCS

/* DBCS support for Info-ZIP's zip  (mainly for japanese (-: )
 * by Yoshioka Tsuneo (QWF00133@nifty.ne.jp,tsuneo-y@is.aist-nara.ac.jp)
 * This code is public domain!   Date: 1998/12/20
 */

/************************/
/* Function plastchar() */
/************************/

char *plastchar(ptr, len)
    ZCONST char *ptr;
    extent len;
{
    extent clen;
    ZCONST char *oldptr = ptr;
    while(*ptr != '\0' && len > 0){
        oldptr = ptr;
        clen = CLEN(ptr);
        ptr += clen;
        len -= clen;
    }
    return (char *)oldptr;
}




# ifdef NEED_UZMBCLEN
/***********************/
/* Function uzmbclen() */
/***********************/

extent uzmbclen(ptr)
    ZCONST unsigned char *ptr;
{
    int mbl;

    mbl = mblen((ZCONST char *)ptr, MB_CUR_MAX);
    /* For use in code scanning through MBCS strings, we need a strictly
       positive "MB char bytes count".  For our scanning purpose, it is not
       not relevant whether the MB character is valid or not. And, the NUL
       char '\0' has a byte count of 1, but mblen() returns 0. So, we make
       sure that the uzmbclen() return value is not less than 1.
     */
    return (extent)(mbl > 0 ? mbl : 1);
}
# endif /* def NEED_UZMBCLEN */




# ifdef NEED_UZMBSCHR
/***********************/
/* Function uzmbschr() */
/***********************/

unsigned char *uzmbschr(str, c)
    ZCONST unsigned char *str;
    unsigned int c;
{
    while(*str != '\0'){
        if (*str == c) {return (unsigned char *)str;}
        INCSTR(str);
    }
    return NULL;
}
# endif /* def NEED_UZMBSCHR */




# ifdef NEED_UZMBSRCHR
/************************/
/* Function uzmbsrchr() */
/************************/

unsigned char *uzmbsrchr(str, c)
    ZCONST unsigned char *str;
    unsigned int c;
{
    unsigned char *match = NULL;
    while(*str != '\0'){
        if (*str == c) {match = (unsigned char *)str;}
        INCSTR(str);
    }
    return match;
}
# endif /* def NEED_UZMBSRCHR */
#endif /* def _MBCS */




#ifdef SMALL_MEM

/*******************************/
/*  Function fLoadFarString()  */   /* (and friends...) */
/*******************************/

char *fLoadFarString(__GPRO__ const char Far *sz)
{
    (void)zfstrcpy(G.rgchBigBuffer, sz);
    return G.rgchBigBuffer;
}

char *fLoadFarStringSmall(__GPRO__ const char Far *sz)
{
    (void)zfstrcpy(G.rgchSmallBuffer, sz);
    return G.rgchSmallBuffer;
}

char *fLoadFarStringSmall2(__GPRO__ const char Far *sz)
{
    (void)zfstrcpy(G.rgchSmallBuffer2, sz);
    return G.rgchSmallBuffer2;
}




# if !defined(_MSC_VER) || (_MSC_VER < 600)
/*************************/
/*  Function zfstrcpy()  */   /* portable clone of _fstrcpy() */
/*************************/

char Far * Far zfstrcpy(char Far *s1, const char Far *s2)
{
    char Far *p = s1;

    while ((*s1++ = *s2++) != '\0');
    return p;
}




#  if !(defined(SFX) || defined(FUNZIP))
/*************************/
/*  Function zfstrcmp()  */   /* portable clone of _fstrcmp() */
/*************************/

int Far zfstrcmp(const char Far *s1, const char Far *s2)
{
    int ret;

    while ((ret = (int)(uch)*s1 - (int)(uch)*s2) == 0
           && *s2 != '\0') {
        ++s2; ++s1;
    }
    return ret;
}
#  endif /* !(defined(SFX) || defined(FUNZIP)) */
# endif /* !defined(_MSC_VER) || (_MSC_VER < 600) */

#endif /* def SMALL_MEM */
