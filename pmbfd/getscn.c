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

void *
pmelf_getscn(Elf_Stream s, Elf_Shdr *psect, void *data, Pmelf_Off offset, Pmelf_Off len)
{
void       *buf = 0;
Pmelf_Off  end;
Pmelf_Off  sh_size, sh_offset;

	switch ( s->clss ) {
#ifdef PMELF_CONFIG_ELF64SUPPORT
		case ELFCLASS64:
			sh_size   = psect->s64.sh_size;
			sh_offset = psect->s64.sh_offset;
		break;
#endif
		case ELFCLASS32:
			sh_size   = psect->s32.sh_size;
			sh_offset = psect->s32.sh_offset;
		break;
		default:
		return 0;
	}

	if ( 0 == len )
		len = sh_size;

	end = offset + len;

	if ( end < offset || end < len ) {
		/* wrap around 0 */
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn invalid offset/length (too large)\n");
		return 0;
	}

	if ( end > sh_size ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn invalid offset/length (requested area not entirely within section)\n");
		return 0;
	}

	if ( pmelf_seek(s, sh_offset + offset) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn unable to seek %s", strerror(errno));
		return 0;
	}

	if ( !data ) {
		buf = malloc(len);
		if ( !buf ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn - no memory");
			return 0;
		}
		data = buf;
	}

	if ( len != SREAD(data, 1, len, s) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn unable to read %s", errno ? strerror(errno) : "");
		free(buf);
		return 0;
	}

	return data;
}
