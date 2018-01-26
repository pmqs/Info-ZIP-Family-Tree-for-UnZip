/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  iconv_map.c

  iconv interface functions Info-ZIP UnZip.

  Contains:  init_conversion_charsets()
             charset_to_intern()

  ---------------------------------------------------------------------------*/

#define UNZIP_INTERNAL
#include "unzip.h"

#ifdef ICONV_MAPPING

/* ISO/OEM (iconv) character conversion. */

# include <iconv.h>
# include <langinfo.h>

typedef struct {
/*  char *local_charset; */
    char *local_lang;
    char *archive_charset;
} CHARSET_MAP;

/* Was:  A mapping of local <-> archive charsets used by default to convert
 *  filenames of DOS/Windows Zip archives.  Currently very basic.
 */
/* Now:  A mapping of environment language <-> archive charsets used by default
 * to convert filenames of DOS/Windows Zip archives.  Currently incomplete.
 */

/*
 * static CHARSET_MAP dos_charset_map[] = {
 *     { "ANSI_X3.4-1968", "CP850" },
 *     { "ISO-8859-1", "CP850" },
 *     { "CP1252", "CP850" },
 *     { "UTF-8", "CP866" },
 *     { "KOI8-R", "CP866" },
 *     { "KOI8-U", "CP866" },
 *     { "ISO-8859-5", "CP866" }
 * };
 */

static CHARSET_MAP dos_charset_map[] = {
    { "C", "CP850" },
    { "en", "CP850" },
    /*  a lot of latin1 not included, by default it will be "CP850"  */
    { "bs", "CP852" },
    { "cs", "CP852" },
    { "hr", "CP852" },
    { "hsb", "CP852" },
    { "hu", "CP852" },
    { "pl", "CP852" },
    { "ro", "CP852" },
    { "sk", "CP852" },
    { "sl", "CP852" },
    { "ru", "CP866" },
    { "be", "CP866" },
    { "bg", "CP866" },
    { "mk", "CP866" },
    { "uk", "CP866" },
    { "ar", "CP864" },
    { "el", "CP869" },
    { "he", "CP862" },
    { "iw", "CP862" },
    { "ku", "CP857" },
    { "tr", "CP857" },
    { "zh", "CP950" },  /*  CP936   */
    { "ja", "CP932" },
    { "ko", "CP949" },
    { "th", "CP874" },
    { "da", "CP865" },
    { "nb", "CP865" },
    { "nn", "CP865" },
    { "no", "CP865" },
    { "is", "CP861" },
    { "lt", "CP775" },
    { "lv", "CP775" },
};


/* Try to guess the default value of G.oem_cp based on the current locale.
 * G.iso_cp is left alone for now.
 */
void init_conversion_charsets( __G)
 __GDEF
{
/*
 *  const char *local_charset;
 *  int i;
 */
    char *locale;
    char *loc = NULL;

    /* Make a guess only if G.oem_cp is not already set. */
/*
 *  if(*OEM_CP == '\0') {
 *      local_charset = nl_langinfo(CODESET);
 *      for (i = 0; i < sizeof(dos_charset_map)/sizeof(CHARSET_MAP); i++)
 *          if (!strcasecmp(local_charset, dos_charset_map[i].local_charset)) {
 *              strncpy(OEM_CP, dos_charset_map[i].archive_charset,
 *               sizeof(OEM_CP));
 *              break;
 *          }
 *  }
 */

    if (*G.oem_cp != '\0')
        return;

    locale = getenv("LC_ALL");
    if (!locale)
        locale = getenv("LANG");

    if (locale && (loc = izu_malloc(strlen(locale) + 1)) != NULL)
    {
        char *p;
        int i;

        strcpy(loc, locale);

        /* Extract language part. */
        p = strchr(loc, '.');
        if (p)
            *p = '\0';

        /* Extract main language part. */
        p = strchr(loc, '_');
        if (p)
            *p = '\0';

        for (i = 0; i < sizeof(dos_charset_map)/sizeof(CHARSET_MAP); i++)
        {
            if (!strcmp(loc, dos_charset_map[i].local_lang))
            {
                strncpy( G.oem_cp, dos_charset_map[i].archive_charset,
                 sizeof( G.oem_cp));
                break;
            }
        }
        /* 2012-11-27 SMS. */
        izu_free( loc);
    }

    if (*G.oem_cp == '\0')      /* Set default one. */
        strncpy( G.oem_cp, "CP850", sizeof( G.oem_cp));
}

/* Convert a string from one encoding to the current locale using iconv().
 * Be as non-intrusive as possible.  If error is encountered during
 * conversion, just leave the string intact.
 */
void charset_to_intern(char *string, char *from_charset)
{
    iconv_t cd;
    char *buf;
    char *d;
    const char *local_charset;
    ICONV_ARG2 char *s;
    size_t buflen;
    size_t dlen;
    size_t slen;

    if (*from_charset == '\0')
        return;

    buf = NULL;
    local_charset = nl_langinfo(CODESET);

    if ((cd = iconv_open(local_charset, from_charset)) == (iconv_t)-1)
        return;

    slen = strlen(string);
    s = string;
    dlen = buflen = 2 * slen;
    d = buf = izu_malloc(buflen + 1);
    if (d)
    {
        /* 2017-11-29 Rene Freingruber, SMS.
         * Was using buflen, ignoring last byte.  Next strcpy() was
         * strncpy() with a bad maxchar parameter (buflen).
         */
        memset( buf, 0, (buflen+ 1));

        /* 2015-02-12 William Robinet, SMS.  CVE-2015-1315.
         * Added FILNAMSIZ check to avoid buffer overflow.  Better would
         * be to pass in an actual destination buffer size.
         *
         * 2018-12-05 SMS.  Worry about "const" on arg2?
         */
        if ((iconv(cd, &s, &slen, &d, &dlen) != (size_t)-1) &&
         (strlen(buf) < FILNAMSIZ))
        {
            strcpy(string, buf);
        }
        izu_free(buf);
    }
    iconv_close(cd);
}

#else /* def ICONV_MAPPING */
			
int dummy_iconv_map;    /* Dummy declaration to quiet compilers. */

#endif /* def ICONV_MAPPING [else] */
