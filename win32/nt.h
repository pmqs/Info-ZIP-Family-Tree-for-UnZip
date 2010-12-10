/*
  Copyright (c) 1990-2010 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* nt.h:  central header for EF_NTSD "SD" extra field */

#ifndef _NT_H
#define _NT_H

#define NTSD_BUFFERSIZE (1024)  /* threshold to cause malloc() */
#define OVERRIDE_BACKUP     1   /* we have SeBackupPrivilege on remote */
#define OVERRIDE_RESTORE    2   /* we have SeRestorePrivilege on remote */
#define OVERRIDE_SACL       4   /* we have SeSystemSecurityPrivilege on remote */

typedef struct {
    BOOL bValid;                /* are our contents valid? */
    BOOL bUsePrivileges;        /* use privilege overrides? */
    DWORD dwFileSystemFlags;    /* describes target file system */
    BOOL bRemote;               /* is volume remote? */
    DWORD dwRemotePrivileges;   /* relevant only on remote volumes */
    DWORD dwFileAttributes;
#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
    wchar_t RootPath[MAX_PATH+1];  /* path to network / filesystem */
#else
    char RootPath[MAX_PATH+1];  /* path to network / filesystem */
#endif
} VOLUMECAPS, *PVOLUMECAPS, *LPVOLUMECAPS;

#if defined(UNICODE_SUPPORT) && defined(WIN32_WIDE)
BOOL SecuritySet(wchar_t *resource, PVOLUMECAPS VolumeCaps, uch *securitydata);
BOOL GetVolumeCaps(wchar_t *rootpath, wchar_t *name, PVOLUMECAPS VolumeCaps);
#else
BOOL SecuritySet(char *resource, PVOLUMECAPS VolumeCaps, uch *securitydata);
BOOL GetVolumeCaps(char *rootpath, char *name, PVOLUMECAPS VolumeCaps);
#endif
BOOL ValidateSecurity(uch *securitydata);

#endif /* _NT_H */
