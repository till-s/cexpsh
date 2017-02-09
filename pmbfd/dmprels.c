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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelfP.h"

#include <inttypes.h>
#define T_REL32    1
#define T_RELA32   2
#define T_REL64    3
#define T_RELA64   4

static const char *reltype(unsigned mach, Elf_Reloc *r)
{
	switch ( mach ) {
		case EM_SPARC:  return pmelf_sparc_rel_name( &r->ra32 );
		case EM_386:    return pmelf_i386_rel_name( &r->r32 );
		case EM_68K:    return pmelf_m68k_rel_name( &r->ra32 );
		case EM_PPC:    return pmelf_ppc_rel_name( &r->ra32 );
		case EM_X86_64: return pmelf_x86_64_rel_name( &r->ra64 );
		case EM_ARM:    return pmelf_arm_rel_name( &r->r32 );
		default:
		break;
	}
	return "Unsup_MACH";
}

void
pmelf_dump_rels(FILE *f, Elf_Stream s, Pmelf_Shtab sht, Pmelf_Symtab symt)
{
uint8_t    *buf;
unsigned   nentries,entsz;
unsigned   type;
Elf_Shdr   *psect;
int        symidx,idx,i;

uint64_t    r_offset;
const char *r_symname;
uint64_t    r_addend = 0 /* silence compiler warning */;
uint64_t    r_info   = 0 /* silence compiler warning */;

uint64_t    st_name;
uint8_t     st_info;
uint16_t    st_shndx;
uint64_t    st_value;

	for ( idx=0; idx < sht->nshdrs; idx ++ ) {

	
		switch ( s->clss ) {
#ifdef PMELF_CONFIG_ELF64SUPPORT
			case ELFCLASS64:
				{
					psect = (Elf_Shdr*)&sht->shdrs.p_s64[idx];
					switch ( psect->s64.sh_type ) {
						case SHT_REL:   type = T_REL64;  break;
						case SHT_RELA:  type = T_RELA64; break;
						default:
										continue;
					}
					r_offset = psect->s64.sh_offset;
					nentries = psect->s64.sh_size / psect->s64.sh_entsize;
					entsz    = psect->s64.sh_entsize;
				}
				break;
#endif

			case ELFCLASS32:
				{
					psect = (Elf_Shdr*)&sht->shdrs.p_s32[idx];
					switch ( psect->s32.sh_type ) {
						case SHT_REL:   type = T_REL32;  break;
						case SHT_RELA:  type = T_RELA32; break;
						default:
										continue;
					}
					r_offset = psect->s32.sh_offset;
					nentries = psect->s32.sh_size / psect->s32.sh_entsize;
					entsz    = psect->s32.sh_entsize;
				}	
				break;

			default:
				/* should never get here; class is already checked when
				 * constructing the section header table.
				 */
				return;
		}

		if ( !(buf = pmelf_getrel(s, psect, 0)) ) {
			PMELF_PRINTF(pmelf_err, PMELF_PRE"error (pmelf_dump_rels()): unable to read section\n");
			return;
		}

		fprintf(f,"\nRelocation section '%s' at offset 0x%"PRIx64" contains %u entries:\n",
			pmelf_get_section_name(sht, idx), r_offset, nentries);

		fprintf(f," Offset     Info    Type            Sym.Value  Sym. Name");
		if ( T_RELA32 == type || T_RELA64 == type )
			fprintf(f," + Addend");
		fprintf(f,"\n");

		for ( i=0; i<nentries*entsz; i+= entsz ) {
			Elf_Reloc *rel = 0 /* silence compiler warning */;
			switch ( type ) {
				case T_REL32:
					{
					Elf32_Rel *r = (Elf32_Rel*)(buf+i);

					r_offset = r->r_offset;
					r_info   = r->r_info;

					rel = (Elf_Reloc *)r;
					}
				break;

				case T_RELA32:
					{
					Elf32_Rela *r = (Elf32_Rela*)(buf+i);

					r_offset = r->r_offset;
					r_info   = r->r_info;
					r_addend = r->r_addend;

					rel = (Elf_Reloc *)r;
					}
				break;

#ifdef PMELF_CONFIG_ELF64SUPPORT
				case T_REL64:
					{
					Elf64_Rel *r = (Elf64_Rel*)(buf+i);

					r_offset = r->r_offset;
					r_info   = r->r_info;

					rel = (Elf_Reloc *)r;
					}
				break;

				case T_RELA64:
					{
					Elf64_Rela *r = (Elf64_Rela*)(buf+i);

					r_offset = r->r_offset;
					r_info   = r->r_info;
					r_addend = r->r_addend;

					rel = (Elf_Reloc *)r;
					}
				break;
#endif
			}
						

			fprintf(f, "%08"PRIx64"  %08"PRIx64" %-17.17s ",
						r_offset,
						r_info,
						reltype(s->machine, rel));

#ifdef PMELF_CONFIG_ELF64SUPPORT
			if ( ELFCLASS64 == s->clss ) {
				symidx = ELF64_R_SYM( r_info );
			} else
#endif
			{
				symidx = ELF32_R_SYM( r_info );
			}

			if ( symidx >= symt->nsyms ) {
					PMELF_PRINTF(pmelf_err, PMELF_PRE"error (pmelf_dump_rels()): invalid symbol (idx %u out of range)\n", symidx);
				continue;
			}

#ifdef PMELF_CONFIG_ELF64SUPPORT
			if ( ELFCLASS64 == s->clss ) {
				st_name  = symt->syms.p_t64[symidx].st_name;
				st_info  = symt->syms.p_t64[symidx].st_info;
				st_shndx = symt->syms.p_t64[symidx].st_shndx;
				st_value = symt->syms.p_t64[symidx].st_value;
			} else
#endif
			{
				st_name  = symt->syms.p_t32[symidx].st_name;
				st_info  = symt->syms.p_t32[symidx].st_info;
				st_shndx = symt->syms.p_t32[symidx].st_shndx;
				st_value = symt->syms.p_t32[symidx].st_value;
			}


			if ( st_name > symt->strtablen ) {
				PMELF_PRINTF(pmelf_err, PMELF_PRE"error (pmelf_dump_rels()): invalid symbol name (strtab idx out of range)\n");
				continue;
			}

			/* ELF64_ST_TYPE == ELF32_ST_TYPE */
			if ( 0 == st_name && STT_SECTION == ELF32_ST_TYPE( st_info ) ) {
				r_symname = pmelf_get_section_name(sht, st_shndx);
			} else {
				r_symname = &symt->strtab[st_name];
			}

			fprintf(f,"%08"PRIx64"   %.22s", st_value, r_symname);

			if ( T_RELA32 == type || T_RELA64 == type ) {
				fprintf(f, " + %"PRIx64, r_addend);
			}

			fprintf(f,"\n");
		}
		free(buf);
	}
}
