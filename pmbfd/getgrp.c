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
#include "pmelfP.h"

Elf32_Word *
pmelf_getgrp(Elf_Stream s, Elf_Shdr *psect, Elf32_Word *data)
{
Elf32_Word *buf;
int        j;
Pmelf_Off  sh_size;
Pmelf_Off  ne;

#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
	if ( s->needswap )
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error (pmelf_getgrp()): host/target byte order mismatch but pmelf was configured w/o support for byte-swapping\n");
		return 0;
#endif

	switch ( s->clss ) {
#ifdef PMELF_CONFIG_ELF64SUPPORT
		case ELFCLASS64:
			if ( SHT_GROUP != psect->s64.sh_type )
				return 0;
			sh_size = psect->s64.sh_size;
		break;
#endif
		case ELFCLASS32:
			if ( SHT_GROUP != psect->s32.sh_type )
				return 0;
			sh_size = psect->s32.sh_size;
		break;
		default:
		return 0;
	}

	if ( sh_size & (((Pmelf_Off)sizeof(*buf))-1) ) {
		PMELF_PRINTF(pmelf_err, "group section size not a multiple of %lu\n", (unsigned long)sizeof(*buf));
		return 0;
	}

	ne  = sh_size/sizeof(*buf);

	if ( ne < 1 ) {
		PMELF_PRINTF(pmelf_err, "group has no members and not even a flag word\n");
		return 0;
	}

	if ( ! (buf = pmelf_getscn(s, psect, data, 0, 0)) )
		return 0;

#ifndef PMELF_CONFIG_NO_SWAPSUPPORT
	if ( s->needswap ) {
		for ( j=0; j < ne; j++ ) {
			elf_swap32( &buf[j] );
		}
	}
#endif

	return buf;
}
