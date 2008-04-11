
#include "pmelfP.h"

const char *
pmelf_sym_name(Pmelf_Elf32_Symtab symt, Elf32_Sym *sym)
{
	if ( sym->st_name > symt->strtablen )
		return 0;
	return &symt->strtab[sym->st_name];
}
