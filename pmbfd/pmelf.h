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
#ifndef _PMELF_H
#define _PMELF_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Elementary Types */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef  int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef  int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef  int64_t Elf64_Sxword;

/**************************************************/
/*  ELF HEADER                                    */
/**************************************************/

/* Object file type */
#define ET_NONE		0		/* none               */
#define ET_REL		1		/* relocatable        */
#define ET_EXEC		2		/* executable         */
#define ET_DYN		3		/* shared object      */
#define ET_CORE		4		/* core               */
#define ET_LOPROC	0xff00	/* processor specific */
#define ET_HIPROC	0xffff	/* processor specific */

/* Machine type (supported by us so far)          */
#define EM_386				 3
#define EM_68K				 4
#define EM_PPC				20
#define EM_X86_64			62

/* Identification indices                          */
#define EI_MAG0				 0
#define EI_MAG1				 1
#define EI_MAG2				 2
#define EI_MAG3				 3
#define EI_CLASS			 4
#define EI_DATA				 5
#define EI_VERSION			 6
#define EI_OSABI			 7
#define EI_PAD				 8

#define ELFMAG0			  0x7f
#define ELFMAG1			   'E'
#define ELFMAG2			   'L'
#define ELFMAG3			   'F'

#define ELFCLASSNONE		 0
#define ELFCLASS32			 1
#define ELFCLASS64			 2

#define ELFDATANONE			 0
#define ELFDATA2LSB			 1
#define ELFDATA2MSB			 2

#define ELFOSABI_NONE		 0
#define ELFOSABI_SYSV		 0 /* yep */

/* Object file version                             */
#define EV_NONE				 0
#define EV_CURRENT			 1

#define EI_NIDENT  16
typedef struct {
	uint8_t		e_ident[EI_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	uint8_t		e_ident[EI_NIDENT];
	Elf64_Half	e_type;
	Elf64_Half	e_machine;
	Elf64_Word	e_version;
	Elf64_Addr	e_entry;
	Elf64_Off	e_phoff;
	Elf64_Off	e_shoff;
	Elf64_Word	e_flags;
	Elf64_Half	e_ehsize;
	Elf64_Half	e_phentsize;
	Elf64_Half	e_phnum;
	Elf64_Half	e_shentsize;
	Elf64_Half	e_shnum;
	Elf64_Half	e_shstrndx;
} Elf64_Ehdr;

typedef union {
	uint8_t    e_ident[EI_NIDENT];
	Elf32_Ehdr e32;
	Elf64_Ehdr e64;
} Elf_Ehdr;

/**************************************************/
/*  SECTION HEADER                                */
/**************************************************/

/* Section indices                                */
#define SHN_UNDEF			 0
#define SHN_LORESERVE		 0xff00
#define SHN_LOPROC			 0xff00
#define SHN_HIPROC			 0xff1f
#define SHN_ABS				 0xfff1
#define SHN_COMMON			 0xfff2
#define SHN_HIRESERVE		 0xffff

#define SHT_NULL			 0
#define SHT_PROGBITS		 1
#define SHT_SYMTAB			 2
#define SHT_STRTAB			 3
#define SHT_RELA			 4
#define SHT_HASH			 5
#define SHT_DYNAMIC			 6
#define SHT_NOTE			 7
#define SHT_NOBITS			 8
#define SHT_REL				 9
#define SHT_SHLIB			10
#define SHT_DYNSYM			11
#define SHT_INIT_ARRAY		14
#define SHT_FINI_ARRAY		15
#define SHT_PREINIT_ARRAY	16
#define SHT_GROUP			17
#define SHT_SYMTAB_SHNDX	18
#define SHT_MAXSUP			18
#define SHT_GNU_ATTRIBUTES  0x6ffffff5
#define SHT_GNU_VERSION     0x6fffffff
#define SHT_GNU_VERSION_R   0x6ffffffe
#define SHT_LOPROC			0x70000000
#define SHT_HIPROC			0x7fffffff
#define SHT_LOUSER			0x80000000
#define SHT_HIUSER			0xffffffff

#define SHF_WRITE			0x00000001
#define SHF_ALLOC			0x00000002
#define SHF_EXECINSTR		0x00000004
#define SHF_MERGE			0x00000010
#define SHF_STRINGS			0x00000020
#define SHF_INFO_LINK		0x00000040
#define SHF_LINK_ORDER		0x00000080
#define SHF_OS_NONCONFORMING	0x00000100
#define SHF_GROUP			0x00000200
#define SHF_MSKSUP			 (~0x3f7)
#define SHF_MASKPROC		0xf0000000

#define GRP_COMDAT			 1

typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} Elf64_Shdr;

typedef union {
	Elf32_Shdr s32;
	Elf64_Shdr s64;
} Elf_Shdr;

/**************************************************/
/*  SYMBOLS                                       */
/**************************************************/

#define ELF32_ST_BIND(x)		(((x)>>4)&0xf)
#define ELF32_ST_TYPE(x)		((x) & 0xf)
#define ELF32_ST_INFO(b,t)		(((b)<<4) | ((t)&0xf))

#define ELF64_ST_BIND(x)		ELF32_ST_BIND(x)
#define ELF64_ST_TYPE(x)		ELF32_ST_TYPE(x)
#define ELF64_ST_INFO(b,t)		ELF32_ST_INFO(b,t)

#define STB_LOCAL		 0
#define STB_GLOBAL		 1
#define STB_WEAK		 2
#define STB_MAXSUP		 2
#define STB_LOPROC		13
#define STB_HIPROC		15

#define STT_NOTYPE		 0
#define STT_OBJECT		 1
#define STT_FUNC		 2
#define STT_SECTION		 3
#define STT_FILE		 4
#define STT_MAXSUP		 4
#define STT_LOPROC		13
#define STT_HIPROC		15

#define STV_DEFAULT		 0
#define STV_INTERNAL	 1
#define STV_HIDDEN		 2
#define STV_PROTECTED	 3

#define ELF32_ST_VISIBILITY(o) ((o)&3)
#define ELF64_ST_VISIBILITY(o) ELF32_ST_VISIBILITY(o)

typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	uint8_t		st_info;
	uint8_t		st_other;
	Elf32_Half	st_shndx;
} Elf32_Sym;

typedef struct {
	Elf64_Word	st_name;
	uint8_t		st_info;
	uint8_t		st_other;
	Elf64_Half	st_shndx;
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;
} Elf64_Sym;

typedef union {
	Elf32_Sym t32;
	Elf64_Sym t64;
} Elf_Sym;

/**************************************************/
/*  RELOCATION RECORDS                            */
/**************************************************/

/*
 * SysvR4 relocation types for i386. Statically linked objects
 * just use R_386_32 and R_386_PC32 which makes our job really easy...
 */
#define R_386_NONE                0
#define R_386_32                  1
#define R_386_PC32                2
#define R_386_GOT32               3
#define R_386_PLT32               4
#define R_386_COPY                5
#define R_386_GLOB_DAT            6
#define R_386_JMP_SLOT            7
#define R_386_RELATIVE            8
#define R_386_GOTOFF              9
#define R_386_GOTPC              10

/*
 * 68k relocation types
 */
#define R_68K_NONE                0
#define R_68K_32                  1
#define R_68K_16                  2
#define R_68K_8                   3
#define R_68K_PC32                4
#define R_68K_PC16                5
#define R_68K_PC8                 6
#define R_68K_GOT32               7
#define R_68K_GOT16               8
#define R_68K_GOT8                9
#define R_68K_GOT320             10
#define R_68K_GOT160             11
#define R_68K_GOT80              12
#define R_68K_PLT32              13
#define R_68K_PLT16              14
#define R_68K_PLT8               15
#define R_68K_PLT320             16
#define R_68K_PLT160             17
#define R_68K_PLT80              18
#define R_68K_COPY               19
#define R_68K_GLOB_DAT           20
#define R_68K_JMP_SLOT           21
#define R_68K_RELATIVE           22


/* PPC relocation types */
#define R_PPC_NONE                0
#define R_PPC_ADDR32              1
#define R_PPC_ADDR24              2
#define R_PPC_ADDR16              3
#define R_PPC_ADDR16_LO           4
#define R_PPC_ADDR16_HI           5
#define R_PPC_ADDR16_HA           6
#define R_PPC_ADDR14              7
#define R_PPC_ADDR14_BRTAKEN      8
#define R_PPC_ADDR14_BRNTAKEN     9
#define R_PPC_REL24              10
#define R_PPC_REL14              11
#define R_PPC_REL14_BRTAKEN      12
#define R_PPC_REL14_BRNTAKEN     13
#define R_PPC_GOT16              14
#define R_PPC_GOT16_LO           15
#define R_PPC_GOT16_HI           16
#define R_PPC_GOT16_HA           17
#define R_PPC_PLTREL24           18
#define R_PPC_COPY               19
#define R_PPC_GLOB_DAT           20
#define R_PPC_JMP_SLOT           21
#define R_PPC_RELATIVE           22
#define R_PPC_LOCAL24PC          23
#define R_PPC_UADDR32            24
#define R_PPC_UADDR16            25
#define R_PPC_REL32              26
#define R_PPC_PLT32              27
#define R_PPC_PLTREL32           28
#define R_PPC_PLT16_LO           29
#define R_PPC_PLT16_HI           30
#define R_PPC_PLT16_HA           31
#define R_PPC_SDAREL16           32
#define R_PPC_SECTOFF            33
#define R_PPC_SECTOFF_LO         34
#define R_PPC_SECTOFF_HI         35
#define R_PPC_SECTOFF_HA         36
#define R_PPC_ADDR30             37

/*
 * SVR4 relocation types for x86_64
 */
#define R_X86_64_NONE             0
#define R_X86_64_64               1
#define R_X86_64_PC32             2
#define R_X86_64_GOT32            3
#define R_X86_64_PLT32            4
#define R_X86_64_COPY             5
#define R_X86_64_GLOB_DAT         6
#define R_X86_64_JUMP_SLOT        7
#define R_X86_64_RELATIVE         8
#define R_X86_64_GOTPCREL         9
#define R_X86_64_32              10
#define R_X86_64_32S             11
#define R_X86_64_16              12
#define R_X86_64_PC16            13
#define R_X86_64_8               14
#define R_X86_64_PC8             15
#define R_X86_64_DTPMOD64        16
#define R_X86_64_DTPOFF64        17
#define R_X86_64_TPOFF64         18
#define R_X86_64_TLSGD           19
#define R_X86_64_TLSLD           20
#define R_X86_64_DTPOFF32        21
#define R_X86_64_GOTTPOFF        22
#define R_X86_64_TPOFF32         23
#define R_X86_64_PC64            24
#define R_X86_64_GOTOFF64        25
#define R_X86_64_GOTPC32         26
#define R_X86_64_SIZE32          32
#define R_X86_64_SIZE64          33
#define R_X86_64_GOTPC32_TLSDESC 34
#define R_X86_64_TLSDESC_CALL    35
#define R_X86_64_TLSDESC         36

#define ELF32_R_SYM(x)		((x) >> 8)
#define ELF32_R_TYPE(x) 	((uint8_t)((x)&0xff))
#define ELF32_R_INFO(s,t)	(((s)<<8) | ((t)&0xff))

#define ELF64_R_SYM(x)		((x) >> 32)
#define ELF64_R_TYPE(x) 	((uint32_t)((x)&0xffffffffL))
#define ELF64_R_INFO(s,t)	(((s)<<32) | ((t)&0xffffffffL))

typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
} Elf32_Rel;

typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
} Elf32_Rela;

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
} Elf64_Rel;

typedef struct {
	Elf64_Addr		r_offset;
	Elf64_Xword		r_info;
	Elf64_Sxword	r_addend;
} Elf64_Rela;


/**************************************************/
/* ANYTHING BELOW HERE IS DEFINED BY THIS LIBRARY */
/* AND NOT BY THE ELF FILE FORMAT.                */
/**************************************************/

typedef union {
	Elf32_Ehdr *p_e32;
	Elf64_Ehdr *p_e64;
} Elf_PEhdr;

typedef union {
	uint8_t    *p_raw;
	Elf32_Shdr *p_s32;
	Elf64_Shdr *p_s64;
} Elf_PShdr;

typedef union {
	uint8_t    *p_raw;
	Elf32_Sym  *p_t32;
	Elf64_Sym  *p_t64;
} Elf_PSym;

typedef unsigned long Pmelf_Off;
typedef unsigned long Pmelf_Size;
typedef long          Pmelf_Long;


/* A Section Header Table */
typedef struct {
	Elf_PShdr	shdrs;        /* Array of Shdrs    */
	uint32_t   	nshdrs;       /* number of entries */
	const char 	*strtab;      /* associated strtab */
	Pmelf_Size  strtablen;
	uint32_t	idx;          /* SH idx of strtab  */
	uint8_t     clss;         /* class (64/32-bit) */
} *Pmelf_Shtab;

/* A Symbol Table         */
typedef struct {
	Elf_PSym	syms;         /* Array of symbols  */
	Pmelf_Long  nsyms;        /* number of entries */
	const char  *strtab;      /* associated strtab */
	Pmelf_Size  strtablen;
	uint32_t	idx;          /* SH idx of strtab  */
	uint8_t     clss;         /* class (64/32-bit) */
} *Pmelf_Symtab;

/* Stream (file) where to read from; we hide the
 * details so that other implementations could be
 * provided.
 */
typedef struct _Elf_Stream *Elf_Stream;

static inline const char *
pmelf32_get_section_name(Pmelf_Shtab stab, uint32_t index)
{
	return &stab->strtab[stab->shdrs.p_s32[index].sh_name];
}

static inline const char *
pmelf64_get_section_name(Pmelf_Shtab stab, uint32_t index)
{
	return &stab->strtab[stab->shdrs.p_s64[index].sh_name];
}

static inline const char *
pmelf_get_section_name(Pmelf_Shtab stab, uint32_t index)
{
	if ( ELFCLASS64 == stab->clss )
		return pmelf64_get_section_name(stab, index);
	else
		return pmelf32_get_section_name(stab, index);
}

/* Create a new stream; if 'name' is given then
 * the named file is opened and used for the stream.
 * Alternatively, an open FILE stream may be passed.
 * The name is unused in this case (except for informative
 * purposes in error messages).
 * 
 * RETURNS: new stream of NULL on error.
 *
 * NOTE:    if an open 'FILE' is passed and creating
 *          the stream fails the FILE is *not* closed.
 */
Elf_Stream
pmelf_newstrm(const char *name, FILE *f);

/*
 * Create a new stream to read from a memory
 * buffer 'buf'.
 */
Elf_Stream
pmelf_memstrm(void *buf, size_t len);

/* Cleanup and delete a stream. Optionally,
 * (pass nonzero 'noclose' argument) the
 * underlying FILE is not closed but left alone.
 */
void
pmelf_delstrm(Elf_Stream s, int noclose);


/* Direct error messages to FILE; a NULL pointer
 * may be passed to silence all messages.
 *
 * NOTE: by default (w/o calling this routine)
 *       the library is silent (as if NULL had
 *       been passed).
 */
void
pmelf_set_errstrm(FILE *f);

/* Position the stream;
 * 
 * RETURNS: 0 on success, nonzero on error.
 *
 * NOTE:    Always use this routine. Do not
 *          fseek the underlying file directly.
 */
int
pmelf_seek(Elf_Stream s, Pmelf_Off where);

/* Read an ELF file header into *pehdr (storage
 * provided by caller).
 *
 * The header is byte-swapped if necessary into
 * host byte order.
 *
 * NOTE:    The stream is rewound to the beginning
 *          by this routine.
 *          After returning successfully from this
 *          routine the stream is positioned after
 *          the datum that was read.
 *
 * RETURNS: 0 on success, nonzero on error.
 */
int
pmelf_getehdr(Elf_Stream s, Elf_Ehdr *pehdr);

/* Read an ELF section header into *pshdr (storage
 * provided by caller).
 *
 * The header is byte-swapped if necessary into
 * host byte order.
 *
 * NOTE:    The stream must have been correctly 
 *          positioned prior to calling this routine.
 *          After returning successfully from this
 *          routine the stream is positioned after
 *          the datum that was read.
 *
 * RETURNS: 0 on success, nonzero on error.
 */
int
pmelf_getshdr32(Elf_Stream s, Elf32_Shdr *pshdr);
int
pmelf_getshdr64(Elf_Stream s, Elf64_Shdr *pshdr);

/* Read an ELF symbol into *psym (storage
 * provided by caller).
 *
 * The symbol is byte-swapped if necessary into
 * host byte order.
 *
 * NOTE:    The stream must have been correctly 
 *          positioned prior to calling this routine.
 *          After returning successfully from this
 *          routine the stream is positioned after
 *          the datum that was read.
 *
 * RETURNS: 0 on success, nonzero on error.
 */
int
pmelf_getsym32(Elf_Stream s, Elf32_Sym *psym);
int
pmelf_getsym64(Elf_Stream s, Elf64_Sym *psym);

/* Read section contents described by *psect
 * from stream 's' into the storage area pointed
 * to by 'data' (provided by caller).
 *
 * No byte-swapping is performed.
 *
 *
 * The stream is positioned to the section
 * offset as indicated by psect->sh_offset.
 *
 * An additional 'offset' may be provided by
 * the caller
 *
 * If the 'len' argument (byte count) is zero
 * then the entire section contents are read.
 *
 * It is legal to pass a NULL 'data' pointer.
 * This instructs the routine to allocate
 * the required amount of memory. It is the
 * responsability of the caller to free such
 * memory.
 *
 * After returning successfully from this
 * routine the stream is positioned after
 * the datum that was read.
 *
 * RETURNS: pointer to 'data' (or allocated area
 *          if a NULL data pointer was passed)
 *          or NULL on failure.
 */
void *
pmelf_getscn(Elf_Stream s, Elf_Shdr *psect, void *data, Pmelf_Off offset, Pmelf_Off len);


/* Read the contents of a SHT_GROUP section identified
 * by the section header 'psect'.
 *
 * The stream is positioned to the start of the section
 * and it's contents are read into the 'data' storage
 * area which may be provided by the caller. If a NULL
 * 'data' pointer is passed then memory is allocated
 * (it is the user's responsability to free it when done).
 *
 * The section contents are byte-swapped as needed.
 *
 * A group section is an array of Elf32_Words listing
 * the section indices of the group members. However,
 * the very first word is a flag word (e.g., containing
 * GRP_COMDAT).
 *
 * RETURNS: 'data' pointer on success, NULL on error.
 *
 * NOTE:    on successful return the stream is positioned
 *          behind the section contents.
 */
Elf32_Word *
pmelf_getgrp(Elf_Stream s, Elf_Shdr *psect, Elf32_Word *data);

/*
 * Allocate memory for a section header table and read
 * the section headers described by file header *pehdr
 * from stream 's'.
 *
 * It is the user's responsibility to destroy the table
 * by calling pmelf_delshtab().
 *
 * RETURNS: pointer to section header table on success,
 *          NULL on error.
 *
 * NOTE:    after successful execution the stream is
 *          positioned behind the section headers.
 */
Pmelf_Shtab
pmelf_getshtab(Elf_Stream s, Elf_Ehdr *pehdr);

/*
 * Destroy section header table and release memory
 */
void
pmelf_delshtab(Pmelf_Shtab sht);

/*
 * Convenience routine: retrieve section name as a
 * string.
 *
 * RETURNS: section name or NULL if parameters are
 *          invalid (e.g., index into strtab found
 *          in shdr is out of bounds).
 */
const char *
pmelf_sec_name(Pmelf_Shtab sht, Elf_Shdr *shdr);

/*
 * Convenience routine: retrieve symbol name as a
 * string.
 *
 * RETURNS: symbol name or NULL if parameters are
 *          invalid (e.g., index into strtab found
 *          in sym is out of bounds).
 */
const char *
pmelf_sym_name(Pmelf_Symtab symt, Elf_Sym *sym);

/*
 * Allocate memory for a symbol table and read the
 * section headers described by section header table
 * 'shtab' from stream 's'.
 *
 * It is the user's responsibility to destroy the table
 * by calling pmelf_delsymtab().
 *
 * RETURNS: pointer to symbol header table on success,
 *          NULL on error.
 *
 * NOTE:    after successful execution the stream is
 *          positioned behind the ELF symbol table.
 */
Pmelf_Symtab
pmelf_getsymtab(Elf_Stream s, Pmelf_Shtab shtab);

/*
 * Destroy symbol table and release memory
 */
void
pmelf_delsymtab(Pmelf_Symtab symtab);

/*
 * Convenience routine for users who want to build their
 * own symbol table (by repeated use of pmelf_getsym()).
 *
 * This routine locates the section headers of the
 * symbol table and it's associated string table,
 * respectively.
 *
 * RETURNS: (positive) number of symbols or a value less
 *          than zero on error.
 *
 *          If successful, pointers to the respective
 *          headers are stored in *psymsh (header of
 *          symbol table section) and *pstrsh (header
 *          of string table used by symbol table).
 */
Pmelf_Long
pmelf_find_symhdrs(Elf_Stream s, Pmelf_Shtab shtab, Elf_Shdr **psymsh, Elf_Shdr **pstrsh);

/* Dump contents of file header to FILE in readable form */
void
pmelf_dump_ehdr(FILE *f, Elf_Ehdr *pehdr);

#define FMT_SHORT  0		/* more concise; fits on one line */
#define FMT_LONG   1        /* slightly longer / more info    */
#define FMT_COMPAT 2        /* 'readelf -Ss' compatible       */

/* Dump contents of section header to FILE in readable form
 * using one of the formats defined above.
 *
 * NOTE: the section name is NOT printed by this routine
 *       (essentially because it has no information about
 *       the string table). It is the user's responsability
 *       to print the name if needed.
 */
void 
pmelf_dump_shdr32(FILE *f, Elf32_Shdr *pshdr, int format);

void 
pmelf_dump_shdr64(FILE *f, Elf64_Shdr *pshdr, int format);

/*
 * Dump contents of section header table in readable
 * form to FILE. If format == FMT_COMPAT the listing is
 * identical to the output of 'readelf -S' (except for some
 * header and footer lines).
 */
void
pmelf_dump_shtab(FILE *f, Pmelf_Shtab shtab, int format);

/*
 * Dump symbol information in readable form to FILE.
 * Either of 'shtab' or 'strtab' may be NULL but then
 * instead of the name of the section defining the symbol
 * or the name of the symbol itself their respective
 * index numbers are printed.
 *
 * If format == FMT_COMPAT the listing is identical to the
 * output of 'readelf -s'.
 */
void
pmelf_dump_sym32(FILE *f, Elf32_Sym *sym, Pmelf_Shtab shtab, const char *strtab, unsigned strtablen, int format);
void
pmelf_dump_sym64(FILE *f, Elf64_Sym *sym, Pmelf_Shtab shtab, const char *strtab, unsigned strtablen, int format);

/*
 * Dump contents of symbol table in readable form to FILE.
 * If format == FMT_COMPAT the listing is identical to the
 * output of 'readelf -s' (except for some header and footer
 * lines).
 */
void
pmelf_dump_symtab(FILE *f, Pmelf_Symtab symtab, Pmelf_Shtab shtab, int format);

/*
 * Dump contents of all section groups to FILE in readable
 * form, compatible with 'readelf -g'.
 *
 * RETURNS:  number of section groups on success or a value
 *           less than zero if an error was found.
 */
int
pmelf_dump_groups(FILE *f, Elf_Stream s, Pmelf_Shtab shtab, Pmelf_Symtab symtab);

/*
 * Object file attributes (stored in '.gnu.attributes' sections of type
 * SHT_GNU_ATTRIBUTES). They describe ABI compatibility features of object
 * files.
 */
typedef struct Pmelf_attribute_set_ Pmelf_attribute_set;

typedef struct Pmelf_attribute_vendor_ Pmelf_attribute_vendor;

/*
 * Vendors implemented so far
 */

extern Pmelf_attribute_vendor pmelf_attributes_vendor_gnu_ppc;

/*
 * Provide a compile-time constant if there is an implementation
 */
#if defined(__PPC__) && defined(_CALL_SYSV)
#define PMELF_ATTRIBUTE_VENDOR (&pmelf_attributes_vendor_gnu_ppc)
#endif

/*
 * Try to find a vendor for 'machine/osabi' at run-time.
 * (Using this routine will trigger linkage of all implemented
 * vendors)
 */
Pmelf_attribute_vendor *
pmelf_attributes_vendor_find_gnu(Elf32_Word machine, Elf32_Word osabi);

/* register a vendor (attribute parser + filter) */
int
pmelf_attributes_vendor_register(Pmelf_attribute_vendor *pv);
	
/* retrieve name of a vendor (pointer to constant string) */
const char * const
pmelf_attributes_vendor_name(Pmelf_attribute_vendor *pv);

/* Create attribute set reading from an ELF stream.
 *
 * 's'       : stream object
 *
 * 'psect'   : section header of type SHT_GNU_ATTRIBUTES (".gnu.attributes")
 * 
 * RETURNS   : attribute set or NULL on error.
 *
 * NOTE      : the stream and section header are not referenced anymore
 *             after this routine returns and may therefore be safely
 *             destroyed.
 */
Pmelf_attribute_set *
pmelf_create_attribute_set(Elf_Stream s, Elf_Shdr *psect);

/* Create attribute set from a static memory buffer.
 *
 * 'b'       : pointer to buffer
 *
 * 'bsize'   : size of data/buffer in bytes
 * 
 * 'needswap': if nonzero, assume that endian-ness of the data does not match the
 *             endian-ness of the executing CPU and hence data need to be swapped
 *
 * 'obj_name': a name to associate with this buffer (mainly used in error messages)
 *
 * RETURNS   : attribute set or NULL on error.
 *
 * NOTE      : It is the responsibility of the caller to manage this buffer and
 *             make sure it 'lives' as long as the attribute set since the
 *             latter uses pointers into the buffer.
 *             Data layout in the buffer is expected to be in ARM aeabi format
 *             (which is used by the GNU toolchain).
 */
Pmelf_attribute_set *
pmelf_read_attribute_set(const uint8_t *b, unsigned bsize, int needswap, const char *obj_name);

/*
 * Destroy attribute set and release all associated resources
 */
void
pmelf_destroy_attribute_set(Pmelf_attribute_set *pa);

/*
 * Dump information contained in an attribute set to FILE 'f'
 * (may be NULL resulting in stdout being used).
 */
void
pmelf_print_attribute_set(Pmelf_attribute_set *pa, FILE *f);

/*
 * Compare two attribute sets.
 *
 * RETURNS: zero if the sets describe compatible objects, negative
 *          if an incompatibility is found indicating that they
 *          may not be linked.
 *
 * NOTE:    positive return values are reserved for future use.
 */
int
pmelf_match_attribute_set(Pmelf_attribute_set *pa, Pmelf_attribute_set *pb);

/* Write headers to a stream */
int
pmelf_putehdr32(Elf_Stream s, Elf32_Ehdr *pehdr);
int
pmelf_putehdr64(Elf_Stream s, Elf64_Ehdr *pehdr);

int
pmelf_putshdr32(Elf_Stream s, Elf32_Shdr *pshdr);
int
pmelf_putshdr64(Elf_Stream s, Elf64_Shdr *pshdr);

int
pmelf_putsym32(Elf_Stream s, Elf32_Sym *psym);
int
pmelf_putsym64(Elf_Stream s, Elf64_Sym *psym);

int
pmelf_write(Elf_Stream s, void *data, Pmelf_Long len);

#ifdef __cplusplus
}
#endif


#endif
