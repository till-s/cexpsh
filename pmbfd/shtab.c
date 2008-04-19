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
pmelf_delshtab(Pmelf_Shtab sht)
{
	if ( sht ) {
		free(sht->shdrs.p_raw);
		free((void*)sht->strtab);
		free(sht);
	}
}

Pmelf_Shtab
pmelf_getshtab(Elf_Stream s, Elf_Ehdr *pehdr)
{
Pmelf_Shtab rval = 0;
uint32_t      i;
Elf_Shdr      *strsh;
Pmelf_Off     e_shoff;
uint32_t      e_shnum;
uint32_t      e_shstrndx;
uint32_t      shdrsz;

	switch ( s->clss ) {
#ifdef PMELF_CONFIG_ELF64SUPPORT
		case ELFCLASS64:
			e_shoff    = pehdr->e64.e_shoff;
			e_shnum    = pehdr->e64.e_shnum;
			e_shstrndx = pehdr->e64.e_shstrndx;
			shdrsz     = sizeof(Elf64_Shdr);
		break;
#endif
		case ELFCLASS32:
			e_shoff    = pehdr->e32.e_shoff;
			e_shnum    = pehdr->e32.e_shnum;
			e_shstrndx = pehdr->e32.e_shstrndx;
			shdrsz     = sizeof(Elf32_Shdr);
		break;
		default:
		return 0;
	}

	if ( 0 == e_shnum )
		return 0;

	if ( pmelf_seek(s, e_shoff) )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	if ( !(rval->shdrs.p_raw = calloc(e_shnum, shdrsz)) )
		goto bail;

	rval->nshdrs = e_shnum;
	rval->clss   = s->clss;

#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( ELFCLASS64 == s->clss ) {
		/* slurp the section headers */
		for ( i=0; i<e_shnum; i++ ) {
			if ( pmelf_getshdr64(s, rval->shdrs.p_s64 + i) )
				goto bail;
		}
		strsh = (Elf_Shdr*)&rval->shdrs.p_s64[e_shstrndx];
		rval->strtablen = strsh->s64.sh_size;
	}
	else
#endif
	{
		/* slurp the section headers */
		for ( i=0; i<e_shnum; i++ ) {
			if ( pmelf_getshdr32(s, rval->shdrs.p_s32 + i) )
				goto bail;
		}
		strsh = (Elf_Shdr*)&rval->shdrs.p_s32[e_shstrndx];
		rval->strtablen = strsh->s32.sh_size;
	}

	/* slurp the string table    */
	if ( ! (rval->strtab = pmelf_getscn( s, strsh, 0, 0, 0 )) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"(shstrtab)\n");
		goto bail;
	}

	rval->idx = e_shstrndx;

	return rval;

bail:
	rval->strtablen = 0;
	pmelf_delshtab(rval);
	return 0;
}
