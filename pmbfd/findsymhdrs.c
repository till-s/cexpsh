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

long
pmelf_find_symhdrs(Elf_Stream s, Pmelf_Elf32_Shtab shtab, Elf32_Shdr **psymsh, Elf32_Shdr **pstrsh)
{
Elf32_Sym    *sym;
Elf32_Shdr   *shdr;
uint32_t     i;
Elf32_Shdr   *symsh  = 0;
Elf32_Shdr   *strsh  = 0;
const char   *name;

	for ( i = 0, shdr = shtab->shdrs; i<shtab->nshdrs; i++, shdr++ ) {
		switch ( shdr->sh_type ) {
			default:
			break;

			case SHT_SYMTAB:
				if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: symtab section name out of bounds\n");
					return -1;
				}
				if ( !strcmp(".symtab", name) ) {
					if ( symsh ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: multiple symtabs\n");
						return -1;
					}
					symsh = shdr;
#define USE_SYM_SH_LINK
#ifdef USE_SYM_SH_LINK
					strsh = shtab->shdrs + symsh->sh_link;
#endif
				}
			break;

#ifndef USE_SYM_SH_LINK
			case SHT_STRTAB:
				if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: strtab section name out of bounds\n");
					return -1;
				}
				if ( !strcmp(".strtab", name) ) {
					if ( strsh ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: multiple strtabs\n");
						return -1;
					}
					strsh = shdr;
				}
			break;
#endif
		}
	}

	if ( !symsh ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: no symtab found\n");
		return -1;
	}

	if ( !strsh ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: no strtab found\n");
		return -1;
	}

	if ( 0 == symsh->sh_size ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: zero size symtab\n");
		return -1;
	}

	if ( symsh->sh_entsize && symsh->sh_entsize != sizeof( *sym ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: symbol size mismatch (sh_entsize %"PRIu32"\n", symsh->sh_entsize);
		return -1;
	}

	if ( (symsh->sh_size % sizeof(*sym)) != 0 ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: sh_size of symtab not a multiple of sizeof(Elf32_Sym)\n");
		return -1;
	}

	if ( pmelf_seek(s, symsh->sh_offset) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs unable to seek to symtab %s\n", strerror(errno));
		return -1;
	}

	if ( psymsh )
		*psymsh = symsh;
	if ( pstrsh )
		*pstrsh = strsh;

	return symsh->sh_size/sizeof(*sym);
}
