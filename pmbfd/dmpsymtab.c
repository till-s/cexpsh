#include "pmelfP.h"

void
pmelf_dump_symtab(FILE *f, Pmelf_Elf32_Symtab symtab, Pmelf_Elf32_Shtab shtab, int format)
{
uint32_t   i;
Elf32_Sym  *sym;

	if ( !f )
		f = stdout;

	fprintf(f,"%6s: %8s%6s %-8s%-7s%-9s",
		"Num", "Value", "Size", "Type", "Bind", "Vis");
	if ( shtab )
		fprintf(f,"%-13s", "Section");
	else
		fprintf(f,"%3s", "Ndx");
	fprintf(f," Name\n");
	for ( i=0, sym = symtab->syms; i<symtab->nsyms; i++, sym++ ) {
		fprintf(f,"%6"PRIu32": ", i);
		pmelf_dump_sym(f, sym, shtab, symtab->strtab, symtab->strtablen, format);
	}
}
