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

#ifdef PMELF_CONFIG_ELF64SUPPORT
void
pmelf_dump_sym64(FILE *f, Elf64_Sym *sym, Pmelf_Shtab shtab, const char *strtab, unsigned strtablen, int format)
{
int        donestr;
const char *name;

	if ( !f )
		f = stdout;

	fprintf(f,"%016"PRIx64, sym->st_value);

	if ( sym->st_size > 99999 )
		fprintf(f," 0x%5"PRIx64, sym->st_size);
	else
		fprintf(f," %5"PRIu64, sym->st_size);

	/*
     * visibility should not be printed with -3s but -8s 
     * in order to fit but that's what readelf does :-(
     */
	fprintf(f," %-8s%-7s%-3s ",
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
							if ( !(name = pmelf_sec_name(shtab, get_shtabN(shtab, sym->st_shndx))) ) {
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
#endif

void
pmelf_dump_sym32(FILE *f, Elf32_Sym *sym, Pmelf_Shtab shtab, const char *strtab, unsigned strtablen, int format)
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

	/*
     * visibility should not be printed with -3s but -8s 
     * in order to fit but that's what readelf does :-(
     */
	fprintf(f," %-8s%-7s%-3s ",
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
							if ( !(name = pmelf_sec_name(shtab, get_shtabN(shtab, sym->st_shndx))) ) {
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
