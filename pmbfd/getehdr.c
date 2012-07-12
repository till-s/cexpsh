/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmelf' ELF file reader) was created by
 *     Till Straumann <strauman@slac.stanford.edu>, 2008,
 * 	   Stanford Linear Accelerator Center, Stanford University.
 * 
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 *     the Stanford Linear Accelerator Center, Stanford University,
 * 	   under Contract DE-AC03-76SFO0515 with the Department of Energy.
 * 
 * Government disclaimer of liability
 * ----------------------------------
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied, or
 * assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately owned
 * rights.
 * 
 * Stanford disclaimer of liability
 * --------------------------------
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * Stanford disclaimer of copyright
 * --------------------------------
 * Stanford University, owner of the copyright, hereby disclaims its
 * copyright and all other rights in this software.  Hence, anyone may
 * freely use it for any purpose without restriction.  
 * 
 * Maintenance of notices
 * ----------------------
 * In the interest of clarity regarding the origin and status of this
 * SLAC software, this and all the preceding Stanford University notices
 * are to remain affixed to any copy or derivative of this software made
 * or distributed by the recipient and are to be affixed to any copy of
 * software made or distributed by the recipient that contains a copy or
 * derivative of this software.
 * 
 * ------------------ SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 
#include "pmelfP.h"

static int
getehdr32_top(Elf_Stream s, Elf32_Ehdr *pehdr)
{
	if ( 1 != SREAD(&pehdr->e_type, sizeof(*pehdr) - EI_NIDENT, 1, s) ) {
		return -1;
	}

#ifndef PMELF_CONFIG_NO_SWAPSUPPORT
	if ( s->needswap ) {
		elf_swap16( &pehdr->e_type      );
		elf_swap16( &pehdr->e_machine   );
		elf_swap32( &pehdr->e_version   );
		elf_swap32( &pehdr->e_entry     );
		elf_swap32( &pehdr->e_phoff     );
		elf_swap32( &pehdr->e_shoff     );
		elf_swap32( &pehdr->e_flags     );
		elf_swap16( &pehdr->e_ehsize    );
		elf_swap16( &pehdr->e_phentsize );
		elf_swap16( &pehdr->e_phnum     );
		elf_swap16( &pehdr->e_shentsize );
		elf_swap16( &pehdr->e_shnum     );
		elf_swap16( &pehdr->e_shstrndx  );
	}
#endif

	if ( pehdr->e_shentsize != sizeof( Elf32_Shdr ) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: SH size mismatch %i"PRIu16"\n", pehdr->e_shentsize);
		return -1;
	}

	if ( pehdr->e_phentsize != sizeof( Elf32_Phdr ) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: PH size mismatch %i"PRIu16"\n", pehdr->e_phentsize);
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
	
#ifdef PMELF_CONFIG_ELF64SUPPORT
static int
getehdr64_top(Elf_Stream s, Elf64_Ehdr *pehdr)
{
	if ( 1 != SREAD(&pehdr->e_type, sizeof(*pehdr) - EI_NIDENT, 1, s) ) {
		return -1;
	}

#ifndef PMELF_CONFIG_NO_SWAPSUPPORT
	if ( s->needswap ) {
		elf_swap16( &pehdr->e_type      );
		elf_swap16( &pehdr->e_machine   );
		elf_swap32( &pehdr->e_version   );
		elf_swap64( &pehdr->e_entry     );
		elf_swap64( &pehdr->e_phoff     );
		elf_swap64( &pehdr->e_shoff     );
		elf_swap32( &pehdr->e_flags     );
		elf_swap16( &pehdr->e_ehsize    );
		elf_swap16( &pehdr->e_phentsize );
		elf_swap16( &pehdr->e_phnum     );
		elf_swap16( &pehdr->e_shentsize );
		elf_swap16( &pehdr->e_shnum     );
		elf_swap16( &pehdr->e_shstrndx  );
	}
#endif

	if ( pehdr->e_shentsize != sizeof( Elf64_Shdr ) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: SH size mismatch %i"PRIu16"\n", pehdr->e_shentsize);
		return -1;
	}

	if ( pehdr->e_phentsize != sizeof( Elf64_Phdr ) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: PH size mismatch %i"PRIu16"\n", pehdr->e_phentsize);
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
#endif
	
int
pmelf_getehdr(Elf_Stream s, Elf_Ehdr *pehdr)
{
uint8_t magic[4] = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };
int     rval;

	if ( pmelf_seek(s, 0) ) {
		return -1;
	}
	if ( 1 != SREAD(pehdr, EI_NIDENT, 1, s) ) {
		return -1;
	}
	if ( memcmp(magic, pehdr->e_ident+EI_MAG0, sizeof(magic)) ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"error: not an ELF file\n");
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
#endif

	switch ( (s->clss = pehdr->e_ident[EI_CLASS]) ) {
		case ELFCLASS32:
			if ( (rval = getehdr32_top(s,&pehdr->e32)) ) {
				return rval;
			}

			s->machine = pehdr->e32.e_machine;

			break;

		case ELFCLASS64:
#ifdef PMELF_CONFIG_ELF64SUPPORT
			if ( (rval = getehdr64_top(s,&pehdr->e64)) ) {
				return rval;
			}

			s->machine = pehdr->e64.e_machine;

			break;
#else
			PMELF_PRINTF(pmelf_err, PMELF_PRE"error: cannot read 64-bit ELF file; pmelf was configured and built without 64-bit support!\n");
			return -3;
#endif
		default:
			s->clss = ELFCLASSNONE;
			PMELF_PRINTF(pmelf_err, PMELF_PRE"error: not a 32/64-bit ELF file\n");
			return -1;
	}

	switch ( s->machine ) {
		case EM_SPARC:
		case EM_386:
		case EM_68K:
		case EM_PPC:
		case EM_X86_64:

		/* SUCCESS */
		return 0;

		default:
			PMELF_PRINTF(pmelf_err, PMELF_PRE"error: machine type 0x%04"PRIx16" not supported\n", s->machine);
		break;
	}
	return -1;
}
