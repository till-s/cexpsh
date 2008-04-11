#include "pmelfP.h"

Elf32_Word *
pmelf_getgrp(Elf_Stream s, Elf32_Shdr *psect, Elf32_Word *data)
{
Elf32_Word *buf;
int        ne,j;

	if ( SHT_GROUP != psect->sh_type )
		return 0;

	if ( psect->sh_size & (sizeof(*buf)-1) ) {
		PMELF_PRINTF(pmelf_err, "group section size not a multiple of %u\n", sizeof(*buf));
		return 0;
	}

	ne  = psect->sh_size/sizeof(*buf);

	if ( ne < 1 ) {
		PMELF_PRINTF(pmelf_err, "group has no members and not even a flag word\n");
		return 0;
	}

	if ( ! (buf = pmelf_getscn(s, psect, data, 0, 0)) )
		return 0;

	if ( s->needswap ) {
#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
		return -2;
#else
		for ( j=0; j < ne; j++ ) {
			e32_swap32( &buf[j] );
		}
#endif
	}

	return buf;
}
