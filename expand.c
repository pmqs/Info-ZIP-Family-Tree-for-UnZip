/* Info-ZIP version 1.0 */

/*
 * expand (unreduce)
 *
 * Written by Peter Backes, 2012
 * Public Domain
 *
 * This is a free replacement for "[t]he last chunk of code in UnZip
 * that was blatantly derived from Sam Smith's unzip 2.0."  The reduce
 * algorithm was used only by PKZIP 0.90 and 0.92.  To enable unreducing
 * capability, define USE_UNREDUCE_PUBLIC (for example, by specifying
 * LOCAL_UNZIP=-DUSE_UNREDUCE_PUBLIC to the Makefile).
 *
 * reduce was rarely ever used, and, in fact, the PKZIP 0.90 and 0.92
 * self-extracting distributions themselves (PKZ090.EXE and PKZ092.EXE)
 * are the only easy to find files that actually make use of it.  So
 * this file might be handy only if you are into computer archeology and
 * old software archives.
 *
 *
 *    "The Reducing algorithm is actually a combination of two
 *     distinct algorithms.  The first algorithm compresses repeated
 *     byte sequences, and the second algorithm takes the compressed
 *     stream from the first algorithm and applies a probabilistic
 *     compression method."                      --APPNOTE.TXT
 *
 *
 *
 *     NOTE: unreduce may or may not infinge or have been covered or
 *     be covered anytime in the future by patent restoration, by
 *     US patent 4,612,532, "Data compression apparatus and method,"
 *     inventor Bacon, assignee Telebyte Corportion, filed Jun. 19,
 *     1984, granted Sep. 16, 1986 (see compression-faq, section 7).
 *
 *     ALL LIABILITY FOR USE OF THIS CODE IN VIOLATION OF APPLICABLE
 *     PATENT LAW IS HEREBY DISCLAIMED.
 *
 */


#define __UNREDUCE_C
#define UNZIP_INTERNAL
#include "unzip.h"

/* extend most significant bit to the right, at most one char, set
 * least significant extended bit to 1
 */
#define M_EXTR1C(j) (((j) | (j) >> 1) | ((j) | (j) >> 1) >> 2 \
	        | (((j) | (j) >> 1) | ((j) |127) >> 2) >> 4)

/* compute log2(n) for n=1,2,...,128.  Stolen from ghostscript gxarith.h */
#define M_L2C(n) ((unsigned int)(05637042010L >> ((((n) % 11) - 1) * 3)) & 7)

#define LMASK 0x0f1f3f7f
/* function L(X): masks out the lower (8-CF) bits of X, where
 * CF = compression factor
 */
#define M_L(cf,x) (LMASK >> ((cf-1)<<3)  &  (x))
/* function F(X): 2 if all masked bits in L(X) are set, 3 otherwise */
#define M_F(cf,x) (3 - (M_L(cf,0xff) == (x)))
/* function D(X,Y):  upper CF bits of X concatenated with the bits of y+1 */
#define M_D(cf,x,y) ((((x) << (cf)) & 0xf00) + (y) + 1)
/* B(N(j)) = bits needed to encode N(j)-1 */
#define M_B(i) (M_L2C((M_EXTR1C((i)-1)&127)+1)+((unsigned)((i)-1)>127))

#define DLE 144

#define XREADBITS(nbits,zdest,stmt) READBITS(nbits,zdest) if (G.zipeof) stmt;

#define SWSIZE M_D(cf, 255, 255)

int unreduce(__G)
	__GDEF
{
	int lc = 0; /* Last-Character */
	int state = 0;
	int len, v;
	int cf = G.lrec.compression_method - 1;
	ulg rest = G.lrec.ucsize;
	int outpos = 0, backptr;
	int error;
	int i;
	/* number of followers N(j) */
	uch (*f_n)[256] = (uch (*)[256])(slide + SWSIZE);
	/* follower set S(j)[0..N(j)-1] */
	uch (*f_s)[256][32] = (uch (*)[256][32])(slide + SWSIZE + sizeof *f_n);

	memzero(slide,SWSIZE);

	Trace((stderr, "unreduce()\ncompression factor %d\n", cf));
	Trace((stderr, "size1 %d size2 %d setlen %d\n", sizeof **f_s,
		sizeof ***f_s, M_L2C(sizeof **f_s / sizeof ***f_s)));
	/* get follower sets */
	for (i = 0; i < sizeof *f_n / sizeof **f_n; i++) {
		int m, j = 255 - i;
		XREADBITS(M_L2C(sizeof **f_s / sizeof ***f_s) + 1,
			(*f_n)[j],return PK_OK)
		Trace((stderr, "N(%d) = %d, S(%d) = {", j, (*f_n)[j], j));
		if ((*f_n)[j] > 32)
			return PK_OK;
		for (m = 0; m < (*f_n)[j]; m++) {
			XREADBITS(8,(*f_s)[j][m], return PK_OK)
			Trace((stderr, "%d, ", (*f_s)[j][m]));
		}
		Trace((stderr, "}\n"));
	}

	/* expand file */
	while (rest) {
		Trace((stderr, "N(%d) = %d ", lc, (*f_n)[lc]));
		if (!(*f_n)[lc]) {
			XREADBITS(8, lc, break)
			Trace((stderr, "=> %d ", lc));
		} else {
			shrint code;
			XREADBITS(1, code, break)
			if (code) {
				XREADBITS(8, lc, break)
				Trace((stderr, "1 => %d", lc));
			} else {
				XREADBITS(M_B((*f_n)[lc]), code, break)
				Trace((stderr, "0 %d %d ",
					M_B((*f_n)[lc]), code));
				if (code >= (*f_n)[lc])
					break;
				lc = (*f_s)[lc][code];
				Trace((stderr, "=> %d", lc));
			}
		}

		Trace((stderr, "\t[%d] ", state));

		switch (state) {
		case 0:
			if (lc != DLE) {
				slide[outpos++] = lc;
				rest--;
				Trace((stderr, "%d/%d", outpos, SWSIZE));
				if (outpos == SWSIZE) {
				 	if ((error = flush(__G__ slide,
							(ulg)SWSIZE, 0)))
						return error;
					outpos = 0;
				}
			} else
				state = 1;
			Trace((stderr, "\n"));
			break;
		case 1:
			if (lc) {
				v = lc;
				len = M_L(cf,v);
				state = M_F(cf,len);
				Trace((stderr, "len=%d", len));
			} else {
				Trace((stderr, "%d/%d", outpos, SWSIZE));
				slide[outpos++] = DLE;
				rest--;
				if (outpos == SWSIZE) {
					if ((error = flush(__G__ slide,
							(ulg)SWSIZE, 0)))
						return error;

					outpos = 0;
				}
				state = 0;
			}
			Trace((stderr, "\n"));
			break;
		case 2:
			len += lc;
			Trace((stderr, "+len=%d\n", len));
			state = 3;
			break;
		case 3:
			backptr = outpos - M_D(cf,v,lc);
			len += 3;
			rest -= len > rest ? rest : len;
			Trace((stderr, "%d  <-%d[%d]-- %d/%d\n", backptr,
				outpos - backptr,len, outpos, SWSIZE));
			if (backptr < 0)
				backptr += SWSIZE;

			if (backptr + len > SWSIZE) {
				len -= SWSIZE - backptr;
				if (outpos >= backptr) {
					while (outpos < SWSIZE)
						slide[outpos++]
							= slide[backptr++];
					if ((error = flush(__G__ slide,
							(ulg)SWSIZE, 0)))
						return error;
					outpos = 0;
				}
				memmove(slide+outpos,slide+backptr,
					SWSIZE - backptr);
				outpos += SWSIZE - backptr;
				backptr = 0;
			}
			if (outpos + len >= SWSIZE) {
				len -= SWSIZE - outpos;
				while (outpos < SWSIZE)
					slide[outpos++]
						= slide[backptr++];
				if ((error = flush(__G__ slide,
						(ulg)SWSIZE, 0)))
					return error;
				outpos = 0;
			}
			while (len-- > 0)
				slide[outpos++]
					= slide[backptr++];

			state = 0;
			break;
		}
	}
	if (outpos && (error = flush(__G__ slide, (ulg)outpos, 0)))
		return error;

	return PK_OK;
}

