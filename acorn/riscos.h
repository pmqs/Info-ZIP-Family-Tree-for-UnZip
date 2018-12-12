/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* riscos.h */

#ifndef __RISCOS_H
# define __RISCOS_H

# include <time.h>

# ifndef __swiven_h
#  include "swiven.h"
# endif

# define MAXPATHLEN 1024
# define MAXFILENAMELEN MAXPATHLEN  /* RISC OS 4 has 1024 limit. 1024 is also the same as FNMAX in zip.h */
# define DIR_BUFSIZE MAXPATHLEN   /* Ensure we can read at least one full-length RISC OS 4 filename */

/* ISO/OEM (iconv) character conversion. */

# ifdef ICONV_MAPPING

# ifndef MAX_CP_NAME
#  define MAX_CP_NAME 25        /* Should be in a header file? */
# endif

#  ifdef SETLOCALE
#   undef SETLOCALE
#  endif
#  define SETLOCALE(category, locale) setlocale(category, locale)

#  include <locale.h>

#  ifdef _ISO_INTERN
#   undef _ISO_INTERN
#  endif
#  define _ISO_INTERN( string) charset_to_intern( string, G.iso_cp)

/* Possible "const" type qualifier for arg 2 of iconv(). */
#  ifndef ICONV_ARG2
#   define ICONV_ARG2
#  endif /* ndef ICONV_ARG2 */

# endif /* def ICONV_MAPPING */


struct stat {
  unsigned int st_dev;
  int st_ino;
  unsigned int st_mode;
  int st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
  unsigned int st_rdev;
  unsigned int st_size;
  unsigned int st_blksize;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

typedef struct {
  char *dirname;
  void *buf;
  int size;
  char *act;
  int offset;
  int read;
} DIR;


struct dirent {
  unsigned int d_off;          /* offset of next disk directory entry */
  int d_fileno;                /* file number of entry */
  size_t d_reclen;             /* length of this record */
  size_t d_namlen;             /* length of d_name */
  char d_name[MAXFILENAMELEN]; /* name */
};

typedef struct {
  short         ID;
  short         size;
  int           ID_2;
  unsigned int  loadaddr;
  unsigned int  execaddr;
  int           attr;
  int           zero;
} extra_block;


# define S_IFMT  0770000

# define S_IFDIR 0040000
# define S_IFREG 0100000  /* 0200000 in UnixLib !?!?!?!? */

# ifndef S_IEXEC
#  define S_IEXEC  0000100
#  define S_IWRITE 0000200
#  define S_IREAD  0000400
# endif

# ifndef NO_UNZIPH_STUFF
#  include <time.h>
#  if (!defined(HAVE_STRNICMP) & !defined(NO_STRNICMP))
#   define NO_STRNICMP
#  endif
#  ifndef DATE_FORMAT
#   define DATE_FORMAT DF_DMY
#  endif
#  define lenEOL 1
#  define PutNativeEOL  *q++ = native(LF);
#  define USE_STRM_INPUT
#  define USE_FWRITE
#  define PIPE_ERROR (errno == 9999)    /* always false */
#  define isatty(x) (TRUE)   /* used in funzip.c to find if stdin redirected:
     should find a better way, now just work as if stdin never redirected */
#  define USE_EF_UT_TIME
#  if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))
#   define TIMESTAMP
#  endif
#  define localtime riscos_localtime
#  define gmtime riscos_gmtime
# endif /* !NO_UNZIPH_STUFF */
# define SET_DIR_ATTRIB

# define NO_EXCEPT_SIGNALS

# define _raw_getc() SWI_OS_ReadC()

extern char *exts2swap; /* Extensions to swap */

#ifndef __GCC__

int stat(char *filename,struct stat *res);
DIR *opendir(char *dirname);
struct dirent *readdir(DIR *d);
void closedir(DIR *d);
int unlink(char *f);
int rmdir(char *d);
int chmod(char *file, int mode);

#endif /* ndef __GCC__ */

void setfiletype(char *fname,int ftype);
void getRISCOSexts(char *envstr);
int checkext(char *suff);
void swapext(char *name, char *exptr);
void remove_prefix(void);
void set_prefix(void);
struct tm *riscos_localtime(const time_t *timer);
struct tm *riscos_gmtime(const time_t *timer);

#endif /* ndef __RISCOS_H */
