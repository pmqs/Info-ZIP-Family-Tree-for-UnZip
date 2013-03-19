
#ifdef MEMDIAG

/* Memory diagnostics.  Currently assume 32-bit pointer. */

/*      Diagnostic structure:
 *    ___  md structure   ___   fill_beg  store_req  fill_end
 *   / siz  ptr_bck  ptr_fwd \
 */

#include <string.h>

#define MD_FILL_BEG_SIZE  512
#define MD_FILL_BEG_FILL  0x5a
#define MD_FILL_END_SIZE  512
#define MD_FILL_END_FILL  0xa5

typedef struct md
{
  int siz;
  struct md *bck;
  struct md *fwd;
} md_t, *md_ptr_t;

md_t md_ht = { 0, &md_ht, &md_ht };


void izu_md_check( void)
{
  md_ptr_t mdp;
  int i;
  int idd = 0;

  mdp = &md_ht;
  while ((mdp = mdp->fwd) != &md_ht)
  {
    /* md  fill_beg  store_req  fill_end */

    /* Verify contents of beginning fill region. */
    for (i = 0; i < MD_FILL_BEG_SIZE; i++)
    {
      if (((unsigned char *)((unsigned char *)mdp)+ (sizeof md_ht))[ i] !=
       MD_FILL_BEG_FILL)
      {
        if (idd == 0)
        {
          idd = 1;
          fprintf( stderr, " mdp = %08x , siz = %8ld.\n", mdp, mdp->siz);
        }
        fprintf( stderr, " bf[%d] = %02x.\n",
         i,
         ((unsigned char *)((unsigned char *)mdp)+ (sizeof md_ht))[ i]);
      }
    }

    /* Verify contents of ending fill region. */
    for (i = 0; i < MD_FILL_END_SIZE; i++)
    {
      if (((unsigned char *)((unsigned char *)mdp)+ (sizeof md_ht)+
       MD_FILL_BEG_SIZE+ mdp->siz)[ i] != MD_FILL_END_FILL)
      {
        if (idd == 0)
        {
          idd = 1;
          fprintf( stderr, " mdp = %08x , siz = %8ld.\n", mdp, mdp->siz);
        }
        fprintf( stderr, " ef[%d] = %02x.\n",
         i,
         ((unsigned char *)((unsigned char *)mdp)+ (sizeof md_ht)+
         MD_FILL_BEG_SIZE+ mdp->siz)[ i]);
      }
    }
  }
}


void izu_free( void *ptr)
{
  md_ptr_t mdp;

  /* md  fill_beg  store_req  fill_end */

  mdp = (md_ptr_t)(((unsigned char *)ptr)- (sizeof md_ht)- MD_FILL_BEG_SIZE);

  fprintf( stderr, "<MD> f  %8ld  %08x  (act: %08x)\n", mdp->siz, ptr, mdp);

  izu_md_check();                       /* Check before work. */

  mdp->bck->fwd = mdp->fwd;             /* Re-link forward. */
  mdp->fwd->bck = mdp->bck;             /* Re-link backward. */

  free( mdp);

  fprintf( stderr, "<MD> F\n");
}


void *izu_malloc( size_t siz)
{
  md_ptr_t mdp;
  size_t siz2;
  void *ptr;

  fprintf( stderr, "<MD> m  %8ld\n", siz);

  izu_md_check();                       /* Check before work. */

  siz2 = (sizeof md_ht)+ MD_FILL_BEG_SIZE+ siz+ MD_FILL_END_SIZE;
  mdp = malloc( siz2);

  /* Add new member at tail. */
  mdp->bck = md_ht.bck;                 /* New backward link = Old tail. */
  md_ht.bck->fwd = mdp;                 /* Old tail forward link = New. */
  md_ht.bck = mdp;                      /* Tail = New. */
  mdp->fwd = &md_ht;                    /* New forward link = Tail. */
  mdp->siz = siz;                       /* New size. */

  /* md  fill_beg  store_req  fill_end */

  /* Set beginning and ending fill regions. */
  memset( ((unsigned char *)mdp+ (sizeof md_ht)),
   MD_FILL_BEG_FILL, MD_FILL_BEG_SIZE);
  memset( ((unsigned char *)mdp+ (sizeof md_ht)+ MD_FILL_BEG_SIZE+ siz),
   MD_FILL_END_FILL, MD_FILL_END_SIZE);

  /* Public pointer. */
  ptr = (unsigned char *)mdp+ (sizeof md_ht)+ MD_FILL_BEG_SIZE;

  fprintf( stderr, "<MD> M  %8ld  %08x  (act: %8ld  %08x)\n",
   siz, ptr, siz2, mdp);

  return ptr;
}


void *izu_realloc( void* ptr, size_t siz)
{
  void *ptr2;
  size_t siz2;
  md_ptr_t mdp;

  /* md  fill_beg  store_req  fill_end */

  mdp = (md_ptr_t)(((unsigned char *)ptr)- (sizeof md_ht)- MD_FILL_BEG_SIZE);
  siz2 = (sizeof md_ht)+ MD_FILL_BEG_SIZE+ siz+ MD_FILL_END_SIZE;

  fprintf( stderr, "<MD> r  %08x  %8ld  (act:%08x  %ld)\n",
   ptr, siz, mdp, siz2);

  izu_md_check();                       /* Check before work. */

  ptr2 = realloc( mdp, siz2);

  if (ptr2 == mdp)
  {
    /* No pointer change. */
    fprintf( stderr, "<MD> R  %08x  (%c=)  (act: %08x)  %8ld\n",
     ptr, '=', ptr2, siz);
  }
  else
  {
    /* Pointer changed.  Calculate new public pointer. */
    ptr = (unsigned char *)mdp+ (sizeof md_ht)+ MD_FILL_BEG_SIZE;

    fprintf( stderr, "<MD> R  %08x  (%c=)  (act: %08x)  %8ld\n",
     ptr, '!', ptr2, siz);

    mdp->bck->fwd = ptr2;               /* Forward pointer to new. */
    mdp->fwd->bck = ptr2;               /* Backward pointer to new. */
    ((md_ptr_t)ptr2)->bck = mdp->bck;   /* New backward pointer. */
    ((md_ptr_t)ptr2)->fwd = mdp->fwd;   /* New forward pointer. */
    mdp = ptr2;
  }

  /* Set (new?) ending fill region. */
  memset( ((unsigned char *)mdp+ (sizeof md_ht)+ MD_FILL_BEG_SIZE+ siz),
   MD_FILL_END_FILL, MD_FILL_END_SIZE);

  mdp->siz = siz;                       /* New size. */

  return ptr;
}

#endif /* def MEMDIAG */
