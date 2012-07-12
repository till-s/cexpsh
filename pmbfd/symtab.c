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
pmelf_delsymtab(Pmelf_Symtab symtab)
{
	if ( symtab ) {
		free(symtab->syms.p_raw);
		free((void*)symtab->strtab);
		free(symtab);
	}
}

static Pmelf_Symtab
_pmelf_getsymtab(Elf_Stream s, Pmelf_Shtab shtab, int dynamic)
{
Pmelf_Symtab rval    = 0;
Elf_Shdr     *symsh  = 0;
Elf_Shdr     *strsh  = 0;
Pmelf_Long   i,nsyms;
uint32_t     symsz;

	if ( (nsyms = dynamic ?
	                pmelf_find_dsymhdrs(s, shtab, &symsh, &strsh) :
	                pmelf_find_symhdrs(s, shtab, &symsh, &strsh)
	      ) < 0
	   )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	rval->nsyms = nsyms;

	rval->idx   = ((uintptr_t)symsh - (uintptr_t)shtab->shdrs.p_raw) / get_shdrsz(shtab);
	rval->clss  = shtab->clss;

	if ( 0 == rval->nsyms ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab: symtab with 0 elements\n");
		goto bail;
	}

	symsz = get_symsz(rval);

	if ( !(rval->syms.p_raw = calloc(rval->nsyms, symsz)) )
		goto bail;

	/* slurp the symbols */
#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( ELFCLASS64 == rval->clss ) {
		for ( i=0; i<rval->nsyms; i++ ) {
			if ( pmelf_getsym64(s, rval->syms.p_t64 + i) ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab unable to read symbol %lu: %s\n", i, errno ? strerror(errno) : "");
				goto bail;
			}
		}
	} 
	else
#endif
	{
		for ( i=0; i<rval->nsyms; i++ ) {
			if ( pmelf_getsym32(s, rval->syms.p_t32 + i) ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab unable to read symbol %lu: %s\n", i, errno ? strerror(errno) : "");
				goto bail;
			}
		}
	}

	if ( ! ( rval->strtab = pmelf_getscn( s, strsh, 0, 0, 0 ) ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"(strtab)\n" );
		goto bail;
	}

	rval->strtablen = get_sh_size(shtab->clss,strsh);

	return rval;

bail:
	pmelf_delsymtab(rval);
	return 0;
}

Pmelf_Symtab
pmelf_getsymtab(Elf_Stream s, Pmelf_Shtab shtab)
{
	return _pmelf_getsymtab(s, shtab, 0);
}

Pmelf_Symtab
pmelf_getdsymtab(Elf_Stream s, Pmelf_Shtab shtab)
{
	return _pmelf_getsymtab(s, shtab, 1);
}
