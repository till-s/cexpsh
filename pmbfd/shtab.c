#include "pmelfP.h"

void
pmelf_delshtab(Pmelf_Elf32_Shtab sht)
{
	if ( sht ) {
		free(sht->shdrs);
		free((void*)sht->strtab);
		free(sht);
	}
}

Pmelf_Elf32_Shtab
pmelf_getshtab(Elf_Stream s, Elf32_Ehdr *pehdr)
{
Pmelf_Elf32_Shtab rval = 0;
uint32_t      i;
Elf32_Shdr    *strsh;

	if ( 0 == pehdr->e_shnum )
		return 0;

	if ( pmelf_seek(s, pehdr->e_shoff) )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	if ( !(rval->shdrs = calloc(pehdr->e_shnum, sizeof(*rval->shdrs))) )
		goto bail;

	rval->nshdrs = pehdr->e_shnum;

	/* slurp the section headers */
	for ( i=0; i<pehdr->e_shnum; i++ ) {
		if ( pmelf_getshdr(s, rval->shdrs + i) )
			goto bail;
	}

	/* slurp the string table    */
	strsh = &rval->shdrs[pehdr->e_shstrndx];
	if ( ! (rval->strtab = pmelf_getscn( s, strsh, 0, 0, 0 )) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"(shstrtab)\n");
		goto bail;
	}

	rval->strtablen = strsh->sh_size;

	return rval;

bail:
	pmelf_delshtab(rval);
	return 0;
}
