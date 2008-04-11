#include "pmelfP.h"

int
pmelf_getshdr(Elf_Stream s, Elf32_Shdr *pshdr)
{
	if ( 1 != fread(pshdr, sizeof(*pshdr), 1, s->f) ) {
		return -1;
	}
	if ( s->needswap ) {
#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
		return -2;
#else
		e32_swap32( &pshdr->sh_name);
		e32_swap32( &pshdr->sh_type);
		e32_swap32( &pshdr->sh_flags);
		e32_swap32( &pshdr->sh_addr);
		e32_swap32( &pshdr->sh_offset);
		e32_swap32( &pshdr->sh_size);
		e32_swap32( &pshdr->sh_link);
		e32_swap32( &pshdr->sh_info);
		e32_swap32( &pshdr->sh_addralign);
		e32_swap32( &pshdr->sh_entsize);
#endif
	}
#ifdef PARANOIA_ON
	{
	unsigned long x;
#if PARANOIA_ON > 0
	if ( (x = pshdr->sh_flags) & SHF_MSKSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getshdr - paranoia: unsupported flags 0x%08lx\n", x);
		return -1;
	}
#if PARANOIA_ON > 1
	if ( (x = pshdr->sh_type) > SHT_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getshdr - paranoia: unsupported type  0x%08lu\n", x);
		return -1;
	}
#endif
#endif
	}
#endif
	return 0;
}
