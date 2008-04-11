#include "pmelfP.h"

const char *
pmelf_sec_name(Pmelf_Elf32_Shtab sht, Elf32_Shdr *shdr)
{
	if ( shdr->sh_name > sht->strtablen )
		return 0;
	return &sht->strtab[shdr->sh_name];
}

