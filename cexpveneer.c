/* Minimal support for veneers */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *    Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 * 	  Stanford Linear Accelerator Center, Stanford University.
 *
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 *     the Stanford Linear Accelerator Center, Stanford University,
 * 	   under Contract DE-AC03-76SFO0515 with the Department of Energy.
 * 
 * Government disclaimer of liability
 * ----------------------------------
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied, or
 * assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately owned
 * rights.
 * 
 * Stanford disclaimer of liability
 * --------------------------------
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * Stanford disclaimer of copyright
 * --------------------------------
 * Stanford University, owner of the copyright, hereby disclaims its
 * copyright and all other rights in this software.  Hence, anyone may
 * freely use it for any purpose without restriction.  
 * 
 * Maintenance of notices
 * ----------------------
 * In the interest of clarity regarding the origin and status of this
 * SLAC software, this and all the preceding Stanford University notices
 * are to remain affixed to any copy or derivative of this software made
 * or distributed by the recipient and are to be affixed to any copy of
 * software made or distributed by the recipient that contains a copy or
 * derivative of this software.
 * 
 * SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 

#include <cexpveneerP.h>
#include <inttypes.h>
#include <string.h>

#define  ARM_A2T_VEN_SZ   8
#define  ARM_A2T_VEN_ALGN 4


int
cexpSymTblFixup(CexpSymTbl symt, CexpSegment veneerSeg)
{
#ifdef __arm__
CexpSym  cesp;
unsigned nv = 0;
uint8_t  *p;
uintptr_t pmsk;
uintptr_t      pint;
const uint16_t bx_pc = 0x4778; 
const uint16_t nop   = 0x46c0;
uint32_t       b_arm = 0xea000000;
	/* Create veneers for all symbols that are not thumb symbols */

	/* Count the number of veneers we have to create */
	for ( cesp=symt->syms; cesp->name; cesp++ ) {
		if (   !! ( cesp->value.type & CEXP_FUN_BIT )
			&& !  ( ((uintptr_t)cesp->value.ptv) & 1 ) ) {
			nv++;
		}
	}

	if ( 0 == nv )
		return 0;

	/* Allocate memory */
	veneerSeg->size = nv*ARM_A2T_VEN_SZ + ARM_A2T_VEN_ALGN - 1; /* word alignment */

	if ( cexpSegsAllocOne( veneerSeg ) ) {
		fprintf(stderr,"cexpSymTblFixup: Unable to allocate segment for veneers\n");
		return -1;
	}

	/* Ensure word alignment */
	pint = (uintptr_t) veneerSeg->chunk;
	pmsk = ARM_A2T_VEN_ALGN - 1;
	pint = (pint + ARM_A2T_VEN_ALGN - 1) & ~pmsk;
	p    = (uint8_t*)pint;

	/* Doit */
	for ( cesp=symt->syms; cesp->name; cesp++ ) {
		if (   !! ( cesp->value.type & CEXP_FUN_BIT )
			&& !  ( ((uintptr_t)cesp->value.ptv) & 1 ) ) {

			int32_t off = (int32_t)cesp->value.ptv - (int32_t)p + 4;
			if ( off > (int32_t)0x01ffffff || off < (int32_t)0xfe000000 ) {
				fprintf(stderr,"WARNING: cexpSymTblFixup: unable to create veneer for (%s) -- out of reach (veneer %p, target %p) -- disabling symbol by making local\n", cesp->name, cesp->value.ptv, p);

				cesp->flags &= ~CEXP_SYMFLG_GLBL;

				continue;
			}

			if ( off & 2 ) {
				fprintf(stderr,"WARNING: cexpSymTblFixup: target of veneer mis-aligned (%s), target %p -- disabling symbol by making local\n", cesp->name, cesp->value.ptv);

				cesp->flags &= ~CEXP_SYMFLG_GLBL;
				continue;
			}

			memcpy(p + 0, &bx_pc, sizeof(bx_pc));
			memcpy(p + 2, &nop,   sizeof(nop  ));

			b_arm = 0xea000000 | ( (off>>2) & 0x00ffffff);

			memcpy(p + 4, &b_arm, sizeof(b_arm));

			/* fixup symbol to point at the new veneer */
			cesp->value.ptv = (CexpVal)( (uintptr_t)p | 1 /* Thumb bit */ );

			p += sizeof(bx_pc) + sizeof(nop) + sizeof(b_arm);
			printf("veneer created for %s\n", cesp->name);
		}
	}
	
#else
	return 0;
#endif
}

