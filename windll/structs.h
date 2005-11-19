/*
  Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
#ifndef __unzip_structs_h
#define __unzip_structs_h

#ifndef Far
#  define Far far
#endif

/* Porting definitions between Win 3.1x and Win32 */
#ifdef WIN32
#  define far
#  define _far
#  define __far
#  define near
#  define _near
#  define __near
#  ifndef FAR
#    define FAR
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Z_UINT8_DEFINED
# if defined(__GNUC__)
   typedef unsigned long long    z_uint8;
#  define Z_UINT8_DEFINED
# elif (defined(_MSC_VER) && (_MSC_VER >= 1100))
   typedef unsigned __int64      z_uint8;
#  define Z_UINT8_DEFINED
# elif (defined(__WATCOMC__) && (__WATCOMC__ >= 1100))
   typedef unsigned __int64      z_uint8;
#  define Z_UINT8_DEFINED
# elif (defined(__IBMC__) && (__IBMC__ >= 350))
   typedef unsigned __int64      z_uint8;
#  define Z_UINT8_DEFINED
# elif (defined(__BORLANDC__) && (__BORLANDC__ >= 0x0500))
   typedef unsigned __int64      z_uint8;
#  define Z_UINT8_DEFINED
# elif (defined(__LCC__))
   typedef unsigned __int64      z_uint8;
#  define Z_UINT8_DEFINED
# endif
#endif

/* The following "function" types are jointly defined in both Zip and UnZip
 * DLLs.  They are guarded by the DEFINED_ONCE symbol to prevent multiple
 * declarations in applications that reference both the Zip and the UnZip DLL.
 */
#ifndef DEFINED_ONCE
#define DEFINED_ONCE

typedef int (WINAPI DLLPRNT) (LPSTR, unsigned long);
typedef int (WINAPI DLLPASSWORD) (LPSTR pwbuf, int bufsiz,
    LPCSTR promptmsg, LPCSTR entryname);
# ifdef Z_UINT8_DEFINED
typedef int (WINAPI DLLSERVICE) (LPCSTR entryname, z_uint8 uncomprsiz);
# else
typedef int (WINAPI DLLSERVICE) (LPCSTR entryname, unsigned long uncomprsiz);
# endif
typedef int (WINAPI DLLSERVICE_I32) (LPCSTR entryname,
    unsigned long ucsz_lo, unsigned long ucsz_hi);
#endif /* DEFINED_ONCE */

typedef void (WINAPI DLLSND) (void);
typedef int (WINAPI DLLREPLACE) (LPSTR efnam);
#ifdef Z_UINT8_DEFINED
typedef void (WINAPI DLLMESSAGE) (z_uint8 ucsize, z_uint8 csize,
    unsigned cfactor,
    unsigned mo, unsigned dy, unsigned yr, unsigned hh, unsigned mm,
    char c, LPCSTR filename, LPCSTR methbuf, unsigned long crc, char fCrypt);
#else
typedef void (WINAPI DLLMESSAGE) (unsigned long ucsize, unsigned long csize,
    unsigned cfactor,
    unsigned mo, unsigned dy, unsigned yr, unsigned hh, unsigned mm,
    char c, LPCSTR filename, LPCSTR methbuf, unsigned long crc, char fCrypt);
#endif
typedef void (WINAPI DLLMESSAGE_I32) (unsigned long ucsiz_l,
    unsigned long ucsiz_h, unsigned long csiz_l, unsigned long csiz_h,
    unsigned cfactor,
    unsigned mo, unsigned dy, unsigned yr, unsigned hh, unsigned mm,
    char c, LPCSTR filename, LPCSTR methbuf, unsigned long crc, char fCrypt);

typedef struct {
DLLPRNT *print;
DLLSND *sound;
DLLREPLACE *replace;
DLLPASSWORD *password;
DLLMESSAGE *SendApplicationMessage;
DLLSERVICE *ServCallBk;
DLLMESSAGE_I32 *SendApplicationMessage_i32;
DLLSERVICE_I32 *ServCallBk_i32;
#ifdef Z_UINT8_DEFINED
z_uint8 TotalSizeComp;
z_uint8 TotalSize;
z_uint8 NumMembers;
#else
struct _TotalSizeComp {
  unsigned long u4Lo;
  unsigned long u4Hi;
} TotalSizeComp;
struct _TotalSize {
  unsigned long u4Lo;
  unsigned long u4Hi;
} TotalSize;
struct _NumMembers {
  unsigned long u4Lo;
  unsigned long u4Hi;
} NumMembers;
#endif
unsigned CompFactor;
WORD cchComment;
} USERFUNCTIONS, far * LPUSERFUNCTIONS;

typedef struct {
int ExtractOnlyNewer;
int SpaceToUnderscore;
int PromptToOverwrite;
int fQuiet;
int ncflag;
int ntflag;
int nvflag;
int nfflag;
int nzflag;
int ndflag;
int noflag;
int naflag;
int nZIflag;
int C_flag;
int fPrivilege;
LPSTR lpszZipFN;
LPSTR lpszExtractDir;
} DCL, far * LPDCL;

#ifdef __cplusplus
}
#endif

/* return codes of the (DLLPASSWORD)() callback function */
#define IDM_REPLACE_NO     100
#define IDM_REPLACE_TEXT   101
#define IDM_REPLACE_YES    102
#define IDM_REPLACE_ALL    103
#define IDM_REPLACE_NONE   104
#define IDM_REPLACE_RENAME 105
#define IDM_REPLACE_HELP   106

#endif /* __unzip_structs_h */
