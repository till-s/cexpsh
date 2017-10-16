/* $Id$ */

/* memory segments for powerpc/RTEMS */

/* 
 * On PPC there are several issues:
 *
 *  1) SYSV and EABI access of short data areas.
 *     We could use the segment facility to allocate
 *     separate areas for an objects small objects
 *     and support sdarel etc. relocations.
 *
 *     This is not implemented (yet) due to difficulties:
 *       - special linker script required to reserve space
 *         in the SDA SDA2 areas for loaded objects.
 *       - what to do with new COMMON symbols that are to
 *         be stored in short data areas? We'd need to
 *         further modify bfdstuff.c to handle this.
 *
 *  2) std. code cannot branch farther than 32MB. There
 *     are a few possible solutions:
 *
 *       a) BSP pretends to only have 32M until that is exhausted;
 *          sbrk then yields the rest of physical memory. No more
 *          modules can be loaded after 'sbrk'.
 *          This approach has been available for a while but
 *          must be implemented by the (rtems) BSP.
 *
 *       b) Let the linker (cexp) detect far branches and insert
 *          trampoline code if necessary.
 *          The problem here is that we cannot know prior to
 *          allocating memory for the text sections what symbols
 *          are out of reach. Hence, worst-case assumptions for
 *          the space required by trampolines must be made
 *          (we must allocate the text area and the trampoines
 *          as a single chunk -- otherwise the trampolines may
 *          not be reachable).
 *          In order to find the semi-worst case (not just allocation
 *          a trampoline for every R_PPC_REL24 relocation but collapsing
 *          multiple references to a single target and ignoring
 *          relocations into the loadee) we must process all relocation
 *          records + symbol table etc. twice. Doable but cumbersome.
 *
 *          Also, for this approach we must assume that R_PPC_REL24
 *          relocations ONLY refer to branches -- otherwise this
 *          doesn't work!
 *
 *       c) Reserve a (configurable) amount of memory in the '.text'
 *          area for module's text and use a dedicated allocator.
 *          This is the approach implemented by this file.
 *
 *          FIXME:
 *          However, the space is compile-time configurable. We
 *          should also have an option to configure the space
 *          when linking the application...
 */

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

#include <rtems.h>
#include <rtems/error.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cexpsegsP.h"

#ifdef USE_PMBFD
#include "pmbfd.h"
#else
#include "bfd.h"
#endif

static int malloc_allocat(CexpSegment s)
{
	if ( s->size ) {
		if ( ! (s->chunk = malloc(s->size)) )
			return -1;
	} else {
		s->chunk = 0;
	}
	return 0;
}

static void malloc_release(CexpSegment s)
{
	free(s->chunk);
	s->chunk = 0;
	s->size  = 0;
}

static int region_allocat(CexpSegment s)
{
rtems_status_code sc;

	if ( 0 == s->size ) {
		s->chunk = 0;
		return 0;
	}

	sc = rtems_region_get_segment(
			(rtems_id)s->pvt,
			s->size, 
			RTEMS_NO_WAIT, 
			RTEMS_NO_TIMEOUT,
			&s->chunk);
	if ( RTEMS_SUCCESSFUL != sc ) {
		rtems_error(sc,"cexpsegs-powerpc-rtems: unable to allocate '%s' segment\n", s->name);
		return -1;
	}

	return 0;
}

static void region_release(CexpSegment s)
{
rtems_status_code sc;

	if ( s->chunk ) {
		sc = rtems_region_return_segment((rtems_id)s->pvt, s->chunk);
		if ( RTEMS_SUCCESSFUL != sc ) {
			rtems_error(sc,"cexpsegs-powerpc-rtems: unable to release '%s' segment\n", s->name);
		} else {
			s->chunk = 0;
			s->size = 0;
		}
	} else {
		s->size = 0; /* just to be sure */
	}
}

#ifdef CEXP_TEXT_REGION_SIZE

#if CEXP_TEXT_REGION_SIZE > 0
char                 cexpTextRegion[CEXP_TEXT_REGION_SIZE] = {0};
unsigned long        cexpTextRegionSize = CEXP_TEXT_REGION_SIZE;
#else
/* Application provides the region */

/* Provide weak symbols so we can fall back to using 'malloc'
 * if they don't provide us with a separate region
 */

/* Here are various options:
 *
 *  The linker script (or another part of the application)
 *  may provide:
 *
 *  EITHER:
 *     cexpTextRegion[] array and cexpTextRegionSize variable
 *  OR:
 *     cexpTextRegionStart and cexpTextRegionEnd addresses
 *
 *  The reason for having both is that we want to be compatible
 *
 *    a) linker script doesn't define either pair
 *    b) linker script defines a pair but user chooses
 *       to configure CEXP_TEXT_REGION_SIZE > 0
 *
 *       in this case, the linker must not clash
 *       with the existing symbols...
 */

extern unsigned long cexpTextRegionSize
	__attribute__(( weak, alias("_cexpTextRegionSize_fallback") ));

extern char          cexpTextRegion[]
	__attribute__(( weak ));

extern char         _cexpTextRegionStart[]
	__attribute__(( weak ));

extern char         _cexpTextRegionEnd[]
	__attribute__(( weak ));

static unsigned long _cexpTextRegionSize_fallback = 0;

#endif

static rtems_id text_region = 0;

#else

unsigned long cexpTextRegionSize = 0;

#endif

#define NSEGS (CEXP_SEG_DATA + 1)

#define LIMIT_32M 0x02000000

int
cexpSegsInit(CexpSegment *ptr)
{
CexpSegment a;

#ifdef CEXP_TEXT_REGION_SIZE

rtems_status_code sc;
unsigned long     start, start_unaligned = LIMIT_32M;
unsigned long     sz;

	/* lazy init of region(s) */

	/* Figure out where we do get the memory... */
	if ( cexpTextRegionSize ) {
		start_unaligned = (unsigned long)cexpTextRegion;
	}
#if CEXP_TEXT_REGION_SIZE <= 0
	else {
		if ( (cexpTextRegionSize = _cexpTextRegionEnd - _cexpTextRegionStart) )
			start_unaligned = (unsigned long)_cexpTextRegionStart;
	}
#endif

#ifndef CEXP_TEXT_PAGE_SIZE
#define CEXP_TEXT_PAGE_SIZE 128
#endif

	/* Basic sanity checks */
	if ( start_unaligned >= LIMIT_32M ) {
		/* cannot use */
		cexpTextRegionSize = 0;
	} else if ( start_unaligned + cexpTextRegionSize > LIMIT_32M ) {
		cexpTextRegionSize = LIMIT_32M - start_unaligned;
	}

	if ( ! text_region && cexpTextRegionSize ) {
		/* Region manager wants an aligned starting
		 * address or it bails :-(
		 */
		start = start_unaligned;
		start = (start + CPU_ALIGNMENT - 1) & ~(CPU_ALIGNMENT-1);
		sz    = cexpTextRegionSize - (start - start_unaligned);
		sz   &= ~(CPU_ALIGNMENT - 1);

		sc = rtems_region_create(
				rtems_build_name('t','e','x','t'),
				(void*)start,	
				sz,
				CEXP_TEXT_PAGE_SIZE,
				RTEMS_DEFAULT_ATTRIBUTES,
				&text_region);

		if ( RTEMS_SUCCESSFUL != sc ) {
			rtems_error(sc,"cexpsegs-powerpc-rtems: unable to create TEXT memory region\n");
			text_region = 0;
			return 0;
		}
	}
#endif

	*ptr = 0;

	if ( ! (a = cexpSegsCreate(NSEGS)) ) {
		return 0;
	}

	/* DFLT can be equal to TEXT */
	a[CEXP_SEG_TEXT].attributes = SEG_ATTR_EXEC | SEG_ATTR_RO;
	a[CEXP_SEG_TEXT].name       = ".text";

	if ( cexpTextRegionSize > 1000 ) {
		a[CEXP_SEG_TEXT].allocat    = region_allocat;
		a[CEXP_SEG_TEXT].release    = region_release;
		a[CEXP_SEG_TEXT].pvt        = (void*)text_region;
	} else {
		a[CEXP_SEG_TEXT].allocat    = malloc_allocat;
		a[CEXP_SEG_TEXT].release    = malloc_release;
	}

	a[CEXP_SEG_DATA].attributes = 0;
	a[CEXP_SEG_DATA].name       = ".data";
	a[CEXP_SEG_DATA].allocat    = malloc_allocat;
	a[CEXP_SEG_DATA].release    = malloc_release;

	*ptr = a;

	return NSEGS;
}


CexpSegment
cexpSegsGet(CexpSegment segArray, CexpSegType type)
{
	if ( type >= CEXP_SEG_TEXT && type <= CEXP_SEG_DATA ) {
		return &segArray[type];
	}
	return 0;
}

/*
 * Find segment index for a given section.
 */
CexpSegment
cexpSegsMatch(CexpSegment segArray, struct bfd *abfd, void *s)
{
	if ( SEC_CODE & bfd_get_section_flags(abfd, (asection*)s) )
		return &segArray[CEXP_SEG_TEXT];
	
	return &segArray[CEXP_SEG_DATA];
}
