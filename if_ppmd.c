/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
 *
 * if_ppmd.c
 *
 * PPMd interface functions.
 *
 *   SzAlloc_ppmd()
 *   SzFree_ppmd()
 *   alloc_ppmd()
 *   g_ppmd_szios_extra_pf()
 *   g_ppmd_szios_res_pf()
 *   g_ppmd8_alloc()
 *   g_ppmd8_decode_symbol()
 *   g_ppmd8_free()
 *   g_ppmd8_pf()
 *   g_ppmd8_prep()
 *   g_ppmd8_range_dec_finished_ok()
 *   g_ppmd8_range_dec_init()
 *   g_ppmd8_stream()
 *   nextbyte_eof_ppmd()
 *   ppmd8_consts()
 *   sz_error_data_ppmd_f()
 *   version_ppmd()
 *
 * This module exists to segregate the 7-Zip PPMd header files (types,
 * version, ...) from the (incompatible) 7-Zip LZMA header files.
 *
 *---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL		
#include "unzip.h"              /* Get PPMD_SUPPORT right first. */

#ifdef PPMD_SUPPORT

# include "ppmd/SzTypes.h"
# include "ppmd/SzVersion.h"
# include "ppmd/Ppmd8.h"


/* Structure declarations. */

/* 7-Zip I/O structure (reduced).
 */
typedef struct
{
  IByteIn p;
  Bool extra;
  SRes res;
  struct Globals *pG;                   /* Useless? */
} CByteInToLook;


/* Global structure pointer macro.
 */
# define G_PPMD_P ((struct_ppmd_t *)(G.struct_ppmd_p))


/* Global PPMd structure.
 */
typedef struct
{
  ISzAlloc g_Alloc_ppmd;        /* Memory allocation function structure. */
  CPpmd8 ppmd8;                 /* PPMd8 structure. */
  CByteInToLook szios;          /* 7-Zip-like I/O structure. */
} struct_ppmd_t;


/* 2011-12-24  SMS.
 * 7-ZIP offers memory allocation functions with diagnostics conditional
 * on _SZ_ALLOC_DEBUG: lzma/Alloc.c: MyAlloc(), MyFree().  Using these
 * functions complicates linking with separately conditional LZMA and
 * PPMd support, so it's easier to use plain malloc() and free() here,
 * or else add the diagnostic messages to these Sz* functions, rather
 * than drag lzma/Alloc.c into the picture.  To use the lzma/Alloc.c
 * functions, add
 *    #include "lzma/Alloc.h"
 * above, change malloc() and free() below to MyAlloc() and MyFree(),
 * and add lzma/Alloc.* back to the builders.  (And then solve the other
 * problems.)
 */


/* Functions. */

/* 7-Zip memory alloc+free functions.
 */
zvoid *SzAlloc_ppmd( zvoid *p, size_t size) \
 { p = p; return izu_malloc(size); }

zvoid SzFree_ppmd( zvoid *p, zvoid *address) \
 { p = p; izu_free(address); }


/* Allocate a global PPMd structure.
 */
zvoid *alloc_ppmd()
{
  return izu_malloc( sizeof( struct_ppmd_t));
}


/* Return pointer to global structure I/O member, Bool.
 */
zvoid *g_ppmd_szios_extra_pf( __GPRO)
{
  return (&G_PPMD_P->szios.extra);
}


/* Return pointer to global structure I/O member, SRes.
 */
zvoid *g_ppmd_szios_res_pf( __GPRO)
{
  return (&G_PPMD_P->szios.res);
}


/* Allocate PPMd8 storage.
 */
int g_ppmd8_alloc( __GPRO__ int size)
{
  return Ppmd8_Alloc( &(G_PPMD_P->ppmd8), size, &(G_PPMD_P->g_Alloc_ppmd));
}


/* Return Ppmd8_DecodeSymbol() value.
 */
Bool g_ppmd8_decode_symbol( __GPRO)
{
  return Ppmd8_DecodeSymbol( &(G_PPMD_P->ppmd8));
}


/* Free any PPMd8 storage.  (Assume that G_PPMD_P is non-NULL.)
 */
zvoid g_ppmd8_free( __GPRO)
{
  /* Free any PPMd-related dynamic storage.
   * (Note that Ppmd8_Free() does not check for pointer validity.)
   */
  if (G_PPMD_P->ppmd8.Base != NULL)
  {
    Ppmd8_Free( &(G_PPMD_P->ppmd8), &(G_PPMD_P->g_Alloc_ppmd));
  }
}


/* Ppmd8_Init().
 */
void g_ppmd8_init( __GPRO__ unsigned order, unsigned restor)
{
  Ppmd8_Init( &(G_PPMD_P->ppmd8), order, restor);
}


/* Return pointer to global structure member, CPpmd8.
 */
zvoid *g_ppmd8_pf( __GPRO)
{
  return &(G_PPMD_P->ppmd8);
}


/* Miscellaneous preparation for 7-Zip PPMd.  (Do once.)
 */
zvoid g_ppmd8_prep( __GPRO__ Byte p_r_b( zvoid *))
{
  /* Set the 7-Zip alloc+free functions. */
  G_PPMD_P->g_Alloc_ppmd.Alloc = SzAlloc_ppmd;
  G_PPMD_P->g_Alloc_ppmd.Free = SzFree_ppmd;

  /* Construct internal PPMd8 structure. */
  Ppmd8_Construct( &(G_PPMD_P->ppmd8));

  /* Initialize the 7-Zip I/O structure. */
  G_PPMD_P->szios.p.Read = p_r_b;
  G_PPMD_P->szios.extra = False;
  G_PPMD_P->szios.res = SZ_OK;
  G_PPMD_P->szios.pG = &G;
}


/* Return Ppmd8_RangeDec_Init() value.
 */
Bool g_ppmd8_range_dec_init( __GPRO)
{
  return Ppmd8_RangeDec_Init( &(G_PPMD_P->ppmd8));
}


/* Return Ppmd8_RangeDec_IsFinishedOK() value.
 */
int g_ppmd8_range_dec_finished_ok( __GPRO)
{
  return Ppmd8_RangeDec_IsFinishedOK( &(G_PPMD_P->ppmd8));
}


/* Set PPMd8 I/O input structure.
 */
zvoid g_ppmd8_stream( __GPRO)
{
  G_PPMD_P->ppmd8.Stream.In = &(G_PPMD_P->szios.p);
}


/* Set PPMd I/O structure members to indicate EOF.
 */
zvoid nextbyte_eof_ppmd( CByteInToLook *szios_p)
{
  szios_p->extra = True;
  szios_p->res = SZ_ERROR_INPUT_EOF;
}


/* Return some 7-Zip and PPMd8 constants.
 */
zvoid ppmd8_consts( unsigned *max, unsigned *min, int *sz_err_dat)
{
  *max = PPMD8_MAX_ORDER;
  *min = PPMD8_MIN_ORDER;
  *sz_err_dat = SZ_ERROR_DATA;
}


/* Return value of SZ_ERROR_DATA.
 */
SRes sz_error_data_ppmd_f()
{
  return SZ_ERROR_DATA;
}


/* Return value of PPMd MY_VERSION.
 */
char *version_ppmd()
{
  return MY_VERSION;
}

#else /* def PPMD_SUPPORT */

/* Quiet compiler, if no real code. */

static int dummy;

#endif /* def PPMD_SUPPORT [else] */
