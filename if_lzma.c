/*
  Copyright (c) 1990-2018 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
 *
 * if_lzma.c
 *
 * LZMA interface functions.
 *
 *   SzAlloc_lzma()
 *   SzFree_lzma()
 *
 *   alloc_lzma()
 *   g_lzma_clzma_props_dicsize_vf()
 *   g_lzma_clzma_props_lc_vf()
 *   g_lzma_clzma_props_lp_vf()
 *   g_lzma_clzma_props_pb_vf()
 *   g_lzma_construct_allocate()
 *   g_lzma_dec_init()
 *   g_lzma_free()
 *   g_lzma_prep_alloc_free()
 *   g_lzma_props_decode()
 *   g_lzma_state_lzma_pf()
 *   version_lzma()
 *
 * This module exists to segregate the 7-Zip LZMA header files (types,
 * version, ...) from the (incompatible) 7-Zip PPMd header files.
 *
 *---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"              /* Get LZMA_SUPPORT right first. */

#ifdef LZMA_SUPPORT

# include "lzma/SzTypes.h"
# include "lzma/SzVersion.h"
# include "lzma/LzmaDec.h"


/* Structure declarations. */

/* Global structure pointer macro.
 */
# define G_LZMA_P ((struct_lzma_t *)(G.struct_lzma_p))


/* Global LZMA structure.
 */
typedef struct
{
  ISzAlloc g_Alloc_lzma;        /* Memory allocation function structure. */
  CLzmaProps clzma_props;       /* LZMA properties. */
  CLzmaDec state_lzma;          /* LZMA context. */
} struct_lzma_t;


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
static zvoid *SzAlloc_lzma( const struct ISzAlloc *p, size_t size) \
 { p = p; return izu_malloc(size); }

static zvoid SzFree_lzma( const struct ISzAlloc *p, zvoid *address) \
 { p = p; izu_free(address); }


/* Allocate a global LZMA structure.
 */
zvoid *alloc_lzma()
{
  return izu_malloc( sizeof( struct_lzma_t));
}


/* Return value of global structure member clzma_props.dicSize.
 */
UInt32 g_lzma_clzma_props_dicsize_vf( __GPRO)
{
  return (G_LZMA_P->clzma_props.dicSize);
}


/* Return value of global structure member clzma_props.lc.
 */
Byte g_lzma_clzma_props_lc_vf( __GPRO)
{
  return (G_LZMA_P->clzma_props.lc);
}


/* Return value of global structure member clzma_props.lp.
 */
Byte g_lzma_clzma_props_lp_vf( __GPRO)
{
  return (G_LZMA_P->clzma_props.lp);
}


/* Return value of global structure member clzma_props.pb.
 */
Byte g_lzma_clzma_props_pb_vf( __GPRO)
{
  return (G_LZMA_P->clzma_props.pb);
}


/* Construct and allocate the storage for 7-Zip LZMA.  (Do once.)
 */
int g_lzma_construct_allocate( __GPRO__ unsigned char *lzma_props)
{
  LzmaDec_Construct( &G_LZMA_P->state_lzma);

  return LzmaDec_Allocate( &G_LZMA_P->state_lzma, lzma_props,
     LZMA_PROPS_SIZE, &G_LZMA_P->g_Alloc_lzma);
}


/* LZMA decode initialize.
 */
zvoid g_lzma_dec_init( __GPRO)
{
  LzmaDec_Init( &G_LZMA_P->state_lzma);
}


/* Free any LZMA storage.  (Assume that G_LZMA_P is non-NULL.)
 */
zvoid g_lzma_free( __GPRO)
{
  /* Free any LZMA-related dynamic storage.
   * (Note that LzmaDec_Free() does not check for pointer validity.
   * For complete safety, ->state_lzma.probs and -> state_lzma.dic could
   * be checked.
   */
  LzmaDec_Free( &G_LZMA_P->state_lzma, &G_LZMA_P->g_Alloc_lzma);
}


/* Set alloc+free functions for 7-Zip LZMA.  (Do once.)
 */
zvoid g_lzma_prep_alloc_free( __GPRO)
{
  /* Set the 7-Zip alloc+free functions. */
  G_LZMA_P->g_Alloc_lzma.Alloc = SzAlloc_lzma;
  G_LZMA_P->g_Alloc_lzma.Free = SzFree_lzma;
}


/* LZMA properties decode.
 */
int g_lzma_props_decode( __GPRO__ unsigned char *lzma_props)
{
  return LzmaProps_Decode( &G_LZMA_P->clzma_props,
   lzma_props, LZMA_PROPS_SIZE);
}


/* Return pointer to global structure member state_lzma.
 */
zvoid *g_lzma_state_lzma_pf( __GPRO)
{
  return (&G_LZMA_P->state_lzma);
}


/* Return value of LZMA MY_VERSION.
 */
char *version_lzma()
{
  return MY_VERSION;
}

#else /* def LZMA_SUPPORT */

/* Quiet compiler, if no real code. */

static int dummy;

#endif /* def LZMA_SUPPORT [else] */
