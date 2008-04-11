#include "pmelfP.h"

int
pmelf_getsym(Elf_Stream s, Elf32_Sym *psym)
{
	if ( 1 != fread(psym, sizeof(*psym), 1, s->f) ) {
		return -1;
	}
	if ( s->needswap ) {
#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
		return -2;
#else
		e32_swap32( &psym->st_name);
		e32_swap32( &psym->st_value);
		e32_swap32( &psym->st_size);
		e32_swap16( &psym->st_shndx);
#endif
	}
#ifdef PARANOIA_ON
	{
	int x;
	if ( (x = ELF32_ST_TYPE( psym->st_info )) > STT_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsym - paranoia: unsupported type %i\n", x);
		return -1;
	}
	if ( (x = ELF32_ST_BIND( psym->st_info )) > STB_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsym - paranoia: unsupported binding %i\n", x);
		return -1;
	}
	}
#endif
	return 0;
}
