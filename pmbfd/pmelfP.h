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
#ifndef PMELF_PRIVATE_H
#define PMELF_PRIVATE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelf.h"

#define PARANOIA_ON 0
#undef PARANOIA_ON

/* NOTE: we currently don't fully support 64-bit targets on a 32-bit host.
 *       otherwise we need to use explicit 64-bit types for sizes and
 *       offsets.
 */

struct _Elf_Stream {
	void    *f;
	 char    *name;
	size_t  (*read) (void *buf, size_t size, size_t nmemb, void* stream);
	size_t  (*write)(const void *buf, size_t size, size_t nmemb, void* stream);
	int     (*seek) (void* stream, off_t offset, int whence);
	off_t   (*tell) (void *stream);
	int     (*close)(void* s);
	uint16_t machine;
	uint8_t  needswap;
	uint8_t  clss;
};

#define SREAD(b,sz,n,s)  (s)->read((b),(sz),(n),(s)->f)
#define SWRITE(b,sz,n,s) (s)->write((b),(sz),(n),(s)->f)

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

#ifndef PMELF_CONFIG_NO_SWAPSUPPORT
static inline uint16_t elf_swab( uint16_t v )
{
	return ( (v>>8) | (v<<8) );
}

static inline uint32_t elf_swah( uint32_t v )
{
	return elf_swab( v>>16 ) | ( elf_swab( v & 0xffff ) << 16 );
}

static inline void elf_swap16( uint16_t *p )
{
	*p = elf_swab( *p );
}

static inline void elf_swap32( uint32_t *p )
{
register uint16_t vl = *p;
register uint16_t vh = *p>>16;
	*p = (elf_swab( vl ) << 16 ) | elf_swab ( vh ); 
}

static inline void elf_swap64( uint64_t *p )
{
register uint32_t vl = *p;
register uint32_t vh = *p>>32;
	*p = (((uint64_t)elf_swah( vl )) << 32 ) | elf_swah ( vh ); 
}
#endif

#define ARRSTR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : "unknown" )
#define ARRCHR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : '?' )

static inline uint32_t get_shdrsz(Pmelf_Shtab shtab)
{
#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( ELFCLASS64 == shtab->clss )
		return sizeof(Elf64_Shdr);
	else
#endif
		return sizeof(Elf32_Shdr);
}

static inline uint32_t get_symsz(Pmelf_Symtab symtab)
{
#ifdef PMELF_CONFIG_ELF64SUPPORT
	if ( ELFCLASS64 == symtab->clss )
		return sizeof(Elf64_Sym);
	else
#endif
		return sizeof(Elf32_Sym);
}

static inline Elf_Shdr *
get_shtabN(Pmelf_Shtab shtab, uint32_t idx)
{
	return (Elf_Shdr *)(shtab->shdrs.p_raw + idx * get_shdrsz(shtab));
}

static inline Elf_Sym *
get_symtabN(Pmelf_Symtab symtab, Pmelf_Long idx)
{
	return (Elf_Sym *)(symtab->syms.p_raw + idx * get_symsz(symtab));
}

#ifdef PMELF_CONFIG_ELF64SUPPORT
#define CREAT_GET_SH(typ,member)	            \
static inline typ                               \
get_sh_##member(uint32_t clss, Elf_Shdr *shdr)	\
{                                               \
	if ( ELFCLASS64 == clss )                   \
		return shdr->s64.sh_##member;           \
	else                                        \
		return shdr->s32.sh_##member;           \
}
#else
#define CREAT_GET_SH(typ,member)	            \
static inline typ                               \
get_sh_##member(uint32_t clss, Elf_Shdr *shdr)	\
{                                               \
	return shdr->s32.sh_##member;               \
}
#endif

CREAT_GET_SH(uint32_t,type)
CREAT_GET_SH(Pmelf_Size,size)
CREAT_GET_SH(uint32_t,link)
CREAT_GET_SH(uint32_t,info)

#endif
