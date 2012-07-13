#include <pmelf.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
Elf_Stream s = 0;
int        rval = 1;
Pmelf_Shtab shtab = 0;
Elf_Ehdr    ehdr;

	if ( argc < 2 ) {
		fprintf(stderr,"%s: need filname arg\n", argv[0]);
		goto bail;
	}
	if ( ! (s = pmelf_newstrm(argv[1], 0) ) ) {
		fprintf(stderr,"Unable to open PMELF stream\n");
		goto bail;
	}

	if ( pmelf_getehdr( s, &ehdr ) ) {
		fprintf(stderr,"Unable to read ELF header\n");
		goto bail;
	}

	if ( ! (shtab = pmelf_getshtab( s, &ehdr )) ) {
		fprintf(stderr,"Unable to read SHTAB\n");
		goto bail;
	}

	printf("Dynamic symbols: %lu\n", pmelf_find_dsymhdrs(s, shtab, 0, 0));
	printf("Symbols        : %lu\n", pmelf_find_symhdrs(s, shtab,  0, 0));
	

	rval = 0;

bail:
	if ( shtab ) {
		pmelf_delshtab(shtab);
	}
	if ( s ) {
		pmelf_delstrm( s, 0 );
	}

	return rval;
}
