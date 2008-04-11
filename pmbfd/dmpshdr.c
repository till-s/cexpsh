#include "pmelfP.h"

static const char *pmelf_shdr_type_s[] = {
	"NULL",
	"PROGBITS",
	"SYMTAB",
	"STRTAB",
	"RELA",
	"HASH",
	"DYNAMIC",
	"NOTE",
	"NOBITS",
	"REL",
	"SHLIB",
	"DYNSYM",
	"RESERVED",
	"RESERVED",
	"INIT_ARRAY",
	"FINI_ARRAY",
	"PREINIT_ARRAY",
	"GROUP",
	"SYMTAB_SHNDX"
};

static const char pmelf_shdr_flags_c[] = {
	'W', 'A', 'X', '?', 'M', 'S', 'I', 'L', 'O', 'G'
};

static int
prflags(char *buf, Elf32_Word flags)
{
int i,m,l;
	for ( l=i=0, m=1; i<sizeof(flags)*8 && flags; i++, m<<=1 ) {
		if ( flags & m ) {
			flags &= ~m;
			buf[l++] = ARRCHR(pmelf_shdr_flags_c, i);
		}
	}
	buf[l]=0;
	return l;
}

void 
pmelf_dump_shdr(FILE *f, Elf32_Shdr *pshdr, int format)
{
char fbuf[4*sizeof(pshdr->sh_flags)+1];
const char *fmt;

	if ( !f )
		f = stdout;

	if ( FMT_COMPAT != format ) {
		fprintf( f, "(%1"PRIx32") ", pshdr->sh_type );
		fmt = "%-12s";
	} else {
		fmt = "%-16s";
	}

	fprintf( f, fmt,
		ARRSTR(pmelf_shdr_type_s, pshdr->sh_type), pshdr->sh_type
	);

	fprintf( f, "%08"PRIx32, pshdr->sh_addr);
	if ( FMT_LONG == format ) {
		fprintf( f, " %08"PRIx32, pshdr->sh_offset);
		fprintf( f, " %08"PRIx32, pshdr->sh_size);
		fprintf( f, " %08"PRIx32, pshdr->sh_link);
		fprintf( f, " %05"PRIx32, pshdr->sh_addralign);
		fprintf( f, " %05"PRIx32, pshdr->sh_entsize);
		fprintf( f, "\n flags: 0x%08"PRIx32, pshdr->sh_flags);
		if ( pshdr->sh_flags ) {
			fprintf(f, "[");
			if ( SHF_WRITE & pshdr->sh_flags ) fprintf(f, " WRITE");
			if ( SHF_ALLOC & pshdr->sh_flags ) fprintf(f, " ALLOC");
			if ( SHF_EXECINSTR & pshdr->sh_flags ) fprintf(f, " EXECINSTR");
			if ( SHF_MERGE & pshdr->sh_flags ) fprintf(f, " MERGE");
			if ( SHF_STRINGS & pshdr->sh_flags ) fprintf(f, " STRINGS");
			if ( SHF_INFO_LINK & pshdr->sh_flags ) fprintf(f, " INFO_LINK");
			if ( SHF_LINK_ORDER & pshdr->sh_flags ) fprintf(f, " LINK_ORDER");
			if ( SHF_OS_NONCONFORMING & pshdr->sh_flags ) fprintf(f, " OS_NONCONFORMING");
			if ( SHF_GROUP & pshdr->sh_flags ) fprintf(f, " GROUP");
			fprintf(f, " ]");
		}
	} else {
		fprintf( f, " %06"PRIx32, pshdr->sh_offset);
		fprintf( f, " %06"PRIx32, pshdr->sh_size);
		fprintf( f, " %02"PRIx32, pshdr->sh_entsize);
		prflags(fbuf, pshdr->sh_flags);
		fprintf( f, "%4s",fbuf);
		fprintf( f, " %2" PRIu32, pshdr->sh_link);
		fprintf( f, " %3" PRIu32, pshdr->sh_info);
		fprintf( f, " %2" PRIu32, pshdr->sh_addralign);
	}
	fprintf(f,"\n");
}
