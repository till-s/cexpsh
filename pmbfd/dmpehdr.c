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

static const char *pmelf_ehdr_type_s[] = {
"none",
"relocatable",
"executable",
"shared object",
"core",
"processor specific (lo)",
"processor specific (hi)"
};

static const char *pmelf_ehdr_machine_s(Elf32_Half idx)
{
	switch ( idx ) {
		case EM_SPARC:  return "SPARC";
		case EM_386:    return "Intel x86";
		case EM_68K:    return "Motorola 68k";
		case EM_PPC:    return "PowerPC";
		case EM_X86_64: return "AMD x86-64";
		case EM_ARM:    return "ARM";
		default: break;
	}
	return "unknown";
}

static const char *pmelf_ehdr_abi_s[]={
	"SYSV",
};

void
pmelf_dump_ehdr(FILE *f, Elf_Ehdr *pehdru)
{
	if ( !f )
		f = stdout;

#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( pehdru->e_ident[EI_CLASS] == ELFCLASS64 )
	{
		Elf64_Ehdr *pehdr = &pehdru->e64;

		fprintf( f, "Class        : ELF64\n");
		fprintf( f, "File    type : %s (%u)\n", ARRSTR(pmelf_ehdr_type_s,    pehdr->e_type), pehdr->e_type );
		fprintf( f, "Machine type : %s\n", pmelf_ehdr_machine_s(pehdr->e_machine) );
		fprintf( f, "ABI          : %s (%u)\n", ARRSTR(pmelf_ehdr_abi_s, pehdr->e_ident[EI_OSABI]), pehdr->e_ident[EI_OSABI]);
		fprintf( f, "Version      : %"PRIu32"\n",     pehdr->e_version );

		fprintf( f, "Entry point  : 0x%08"PRIx64"\n", pehdr->e_entry );

		fprintf( f, "PH offset    : %"    PRIu64"\n", pehdr->e_phoff  );
		fprintf( f, "SH offset    : %"    PRIu64"\n", pehdr->e_shoff  );
		fprintf( f, "Flags        : 0x%08"PRIx32"\n", pehdr->e_flags  );
		fprintf( f, "EH size      : %"    PRIu16"\n", pehdr->e_ehsize );
		fprintf( f, "PH entry size: %"    PRIu16"\n", pehdr->e_phentsize );
		fprintf( f, "# of PH      : %"    PRIu16"\n", pehdr->e_phnum );
		fprintf( f, "SH entry size: %"    PRIu16"\n", pehdr->e_shentsize );
		fprintf( f, "# of SH      : %"    PRIu16"\n", pehdr->e_shnum );
		fprintf( f, "SHSTR index  : %"    PRIu16"\n", pehdr->e_shstrndx );
	}
	else 
#endif
	{
		Elf32_Ehdr *pehdr = &pehdru->e32;

		fprintf( f, "Class        : ELF32\n");
		fprintf( f, "File    type : %s (%u)\n", ARRSTR(pmelf_ehdr_type_s,    pehdr->e_type), pehdr->e_type );
		fprintf( f, "Machine type : %s\n", pmelf_ehdr_machine_s(pehdr->e_machine) );
		fprintf( f, "ABI          : %s (%u)\n", ARRSTR(pmelf_ehdr_abi_s, pehdr->e_ident[EI_OSABI]), pehdr->e_ident[EI_OSABI]);
		fprintf( f, "Version      : %"PRIu32"\n",     pehdr->e_version );

		fprintf( f, "Entry point  : 0x%08"PRIx32"\n", pehdr->e_entry );

		fprintf( f, "PH offset    : %"    PRIu32"\n", pehdr->e_phoff  );
		fprintf( f, "SH offset    : %"    PRIu32"\n", pehdr->e_shoff  );
		fprintf( f, "Flags        : 0x%08"PRIx32"\n", pehdr->e_flags  );
		fprintf( f, "EH size      : %"    PRIu16"\n", pehdr->e_ehsize );
		fprintf( f, "PH entry size: %"    PRIu16"\n", pehdr->e_phentsize );
		fprintf( f, "# of PH      : %"    PRIu16"\n", pehdr->e_phnum );
		fprintf( f, "SH entry size: %"    PRIu16"\n", pehdr->e_shentsize );
		fprintf( f, "# of SH      : %"    PRIu16"\n", pehdr->e_shnum );
		fprintf( f, "SHSTR index  : %"    PRIu16"\n", pehdr->e_shstrndx );
	}
}
