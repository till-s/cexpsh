#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelf.h"

#define PARANOIA_ON 0

#define DO_EHDR		1
#define DO_SHDRS	2
#define DO_SYMS		4
#define DO_GROUP	8

#include <getopt.h>

static char *optstr="achSsHg";

static void usage(char *nm)
{
	fprintf(stderr,"usage: %s [-%s] <elf_file>\n",nm,optstr);
	fprintf(stderr,"   -H: print this information\n");
	fprintf(stderr,"   -h: print file header (ehdr)\n");
	fprintf(stderr,"   -S: print section headers (shdr)\n");
	fprintf(stderr,"   -g: print section groups\n");
	fprintf(stderr,"   -s: print symbol table\n");
	fprintf(stderr,"   -a: enable all of -h, -s, -S, -g\n");
	fprintf(stderr,"   -c: enable 'readelf' compatibility\n");

}

int main(int argc, char ** argv)
{
int              rval = 1;
Elf_Stream       s;
Elf32_Ehdr       ehdr;
Txx_Elf32_Shtab  shtab = 0;
Txx_Elf32_Symtab symtab = 0;
int              ch;
int              doit=0;
int              compat=0;

	txx_set_errstrm(stderr);

	while ( (ch=getopt(argc, argv, optstr)) >= 0 ) {
		switch (ch) {
			case 'c': compat=1;          break;
			case 'h': doit |= DO_EHDR;   break;
			case 'S': doit |= DO_SHDRS;  break;
			case 's': doit |= DO_SYMS;   break;
			case 'g': doit |= DO_GROUP;  break;
			case 'a': doit |= -1;        break;
			case 'H': usage(argv[0]);    return 0;
			default:
				break;
		}
	}

	if ( optind >= argc ) {
		fprintf(stderr, "Not enough arguments -- need filename\n");
		usage(argv[0]);
		return 1;
	}

	if ( ! (s = txx_newstrm(argv[optind], 0)) ) {
		perror("Creating ELF stream");
		return 1;
	}

	if ( txx_getehdr( s, &ehdr ) ) {
		perror("Reading ELF header");
		goto bail;
	}

	if ( doit & DO_EHDR )
		txx_dump_ehdr( stdout, &ehdr );

	if ( ! (shtab = txx_getshtab(s, &ehdr)) )
		goto bail;
		
	if ( doit & DO_SHDRS )
		txx_dump_shtab( stdout, shtab, compat ? FMT_COMPAT : FMT_SHORT );

	if ( ! (symtab = txx_getsymtab(s, shtab)) ) {
		goto bail;
	}

	if ( doit & DO_GROUP )
		txx_dump_groups( stdout, s, shtab, symtab);

	if ( doit & DO_SYMS )
		txx_dump_symtab( stdout, symtab, compat ? 0 : shtab, compat ? FMT_COMPAT : FMT_SHORT );

	rval = 0;
bail:
	txx_delsymtab(symtab);
	txx_delshtab(shtab);
	txx_delstrm(s);
	return rval;
}
