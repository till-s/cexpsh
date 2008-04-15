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
pmelf_delsymtab(Pmelf_Elf32_Symtab symtab)
{
	if ( symtab ) {
		free(symtab->syms);
		free((void*)symtab->strtab);
		free(symtab);
	}
}

Pmelf_Elf32_Symtab
pmelf_getsymtab(Elf_Stream s, Pmelf_Elf32_Shtab shtab)
{
Pmelf_Elf32_Symtab rval = 0;
uint32_t      i;
long          nsyms;
Elf32_Shdr   *symsh  = 0;
Elf32_Shdr   *strsh  = 0;

	if ( (nsyms = pmelf_find_symhdrs(s, shtab, &symsh, &strsh)) < 0 )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	rval->nsyms = nsyms;

	rval->idx   = symsh - shtab->shdrs;

	if ( 0 == rval->nsyms ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab: symtab with 0 elements\n");
		goto bail;
	}

	if ( !(rval->syms = calloc(rval->nsyms, sizeof(*rval->syms))) )
		goto bail;

	/* slurp the symbols */
	for ( i=0; i<rval->nsyms; i++ ) {
		if ( pmelf_getsym(s, rval->syms + i) ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab unable to read symbol %"PRIu32": %s\n", i, strerror(errno));
			goto bail;
		}
	}

	if ( ! ( rval->strtab = pmelf_getscn( s, strsh, 0, 0, 0 ) ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"(strtab)\n" );
		goto bail;
	}

	rval->strtablen = strsh->sh_size;

	return rval;

bail:
	pmelf_delsymtab(rval);
	return 0;
}
