/*
  Copyright (c) 1990-2013 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE.txt, version 2000-Apr-09 or later
  for terms of use.  If this file is missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/

/*
 * iz_aes_wg.h -- Info-ZIP-specific data associated with WinZip/Gladman
 * AES encryption.
 */

#ifndef __iz_aes_wg_h           /* Don't include more than once. */
#define __iz_aes_wg_h

#ifndef IZ_AES_WG_BETA
#  define IZ_AES_WG_BETA        /* This is a beta version. */
#endif

#ifdef IZ_AES_WG_BETA
#  undef IZ_AES_WG_BETA         /* This is not a beta version. */
#endif


#define IZ_AES_WG_MAJORVER      1
#define IZ_AES_WG_MINORVER      3
#ifdef IZ_AES_WG_BETA
#  define IZ_AES_WG_BETA_VER     "a BETA"
#  define IZ_AES_WG_VERSION_DATE "12 Apr 2013"  /* Last real code change. */
#else
#  define IZ_AES_WG_BETA_VER     ""
#  define IZ_AES_WG_VERSION_DATE "18 Nov 2013"  /* Last public release date. */
#  define IZ_AES_WG_RELEASE
#endif

#endif /* ndef __iz_aes_wg_h */
