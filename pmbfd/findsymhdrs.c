#include "pmelfP.h"

long
pmelf_find_symhdrs(Elf_Stream s, Pmelf_Elf32_Shtab shtab, Elf32_Shdr **psymsh, Elf32_Shdr **pstrsh)
{
Elf32_Sym    *sym;
Elf32_Shdr   *shdr;
uint32_t     i;
Elf32_Shdr   *symsh  = 0;
Elf32_Shdr   *strsh  = 0;
const char   *name;

	for ( i = 0, shdr = shtab->shdrs; i<shtab->nshdrs; i++, shdr++ ) {
		switch ( shdr->sh_type ) {
			default:
			break;

			case SHT_SYMTAB:
				if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: symtab section name out of bounds\n");
					return -1;
				}
				if ( !strcmp(".symtab", name) ) {
					if ( symsh ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: multiple symtabs\n");
						return -1;
					}
					symsh = shdr;
#define USE_SYM_SH_LINK
#ifdef USE_SYM_SH_LINK
					strsh = shtab->shdrs + symsh->sh_link;
#endif
				}
			break;

#ifndef USE_SYM_SH_LINK
			case SHT_STRTAB:
				if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: strtab section name out of bounds\n");
					return -1;
				}
				if ( !strcmp(".strtab", name) ) {
					if ( strsh ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: multiple strtabs\n");
						return -1;
					}
					strsh = shdr;
				}
			break;
#endif
		}
	}

	if ( !symsh ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: no symtab found\n");
		return -1;
	}

	if ( !strsh ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: no strtab found\n");
		return -1;
	}

	if ( 0 == symsh->sh_size ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: zero size symtab\n");
		return -1;
	}

	if ( symsh->sh_entsize && symsh->sh_entsize != sizeof( *sym ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: symbol size mismatch (sh_entsize %"PRIu32"\n", symsh->sh_entsize);
		return -1;
	}

	if ( (symsh->sh_size % sizeof(*sym)) != 0 ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs: sh_size of symtab not a multiple of sizeof(Elf32_Sym)\n");
		return -1;
	}

	if ( pmelf_seek(s, symsh->sh_offset) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_find_symhdrs unable to seek to symtab %s\n", strerror(errno));
		return -1;
	}

	if ( psymsh )
		*psymsh = symsh;
	if ( pstrsh )
		*pstrsh = strsh;

	return symsh->sh_size/sizeof(*sym);
}
