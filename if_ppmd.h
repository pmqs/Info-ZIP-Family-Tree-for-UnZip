/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
 * if_ppmd.h (for UnZip) by Info-ZIP.
 */

#ifndef __IF_PPMD_H
# define __IF_PPMD_H

/* Function prototypes for PPMd interface functions in if_ppmd.c. */

zvoid *alloc_ppmd();

zvoid *g_ppmd_alloc_ppmd_pf( __GPRO);
zvoid *g_ppmd_szios_extra_pf( __GPRO);
zvoid *g_ppmd_szios_res_pf( __GPRO);

int    g_ppmd8_alloc( __GPRO__ int);
zvoid  g_ppmd8_free( __GPRO);
zvoid *g_ppmd8_pf( __GPRO);

zvoid  g_alloc_ppmd_set( __GPRO);


zvoid  g_ppmd8_prep( __GPRO__ zvoid *);

zvoid  g_ppmd_io( __GPRO__ zvoid *);
zvoid *g_ppmd8_construct( __GPRO);
zvoid *g_ppmd8_stream( __GPRO);

int    sz_error_data_ppmd_f();
int    g_ppmd8_range_dec_finished_ok( __GPRO);

zvoid  nextbyte_eof_ppmd( zvoid *);

char  *version_ppmd();

#endif /* ndef __IF_PPMD_H */
