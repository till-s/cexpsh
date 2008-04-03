#if 0
#include <bfd.h>
#include <elf-bfd.h>
#define bfd_get_section_filepos(abfd, sect) (sect)->filepos
#else
#include "pmbfd.h"
#endif
#include <stdio.h>
/*
#include "pmelf.h"
*/

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

	printf("  %08lx", bfd_get_section_filepos(abfd, sect));

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

#if 0
	txx_dump_shdr(stdout, (void*)&(elf_section_data(sect)->this_hdr), 2/*FMT_COMPAT*/);
#endif
	*(int*)arg = n+1;
}

#undef PFLG
#undef PFLG1
#undef PDUP

int
main(int argc, char **argv)
{
bfd *abfd;
FILE *f;
int  rval = 1;
int  n;

	if ( argc < 2 ) {
		fprintf(stderr,"Need filename arg\n");
		return 1;
	}
	if ( !(f=fopen(argv[1],"r")) ) {
		perror("unable to open file for reading");
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

	n = 0;
	bfd_map_over_sections(abfd, dsects, &n);

	rval = 0;

#if 0
cleanup:
#endif
	bfd_close_all_done(abfd);
	return rval;
}
