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
pmelf_dump_groups(FILE *f, Elf_Stream s, Pmelf_Elf32_Shtab shtab, Pmelf_Elf32_Symtab symtab)
{
int i,j;
int ng,ne;
Elf32_Shdr *shdr;
int sz=0;
Elf32_Word *buf = 0;
Elf32_Word *nbuf;
const char *name;

	if ( !f )
		f = stdout;

	for ( ng = i = 0, shdr = shtab->shdrs; i < shtab->nshdrs; i++, shdr++ ) {

		if ( SHT_GROUP == shdr->sh_type ) {

			if ( shdr->sh_size > sz ) {
				free(buf); buf = 0;
				sz = shdr->sh_size;
			}

			if ( ! (nbuf = pmelf_getgrp(s, shdr, buf)) ) {
				break;
			}
			buf = nbuf;

			ne  = shdr->sh_size/sizeof(*buf);

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
				if ( symtab->idx == shdr->sh_link ) {
					if ( shdr->sh_info >= symtab->nsyms ) {
						PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_dump_groups: group sig. symbol index out of bounds\n");
						ng = -1;
						goto cleanup;
					}
					if ( !(name = pmelf_sym_name(symtab, &symtab->syms[shdr->sh_info])) )
						name = "<OUT-OF-BOUNDS>";
					fprintf(f, "[%s]", name);
				} else {
					fprintf(f, "<unable to print group id symbol: symtab index mismatch>");
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
				if ( ! (name = pmelf_sec_name(shtab, &shtab->shdrs[buf[j]])) ) {
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
