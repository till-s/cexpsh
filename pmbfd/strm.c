#include "pmelfP.h"

FILE *pmelf_err = 0;

Elf_Stream
pmelf_newstrm(char *name, FILE *f)
{
FILE *nf = 0;
Elf_Stream s;

	if ( ! f ) {
		if ( !(f = nf = fopen(name,"r")) ) {
			return 0;	
		}
	}
	if ( ! (s = calloc(1, sizeof(*s))) ) {
		if ( nf )
			fclose(nf);
		return 0;
	}
	s->f        = f;
	return s;
}

int
pmelf_seek(Elf_Stream s, Elf32_Off where)
{
	return fseek(s->f, where, SEEK_SET);
}

void
pmelf_delstrm(Elf_Stream s, int noclose)
{
	if ( !noclose && s->f )
		fclose(s->f);
	free(s);
}

void
pmelf_set_errstrm(FILE *f)
{
	pmelf_err = f;
}
