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

#ifdef USE_REAL_BFD
#include <sys/types.h>
#include <bfd.h>
#include <elf-bfd.h>
#define get_section_filepos(abfd, sect) (sect)->filepos

static inline unsigned
elf_get_align(bfd *abfd, asymbol *sp)
{
elf_symbol_type *esp;
unsigned         rval;
	if ( (esp = elf_symbol_from(abfd, sp)) && (rval = esp->internal_elf_sym.st_value) ) {
		return rval;
	}
	return  1;
}

static inline int
elf_get_size(bfd *abfd, asymbol *asym)
{
elf_symbol_type *elfsp= elf_symbol_from(abfd, asym);
	return elfsp ? elfsp->internal_elf_sym.st_size : 0 ;
}

#else
#include "pmbfd.h"
#define get_section_filepos(abfd, sect) pmbfd_get_section_filepos(abfd,sect)
#endif
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#define PFLG(tst,nm) \
	if ( fl & tst ) { fl &= ~tst; printf("%s%s",sep,nm); sep=", "; }
#define PFLG1(x) PFLG(SEC_##x,#x)

#define PDUP(x) \
	case SEC_LINK_DUPLICATES_##x: printf("%sLINK_ONCE_"#x,sep); break;

static void
dsects(bfd *abfd, asection *sect, PTR arg)
{
int       n = *(int*)arg;
flagword fl = bfd_get_section_flags(abfd, sect);
char   *sep = "";

	printf("%3i %-13s %08lx  %08lx  %08lx",
		n,
		bfd_get_section_name(abfd, sect),
		bfd_get_section_size(sect),
		bfd_get_section_vma(abfd, sect),
		bfd_get_section_lma(abfd, sect)
	);

	printf("  %08lx", (unsigned long)get_section_filepos(abfd, sect));

	/* WEIRDNESS (bug ?) if I tried a single printf statement the alignment
	 * was always printed as 0
	 */
	printf("  2**%u\n", bfd_get_section_alignment(abfd, sect));

	printf("%-18s","");
#ifdef SEC_HAS_CONTENTS
	PFLG( SEC_HAS_CONTENTS, "CONTENTS" );
#endif
	PFLG1( ALLOC );
#ifdef SEC_CONSTRUCTOR
	PFLG1( CONSTRUCTOR );
#endif
#ifdef SEC_LOAD
	PFLG1( LOAD );
#endif
#ifdef SEC_RELOC
	PFLG1( RELOC );
#endif
#ifdef SEC_READONLY
	PFLG1( READONLY );
#endif
#ifdef SEC_CODE
	PFLG1( CODE );
#endif
#ifdef SEC_DATA
	PFLG1( DATA );
#endif
#ifdef SEC_ROM
	PFLG1( ROM );
#endif
#ifdef SEC_DEBUGGING
	PFLG1( DEBUGGING );
#endif
#ifdef SEC_EXCLUDE
	PFLG1( EXCLUDE );
#endif
#ifdef SEC_SORT_ENTRIES
	PFLG1( SORT_ENTRIES );
#endif
#ifdef SEC_SMALL_DATA
	PFLG1( SMALL_DATA );
#endif
#ifdef SEC_GROUP
	PFLG1( GROUP );
#endif

	if ( fl & SEC_LINK_ONCE ) {
		/* seems currently broken in bfd (2.17); SEC_LINK_DUPLICATES is
		 * not a bitmask selecing the different flavors...
		 */
		switch ( fl & SEC_LINK_DUPLICATES ) {
#ifdef SEC_LINK_DUPLICATES_DISCARD
			PDUP(DISCARD)
#endif
#ifdef SEC_LINK_DUPLICATES_ONE_ONLY
			PDUP(ONE_ONLY)
#endif
#ifdef SEC_LINK_DUPLICATES_SAME_SIZE
			PDUP(SAME_SIZE)
#endif
#ifdef SEC_LINK_DUPLICATES_SAME_CONTENTS
			PDUP(SAME_CONTENTS)
#endif
			default:
				fprintf(stderr,"ERROR: Unkown 'link duplicates variant'\n");
			break;
		}
		fl &= ~(SEC_LINK_ONCE | SEC_LINK_DUPLICATES);
	}

	printf("\n");

#if 0
	/* there is stuff set... */
	if ( fl ) {
		fprintf(stderr,"************* unkown flags set: 0x%08x ******************\n", fl);
	}
#endif

	*(int*)arg = n+1;
}

#undef PFLG
#undef PFLG1
#undef PDUP

static void
usage(char *nm)
{
	fprintf(stderr,"usage: %s [-thH] <elf_file>\n",nm);
	fprintf(stderr,"       -t : print symbol table\n");
	fprintf(stderr,"       -h : print section table\n");
	fprintf(stderr,"       -H : this message\n");
}

#define DO_SECS 1
#define DO_SYMS 2

int
main(int argc, char **argv)
{
bfd *abfd;
FILE *f;
int  rval = 1;
int  n;
asymbol **syms;
long     nsyms;
flagword fl;
asymbol  *s;
int doit=0;

	while ( (n = getopt(argc, argv, "thH")) > 0 ) {
		switch (n) {
			case 'H':
				usage(argv[0]);
			return 0;

			case 't': doit |= DO_SYMS;
			break;

			case 'h': doit |= DO_SECS;
			break;
		
			default:
				fprintf(stderr,"Unknown option %c\n",n);
				usage(argv[0]);
			return 1;
		}
	}

	if ( optind >= argc ) {
		usage(argv[0]);
		return 1;
	}
	if ( !(f=fopen(argv[optind],"r")) ) {
		bfd_perror("unable to open file for reading");
		return 1;
	}
	bfd_init();
	if ( !(abfd=bfd_openstreamr(argv[1],0,f)) ) {
		bfd_perror("Unable to attach BFD");
		return 1;
	}
	if ( ! bfd_check_format(abfd, bfd_object) ) {
		fprintf(stderr,"Warning: not a relocatable object file\n");
	}

#if 0
	if ( !bfd_get_arch_info(abfd) ) {
		fprintf(stderr, "Unable to determine target architecture\n");
		goto cleanup;
	}
#endif

	printf("Target: %s\n",bfd_get_target(abfd));

	if ( DO_SECS & doit ) {
		printf("Sections:\n");
		printf("Idx Name          Size      VMA       LMA       File off  Algn\n");

		n = 0;
		bfd_map_over_sections(abfd, dsects, &n);
	}

	if ( DO_SYMS & doit ) {
		if ( ! (syms = malloc(bfd_get_symtab_upper_bound(abfd))) ) {
			fprintf(stderr,"Unable to allocate memory for symbol table\n");
			return 1;	
		}

		if ( (nsyms=bfd_canonicalize_symtab(abfd, syms)) < 0 ) {
			fprintf(stderr,"Canonicalizing symtab failed\n");
			return 1;
		}

		for ( n=0; n<nsyms; n++ ) {
			asection *symsec;
			s  = syms[n];
			fl = s->flags;
			symsec = bfd_get_section(s);
			printf("%08lx", bfd_asymbol_value(s));
#if 0
			printf(" %c%c%c%c%c%c%c",
					((fl & BSF_LOCAL) ? (fl & BSF_GLOBAL) ? '!' : 'l' : ( fl & BSF_GLOBAL ) ? 'g' : ' '),
					(fl & BSF_WEAK)        ? 'w' : ' ',
					(fl & BSF_CONSTRUCTOR) ? 'C' : ' ',
					(fl & BSF_WARNING)     ? 'W' : ' ',
					(fl & BSF_INDIRECT)    ? 'I' : ' ',
					(fl & BSF_DEBUGGING)   ? 'd' : (fl & BSF_DYNAMIC) ? 'D' : ' ',
					((fl & BSF_FUNCTION)   ? 'F' : ((fl & BSF_FILE)   ? 'f' : ((fl & BSF_OBJECT) ? 'O' : ' ')))
				  );
#else
			printf(" %c%c   %c%c",
					((fl & BSF_LOCAL) ? (fl & BSF_GLOBAL) ? '!' : 'l' : ( fl & BSF_GLOBAL ) ? 'g' : ' '),
					(fl & BSF_WEAK)        ? 'w' : ' ',
					(fl & BSF_DEBUGGING)   ? 'd' : ' ',
					((fl & BSF_FUNCTION)   ? 'F' : ((fl & BSF_FILE)   ? 'f' : ((fl & BSF_OBJECT) ? 'O' : ' ')))
				  );
#endif
			printf(" %s\t", symsec ? bfd_get_section_name(abfd,symsec) : "(*none*)");
			if ( bfd_is_com_section(symsec) )
				printf("%08lx", (unsigned long)elf_get_align(abfd, s));
			else {
				printf("%08lx", (unsigned long)elf_get_size(abfd,s));
			}
			/* FIXME: print st_other */
			printf(" %s\n", s->name);
		}
	}

	rval = 0;

#if 0
cleanup:
#endif
	bfd_close_all_done(abfd);
	return rval;
}
