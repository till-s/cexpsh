/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmelf' ELF file reader) was created by
 *     Till Straumann <strauman@slac.stanford.edu>, 2008,
 * 	   Stanford Linear Accelerator Center, Stanford University.
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
 * ------------------ SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 
#ifndef PMELF_PRIVATE_H
#define PMELF_PRIVATE_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelf.h"

#define PARANOIA_ON 0
#undef PARANOIA_ON

struct _Elf_Stream {
	FILE *f;
	int  needswap;
};

extern FILE *pmelf_err;

#define PMELF_PRE    "pmelf: "
#define PMELF_PRINTF(f,a...) do { if (f) fprintf(f,a); } while (0)

#define ElfNumberOf(a) (sizeof(a)/sizeof((a)[0]))

static inline int iamlsb()
{
union {
	char b[2];
	short s;
} tester = { s: 1 };

	return tester.b[0] ? 1 : 0;
}

static inline uint16_t e32_swab( uint16_t v )
{
	return ( (v>>8) | (v<<8) );
}

static inline void e32_swap16( uint16_t *p )
{
	*p = e32_swab( *p );
}

static inline void e32_swap32( uint32_t *p )
{
register uint16_t vl = *p;
register uint16_t vh = *p>>16;
	*p = (e32_swab( vl ) << 16 ) | e32_swab ( vh ); 
}

#define ARRSTR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : "unknown" )
#define ARRCHR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : '?' )

#endif
