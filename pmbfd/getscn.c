#include "pmelfP.h"

void *
pmelf_getscn(Elf_Stream s, Elf32_Shdr *psect, void *data, Elf32_Off offset, Elf32_Word len)
{
void       *buf = 0;
Elf32_Word end;

	if ( 0 == len )
		len = psect->sh_size;

	end = offset + len;

	if ( end < offset || end < len ) {
		/* wrap around 0 */
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn invalid offset/length (too large)\n");
		return 0;
	}

	if ( end > psect->sh_size ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn invalid offset/length (requested area not entirely within section)\n");
		return 0;
	}

	if ( pmelf_seek(s, psect->sh_offset + offset) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn unable to seek %s", strerror(errno));
		return 0;
	}

	if ( !data ) {
		buf = malloc(len);
		if ( !buf ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn - no memory");
			return 0;
		}
		data = buf;
	}

	if ( len != fread( data, 1, len, s->f ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getscn unable to read %s", strerror(errno));
		free(buf);
		return 0;
	}

	return data;
}
