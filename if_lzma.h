/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
 * if_lzma.h (for UnZip) by Info-ZIP.
 */

#ifndef __IF_LZMA_H
# define __IF_LZMA_H

/* Function prototypes for LZMA interface functions in if_lzma.c. */

zvoid *alloc_lzma();

zvoid  g_lzma_free( __GPRO);
zvoid *g_lzma_state_lzma_pf( __GPRO);
zvoid *g_lzma_state_lzma_probs_vf( __GPRO);
int    g_lzma_clzma_props_lc_vf( __GPRO);
int    g_lzma_clzma_props_lp_vf( __GPRO);
int    g_lzma_clzma_props_pb_vf( __GPRO);
int    g_lzma_clzma_props_dicsize_vf( __GPRO);
zvoid  g_lzma_prep_alloc_free( __GPRO);
int    g_lzma_construct_allocate( __GPRO__ unsigned char *lzma_props);
int    g_lzma_props_decode( __GPRO__ unsigned char *lzma_props);
zvoid  g_lzma_dec_init( __GPRO);

char  *version_lzma();

#endif /* ndef __IF_LZMA_H */
