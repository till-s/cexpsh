#include "pmelfP.h"

int
pmelf_getehdr(Elf_Stream s, Elf32_Ehdr *pehdr)
{
uint8_t magic[4] = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };

	if ( pmelf_seek(s, 0) ) {
		return -1;
	}
	if ( 1 != fread(pehdr, sizeof(*pehdr), 1, s->f) ) {
		return -1;
	}
	if ( memcmp(magic, pehdr->e_ident+EI_MAG0, sizeof(magic)) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: not an ELF file\n");
		return -1;
	}
	if ( pehdr->e_ident[EI_CLASS] != ELFCLASS32 ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: not an 32-bit ELF file\n");
		return -1;
	}

	if ( pehdr->e_ident[EI_VERSION] != EV_CURRENT ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: not a version %i ELF file\n", EV_CURRENT);
		return -1;
	}

	s->needswap = iamlsb() ^ (pehdr->e_ident[EI_DATA] == ELFDATA2LSB ? 1 : 0);

#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
	if ( s->needswap ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: host/target byte order mismatch but pmelf was configured w/o support for byte-swapping\n");
		return -2;
	}
#else
	if ( s->needswap ) {
		e32_swap16( &pehdr->e_type      );
		e32_swap16( &pehdr->e_machine   );
		e32_swap32( &pehdr->e_version   );
		e32_swap32( &pehdr->e_entry     );
		e32_swap32( &pehdr->e_phoff     );
		e32_swap32( &pehdr->e_shoff     );
		e32_swap32( &pehdr->e_flags     );
		e32_swap16( &pehdr->e_ehsize    );
		e32_swap16( &pehdr->e_phentsize );
		e32_swap16( &pehdr->e_phnum     );
		e32_swap16( &pehdr->e_shentsize );
		e32_swap16( &pehdr->e_shnum     );
		e32_swap16( &pehdr->e_shstrndx  );
	}
#endif

	if ( pehdr->e_shentsize != sizeof( Elf32_Shdr ) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: SH size mismatch %i"PRIu16"\n", pehdr->e_shentsize);
		return -1;
	}

	if ( pehdr->e_shnum > 0 ) {
		if ( pehdr->e_shstrndx == 0 || pehdr->e_shstrndx >= pehdr->e_shnum ) {
			PMELF_PRINTF(pmelf_err, PMELF_PRE"error: shstrndx out of bounds\n");
			return -1;
		}
	}
	
	return 0;
}
