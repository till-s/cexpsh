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
 *          a trampoline for every PPCREL relocation but collapsing
 *          multiple references to a single target and ignoring
 *          relocations into the loadee) we must process all relocation
 *          records + symbol table etc. twice. Doable but cumbersome.
 *
 *       c) Reserve a (configurable) amount of memory in the '.text'
 *          area for module's text and use a dedicated allocator.
 *          This is the approach implemented by this file.
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
		}
	}
}

#define SEG_TEXT	0
#define SEG_DFLT    1

#if 1
#define CEXP_TEXT_REGION_SIZE (1024*1024*2)
#endif

#if CEXP_TEXT_REGION_SIZE > 1000
/* Hopefully put into .bss ... */
static char theTextRegion[CEXP_TEXT_REGION_SIZE] __attribute__((aligned(CPU_ALIGNMENT)));

static rtems_id text_region = 0;
#endif

#define NSEGS 2

int
cexpSegsInit(CexpSegment *ptr)
{
CexpSegment a;
#if CEXP_TEXT_REGION_SIZE > 1000
rtems_status_code sc;
rtems_id          id;
unsigned          sz = CEXP_TEXT_REGION_SIZE;
#endif

	/* lazy init of region(s) */
#if CEXP_TEXT_REGION_SIZE > 1000
#ifndef CEXP_TEXT_PAGE_SIZE
#define CEXP_TEXT_PAGE_SIZE 128
#endif
	if ( ! text_region ) {
		sc = rtems_region_create(
				rtems_build_name('t','e','x','t'),
				(void*)theTextRegion,	
				sizeof(theTextRegion),
				CEXP_TEXT_PAGE_SIZE,
				RTEMS_DEFAULT_ATTRIBUTES,
				&text_region);


		if ( RTEMS_SUCCESSFUL != sc ) {
			rtems_error(sc,"cexpsegs-powerpc-rtems: unable to create TEXT memory region\n");
			return 0;
		}
	}
#endif


	*ptr = 0;

	if ( ! (a = cexpSegsCreate(NSEGS)) ) {
		return 0;
	}

	a[SEG_TEXT].attributes = SEG_ATTR_EXEC | SEG_ATTR_RO;
	a[SEG_TEXT].name       = ".text";
#if CEXP_TEXT_REGION_SIZE <= 1000
	a[SEG_TEXT].allocat    = malloc_allocat;
	a[SEG_TEXT].release    = malloc_release;
#else
	a[SEG_TEXT].allocat    = region_allocat;
	a[SEG_TEXT].release    = region_release;
	a[SEG_TEXT].pvt        = (void*)text_region;
#endif

	a[SEG_DFLT].attributes = 0;
	a[SEG_DFLT].name       = ".data";
	a[SEG_DFLT].allocat    = malloc_allocat;
	a[SEG_DFLT].release    = malloc_release;

	*ptr = a;

	return NSEGS;
}

/*
 * Find segment index for a given section.
 * RETURNS: non-negative number or -1 on error.
 */
CexpSegment
cexpSegsMatch(CexpSegment segArray, struct bfd *abfd, struct sec *s)
{
const char *nam = bfd_get_section_name(abfd, s);

	if (    0 == strncmp(nam,".text",5)
	     || 0 == strncmp(nam,".gnu.linkonce.t.",16) )
		return &segArray[SEG_TEXT];
	
	
	return &segArray[SEG_DFLT];
}
