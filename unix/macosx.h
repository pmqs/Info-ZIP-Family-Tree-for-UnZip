/*
  macosx.h - UnZip 6.1

  Copyright (c) 2008-2016 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/

#ifndef __MACOSX_H
# define __MACOSX_H

# if defined( UNIX) && defined( __APPLE__)

#  include <sys/attr.h>
#  include <sys/vnode.h>
#  ifdef APPLE_XATTR
#   include <sys/xattr.h>
#  endif /* def APPLE_XATTR */

#  define APL_DBL_PFX "._"
#  define APL_DBL_PFX_SQR "__MACOSX/"

   /* Select modern ("/..namedfork/rsrc") or old ("/rsrc") suffix
    * for resource fork access.
    */
#  ifndef APPLE_NFRSRC
#   if defined( __ppc__) || defined( __ppc64__)
#    define APPLE_NFRSRC 0
#   else /* defined( __ppc__) || defined( __ppc64__) */
#    define APPLE_NFRSRC 1
#   endif /* defined( __ppc__) || defined( __ppc64__) [else] */
#  endif /* ndef APPLE_NFRSRC */
#  if APPLE_NFRSRC
#   define APL_DBL_SUFX "/..namedfork/rsrc"
#  else /* APPLE_NFRSRC */
#   define APL_DBL_SUFX "/rsrc"
#  endif /* APPLE_NFRSRC [else] */

#  define APL_DBL_OFS_MAGIC             0
#  define APL_DBL_OFS_VERSION           4
#  define APL_DBL_OFS_FILLER            8
#  define APL_DBL_OFS_ENT_CNT          24
#  define APL_DBL_OFS_ENT_DSCR         28
#  define APL_DBL_OFS_ENT_DSCR_OFS1    42
#  define APL_DBL_OFS_FNDR_INFO        50
#  define APL_DBL_SIZE_FNDR_INFO       32
#  define APL_DBL_SIZE_HDR              \
    (APL_DBL_OFS_FNDR_INFO+ APL_DBL_SIZE_FNDR_INFO)
#  define APL_DBL_OFS_ATTR             (APL_DBL_SIZE_HDR+ 2)  /* 2-byte pad. */

   /* Macros to convert big-endian byte (unsigned char) array segments
    * to 16- or 32-bit entities.
    * Note that the larger entities must be naturally aligned in the
    * byte array for the simple type casts to work (on PowerPC).  This
    * should be true for the AppleDouble data where we use these macros.
    */
#  if defined( __ppc__) || defined( __ppc64__)
    /* Big-endian to Big-endian. */
#   define BIGC_TO_HOST16( i16) (*((unsigned short *)(i16)))
#   define BIGC_TO_HOST32( i32) (*((unsigned int *)(i32)))
#  else /* defined( __ppc__) || defined( __ppc64__) */
    /* Little-endian to Big-endian. */
#   define BIGC_TO_HOST16( i16) \
     (((unsigned short)*(i16)<< 8)+ (unsigned short)*(i16+ 1))
#   define BIGC_TO_HOST32( i32) \
     (((unsigned int)*(i32)<< 24) + ((unsigned int)*(i32+ 1)<< 16) +\
     ((unsigned int)*(i32+ 2)<< 8)+ ((unsigned int)*(i32+ 3)))
#  endif /* defined( __ppc__) || defined( __ppc64__) [else] */

# endif /* defined( unix) && defined( __APPLE__) */

#endif /* ndef __MACOSX_H */

