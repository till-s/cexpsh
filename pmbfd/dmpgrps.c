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
