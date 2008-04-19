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
prflags(char *buf, Elf64_Xword flags)
{
int i,l;
Elf64_Xword m;
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
pmelf_dump_shdr32(FILE *f, Elf32_Shdr *pshdr, int format)
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

#ifdef PMELF_CONFIG_ELF64SUPPORT
void 
pmelf_dump_shdr64(FILE *f, Elf64_Shdr *pshdr, int format)
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

	fprintf( f, "%016"PRIx64, pshdr->sh_addr);
	if ( FMT_LONG == format ) {
		fprintf( f, " %08"PRIx64, pshdr->sh_offset);
		fprintf( f, " %08"PRIx64, pshdr->sh_size);
		fprintf( f, " %08"PRIx32, pshdr->sh_link);
		fprintf( f, " %05"PRIx64, pshdr->sh_addralign);
		fprintf( f, " %05"PRIx64, pshdr->sh_entsize);
		fprintf( f, "\n flags: 0x%08"PRIx64, pshdr->sh_flags);
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
		fprintf( f, " %06"PRIx64, pshdr->sh_offset);
		fprintf( f, " %06"PRIx64, pshdr->sh_size);
		fprintf( f, " %02"PRIx64, pshdr->sh_entsize);
		prflags(fbuf, pshdr->sh_flags);
		fprintf( f, "%4s",fbuf);
		fprintf( f, " %2" PRIu32, pshdr->sh_link);
		fprintf( f, " %3" PRIu32, pshdr->sh_info);
		fprintf( f, " %2" PRIu64, pshdr->sh_addralign);
	}
	fprintf(f,"\n");
}

#endif
