#ifndef PMELF_PRIVATE_H
#define PMELF_PRIVATE_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelf.h"

#define PARANOIA_ON 0
#undef PARANOIA_ON

struct _Elf_Stream {
	FILE *f;
	int  needswap;
};

extern FILE *pmelf_err;

#define PMELF_PRE    "pmelf: "
#define PMELF_PRINTF(f,a...) do { if (f) fprintf(f,a); } while (0)

#define ElfNumberOf(a) (sizeof(a)/sizeof((a)[0]))

static inline int iamlsb()
{
union {
	char b[2];
	short s;
} tester = { s: 1 };

	return tester.b[0] ? 1 : 0;
}

static inline uint16_t e32_swab( uint16_t v )
{
	return ( (v>>8) | (v<<8) );
}

static inline void e32_swap16( uint16_t *p )
{
	*p = e32_swab( *p );
}

static inline void e32_swap32( uint32_t *p )
{
register uint16_t vl = *p;
register uint16_t vh = *p>>16;
	*p = (e32_swab( vl ) << 16 ) | e32_swab ( vh ); 
}

#define ARRSTR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : "unknown" )
#define ARRCHR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : '?' )

#endif
