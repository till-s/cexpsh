/* $Id$ */

/* cexp memory segments; implementation of common utilities */

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

CexpSegment
cexpSegsCreate(int n)
{
CexpSegment rval = calloc(n+1, sizeof(*rval));

	return rval;
}

/*
 * Allocate memory for the segments themselves
 * and attach to 'chunk' pointers.
 */

int
cexpSegsAllocAll(CexpSegment s)
{
	while ( s->name ) {
		if ( cexpSegsAllocOne(s) )
			return -1;
		s++;
	}
	return 0;
}

int
cexpSegsAllocOne(CexpSegment s)
{
	if ( s->chunk || !s->allocat || s->allocat(s) )
		return -1;
}

void
cexpSegsDeleteAll(CexpSegment  s)
{
CexpSegment p;
	if ( !s )
		return;

	for ( p=s; p->name; p++ ) {
		cexpSegsDeleteOne(p);
	}
	free(s);
}

void
cexpSegsDeleteOne(CexpSegment  s)
{
	if ( s && s->release )
		s->release(s);
}
