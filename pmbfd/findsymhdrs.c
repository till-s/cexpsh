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

#define USE_SYM_SH_LINK

static Pmelf_Long
_pmelf_find_symhdrs(Elf_Stream s, Pmelf_Shtab shtab, Elf_Shdr **psymsh, Elf_Shdr **pstrsh, int dynamic)
{
Elf_Shdr   *shdr;
uint32_t   i;
Elf_Shdr   *symsh  = 0;
Elf_Shdr   *strsh  = 0;
const char *name;
uint8_t    *p;
uint32_t   shdrsz = get_shdrsz(shtab);
uint32_t   symsz;
Pmelf_Size sh_size, sh_entsize;
Pmelf_Off  sh_offset;
uint32_t   type, destype;
const char *desname;
#ifndef USE_SYM_SH_LINK
const char *desstrn;
#endif

	if ( dynamic ) {
		destype = SHT_DYNSYM;
		desname = ".dynsym";
#ifndef USE_SYM_SH_LINK
		desstrn = ".dynstr";
#endif
	} else {
		destype = SHT_SYMTAB;
		desname = ".symtab";
#ifndef USE_SYM_SH_LINK
		desstrn = ".strtab";
#endif
	}
	destype = dynamic ? SHT_DYNSYM : SHT_SYMTAB;

	for ( i = 0, p = shtab->shdrs.p_raw; i<shtab->nshdrs; i++, p+=shdrsz ) {
		shdr = (Elf_Shdr*)p;

		type = get_sh_type(shtab->clss, shdr);

		if ( type == destype ) {
			if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: symtab section name out of bounds\n");
				return -1;
			}
			if ( !strcmp(desname, name) ) {
				if ( symsh ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: multiple symtabs\n");
					return -1;
				}
				symsh = shdr;
#ifdef USE_SYM_SH_LINK
				strsh = get_shtabN(shtab,get_sh_link(shtab->clss, symsh));
#endif
			}
		}
#ifndef USE_SYM_SH_LINK
		else if ( SHT_STRTAB == type ) {
				if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: strtab section name out of bounds\n");
					return -1;
				}
				if ( !strcmp(desstrn, name) ) {
					if ( strsh ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: multiple strtabs\n");
						return -1;
					}
					strsh = shdr;
				}
		}
#endif
	}

	if ( !symsh ) {
#if 0
		/* let the caller barf it they think it is appropriate */
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: no symtab found\n");
#endif
		return -1;
	}

#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( ELFCLASS64 == shtab->clss ) {
		sh_size    = symsh->s64.sh_size;
		sh_entsize = symsh->s64.sh_entsize;
		sh_offset  = symsh->s64.sh_offset;
		symsz      = sizeof(Elf64_Sym);
	}
	else
#endif
	{
		sh_size    = symsh->s32.sh_size;
		sh_entsize = symsh->s32.sh_entsize;
		sh_offset  = symsh->s32.sh_offset;
		symsz      = sizeof(Elf32_Sym);
	}


	if ( !strsh ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: no strtab found\n");
		return -1;
	}

	if ( 0 == sh_size ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: zero size symtab\n");
		return -1;
	}

	if ( sh_entsize && sh_entsize != symsz ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: symbol size mismatch (sh_entsize %lu\n", (unsigned long)sh_entsize);
		return -1;
	}

	if ( (sh_size % symsz) != 0 ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: sh_size of symtab not a multiple of sizeof(Elf32_Sym)\n");
		return -1;
	}

	if ( pmelf_seek(s, sh_offset) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs unable to seek to symtab %s\n", strerror(errno));
		return -1;
	}

	if ( psymsh )
		*psymsh = symsh;
	if ( pstrsh )
		*pstrsh = strsh;

	return sh_size/symsz;
}

Pmelf_Long
pmelf_find_symhdrs(Elf_Stream s, Pmelf_Shtab shtab, Elf_Shdr **psymsh, Elf_Shdr **pstrsh)
{
	return _pmelf_find_symhdrs(s, shtab, psymsh, pstrsh, 0);
}
Pmelf_Long
pmelf_find_dsymhdrs(Elf_Stream s, Pmelf_Shtab shtab, Elf_Shdr **psymsh, Elf_Shdr **pstrsh)
{
	return _pmelf_find_symhdrs(s, shtab, psymsh, pstrsh, 1);
}
