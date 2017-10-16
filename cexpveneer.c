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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_RTEMS_CACHE_H
#include <rtems/rtems/cache.h>
#endif

#include <cexpveneerP.h>
#include <inttypes.h>
#include <string.h>

#ifdef USE_PMBFD
#include <pmbfd.h>
#endif

#ifdef USE_PMBFD
static uint16_t mkflgs(CexpSym csym)
{
uint16_t flags = 0;
	if ( (csym->flags & CEXP_SYMFLG_GLBL) )
		flags=BSF_GLOBAL;
	if ( (csym->flags & CEXP_SYMFLG_SECT) )
		flags |= BSF_SECTION_SYM;
	if ( (csym->flags & CEXP_SYMFLG_WEAK) )
		flags |= BSF_WEAK;
	if ( (csym->value.type & CEXP_FUN_BIT) )
		flags |= BSF_FUNCTION;
	return flags;
}
#endif

int
cexpSymTblFixup(CexpSymTbl symt, CexpSegment veneerSeg)
{
#ifdef USE_PMBFD
CexpSym               cesp;
unsigned              align = 0;
unsigned              nv    = 0;
unsigned long         totsz = 0;
unsigned              sz;

uint8_t              *p;
uintptr_t             pmsk;
uintptr_t             pint;
CexpSymXtraVeneerInfo xtra;


	/* Count the number of veneers we have to create */
	for ( cesp=symt->syms; cesp->name; cesp++ ) {
		if (   !! ( cesp->value.type & CEXP_FUN_BIT ) ) {
			if ( (sz = pmbfd_make_veneer( (bfd_vma)cesp->value.ptv, mkflgs(cesp), 0 )) ) {
				if ( (cesp->flags & CEXP_SYMFLG_HAS_XTRA) ) {
					fprintf(stderr,"INTERNAL ERROR: Symbol %s already has xtra info -- unable to attach veneer\n", cesp->name );
					return -1;
				}
				if ( sz > align )
					align = sz;
				nv++;
			}
		}
	}

	if ( 0 == nv )
		return 0;

	/* Allocate memory */
	veneerSeg->size = nv*align + align - 1; /* word alignment */

	if ( cexpSegsAllocOne( veneerSeg ) ) {
		fprintf(stderr, "cexpSymTblFixup: Unable to allocate segment for veneers\n");
		return -1;
	}

	/* Ensure word alignment */
	pint = (uintptr_t) veneerSeg->chunk;
	pmsk = align - 1;
	pint = (pint + pmsk) & ~pmsk;
	p    = (uint8_t*)pint;

	/* Doit */
	for ( cesp=symt->syms; cesp->name; cesp++ ) {
		if (   !! ( cesp->value.type & CEXP_FUN_BIT ) ) {

			sz = pmbfd_make_veneer( (bfd_vma)cesp->value.ptv, mkflgs(cesp), &p );

			if ( 0 == sz )
				continue;

			if ( ! (xtra = malloc( sizeof(*xtra) ) ) ) {
				fprintf(stderr, "ERROR: cexpSymTblFixup: no memory for extra info\n");
				return -1;
			}

			xtra->base.help = cesp->xtra.help;
			xtra->veneer    = (void*)p;

			/* fixup symbol to point at the new veneer */
			cesp->xtra.info = &xtra->base;
			cesp->flags    |= CEXP_SYMFLG_VENR;

			p += sz;

			printf("veneer created for %s\n", cesp->name);
		}
	}

#ifdef HAVE_RTEMS_CACHE_H
	rtems_cache_flush_multiple_data_lines( (void*)veneerSeg->chunk, veneerSeg->size );
	rtems_cache_invalidate_multiple_instruction_lines( (void*)veneerSeg->chunk, veneerSeg->size );
#endif
#endif
	return 0;
}

