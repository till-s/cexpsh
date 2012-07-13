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

static const char *pmelf_phdr_type_s[] = {
	"NULL",
	"LOAD",
	"DYNAMIC",
	"INTERP",
	"NOTE",
	"SHLIB",
	"PHDR",
};

static const char *pmelf_gnu_phdr_type_s[] = {
	"GNU_EH_FRAME",
	"GNU_STACK",
	"GNU_RELRO",
};

void 
pmelf_dump_phdr32(FILE *f, Elf32_Phdr *pphdr, int format)
{
const char *fmt;
unsigned    idx;

	if ( !f )
		f = stdout;

	if ( FMT_COMPAT != format ) {
		fprintf( f, "(%1"PRIx32") ", pphdr->p_type );
		fmt = "%-14s";
	} else {
		fmt = "  %-14s";
	}

	if ( pphdr->p_type < ElfNumberOf(pmelf_phdr_type_s) ) {
		fprintf( f, fmt,
				ARRSTR(pmelf_phdr_type_s, pphdr->p_type)
		);
	} else if ( pphdr->p_type >= PT_GNU_EH_FRAME ) {
		idx = (unsigned) (pphdr->p_type - PT_GNU_EH_FRAME);

		fprintf( f, fmt,
				ARRSTR(pmelf_gnu_phdr_type_s, idx)
			   );
	}

	if ( FMT_LONG == format ) {
		fprintf( f, " 0x%06"PRIx32,  pphdr->p_offset);
		fprintf( f, " 0x%016"PRIx32, pphdr->p_vaddr);
		fprintf( f, " 0x%016"PRIx32, pphdr->p_paddr);
		fprintf( f, " 0x%06"PRIx32,  pphdr->p_filesz);
		fprintf( f, " 0x%06"PRIx32,  pphdr->p_memsz);
		fprintf( f, " %c%c%c",
			(pphdr->p_flags & PF_R) ? 'R' : ' ',
			(pphdr->p_flags & PF_W) ? 'W' : ' ',
			(pphdr->p_flags & PF_X) ? 'E' : ' '
		);
		fprintf( f, " 0x%1"PRIx32,  pphdr->p_align);
	} else {
		fprintf( f, " 0x%016"PRIx32, pphdr->p_offset);
		fprintf( f, " 0x%016"PRIx32, pphdr->p_vaddr);
		fprintf( f, " 0x%016"PRIx32"\n                ", pphdr->p_paddr);
		fprintf( f, " 0x%016"PRIx32,  pphdr->p_filesz);
		fprintf( f, " 0x%016"PRIx32,  pphdr->p_memsz);
		fprintf( f, " %c%c%c  ",
			(pphdr->p_flags & PF_R) ? 'R' : ' ',
			(pphdr->p_flags & PF_W) ? 'W' : ' ',
			(pphdr->p_flags & PF_X) ? 'E' : ' '
		);
		fprintf( f, " %1"PRIx32,  pphdr->p_align);
	}
	fprintf(f,"\n");

}

#ifdef PMELF_CONFIG_ELF64SUPPORT
void 
pmelf_dump_phdr64(FILE *f, Elf64_Phdr *pphdr, int format)
{
const char *fmt;
unsigned    idx;

	if ( !f )
		f = stdout;

	if ( FMT_COMPAT != format ) {
		fprintf( f, "(%1"PRIx32") ", pphdr->p_type );
		fmt = "%-14s";
	} else {
		fmt = "  %-14s";
	}

	if ( pphdr->p_type < ElfNumberOf(pmelf_phdr_type_s) ) {
		fprintf( f, fmt,
				ARRSTR(pmelf_phdr_type_s, pphdr->p_type)
		);
	} else if ( pphdr->p_type >= PT_GNU_EH_FRAME ) {
		idx = (unsigned) (pphdr->p_type - PT_GNU_EH_FRAME);

		fprintf( f, fmt,
				ARRSTR(pmelf_gnu_phdr_type_s, idx)
			   );
	}

	if ( FMT_LONG == format )
	{
		fprintf( f, " 0x%016"PRIx64, pphdr->p_offset);
		fprintf( f, " 0x%016"PRIx64, pphdr->p_vaddr);
		fprintf( f, " 0x%016"PRIx64"\n                ", pphdr->p_paddr);
		fprintf( f, " 0x%016"PRIx64,  pphdr->p_filesz);
		fprintf( f, " 0x%016"PRIx64,  pphdr->p_memsz);
		fprintf( f, " %c%c%c  ",
			(pphdr->p_flags & PF_R) ? 'R' : ' ',
			(pphdr->p_flags & PF_W) ? 'W' : ' ',
			(pphdr->p_flags & PF_X) ? 'E' : ' '
		);
		fprintf( f, " %1"PRIx64,  pphdr->p_align);
	}
	else
	{
		fprintf( f, " 0x%06"PRIx64,  pphdr->p_offset);
		fprintf( f, " 0x%016"PRIx64, pphdr->p_vaddr);
		fprintf( f, " 0x%016"PRIx64, pphdr->p_paddr);
		fprintf( f, " 0x%06"PRIx64,  pphdr->p_filesz);
		fprintf( f, " 0x%06"PRIx64,  pphdr->p_memsz);
		fprintf( f, " %c%c%c",
			(pphdr->p_flags & PF_R) ? 'R' : ' ',
			(pphdr->p_flags & PF_W) ? 'W' : ' ',
			(pphdr->p_flags & PF_X) ? 'E' : ' '
		);
		fprintf( f, " 0x%1"PRIx64,  pphdr->p_align);
	}
	fprintf(f,"\n");
}

#endif

int
pmelf_dump_phdrs(FILE *f, Elf_Stream s, int format)
{
int         i;
Elf_Ehdr    e;
Elf_Phdr    ph;
Pmelf_Off   curoff;

	if ( !f )
		f = stdout;

	if ( pmelf_getehdr( s, &e ) ) {
		return -1;
	}

	if ( FMT_LONG == format ) {
		fprintf(f, "  Type           Offset             VirtAddr           PhysAddr\n");
		fprintf(f, "                 FileSiz            MemSiz              Flags  Align\n");
	} else {
		fprintf(f,"  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align\n");
	}

#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( e.e_ident[EI_CLASS] == ELFCLASS64 ) 
	{
		if ( PN_XNUM == e.e64.e_phnum ) {
			fprintf(stderr,"Too many program headers; PN_XNUM not supported for now\n");
			return -1;
		}

		if ( pmelf_seek( s, e.e64.e_phoff ) ) 
			return -1;
		
		for ( i = 0; i < e.e64.e_phnum; i++ ) {
			if ( pmelf_getphdr64( s, &ph.p64 ) ) {
				return -1;
			}
			pmelf_dump_phdr64( f, &ph.p64, format );

			if ( PT_INTERP == ph.p64.p_type && 0 == pmelf_tell(s, &curoff) ) {
				if ( ph.p64.p_filesz > 1000 ) {
					fprintf(stderr,"pmelf_dump_phdr: file size of PT_INTERP seems too big; ignoring\n");
				} else {
					if ( 0 == pmelf_seek(s, ph.p64.p_offset) ) {
						char       *buf = 0;
						size_t      got;
						if ( (buf = malloc( ph.p64.p_filesz + 1) ) ) {
							if ( (got = SREAD(buf, 1, ph.p64.p_filesz, s)) > 0 ) {
								buf[got] = 0;
								fprintf( f, "      [Requesting program interpreter: %s]\n", buf);
							}
						}
						free(buf);
						pmelf_seek(s, curoff);
					}
				}
			}
		}
	}
	else
#endif
	{
		if ( ELFCLASS32 != e.e_ident[EI_CLASS] ) {
			fprintf(stderr,"INTERNAL ERROR -- unknown ELF class or library not configured for 64-bits\n");
			return -1;
		}
		if ( PN_XNUM == e.e32.e_phnum ) {
			fprintf(stderr,"Too many program headers; PN_XNUM not supported for now\n");
			return -1;
		}

		if ( pmelf_seek( s, e.e32.e_phoff ) ) 
			return -1;
		
		for ( i = 0; i < e.e32.e_phnum; i++ ) {
			if ( pmelf_getphdr32( s, &ph.p32 ) ) {
				return -1;
			}
			pmelf_dump_phdr32( f, &ph.p32, format );
			if ( PT_INTERP == ph.p32.p_type && 0 == pmelf_tell(s, &curoff) ) {
				if ( ph.p32.p_filesz > 1000 ) {
					fprintf(stderr,"pmelf_dump_phdr: file size of PT_INTERP seems too big; ignoring\n");
				} else {
					if ( 0 == pmelf_seek(s, ph.p32.p_offset) ) {
						char       *buf = 0;
						size_t      got;
						if ( (buf = malloc( ph.p32.p_filesz + 1) ) ) {
							if ( (got = SREAD(buf, 1, ph.p32.p_filesz, s)) > 0 ) {
								buf[got] = 0;
								fprintf( f, "      [Requesting program interpreter: %s]\n", buf);
							}
						}
						free(buf);
						pmelf_seek(s, curoff);
					}
				}
			}
		}
	}

	return 0;
}
