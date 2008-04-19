/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmbfd' BFD emulation for cexpsh) was created by
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

#ifndef PMBFD_P_H
#define PMBFD_P_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pmelf.h"
#include "pmbfd.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#define ERRPR(x...) fprintf(stderr,x)

#define BFD_TRUE  1
#define BFD_FALSE 0

struct bfd_arch_info_type {
	const char *arch_name;
};

#define SEC_BOGUS (-1)

#define DEBUG_RELOC (1)

#undef DEBUG

/* Assume we have less than 64k sections */
typedef uint16_t	Secndx;

struct sec {
	const char   *name;
	bfd_size_type size;
	bfd_vma       vma;
	unsigned      align_power;
	flagword      flags;
	Elf_Shdr      *shdr;
#define GRP_NULL	0
	Secndx        grp_next;		/* half-word */
#define RELS_NULL	0
	Secndx        rels;		    /* half-word */
};

/* Index into strtabs */
#define SYMSTRTAB	0

typedef struct strtab {
	const char    *strs;
	unsigned long  size;
} strtab;

struct bfd {
	Elf_Stream      s;
	Elf_Ehdr        ehdr;
	const char      *arch;
#if SECCHUNKSZ == 0
	asection        *sects;
	uint32_t        nsects;
	uint32_t		maxsects;
#endif
	asymbol         *syms;
	long            nsyms;
	strtab          *strtabs;
	uint32_t        nstrtabs;
	uint32_t		str_avail;
	Pmelf_Shtab	    shtab;
	Elf_Shdr        *symsh;	    /* sh of symbol table           */
	Elf_Shdr		*symstrs;	/* sh of symbol table stringtab */
#if SECCHUNKSZ > 0
	struct secchunk	*secmemh;
	struct secchunk *secmemt;
#endif
	struct symchunk	*symmemh;
	struct symchunk *symmemt;
};

#define BFD_ELFCLASS(abfd)  ((abfd)->ehdr.e_ident[EI_CLASS])
#define BFD_IS_ELF64(abfd) (ELFCLASS64 == BFD_ELFCLASS(abfd))

#ifdef PMELF_CONFIG_ELF64SUPPORT

#define CREAT_GET_SH(typ,member)                \
static inline typ                               \
get_sh_##member(bfd *abfd, Elf_Shdr *shdr)      \
{                                               \
    if ( BFD_IS_ELF64(abfd) )                   \
        return shdr->s64.sh_##member;           \
    else                                        \
        return shdr->s32.sh_##member;           \
}

#else

#define CREAT_GET_SH(typ,member)                \
static inline typ                               \
get_sh_##member(bfd *abfd, Elf_Shdr *shdr) 		\
{                                               \
    return shdr->s32.sh_##member;               \
}

#endif

CREAT_GET_SH(uint32_t,type)
CREAT_GET_SH(Pmelf_Size,size)
CREAT_GET_SH(uint32_t,link)
CREAT_GET_SH(uint32_t,info)
CREAT_GET_SH(Pmelf_Off,offset)

static inline uint32_t get_shdrsz(bfd *abfd)
{
#ifdef PMELF_CONFIG_ELF64SUPPORT
    if ( BFD_IS_ELF64(abfd) )
        return sizeof(Elf64_Shdr);
    else
#endif
        return sizeof(Elf32_Shdr);
}

static inline uint32_t get_symsz(bfd *abfd)
{
#ifdef PMELF_CONFIG_ELF64SUPPORT
    if ( BFD_IS_ELF64(abfd) )
        return sizeof(Elf64_Sym);
    else
#endif
        return sizeof(Elf32_Sym);
}

static inline uint32_t get_shndx(bfd *abfd, Elf_Shdr *shdr)
{
#ifdef PMELF_CONFIG_ELF64SUPPORT
    if ( BFD_IS_ELF64(abfd) )
        return &shdr->s64 - abfd->shtab->shdrs.p_s64;
    else
#endif
        return &shdr->s32 - abfd->shtab->shdrs.p_s32;
}

static inline Elf_Shdr *
get_shtabN(bfd *abfd, uint32_t idx)
{
	return (Elf_Shdr *)(abfd->shtab->shdrs.p_raw + idx * get_shdrsz(abfd));
}

struct pmbfd_areltab {
	Elf32_Word	entsz;
	Elf_Shdr    *shdr;
	uint8_t		data[];
};

union pmbfd_arelent {
	Elf32_Rel	rel32;
	Elf32_Rela	rela32;
	Elf64_Rel   rel64;
	Elf64_Rela  rela64;
	char        raw[sizeof(Elf64_Rela)];
};

#define namecase(rel)	case rel: return #rel;

#endif
