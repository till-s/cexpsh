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

void
pmelf_dump_shtab(FILE *f, Pmelf_Shtab shtab, int format)
{
uint32_t i;
Elf_Shdr *shdr;
const char *name, *fmt;
uint8_t    *p;
uint32_t   shdrsz = get_shdrsz(shtab);

	if ( !f )
		f = stdout;

	fprintf(f,"Section Headers:\n");
#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( ELFCLASS64 == shtab->clss )
		fprintf(f,"  [Nr] %-18s%-16s%-17s%-7s%-7s%2s%4s%3s%4s%3s\n",
			"Name", "Type", "Address", "Off", "Size", "ES", "Flg", "Lk", "Inf", "Al");
	else
#endif
		fprintf(f,"  [Nr] %-18s%-16s%-9s%-7s%-7s%2s%4s%3s%4s%3s\n",
			"Name", "Type", "Addr", "Off", "Size", "ES", "Flg", "Lk", "Inf", "Al");

	for ( i = 0, p = shtab->shdrs.p_raw; i<shtab->nshdrs; i++, p+=shdrsz ) {
		shdr = (Elf_Shdr*)p;
		if ( ! (name = pmelf_sec_name(shtab, shdr)) ) {
			name = "<OUT-OF-BOUND>";
		}
		if ( FMT_COMPAT == format )
			fmt = "  [%2u] %-18.17s";
		else
			fmt = "  [%2u] %-18s";
		fprintf( f, fmt, i, name );
#ifdef PMELF_CONFIG_ELF64SUPPORT
		if ( ELFCLASS64 == shtab->clss )
			pmelf_dump_shdr64( f, &shdr->s64, format );
		else
#endif
			pmelf_dump_shdr32( f, &shdr->s32, format );
	}
	fprintf(f,"Key to Flags:\n");
	fprintf(f,"  W (write), A (alloc), X (execute), M (merge), S (strings)\n");
	fprintf(f,"  I (info), L (link order), G (group), ? (unknown)\n");
	fprintf(f,"  O (extra OS processing required)\n");
}
