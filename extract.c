/*
  Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  extract.c

  This file contains the high-level routines ("driver routines") for extrac-
  ting and testing zipfile members.  It calls the low-level routines in files
  explode.c, inflate.c, unreduce.c and unshrink.c.

  Contains:  allocate_name_match_arrays()
             check_unmatched_names()
             match_include_exclude()
             name_abs_rel()
             name_junk()
             name_junkw()
             test_compr_eb()
             TestExtraField()
             ef_scan_for_cafe()
             ef_scan_for_stream()
             extract_dest_dir()
             store_info()
             set_deferred_symlink()             (SYMLINKS)
             set_deferred_symlinks()            (SYMLINKS)
             dircomp()                          (SET_DIR_ATTRIB)
             dircompw()                         (SET_DIR_ATTRIB && WIN32_WIDE)
             aes_wg_prep()                      (IZ_CRYPT_AES_WG)
             size_check()
             password_check()                   (IZ_CRYPT_ANY)
             detect_apl_dbl()                   (UNIX && __APPLE__)
             backslash_slash()                  (ndef SFX)
             extract_test_trailer()
             extract_or_test_member()
             mapname_dir_vollab()
             mapname_dir_vollabw()
             conflict_query_qr()
             conflict_query()
             conflict_queryw()
             set_dir_attribs()                  SET_DIR_ATTRIB
             extract_or_test_stream()           Entry - stream
             close_segment()
             find_local_header()
             central_local_name_check()
             extract_or_test_entrylist()
             extract_or_test_entrylistw()       (WIN32_WIDE)
             extract_or_test_files()            Entry - seekable file
  (ext)      find_compr_idx()
  (ext)      memextract()
  (ext)      memflush()
             decompress_bits()                  (VMS or VMS_TEXT_CONV)
  (ext)      extract_izvms_block()              (VMS or VMS_TEXT_CONV)
  (ext)      fnfilter()
  (ext)      fnfilterw()                        (WIN32_WIDE)
             UZbunzip2()                        (BZIP2_SUPPORT)
             UZlzma()                           (LZMA_SUPPORT)
             ppmd_read_byte()                   (PPMD_SUPPORT)
             UZppmd()                           (PPMD_SUPPORT)

  ---------------------------------------------------------------------------*/


#define __EXTRACT_C     /* identifies this source module */
#define UNZIP_INTERNAL
#include "unzip.h"
#if defined( UNIX) && defined( __APPLE__)
# include "unix/macosx.h"
#endif /* defined( UNIX) && defined( __APPLE__) */
#ifdef WINDLL
# ifdef POCKET_UNZIP
#  include "wince/intrface.h"
# else
#  include "windll/windll.h"
# endif
#endif
#include "crc32.h"
#include "crypt.h"

#ifdef BZIP2_SUPPORT
static int UZbunzip2 OF((__GPRO));
#endif

#ifdef LZMA_SUPPORT
static int UZlzma OF((__GPRO));
#endif

#ifdef PPMD_SUPPORT
static int UZppmd OF((__GPRO));
#endif

#define GRRDUMP(buf,len) { \
    int i, j; \
 \
    for (j = 0;  j < (len)/16;  ++j) { \
        printf("        "); \
        for (i = 0;  i < 16;  ++i) \
            printf("%02x ", (uch)(buf)[i+(j<<4)]); \
        printf("\n        "); \
        for (i = 0;  i < 16;  ++i) { \
            char c = (char)(buf)[i+(j<<4)]; \
 \
            if (c == '\n') \
                printf("\\n "); \
            else if (c == '\r') \
                printf("\\r "); \
            else \
                printf(" %c ", c); \
        } \
        printf("\n"); \
    } \
    if ((len) % 16) { \
        printf("        "); \
        for (i = j<<4;  i < (len);  ++i) \
            printf("%02x ", (uch)(buf)[i]); \
        printf("\n        "); \
        for (i = j<<4;  i < (len);  ++i) { \
            char c = (char)(buf)[i]; \
 \
            if (c == '\n') \
                printf("\\n "); \
            else if (c == '\r') \
                printf("\\r "); \
            else \
                printf(" %c ", c); \
        } \
        printf("\n"); \
    } \
}


/* Values for local skip_entry flag(s): */
#define SKIP_NO         0       /* Do not skip this entry. */
#define SKIP_Y_EXISTING 1       /* Skip this entry, do not overwrite file. */
#define SKIP_Y_NONEXIST 2       /* Skip this entry, do not create new file. */


/*******************************/
/*  Strings used in extract.c  */
/*******************************/

static ZCONST char Far VersionMsg[] =
  "   skipping: %-22s   need %s compat. v%u.%u (can do v%u.%u)\n";
static ZCONST char Far ComprMsgNum[] =
  "   skipping: %-22s   unsupported compression method: %u\n";
#ifndef SFX
   static ZCONST char Far ComprMsgName[] =
     "   skipping: %-22s   unsupported compression method: %s\n";
   static ZCONST char Far CmprNone[]       = "store";
   static ZCONST char Far CmprShrink[]     = "shrink";
   static ZCONST char Far CmprReduce[]     = "reduce";
   static ZCONST char Far CmprImplode[]    = "implode";
   static ZCONST char Far CmprTokenize[]   = "tokenize";
   static ZCONST char Far CmprDeflate[]    = "deflate";
   static ZCONST char Far CmprDeflat64[]   = "deflate64";
   static ZCONST char Far CmprDCLImplode[] = "DCL_implode";
   static ZCONST char Far CmprBzip[]       = "bzip2";
   static ZCONST char Far CmprLZMA[]       = "LZMA";
   static ZCONST char Far CmprIBMTerse[]   = "IBM_Terse";
   static ZCONST char Far CmprIBMLZ77[]    = "IBM_LZ77";
   static ZCONST char Far CmprJPEG[]       = "JPEG";
   static ZCONST char Far CmprWavPack[]    = "WavPack";
   static ZCONST char Far CmprPPMd[]       = "PPMd";
   static ZCONST char Far CmprAES[]        = "AES_WG(encr)";
   static ZCONST char Far *ComprNames[NUM_METHODS] = {
     CmprNone, CmprShrink, CmprReduce, CmprReduce, CmprReduce, CmprReduce,
     CmprImplode, CmprTokenize, CmprDeflate, CmprDeflat64, CmprDCLImplode,
     CmprBzip, CmprLZMA, CmprIBMTerse, CmprIBMLZ77, CmprJPEG, CmprWavPack,
     CmprPPMd, CmprAES
   };
   static ZCONST unsigned ComprIDs[NUM_METHODS] = {
     STORED, SHRUNK, REDUCED1, REDUCED2, REDUCED3, REDUCED4,
     IMPLODED, TOKENIZED, DEFLATED, ENHDEFLATED, DCLIMPLODED,
     BZIPPED, LZMAED, IBMTERSED, IBMLZ77ED, JPEGED, WAVPACKED,
     PPMDED, AESENCRED
   };
#endif /* ndef SFX */
static ZCONST char Far FilNamMsg[] =
  "%s:  bad filename length (%s)\n";
#ifndef SFX
static ZCONST char Far WarnNoMemCFName[] =
 "%s:  warning, no memory for comparison with local header\n";
static ZCONST char Far LvsCFNamMsg[] =
 "%s:  mismatching \"local\" filename (%s),\n\
         continuing with \"central\" filename version\n";
#endif /* ndef SFX */
#if !defined(SFX) && defined(UNICODE_SUPPORT)
   static ZCONST char Far GP11FlagsDiffer[] =
     "file #%lu (%s):\n\
         mismatch between local and central GPF bit 11 (\"UTF-8\"),\n\
         continuing with central flag (IsUTF8 = %d)\n";
#endif /* !defined(SFX) && defined(UNICODE_SUPPORT) */
static ZCONST char Far WrnStorUCSizCSizDiff[] =
  "%s:  ucsize %s <> csize %s for STORED entry\n\
         continuing with \"compressed\" size value\n";
static ZCONST char Far ExtFieldMsg[] =
  "%s:  bad extra field length (%s)\n";
static ZCONST char Far OffsetMsg[] =
  "file #%lu:  bad zipfile offset (%s):  %ld\n";
#ifndef SFX
   static ZCONST char Far LengthMsg[] =
     "%s  %s:  %s bytes required to uncompress to %s bytes;\n    %s\
      supposed to require %s bytes%s%s%s\n";
#endif

#ifdef IZ_CRYPT_AES_WG
static ZCONST char Far BadAesExtFieldMsg[] =
  "%s:  bad AES_WG extra field (mode = %d)\n";
static ZCONST char Far BadAesMacMsg[] = " bad AES_WG MAC\n";
#endif /* def IZ_CRYPT_AES_WG */

static ZCONST char Far BadFileCommLength[] = "%s:  bad file comment length\n";
static ZCONST char Far LocalHdrSig[] = "local header sig";
static ZCONST char Far BadLocalHdr[] = "file #%lu:  bad local header\n";
static ZCONST char Far AttemptRecompensate[] =
  "  (attempting to re-compensate)\n";
#ifndef SFX
   static ZCONST char Far BackslashPathSep[] =
     "warning:  %s appears to use backslashes as path separators\n";
#endif
static ZCONST char Far AbsolutePathWarning[] =
  "warning:  stripped absolute path spec from %s\n";
static ZCONST char Far SkipVolumeLabel[] =
  "   skipping: %-22s  %svolume label\n";
static ZCONST char Far JunkTooMany[] =
  "warning:  not enough directories (%d) to junk %d:";
static ZCONST char Far JunkTooManyDir[] =
  "warning:  not enough Directories (%d) to junk %d: %s\n";


#if defined( UNIX) && defined( __APPLE__)
static ZCONST char Far AplDblNameTooLong[] =
  "ERROR:  file name too long with AppleDouble suffix: %s\n";
#endif /* defined( UNIX) && defined( __APPLE__) */

#ifdef SET_DIR_ATTRIB   /* messages of code for setting directory attributes */
   static ZCONST char Far DirlistEntryNoMem[] =
     "warning:  cannot alloc memory for dir times/permissions/UID/GID\n";
   static ZCONST char Far DirlistSortNoMem[] =
     "warning:  cannot alloc memory to sort dir times/perms/etc.\n";
   static ZCONST char Far DirlistSetAttrFailed[] =
     "warning:  set times/attribs failed for %s\n";
   static ZCONST char Far DirlistFailAttrSum[] =
     "     failed setting times/attribs for %lu dir entries";
#endif

#ifdef SYMLINKS         /* messages of the deferred symlinks handler */
   static ZCONST char Far SymLnkWarnNoMem[] =
     "warning:  deferred symlink (%s) failed:\n\
          out of memory\n";
   static ZCONST char Far SymLnkWarnInvalid[] =
     "warning:  deferred symlink (%s) failed:\n\
          invalid placeholder file\n";
   static ZCONST char Far SymLnkDeferred[] =
     "finishing deferred symbolic links:\n";
   static ZCONST char Far SymLnkFinish[] =
     "  %-22s -> %s\n";
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
   static ZCONST char Far SymLnkFinishW[] =
     "  %-22S -> %s\n";
# endif
   static ZCONST char Far SymLnkErrUnlink[] = "%sunlink error: %s";
   static ZCONST char Far SymLnkErrSymlink[] = "symlink%s error: %s";
# ifdef VMS
   static ZCONST char Far SymLnkError[] =
     "  %s\n";
# endif /* def VMS */
# ifdef WIN32
   static ZCONST char Far SymLnkErrSymlinkPriv[] =
     "Insufficient privilege to create symlink (may need Administrative)";
# endif /* def WIN32 */
#endif

#ifndef WINDLL
   static ZCONST char Far ReplaceQuery[] =
# ifdef VMS
     "new version of %s? [y]es, [n]o, [A]ll, [N]one, [r]ename: ";
# else /* def VMS */
     "replace %s ? [y]es, [n]o, [A]ll, [N]one, [r]ename: ";
#  if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
   static ZCONST char Far ReplaceQueryw[] =
     "replace %S ? [y]es, [n]o, [A]ll, [N]one, [r]ename: ";
#  endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
# endif /* def VMS [else] */
   static ZCONST char Far AssumeNone[] =
     " NULL\n(EOF or read error, treating as \"[N]one\" ...)\n";
   static ZCONST char Far NewNameQuery[] = "new name: ";
   static ZCONST char Far InvalidResponse[] =
     "error:  invalid response [%s]\n";
#endif /* ndef WINDLL */

static ZCONST char Far ErrorInArchive[] =
  "At least one %serror %swas detected in %s.\n";
static ZCONST char Far ZeroFilesTested[] =
  "Caution:  zero files tested in %s.\n";

#ifndef VMS
   static ZCONST char Far VMSFormatQuery[] =
     "\n%s:  stored in VMS format.  Extract anyway? (y/n) ";
#endif

#ifdef IZ_CRYPT_ANY
   static ZCONST char Far SkipCannotGetPasswd[] =
     "   skipping: %-22s  unable to get password\n";
   static ZCONST char Far SkipIncorrectPasswd[] =
     "   skipping: %-22s  incorrect password\n";
   static ZCONST char Far FilesSkipBadPasswd[] =
     "%lu file%s skipped because of incorrect password.\n";
   static ZCONST char Far MaybeBadPasswd[] =
     "    (may instead be incorrect password)\n";
#else /* def IZ_CRYPT_ANY */
   static ZCONST char Far SkipEncrypted[] =
     "   skipping: %-22s  encrypted (not supported)\n";
#endif /* def IZ_CRYPT_ANY [else] */

static ZCONST char Far NoErrInCompData[] =
  "No errors detected in compressed data of %s.\n";
static ZCONST char Far NoErrInTestedFiles[] =
  "No errors detected in %s for the %lu file%s tested.\n";
static ZCONST char Far FilesSkipped[] =
  "%lu file%s skipped because of unsupported compression or encryption.\n";

static ZCONST char Far ErrUnzipFile[] = "ERROR:  %s%s %s\n";
static ZCONST char Far ErrUnzipNoFile[] = "ERROR:  %s%s\n";
static ZCONST char Far NotEnoughMem[] = "not enough memory to ";
static ZCONST char Far InvalidComprData[] = "invalid compressed data to ";
#ifdef DEFLATE_SUPPORT
static ZCONST char Far Inflate[] = "inflate";
#endif
#ifdef BZIP2_SUPPORT
  static ZCONST char Far BUnzip[] = "bunzip";
#endif
#ifdef LZMA_SUPPORT
  static ZCONST char Far UnLZMA[] = "unLZMA";
#endif
#ifdef PPMD_SUPPORT
  static ZCONST char Far UnPPMd[] = "unPPMd";
#endif
#ifndef SFX
   static ZCONST char Far Explode[] = "explode";
#endif
#ifndef LZW_CLEAN
   static ZCONST char Far Unshrink[] = "unshrink";
#endif

#if !defined(DELETE_IF_FULL) || !defined(HAVE_UNLINK)
   static ZCONST char Far FileTruncated[] =
     "warning:  %s is probably truncated\n";
#endif

static ZCONST char Far UnknownCmprMthdFile[] =
  "%s:  unknown compression method: %d\n";

static ZCONST char Far BadCRC[] = " bad CRC %08lx  (should be %08lx)\n";

/* TruncEAs[] also used in os2/os2.c:[close_outfile(), mapname()]. */
ZCONST char Far TruncEAs[] = " compressed EA data missing (%ld bytes)%s";

/* TruncNTSD also used in win32/win32.c:[close_outfile(),
 *  set_direc_attribs(), set_direc_attribsw()].
 */
ZCONST char Far TruncNTSD[] =
 " compressed WinNT security data missing (%ld bytes)%s";

#ifndef SFX
   static ZCONST char Far InconsistEFlength[] = "bad extra-field entry:\n\
     EF block length (%ld bytes) exceeds remaining EF data (%ld bytes)\n";
   static ZCONST char Far TooSmallEBlength[] = "bad extra-field entry:\n\
     EF block length (%lu bytes) invalid (< %d)\n";
   static ZCONST char Far InvalidComprDataEAs[] =
     " invalid compressed data for EAs\n";
# if defined(WIN32) && defined(NTSD_EAS)
     static ZCONST char Far InvalidSecurityEAs[] =
       " EAs fail security check\n";
# endif
   static ZCONST char Far UnsuppNTSDVersEAs[] =
     " unsupported NTSD EAs version %d\n";
   static ZCONST char Far BadCRC_EAs[] = " bad CRC for extended attributes\n";
   static ZCONST char Far UnknownCmprMthdEA[] =
     " unknown compression method for EAs (%u)\n";
   static ZCONST char Far NotEnoughMemEAs[] =
     " out of memory while inflating EAs\n";
   static ZCONST char Far UnknErrorEAs[] =
     " unknown error on extended attributes\n";
#endif /* ndef SFX */

static ZCONST char Far UnsupportedExtraField[] =
  "ERROR:  unsupported extra-field compression type (%u)--skipping\n";
static ZCONST char Far BadExtraFieldCRC[] =
  "ERROR [%s]:  bad extra-field CRC %08lx (should be %08lx)\n";

/* Data-dependent info for verbose test (-tv) report. */
static ZCONST char Far InfoMsgLZMA[] =
  "\n     LZMA: dicSize = %u, lc = %u, lp = %u, pb = %u. ";
static ZCONST char Far InfoMsgPPMd[] =
  "\n     PPMd: flags = 0x%04x.  Order = %d, Mem = %dMB, Restore = %d. ";

/* Inconsistent Java "CAFE" extra fields (local v. central dir.). */
static ZCONST char Far InfoInconsistentJavaCAFE[] =
     "Inconsistent cantral-local Java CAFE.  Expect name warnings/problems.\n";



/*******************************************/
/*  Function allocate_name_match_arrays()  */
/*******************************************/

static int allocate_name_match_arrays( __G) /* Return PK-type error code. */
    __GDEF
{
    int error = PK_OK;

    /* Allocate space for checking (included) member name list. */
    if (G.filespecs > 0)
    {   /* Member list. */
        G.fn_matched = (char *)izu_malloc( G.filespecs* sizeof( char));
        if (G.fn_matched == (char *)NULL)
        {
            Info( slide, 0x401, ((char *)slide,
             "Not enough memory for (include) file list"));
            error = PK_MEM;
        }
        else
        {
            memset( G.fn_matched, FALSE, G.filespecs);
        }
    }

    /* Allocate space for checking excluded (-x) member name list. */
    if ((error == PK_OK) && (G.xfilespecs > 0))
    {   /* -x. */
        G.xn_matched = (char *)izu_malloc( G.xfilespecs* sizeof( char));
        if (G.xn_matched == (char *)NULL)
        {
            Info( slide, 0x401, ((char *)slide,
             "Not enough memory for exclude file list"));
            error = PK_MEM;
        }
        else
        {
            memset( G.xn_matched, FALSE, G.xfilespecs);
        }
    }

    return error;
} /* allocate_name_match_arrays(). */


/**************************************/
/*  Function check_unmatched_names()  */
/**************************************/

static int check_unmatched_names( __G__ check, err_in_arch)
  __GDEF
  int check;
  int err_in_arch;
{
  int error = PK_OK;                    /* Return PK-type error code. */
  unsigned i;

  /* Check for unmatched member names on command line, and print a
   * warning if any are found.
   * Suppress check when central dir scan was interrupted prematurely.
   * Free allocated name list (include and exclude) memory.
   */
  if (G.fn_matched)
  {
    if (check)
    {
      for (i = 0; i < G.filespecs; ++i)
      {
        if (!G.fn_matched[i])
        {
#ifdef DLL
          if (!G.redirect_data && !G.redirect_text)
          {
            Info(slide, 0x401, ((char *)slide,
             LoadFarString(FilenameNotMatched), G.pfnames[i]));
          }
          else
          {
            setFileNotFound(__G);
          }
#else
          Info(slide, 1, ((char *)slide,
           LoadFarString(FilenameNotMatched), G.pfnames[i]));
#endif
          if (err_in_arch <= PK_WARN)
          {
            error = PK_FIND;            /* Some file(s) not found. */
          }
        }
      }
    }
    izu_free( (zvoid *)G.fn_matched);
  } /* if (G.fn_matched) */

  if (G.xn_matched)
  {
    if (check)
    {
      for (i = 0;  i < G.xfilespecs;  ++i)
      {
        if (!G.xn_matched[i])
        {
          Info(slide, 0x401, ((char *)slide,
           LoadFarString(ExclFilenameNotMatched), G.pxnames[i]));
          /* No error/warning status for unmatched exclude (-x) names??? */
        }
      }
    }
  izu_free((zvoid *)G.xn_matched);
  } /* if (G.xn_matched) */

  return error;
} /* check_unmatched_names(). */


/**************************************/
/*  Function match_include_exclude()  */
/**************************************/

static int match_include_exclude( __G)
  __GDEF
{
  int do_this_file = -1;                /* Undetermined. */
  unsigned i;

#if defined( UNIX) && defined( __APPLE__)
  G.apl_dbl = 0;                        /* Not (yet) AppleDouble. */
#endif /* defined( UNIX) && defined( __APPLE__) */

  if (G.process_all_files)
  {
    do_this_file = 1;
  }
#if defined( UNIX) && defined( __APPLE__)
  /* Do AppleDouble member, if its antecedent was done.
   * G.seeking_apl_dbl will be set only if we're doing integrated
   * AppleDouble processing, not if members are being treated
   * independently.
   * We assume that the archive is well structured, that is, the archive
   * member preceding an AppleDouble member is the related main member.
   * (Names could be saved and checked, instead.)
   */
  else if (G.seeking_apl_dbl)
  {
    G.apl_dbl = revert_apl_dbl_path( G.filename, NULL);
    if (G.apl_dbl)
    {
      do_this_file = G.do_this_prev;
    }
  }
#endif /* defined( UNIX) && defined( __APPLE__) [else] */

  if (do_this_file < 0)       /* Continue, if still undetermined. */
  {
    /* Determine if this entry matches an "include" pattern. */
    if (G.filespecs == 0)
    {
      /* No "include" patterns. */
      do_this_file = 1;
    }
    else
    {
      for (i = 0; i < G.filespecs; i++)
      {
        if (match(G.filename, G.pfnames[i], uO.C_flag WISEP))
        {
          do_this_file = 1;  /* ^-- ignore case or not? */
          G.fn_matched[i] = TRUE;
          break;       /* found match, so stop looping */
        }
      }
    }

    if (do_this_file > 0)
    {
      /* Determine if this entry matches an "exclude" pattern. */
      for (i = 0; i < G.xfilespecs; i++)
      {
        if (match(G.filename, G.pxnames[i], uO.C_flag WISEP))
        {
          do_this_file = 0; /* ^-- ignore case or not? */
          G.xn_matched[i] = TRUE;
          break;
        }
      }
    }
  }

  return do_this_file;
} /* match_include_exclude(). */


/*****************************/
/*  Function name_abs_rel()  */
/*****************************/

static int name_abs_rel( __G)           /* Return PK-type error code. */
  __GDEF
{
  int error = PK_OK;

  /* Transform an absolute path to a relative path, and warn user. */
  if (G.filename[0] == '/')
  {
    Info(slide, 0x401, ((char *)slide,
     LoadFarString(AbsolutePathWarning),
     FnFilter1(G.filename)));
    error = PK_WARN;

    do                          /* While there's a leading slash, ... */
    {
      char *p;

      p = G.filename + 1;
      do                        /* Left-shift all characters one place. */
      {
        *(p-1) = *p;
      } while (*p++ != '\0');
    } while (G.filename[0] == '/');
  }

  return error;
} /* name_abs_rel(). */


/**************************/
/*  Function name_junk()  */
/**************************/

static int name_junk( __G)              /* Return PK-type error code. */
  __GDEF
{
  int error = PK_OK;                    /* (What could go wrong?) */
  char *slp;

  /* 2012-07-13 SMS.
   * Moved -j/--junkdirs (.jflag) processing up to here from
   * OS-specific mapname() functions.
   * G.jdir_filename is a pointer (within G.filename) to the
   * path being kept (not junked).
   */
  G.jdir_filename = G.filename; /* (Already right, if "-j-", .jflag == 0.) */
  if (uO.jflag < 0)
  {
    /* "-j": Junking all directories.  Find last slash (if any). */
    slp = strrchr( G.filename, '/');
    if (slp != NULL)
    {
      /* Found a last slash.  Name begins after it. */
      G.jdir_filename = slp+ 1;
    }
  }
  else if (uO.jflag > 0)
  {
    /* "-j=N": Junking N directories.  Loop through slashes. */
    int i;

    for (i = 0; i < uO.jflag; i++)
    {
      slp = strchr( G.jdir_filename, '/');
      if (slp == NULL)
      {
        if (G.filename[ strlen( G.filename)- 1] == '/')
        {
          /* Directory.  (Won't be created, so show name.) */
          Info( slide, 0x401, ((char *)slide,
           LoadFarString( JunkTooManyDir),
           i, uO.jflag, FnFilter1( G.filename)));
        }
        else
        {
          /* File.  (Will be extracted, so name follows.) */
          Info( slide, 0x401, ((char *)slide,
           LoadFarString( JunkTooMany),
           i, uO.jflag));
        }
        break;
      }
      else
      {
        /* Advance the retained-name pointer. */
        G.jdir_filename = ++slp;
      }
    }
  }

  return error;
} /* name_junk(). */


#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)

/***************************/
/*  Function name_junkw()  */
/***************************/

static int name_junkw( __G)             /* Return PK-type error code. */
  __GDEF
{
  int error = PK_OK;                    /* (What could go wrong?) */
  wchar_t *wslp;

  /* 2012-07-14 SMS.
   * Moved -j/--junkdirs (.jflag) processing up to here from
   * OS-specific mapnamew() function.
   * G.unipath_jdir_widefilename is a pointer (within the
   * G.unipath_widefilename string) to the path being kept
   * (not junked).
   */
  G.unipath_jdir_widefilename = G.unipath_widefilename;
  /* (Already right, if "-j-", .jflag == 0.) */
  if (uO.jflag < 0)
  {
    /* "-j": Junking all directories.  Find last slash (if any). */
    wslp = wcschr( G.unipath_widefilename, '/');
    if (wslp != NULL)
    {
      /* Found a last slash.  Name begins after it. */
      G.unipath_jdir_widefilename = wslp+ 1;
    }
  }
  else if (uO.jflag > 0)
  {
    /* "-j=N": Junking N directories.  Loop through slashes. */
    int i;

    for (i = 0; i < uO.jflag; i++)
    {
      wslp = wcschr( G.unipath_jdir_widefilename, '/');
      if (wslp == NULL)
      {
        if (G.filename[ strlen( G.filename)- 1] == '/')
        {
          /* Directory.  (Won't be created, so show name.) */
          Info( slide, 0x401, ((char *)slide,
           LoadFarString( JunkTooManyDir),
           i, uO.jflag, FnFilter1( G.filename)));
        }
        else
        {
          /* File.  (Will be extracted, so name follows.) */
          Info( slide, 0x401, ((char *)slide,
           LoadFarString( JunkTooMany),
           i, uO.jflag));
        }
        break;
      }
      else
      {
        /* Advance the retained-name pointer. */
        G.unipath_jdir_widefilename = ++wslp;
      }
    }
  }

  return error;
} /* name_junkw(). */

#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */


#ifndef SFX

/******************************/
/*  Function test_compr_eb()  */
/******************************/

# ifdef PROTO
static int test_compr_eb(
    __GPRO__
    uch *eb,
    long eb_len,
    long compr_offset,
    int (*test_uc_ebdata)(__GPRO__ uch *eb, long eb_len,
                          uch *eb_ucptr, ulg eb_ucsize))
# else /* def PROTO */
static int test_compr_eb(__G__ eb, eb_len, compr_offset, test_uc_ebdata)
    __GDEF
    uch *eb;
    long eb_len;
    long compr_offset;
    int (*test_uc_ebdata)();
# endif /* def PROTO [else] */
{
    ulg eb_ucsize;
    uch *eb_ucptr;
    int r;
    ush eb_compr_method;

    if (compr_offset < 4)                /* field is not compressed: */
        return PK_OK;                    /* do nothing and signal OK */

    /* Return no/bad-data error status if any problem is found:
     *    1. eb_len is too small to hold the uncompressed size
     *       (eb_ucsize).  (Else extract eb_ucsize.)
     *    2. eb_ucsize is zero (invalid).  2014-12-04 SMS.  (oCERT.org report.)
     *    3. eb_ucsize is positive, but eb_len is too small to hold
     *       the compressed data header.
     */
    if ((eb_len < (EB_UCSIZE_P + 4)) ||
     ((eb_ucsize = makelong( eb+ (EB_HEADSIZE+ EB_UCSIZE_P))) == 0L) ||
     ((eb_ucsize > 0L) && (eb_len <= (compr_offset + EB_CMPRHEADLEN))))
        return IZ_EF_TRUNC;             /* no/bad compressed data! */

    /* 2015-02-10 Mancha(?), Michal Zalewski, Tomas Hoger, SMS.
     * For STORE method, compressed and uncompressed sizes must agree.
     * http://www.info-zip.org/phpBB3/viewtopic.php?f=7&t=450
     */
    eb_compr_method = makeword( eb + (EB_HEADSIZE + compr_offset));
    if ((eb_compr_method == STORED) &&
     (eb_len != compr_offset + EB_CMPRHEADLEN + eb_ucsize))
        return PK_ERR;

    if (
# ifdef INT_16BIT
        (((ulg)(extent)eb_ucsize) != eb_ucsize) ||
# endif
        (eb_ucptr = (uch *)izu_malloc((extent)eb_ucsize)) == (uch *)NULL)
        return PK_MEM4;

    r = memextract(__G__ eb_ucptr, eb_ucsize,
                   eb + (EB_HEADSIZE + compr_offset),
                   (ulg)(eb_len - compr_offset));

    if (r == PK_OK && test_uc_ebdata != NULL)
        r = (*test_uc_ebdata)(__G__ eb, eb_len, eb_ucptr, eb_ucsize);

    izu_free(eb_ucptr);
    return r;

} /* test_compr_eb(). */


/*******************************/
/*  Function TestExtraField()  */
/*******************************/

/* 2012-11-25 SMS.  (OUSPG report.)
 * Changed eb_len and ef_len from unsigned to signed, to
 * catch underflow of ef_len caused by corrupt/malicious
 * data.  (32-bit is adequate.  Used "long" to
 * accommodate any systems with 16-bit "int".)
 * 2015-01-01 SMS.
 * And eb_cmpr_offs.  See also test_compr_eb():compr_offset.
 */

# ifdef PROTO
static int TestExtraField(
    __GPRO__
    uch *ef_buf,
    long ef_len)
# else /* def PROTO */
static int TestExtraField(__G__ ef_buf, ef_len)
    __GDEF
    uch *ef_buf;
    long ef_len;
# endif /* def PROTO [else] */
{
    ush eb_id;
    long eb_len;
    long eb_cmpr_offs = 0;
    int r;

    /* we know the regular compressed file data tested out OK, or else we
     * wouldn't be here ==> print filename if any extra-field errors found
     */
    while (ef_len >= EB_HEADSIZE) {
        eb_id = makeword(ef_buf);
        eb_len = (unsigned)makeword(ef_buf+EB_LEN);

        if (eb_len > (ef_len - EB_HEADSIZE))
        {
            /* Extra block length > remaining size of extra field. */
            if (uO.qflag)
                Info(slide, 1, ((char *)slide, "%-22s ",
                  FnFilter1(G.filename)));
            Info(slide, 1, ((char *)slide, LoadFarString(InconsistEFlength),
              eb_len, (ef_len - EB_HEADSIZE)));
            return PK_ERR;
        }

        switch (eb_id) {
            case EF_OS2:
            case EF_ACL:
            case EF_MAC3:
            case EF_BEOS:
            case EF_ATHEOS:
                switch (eb_id) {
                  case EF_OS2:
                  case EF_ACL:
                    eb_cmpr_offs = EB_OS2_HLEN;
                    break;
                  case EF_MAC3:
                    if (eb_len >= EB_MAC3_HLEN &&
                     (makeword(ef_buf+(EB_HEADSIZE+EB_FLGS_OFFS))
                     & EB_M3_FL_UNCMPR) &&
                     (makelong(ef_buf+EB_HEADSIZE) == eb_len - EB_MAC3_HLEN))
                        eb_cmpr_offs = 0;
                    else
                        eb_cmpr_offs = EB_MAC3_HLEN;
                    break;
                  case EF_BEOS:
                  case EF_ATHEOS:
                    if (eb_len >= EB_BEOS_HLEN &&
                     (*(ef_buf+(EB_HEADSIZE+EB_FLGS_OFFS)) & EB_BE_FL_UNCMPR) &&
                     (makelong(ef_buf+EB_HEADSIZE) == eb_len - EB_BEOS_HLEN))
                        eb_cmpr_offs = 0;
                    else
                        eb_cmpr_offs = EB_BEOS_HLEN;
                    break;
                }
                if ((r = test_compr_eb(__G__ ef_buf, (long)eb_len,
                 eb_cmpr_offs, NULL)) != PK_OK)
                {
                    if (uO.qflag)
                        Info(slide, 1, ((char *)slide, "%-22s ",
                         FnFilter1(G.filename)));
                    switch (r) {
                        case IZ_EF_TRUNC:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(TruncEAs),
                             eb_len-(eb_cmpr_offs+EB_CMPRHEADLEN), "\n"));
                            break;
                        case PK_ERR:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(InvalidComprDataEAs)));
                            break;
                        case PK_MEM3:
                        case PK_MEM4:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(NotEnoughMemEAs)));
                            break;
                        default:
                            if ((r & 0xff) != PK_ERR)
                                Info(slide, 1, ((char *)slide,
                                 LoadFarString(UnknErrorEAs)));
                            else
                            {
                                ush m = (ush)(r >> 8);
                                if (m == DEFLATED)            /* GRR KLUDGE! */
                                    Info(slide, 1, ((char *)slide,
                                     LoadFarString(BadCRC_EAs)));
                                else
                                    Info(slide, 1, ((char *)slide,
                                     LoadFarString(UnknownCmprMthdEA), m));
                            }
                            break;
                    }
                    return r;
                }
                break;

            case EF_NTSD:
                Trace((stderr, "eb_id: %i / eb_len: %ld\n", eb_id, eb_len));
                r = eb_len < EB_NTSD_L_LEN ? IZ_EF_TRUNC :
                 ((ef_buf[EB_HEADSIZE+EB_NTSD_VERSION] > EB_NTSD_MAX_VER) ?
                 (PK_WARN | 0x4000) :
                 test_compr_eb(__G__ ef_buf, (long)eb_len,
                 EB_NTSD_L_LEN, TEST_NTSD));
                if (r != PK_OK) {
                    if (uO.qflag)
                        Info(slide, 1, ((char *)slide, "%-22s ",
                         FnFilter1(G.filename)));
                    switch (r) {
                        case IZ_EF_TRUNC:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(TruncNTSD),
                             eb_len-(EB_NTSD_L_LEN+EB_CMPRHEADLEN), "\n"));
                            break;
# if defined(WIN32) && defined(NTSD_EAS)
                        case PK_WARN:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(InvalidSecurityEAs)));
                            break;
# endif
                        case PK_ERR:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(InvalidComprDataEAs)));
                            break;
                        case PK_MEM3:
                        case PK_MEM4:
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(NotEnoughMemEAs)));
                            break;
                        case (PK_WARN | 0x4000):
                            Info(slide, 1, ((char *)slide,
                             LoadFarString(UnsuppNTSDVersEAs),
                             (int)ef_buf[EB_HEADSIZE+EB_NTSD_VERSION]));
                            r = PK_WARN;
                            break;
                        default:
                            if ((r & 0xff) != PK_ERR)
                                Info(slide, 1, ((char *)slide,
                                 LoadFarString(UnknErrorEAs)));
                            else
                            {
                                ush m = (ush)(r >> 8);
                                if (m == DEFLATED)            /* GRR KLUDGE! */
                                    Info(slide, 1, ((char *)slide,
                                     LoadFarString(BadCRC_EAs)));
                                else
                                    Info(slide, 1, ((char *)slide,
                                     LoadFarString(UnknownCmprMthdEA), m));
                            }
                            break;
                    }
                    return r;
                }
                break;
            case EF_PKVMS:
                /* 2015-01-30 SMS.  Added sufficient-bytes test/message
                 * here.  (Removed defective eb_len test above.)
                 *
                 * If sufficient bytes (EB_PKVMS_MINLEN) are available,
                 * then compare the stored CRC value with the calculated
                 * CRC for the remainder of the data (and complain about
                 * a mismatch).
                 */
                if (eb_len < EB_PKVMS_MINLEN)
                {
                    /* Insufficient bytes available. */
                    Info( slide, 1,
                     ((char *)slide, LoadFarString( TooSmallEBlength),
                     eb_len, EB_PKVMS_MINLEN));
                }
                else if (makelong( ef_buf+ EB_HEADSIZE) !=
                 crc32( CRCVAL_INITIAL,
                 (ef_buf+ EB_HEADSIZE+ EB_PKVMS_MINLEN),
                 (extent)(eb_len- EB_PKVMS_MINLEN)))
                {
                    Info(slide, 1, ((char *)slide,
                      LoadFarString(BadCRC_EAs)));
                }
                break;
            case EF_JAVA:
                /* 2012-05-20 SMS.
                 * Setting java_cafe here may be too late to be useful.
                 * If it wasn't done when the central directory was
                 * processed, then now may be too late.  Perhaps it
                 * would make some sense not to set the flag here, if
                 * we're just now seeing a "CAFE" extra block here, than
                 * to set the flag now, and risk inconsistency with
                 * previous name processing.
                 */
                if (uO.java_cafe <= 0)
                {
                   Info( slide, 1, ((char *)slide,
                    LoadFarString( InfoInconsistentJavaCAFE)));
                }
                uO.java_cafe = 1;
                break;
            case EF_PKW32:
            case EF_PKUNIX:
            case EF_ASIUNIX:
            case EF_IZVMS:
            case EF_IZUNIX:
            case EF_VMCMS:
            case EF_MVS:
            case EF_SPARK:
            case EF_TANDEM:
            case EF_THEOS:
            case EF_AV:
            default:
                break;
        }
        ef_len -= (eb_len + EB_HEADSIZE);
        ef_buf += (eb_len + EB_HEADSIZE);
    }

    if (!uO.qflag)
        Info(slide, 0, ((char *)slide, " OK\n"));

    return PK_OK;

} /* TestExtraField(). */

#endif /* ndef SFX */


/*********************************/
/*  Function ef_scan_for_cafe()  */
/*********************************/

static void ef_scan_for_cafe( __G__ ef_buf, ef_len)
  __GDEF
  uch *ef_buf;
  long ef_len;
{
  /* 2012-05-20 SMS.
   * Accommodation for Java "jar" archives whose
   * (over-)simplified central header info ("version made
   * by") lacks any accurate host-system info.  (Internal
   * and external file atributes are zero, high byte of
   * "version made by" is zero, so we see "MS-DOS", and
   * try to adjust names accordingly, which damages Java's
   * UTF-8-encoded names.  See fileio.c:do_string().)
   *
   * Look for, and remember finding, a Java "CAFE" extra
   * field (once).  This affects name processing, so
   * waiting for TestExtraField() to detect it in a
   * local-header extra field would cause complaints about
   * inconsistent file names.  Small-sample evidence
   * suggests that if a Java "CAFE" extra field is
   * present, then it'll be in the first header, and
   * alone, so most of the robustness here is probably
   * wasted.  Also, if uO.java_cafe is not set once before
   * the first file name is processed, then we'll be
   * handling names inconsistently, so this had better be
   * adequate.
   */
  /* 2012-11-25 SMS.  (OUSPG report.)
   * Changed eb_len and ef_len from unsigned to signed, to
   * catch underflow of ef_len caused by corrupt/malicious
   * data.  (32-bit is adequate.  Used "long" to
   * accommodate any systems with 16-bit "int".)
   */
  ush eb_id;
  long eb_len;

  while (ef_len >= EB_HEADSIZE)
  {
    eb_id = makeword( ef_buf);
    if (eb_id == EF_JAVA)
    {
      /* Found one. */
      uO.java_cafe = 1;
    }

    eb_len = (unsigned)makeword( ef_buf+ EB_LEN);
    ef_len -= (eb_len + EB_HEADSIZE);
    ef_buf += (eb_len + EB_HEADSIZE);
  }
  if (uO.java_cafe == 0)
  {
    /* None found in the first extra field.  Quit looking. */
    uO.java_cafe = -1;
  }
} /* ef_scan_for_cafe(). */


#ifdef USE_EF_STREAM

/*********************************/
/* Function ef_scan_for_stream() */
/*********************************/

/* 2015-01-08 SMS.  New.
 *
 * Return (status) value:
 *   -1 if EF_STREAM not found,
 *    0 if EF_STREAM found and valid,
 *   >0 if error.
 * Returns bitmap in user's btmp[ *btmp_siz] array.
 * Returns bitmap length in user's btmp_siz.
 * Returns data in user's xlhdr structure.
 * Returns comment pointer in user's cmnt pointer (not NUL-terminated,
 * but length is in xlhdr.file_comment_length).  cmnt may be NULL, if
 * the user is not interested.
 */
static int ef_scan_for_stream( ef_ptr, ef_len, btmp_siz, btmp, xlhdr, cmnt)
    ZCONST uch *ef_ptr;                 /* Buffer containing extra field. */
    long ef_len;                        /* Total length of extra field. */
    int *btmp_siz;                      /* Size of bitmap array. */
    uch *btmp;                          /* Bitmap (array). */
    ext_local_file_hdr *xlhdr;          /* Extended local header data. */
    char **cmnt;                        /* File comment. */
{
  unsigned eb_id;
  long eb_len;
  uch bitmap;
  int bitmap_byte_max = 0;
  int bitmap_byte;
  int data_byte;
  int sts = -1;

/*---------------------------------------------------------------------------
    This function scans the extra field for EF_STREAM.
  ---------------------------------------------------------------------------*/

  if ((ef_len == 0) || (ef_ptr == NULL))
    return sts;

  TTrace((stderr,
   "\nef_scan_for_stream: scanning extra field of length %ld\n", ef_len));

  while (ef_len >= EB_HEADSIZE)
  {
    eb_id = makeword( ef_ptr+ EB_ID);
    eb_len = makeword( ef_ptr+ EB_LEN);
    ef_ptr += EB_HEADSIZE;
    ef_len -= EB_HEADSIZE;

    if (eb_len > ef_len)
    {
      /* Discovered some extra field inconsistency! */
      TTrace((stderr,
       "ef_scan_for_stream: block length %ld > rest ef_size %ld\n",
       eb_len, ef_len));
      sts = 1;
      break;
    }

    if (eb_id == EF_STREAM)
    {
      sts = 0;                                  /* Indicate EF_STREAM found. */
      if (cmnt != NULL)
      {
        *cmnt = NULL;                           /* Clear comment pointer. */
      }
      xlhdr->file_comment_length = 0;           /* Clear comment length. */

      /* Loop through (and save) the bitmap bytes. */
      data_byte = 0;
      do
      {
        if (bitmap_byte_max >= eb_len)          /* Stay within the block. */
        {
          /* Out of data before end-of-bitmap. */
          TTrace((stderr,
           "ef_scan_for_stream: No end-of-bitmap.\n"));
          sts = 2;
          break;
        }

        /* If the user has space for another, then return a bitmap byte. */
        bitmap = *(ef_ptr+ (data_byte++));
        if (bitmap_byte_max < *btmp_siz)
        {
          btmp[ bitmap_byte_max++] = bitmap;
        }
      } while ((bitmap& EB_STREAM_EOB) != 0);   /* Loop while more bitmap. */
      *btmp_siz = bitmap_byte_max;              /* Return bitmap length. */

      /* Loop through bitmap bytes.  Stay within the extra block. */
      for (bitmap_byte = 0; bitmap_byte < bitmap_byte_max; bitmap_byte++)
      {
        bitmap = btmp[ bitmap_byte];            /* Next bitmap byte. */
        switch (bitmap_byte)
        {
          case 0:
            if (bitmap& (1 << 0))               /* Version made by. */
            {
              if (data_byte+ 2 > eb_len)        /* Missing data. */
              {
                sts = 3;
                break;
              }
              xlhdr->version_made_by[ 0] = (ef_ptr+ data_byte)[ 0];
              xlhdr->version_made_by[ 1] = (ef_ptr+ data_byte)[ 1];
              data_byte += 2;
            }

            if (bitmap& (1 << 1))               /* Internal file attributes. */
            {
              if (data_byte+ 2 > eb_len)        /* Missing data. */
              {
                sts = 4;
                break;
              }
              xlhdr->internal_file_attributes = makeword( ef_ptr+ data_byte);
              data_byte += 2;
            }

            if (bitmap& (1 << 2))               /* External file attributes. */
            {
              if (data_byte+ 4 > eb_len)        /* Missing data. */
              {
                sts = 5;
                break;
              }
              xlhdr->external_file_attributes = makelong( ef_ptr+ data_byte);
              data_byte += 4;
            }

            if (bitmap& (1 << 3))               /* File comment. */
            {
              if (data_byte+ 2 > eb_len)        /* Missing data. */
              {
                sts = 6;
                break;
              }
              xlhdr->file_comment_length = makeword( ef_ptr+ data_byte);
              data_byte += 2;
              if (xlhdr->file_comment_length > 0)
              {
                if (data_byte+ xlhdr->file_comment_length >
                 eb_len)                        /* Missing data. */
                {
                  sts = 7;
                  break;
                }
                if (cmnt != NULL)
                {                               /* Set comment pointer. */
                  *cmnt = (char *)(ef_ptr+ data_byte);
                }
                data_byte += xlhdr->file_comment_length;
              }
            }

          default:                              /* Past known bits. */
            break;
        } /* switch */
        bitmap_byte++;                          /* Next bitmap index. */
      } /* for */
    } /* if (eb_id == EF_STREAM) */

    /* Advance to the next extra field block. */
    ef_ptr += eb_len;
    ef_len -= eb_len;
  } /* while */

  return sts;
} /* ef_scan_for_stream(). */

#endif /* def USE_EF_STREAM */


#if !defined(SFX) || defined(SFX_EXDIR)

/*********************************/
/*  Function extract_dest_dir()  */
/*********************************/

static int extract_dest_dir( __G)       /* Return PK-type error code. */
  __GDEF
{
  int error = PK_OK;

  /* If extracting, set the extraction destination directory.
   * If not in Freshen (-f) mode, create it, if necessary.)
   */
  if ((uO.exdir != (char *)NULL) && G.extract_flag)
  {
    G.create_dirs = !uO.fflag;
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
    if (G.has_win32_wide)
    {
      wchar_t *exdirw = local_to_wchar_string(uO.exdir);
      if ((error = checkdirw(__G__ exdirw, ROOT)) > MPN_INF_SKIP)
      {
        /* Out of memory, or non-dir file already exists. */
        (error == MPN_NOMEM) ? PK_MEM : PK_ERR;
      }
      izu_free(exdirw);
    }
    else
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
    {
      if ((error = checkdir(__G__ uO.exdir, ROOT)) > MPN_INF_SKIP)
      {
        /* Out of memory, or non-dir file already exists. */
        error = ((error == MPN_NOMEM) ? PK_MEM : PK_ERR);
      }
    }
  } /* if ((uO.exdir != (char *)NULL) && G.extract_flag) */

  return error;
} /* extract_dest_dir(). */

#endif /* !defined(SFX) || defined(SFX_EXDIR) */


/* UNKN_COMPR macro: Unsupported compression method tests.
 *
 * Assume that compression methods from Store (STORED = 0) through
 * Deflate64 (ENHDEFLATED = 9) are allowed, unless an UNKN_xxxx
 * exception is true.
 * Store (0) is always allowed.
 * Currently, Implode (6) is allowed for non-SFX.
 */
#ifdef LZW_CLEAN                        /* Shrink (1) not allowed. */
# define UNKN_SHR( mthd) (mthd == SHRUNK)
#else
# define UNKN_SHR( mthd) FALSE          /* Shrink (1) allowed. */
#endif

#if !defined( USE_UNREDUCE_PUBLIC) && !defined( USE_UNREDUCE_SMITH)
                                        /* Reduce (2-5) not allowed. */
# define UNKN_RED( mthd) (mthd >= REDUCED1 && mthd <= REDUCED4)
#else
# define UNKN_RED( mthd) FALSE          /* Reduce (2-5) allowed. */
#endif

#ifdef SFX                              /* Implode should have its own macro. */
# define UNKN_IMPL( mthd) (mthd == IMPLODED)
#else
# define UNKN_IMPL( mthd) FALSE         /* Implode (6) allowed. */
#endif

                                        /* Tokenize (7) not allowed. */
#define UNKN_TOKN( mthd) (mthd == TOKENIZED)

#ifdef DEFLATE_SUPPORT
# define UNKN_DEFL( mthd) FALSE         /* Deflate (8) allowed. */
#else
# define UNKN_DEFL( mthd) (mthd == DEFLATED)
#endif

#if defined( DEFLATE_SUPPORT) && defined( DEFLATE64_SUPPORT)
# define UNKN_DEFL64( mthd) FALSE       /* Deflate64 (9) allowed. */
#else
# define UNKN_DEFL64( mthd) (mthd == ENHDEFLATED)
#endif

/* Assume that compression methods above Deflate64 (ENHDEFLATED = 9) are
 * not allowed, unless an UNKN_xxxx exception is false.
 */
#ifdef IZ_CRYPT_AES_WG
# define UNKN_AES_WG( mthd) (mthd != AESENCRED)
#else
# define UNKN_AES_WG( mthd) TRUE        /* AES_WG (99) unknown. */
#endif

#ifdef BZIP2_SUPPORT
# define UNKN_BZ2( mthd) (mthd != BZIPPED)
#else
# define UNKN_BZ2( mthd) TRUE           /* Bzip2 (12) unknown. */
#endif

#ifdef JPEG_SUPPORT
# define UNKN_JPEG( mthd) (mthd != JPEGED)
#else
# define UNKN_JPEG( mthd) TRUE          /* JPEG (96) unknown */
#endif

#ifdef LZMA_SUPPORT
# define UNKN_LZMA( mthd) (mthd != LZMAED)
#else
# define UNKN_LZMA( mthd) TRUE          /* LZMA (14) unknown */
#endif

#ifdef PPMD_SUPPORT
# define UNKN_PPMD( mthd) (mthd != PPMDED)
#else
# define UNKN_PPMD( mthd) TRUE          /* PPMd (98) unknown */
#endif

#ifdef WAVP_SUPPORT
# define UNKN_WAVP( mthd) (mthd != WAVPACKED)
#else
# define UNKN_WAVP( mthd) TRUE          /* WavPack (97) unknown */
#endif


/******************************/
/*  Function unkn_cmpr_mthd() */
/******************************/

static int unkn_cmpr_mthd( __G)
  __GDEF
{
  /* Return 0: supported compression method
   *        1: unsupported compression method
   */
  ush mthd;
  int sts;

  mthd = G.crec.compression_method;

#ifdef IZ_CRYPT_AES_WG
  if (mthd == AESENCRED)
  {
    /* Scan the (central) extra field for an AES block. */
    sts = ef_scan_for_aes( G.extra_field,
                           G.crec.extra_field_length,
                           NULL,                        /* AES version. */
                           NULL,                        /* AES vendor. */
                           NULL,                        /* AES mode. */
                           &G.pInfo->cmpr_mthd_aes);    /* AES method. */
    if (sts == 1)
    {
      /* Found a valid AES_WG extra block.  Use its compression method. */
      mthd = G.pInfo->cmpr_mthd_aes;
    }
  }
#endif /* def IZ_CRYPT_AES_WG */

  sts = ((UNKN_DEFL( mthd) || UNKN_DEFL64( mthd) || UNKN_IMPL( mthd) ||
   UNKN_RED( mthd) || UNKN_SHR( mthd) || UNKN_TOKN( mthd)) ||
   ((mthd > ENHDEFLATED) &&
   (UNKN_AES_WG( mthd) && UNKN_BZ2( mthd) && UNKN_JPEG( mthd) &&
    UNKN_LZMA( mthd) && UNKN_PPMD( mthd) && UNKN_WAVP( mthd))));

  return sts;
}


/* UNZVERS_SUPPORT macro: Version Needed to Extract. */

#if defined(BZIP2_SUPPORT) && (UNZIP_VERSION < UNZIP_BZIP2_VERS)
#  define UNZVERS_SUPPORT( mthd) \
     (UNKN_BZ2( mthd) ? UNZIP_VERSION : UNZIP_BZIP2_VERS)
#else
#  define UNZVERS_SUPPORT( mthd) UNZIP_VERSION
#endif


/***************************/
/*  Function store_info()  */
/***************************/

static int store_info(__G)      /* Return 0 if ok, non-zero if skipping. */
    __GDEF
{
/*---------------------------------------------------------------------------
    Check central directory info for version/compatibility requirements.
  ---------------------------------------------------------------------------*/
    G.pInfo->encrypted = G.crec.general_purpose_bit_flag & 1;   /* bit field */
    G.pInfo->ExtLocHdr = (G.crec.general_purpose_bit_flag & 8) == 8;  /* bit */
    G.pInfo->textfile = G.crec.internal_file_attributes & 1;    /* bit field */
    G.pInfo->crc = G.crec.crc32;
    G.pInfo->compr_size = G.crec.csize;
    G.pInfo->uncompr_size = G.crec.ucsize;

    switch (uO.aflag) {
        case 0:
            G.pInfo->textmode = FALSE;   /* bit field */
            break;
        case 1:
            G.pInfo->textmode = G.pInfo->textfile;   /* auto-convert mode */
            break;
        default:  /* case 2: */
            G.pInfo->textmode = TRUE;
            break;
    }

    /* First, complain about an unsupported compression method. */
    if (unkn_cmpr_mthd( __G))
    {
#ifdef IZ_CRYPT_AES_WG
        ush mthd;

        if (G.crec.compression_method == AESENCRED)
          mthd = G.pInfo->cmpr_mthd_aes;
        else
          mthd = G.crec.compression_method;

# define MTHD mthd
#else /* def IZ_CRYPT_AES_WG */
# define MTHD G.crec.compression_method
#endif /* def IZ_CRYPT_AES_WG [else] */

        if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))) {
#ifndef SFX
            unsigned cmpridx;

            if ((cmpridx = find_compr_idx( MTHD)) < NUM_METHODS)
                Info(slide, 0x401, ((char *)slide, LoadFarString(ComprMsgName),
                  FnFilter1(G.filename),
                  LoadFarStringSmall(ComprNames[cmpridx])));
            else
#endif
                Info(slide, 0x401, ((char *)slide, LoadFarString(ComprMsgNum),
                  FnFilter1(G.filename), MTHD));
        }
        return 1;
    }

    /* Second, complain about an insufficient version number. */
    if (G.crec.version_needed_to_extract[1] == VMS_)
    {
        if (G.crec.version_needed_to_extract[0] > VMS_UNZIP_VERSION) {
            if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
                Info(slide, 0x401, ((char *)slide, LoadFarString(VersionMsg),
                  FnFilter1(G.filename), "VMS",
                  G.crec.version_needed_to_extract[0] / 10,
                  G.crec.version_needed_to_extract[0] % 10,
                  VMS_UNZIP_VERSION / 10, VMS_UNZIP_VERSION % 10));
            return 2;
        }
#ifndef VMS   /* won't be able to use extra field, but still have data */
        else if (!uO.tflag && !IS_OVERWRT_ALL)  /* If -o, extract anyway. */
        {
            Info(slide, 0x481, ((char *)slide, LoadFarString(VMSFormatQuery),
              FnFilter1(G.filename)));
            if ((fgets_ans( __G) < 0) || (toupper( *G.answerbuf != 'Y')))
                return 3;
        }
#endif /* ndef VMS */
    /* usual file type:  don't need VMS to extract */
    }
    else if (G.crec.version_needed_to_extract[0] > UNZVERS_SUPPORT( MTHD))
    {
        if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
            Info(slide, 0x401, ((char *)slide, LoadFarString(VersionMsg),
              FnFilter1(G.filename), "PK",
              G.crec.version_needed_to_extract[0] / 10,
              G.crec.version_needed_to_extract[0] % 10,
              UNZVERS_SUPPORT( MTHD) / 10, UNZVERS_SUPPORT( MTHD) % 10));
        return 4;
    }

#ifndef IZ_CRYPT_ANY
    /* Third, complain about missing encryption support. */
    if (G.pInfo->encrypted) {
        if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
            Info(slide, 0x401, ((char *)slide, LoadFarString(SkipEncrypted),
              FnFilter1(G.filename)));
        return 5;
    }
#endif /* ndef IZ_CRYPT_ANY */

#ifndef SFX
    /* store a copy of the central header filename for later comparison */
    if ((G.pInfo->cfilname = zfmalloc(strlen(G.filename) + 1)) == NULL) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(WarnNoMemCFName),
          FnFilter1(G.filename)));
    } else
        zfstrcpy(G.pInfo->cfilname, G.filename);
#endif /* ndef SFX */

    /* map whatever file attributes we have into the local format */
    mapattr(__G);   /* GRR:  worry about return value later */

    G.pInfo->diskstart = G.crec.disk_number_start;
    G.pInfo->offset = (zoff_t)G.crec.relative_offset_local_header;

    return 0;
} /* store_info(). */


#ifdef SYMLINKS

/***********************************/
/* Function set_deferred_symlink() */
/***********************************/

static void set_deferred_symlink(__G__ slnk_entry)
    __GDEF
    slinkentry *slnk_entry;
{
    int errno1 = 0;
    int sts1;
    int sts2;
    extent ucsize = slnk_entry->targetlen;
    char *linkfname = slnk_entry->fname;
    char *linktarget = (char *)izu_malloc(ucsize+1);
#ifdef WIN32
    char err_msg[ 32];
#endif /* def WIN32 */

#ifdef VMS
    static int vms_symlink_works = -1;

    if (vms_symlink_works < 0)
    {
        /* Test symlink() with an invalid file name.  If errno comes
         * back ENOSYS ("Function not implemented"), then don't try to
         * use it below on the symlink placeholder text files.
         */
        vms_symlink_works = symlink( "", "?");
        if (errno == ENOSYS)
            vms_symlink_works = 0;
        else
            vms_symlink_works = 1;
    }
#endif /* def VMS */

    if (!linktarget) {
        Info(slide, 0x201, ((char *)slide,
          LoadFarString(SymLnkWarnNoMem), FnFilter1(linkfname)));
        return;
    }
    linktarget[ucsize] = '\0';
    /* Open link placeholder for reading. */
#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
    if (slnk_entry->wide)
    {
        G.outfile = zfopenw( (wchar_t *)linkfname, FOPR_W);
    }
    else
    {
#else /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
    {
#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */
        G.outfile = zfopen( linkfname, FOPR);
    }
    /* Check that the following conditions are all fulfilled:
     * a) the placeholder file exists,
     * b) the placeholder file contains exactly "ucsize" bytes
     *    (read the expected placeholder content length + 1 extra byte, this
     *    should return the expected content length),
     * c) the placeholder content matches the link target specification as
     *    stored in the symlink control structure.
     */
    if (!G.outfile ||
        fread( linktarget, 1, (ucsize+ 1), G.outfile) != ucsize ||
        strcmp( slnk_entry->target, linktarget))
    {
        Info(slide, 0x201, ((char *)slide,
          LoadFarString(SymLnkWarnInvalid), FnFilter1(linkfname)));
        izu_free(linktarget);
        if (G.outfile)
            fclose(G.outfile);
        return;
    }
    fclose(G.outfile);                  /* Close "data" file for good... */

#ifdef VMS
    if (vms_symlink_works == 0)
    {
        Info(slide, 0, ((char *)slide, LoadFarString(SymLnkFinish),
          FnFilter1(linkfname), FnFilter2(linktarget)));
        Info(slide, 0x401, ((char *)slide, LoadFarString(SymLnkError),
          strerror( ENOSYS)));
        izu_free(linktarget);
        return;
    }
#endif /* def VMS */

    /* Delete the old placeholder, and create the real symlink. */
#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
    sts1 = _wunlink( (wchar_t *)linkfname);
    errno1 = errno;
    sts2 = symlinkw( linktarget, (wchar_t *)linkfname, slnk_entry->is_dir);
#else /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
    sts1 = unlink( linkfname);
    errno1 = errno;
    sts2 = symlink( linktarget, linkfname);
#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */

    if (QCOND2 || (sts1 != 0) || (sts2 != 0))
    {
#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
        if (slnk_entry->wide)
        {
            Info(slide, 0, ((char *)slide, LoadFarString( SymLnkFinishW),
             FnFilterW1( (ZCONST wchar_t *)linkfname),
             FnFilter2( linktarget)));
        }
        else
        {
#else /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
        {
#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) [else] */

            Info(slide, 0, ((char *)slide, LoadFarString(SymLnkFinish),
             FnFilter1(linkfname), FnFilter2(linktarget)));
        }
    }

#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
# define WIDE_STR (slnk_entry->wide ? "w" : "")
#else
# define WIDE_STR ""
#endif
    if (sts1 != 0)
    {
        Info( slide, 0x61, ((char *)slide, LoadFarString( SymLnkErrUnlink),
         WIDE_STR, strerror( errno1)));
    }
    if (sts2 != 0)
    {
#ifdef WIN32
        if (sts2 == 1314)
        {
          /* Insufficient privilege error. */
          Info( slide, 0x61, ((char *)slide,
           LoadFarString( SymLnkErrSymlinkPriv)));
        }
        else
        {
            /* Use "decimal (0xhexadecimal)" msg for GetLast Error() result. */
            sprintf( err_msg, "%d (0x%x)", sts2, sts2);
            Info( slide, 0x61, ((char *)slide,
             LoadFarString( SymLnkErrSymlink), WIDE_STR, err_msg));
        }
#else /* def WIN32 */
        /* Use normal C RTL message for errno. */
        Info( slide, 0x61, ((char *)slide,
         LoadFarString( SymLnkErrSymlink), WIDE_STR, strerror( errno)));
#endif /* def WIN32 [else] */
    }

    izu_free(linktarget);
#ifdef SET_SYMLINK_ATTRIBS
    set_symlnk_attribs(__G__ slnk_entry);
#endif
    return;                             /* can't set time on symlinks */

} /* set_deferred_symlink(). */


/************************************/
/* Function set_deferred_symlinks() */
/************************************/

static int set_deferred_symlinks(__G)   /* Return PK-type error code. */

    __GDEF
{
  int error = PK_OK;                    /* (What could go wrong?) */

  if (G.slink_last != NULL)
  {
    if (QCOND2)
      Info(slide, 0, ((char *)slide, LoadFarString(SymLnkDeferred)));
      while (G.slink_head != NULL)
      {
        set_deferred_symlink(__G__ G.slink_head);
        /* remove the processed entry from the chain and free its memory */
        G.slink_last = G.slink_head;
        G.slink_head = G.slink_last->next;
        izu_free(G.slink_last);
      }
      G.slink_last = NULL;
    }

  return error;
} /* set_deferred_symlinks(). */

#endif /* SYMLINKS */


#ifdef SET_DIR_ATTRIB

/************************/
/*  Function dircomp()  */
/************************/

/* Comparison functions for sorting saved directories.  Sorting lets us
 * set directory permissions from the bottom up, as required.
 */

static int Cdecl dircomp( OFT( ZCONST zvoid *) a, OFT( ZCONST zvoid *) b)
# ifdef NO_PROTO
    ZCONST zvoid *a;
    ZCONST zvoid *b;
# endif /* def NO_PROTO */
{
    /* Order is significant:  This sorts in reverse order (deepest first). */
    return strcmp((*(direntry **)b)->fn, (*(direntry **)a)->fn);
 /* return namecmp((*(direntry **)b)->fn, (*(direntry **)a)->fn); */
} /* dircomp(). */


#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)

/*************************/
/*  Function dircompw()  */
/*************************/

static int Cdecl dircompw( OFT( ZCONST zvoid *) a, OFT( ZCONST zvoid *) b)
# ifdef NO_PROTO
    ZCONST zvoid *a;
    ZCONST zvoid *b;
# endif /* def NO_PROTO */
{
    /* Order is significant:  This sorts in reverse order (deepest first). */
    return wcscmp((*(direntryw **)b)->fnw, (*(direntryw **)a)->fnw);
 /* return namecmp((*(direntry **)b)->fn, (*(direntry **)a)->fn); */
} /* dircompw(). */

# endif /* (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)) */

#endif /* def SET_DIR_ATTRIB */


# ifdef IZ_CRYPT_AES_WG

/****************************/
/*  Function aes_wg_prep()  */
/****************************/

int aes_wg_prep( __G__ temp_cmpr_mthd_p, temp_sto_size_decr_p )
  __GDEF
  ush *temp_cmpr_mthd_p;
  int *temp_sto_size_decr_p;
{
  int error = PK_OK;                    /* Return PK-type error code. */

  /* Analyze any AES encryption extra block before calculating
   * the true uncompressed file size.
   */
  if (G.lrec.compression_method == AESENCRED)
  {
    /* Extract info from an AES extra block, if there is one. */
    /* Set mode negative.  (Valid values are positive.) */
    G.pInfo->cmpr_mode_aes = -1;
    /* Scan the (local) extra field for an AES block. */
    ef_scan_for_aes( G.extra_field,
                     G.lrec.extra_field_length,
                     &G.pInfo->cmpr_vers_aes,   /* AES version. */
                     NULL,                      /* AES vendor. */
                     &G.pInfo->cmpr_mode_aes,   /* AES mode. */
                     &G.pInfo->cmpr_mthd_aes);  /* AES method. */

    if ((G.pInfo->cmpr_mode_aes <= 0) || (G.pInfo->cmpr_mode_aes > 3))
    {
      Info(slide, 0x401, ((char *)slide,
       LoadFarString(BadAesExtFieldMsg),
       FnFilter1(G.filename), G.pInfo->cmpr_mode_aes));
      error = PK_ERR;   /* Skip this member. */
    }
    else
    {
      *temp_cmpr_mthd_p = G.pInfo->cmpr_mthd_aes;
      *temp_sto_size_decr_p = MAC_LENGTH( G.pInfo->cmpr_mode_aes)+
       SALT_LENGTH( G.pInfo->cmpr_mode_aes)+ PWD_VER_LENGTH;
    }
  }
  else
  {
    /* Use non-AES values. */
    *temp_cmpr_mthd_p = G.lrec.compression_method;
    *temp_sto_size_decr_p = RAND_HEAD_LEN;
  }

  return error;
} /* aes_wg_prep(). */

#endif /* def IZ_CRYPT_AES_WG */


/***************************/
/*  Function size_check()  */
/***************************/

int size_check( __G__ real_stored_size_decr)
  __GDEF
  int real_stored_size_decr;
{
  int error = PK_OK;                    /* Return PK-type error code. */

  /* Size consistency checks must come after reading in the local extra
   * field, so that any Zip64 extension local e.f. block has already
   * been processed.
   */
  zusz_t csiz_decrypted = G.lrec.csize;

  if (G.pInfo->encrypted)
  {
    csiz_decrypted -= real_stored_size_decr;
  }
  if (G.lrec.ucsize != csiz_decrypted)
  {
    Info(slide, 0x401, ((char *)slide,
     LoadFarStringSmall2(WrnStorUCSizCSizDiff), FnFilter1(G.filename),
     FmZofft(G.lrec.ucsize, NULL, "u"), FmZofft(csiz_decrypted, NULL, "u")));
    G.lrec.ucsize = csiz_decrypted;
    error = PK_WARN;
  }

  return error;
} /* size_check(). */


#ifdef IZ_CRYPT_ANY

/*******************************/
/*  Function password_check()  */
/*******************************/

int password_check( __G)
  __GDEF
{
  int error;                            /* Return PK-type error code. */

  error = decrypt(__G__ uO.pwdarg);
  if (error != PK_OK)
  {
    if (error == PK_WARN)
    {
      if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
      {
        Info(slide, 0x401, ((char *)slide,
         LoadFarString(SkipIncorrectPasswd), FnFilter1(G.filename)));
      }
    }
    else
    {
      Info(slide, 0x401, ((char *)slide,
       LoadFarString(SkipCannotGetPasswd), FnFilter1(G.filename)));
    }
  }

  return error;
} /* password_check(). */

#endif /* def IZ_CRYPT_ANY */


#if defined( UNIX) && defined( __APPLE__)

/*******************************/
/*  Function detect_apl_dbl()  */
/*******************************/

static int detect_apl_dbl( __G)
  __GDEF
{
  int error = PK_OK;                    /* Return PK-type error code. */

  /* Unless the user objects, or the destination volume does not
   * support setattrlist(), detect an AppleDouble file (by name),
   * and set flags and adjusted file name accordingly.
   */
  G.apple_double = 0;           /* Assume it's not an AppleDouble file. */
  if ((!uO.J_flag) && G.exdir_attr_ok)
  {
    int sts;

    /* Excise the "._" name prefix from the AppleDouble file name. */
    sts = revert_apl_dbl_path( G.filename, G.ad_filename);

    if (sts < 0)                /* Skip a sequestration directory. */
    {
      error = PK_WARN;          /* Skip, but no problem. */
    }
    else
    {
      G.apple_double = sts;     /* Set the AppleDouble flag. */
      if (G.apple_double)
      {
        /* 2013-08-16 SMS.
         * This test might make more sense after mapname()
         * changes everything.
         */
        /* Check that the file name will not be too long when the
         * "/rsrc" (APL_DBL_SUFX) string is appended (fileio.c:
         * open_outfile()).  (strlen() ignores its NUL, sizeof()
         * includes its NUL.  FILNAMSIZ includes a NUL.)
         */
        if (strlen( G.ad_filename)+ sizeof( APL_DBL_SUFX) > FILNAMSIZ)
        {
          /* Name too long.  Skip.  Problem. */
          Info(slide, 0x401, ((char *)slide,
           AplDblNameTooLong, G.ad_filename));
          error = PK_ERR;
        }
        else if (strcmp( G.ad_filename, G.pq_filename) == 0)
        {
          /* The current file is the AppleDouble file for the previous
           * file (their names match), so arrange to handle this
           * AppleDouble file the way the previous file was handled.
           */
          if (*G.pr_filename != '\0')
          {
            /* The previous normal file had its name replaced, so
             * replace the name of this AppleDouble file name, too.
             * Without extra effort, the "renamed" flag will be
             * misleadingly FALSE for mapname() later, but the
             * preceding normal file should have paved the way by
             * getting all the directories created as needed.
             */
            strcpy( G.ad_filename, G.pr_filename);
          }
          else
          {
            *G.pq_filename = '\0';  /* Pointless? */
            *G.pr_filename = '\0';  /* Pointless? */
          }
        }
        else
        {
          /* Looks like an AppleDouble file, but its name does not match
           * the previous normal file.  We're confused.  Should be an
           * error/warning?  For now, just clear the AppleDouble flag,
           * and pretend that it's a normal file.
           */
          G.apple_double = 0;
        }
      }
      else /* if (G.apple_double) */
      {
        /* Save a normal file name for comparison with the next
         * AppleDouble file name.
         */
        strcpy( G.pq_filename, G.filename);
        *G.pr_filename = '\0';                  /* Not yet renamed. */
      } /* if (G.apple_double) [else] */
    } /* if (sts < 0) */
  } /* if ((!uO.J_flag) && G.exdir_attr_ok) */

  return error;
} /* detect_apl_dbl(). */

#endif /* defined( UNIX) && defined( __APPLE__) */


# ifndef SFX

/********************************/
/*  Function backslash_slash()  */
/********************************/
static int backslash_slash( __G)
  __GDEF
{
  int error = PK_OK;                    /* Return PK-type error code. */

  if ((G.pInfo->hostnum == FS_FAT_) && !MBSCHR(G.filename, '/'))
  {
    char *p = G.filename;

    if (*p)
    {
      do
      {
        if (*p == '\\')
        {
          if (!G.reported_backslash)
          {
            Info(slide, 0x21, ((char *)slide,
             LoadFarString(BackslashPathSep), G.zipfn));
            G.reported_backslash = TRUE;
            error = PK_WARN;
          }
          *p = '/';
        }
      } while (*PREINCSTR(p));
    }
  }

  return error;
} /* backslash_slash(). */

# endif /* ndef SFX */


/*************************************/
/*  Function extract_test_trailer()  */
/*************************************/

static int extract_test_trailer(__G__ n_fil, n_bad_pwd, n_skip, err_in_arch)
  __GDEF
  ulg n_fil;
  ulg n_bad_pwd;
  ulg n_skip;
  int err_in_arch;              /* Return PK-type error code. */
{

  if (uO.tflag)
  {
    /* Put out test summary report trailer. */
    ulg num = n_fil - n_bad_pwd;

    if (uO.qflag < 2)           /* GRR 930710:  was (uO.qflag == 1) */
    {
      if (err_in_arch)
      {
        char errnrstr[ 16];

        if (uO.vflag)
          sprintf( errnrstr, "(%d) ", err_in_arch);
        else
          *errnrstr = '\0';

        Info(slide, 0, ((char *)slide, LoadFarString(ErrorInArchive),
         (err_in_arch == PK_WARN)? "warning-" : "", errnrstr, G.zipfn));
      }
      else if (num == 0L)
        Info(slide, 0, ((char *)slide, LoadFarString(ZeroFilesTested),
         G.zipfn));
      else if (G.process_all_files && (n_skip+n_bad_pwd == 0L))
        Info(slide, 0, ((char *)slide, LoadFarString(NoErrInCompData),
         G.zipfn));
      else
        Info(slide, 0, ((char *)slide, LoadFarString(NoErrInTestedFiles),
         G.zipfn, num, (num==1L)? "":"s"));
        if (n_skip > 0L)
          Info(slide, 0, ((char *)slide, LoadFarString(FilesSkipped),
           n_skip, (n_skip==1L)? "":"s"));
#ifdef IZ_CRYPT_ANY
          if (n_bad_pwd > 0L)
            Info(slide, 0, ((char *)slide, LoadFarString(FilesSkipBadPasswd),
             n_bad_pwd, (n_bad_pwd==1L)? "":"s"));
#endif /* def IZ_CRYPT_ANY */
    }
  }

  /* Put out extract or test summary report trailer. */

  /* give warning if files not tested or extracted (first condition can still
   * happen if zipfile is empty and no files specified on command line)
   */
  if ((n_fil == 0) && err_in_arch <= PK_WARN)
  {
    if (n_skip > 0L)
      err_in_arch = IZ_UNSUP; /* unsupport. compression/encryption */
    else
      err_in_arch = PK_FIND;  /* no files found at all */
  }
#ifdef IZ_CRYPT_ANY
  else if ((n_fil == n_bad_pwd) && err_in_arch <= PK_WARN)
    err_in_arch = IZ_BADPWD;    /* bad passwd => all files skipped */
#endif /* def IZ_CRYPT_ANY */
  else if ((n_skip > 0L) && err_in_arch <= PK_WARN)
    err_in_arch = IZ_UNSUP;     /* was PK_WARN; Jean-loup complained */
#ifdef IZ_CRYPT_ANY
  else if ((n_bad_pwd > 0L) && !err_in_arch)
    err_in_arch = PK_WARN;
#endif /* def IZ_CRYPT_ANY */

  return err_in_arch;
} /* extract_test_trailer(). */


/***************************/
/*  Function action_msg()  */
/***************************/

#ifdef DOS_FLX_NLM_OS2_W32
# define NEWLINE "\r\n"
#else /* def DOS_FLX_NLM_OS2_W32 */
# define NEWLINE "\n"
#endif /* def DOS_FLX_NLM_OS2_W32 [else] */

#define TYPE_BINARY "[binary]"
#define TYPE_EMPTY  "[empty] "
#define TYPE_TEXT   "[text]  "
#ifdef CMS_MVS
# define TYPE_EBC   "[ebcdic]";         /* Not actually used? */
#endif

void action_msg( __G__ action, flag)
  __GDEF
  ZCONST char *action;
  int flag;                             /* 0: Name only; 1: Name + [type]. */
{
  char *str1;
  char *str2;

  if (flag == 0)
  {
    str1 = "";
    str2 = "";
  }
  else
  {
    str1 = (((uO.aflag != 1) /* && G.pInfo->textfile==G.pInfo->textmode */ ) ?
            "" : ((G.lrec.ucsize == 0L) ?
            TYPE_EMPTY : (G.pInfo->textfile ? TYPE_TEXT : TYPE_BINARY)));

    str2 = (uO.cflag ? NEWLINE : "");
  }
#if (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE))
  if (G.has_win32_wide)
  {
    /* Display wide path. */
    Info(slide, 0, ((char *)slide, LoadFarString(ActionMsgw), action,
     FnFilterW1(G.unipath_widefilename), str1, str2));
  }
  else
#endif /* (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)) */
  {
    /* Display normal path. */
    Info(slide, 0, ((char *)slide, LoadFarString(ActionMsg), action,
     FnFilter1(G.filename), str1, str2));
  }
} /* action_msg(). */


/***************************************/
/*  Function extract_or_test_member()  */
/***************************************/

/* Note that the names "temp_compression_method" and
 * "temp_stored_size_decr" are used for local variables (and the related
 * macros REAL_COMPRESSION_METHOD and REAL_STORED_SIZE_DECR are used) in
 * multiple functions.
 */
#ifdef IZ_CRYPT_AES_WG
#  define REAL_COMPRESSION_METHOD temp_compression_method
#  define REAL_STORED_SIZE_DECR temp_stored_size_decr
#else /* def IZ_CRYPT_AES_WG */
   /* RAND_HEAD_LEN normally comes from crypt.h, but may be disabled,
    * if CRYPT is not defined.
    */
#  ifndef RAND_HEAD_LEN
#    define RAND_HEAD_LEN 12
#  endif /* ndef RAND_HEAD_LEN */
#  define REAL_COMPRESSION_METHOD G.lrec.compression_method
#  define REAL_STORED_SIZE_DECR RAND_HEAD_LEN
#endif /* def IZ_CRYPT_AES_WG) [else] */


/* wsize is used in extract_or_test_member() and various method-specific
 * UZ functions: UZbunzip2(), UZlzma(), UZppmd().
 */
#if defined(DLL) && !defined(NO_SLIDE_REDIR)
# define wsize G._wsize         /* wsize is a variable. */
#else
# define wsize WSIZE            /* wsize is a constant. */
#endif


static int extract_or_test_member(__G)  /* return PK-type error code */
     __GDEF
{
    register int b;
    int r;
    int error = PK_OK;

    /* AES-encrypted data include a trailer which must not be put out.
     * For STORED data, the output bytes are counted in bytes_put_out,
     * and limited (below) to the known uncompressed data size.
     */
    zusz_t bytes_put_out;

#ifdef IZ_CRYPT_AES_WG
    int g_csize_adj;                    /* Temporary adjustment to G.csize. */
    int aes_mac_mismatch;               /* Flag indicating MAC mismatch. */
    ush temp_compression_method;        /* Actual compression method. */
    /* AES Message Authentication Code (MAC) storage.
     * Note that MAC_LENGTH is actually a constant, independent of its
     * (mode) argument.  See aes_wg/fileenc.h.
     */
    uch aes_wg_mac_file[ MAC_LENGTH(0)];        /* AES MAC from file. */
    uch aes_wg_mac_calc[ MAC_LENGTH(0)];        /* AES MAC calculated. */
#endif /* def IZ_CRYPT_AES_WG */

/*---------------------------------------------------------------------------
    Initialize variables, buffers, etc.
  ---------------------------------------------------------------------------*/

    G.bits_left = 0;
    G.bitbuf = 0L;       /* unreduce and unshrink only */
    G.zipeof = 0;
    G.newfile = TRUE;
    G.crc32val = CRCVAL_INITIAL;

#ifdef SYMLINKS
    /* If file is a (POSIX-compatible) symbolic link and we are extracting
     * to disk, prepare to restore the link. */
    G.symlnk = (G.pInfo->symlink &&
                !uO.tflag && !uO.cflag && (G.lrec.ucsize > 0));
#endif /* SYMLINKS */

    if (uO.tflag) {
#ifdef ENABLE_USER_PROGRESS
        G.action_msg_str = "test";
#endif /* def ENABLE_USER_PROGRESS */
        if (!uO.qflag)
        {
            action_msg( __G__ "test", 0);
        }
    } else {
#ifdef DLL
        if (uO.cflag && !G.redirect_data)
#else
        if (uO.cflag)
#endif
        {
#if defined(OS2) && defined(__IBMC__) && (__IBMC__ >= 200)
            G.outfile = freopen("", "wb", stdout);   /* VAC++ ignores setmode */
#else
            G.outfile = stdout;
#endif

#ifdef DOS_FLX_NLM_OS2_W32
# if defined(__HIGHC__) && !defined(FLEXOS)
            setmode(G.outfile, _BINARY);
# else /* defined(__HIGHC__) && !defined(FLEXOS) */
            setmode(fileno(G.outfile), O_BINARY);
# endif /* defined(__HIGHC__) && !defined(FLEXOS) [else] */
#endif /* def DOS_FLX_NLM_OS2_W32 */

#ifdef VMS
            /* VMS:  required even for stdout! */
            if ((r = open_outfile(__G)) != 0)
                switch (r) {
                  case OPENOUT_SKIPOK:
                    return PK_OK;
                  case OPENOUT_SKIPWARN:
                    return PK_WARN;
                  default:
                    return PK_DISK;
                }
        } else if ((r = open_outfile(__G)) != 0)
            switch (r) {
              case OPENOUT_SKIPOK:
                return PK_OK;
              case OPENOUT_SKIPWARN:
                return PK_WARN;
              default:
                return PK_DISK;
            }
#else /* def VMS */
        } else if (open_outfile(__G))
            return PK_DISK;
#endif /* def VMS [else] */
    }

/*---------------------------------------------------------------------------
    Unpack the file.
  ---------------------------------------------------------------------------*/

#ifdef IZ_CRYPT_AES_WG
    if (G.lrec.compression_method == AESENCRED)
    {
        /* Subtract the AES_WG MAC data size from G.csize, to keep the
         * MAC data away from the method-specific extraction functions,
         * below.  Remember doing this, so that G.csize can be restored
         * later, before trying to read the MAC data.
         */
        g_csize_adj = MAC_LENGTH( G.pInfo->cmpr_mode_aes);
        G.csize -= g_csize_adj;
        if (G.csize < 0)                /* Data sanity check. */
            return PK_WARN;

        /* The "compression_method" is AES_WG, so use the real method. */
        temp_compression_method = G.pInfo->cmpr_mthd_aes;
    }
    else
    {
        /* Not AES_WG-encrypted, so use the apparent compression_method,
         * and clear the AES MAC-mismatch indicator.
         */
        temp_compression_method = G.lrec.compression_method;
        aes_mac_mismatch = 0;
    }
#endif /* def IZ_CRYPT_AES_WG */

    defer_leftover_input(__G);  /* So NEXTBYTE bounds check will work. */

    switch (REAL_COMPRESSION_METHOD) {
        case STORED:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "link";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2) {
#ifdef SYMLINKS
                if (G.symlnk)   /* can also be deflated, but rarer... */
                {
                    action_msg( __G__ "link", 0);
                }
                else
#endif /* SYMLINKS */
                {
                    action_msg( __G__ "extract", 1);
#ifdef ENABLE_USER_PROGRESS
                    G.action_msg_str = "extract";
#endif /* def ENABLE_USER_PROGRESS */
                }
              }
            }
#if defined(DLL) && !defined(NO_SLIDE_REDIR)
            if (G.redirect_slide) {
                wsize = G.redirect_size; redirSlide = G.redirect_buffer;
            } else {
                wsize = WSIZE; redirSlide = slide;
            }
#endif
            G.outptr = redirSlide;
            G.outcnt = 0L;
            bytes_put_out = 0;

            while (bytes_put_out < G.pInfo->uncompr_size)
            {
                if ((b = NEXTBYTE) == EOF)
                    break;
                *G.outptr++ = (uch)b;
                bytes_put_out++;

                if (++G.outcnt == wsize) {
                    error = flush(__G__ redirSlide, G.outcnt, 0);
                    G.outptr = redirSlide;
                    G.outcnt = 0L;
                    if (error != PK_COOL || G.disk_full) break;
                }
            }
            if (G.outcnt) {        /* flush final (partial) buffer */
                r = flush(__G__ redirSlide, G.outcnt, 0);
                if (error < r) error = r;

            bytes_put_out += G.outcnt;
            }
            break;

#ifndef LZW_CLEAN
        case SHRUNK:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = Unshrink;
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ LoadFarStringSmall(Unshrink), 1);
              }
            }
            if ((r = unshrink(__G)) != PK_COOL) {
                if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Unshrink),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Unshrink)));
                }
                error = r;
            }
            break;
#endif /* ndef LZW_CLEAN */

#if defined( USE_UNREDUCE_PUBLIC) || defined( USE_UNREDUCE_SMITH)
        case REDUCED1:
        case REDUCED2:
        case REDUCED3:
        case REDUCED4:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "unreduc";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ "unreduc", 1);
              }
            }
            if ((r = unreduce(__G)) != PK_COOL) {
                /* unreduce() returns only PK_COOL, PK_DISK, or IZ_CTRLC */
                error = r;
            }
            break;
#endif /* defined( USE_UNREDUCE_PUBLIC) || defined( USE_UNREDUCE_SMITH) */

#ifndef SFX                     /* Implode should have its own macro. */
        case IMPLODED:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "explod";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ "explod", 1);
              }
            }
            if ((r = explode(__G)) != 0) {
                if (r == 5) { /* treat 5 specially */
                    int warning = ((zusz_t)G.used_csize <= G.lrec.csize);

                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(LengthMsg),
                          "", warning ? "warning" : "error",
                          FmZofft(G.used_csize, NULL, NULL),
                          FmZofft(G.lrec.ucsize, NULL, "u"),
                          warning ? "  " : "",
                          FmZofft(G.lrec.csize, NULL, "u"),
                          " [", FnFilter1(G.filename), "]"));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(LengthMsg),
                          "\n", warning ? "warning" : "error",
                          FmZofft(G.used_csize, NULL, NULL),
                          FmZofft(G.lrec.ucsize, NULL, "u"),
                          warning ? "  " : "",
                          FmZofft(G.lrec.csize, NULL, "u"),
                          "", "", "."));
                    error = warning ? PK_WARN : PK_ERR;
                } else if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Explode),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Explode)));
                    error = ((r == 3) ? PK_MEM3 : PK_ERR);
                } else {
                    error = r;
                }
            }
            break;
#endif /* ndef SFX */

#ifdef DEFLATE_SUPPORT
        case DEFLATED:
#  ifdef DEFLATE64_SUPPORT
        case ENHDEFLATED:
#  endif
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "inflat";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ "inflat", 1);
              }
            }

#  ifndef USE_ZLIB  /* zlib's function is called inflate(), too */
#    define UZinflate inflate
#  endif
            if ((r = UZinflate(__G__
                               (G.lrec.compression_method == ENHDEFLATED)))
                != 0) {
                if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate)));
                    error = ((r == 3) ? PK_MEM3 : PK_ERR);
                } else {
                    error = r;
                }
            }
            break;
#endif

#ifdef BZIP2_SUPPORT
        case BZIPPED:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "bunzipp";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ "bunzipp", 1);
              }
            }

            if ((r = UZbunzip2(__G)) != 0)
            {
                if (r < PK_DISK)
                {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(BUnzip),
                          FnFilter1(G.filename)));
                    }
                    else
                    {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(BUnzip)));
                    }
                }
                error = r;
            }
            break;
#endif /* def BZIP2_SUPPORT */

#ifdef LZMA_SUPPORT
        case LZMAED:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "unLZMA";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ "unLZMA", 1);
              }
            }

            if ((r = UZlzma(__G)) != 0)
            {
                if (r < PK_DISK)
                {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(UnLZMA),
                          FnFilter1(G.filename)));
                    }
                    else
                    {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(UnLZMA)));
                    }
                }
                error = r;
            }
            break;
#endif /* LZMA_SUPPORT */

#ifdef PPMD_SUPPORT
        case PPMDED:
            if (!uO.tflag)
            {
#ifdef ENABLE_USER_PROGRESS
              G.action_msg_str = "unPPMd";
#endif /* def ENABLE_USER_PROGRESS */
              if (QCOND2)
              {
                action_msg( __G__ "unPPMd", 1);
              }
            }

            if ((r = UZppmd(__G)) != 0)
            {
                if (r < PK_DISK)
                {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                    {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(UnPPMd),
                          FnFilter1(G.filename)));
                    }
                    else
                    {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(UnPPMd)));
                    }
                }
                error = r;
            }
            break;
#endif /* PPMD_SUPPORT */

        default:        /* Should never get here.  Bad unkn_cmpr_mthd()? */
            Info(slide, 0x401, ((char *)slide,
             LoadFarString(UnknownCmprMthdFile),
             FnFilter1(G.filename), REAL_COMPRESSION_METHOD));
            /* close and delete file before return? */
            undefer_input(__G);
            return PK_WARN;

    } /* end switch (compression method) */

#ifdef IZ_CRYPT_AES_WG
    if (G.lrec.compression_method == AESENCRED)
    {
        int i;

        /* Add the AES_WG Message Authorization Code (MAC) trailer size
         * back into G.csize, so that the MAC can be read, below.
         */
        undefer_input(__G);
        G.csize += g_csize_adj;
        defer_leftover_input(__G);

        /* Save the AES_WG MAC from the file data. */
        for (i = 0; i < MAC_LENGTH( G.pInfo->cmpr_mode_aes); i++)
        {
            unsigned int uichar;

            if ((uichar = NEXTBYTE) == (unsigned int)EOF)
                break;
            aes_wg_mac_file[ i] = uichar;
        }

        /* Get the calculated MAC from the encryption package. */
        i = fcrypt_end( aes_wg_mac_calc, G.zcx);
        /* Verify MAC match.  Record result. */
        aes_mac_mismatch = (i != MAC_LENGTH( G.pInfo->cmpr_mode_aes)) ||
         (memcmp( aes_wg_mac_file, aes_wg_mac_calc, i));
    }
#endif /* def IZ_CRYPT_AES_WG */

/*---------------------------------------------------------------------------
    Close the file and set its date and time (not necessarily in that order),
    and make sure the CRC checked out OK.  Logical-AND the CRC for 64-bit
    machines (redundant on 32-bit machines).
  ---------------------------------------------------------------------------*/

#ifdef VMS                  /* VMS: required even for stdout (final flush)! */
    if (!uO.tflag)          /* Don't close NULL file. */
        close_outfile(__G);
#else /* def VMS */
# ifdef DLL
    if (!uO.tflag && (!uO.cflag || G.redirect_data)) {
        if (G.redirect_data)
            FINISH_REDIRECT();
        else
            close_outfile(__G);
    }
# else
    if (!uO.tflag && !uO.cflag)   /* don't close NULL file or stdout */
        close_outfile(__G);
# endif
#endif /* def VMS [else] */

            /* GRR: CONVERT close_outfile() TO NON-VOID:  CHECK FOR ERRORS! */

    if (G.disk_full) {            /* set by flush() */
        if (G.disk_full > 1) {
#if defined(DELETE_IF_FULL) && defined(HAVE_UNLINK)
            /* delete the incomplete file if we can */
            if (unlink(G.filename) != 0)
                Trace((stderr, "extract.c:  could not delete %s\n",
                  FnFilter1(G.filename)));
#else
            /* warn user about the incomplete file */
            Info(slide, 0x421, ((char *)slide, LoadFarString(FileTruncated),
              FnFilter1(G.filename)));
#endif
            error = PK_DISK;
        } else {
            error = PK_WARN;
        }
    }

    if (error > PK_WARN) {/* don't print redundant CRC error if error already */
        undefer_input(__G);
        return error;
    }

    /* Complain about a bad CRC or an AES MAC mis-match.
     * Ignore a bad CRC for AES version 2.
     */
    if (((G.crc32val != G.lrec.crc32)
#ifdef IZ_CRYPT_AES_WG
     && (G.pInfo->cmpr_vers_aes != 2)) || aes_mac_mismatch
#else /* def IZ_CRYPT_AES_WG */
     )
#endif /* def IZ_CRYPT_AES_WG [else] */
    )
    {
        /* if quiet enough, we haven't output the filename yet:  do it */
        if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
            Info(slide, 0x401, ((char *)slide, "%-22s ",
              FnFilter1(G.filename)));

#ifdef IZ_CRYPT_AES_WG
        if (aes_mac_mismatch)
        {
            /* Bad AES Message Authentication Code.
             * It's ten bytes of bad data or a bad length.  Will anyone
             * want to see all the details?
             */
            Info(slide, 0x401, ((char *)slide, LoadFarString(BadAesMacMsg)));
        }
        else
#endif /* def IZ_CRYPT_AES_WG */
        {
            /* Bad CRC checksum. */
            Info(slide, 0x401, ((char *)slide, LoadFarString(BadCRC),
             G.crc32val, G.lrec.crc32));
        }
#ifdef IZ_CRYPT_ANY
        if (G.pInfo->encrypted)
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybeBadPasswd)));
#endif /* def IZ_CRYPT_ANY */
        error = PK_ERR;
    } else if (uO.tflag) {
#ifndef SFX
        if (G.extra_field) {
            if ((r = TestExtraField(__G__ G.extra_field,
                      (long)G.lrec.extra_field_length)) > error)
                error = r;
        } else
#endif /* ndef SFX */
        if (!uO.qflag)
            Info(slide, 0, ((char *)slide, " OK\n"));
    } else {
        if (QCOND2 && !error)   /* GRR:  is stdout reset to text mode yet? */
            Info(slide, 0, ((char *)slide, "\n"));
    }

    undefer_input(__G);         /* Re-adjust input counts, pointers. */

#ifdef ENABLE_USER_PROGRESS
    G.action_msg_str = NULL;            /* Clear the progress mthd str. */
#endif /* def ENABLE_USER_PROGRESS */
    return error;

} /* extract_or_test_member(). */


/***********************************/
/*  Function mapname_dir_vollab()  */
/***********************************/
static int mapname_dir_vollab( __G__ renamed,
#ifdef SET_DIR_ATTRIB
                                     pnum_dirs, pdir_list,
#endif
                                     perr_in_arch)
  __GDEF
  int renamed;
#ifdef SET_DIR_ATTRIB
  unsigned *pnum_dirs;
  direntry **pdir_list;
#endif
  int *perr_in_arch;
{
  int error;
  int errcode;
  int mapname_attrs;
  int done = 0;         /* Return 0 if more to do, 1 if done. */

  mapname_attrs = mapname(__G__ renamed);

  /* Separate error code and attribute field. */
  errcode = mapname_attrs & ~MPN_MASK;
  mapname_attrs &= MPN_MASK;

#if defined( UNIX) && defined( __APPLE__)
  /* If the destination volume attributes are still a mystery,
   * and mapname() admits that it made any destination directories,
   * then try again to determine the volume attributes.
   * We're hoping that a normal file precedes any AppleDouble
   * files, so that the flag gets set before it's too late.
   * We're also ignoring the possibility that a user rename
   * has sent us onto a volume with different attributes.
   * Otherwise, we'd need to do more complex, rename-aware
   * volume attribute determination.
   */
  if ((G.exdir_attr_ok < 0) && (mapname_attrs == MPN_CREATED_DIR))
  {
    G.exdir_attr_ok = vol_attr_ok( uO.exdir);
  }
#endif /* defined( UNIX) && defined( __APPLE__) */

  /* Set error_in_archive by mapname() result. */
  if ((errcode != PK_OK) && (*perr_in_arch < errcode))
  {
    *perr_in_arch = errcode;
  }

  if (mapname_attrs > MPN_INF_TRUNC)
  {
    /* Some special handling needed. */
    if (mapname_attrs == MPN_CREATED_DIR)
    {
      /* Directory (created).  Save attributes for later use, if supported. */
#ifdef SET_DIR_ATTRIB
      direntry *d_entry;

      error = defer_dir_attribs(__G__ &d_entry);
      if (d_entry == (direntry *)NULL)
      {
        /* There may be no dir_attribs info available, or
         * we have encountered a mem allocation error.
         * In case of an error, report it and set program
         * error state to warning level.
         */
         if (error)
         {
           Info(slide, 0x401, ((char *)slide,
            LoadFarString(DirlistEntryNoMem)));
           if (*perr_in_arch == PK_OK)
             *perr_in_arch = PK_WARN;
         }
      }
      else
      {
        /* Add this directory to the directories-to-be-processed list. */
        d_entry->next = (*pdir_list);
        (*pdir_list) = d_entry;
        ++(*pnum_dirs);
      }
#endif /* SET_DIR_ATTRIB */
    }
    else if (mapname_attrs == MPN_VOL_LABEL)
    {
      /* Volume label. */
#ifdef DOS_OS2_W32
      Info(slide, 0x401, ((char *)slide,
       LoadFarString(SkipVolumeLabel),
       FnFilter1(G.filename),
       uO.volflag? "hard disk " : ""));
#else
      Info(slide, 1, ((char *)slide,
       LoadFarString(SkipVolumeLabel),
       FnFilter1(G.filename), ""));
#endif
    }
    else if ((mapname_attrs > MPN_INF_SKIP) && (*perr_in_arch < PK_ERR))
    {
      /* Not dir or vol-lab, so set error_in_archive. */
      *perr_in_arch = PK_ERR;
    }
    Trace((stderr, "mapname(%s) returns error code = %d\n",
     FnFilter1(G.filename), error));

    done = 1;   /* Go on to the next member. */
  }

  return done;
} /* mapname_dir_vollab(). */


#if (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE))

/************************************/
/*  Function mapname_dir_vollabw()  */
/************************************/
static int mapname_dir_vollabw( __G__ renamed,
# ifdef SET_DIR_ATTRIB
                                      pnum_dirs, pdir_listw,
# endif
                                      perr_in_arch)
  __GDEF
  int renamed;
# ifdef SET_DIR_ATTRIB
  unsigned *pnum_dirs;
  direntryw **pdir_listw;
# endif
  int *perr_in_arch;
{
  int error;
  int errcode;
  int mapname_attrs;
  int done = 0;         /* Return 0 if more to do, 1 if done. */

  mapname_attrs = mapnamew(__G__ renamed);

  /* Separate error code and attribute field. */
  errcode = mapname_attrs & ~MPN_MASK;
  mapname_attrs &= MPN_MASK;

  /* Set error_in_archive by mapnamew() result. */
  if ((errcode != PK_OK) && (*perr_in_arch < errcode))
  {
    *perr_in_arch = errcode;
  }

  if (mapname_attrs > MPN_INF_TRUNC)
  {
    /* Some special handling needed. */
    if (mapname_attrs == MPN_CREATED_DIR)
    {
      /* Directory (created).  Save attributes for later use, if supported. */
# ifdef SET_DIR_ATTRIB
      direntryw *d_entryw;

      error = defer_dir_attribsw(__G__ &d_entryw);
      if (d_entryw == (direntryw *)NULL)
      {
        /* There may be no dir_attribs info available, or
         * we have encountered a mem allocation error.
         * In case of an error, report it and set program
         * error state to warning level.
         */
         if (error)
         {
           Info(slide, 0x401, ((char *)slide,
            LoadFarString(DirlistEntryNoMem)));
           if (*perr_in_arch == PK_OK)
             *perr_in_arch = PK_WARN;
         }
      }
      else
      {
        /* Add this directory to the directories-to-be-processed list. */
        d_entryw->next = (*pdir_listw);
        (*pdir_listw) = d_entryw;
        ++(*pnum_dirs);
      }
# endif /* SET_DIR_ATTRIB */
    }
    else if (mapname_attrs == MPN_VOL_LABEL)
    {
      /* Volume label. */
# ifdef DOS_OS2_W32
      Info(slide, 0x401, ((char *)slide,
       LoadFarString(SkipVolumeLabel),
       FnFilter1(G.filename),
       uO.volflag? "hard disk " : ""));
# else
      Info(slide, 1, ((char *)slide,
       LoadFarString(SkipVolumeLabel),
       FnFilter1(G.filename), ""));
# endif
    }
    else if ((mapname_attrs > MPN_INF_SKIP) && (*perr_in_arch < PK_ERR))
    {
      /* Not dir or vol-lab, so set error_in_archive. */
      *perr_in_arch = PK_ERR;
    }
    Trace((stderr, "mapnamew(%s) returns error code = %d\n",
     FnFilter1(G.filename), error));

    done = 1;   /* Go on to the next member. */
  }

  return done;
} /* mapname_dir_vollabw(). */

#endif /* (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)) */


/**********************************/
/*  Function conflict_query_qr()  */
/**********************************/
static int conflict_query_qr( __G__ prenamed,
                                    perr_in_arch)
  __GDEF
  int *prenamed;
  int *perr_in_arch;
{
  int skip_entry = 0;           /* Return the skip_entry value:
                                 * <0: Start over.
                                 *  0: Do this member.
                                 * >0: Skip this member.
                                 */

  /* Query the user as to how to resolve the conflict. */

#ifdef WINDLL
  /* Use the DLL user-supplied query function. */
  switch (G.lpUserFunctions->replace != NULL ?
          (*G.lpUserFunctions->replace)(G.filename, FILNAMSIZ) :
          IDM_REPLACE_NONE)
  {
    case IDM_REPLACE_RENAME:
      _ISO_INTERN(G.filename);
      *prenamed = TRUE;
      goto start_over;
    case IDM_REPLACE_ALL:
      G.overwrite_mode = OVERWRT_ALWAYS;
      /* FALL THROUGH, extract */
    case IDM_REPLACE_YES:
      break;
    case IDM_REPLACE_NONE:
      G.overwrite_mode = OVERWRT_NEVER;
      /* FALL THROUGH, skip */
    case IDM_REPLACE_NO:
      skip_entry = SKIP_Y_EXISTING;
      break;
  } /* switch */
#else /* def WINDLL */
  /* Use our own query function. */
  extent fnlen;

  /* Prompt the user for a new (non-conflicting) path/name.
   * 2015-03-10 SMS.
   * Changed to use the name in G.unipath_widefilename when it's active,
   * instead of always using G.filename.
   */
reprompt:

# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
  if (G.has_win32_wide)
  {
    /* "replace/rename" query prompt showing the wide name. */
    Info(slide, 0x81, ((char *)slide,
     LoadFarString(ReplaceQueryw), FnFilterW1( G.unipath_widefilename)));
  }
  else
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
  {
    /* "replace/rename" query prompt showing the normal name. */
    Info(slide, 0x81, ((char *)slide,
     LoadFarString(ReplaceQuery), FnFilter1(G.filename)));
  }

  if (fgets_ans( __G) < 0)
  {
    Info(slide, 1, ((char *)slide, LoadFarString(AssumeNone)));
    *G.answerbuf = 'N';
    if (*perr_in_arch == PK_OK)
      *perr_in_arch = 1;    /* not extracted:  warning */
  }
  switch (*G.answerbuf)
  {
    case 'r':
    case 'R':
      do
      {
        Info(slide, 0x81, ((char *)slide, LoadFarString(NewNameQuery)));
        if (fgets( G.filename, FILNAMSIZ, G.query_fp) == NULL)
        { /* read() error.  Try again. */
          goto reprompt;
        }
        else
        {
          /* Usually get \n here.  Check for it. */
          fnlen = strlen(G.filename);
          if (lastchar(G.filename, fnlen) == '\n')
            G.filename[--fnlen] = '\0';
        }
      } while (fnlen == 0);
      *prenamed = TRUE;         /* Record fact of user-supplied name. */

      /* Make any code-page and/or Unicode adjustments/copies of the
       * new, user-supplied name.
       * 2015-03-10 SMS.
       * There's no reason to believe that the following code handles
       * Unicode (or anything else) correctly, but it may at least store
       * something in G.unipath_widefilename for a wide Unicode name,
       * which seems more reasonable than expecting G.filename to do the
       * whole job, which is what was done before.
       * Potential sources of complication include how, exactly, the
       * "rename" input is read, which translations or transformations
       * need to be done, where the results should be stored, and which
       * previously used name storage should be cleared and/or free()'d.
       */
# ifdef WIN32   /* WIN32 fgets( ... , stdin) returns OEM coded strings */
      _OEM_INTERN(G.filename);
# endif

# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
      /* Free any old G.unipath_filename storage. */
      if (G.unipath_filename != NULL)
      {
        izu_free( G.unipath_filename);
        G.unipath_filename = NULL;
      }
#  ifdef DYNAMIC_WIDE_NAME
      /* Free any old G.unipath_widefilename storage. */
      if (G.unipath_widefilename != NULL)
      {
        izu_free( G.unipath_widefilename);
        G.unipath_widefilename = NULL;
      }

      /* 2015-03-10 SMS.
       * Should G.has_win32_wide be re-determined according to the
       * new, user-supplied path (which may point to a different
       * device/drive)?
       */
      if (G.has_win32_wide)
      {
        /* Get wide path from G.filename. */
        G.unipath_widefilename = utf8_to_wchar_string(G.filename);
      }
#  else /* def DYNAMIC_WIDE_NAME */
      if (G.has_win32_wide)
      {
        /* Get wide path from G.filename. */
        utf8_to_wchar_string( G.unipath_widefilename, G.filename);
      }
#  endif /* def DYNAMIC_WIDE_NAME [else] */
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */

      goto start_over;  /* Go retry with the new, user-supplied name. */

    case 'A':   /* Dangerous option.  Demand upper-case response. */
      G.overwrite_mode = OVERWRT_ALWAYS;
      /* FALL THROUGH, extract */
    case 'y':
    case 'Y':
      break;
    case 'N':   /* Dangerous (slightly) option.  Demand upper-case response. */
      G.overwrite_mode = OVERWRT_NEVER;
      /* FALL THROUGH, skip */
    case 'n':
      /* skip file */
      skip_entry = SKIP_Y_EXISTING;
      break;
    case '\n':
    case '\r':
      /* Improve echo of '\n' and/or '\r'.
       * (sizeof(G.answerbuf) == 10 (see globals.h), so
       * there is enough space for the provided text...)
       */
      strcpy(G.answerbuf, "{ENTER}");
      /* fall through ... */
    default:
      /* Usually get \n here.  Remove it for nice display.
       * (fnlen can be re-used here, we are outside the
       * "enter new filename" loop)
       */
      fnlen = strlen(G.answerbuf);
      if (lastchar(G.answerbuf, fnlen) == '\n')
        G.answerbuf[--fnlen] = '\0';
      Info(slide, 1, ((char *)slide,
       LoadFarString(InvalidResponse), G.answerbuf));
      goto reprompt;    /* yet another goto? */
  } /* switch (*G.answerbuf) */
#endif /* def WINDLL [else] */
  goto normal_exit;     /* At this point, who carse about another goto? */
			
start_over:
  skip_entry = -1;

normal_exit:
  return skip_entry;
} /* conflict_query_qr(). */


/*******************************/
/*  Function conflict_query()  */
/*******************************/
static int conflict_query( __G__ prenamed,
#ifdef SET_DIR_ATTRIB
                                 pnum_dirs, pdir_list,
#endif
                                 perr_in_arch)
  __GDEF
  int *prenamed;
#ifdef SET_DIR_ATTRIB
  unsigned *pnum_dirs;
  direntry **pdir_list;
#endif
  int *perr_in_arch;
{
  int error = PK_OK;                    /* Return PK-type error code. */
  int query;
  int skip_entry;
#if defined( UNIX) && defined( __APPLE__)
  int cfn;
#endif /* defined( UNIX) && defined( __APPLE__) */

  /*
   * Almost ready to extract member.  If extracting to a file, check if
   * file already exists, and if so, take appropriate action according
   * to fflag/uflag/overwrite_all/etc.  (We couldn't do earlier, because
   * we don't store the possibly renamed filename[] in info[].)
   * Test mode (-t, uO.tflag) and stdout (-c, uO.cflag) create no files,
   * so are always safe.
   */
#ifdef DLL
  if (!uO.tflag && !uO.cflag && !G.redirect_data)
#else
  if (!uO.tflag && !uO.cflag)
#endif
  {
startover:
    query = FALSE;              /* Query user about conflict resolution. */
    skip_entry = SKIP_NO;       /* Not (yet) decided to skip this member. */

    /* For files from DOS FAT, check for use of backslash instead
     * of slash as directory separator.  (Bug in some zipper(s).  So
     * far, not a problem in HPFS, NTFS or VFAT systems.)  Not done for
     * SFX, because we assume that our Zip doesn't do it.
     */
#ifndef SFX
    error = backslash_slash( __G);
    if (error != PK_OK)
    {
      if (*perr_in_arch == PK_OK)
        *perr_in_arch = error;
    }
#endif /* ndef SFX */

    if (!*prenamed)
    {
      /* Transform absolute path to relative path, and warn user. */
      error = name_abs_rel( __G);
      if (error != PK_OK)
        *perr_in_arch = error;
    }

    /* Junk (full or partial) path info (-j[=N]). */
    name_junk( __G);

    /* mapname() can create dirs if not freshening or if renamed. */
    error = mapname_dir_vollab( __G__ *prenamed,
#ifdef SET_DIR_ATTRIB
     pnum_dirs, pdir_list,
#endif
     perr_in_arch);

    if (error != PK_OK)
      goto exit;        /* No more to do with this one.  Go to next. */

#ifdef QDOS
    QFilename(__G__ G.filename);
#endif

#if defined( UNIX) && defined( __APPLE__)
    /* If we are doing special AppleDouble file processing,
     * and this is an AppleDouble file,
     * then we should ignore a file-exists test, which may be
     * expected to succeed.
     */
    if (G.apple_double && (!uO.J_flag))
    {
      /* Fake a does-not-exist value for this AppleDouble file. */
      cfn = DOES_NOT_EXIST;
    }
    else
    {
      /* Do the real test. */
      cfn = check_for_newer(__G__ G.filename);
    }
# define CHECK_FOR_NEWER cfn
#else
# define CHECK_FOR_NEWER check_for_newer(__G__ G.filename)
#endif

    /* Determine whether to query the user, or, if we have adequate
     * info/instructions, how to handle the conflict automatically.
     * Use "cfn" on Mac, plain check_for_newer() elsewhere.
     */
    switch (CHECK_FOR_NEWER)
    {
      case DOES_NOT_EXIST:
#ifdef NOVELL_BUG_FAILSAFE
        G.dne = TRUE;   /* stat() says file DOES NOT EXIST */
#endif
        /* If Freshen (no new files), then skip unless just renamed. */
        if (uO.fflag && !*prenamed)
          skip_entry = SKIP_Y_NONEXIST;
        break;

      case EXISTS_AND_OLDER:
#ifdef UNIXBACKUP
        if (!uO.B_flag)
#endif
        {
          if (IS_OVERWRT_NONE)
          {
            /* Never overwrite, so always skip existing. */
            skip_entry = SKIP_Y_EXISTING;
          }
          else if (!IS_OVERWRT_ALL)
          {
            /* Not overwrite-all, so query the user. */
            query = TRUE;
          }
        }
        break;

      case EXISTS_AND_NEWER:             /* (or equal) */
#ifdef UNIXBACKUP
        if ((!uO.B_flag && IS_OVERWRT_NONE) ||
#else
        if (IS_OVERWRT_NONE ||
#endif
         (uO.uflag && !*prenamed))
        {
          /* Freshen or Update, and not renamed, so skip existing. */
          skip_entry = SKIP_Y_EXISTING;
        }
        else
        {
#ifdef UNIXBACKUP
          if (!IS_OVERWRT_ALL && !uO.B_flag)
#else
          if (!IS_OVERWRT_ALL)
#endif
            /* Not overwrite-all, so query the user. */
            query = TRUE;
        }
        break;
    } /* switch */

#ifdef VMS
    /* 2008-07-24 SMS.
     * On VMS, if the file name includes a version number,
     * and "-V" ("retain VMS version numbers", V_flag) is in
     * effect, then the VMS-specific code will handle any
     * conflicts with an existing file, making this query
     * redundant.  (Implicit "y" response here.)
     */
    if (query && (uO.V_flag > 0))
    {
      /* Not discarding file versions.  Look for one. */
      int cndx = strlen(G.filename) - 1;

      while ((cndx > 0) && (isdigit(G.filename[cndx])))
        cndx--;
      if (G.filename[cndx] == ';')
        /* File version found; skip the generic query,
         * proceeding with its default response "y".
         */
        query = FALSE;
    }
#endif /* VMS */

    /* Query the user to resolve conflict? */
    if (query)
    {
      int cqq;
      cqq = conflict_query_qr( __G__ prenamed, perr_in_arch);
      if (cqq < 0)
      {
        goto startover;
      }
      else if (cqq != 0)
      {
        skip_entry = cqq;       /* Probably SKIP_Y_EXISTING? */
      }
    } /* if (query) */

    if (skip_entry != SKIP_NO)
    {
#ifdef WINDLL
      if (skip_entry == SKIP_Y_EXISTING)
      {
        /* Report skipping of an existing entry. */
        Info(slide, 0, ((char *)slide,
         ((IS_OVERWRT_NONE || !uO.uflag || *prenamed) ?
         "Target file exists.\nSkipping %s\n" :
         "Target file newer.\nSkipping %s\n"),
         FnFilter1(G.filename)));
      }
#endif /* def WINDLL */
      error = PK_ERR;   /* If skipping, report error. */
      /* goto exit; */
    }
  }

exit:

  return error;
} /* conflict_query(). */


#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)

/********************************/
/*  Function conflict_queryw()  */
/********************************/
static int conflict_queryw( __G__ prenamed,
#ifdef SET_DIR_ATTRIB
                                  pnum_dirs, pdir_list,
#endif
                                  perr_in_arch)
  __GDEF
  int *prenamed;
#ifdef SET_DIR_ATTRIB
  unsigned *pnum_dirs;
  direntry **pdir_list;
#endif
  int *perr_in_arch;
{
  int error = PK_OK;                    /* Return PK-type error code. */
  int query;
  int skip_entry;

  /*
   * Almost ready to extract member.  If extracting to a file, check if
   * file already exists, and if so, take appropriate action according
   * to fflag/uflag/overwrite_all/etc.  (We couldn't do earlier, because
   * we don't store the possibly renamed filename[] in info[].)
   * Test mode (-t, uO.tflag) and stdout (-c, uO.cflag) create no files,
   * so are always safe.
   */
#ifdef DLL
  if (!uO.tflag && !uO.cflag && !G.redirect_data)
#else
  if (!uO.tflag && !uO.cflag)
#endif
  {
startover:
    query = FALSE;              /* Query user about conflict resolution. */
    skip_entry = SKIP_NO;       /* Not (yet) decided to skip this member. */

    /* For files from DOS FAT, check for use of backslash instead
     * of slash as directory separator.  (Bug in some zipper(s).  So
     * far, not a problem in HPFS, NTFS or VFAT systems.)  Not done for
     * SFX, because we assume that our Zip doesn't do it.
     */
#ifndef SFX
    error = backslash_slash( __G);
    if (error != PK_OK)
    {
      if (*perr_in_arch == PK_OK)
        *perr_in_arch = error;
    }
#endif /* ndef SFX */

    if (!*prenamed)
    {
      /* Transform absolute path to relative path, and warn user. */
      error = name_abs_rel( __G);
      if (error != PK_OK)
        *perr_in_arch = error;
    }

    /* Junk (full or partial) path info (-j[=N]). */
    name_junkw( __G);

    /* mapnamew() can create dirs if not freshening or if renamed. */
    error = mapname_dir_vollabw( __G__ *prenamed,
#ifdef SET_DIR_ATTRIB
     pnum_dirs, pdir_list,
#endif
     perr_in_arch);

    if (error != PK_OK)
      goto exit;        /* No more to do with this one.  Go to next. */

    /* Determine whether to query the user, or, if we have adequate
     * info/instructions, how to handle the conflict automatically.
     */
    switch (check_for_newerw(__G__ G.unipath_widefilename))
    {
      case DOES_NOT_EXIST:
#ifdef NOVELL_BUG_FAILSAFE
        G.dne = TRUE;   /* stat() says file DOES NOT EXIST */
#endif
        /* If Freshen (no new files), then skip unless just renamed. */
        if (uO.fflag && !*prenamed)
          skip_entry = SKIP_Y_NONEXIST;
        break;

      case EXISTS_AND_OLDER:
#ifdef UNIXBACKUP
        if (!uO.B_flag)
#endif
        {
          if (IS_OVERWRT_NONE)
          {
            /* Never overwrite, so always skip existing. */
            skip_entry = SKIP_Y_EXISTING;
          }
          else if (!IS_OVERWRT_ALL)
          {
            /* Not overwrite-all, so query the user. */
            query = TRUE;
          }
        }
        break;

      case EXISTS_AND_NEWER:             /* (or equal) */
#ifdef UNIXBACKUP
        if ((!uO.B_flag && IS_OVERWRT_NONE) ||
#else
        if (IS_OVERWRT_NONE ||
#endif
         (uO.uflag && !*prenamed))
        {
          /* Freshen or Update, and not renamed, so skip existing. */
          skip_entry = SKIP_Y_EXISTING;
        }
        else
        {
#ifdef UNIXBACKUP
          if (!IS_OVERWRT_ALL && !uO.B_flag)
#else
          if (!IS_OVERWRT_ALL)
#endif
            /* Not overwrite-all, so query the user. */
            query = TRUE;
        }
        break;
    } /* switch */

    /* Query the user to resolve conflict? */
    if (query)
    {
      int cqq;
      cqq = conflict_query_qr( __G__ prenamed, perr_in_arch);
      if (cqq < 0)
      {
        goto startover;
      }
      else if (cqq != 0)
      {
        skip_entry = cqq;       /* Probably SKIP_Y_EXISTING? */
      }
    } /* if (query) */

    if (skip_entry != SKIP_NO)
    {
#ifdef WINDLL
      if (skip_entry == SKIP_Y_EXISTING)
      {
        /* Report skipping of an existing entry. */
        Info(slide, 0, ((char *)slide,
         ((IS_OVERWRT_NONE || !uO.uflag || *prenamed) ?
         "Target file exists.\nSkipping %s\n" :
         "Target file newer.\nSkipping %s\n"),
         FnFilter1(G.filename)));
      }
#endif /* def WINDLL */
      error = PK_ERR;   /* If skipping, report error. */
      /* goto exit; */
    }
  }

exit:

  return error;
} /* conflict_queryw(). */

#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */


#ifdef SET_DIR_ATTRIB

/********************************/
/*  Function set_dir_attribs()  */
/********************************/

static int set_dir_attribs( __G__ num_dirs,
                                  dir_list,
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
                                  dir_listw,
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
                                  perr_in_arch)
  __GDEF
  int num_dirs;
  direntry *dir_list;
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
  direntryw *dir_listw;
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
  int *perr_in_arch;
{
  int error = PK_OK;                    /* Return PK-type error code. */
  int i;
  direntry **sort_dir_list;
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
  direntryw **sort_dir_listw;
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */

/*---------------------------------------------------------------------------
    Go back through saved list of directories, sort and set times/perms/UIDs
    and GIDs from the deepest level on up.
  ---------------------------------------------------------------------------*/

# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
  if (G.has_win32_wide)
  {
    sort_dir_listw = (direntryw **)izu_malloc(num_dirs* sizeof(direntryw *));
    if (sort_dir_listw == (direntryw **)NULL)
    {
      /* Memory allocation failed. */
      Info(slide, 0x401, ((char *)slide, LoadFarString(DirlistSortNoMem)));

      /* Free (unsortable) unsorted dir list. */
      while (dir_listw != (direntryw *)NULL)
      {
        direntryw *dw = dir_listw;      /* List head. */

        dir_listw = dir_listw->next;    /* Next. */
        izu_free( dw);                  /* Free storage. */
      }
    }
    else
    {
      ulg ndirs_fail = 0;

      if (num_dirs == 1)
      {
        /* No need to sort a list of length 1. */
        sort_dir_listw[0] = dir_listw;
      }
      else
      {
        /* Copy the unsorted list to the sort array. */
        for (i = 0;  i < num_dirs;  ++i)
        {
          sort_dir_listw[i] = dir_listw;
          dir_listw = dir_listw->next;
        }
        /* Sort the sort array. */
        qsort((char *)sort_dir_listw, num_dirs, sizeof(direntryw *), dircompw);
      }

      Trace((stderr, "setting directory times/perms/attributes\n"));

      /* Loop through the sort array, setting dir attributes for each. */
      for (i = 0;  i < num_dirs;  ++i)
      {
        direntryw *dw = sort_dir_listw[i];

        Trace((stderr, "dir = %s\n", dw->fnw));
        if ((error = set_direc_attribsw(__G__ dw)) != PK_OK)
        {
          ndirs_fail++;
          Info(slide, 0x201, ((char *)slide,
           LoadFarString(DirlistSetAttrFailed), dw->fnw));
          if (*perr_in_arch == PK_OK)
            *perr_in_arch = error;
        }
        izu_free( dw);
      }
      izu_free(sort_dir_listw);
      if (!uO.tflag && QCOND2)
      {
        if (ndirs_fail > 0)
        {
          Info(slide, 0, ((char *)slide,
           LoadFarString(DirlistFailAttrSum), ndirs_fail));
        }
      }
    }
  }
  else
# endif /* (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)) */
  {
    sort_dir_list = (direntry **)izu_malloc(num_dirs* sizeof(direntry *));
    if (sort_dir_list == (direntry **)NULL)
    {
      /* Memory allocation failed. */
      Info(slide, 0x401, ((char *)slide, LoadFarString(DirlistSortNoMem)));

      /* Free (unsortable) unsorted dir list. */
      while (dir_list != (direntry *)NULL)
      {
        direntry *d = dir_list;         /* List head. */

        dir_list = dir_list->next;      /* Next. */
        izu_free( d);                   /* Free storage. */
      }
    }
    else
    {
      ulg ndirs_fail = 0;

      if (num_dirs == 1)
      {
        /* No need to sort a list of length 1. */
        sort_dir_list[0] = dir_list;
      }
      else
      {
        /* Copy the unsorted list to the sort array. */
        for (i = 0;  i < num_dirs;  ++i)
        {
          sort_dir_list[i] = dir_list;
          dir_list = dir_list->next;
        }
        /* Sort the sort array. */
        qsort((char *)sort_dir_list, num_dirs, sizeof(direntry *), dircomp);
      }

      Trace((stderr, "setting directory times/perms/attributes\n"));

      /* Loop through the sort array, setting dir attributes for each. */
      for (i = 0;  i < num_dirs;  ++i)
      {
        direntry *d = sort_dir_list[i];

        Trace((stderr, "dir = %s\n", d->fn));
        if ((error = set_direc_attribs(__G__ d)) != PK_OK)
        {
          ndirs_fail++;
          Info(slide, 0x201, ((char *)slide,
           LoadFarString(DirlistSetAttrFailed), d->fn));
          if (*perr_in_arch == PK_OK)
            *perr_in_arch = error;
        }
        izu_free( d);
      }
      izu_free( sort_dir_list);
      if (!uO.tflag && QCOND2)
      {
        if (ndirs_fail > 0)
        {
          Info(slide, 0, ((char *)slide,
           LoadFarString(DirlistFailAttrSum), ndirs_fail));
        }
      }
    }
  }

  return error;
} /* set_dir_attribs(). */

#endif /* def SET_DIR_ATTRIB */


#ifdef ARCHIVE_STDIN

/***************************************/
/*  Function extract_or_test_stream()  */
/***************************************/

int extract_or_test_stream( __G)        /* Return PK-type error code. */
    __GDEF
{
  int do_this_file;
  int error;
  int error_in_archive = PK_OK;
  zucn_t members_processed;
  ulg filnum = 0L;
  ulg num_skipped = 0L;
  ulg num_bad_pwd = 0L;
  int renamed;
  int byte;
  int state;
  int data_descr_expected;

#ifdef IZ_CRYPT_AES_WG
    ush temp_compression_method;
    int temp_stored_size_decr;
#endif /* def IZ_CRYPT_AES_WG) */

# ifdef SET_DIR_ATTRIB
  unsigned num_dirs = 0;
  direntry *dir_list = (direntry *)NULL;
#  if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
  direntryw *dir_listw = (direntryw *)NULL;
#  endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
# endif /* def SET_DIR_ATTRIB */

#ifdef IZ_CRYPT_ANY
  G.newzip = TRUE;
#endif
#ifndef SFX
  G.reported_backslash = FALSE;
#endif

# if defined( UNIX) && defined( __APPLE__)
  G.do_this_prev = 0;
  G.seeking_apl_dbl = 0;
# endif /* defined( UNIX) && defined( __APPLE__) */

  members_processed = 0;

  /* Initialize the CRC table pointer (once). */
  if (CRC_32_TAB == NULL)
  {
    if ((CRC_32_TAB = get_crc_table()) == NULL)
    {
      return PK_MEM;
    }
  }

# if !defined(SFX) || defined(SFX_EXDIR)
  /* Verify/create extraction destination directory. */
  error = extract_dest_dir( __G);
  if (error != PK_OK)
    return error;
# endif /* !defined(SFX) || defined(SFX_EXDIR) */

  /* Allocate space for checking unmatched list/-x list file specs. */
  error = allocate_name_match_arrays( __G);
  if (error != PK_OK)
    return error;

  /* Fake a compressed size (bigger than SFX) to get readbyte() started.
   * (2015-02-04 SMS.  Note [A].
   * Sadly, fileio.c:defer_leftover_input() couples the real buffer
   * byte count, G.incnt, with the not-yet-known archive member
   * compressed data size, G.csize, so, until a real G.csize is
   * determined, we need to use a fake G.csize which is large enough to
   * reach the data for the first archive member.  Or rewrite the whole
   * file-read scheme.)
   */
  G.csize = INBUFSIZ* 128;      /* 1Mi, for INBUFSIZ = 8Ki. */
  defer_leftover_input(__G);    /* So NEXTBYTE bounds check will work. */

  while (1)
  {
    /* Fake a compressed size to get readbyte() started.  See Note [A],
     * above.  We could return here with little or no G.incnt data.
     * (INBUFSIZ >= G.incnt, to avoid confusing NEXTBYTE+readbyte().)
     */
    if (G.csize < INBUFSIZ)
      G.csize = INBUFSIZ;

    /* Skip to local file header (or central directory header) signature. */
    state = 0;                  /* Seeking signature byte 0. */
    data_descr_expected = 0;    /* Set later, according to header flag bit. */

    while ((byte = NEXTBYTE) != EOF)
    {
      if (state == 0)
      {
        if (byte == local_hdr_sig[ 0])          /* "P". */
        {
          state = 1;    /* Match.  Now seeking signature byte 1. */
        }
        else
        {
          state = 0;    /* Mismatch.  Now seeking signature byte 0. */
        }
      }
      else if (state == 1)
      {
        if (byte == local_hdr_sig[ 1])          /* "K". */
        {
          state = 2;    /* Match.  Now seeking signature byte 2. */
        }
        else
        {
          state = 0;    /* Mismatch.  Now seeking signature byte 0. */
        }
      }
      else if (state == 2)
      {
        if (byte == local_hdr_sig[ 2])          /* 0x03. */
        {
          state = 3;    /* Match.  Now seeking local signature byte 3. */
        }
        else if (byte == central_hdr_sig[ 2])   /* 0x01. */
        {
          state = 5;    /* Match.  Now seeking central signature byte 3. */
        }
        else
        {
          state = 0;    /* Mismatch.  Now seeking signature byte 0. */
        }
      }
      else if (state == 3)
      {
        if (byte == local_hdr_sig[ 3])          /* 0x04. */
        {
          state = 4;    /* Match.  Found local signature. */
          break;
        }
        else
        {
          state = 0;    /* Mismatch.  Now seeking signature byte 0. */
        }
      }
      else if (state == 5)
      {
        if (byte == central_hdr_sig[ 3])        /* 0x02. */
        {
          state = 6;    /* Match.  Found central signature. */
          break;
        }
        else
        {
          state = 0;    /* Mismatch.  Now seeking signature byte 0. */
        }
      }
    } /* while ((byte = NEXTBYTE) != EOF) */

    if (byte == EOF)
    {
      error = PK_EOF;
      break;
    }

    if (state == 4)
    {
      /* Read the local header (through the EF length). */
      error = process_local_file_hdr( __G);

      if (error != PK_OK)
      {
        Info(slide, 0x421, ((char *)slide, LoadFarString(BadLocalHdr), 0L));
        error_in_archive = error;       /* Only PK_EOF defined. */
      }
      else
      {
        /* Fake some G.crec.general_purpose_bit_flag from local file header. */
        G.crec.general_purpose_bit_flag = G.lrec.general_purpose_bit_flag;
        G.crec.crc32 = G.lrec.crc32;
        G.crec.csize = G.lrec.csize;
        G.crec.ucsize = G.lrec.ucsize;

        /* Read the member name. */
        error = do_string(__G__ G.lrec.filename_length, DS_FN_L);

        if (error > error_in_archive)
          error_in_archive = error;

        if (error > PK_WARN)
        {
          Info(slide, 0x401, ((char *)slide, LoadFarString(FilNamMsg),
           FnFilter1(G.filename), "local"));
        }
      } /* if (error != PK_OK) [else] */

      if (error == PK_OK)
      {
        /* Read the extra field.  (Position left at start of compr data.)*/
        error = do_string(__G__ G.lrec.extra_field_length, EXTRA_FIELD);

        if (error > error_in_archive)
          error_in_archive = error;

        if (error > PK_WARN)
        {
          Info(slide, 0x401, ((char *)slide, LoadFarString(ExtFieldMsg),
           FnFilter1(G.filename), "local"));
        }
      } /* if (error == PK_OK) */

#ifdef USE_EF_STREAM
      if (error == PK_OK)
      {
        uch bitmap[ EF_STREAM_BM_SIZE];
        ext_local_file_hdr xlhdr;
        char *cmnt;
        int btmp_siz = EF_STREAM_BM_SIZE;
        int sts;

        sts = ef_scan_for_stream( G.extra_field,
                                  (long)G.lrec.extra_field_length,
                                  &btmp_siz,
                                  &bitmap[ 0],
                                  &xlhdr,
                                  &cmnt);
        if (sts == 0)
        {
          /* Fill in some G.pInfo fields (and fake some G.crec fields)
           * from extended local header data.
           */
          if (btmp_siz > 0)                     /* Bitmap byte 0 available. */
          {
            if (bitmap[ 0]& (1 << 0))           /* Version made by. */
            {
              G.pInfo->hostver = xlhdr.version_made_by[ 0];
              G.pInfo->hostnum = IZ_MIN( xlhdr.version_made_by[ 1], NUM_HOSTS);
            }

            if (bitmap[ 0]& (1 << 1))           /* Internal file attributes. */
            {
              G.crec.internal_file_attributes = xlhdr.internal_file_attributes;
            }

            if (bitmap[ 0]& (1 << 2))           /* External file attributes. */
            {
              G.crec.external_file_attributes = xlhdr.external_file_attributes;
            }

#ifdef FUTURE
            /* We currently do nothing with a member comment which is
             * not in the central directory.
             */
            if (bitmap[ 0]& (1 << 3))           /* File comment. */
            {
              length: xlhdr->file_comment_length
              string: cmnt
            }
#endif /* def FUTURE */

          }

          /* Fill more G.pInfo structure (from G.crec). */
          error = process_file_hdr( __G);

        } /* if (sts == 0) */
      } /* if (error == PK_OK) */

#endif /* def USE_EF_STREAM */

      if (error == PK_OK)
      {

        /* No filename consistency checks possible without the central
         * directory.  We could do something with a file comment from an
         * extended local file header.
         */

        /* Set the Java CAFE flag by the first extra field found. */
        if (uO.java_cafe == 0)
        {
          ef_scan_for_cafe( __G__ G.extra_field,
           (long)G.lrec.extra_field_length);
        }
      } /* if (error == PK_OK) */

      if (error == PK_OK)
      {
        /* Decide whether to process this archive member.
         * match_include_exclude() handles name-based
         * inclusion/exclusion of AppleDouble files.
         */
        do_this_file = match_include_exclude( __G);
        if (do_this_file <= 0)
        {
          error = PK_ERR;

# if defined( UNIX) && defined( __APPLE__)
          G.do_this_prev = do_this_file;
          G.seeking_apl_dbl = 0;
# endif /* defined( UNIX) && defined( __APPLE__) */

          /* Waste the remainder of this member. */
          if (G.csize > 0)
          {
            zoff_t cs;
            int byte2;

            cs = G.csize;
            while (cs-- > 0)
            {
              if ((byte2 = NEXTBYTE) == EOF)
              {
                break;
              }
            }
            undefer_input(__G);         /* Re-adjust input counts, pointers. */
          }
        }
        else
        {
          members_processed++;
        }

        if (error == PK_OK)
        {
          if (store_info(__G) != 0)
          {
            ++num_skipped;      /* Unsupported compression or encryption. */
          }
        }

# if defined( UNIX) && defined( __APPLE__)
        /* Save do_this_file, for possible use with the next (AD?) file. */
        G.do_this_prev = do_this_file;
        if ((!uO.J_flag) && G.exdir_attr_ok)
        {
          /* Using integrated AppleDouble processing.  Look for an
           * AppleDouble next, iff this one was normal.
           */
          G.seeking_apl_dbl = !G.apl_dbl;
        }
# endif /* defined( UNIX) && defined( __APPLE__) */

# ifdef IZ_CRYPT_AES_WG

        if (error == PK_OK)
        {
          /* Analyze any AES encryption extra block before calculating
           * the true uncompressed file size.
           */
          error = aes_wg_prep( __G__ &temp_compression_method,
                                     &temp_stored_size_decr);
        }
# endif /* def IZ_CRYPT_AES_WG */

        if (error == PK_OK)
        {
          if (REAL_COMPRESSION_METHOD == STORED)
          {
            /* Check compressed size against uncompressed size. */
            error = size_check( __G__ REAL_STORED_SIZE_DECR);
            if (error != PK_OK)
            {
              error_in_archive = IZ_MAX( error_in_archive, PK_WARN);
            }
          }

# ifdef IZ_CRYPT_ANY
          if (G.pInfo->encrypted)
          {
            /* Encrypted.  Verify the password. */
            error = password_check( __G);
            if (error != PK_OK)
            {
              if (error == PK_WARN)
              {
                ++num_bad_pwd;
              }
              else
              {
                error_in_archive = IZ_MAX( error_in_archive, error);
              }
            }
          }
# endif /* def IZ_CRYPT_ANY */
        }

# if defined( UNIX) && defined( __APPLE__)
        if (error == PK_OK)
        {
          /* Unless the user objects, or the destination volume does not
           * support setattrlist(), detect an AppleDouble file (by name),
           * and set flags and adjusted file name accordingly.
           */
          error = detect_apl_dbl( __G);
          if (error != PK_OK)
          {
            /* Skip a sequestration directory (or worse). */
            if (error > PK_WARN)
            {
              error_in_archive = error;
            }
          }
        }

        if (error == PK_OK)
        {
          if (G.apple_double == 0)
          {
            /* User has not yet renamed this (non-AppleDouble) file. */
            renamed = 0;
          }
#endif /* defined( UNIX) && defined( __APPLE__) [else] */

#if defined( UNIX) && defined( __APPLE__)
          /* If we are doing special AppleDouble file processing, and
           * the main file was skipped, then skip this AppleDouble
           * member, too.
           */
          if ((!uO.J_flag) && G.apple_double && (G.do_this_prev == 0))
          {
            error = PK_ERR;
          }
        }
#else /* defined( UNIX) && defined( __APPLE__) */
        if (error == PK_OK)
        {
          renamed = 0;  /* User has not yet renamed this file. */
        }
# endif /* defined( UNIX) && defined( __APPLE__) */

        /* Getting close to extraction.  If extracting to file, then
         * detect and deal with file name conflicts (rename/retry/skip).
         */
        if (error == PK_OK)
        {
          error = conflict_query( __G__ &renamed,
# ifdef SET_DIR_ATTRIB
                                        &num_dirs, &dir_list,
# endif
                                        &error_in_archive);
        }

        if (error != PK_OK)
        {
          /* Skip this member.  We created a directory, or there was
           * some other member-terminal condition.
           */
#if defined( UNIX) && defined( __APPLE__)
          /* If we are doing special AppleDouble file processing, then
           * record the fact that we're skipping this member.
           * (Significant only for a following non-AppleDouble file.)
           */
          if (!uO.J_flag)
          {
            G.do_this_prev = 0;
          }
#endif /* defined( UNIX) && defined( __APPLE__) */
        }

#if defined( UNIX) && defined( __APPLE__)
        if (error == PK_OK)
        {
          /* If we are doing special AppleDouble file processing, and the
           * user renamed the main file, then act accordingly.
           */
          if ((!uO.J_flag) && renamed)
          {
            if (G.apple_double)
            {
              /* This is the AppleDouble file, so substitute the saved
               * replacement name.
               */
              strcpy( G.filename, G.pr_filename);
            }
            else
            {
              /* This is the main file, so save the replacement name for
               * future use on the following AppleDouble file.
               */
              strcpy( G.pr_filename, G.filename);
            }
          }
        }
#endif /* defined( UNIX) && defined( __APPLE__) */

        if (error == PK_OK)
        {
          /* Extract or test the member. */
          error = extract_or_test_member( __G);
          if (error > error_in_archive)
          {
            error_in_archive = error;       /* ...and keep going */
          }
        } /* if (error == PK_OK) */
      } /* if (error == PK_OK) */
    } /* if (state == 4) */
    else if (state == 6)
    {
      /* Found central directory file header.  Expecting no more members. */
      break;
    }
  } /* while (1) */

/*---------------------------------------------------------------------------
    Process the list of deferred symlink extractions and finish up
    the symbolic links.
  ---------------------------------------------------------------------------*/
#ifdef SYMLINKS
  set_deferred_symlinks(__G);
#endif /* SYMLINKS */

/*---------------------------------------------------------------------------
    Go back through saved list of directories, sort and set times/perms/UIDs
    and GIDs from the deepest level on up.
  ---------------------------------------------------------------------------*/

#ifdef SET_DIR_ATTRIB

  if (num_dirs > 0)
  {
    error = set_dir_attribs( __G__ num_dirs,
                                   dir_list,
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
                                   dir_listw,
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
                                   &error_in_archive);
  }
#endif /* SET_DIR_ATTRIB */

  /* Emit warning if unmatched include or exclued (-x) names.
   * Free name-match storage.
   */
  error = check_unmatched_names( __G__ 1, error_in_archive);
  if (error > error_in_archive)
      error_in_archive = error;

  error_in_archive = extract_test_trailer( __G__ (ulg)members_processed,
   num_bad_pwd, num_skipped, error_in_archive);

  return error;
} /* extract_or_test_stream(). */

#endif /* def ARCHIVE_STDIN */


/*******************************/
/*  Function close_segment().  */
/*******************************/
static void close_segment(__G)
  __GDEF
{
  if (G.zipfn_sgmnt != NULL)
  {
    izu_free(G.zipfn_sgmnt);
    G.zipfn_sgmnt = NULL;
  }
  CLOSE_INFILE( &G.zipfd_sgmnt);

} /* close_segment(). */


/**********************************/
/*  Function find_local_header()  */
/**********************************/

static int find_local_header( __G__ perr_in_arch, filnum, pold_extra_bytes)
    __GDEF
    int *perr_in_arch;
    ulg filnum;
    zoff_t *pold_extra_bytes;
{
  zoff_t bufstart;
  zoff_t inbuf_offset;
  zoff_t request;
  int error = PK_OK;                    /* Return PK-type error code. */

  /* If the target position is not within the current input buffer
   * (either haven't yet read far enough, or (maybe) skipping backward),
   * then skip to the target position and reset readbuf().
   */
  /*if(G.ecrec.number_this_disk != G.pInfo->diskstart) {*/
  if (G.ecrec.number_this_disk > 0)
  {
    /* Open file (when it is segmented) even if it is a duplicate
     * of the same fd, due to possible unwanted changes of G.zipfd
     * and G.sgmnt_nr which were saved in extract_or_test_files().
     */
    if (!fd_is_valid(G.zipfd_sgmnt) || G.sgmnt_nr != G.pInfo->diskstart)
    {
      if (fd_is_valid(G.zipfd_sgmnt))
        close_segment( __G);            /* We need a different file. */

      set_zipfn_sgmnt_name( __G__ G.pInfo->diskstart);
      if (open_infile( __G__ OIF_SEGMENT))
      {
        /* TODO: ask for place/path of zipfile, see wild*!, ... */
        /* Create new function for this all? */
        izu_free(G.zipfn_sgmnt);
        G.zipfn_sgmnt = NULL;
        error = PK_NOZIP;
        return error;
      }
      else
      {
        /* TODO: that's not best solution but for
         * testing/alpha version is good enough now. */
        G.zipfd = G.zipfd_sgmnt;
        G.sgmnt_nr = G.pInfo->diskstart;
      }
      /* When we change file, we must refill buffer always!
       * (Same method as in seek_zipf().)
       */
      G.cur_zipfile_bufstart = -1;
    }
    /* TODO: check better too -- Is G.extra_bytes important in
     * segmented?  Otherwise bigger harakiri is here
     * needed (here = whole request block below)
     */
    request = G.pInfo->offset;
  }
  else
  {
    request = G.pInfo->offset + G.extra_bytes;
  }

  /* seek_zipf(__G__ pInfo->offset);  */
  /* TODO: this code must be checked / verified better by someone else!
   * I'm not sure about impact of changes what are here done for
   * multi-disk support.
   */
  inbuf_offset = request % INBUFSIZ;
  bufstart = request - inbuf_offset;

  Trace((stderr, "\ndebug: request = %ld, inbuf_offset = %ld\n",
   (long)request, (long)inbuf_offset));
  Trace((stderr,
   "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
   (long)bufstart, (long)G.cur_zipfile_bufstart));
  if (request < 0)
  {
    Info(slide, 0x401, ((char *)slide, LoadFarStringSmall(SeekMsg),
     G.zipfn, 1, LoadFarString(ReportMsg)));
    *perr_in_arch = PK_ERR;
    if (filnum == 1 && G.extra_bytes != 0L)
    {
      Info(slide, 0x401, ((char *)slide,
       LoadFarString(AttemptRecompensate)));
      /* Try a different "request". */
      *pold_extra_bytes = G.extra_bytes;
      G.extra_bytes = 0L;
      request = G.pInfo->offset;  /* could also check if != 0 */
      inbuf_offset = request % INBUFSIZ;
      bufstart = request - inbuf_offset;
      Trace((stderr, "debug: request = %ld, inbuf_offset = %ld\n",
       (long)request, (long)inbuf_offset));
      Trace((stderr,
       "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
       (long)bufstart, (long)G.cur_zipfile_bufstart));
      if (request < 0)
      {
        Trace((stderr,
         "debug: recompensated request still < 0\n"));
         Info(slide, 0x401, ((char *)slide,
         LoadFarStringSmall(SeekMsg),
         G.zipfn, 2, LoadFarString(ReportMsg)));
        error = *perr_in_arch = PK_BADERR;      /* Error.  Skip this member. */
      }
    }
    else
    {
      error = *perr_in_arch = PK_BADERR;        /* Error.  Skip this member. */
    }
  /* Else, must be ok. */
  }

  if (error == PK_OK)
  {
    if (bufstart != G.cur_zipfile_bufstart)
    {
      /* Not already at the desired file offset. */
      Trace((stderr, "debug: bufstart != cur_zipfile_bufstart\n"));
#ifdef USE_STRM_INPUT
      zfseeko(G.zipfd, bufstart, SEEK_SET);
      G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* def USE_STRM_INPUT */
      G.cur_zipfile_bufstart = zlseek(G.zipfd, bufstart, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */
      /* Read the desired first buffer. */
      if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) <= 0)
      {
        Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
         filnum, "seek/read", (long)bufstart));
        error = *perr_in_arch = PK_BADERR;      /* Error.  Skip this member. */
      }
      else
      {
        /* Set the count and pointer for the new desired buffer. */
        G.inptr = G.inbuf + (int)inbuf_offset;
        G.incnt -= (int)inbuf_offset;
      }
    }
    else
    {
      /* Set the count and pointer for the old buffer. */
      G.incnt += (int)(G.inptr-G.inbuf) - (int)inbuf_offset;
      G.inptr = G.inbuf + (int)inbuf_offset;
    }
  }

  if (error == PK_OK)
  {
    /* Should be in proper position now, so check for signature. */
    if (readbuf(__G__ G.sig, 4) == 0)
    {
      /* Signature read (1) failed.  (Bad offset?) */
      Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
       filnum, "EOF", (long)request));
      error = *perr_in_arch = PK_BADERR;        /* Error.  Skip this member. */
    }
  }

  if (error == PK_OK)
  {
    if (memcmp(G.sig, local_hdr_sig, 4))
    {
      /* Not the expected signature (1). */
      Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
       filnum, LoadFarStringSmall(LocalHdrSig), (long)request));
#if DIAGNOSTIC
      GRRDUMP(G.sig, 4)
      GRRDUMP(local_hdr_sig, 4)
#endif /* DIAGNOSTIC */

      *perr_in_arch = PK_ERR;
      if ((filnum == 1 && G.extra_bytes != 0L) ||
       (G.extra_bytes == 0L && *pold_extra_bytes != 0L))
      {
        Info(slide, 0x401, ((char *)slide,
         LoadFarString(AttemptRecompensate)));
        if (G.extra_bytes)
        {
          *pold_extra_bytes = G.extra_bytes;
          G.extra_bytes = 0L;
        }
        else
        {
          G.extra_bytes = *pold_extra_bytes; /* third attempt */
        }
        if (((error = seek_zipf(__G__ G.pInfo->offset)) != PK_OK) ||
         (readbuf(__G__ G.sig, 4) == 0))
        {
          /* Signature seek/read (2) failed.  (Bad offset?) */
          if (error != PK_BADERR)       /* (seek_zipf() status.) */
          {
            Info(slide, 0x401, ((char *)slide,
             LoadFarString(OffsetMsg), filnum, "EOF", (long)request));
          }
          error = *perr_in_arch = PK_BADERR;    /* Error.  Skip this member. */
        }
        else
        {
          if (memcmp(G.sig, local_hdr_sig, 4))
          {
            /* Not the expected signature (2). */
            Info(slide, 0x401, ((char *)slide,
             LoadFarString(OffsetMsg), filnum,
             LoadFarStringSmall(LocalHdrSig), (long)request));
            *perr_in_arch = PK_BADERR;
            error = *perr_in_arch = PK_BADERR;  /* Error.  Skip this member. */
          }
        }
      }
      else
      {
        error = PK_BADERR;  /* Error.  Skip this member. */
      }
    }
    /* Else, all is well.  We found the expected signature. */
  }

  return error;
} /* find_local_header(). */


#ifndef SFX

/*****************************************/
/*  Function central_local_name_check()  */
/*****************************************/

static void central_local_name_check( __G__ perr_in_arch)
  __GDEF
  int *perr_in_arch;
{
  if (G.pInfo->cfilname != (char Far *)NULL)
  {
    /* There is a central header name. */
    if (zfstrcmp(G.pInfo->cfilname, G.filename) != 0)
    {
      /* Name mismatch. */
# ifdef SMALL_MEM
      char *temp_cfilnam = slide + (7 * (WSIZE>>3));

      zfstrcpy((char Far *)temp_cfilnam, G.pInfo->cfilname);
#  define cFile_PrintBuf  temp_cfilnam
# else
#  define cFile_PrintBuf  G.pInfo->cfilname
# endif
      Info(slide, 0x401, ((char *)slide,
       LoadFarStringSmall2(LvsCFNamMsg),
       FnFilter2(cFile_PrintBuf), FnFilter1(G.filename)));
# undef   cFile_PrintBuf
      zfstrcpy(G.filename, G.pInfo->cfilname);
      if (*perr_in_arch < PK_WARN)
        *perr_in_arch = PK_WARN;
    }
    zffree(G.pInfo->cfilname);
    G.pInfo->cfilname = (char Far *)NULL;
  }
} /* central_local_name_check(). */
#endif /* ndef SFX */


/******************************************/
/*  Function extract_or_test_entrylist()  */
/******************************************/

static int extract_or_test_entrylist(__G__ mbr_ndx,
                pfilnum, pnum_bad_pwd, pold_extra_bytes,
#ifdef SET_DIR_ATTRIB
                pnum_dirs, pdir_list,
#endif
                error_in_archive)    /* return PK-type error code */
    __GDEF
    unsigned mbr_ndx;
    ulg *pfilnum;
    ulg *pnum_bad_pwd;
    zoff_t *pold_extra_bytes;
#ifdef SET_DIR_ATTRIB
    unsigned *pnum_dirs;
    direntry **pdir_list;
#endif
    int error_in_archive;
{
    unsigned i;
    int renamed;
    int error;

#ifdef IZ_CRYPT_AES_WG
    ush temp_compression_method;
    int temp_stored_size_decr;
#endif /* def IZ_CRYPT_AES_WG) */

#if defined( UNIX) && defined( __APPLE__)
    int cfn;
#endif /* defined( UNIX) && defined( __APPLE__) */

    /* Loop through the G.info[] array (which was filled in by
     * extract_or_test_files()), extracting or testing each member.
     */
    for (i = 0; i < mbr_ndx; ++i) {
        (*pfilnum)++;   /* *pfilnum = i + blknum*DIR_BLKSIZ + 1; */
        G.pInfo = &G.info[i];
#ifdef NOVELL_BUG_FAILSAFE
        G.dne = FALSE;  /* assume file exists until stat() says otherwise */
#endif

        /* Find the local header for this member. */
        error = find_local_header( __G__
                                   &error_in_archive,
                                   *pfilnum,
                                   pold_extra_bytes);

        if (error != PK_OK)
            continue;           /* Find header failed.  Skip this member. */

        if ((error = process_local_file_hdr(__G)) != PK_COOL) {
            Info(slide, 0x421, ((char *)slide, LoadFarString(BadLocalHdr),
              *pfilnum));
            error_in_archive = error;   /* only PK_EOF defined */
            continue;   /* can still try next one */
        }

        if ((error =
         do_string(__G__ G.lrec.filename_length, DS_FN_L)) != PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401, ((char *)slide, LoadFarString(FilNamMsg),
                  FnFilter1(G.filename), "local"));
                continue;   /* go on to next one */
            }
        }

        if ((error =
         do_string(__G__ G.lrec.extra_field_length, EXTRA_FIELD)) != PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExtFieldMsg),
                  FnFilter1(G.filename), "local"));
                continue;   /* go on */
            }
        }

#ifdef USE_EF_STREAM

        /* Process any EF_STREAM extra block. */
        {
          uch bitmap[ EF_STREAM_BM_SIZE];
          ext_local_file_hdr xlhdr;
          char *cmnt;
          int btmp_siz = EF_STREAM_BM_SIZE;
          int sts;

          sts = ef_scan_for_stream( G.extra_field,
                                    (long)G.lrec.extra_field_length,
                                    &btmp_siz,
                                    &bitmap[ 0],
                                    &xlhdr,
                                    &cmnt);

          if (sts >= 0)
          {

#ifdef DIAGNOSTIC
            fprintf( stderr, "    e_s_f_s() = %d.\n", sts);
            fprintf( stderr, "     btmp_len = %d.\n", btmp_siz);
            fprintf( stderr, "     bitmap[ 0] = %02x .\n", bitmap[ 0]);

            fprintf( stderr,
             "     xlhdr.version_made_by[ 0, 1] = %02x %02x.\n",
             xlhdr.version_made_by[ 0], xlhdr.version_made_by[ 1]);

            fprintf( stderr,
             "     xlhdr.internal_file_attributes = %04x .\n",
             xlhdr.internal_file_attributes);

            fprintf( stderr,
             "     xlhdr.external_file_attributes = %08x .\n",
             (unsigned int)xlhdr.external_file_attributes);

            fprintf( stderr, "     xlhdr.file_comment_length = %d.\n",
             xlhdr.file_comment_length);

            fprintf( stderr, "     cmnt = %08x .\n", (unsigned int)cmnt);
            if (cmnt != NULL)
            {
              fprintf( stderr,
               "     cmnt: >%*s<.\n", xlhdr.file_comment_length, cmnt);
            }
#endif /* def DIAGNOSTIC */

            /* Fill in some G.pInfo fields (and fake some G.crec fields)
             * from extended local header data.
             */
            if (btmp_siz > 0)                   /* Bitmap byte 0 available. */
            {
              if (bitmap[ 0]& (1 << 0))         /* Version made by. */
              {
                G.pInfo->hostver = xlhdr.version_made_by[ 0];
                G.pInfo->hostnum =
                 IZ_MIN( xlhdr.version_made_by[ 1], NUM_HOSTS);
              }

              if (bitmap[ 0]& (1 << 1))         /* Internal file attributes. */
              {
                G.crec.internal_file_attributes =
                 xlhdr.internal_file_attributes;
              }

              if (bitmap[ 0]& (1 << 2))         /* External file attributes. */
              {
                G.crec.external_file_attributes =
                 xlhdr.external_file_attributes;
              }

#ifdef FUTURE
              /* We currently do nothing with a member comment which is
               * not in the central directory.
               */
              if (bitmap[ 0]& (1 << 3))         /* File comment. */
              {
                length: xlhdr->file_comment_length
                string: cmnt
              }
#endif /* def FUTURE */

            } /* if (btmp_siz > 0) */
          } /* if (sts >= 0) */
        } /* (scope block) */

#endif /* def USE_EF_STREAM */

#ifndef SFX
        /* Filename consistency checks must come after reading in the
         * local extra field, so that a UTF-8 entry name e.f. block has
         * already been processed.
         */
        central_local_name_check( __G__ &error_in_archive);
#endif /* ndef SFX */

#ifdef IZ_CRYPT_AES_WG
        /* Analyze any AES encryption extra block before calculating
         * the true uncompressed file size.
         */
        error = aes_wg_prep( __G__ &temp_compression_method,
                                   &temp_stored_size_decr);
        if (error != PK_OK)
        {
            continue;   /* Go on to the next member. */
        }

#endif /* def IZ_CRYPT_AES_WG */

        if (REAL_COMPRESSION_METHOD == STORED)
        {
            /* Check compressed size against uncompressed size. */
            error = size_check( __G__ REAL_STORED_SIZE_DECR);
            if (error != PK_OK)
            {
                error_in_archive = IZ_MAX( error_in_archive, PK_WARN);
            }
        }

#ifdef IZ_CRYPT_ANY
        if (G.pInfo->encrypted)
        {
          /* Encrypted.  Verify the password. */
          error = password_check( __G);
          if (error != PK_OK)
          {
            if (error == PK_WARN)
            {
              ++(*pnum_bad_pwd);
            }
            else
            {
              error_in_archive = IZ_MAX( error_in_archive, error);
            }
            continue;         /* Go on to next member. */
          }
        }
#endif /* def IZ_CRYPT_ANY */

#if defined( UNIX) && defined( __APPLE__)
        /* Unless the user objects, or the destination volume does not
         * support setattrlist(), detect an AppleDouble file (by name),
         * and set flags and adjusted file name accordingly.
         */
        error = detect_apl_dbl( __G);
        if (error != PK_OK)
        {
          /* Skip a sequestration directory (or worse). */
          if (error > PK_WARN)
          {
            error_in_archive = error;
          }
          continue;
        }

        if (G.apple_double == 0)
        {
          /* User has not yet renamed this (non-AppleDouble) file. */
          renamed = 0;
        }
#endif /* defined( UNIX) && defined( __APPLE__) [else] */

#if defined( UNIX) && defined( __APPLE__)
        /* If we are doing special AppleDouble file processing, and
         * the main file was skipped, then skip this AppleDouble
         * member, too.
         */
        if ((!uO.J_flag) && G.apple_double && (G.do_this_prev == 0))
        {
          error = PK_ERR;
        }
        else
#else /* defined( UNIX) && defined( __APPLE__) */
        renamed = 0;    /* User has not yet renamed this file. */
#endif /* defined( UNIX) && defined( __APPLE__) */
        {

          /* Getting close to extraction.  If extracting to file, then
           * detect and deal with file name conflicts (rename/retry/skip).
           */
          error = conflict_query( __G__ &renamed,
# ifdef SET_DIR_ATTRIB
                                        pnum_dirs, pdir_list,
# endif
                                        &error_in_archive);
        }

        if (error != PK_OK)
        {
          /* Skip this member.  We created a directory, or there was
           * some other member-terminal condition.
           */
#if defined( UNIX) && defined( __APPLE__)
          /* If we are doing special AppleDouble file processing, then
           * record the fact that we're skipping this member.
           * (Significant only for a following non-AppleDouble file.)
           */
          if (!uO.J_flag)
          {
            G.do_this_prev = 0;
          }
#endif /* defined( UNIX) && defined( __APPLE__) */
          continue;
        }

#if defined( UNIX) && defined( __APPLE__)
        /* Set do_this_prev, for possible use with the next (AD?) file. */
        G.do_this_prev = (G.apple_double == 0);

        /* If we are doing special AppleDouble file processing, and the
         * user renamed the main file, then act accordingly.
         */
        if ((!uO.J_flag) && renamed)
        {
          if (G.apple_double)
          {
            /* This is the AppleDouble file, so substitute the saved
             * replacement name.
             */
            strcpy( G.filename, G.pr_filename);
          }
          else
          {
            /* This is the main file, so save the replacement name for
             * future use on the following AppleDouble file.
             */
            strcpy( G.pr_filename, G.filename);
          }
        }
#endif /* defined( UNIX) && defined( __APPLE__) */

#ifdef DLL
        if ((G.statreportcb != NULL) &&
         (*G.statreportcb)(__G__ UZ_ST_START_EXTRACT, G.zipfn,
         G.filename, NULL))
        {
          close_segment( __G);
          return IZ_CTRLC;      /* Cancel operation by user request. */
        }
#endif
#ifdef MACOS  /* MacOS is no preemptive OS, thus call event-handling by hand */
        UserStop();
#endif
#ifdef AMIGA
        G.filenote_slot = i;
#endif
        G.disk_full = 0;

        if ((error = extract_or_test_member(__G)) != PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;       /* ...and keep going */

            if ((G.disk_full > 1)
#ifdef DLL
             || (error_in_archive == IZ_CTRLC)
#endif
             )
            {
              close_segment( __G);
              return error_in_archive;          /* (unless disk full) */
            }
        }
#ifdef DLL
        if ((G.statreportcb != NULL) &&
         (*G.statreportcb)(__G__ UZ_ST_FINISH_MEMBER, G.zipfn,
         G.filename, (zvoid *)&G.lrec.ucsize))
        {
          close_segment( __G);
          return IZ_CTRLC;          /* Cancel operation by user request. */
        }
#endif
#ifdef MACOS  /* MacOS is no preemptive OS, thus call event-handling by hand */
        UserStop();
#endif
    } /* end for-loop (i:  files in current block) */

    close_segment( __G);
    return error_in_archive;

} /* extract_or_test_entrylist(). */


#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)

/*******************************************/
/*  Function extract_or_test_entrylistw()  */
/*******************************************/

static int extract_or_test_entrylistw(__G__ mbr_ndx,
                pfilnum, pnum_bad_pwd, pold_extra_bytes,
# ifdef SET_DIR_ATTRIB
                pnum_dirs, pdir_listw,
# endif
                error_in_archive)    /* return PK-type error code */
    __GDEF
    unsigned mbr_ndx;
    ulg *pfilnum;
    ulg *pnum_bad_pwd;
    zoff_t *pold_extra_bytes;
# ifdef SET_DIR_ATTRIB
    unsigned *pnum_dirs;
    direntryw **pdir_listw;
# endif
    int error_in_archive;
{
    unsigned i;
    int renamed;
    int error;

# ifdef IZ_CRYPT_AES_WG
    ush temp_compression_method;
    int temp_stored_size_decr;
# endif /* def IZ_CRYPT_AES_WG */

    /* Loop through the G.info[] array (which was filled in by
     * extract_or_test_files()), extracting or testing each member.
     */
    for (i = 0; i < mbr_ndx; ++i) {
        (*pfilnum)++;   /* *pfilnum = i + blknum*DIR_BLKSIZ + 1; */
        G.pInfo = &G.info[i];
# ifdef NOVELL_BUG_FAILSAFE
        G.dne = FALSE;  /* assume file exists until stat() says otherwise */
# endif

        /* Find the local header for this member. */
        error = find_local_header( __G__
                                   &error_in_archive,
                                   *pfilnum,
                                   pold_extra_bytes);

        if (error != PK_OK)
            continue;           /* Find header failed.  Skip this member. */

        if ((error = process_local_file_hdr(__G)) != PK_COOL) {
            Info(slide, 0x421, ((char *)slide, LoadFarString(BadLocalHdr),
              *pfilnum));
            error_in_archive = error;   /* only PK_EOF defined */
            continue;   /* can still try next one */
        }
# if !defined(SFX) && defined(UNICODE_SUPPORT)
        if (((G.lrec.general_purpose_bit_flag & UTF8_BIT) != 0)
            != (G.pInfo->GPFIsUTF8 != 0)) {
            if (QCOND2) {
#  ifdef SMALL_MEM
                char *temp_cfilnam = slide + (7 * (WSIZE>>3));

                zfstrcpy((char Far *)temp_cfilnam, G.pInfo->cfilname);
#   define  cFile_PrintBuf  temp_cfilnam
#  else
#   define  cFile_PrintBuf  G.pInfo->cfilname
#  endif
                Info(slide, 0x421, ((char *)slide,
                  LoadFarStringSmall2(GP11FlagsDiffer),
                  *pfilnum, FnFilter1(cFile_PrintBuf), G.pInfo->GPFIsUTF8));
#  undef    cFile_PrintBuf
            }
            if (error_in_archive < PK_WARN)
                error_in_archive = PK_WARN;
        }
# endif /* # if !defined(SFX) && defined(UNICODE_SUPPORT) */
        if ((error =
         do_string(__G__ G.lrec.filename_length, DS_FN_L)) != PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401, ((char *)slide, LoadFarString(FilNamMsg),
                  FnFilter1(G.filename), "local"));
                continue;   /* go on to next one */
            }
        }

# ifdef DYNAMIC_WIDE_NAME
        /* 2013-02-10 SMS.
         * fileio.c:do_string() will free() and NULL G.unipath_filename,
         * as needed.  G.unipath_widefilename is handled here.
         */
        if (G.unipath_widefilename) {
            izu_free(G.unipath_widefilename);
            G.unipath_widefilename = NULL;
        }
# else /* def DYNAMIC_WIDE_NAME */
        *G.unipath_widefilename = L'\0';
# endif /* def DYNAMIC_WIDE_NAME [else] */

        if ((error =
         do_string(__G__ G.lrec.extra_field_length, EXTRA_FIELD)) != PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExtFieldMsg),
                  FnFilter1(G.filename), "local"));
                continue;   /* go on */
            }
        }

#ifdef USE_EF_STREAM

        /* Process any EF_STREAM extra block. */
        {
          uch bitmap[ EF_STREAM_BM_SIZE];
          ext_local_file_hdr xlhdr;
          char *cmnt;
          int btmp_siz = EF_STREAM_BM_SIZE;
          int sts;

          sts = ef_scan_for_stream( G.extra_field,
                                    (long)G.lrec.extra_field_length,
                                    &btmp_siz,
                                    &bitmap[ 0],
                                    &xlhdr,
                                    &cmnt);

          if (sts >= 0)
          {
            /* Fill in some G.pInfo fields (and fake some G.crec fields)
             * from extended local header data.
             */
            if (btmp_siz > 0)                   /* Bitmap byte 0 available. */
            {
              if (bitmap[ 0]& (1 << 0))         /* Version made by. */
              {
                G.pInfo->hostver = xlhdr.version_made_by[ 0];
                G.pInfo->hostnum = IZ_MIN( xlhdr.version_made_by[ 1], NUM_HOSTS);
              }

              if (bitmap[ 0]& (1 << 1))         /* Internal file attributes. */
              {
                G.crec.internal_file_attributes = xlhdr.internal_file_attributes;
              }

              if (bitmap[ 0]& (1 << 2))         /* External file attributes. */
              {
                G.crec.external_file_attributes = xlhdr.external_file_attributes;
              }

#ifdef FUTURE
              /* We currently do nothing with a member comment which is
               * not in the central directory.
               */
              if (bitmap[ 0]& (1 << 3))         /* File comment. */
              {
                length: xlhdr->file_comment_length
                string: cmnt
              }
#endif /* def FUTURE */

            } /* if (btmp_siz > 0) */
          } /* if (sts >= 0) */
        } /* (scope block) */

#endif /* def USE_EF_STREAM */

# ifdef DYNAMIC_WIDE_NAME
        if (G.unipath_widefilename == NULL) {
            if (G.unipath_filename) {
                /* Get wide path from UTF-8 */
                G.unipath_widefilename =
                 utf8_to_wchar_string(G.unipath_filename);
            }
            else {
                G.unipath_widefilename = utf8_to_wchar_string(G.filename);
            }
            if (G.pInfo->lcflag) {      /* replace with lowercase filename */
                wcslwr(G.unipath_widefilename);
            }
        }
# else /* def DYNAMIC_WIDE_NAME */
        if (*G.unipath_widefilename == L'\0') {
            if (G.unipath_filename) {
                /* Get wide path from UTF-8 */
                utf8_to_wchar_string( G.unipath_widefilename,
                 G.unipath_filename);
            }
            else {
                utf8_to_wchar_string( G.unipath_widefilename, G.filename);
            }
            if (G.pInfo->lcflag) {      /* replace with lowercase filename */
                wcslwr(G.unipath_widefilename);
            }
# endif /* def DYNAMIC_WIDE_NAME [else] */
# if 0
            if (G.pInfo->vollabel && (length > 8) &&
             (G.unipath_widefilename[8] == '.')) {
                wchar_t *p = G.unipath_widefilename+8;
                while (*p++)
                    p[-1] = *p;  /* disk label, and 8th char is dot: remove dot */
            }
# endif /* 0 */
        }

# ifndef SFX
        /* Filename consistency checks must come after reading in the
         * local extra field, so that a UTF-8 entry name e.f. block has
         * already been processed.
         */
        central_local_name_check( __G__ &error_in_archive);
# endif /* ndef SFX */

#ifdef IZ_CRYPT_AES_WG
        /* Analyze any AES encryption extra block before calculating
         * the true uncompressed file size.
         */
        error = aes_wg_prep( __G__ &temp_compression_method,
                                   &temp_stored_size_decr);
        if (error != PK_OK)
        {
            continue;   /* Go on to the next member. */
        }
#endif /* def IZ_CRYPT_AES_WG */

        if (REAL_COMPRESSION_METHOD == STORED)
        {
            /* Check compressed size against uncompressed size. */
            error = size_check( __G__ REAL_STORED_SIZE_DECR);
            if (error != PK_OK)
            {
                error_in_archive = IZ_MAX( error_in_archive, PK_WARN);
            }
        }

# ifdef IZ_CRYPT_ANY
        if (G.pInfo->encrypted)
        {
          /* Encrypted.  Verify the password. */
          error = password_check( __G);
          if (error != PK_OK)
          {
            if (error == PK_WARN)
            {
              ++(*pnum_bad_pwd);
            }
            else
            {
              error_in_archive = IZ_MAX( error_in_archive, error);
            }
            continue;           /* Go on to the next member. */
          }
        }
# endif /* def IZ_CRYPT_ANY */

        /* Getting close to extraction.  If extracting to file, then
         * detect and deal with file name conflicts (rename/retry/skip).
         */
        renamed = 0;
        error = conflict_queryw( __G__ &renamed,
# ifdef SET_DIR_ATTRIB
                                       pnum_dirs, pdir_listw,
# endif
                                       &error_in_archive);

        if (error != PK_OK)
        {
          /* Skip this member.  We created a directory, or there was
           * some other member-terminal condition.
           */
          continue;
        }

# ifdef DLL
        if ((G.statreportcb != NULL) &&
            (*G.statreportcb)(__G__ UZ_ST_START_EXTRACT, G.zipfn,
                              G.filename, NULL))
        {
          close_segment( __G);
          return IZ_CTRLC;      /* Cancel operation by user request. */
        }
# endif

        G.disk_full = 0;
        if ((error = extract_or_test_member(__G)) != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error;       /* ...and keep going */

            if ((G.disk_full > 1)
# ifdef DLL
             || (error_in_archive == IZ_CTRLC)
# endif
             )
            {
              close_segment( __G);
              return error_in_archive;          /* (unless disk full) */
            }
        }
# ifdef DLL
        if ((G.statreportcb != NULL) &&
         (*G.statreportcb)(__G__ UZ_ST_FINISH_MEMBER, G.zipfn,
         G.filename, (zvoid *)&G.lrec.ucsize))
        {
          close_segment( __G);
          return IZ_CTRLC;          /* Cancel operation by user request. */
        }
# endif
    } /* end for-loop (i:  files in current block) */

    close_segment( __G);
    return error_in_archive;

} /* extract_or_test_entrylistw(). */

#endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */


/**************************************/
/*  Function extract_or_test_files()  */
/**************************************/

int extract_or_test_files(__G)    /* return PK-type error code */
     __GDEF
{
    unsigned mbr_ndx;
    zoff_t cd_bufstart;
    uch *cd_inptr;
    int cd_incnt;
    ulg filnum = 0L;
    ulg blknum = 0L;
    int reached_end;
    int sts;
    zuvl_t sgmnt_nr;
    zipfd_t zipfd;
#ifndef SFX
    long enddigsig_len;
    int no_endsig_found;
#endif
    int do_this_file;
    int error;
    int error_in_archive = PK_COOL;
    zucn_t members_processed;
    ulg num_skipped = 0L;
    ulg num_bad_pwd = 0L;
    zoff_t old_extra_bytes = 0L;
#ifdef SET_DIR_ATTRIB
    unsigned num_dirs = 0;
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
    direntryw *dir_listw = (direntryw *)NULL;
# endif
    direntry *dir_list = (direntry *)NULL;
#endif

    /*
     * First, two general initializations are applied. These have been moved
     * here from process_zipfiles() because they are only needed for accessing
     * and/or extracting the data content of the zip archive.
     */

    /* a) initialize the CRC table pointer (once) */
    if (CRC_32_TAB == NULL) {
        if ((CRC_32_TAB = get_crc_table()) == NULL) {
            return PK_MEM;
        }
    }

#if !defined(SFX) || defined(SFX_EXDIR)
    /* b) Verify/create extraction destination directory. */
    error = extract_dest_dir( __G);
    if (error != PK_OK)
        return error;
#endif /* !defined(SFX) || defined(SFX_EXDIR) */

/*---------------------------------------------------------------------------
    The basic idea of this function is as follows.  Since the central di-
    rectory lies at the end of the zipfile and the member files lie at the
    beginning or middle or wherever, it is not very desirable to simply
    read a central directory entry, jump to the member and extract it, and
    then jump back to the central directory.  In the case of a large zipfile
    this would lead to a whole lot of disk-grinding, especially if each mem-
    ber file is small.  Instead, we read from the central directory the per-
    tinent information for a block of files, then go extract/test the whole
    block.  Thus this routine contains two small(er) loops within a very
    large outer loop:  the first of the small ones reads a block of files
    from the central directory; the second extracts or tests each file; and
    the outer one loops over blocks.  There's some file-pointer positioning
    stuff in between, but that's about it.  Btw, it's because of this jump-
    ing around that we can afford to be lenient if an error occurs in one of
    the member files:  we should still be able to go find the other members,
    since we know the offset of each from the beginning of the zipfile.
  ---------------------------------------------------------------------------*/

    G.pInfo = G.info;

#ifdef IZ_CRYPT_ANY
    G.newzip = TRUE;
#endif
#ifndef SFX
    G.reported_backslash = FALSE;
#endif

    /* Allocate space for checking unmatched list/-x list file specs. */
    error = allocate_name_match_arrays( __G);
    if (error != PK_OK)
        return error;

/*---------------------------------------------------------------------------
    Begin main loop over blocks of member files.  We know the entire central
    directory is on this disk:  we would not have any of this information un-
    less the end-of-central-directory record was on this disk, and we would
    not have gotten to this routine unless this is also the disk on which
    the central directory starts.  In practice, this had better be the ONLY
    disk in the archive, but we'll add multi-disk support soon.
  ---------------------------------------------------------------------------*/

    members_processed = 0;
#ifndef SFX
    enddigsig_len = -1;
    no_endsig_found = FALSE;
#endif
    reached_end = FALSE;
    while (!reached_end)
    {
#if defined( UNIX) && defined( __APPLE__)
        G.do_this_prev = 0;
        G.seeking_apl_dbl = 0;
#endif /* defined( UNIX) && defined( __APPLE__) */

        mbr_ndx = 0;

#ifdef AMIGA
        memzero(G.filenotes, DIR_BLKSIZ * sizeof(char *));
#endif
        /*
         * Loop through files in central directory, storing offsets, file
         * attributes, case-conversion and text-conversion flags until block
         * size is reached.
         */
        while ((mbr_ndx < DIR_BLKSIZ))
        {
            G.pInfo = &G.info[mbr_ndx];

            if (readbuf(__G__ G.sig, 4) == 0) {
                Info(slide, 0x221, ((char *)slide,
                 LoadFarString( ErrorUnexpectedEOF), 1, G.zipfn));
                error_in_archive = PK_EOF;
                reached_end = TRUE;     /* ...so no more left to do */
                break;
            }

            if (memcmp( G.sig, central_digsig_sig, 4) == 0)
            { /* Central directory digital signature.  Record its
               * existence (unless SFX).  Read (and, for now, ignore)
               * the data.
               */
#ifdef SFX
# define ENDDIGSIGLENPTR NULL
#else
# define ENDDIGSIGLENPTR &enddigsig_len
#endif /* def SFX [else] */

              error = process_cdir_digsig( __G__ ENDDIGSIGLENPTR);
              if (error != PK_OK)
              {
                error_in_archive = error;
                reached_end = TRUE;    /* Actual EOF or no mem. */
                break;
              }
            } /* process_cdir_digsig() should have read the next sig. */

            if (memcmp(G.sig, central_hdr_sig, 4)) {  /* is it a new entry? */
                /* no new central directory entry
                 * -> is the number of processed entries compatible with the
                 *    number of entries as stored in the end_central record?
                 */
                if ((members_processed
                     & (G.ecrec.have_ecr64 ? MASK_ZUCN64 : MASK_ZUCN16))
                    == G.ecrec.total_entries_central_dir) {
#ifndef SFX
                    /* yes, so look if we ARE back at the end_central record
                     */
                    no_endsig_found =
                     ((memcmp( G.sig,
                     (G.ecrec.have_ecr64 ?
                     end_central64_sig : end_central_sig), 4) != 0) &&
                     (!G.ecrec.is_zip64_archive) &&
                     (memcmp( G.sig, end_central_sig, 4) != 0));
#endif /* ndef SFX */
                } else {
                    /* no; we have found an error in the central directory
                     * -> report it and stop searching for more Zip entries
                     */
                    Info(slide, 0x401, ((char *)slide,
                     LoadFarString(CentSigMsg),
                     mbr_ndx+ blknum* DIR_BLKSIZ + 1));
                    Info(slide, 0x401, ((char *)slide,
                     LoadFarString(ReportMsg)));
                    error_in_archive = PK_BADERR;
                }
                reached_end = TRUE;     /* ...so no more left to do */
                break;
            }
            /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */
            if ((error = process_cdir_file_hdr(__G)) != PK_COOL) {
                error_in_archive = error;   /* only PK_EOF defined */
                reached_end = TRUE;     /* ...so no more left to do */
                break;
            }
            if ((error =
             do_string(__G__ G.crec.filename_length, DS_FN)) != PK_COOL)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal:  no more left to do */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(FilNamMsg),
                      FnFilter1(G.filename), "central"));
                    reached_end = TRUE;
                    break;
                }
            }
            if ((error =
             do_string(__G__ G.crec.extra_field_length, EXTRA_FIELD)) !=
             PK_COOL)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(ExtFieldMsg),
                      FnFilter1(G.filename), "central"));
                    reached_end = TRUE;
                    break;
                }
            }
            else if (uO.java_cafe == 0)
            {
                /* Set the Java CAFE flag by the first extra field found. */
                ef_scan_for_cafe( __G__ G.extra_field,
                 (long)G.crec.extra_field_length);
            }

#ifdef AMIGA
            G.filenote_slot = mbr_ndx;
            if ((error = do_string(__G__ G.crec.file_comment_length,
             uO.N_flag ? FILENOTE : SKIP)) != PK_COOL)
#else
            if ((error =
             do_string(__G__ G.crec.file_comment_length, SKIP)) != PK_COOL)
#endif
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal */
                    Info(slide, 0x421, ((char *)slide,
                      LoadFarString(BadFileCommLength),
                      FnFilter1(G.filename)));
                    reached_end = TRUE;
                    break;
                }
            }

            /* Decide whether to process this archive member.
             * match_include_exclude() handles name-based
             * inclusion/exclusion of AppleDouble files.
             */
            do_this_file = match_include_exclude( __G);

# if defined( UNIX) && defined( __APPLE__)
            G.do_this_prev = do_this_file;
            G.seeking_apl_dbl = 0;
# endif /* defined( UNIX) && defined( __APPLE__) */

            members_processed++;
            if (do_this_file > 0)
            {
                if (store_info(__G) == 0)
                    ++mbr_ndx;      /* File is OK.  Advance the index. */
                else
                    ++num_skipped;  /* Unsupported compression or encryption. */
            }

#if defined( UNIX) && defined( __APPLE__)
            if ((!uO.J_flag) && G.exdir_attr_ok)
            {
              /* Using integrated AppleDouble processing.  Look for an
               * AppleDouble next, iff this one was normal.
               */
              G.seeking_apl_dbl = !G.apl_dbl;
            }
#endif /* defined( UNIX) && defined( __APPLE__) */

        } /* end while-loop (adding files to current block) */

        /* Save position in central directory so can come back later. */
        cd_bufstart = G.cur_zipfile_bufstart;
        cd_inptr = G.inptr;
        cd_incnt = G.incnt;

        /* Save the file descr/pointer and segment number, which are subject
         * to change when the archive is segmented.
         */
        zipfd = G.zipfd;
        sgmnt_nr = G.sgmnt_nr;

    /*-----------------------------------------------------------------------
        Second loop:  process files in current block, extracting or testing
        each one.
      -----------------------------------------------------------------------*/

#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
        if (G.has_win32_wide)
        {
          error = extract_or_test_entrylistw(__G__ mbr_ndx,
                        &filnum, &num_bad_pwd, &old_extra_bytes,
# ifdef SET_DIR_ATTRIB
                        &num_dirs, &dir_listw,
# endif
                        error_in_archive);
        }
        else
#endif /* (defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)) */
        {
          error = extract_or_test_entrylist(__G__ mbr_ndx,
                        &filnum, &num_bad_pwd, &old_extra_bytes,
#ifdef SET_DIR_ATTRIB
                        &num_dirs, &dir_list,
#endif
                        error_in_archive);
        }
        /* Restore the file descr/pointer and segment number. */
        G.zipfd = zipfd;
        G.sgmnt_nr = sgmnt_nr;

        if (error != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error;
            /* ...and keep going (unless disk full or user break) */
            if (G.disk_full > 1 || error_in_archive == IZ_CTRLC) {
                /* clear reached_end to signal premature stop ... */
                reached_end = FALSE;
                /* ... and cancel scanning the central directory */
                break;
            }
        }

        /*
         * Jump back to where we were in the central directory, then go and do
         * the next batch of files.
         */
        /* 2014-03-14 SMS.
         * Added some basic I/O status checking.  Untested, but possibly
         * better than the previous complete lack of checking.  (Quiets
         * some fussy compiler/run-time checks, too.)
         */
#ifdef USE_STRM_INPUT
        sts = zfseeko( G.zipfd, cd_bufstart, SEEK_SET);
        if (sts >= 0)
        {
            G.cur_zipfile_bufstart = zftello( G.zipfd);
        }
        else
        {
            G.cur_zipfile_bufstart = -1;
        }
#else /* def USE_STRM_INPUT */
        G.cur_zipfile_bufstart =
         zlseek( G.zipfd, cd_bufstart, SEEK_SET);
#endif /* def USE_STRM_INPUT [else] */
        if (G.cur_zipfile_bufstart >= 0)
        {
            sts = read( G.zipfd, (char *)G.inbuf, INBUFSIZ);
        }
#ifndef USE_STRM_INPUT
        else
        {
            sts = -1;
        }
#endif /* ndef USE_STRM_INPUT */
        if (sts >= 0)
        {
          G.inptr = cd_inptr;
          G.incnt = cd_incnt;
          ++blknum;
        }
        else
        {
            Info(slide, 0x221, ((char *)slide,
             LoadFarString( ErrorUnexpectedEOF), 11, G.zipfn));
            error_in_archive = PK_EOF;
            reached_end = TRUE;     /* ...so no more left to do */
            break;
        }

#ifdef TEST
        printf("\ncd_bufstart = %ld (%.8lXh)\n", cd_bufstart, cd_bufstart);
        printf("cur_zipfile_bufstart = %ld (%.8lXh)\n", cur_zipfile_bufstart,
          cur_zipfile_bufstart);
        printf("inptr-inbuf = %d\n", G.inptr-G.inbuf);
        printf("incnt = %d\n\n", G.incnt);
#endif

    } /* end while-loop (blocks of files in central directory) */

/*---------------------------------------------------------------------------
    Process the list of deferred symlink extractions and finish up
    the symbolic links.
  ---------------------------------------------------------------------------*/

#ifdef SYMLINKS
    set_deferred_symlinks(__G);
#endif /* SYMLINKS */

/*---------------------------------------------------------------------------
    Go back through saved list of directories, sort and set times/perms/UIDs
    and GIDs from the deepest level on up.
  ---------------------------------------------------------------------------*/

#ifdef SET_DIR_ATTRIB

    if (num_dirs > 0)
    {
      error = set_dir_attribs( __G__ num_dirs,
                                     dir_list,
# if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
                                     dir_listw,
# endif /* defined(UNICODE_SUPPORT) && defined(WIN32_WIDE) */
                                     &error_in_archive);
    }
#endif /* SET_DIR_ATTRIB */

    /* Emit warning if unmatched include or exclued (-x) names.
     * Free name-match storage.
     */
    error = check_unmatched_names( __G__ reached_end, error_in_archive);
    if (error > error_in_archive)
        error_in_archive = error;

/*---------------------------------------------------------------------------
    Now, all locally allocated memory has been released.  When the central
    directory processing has been interrupted prematurely, it is safe to
    return immediately.  All completeness checks and summary messages are
    skipped in this case.
  ---------------------------------------------------------------------------*/
    if (!reached_end)
        return error_in_archive;

/*---------------------------------------------------------------------------
    Double-check that we're back at the end-of-central-directory record, and
    print quick summary of results, if we were just testing the archive.  We
    send the summary to stdout so that people doing the testing in the back-
    ground and redirecting to a file can just do a "tail" on the output file.
  ---------------------------------------------------------------------------*/

#ifndef SFX
    if (no_endsig_found) {                      /* just to make sure */
        Info(slide, 0x401, ((char *)slide, LoadFarString(EndSigMsg)));
        Info(slide, 0x401, ((char *)slide, LoadFarString(ReportMsg)));
        if (!error_in_archive)       /* don't overwrite stronger error */
            error_in_archive = PK_WARN;
    }

    if (enddigsig_len >= 0)
    {
      Info( slide, 0x401, ((char *)slide, LoadFarString( DigSigMsg),
       enddigsig_len));
# if 0   /* Enable to make this a warning. */
      if (error_in_archive < PK_WARN)           /* Keep more severe error. */
        error_in_archive = PK_WARN;
# endif /* 0 */
    }
#endif /* ndef SFX */

    error_in_archive = extract_test_trailer( __G__ filnum,
     num_bad_pwd, num_skipped, error_in_archive);

    return error_in_archive;

} /* extract_or_test_files(). */


#ifndef SFX
/*******************************/
/*  Function find_compr_idx()  */
/*******************************/

unsigned find_compr_idx(compr_methodnum)
    unsigned compr_methodnum;
{
   unsigned i;

   for (i = 0; i < NUM_METHODS; i++) {
      if (ComprIDs[i] == compr_methodnum) break;
   }
   return i;
} /* find_compr_idx(). */
#endif /* ndef SFX */


/***************************/
/*  Function memextract()  */
/***************************/

int memextract(__G__ tgt, tgtsize, src, srcsize)  /* extract compressed */
    __GDEF                                        /*  extra field block; */
    uch *tgt;                                     /*  return PK-type error */
    ulg tgtsize;                                  /*  level */
    ZCONST uch *src;
    ulg srcsize;
{
    zoff_t old_csize=G.csize;
    uch   *old_inptr=G.inptr;
    int    old_incnt=G.incnt;
    int    r;
    int    error = PK_OK;
    ush    method;
    ulg    extra_field_crc;


    method = makeword(src);
    extra_field_crc = makelong(src+2);

    /* compressed extra field exists completely in memory at this location: */
    G.inptr = (uch *)src + (2 + 4);     /* method and extra_field_crc */
    G.incnt = (int)(G.csize = (long)(srcsize - (2 + 4)));
    G.mem_mode = TRUE;
    G.outbufptr = tgt;
    G.outsize = tgtsize;

    switch (method) {
        case STORED:
            memcpy((char *)tgt, (char *)G.inptr, (extent)G.incnt);
            G.outcnt = (ulg)G.csize;    /* for CRC calculation */
            break;

#ifdef DEFLATE_SUPPORT
        case DEFLATED:
# ifdef DEFLATE64_SUPPORT
        case ENHDEFLATED:
# endif
            G.outcnt = 0L;
            if ((r = UZinflate(__G__ (method == ENHDEFLATED))) != 0) {
                if (!uO.tflag)
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                      LoadFarString(NotEnoughMem) :
                      LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Inflate)));
                error = (r == 3)? PK_MEM3 : PK_ERR;
            }
            if (G.outcnt == 0L)   /* inflate's final FLUSH sets outcnt */
                break;
            break;
#endif
        default:
            if (uO.tflag)
                error = PK_ERR | ((int)method << 8);
            else {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(UnsupportedExtraField), method));
                error = PK_ERR;  /* GRR:  should be passed on up via SetEAs() */
            }
            break;
    }

    G.inptr = old_inptr;
    G.incnt = old_incnt;
    G.csize = old_csize;
    G.mem_mode = FALSE;

    if (!error) {
        register ulg crcval = crc32(CRCVAL_INITIAL, tgt, (extent)G.outcnt);

        if (crcval != extra_field_crc) {
            if (uO.tflag)
                error = PK_ERR | (DEFLATED << 8);  /* kludge for now */
            else {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(BadExtraFieldCRC), G.zipfn, crcval,
                  extra_field_crc));
                error = PK_ERR;
            }
        }
    }
    return error;

} /* memextract(). */


/*************************/
/*  Function memflush()  */
/*************************/

int memflush(__G__ rawbuf, size)
    __GDEF
    ZCONST uch *rawbuf;
    ulg size;
{
    if (size > G.outsize)
        /* Here, PK_DISK is a bit off-topic, but in the sense of marking
           "overflow of output space", its use may be tolerated. */
        return PK_DISK;   /* more data than output buffer can hold */

    memcpy((char *)G.outbufptr, (char *)rawbuf, (extent)size);
    G.outbufptr += (unsigned int)size;
    G.outsize -= size;
    G.outcnt += size;

    return PK_OK;

} /* memflush(). */


#if defined(VMS) || defined(VMS_TEXT_CONV)

/********************************/
/*  Function decompress_bits()  */
/********************************/
/*
 *  Simple uncompression routine. The compression uses bit stream.
 *  Compression scheme:
 *
 *  if (byte!=0)
 *      putbit(1),putbyte(byte)
 *  else
 *      putbit(0)
 */
static void decompress_bits(outptr, needlen, bitptr)
    uch *outptr;        /* Pointer into output block */
    unsigned needlen;   /* Size of uncompressed block */
    ZCONST uch *bitptr; /* Pointer into compressed data */
{
    ulg bitbuf = 0;
    int bitcnt = 0;

#define _FILL   {       bitbuf |= (*bitptr++) << bitcnt;\
                        bitcnt += 8;                    \
                }

    while (needlen--)
    {
        if (bitcnt <= 0)
            _FILL;

        if (bitbuf & 1)
        {
            bitbuf >>= 1;
            if ((bitcnt -= 1) < 8)
                _FILL;
            *outptr++ = (uch)bitbuf;
            bitcnt -= 8;
            bitbuf >>= 8;
        }
        else
        {
            *outptr++ = '\0';
            bitcnt -= 1;
            bitbuf >>= 1;
        }
    }
} /* decompress_bits(). */


/************************************/
/*  Function extract_izvms_block()  */
/************************************/

/*
 * Extracts block from p. If resulting length is less than needed, fill
 * extra space with corresponding bytes from 'init'.
 * Currently understands 3 formats of block compression:
 * - Simple storing
 * - Compression of zero bytes to zero bits
 * - Deflation (see memextract())
 * The IZVMS block data is returned in malloc'd space.
 */
uch *extract_izvms_block(__G__ ebdata, size, retlen, init, needlen)
    __GDEF
    ZCONST uch *ebdata;
    unsigned size;
    unsigned *retlen;
    ZCONST uch *init;
    unsigned needlen;
{
    uch *ucdata;       /* Pointer to block allocated */
    int cmptype;
    unsigned usiz, csiz;

    cmptype = (makeword(ebdata+EB_IZVMS_FLGS) & EB_IZVMS_BCMASK);
    csiz = size - EB_IZVMS_HLEN;
    usiz = (cmptype == EB_IZVMS_BCSTOR ?
            csiz : makeword(ebdata+EB_IZVMS_UCSIZ));

    if (retlen)
        *retlen = usiz;

    if ((ucdata = (uch *)izu_malloc(IZ_MAX(needlen, usiz))) == NULL)
        return NULL;

    if (init && (usiz < needlen))
        memcpy((char *)ucdata, (ZCONST char *)init, needlen);

    switch (cmptype)
    {
        case EB_IZVMS_BCSTOR: /* The simplest case */
            memcpy(ucdata, ebdata+EB_IZVMS_HLEN, usiz);
            break;
        case EB_IZVMS_BC00:
            decompress_bits(ucdata, usiz, ebdata+EB_IZVMS_HLEN);
            break;
        case EB_IZVMS_BCDEFL:
            memextract(__G__ ucdata, (ulg)usiz,
                       ebdata+EB_IZVMS_HLEN, (ulg)csiz);
            break;
        default:
            izu_free(ucdata);
            ucdata = NULL;
    }
    return ucdata;

} /* extract_izvms_block(). */

#endif /* defined(VMS) || defined(VMS_TEXT_CONV) */


/*
 * If Unicode is supported, assume we have what we need to do this
 * check using wide characters, avoiding MBCS issues.
 */

#ifndef UZ_FNFILTER_REPLACECHAR
    /* A convenient choice for the replacement of unprintable char codes is
     * the "single char wildcard", as this character is quite unlikely to
     * appear in filenames by itself.  The following default definition
     * sets the replacement char to a question mark as the most common
     * "single char wildcard"; this setting should be overridden in the
     * appropiate system-specific configuration header when needed.
     */
# define UZ_FNFILTER_REPLACECHAR      '?'
#endif


/*************************/
/*  Function fnfilter()  */     /* (Here instead of in list.c for SFX.) */
/*************************/

/* fnfilter() - Convert name to safely printable form. */

char *fnfilter(raw, space, size)
    ZCONST char *raw;
    uch *space;
    extent size;
{

#ifndef NATIVE          /* ASCII:  filter ANSI escape codes, etc. */

    ZCONST uch *r;
    uch *s = space;
    uch *slim = NULL;
    uch *se = NULL;
    int have_overflow = FALSE;

# if defined( UNICODE_SUPPORT) && defined( _MBCS)
/* If Unicode support is enabled, and we have multi-byte characters,
 * then do the isprint() checks by first converting to wide characters
 * and checking those.  This avoids our having to parse multi-byte
 * characters for ourselves.  After the wide-char replacements have been
 * made, the wide string is converted back to the local character set.
 */
    wchar_t *wstring;    /* wchar_t version of raw */
    size_t wslen;        /* length of wstring */
    wchar_t *wostring;   /* wchar_t version of output string */
    size_t woslen;       /* length of wostring */
    char *newraw;        /* new raw */

    /* 2012-11-06 SMS.
     * Changed to check the value returned by mbstowcs(), and bypass the
     * Unicode processing if it fails.  This seems to fix a problem
     * reported in the SourceForge forum, but it's not clear that we
     * should be doing any Unicode processing without some evidence that
     * the name actually is Unicode.  (Check bit 11 in the flags before
     * coming here?)
     * http://sourceforge.net/p/infozip/bugs/40/
     */

    if (MB_CUR_MAX <= 1)
    {
        /* There's no point to converting multi-byte chars if there are
         * no multi-byte chars.
         */
        wslen = (size_t)-1;
    }
    else
    {
        /* Get Unicode wide character count (for storage allocation). */
        wslen = mbstowcs( NULL, raw, 0);
    }

    if (wslen != (size_t)-1)
    {
        /* Apparently valid Unicode.  Allocate wide-char storage. */
        wstring = (wchar_t *)izu_malloc((wslen + 1) * sizeof(wchar_t));
        if (wstring == NULL) {
            strcpy( (char *)space, raw);
            return (char *)space;
        }
        wostring = (wchar_t *)izu_malloc(2 * (wslen + 1) * sizeof(wchar_t));
        if (wostring == NULL) {
            izu_free(wstring);
            strcpy( (char *)space, raw);
            return (char *)space;
        }

        /* Convert the multi-byte Unicode to wide chars. */
        wslen = mbstowcs(wstring, raw, wslen + 1);

        /* Filter the wide-character string. */
        fnfilterw( wstring, wostring, (2 * (wslen + 1) * sizeof(wchar_t)));

        /* Convert filtered wide chars back to multi-byte. */
        woslen = wcstombs( NULL, wostring, 0);
        if ((newraw = izu_malloc(woslen + 1)) == NULL) {
            izu_free(wstring);
            izu_free(wostring);
            strcpy( (char *)space, raw);
            return (char *)space;
        }
        woslen = wcstombs( newraw, wostring, (woslen * MB_CUR_MAX) + 1);

        if (size > 0) {
            slim = space + size - 4;
        }
        r = (ZCONST uch *)newraw;
        while (*r) {
            if (size > 0 && s >= slim && se == NULL) {
                se = s;
            }
#  ifdef QDOS
            if (qlflag & 2) {
                if (*r == '/' || *r == '.') {
                    if (se != NULL && (s > (space + (size-3)))) {
                        have_overflow = TRUE;
                        break;
                    }
                    ++r;
                    *s++ = '_';
                    continue;
                }
            } else
#  endif
            {
                if (se != NULL && (s > (space + (size-3)))) {
                    have_overflow = TRUE;
                    break;
                }
                *s++ = *r++;
            }
        }
        if (have_overflow) {
            strcpy((char *)se, "...");
        } else {
            *s = '\0';
        }

        izu_free(wstring);
        izu_free(wostring);
        izu_free(newraw);
    }
    else
# endif /* defined( UNICODE_SUPPORT) && defined( _MBCS) */
    {
        /* No Unicode support, or apparently invalid Unicode. */
        r = (ZCONST uch *)raw;

        if (size > 0) {
            slim = space + size
# ifdef _MBCS
                         - (MB_CUR_MAX - 1)
# endif
                         - 4;
        }
        while (*r) {
            if (size > 0 && s >= slim && se == NULL) {
                se = s;
            }
# ifdef QDOS
            if (qlflag & 2) {
                if (*r == '/' || *r == '.') {
                    if (se != NULL && (s > (space + (size-3)))) {
                        have_overflow = TRUE;
                        break;
                    }
                    ++r;
                    *s++ = '_';
                    continue;
                }
            } else
# endif
# ifdef HAVE_WORKING_ISPRINT
            if (!isprint(*r)) {
                if (*r < 32) {
                    /* ASCII control codes are escaped as "^{letter}". */
                    if (se != NULL && (s > (space + (size-4)))) {
                        have_overflow = TRUE;
                        break;
                    }
                    *s++ = '^', *s++ = (uch)(64 + *r++);
                } else {
                    /* Other unprintable codes are replaced by the
                     * placeholder character. */
                    if (se != NULL && (s > (space + (size-3)))) {
                        have_overflow = TRUE;
                        break;
                    }
                    *s++ = UZ_FNFILTER_REPLACECHAR;
                    INCSTR(r);
                }
# else /* def HAVE_WORKING_ISPRINT */
            if (*r < 32) {
                /* ASCII control codes are escaped as "^{letter}". */
                if (se != NULL && (s > (space + (size-4)))) {
                    have_overflow = TRUE;
                    break;
                }
                *s++ = '^', *s++ = (uch)(64 + *r++);
# endif /* def HAVE_WORKING_ISPRINT [else] */
            } else {
# ifdef _MBCS
                extent i = CLEN(r);
                if (se != NULL && (s > (space + (size-i-2)))) {
                    have_overflow = TRUE;
                    break;
                }
                for (; i > 0; i--)
                    *s++ = *r++;
# else /* def _MBCS */
                if (se != NULL && (s > (space + (size-3)))) {
                    have_overflow = TRUE;
                    break;
                }
                *s++ = *r++;
# endif /* def _MBCS [else] */
             }
        }
        if (have_overflow) {
            strcpy((char *)se, "...");
        } else {
            *s = '\0';
        }
    }

# ifdef WINDLL
    INTERN_TO_ISO((char *)space, (char *)space);  /* translate to ANSI */
# else /* def WINDLL */
#  if defined(WIN32) && !defined(_WIN32_WCE)
    /* Win9x console always uses OEM character coding, and
       WinNT console is set to OEM charset by default, too */
    INTERN_TO_OEM((char *)space, (char *)space);
#  endif /* defined(WIN32) && !defined(_WIN32_WCE) */
# endif /* def WINDLL [else] */

    return (char *)space;

#else /* def NATIVE */  /* EBCDIC or whatever. */
    return (char *)raw;
#endif /* def NATIVE [else] */

} /* fnfilter(). */


#if defined( UNICODE_SUPPORT) && defined( _MBCS)

/****************************/
/*  Function fnfilter[w]()  */  /* (Here instead of in list.c for SFX.) */
/****************************/

/* fnfilterw() - Convert wide name to safely printable form. */

/* fnfilterw() - Convert wide-character name to safely printable form. */

wchar_t *fnfilterw( src, dst, siz)
    ZCONST wchar_t *src;        /* Pointer to source char (string). */
    wchar_t *dst;               /* Pointer to destination char (string). */
    extent siz;                 /* Not used (!). */
{
    wchar_t *dsx = dst;

    /* Filter the wide chars. */
    while (*src)
    {
        if (iswprint( *src))
        {
            /* Printable code.  Copy it. */
            *dst++ = *src;
        }
        else
        {
            /* Unprintable code.  Substitute something printable for it. */
            if (*src < 32)
            {
                /* Replace ASCII control code with "^{letter}". */
                *dst++ = (wchar_t)'^';
                *dst++ = (wchar_t)(64 + *src);
            }
            else
            {
                /* Replace other unprintable code with the placeholder. */
                *dst++ = (wchar_t)UZ_FNFILTER_REPLACECHAR;
            }
        }
        src++;
    }
    *dst = (wchar_t)0;  /* NUL-terminate the destination string. */
    return dsx;
} /* fnfilterw(). */

#endif /* defined( UNICODE_SUPPORT) && defined( _MBCS) */


#ifdef BZIP2_SUPPORT

/**************************/
/*  Function UZbunzip2()  */
/**************************/

static int UZbunzip2(__G)
    __GDEF
/* decompress a bzipped entry using the libbz2 routines */
{				
    int retval = PK_OK;         /*  Return PK-type error code. */
    int err = BZ_OK;
    int repeated_buf_err;
    bz_stream bstrm;

    /* Data sanity check.  (Avoid infinite loop.) */
    if ((G.incnt <= 0) && (G.csize <= 0))
    {
        Trace(( stderr, "UZbunzip2(): Data sanity failure.\n"));
        return PK_WARN;
    }

#if defined(DLL) && !defined(NO_SLIDE_REDIR)
    if (G.redirect_slide)
        wsize = G.redirect_size, redirSlide = G.redirect_buffer;
    else
        wsize = WSIZE, redirSlide = slide;
#endif

    bstrm.next_out = (char *)redirSlide;
    bstrm.avail_out = wsize;

    bstrm.next_in = (char *)G.inptr;
    bstrm.avail_in = G.incnt;

    /* local buffer for efficiency */
    /* $TODO Check for BZIP LIB version? */

    bstrm.bzalloc = NULL;
    bstrm.bzfree = NULL;
    bstrm.opaque = NULL;

    Trace((stderr, "initializing bzlib()\n"));
    err = BZ2_bzDecompressInit(&bstrm, 0, 0);

    if (err == BZ_MEM_ERROR)
        return PK_MEM;
#ifdef Tracing  /* 2012-12-03 SMS.  Avoid complaints about empty if(). */
    else if (err != BZ_OK)
        Trace((stderr, "oops!  (BZ2_bzDecompressInit() err = %d)\n", err));
#endif /* def Tracing */

#ifdef FUNZIP
    while (err != BZ_STREAM_END) {
#else /* def FUNZIP */
    while (G.csize > 0) {
        Trace((stderr, "first loop:  G.csize = %s\n",
         FmZofft( G.csize, NULL, "u")));
#endif /* def FUNZIP [else] */
        while (bstrm.avail_out > 0) {
            err = BZ2_bzDecompress(&bstrm);

            if (err == BZ_DATA_ERROR) {
                retval = PK_ERR; goto uzbunzip_cleanup_exit;
            } else if (err == BZ_MEM_ERROR) {
                retval = PK_MEM3; goto uzbunzip_cleanup_exit;
            }
#ifdef Tracing  /* 2012-12-03 SMS.  Avoid complaints about empty if(). */
            else if ((err != BZ_OK) && (err != BZ_STREAM_END))
                Trace((stderr, "oops!  (bzip(first loop) err = %d)\n", err));
#endif /* def Tracing */

#ifdef FUNZIP
            if (err == BZ_STREAM_END)   /* "END-of-entry-condition" ? */
#else /* def FUNZIP */
            if (G.csize <= 0L)          /* "END-of-entry-condition" ? */
#endif /* def FUNZIP [else] */
                break;

            if (bstrm.avail_in == 0) {
                if (fillinbuf(__G) == 0) {
                    /* no "END-condition" yet, but no more data */
                    retval = PK_ERR; goto uzbunzip_cleanup_exit;
                }
                bstrm.next_in = (char *)G.inptr;
                bstrm.avail_in = G.incnt;
            }
            Trace((stderr, "     avail_in = %u\n", bstrm.avail_in));
        }
        /* flush slide[] */
        if ((retval = FLUSH(wsize - bstrm.avail_out)) != 0)
            goto uzbunzip_cleanup_exit;
        Trace((stderr, "inside loop:  flushing %ld bytes (ptr diff = %ld)\n",
          (long)(wsize - bstrm.avail_out),
          (long)(bstrm.next_out-(char *)redirSlide)));
        bstrm.next_out = (char *)redirSlide;
        bstrm.avail_out = wsize;
    }

    /* no more input, so loop until we have all output */
    Trace((stderr, "beginning final loop:  err = %d\n", err));
    repeated_buf_err = FALSE;
    while (err != BZ_STREAM_END) {
        err = BZ2_bzDecompress(&bstrm);
        if (err == BZ_DATA_ERROR) {
            retval = PK_ERR; goto uzbunzip_cleanup_exit;
        } else if (err == BZ_MEM_ERROR) {
            retval = PK_MEM3; goto uzbunzip_cleanup_exit;
        } else if (err != BZ_OK && err != BZ_STREAM_END) {
            Trace((stderr, "oops!  (bzip(final loop) err = %d)\n", err));
            DESTROYGLOBALS();
            EXIT(PK_MEM3);
        }
        /* final flush of slide[] */
        if ((retval = FLUSH(wsize - bstrm.avail_out)) != 0)
            goto uzbunzip_cleanup_exit;
        Trace((stderr, "final loop:  flushing %ld bytes (ptr diff = %ld)\n",
          (long)(wsize - bstrm.avail_out),
          (long)(bstrm.next_out-(char *)redirSlide)));
        bstrm.next_out = (char *)redirSlide;
        bstrm.avail_out = wsize;
    }
#ifdef LARGE_FILE_SUPPORT
    Trace((stderr, "total in = %llu, total out = %llu\n",
      (zusz_t)(bstrm.total_in_lo32) + ((zusz_t)(bstrm.total_in_hi32))<<32,
      (zusz_t)(bstrm.total_out_lo32) + ((zusz_t)(bstrm.total_out_hi32))<<32));
#else
    Trace((stderr, "total in = %lu, total out = %lu\n", bstrm.total_in_lo32,
      bstrm.total_out_lo32));
#endif

    G.inptr = (uch *)bstrm.next_in;
    G.incnt = bstrm.avail_in;

uzbunzip_cleanup_exit:
    err = BZ2_bzDecompressEnd(&bstrm);
#ifdef Tracing  /* 2012-12-03 SMS.  Avoid complaints about empty if(). */
    if (err != BZ_OK)
        Trace((stderr, "oops!  (BZ2_bzDecompressEnd() err = %d)\n", err));
#endif /* def Tracing */

    return retval;
} /* UZbunzip2(). */
#endif /* BZIP2_SUPPORT */


#if defined( LZMA_SUPPORT) || defined( PPMD_SUPPORT)

#include "szip/Types.h"

/* 2011-12-24  SMS.
 * 7-ZIP offers memory allocation functions with diagnostics conditional
 * on _SZ_ALLOC_DEBUG: szip/Alloc.c: MyAlloc(), MyFree().  Using these
 * functions complicates linking with separately conditional LZMA and
 * PPMd support, so it's easier to use plain malloc() and free() here,
 * or else add the diagnostic messages to these Sz* functions, rather
 * than drag szip/Alloc.c into the picture.  To use the szip/Alloc.c
 * functions, add
 *    #include "szip/Alloc.h"
 * above, change malloc() and free() below to MyAlloc() and MyFree(),
 * and add szip/Alloc.* back to the builders.  (And then solve the other
 * problems.)
 */
void *SzAlloc(void *p, size_t size) { p = p; return izu_malloc(size); }
void SzFree(void *p, void *address) { p = p; izu_free(address); }
#endif /* defined( LZMA_SUPPORT) || defined( PPMD_SUPPORT) */


#ifdef LZMA_SUPPORT

#include "szip/LzmaDec.h"

/***********************/
/*  Function UZlzma()  */
/***********************/

/* Notes:
 * Capitalized types (like "SRes") or "SZ_*" macros (like SZ_OK) are
 * probably defined in a 7-ZIP header file.
 */

static int UZlzma(__G)
    __GDEF
/* Decompress an LZMA-compressed entry using the LZMA routines. */
{
    SRes sts;
    ELzmaStatus sts2;
    ELzmaFinishMode finishMode = LZMA_FINISH_ANY;

    SizeT avail_in;
    SizeT avail_out;
    unsigned char *next_in;
    unsigned char *next_out;

    /* Bidirectional arguments for LzmaDec_DecodeToBuf(). */
    SizeT in_buf_size_len;      /* Buffer size (in), buffer used (out). */
    SizeT out_buf_size_len;     /* Buffer size (in), buffer filled (out). */

    /* LZMA Header. */
    unsigned char lzma_version_major;           /* LZMA version (major). */
    unsigned char lzma_version_minor;           /* LZMA version (minor). */
    unsigned short lzma_props_len;              /* Properties length. */
    unsigned char lzma_props[ LZMA_PROPS_SIZE]; /* LMZA properties. */
    int b;

    zusz_t ucsize_lzma;         /* LZMA uncompressed bytes left to put out. */

    /* Initialize 7-Zip (LZMA, PPMd) memory allocation function pointer
     * structure (once).
     */
    if ((G.g_Alloc.Alloc == NULL) || (G.g_Alloc.Free == NULL))
    {
        G.g_Alloc.Alloc = SzAlloc;
        G.g_Alloc.Free = SzFree;
    }

    /* Uncompressed bytes to put out. */
    ucsize_lzma = G.lrec.ucsize;

    /* Extract LZMA version (curiosity) and properties length (crucial). */
    if ((b = NEXTBYTE) == EOF)
        return PK_ERR;
    lzma_version_major = b;
    if ((b = NEXTBYTE) == EOF)
        return PK_ERR;
    lzma_version_minor = b;
    if ((b = NEXTBYTE) == EOF)
        return PK_ERR;
    lzma_props[ 0] = b;
    if ((b = NEXTBYTE) == EOF)
        return PK_ERR;
    lzma_props[ 1] = b;
    lzma_props_len = makeword( lzma_props);

    /* LZMA properties length must match, or we're in big trouble. */
    if (lzma_props_len != LZMA_PROPS_SIZE)
        return PK_ERR;

    /* Save the actual LZMA properties. */
    for (lzma_props_len = 0; lzma_props_len < LZMA_PROPS_SIZE; lzma_props_len++)
    {
        if ((b = NEXTBYTE) == EOF)
            break;
        lzma_props[ lzma_props_len] = b;
    }
    if (lzma_props_len != LZMA_PROPS_SIZE)
        return PK_ERR;

    sts = LzmaProps_Decode( &G.clzma_props, lzma_props, LZMA_PROPS_SIZE);

    Trace(( stderr,
     "LzmaProps_Decode() = %d, dicSize = %u, lc = %u, lp = %u, pb = %u.\n",
     sts, G.clzma_props.dicSize, G.clzma_props.lc,
     G.clzma_props.lp, G.clzma_props.pb));

    /* Verbose test (-tv) information. */
    if (uO.tflag && uO.vflag)
    {
        Info( slide, 0, ((char *)slide, LoadFarString( InfoMsgLZMA),
         G.clzma_props.dicSize, G.clzma_props.lc,
         G.clzma_props.lp, G.clzma_props.pb));
    }

    /* Require valid LZMA properties. */
    if (sts != SZ_OK)
        return PK_ERR;

    /* Set up LZMA decode. */
    LzmaDec_Construct( &G.state_lzma);
    sts = LzmaDec_Allocate( &G.state_lzma, lzma_props,
     LZMA_PROPS_SIZE, &G.g_Alloc);

    if (sts != SZ_OK)
    {
        return PK_MEM3;
    }

    LzmaDec_Init( &G.state_lzma);

#if defined(DLL) && !defined(NO_SLIDE_REDIR)
    if (G.redirect_slide)
        wsize = G.redirect_size, redirSlide = G.redirect_buffer;
    else
        wsize = WSIZE, redirSlide = slide;
#endif

    next_out = redirSlide;
    avail_out = wsize;
    next_in = G.inptr;
    avail_in = G.incnt;
    sts2 = -1;

    while ((sts == SZ_OK) &&
     (sts2 != LZMA_STATUS_FINISHED_WITH_MARK) &&
     (sts2 != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK))
    {
        in_buf_size_len = avail_in;
        out_buf_size_len = avail_out;
        if (ucsize_lzma <= out_buf_size_len)
        {
            /* Expecting this to be the last decode operation. */
            finishMode = LZMA_FINISH_END;
            out_buf_size_len = (SizeT)ucsize_lzma;
        }

        sts = LzmaDec_DecodeToBuf( &G.state_lzma, next_out, &out_buf_size_len,
         next_in, &in_buf_size_len, finishMode, &sts2);
        avail_in -= in_buf_size_len;    /* Input unused. */
        avail_out -= out_buf_size_len;  /* Output unused. */

        /* Flush the output (slide[]). */
        if ((sts = FLUSH( wsize- avail_out)) != 0)
            goto uzlzma_cleanup_exit;
        Trace((stderr, "inside loop:  flushing %ld bytes (ptr diff = %ld)\n",
          (unsigned long)(wsize- avail_out),
          (unsigned long)(next_out- redirSlide)));

        /* Decrement bytes-left-to-put-out count. */
        ucsize_lzma -= (wsize- avail_out);

        next_out = redirSlide;
        avail_out = wsize;

        if ((sts2 != LZMA_STATUS_FINISHED_WITH_MARK) &&
         (sts2 != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK))
        {
            if (avail_in == 0)
            {
                if (fillinbuf(__G) == 0)
                {
                    /* No "END-condition" yet, but no more data. */
                    sts = PK_ERR; goto uzlzma_cleanup_exit;
                }

                next_in = G.inptr;
                avail_in = G.incnt;
            }
            else
            {
                next_in += in_buf_size_len;
            }
            Trace((stderr, "     avail_in = %lu\n", (unsigned long)avail_in));
        }
    } /* while ((G.csize > 0) || (G.incnt > 0)) */

uzlzma_cleanup_exit:

    LzmaDec_Free( &G.state_lzma, &G.g_Alloc);

    /* Advance the global input pointer to past the used data. */
    G.inptr = next_in+ in_buf_size_len;
    G.incnt = (int)avail_in;

    return sts;
} /* UZlzma(). */
#endif /* def LZMA_SUPPORT */


#ifdef PPMD_SUPPORT

#include "szip/Ppmd8.h"

/*******************************/
/*  Function ppmd_read_byte()  */
/*******************************/

/* 7-Zip-compatible I/O Read function. */
static Byte ppmd_read_byte( void *pp)
{
    int b;

    /* 2012-03-17 SMS.
     * Note that if REENTRANT and USETHREADID are defined (globals.h),
     * then GETGLOBALS() is actually a function call, to
     * getGlobalPointer().  It might be smarter to add a "G" pointer to
     * the CByteInToLook structure (done), set it once (done), and then
     * use it here (not done), instead of calling getGlobalPointer() for
     * every byte fetched.  (Add a "G" parameter to NEXTBYTE?)
     */
    CByteInToLook *p = (CByteInToLook *)pp;
    GETGLOBALS();

    b = NEXTBYTE;
    if (b == EOF)
    {
        p->extra = True;
        p->res = SZ_ERROR_INPUT_EOF;
        b = 0;
    }
    return b;
} /* ppmd_read_byte(). */


/***********************/
/*  Function UZppmd()  */
/***********************/

static int UZppmd(__G)
    __GDEF
/* Decompress a PPMd-compressed entry using the PPMd routines. */
{
    int sts;
    int sts2;

    unsigned avail_out;         /* Output buffer size. */
    unsigned char *next_out;    /* Output buffer pointer. */

    /* PPMd Header. */
    unsigned char ppmd_props[ 2];       /* PPMd properties. */
    unsigned short ppmd_prop_word;
    int b;

    /* PPMd parameters. */
    unsigned order;
    unsigned memSize;
    unsigned restor;

    /* Initialize 7-Zip (LZMA, PPMd) memory allocation function pointer
     * structure (once).
     */

    if ((G.g_Alloc.Alloc == NULL) || (G.g_Alloc.Free == NULL))
    {
        G.g_Alloc.Alloc = SzAlloc;
        G.g_Alloc.Free = SzFree;
    }

    /* Initialize PPMd8 structure (once). */
    if (G.ppmd_constructed == 0)
    {
        Ppmd8_Construct( &G.ppmd8);
        G.ppmd_constructed = 1;
    }

    /* Initialize simulated 7-Zip I/O structure. */
    G.szios.p.Read = ppmd_read_byte;
    G.szios.extra = False;
    G.szios.res = SZ_OK;
    G.szios.pG = &G;

    sts = 0;
    /* Extract PPMd properties. */
    if ((b = NEXTBYTE) == EOF)
        return PK_ERR;
    ppmd_props[ 0] = b;
    if ((b = NEXTBYTE) == EOF)
        return PK_ERR;
    ppmd_props[ 1] = b;
    ppmd_prop_word = makeword( ppmd_props);

/* wPPMd = (Model order - 1) +
 *         ((Sub-allocator size - 1) << 4) +
 *         (Model restoration method << 12)
 *
 *  15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
 *  Mdl_Res_Mth ___Sub-allocator_size-1 Mdl_Order-1
 */
    order = (ppmd_prop_word& 0xf)+ 1;
    memSize = ((ppmd_prop_word>> 4)& 0xff)+ 1;
    restor = (ppmd_prop_word>> 12);

    /* Verbose test (-tv) information. */
    if (uO.tflag && uO.vflag)
    {
        Info( slide, 0, ((char *)slide, LoadFarString( InfoMsgPPMd),
         ppmd_prop_word, order, memSize, restor));
    }

    /* Convert archive MB value into raw byte value. */
    memSize <<= 20;

    if ((order < PPMD8_MIN_ORDER) || (order > PPMD8_MAX_ORDER))
        return PK_ERR;

    if (!Ppmd8_Alloc( &G.ppmd8, memSize, &G.g_Alloc))
        return PK_MEM3;

    G.ppmd8.Stream.In = &G.szios.p;

    sts = Ppmd8_RangeDec_Init( &G.ppmd8);
    if (!sts)
        return PK_ERR;
    else if (G.szios.extra)
      sts = ((G.szios.res != SZ_OK) ? G.szios.res : SZ_ERROR_DATA);
    else
    {
        int sym;

        Ppmd8_Init( &G.ppmd8, order, restor);

#if defined(DLL) && !defined(NO_SLIDE_REDIR)
        if (G.redirect_slide) {
            wsize = G.redirect_size; redirSlide = G.redirect_buffer;
        } else {
            wsize = WSIZE; redirSlide = slide;
        }
#endif

        sym = 0;
        while ((sym >= 0) && (G.szios.extra == 0))
        {
            /* Reset output buffer pointer. */
            next_out = redirSlide;

            /* Decode input to fill the output buffer. */
            for (avail_out = wsize; avail_out > 0; avail_out--)
            {
                sym = Ppmd8_DecodeSymbol( &G.ppmd8);
                if (G.szios.extra || sym < 0)
                    break;
                *(next_out++) = sym;
            }

            /* Flush the output (slide[]). */
            if ((sts = FLUSH( wsize- avail_out)) != 0)
                goto uzppmd_cleanup_exit;
            Trace((stderr,
             "inside loop:  flushing %ld bytes (ptr diff = %ld)\n",
             (unsigned long)(wsize- avail_out),
             (unsigned long)(next_out- redirSlide)));
        }

        if (G.szios.extra)
        {
            /* Insufficient input data. */
            sts = PK_ERR;
        }
        else if ((sym < 0) && (sym != -1))
        {
            /* Invalid end of input data? */
            sts = PK_ERR;
        }
        else
        {
            sts2 = Ppmd8_RangeDec_IsFinishedOK( &G.ppmd8);
            if (sts2 == 0)
            {
                sts = PK_ERR;
            }
        }
    }

uzppmd_cleanup_exit:

    Ppmd8_Free( &G.ppmd8, &G.g_Alloc);

    return sts;
} /* UZppmd(). */
#endif /* def PPMD_SUPPORT */
