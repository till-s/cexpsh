#include "pmelfP.h"

void
pmelf_delsymtab(Pmelf_Elf32_Symtab symtab)
{
	if ( symtab ) {
		free(symtab->syms);
		free((void*)symtab->strtab);
		free(symtab);
	}
}

Pmelf_Elf32_Symtab
pmelf_getsymtab(Elf_Stream s, Pmelf_Elf32_Shtab shtab)
{
Pmelf_Elf32_Symtab rval = 0;
uint32_t      i;
long          nsyms;
Elf32_Shdr   *symsh  = 0;
Elf32_Shdr   *strsh  = 0;

	if ( (nsyms = pmelf_find_symhdrs(s, shtab, &symsh, &strsh)) < 0 )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	rval->nsyms = nsyms;

	rval->idx   = symsh - shtab->shdrs;

	if ( 0 == rval->nsyms ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab: symtab with 0 elements\n");
		goto bail;
	}

	if ( !(rval->syms = calloc(rval->nsyms, sizeof(*rval->syms))) )
		goto bail;

	/* slurp the symbols */
	for ( i=0; i<rval->nsyms; i++ ) {
		if ( pmelf_getsym(s, rval->syms + i) ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsymtab unable to read symbol %"PRIu32": %s\n", i, strerror(errno));
			goto bail;
		}
	}

	if ( ! ( rval->strtab = pmelf_getscn( s, strsh, 0, 0, 0 ) ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"(strtab)\n" );
		goto bail;
	}

	rval->strtablen = strsh->sh_size;

	return rval;

bail:
	pmelf_delsymtab(rval);
	return 0;
}
