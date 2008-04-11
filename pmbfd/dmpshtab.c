#include "pmelfP.h"

void
pmelf_dump_shtab(FILE *f, Pmelf_Elf32_Shtab shtab, int format)
{
uint32_t i;
Elf32_Shdr *shdr;
const char *name, *fmt;

	if ( !f )
		f = stdout;

	fprintf(f,"Section Headers:\n");
	fprintf(f,"  [Nr] %-18s%-16s%-9s%-7s%-7s%2s%4s%3s%4s%3s\n",
		"Name", "Type", "Addr", "Off", "Size", "ES", "Flg", "Lk", "Inf", "Al");

	for ( i = 0, shdr = shtab->shdrs; i<shtab->nshdrs; i++, shdr++ ) {
		if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
			name = "<OUT-OF-BOUND>";
		}
		if ( FMT_COMPAT == format )
			fmt = "  [%2u] %-18.17s";
		else
			fmt = "  [%2u] %-18s";
		fprintf( f, fmt, i, name );
		pmelf_dump_shdr( f, shdr, format );
	}
	fprintf(f,"Key to Flags:\n");
	fprintf(f,"  W (write), A (alloc), X (execute), M (merge), S (strings)\n");
	fprintf(f,"  I (info), L (link order), G (group), ? (unknown)\n");
	fprintf(f,"  O (extra OS processing required)\n");
}
