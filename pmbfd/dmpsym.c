#include "pmelfP.h"

static const char *pmelf_st_bind_s[] = {
	"LOCAL",
	"GLOBAL",
	"WEAK"
};

static const char *pmelf_st_type_s[] = {
	"NOTYPE",
	"OBJECT",
	"FUNC",
	"SECTION",
	"FILE"
};

static const char *pmelf_st_vis_s[] = {
	"DEFAULT",
	"INTERNAL",
	"HIDDEN",
	"PROTECTED"
};

void
pmelf_dump_sym(FILE *f, Elf32_Sym *sym, Pmelf_Elf32_Shtab shtab, const char *strtab, unsigned strtablen, int format)
{
int        donestr;
const char *name;

	if ( !f )
		f = stdout;
	fprintf(f,"%08"PRIx32, sym->st_value);
	if ( sym->st_size > 99999 )
		fprintf(f," 0x%5"PRIx32, sym->st_size);
	else
		fprintf(f," %5"PRIu32, sym->st_size);
	fprintf(f," %-8s%-7s%-8s",
			ARRSTR(pmelf_st_type_s, ELF32_ST_TYPE(sym->st_info)),
			ARRSTR(pmelf_st_bind_s, ELF32_ST_BIND(sym->st_info)),
			ARRSTR(pmelf_st_vis_s,  ELF32_ST_VISIBILITY(sym->st_other))
		   );

	donestr = 0;

	switch ( sym->st_shndx ) {
		case SHN_UNDEF: fprintf(f, " UND"); break;
		case SHN_ABS:	fprintf(f, " ABS"); break;
		case SHN_COMMON:fprintf(f, " COM"); break;
		default:
						if ( sym->st_shndx < SHN_LORESERVE && shtab && sym->st_shndx < shtab->nshdrs ) {
							if ( !(name = pmelf_sec_name(shtab, &shtab->shdrs[sym->st_shndx])) ) {
								name = "<OUT-OF-BOUNDS>";
							}
							fprintf(f," %-13s", name);
							donestr = 1;
						} else {
							fprintf(f, "%4"PRIu16, sym->st_shndx);
						}
						break;
	}

	if ( shtab && !donestr )
		fprintf(f,"         ");

	if ( !strtab || sym->st_name >= strtablen ) {
		name = "<OUT-OF-BOUNDS>";
	} else {
		name = &strtab[sym->st_name];
	}
	fprintf(f, FMT_COMPAT == format ? " %.25s\n" : " %s\n", name);
}
