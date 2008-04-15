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

void
pmelf_delshtab(Pmelf_Elf32_Shtab sht)
{
	if ( sht ) {
		free(sht->shdrs);
		free((void*)sht->strtab);
		free(sht);
	}
}

Pmelf_Elf32_Shtab
pmelf_getshtab(Elf_Stream s, Elf32_Ehdr *pehdr)
{
Pmelf_Elf32_Shtab rval = 0;
uint32_t      i;
Elf32_Shdr    *strsh;

	if ( 0 == pehdr->e_shnum )
		return 0;

	if ( pmelf_seek(s, pehdr->e_shoff) )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	if ( !(rval->shdrs = calloc(pehdr->e_shnum, sizeof(*rval->shdrs))) )
		goto bail;

	rval->nshdrs = pehdr->e_shnum;

	/* slurp the section headers */
	for ( i=0; i<pehdr->e_shnum; i++ ) {
		if ( pmelf_getshdr(s, rval->shdrs + i) )
			goto bail;
	}

	/* slurp the string table    */
	strsh = &rval->shdrs[pehdr->e_shstrndx];
	if ( ! (rval->strtab = pmelf_getscn( s, strsh, 0, 0, 0 )) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"(shstrtab)\n");
		goto bail;
	}

	rval->strtablen = strsh->sh_size;

	return rval;

bail:
	pmelf_delshtab(rval);
	return 0;
}
