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
		case EM_386: return "Intel x86";
		case EM_68K: return "Motorola 68k";
		case EM_PPC: return "PowerPC";
		default: break;
	}
	return "unknown";
}

static const char *pmelf_ehdr_abi_s[]={
	"SYSV",
};

void
pmelf_dump_ehdr(FILE *f, Elf32_Ehdr *pehdr)
{
	if ( !f )
		f = stdout;
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
