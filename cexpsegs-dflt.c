/* $Id$ */

/* default implementation of memory segments (1 single segment using malloc/free) */

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

#include <stdlib.h>

#include "cexpsegsP.h"

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

int
cexpSegsInit(CexpSegment *ptr)
{
	if ( ! (*ptr = cexpSegsCreate(1)) ) {
		return 0;
	}

	(*ptr)->attributes = SEG_ATTR_EXEC;
	(*ptr)->name       = ".all";
	(*ptr)->allocat    = malloc_allocat;
	(*ptr)->release    = malloc_release;

	return 1;
}

/*
 * Find segment index for a given section.
 * RETURNS: non-negative number or -1 on error.
 */
CexpSegment
cexpSegsMatch(CexpSegment segArray, struct bfd *abfd, void *s)
{
	return &segArray[0];
}
