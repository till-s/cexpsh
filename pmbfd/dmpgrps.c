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

int
pmelf_dump_groups(FILE *f, Elf_Stream s, Pmelf_Shtab shtab, Pmelf_Symtab symtab)
{
int i,j;
int ng,ne;
Elf_Shdr *shdr;
int sz=0;
int shdrsz;
Elf32_Word *buf = 0;
Elf32_Word *nbuf;
const char *name;
uint8_t    *p;
Pmelf_Size sh_size;
uint32_t   sh_type, sh_link, sh_info;


	if ( !f )
		f = stdout;

	shdrsz = get_shdrsz(shtab);

	for ( ng = i = 0, p = shtab->shdrs.p_raw; i < shtab->nshdrs; i++, p+=shdrsz ) {
		shdr = (Elf_Shdr*)p;

#ifdef PMELF_CONFIG_ELF64SUPPORT
		if ( ELFCLASS64 == shtab->clss )
			sh_type = shdr->s64.sh_type;
		else
#endif
			sh_type = shdr->s32.sh_type;

		if ( SHT_GROUP == sh_type ) {

#ifdef PMELF_CONFIG_ELF64SUPPORT
			if ( ELFCLASS64 == shtab->clss ) {
				sh_size = shdr->s64.sh_size;
				sh_link = shdr->s64.sh_link;
				sh_info = shdr->s64.sh_info;
			}
			else
#endif
			{
				sh_size = shdr->s32.sh_size;
				sh_link = shdr->s32.sh_link;
				sh_info = shdr->s32.sh_info;
			}

			if ( sh_size > sz ) {
				free(buf); buf = 0;
				sz = sh_size;
			}

			if ( ! (nbuf = pmelf_getgrp(s, shdr, buf)) ) {
				break;
			}
			buf = nbuf;

			ne  = sh_size/sizeof(*buf);

			if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
				name = "<OUT-OF-BOUNDS>";
			}

			/* first entry holds flags */
			fprintf(f,"\n%-7sgroup section [%5u] `%s' ",
				buf[0] & GRP_COMDAT ? "COMDAT":"",
				i,
				name
			);
			if ( symtab ) {
				if ( symtab->idx == sh_link ) {
					if ( sh_info >= symtab->nsyms ) {
						PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_dump_groups: group sig. symbol index out of bounds\n");
						ng = -1;
						goto cleanup;
					}
					if ( !(name = pmelf_sym_name(symtab, get_symtabN(symtab,sh_info))) )
						name = "<OUT-OF-BOUNDS>";
					fprintf(f, "[%s]", name);
				} else {
					fprintf(f, "<unable to print group id symbol: symtab index mismatch> (expecting %"PRIu32", got %"PRIu32")", symtab->idx, sh_link );
				}
			} else {
				fprintf(f,"<unable to print group id symbol: no symbol table given");
			}
			fprintf(f," contains %u sections:\n", ne - 1);

			fprintf(f,"   [Index]    Name\n");

			for ( j=1; j < ne; j++ ) {
				if ( buf[j] >= shtab->nshdrs ) {
					PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_dump_groups: group member index out of bounds\n");
					ng = -1;
					goto cleanup;
				}
				if ( ! (name = pmelf_sec_name(shtab, (Elf_Shdr*)(shtab->shdrs.p_raw + shdrsz*buf[j]))) ) {
					name = "<OUT-OF-BOUNDS>";
				}
				fprintf(f,"   [%5"PRIu32"]   %s\n", buf[j], name);
			}

			ng++;
		}
	}

cleanup:
	free(buf);
	return ng;
}
