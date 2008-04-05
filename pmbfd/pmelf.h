#ifndef _PMELF_H
#define _PMELF_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EI_NIDENT  16

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef  int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

/* Object file type */
#define ET_NONE		0		/* none               */
#define ET_REL		1		/* relocatable        */
#define ET_EXEC		2		/* executable         */
#define ET_DYN		3		/* shared object      */
#define ET_CORE		4		/* core               */
#define ET_LOPROC	0xff00	/* processor specific */
#define ET_HIPROC	0xffff	/* processor specific */

/* Machine type (supported by us so far)                      */
#define EM_386				 3      /* Intel                  */
#define EM_68K				 4      /* Motorola 68k           */
#define EM_PPC				20      /* PowerPC                */

/* Identification indices                                     */
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

/* Object file version                                        */
#define EV_NONE				 0
#define EV_CURRENT			 1

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

/* Section indices                                             */
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

#define ELF32_ST_BIND(x)		(((x)>>4)&0xf)
#define ELF32_ST_TYPE(x)		((x) & 0xf)
#define ELF32_ST_INFO(b,t)		(((b)<<4) | ((t)&0xf))

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

typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	uint8_t		st_info;
	uint8_t		st_other;
	Elf32_Half	st_shndx;
} Elf32_Sym;

#define STV_DEFAULT		 0
#define STV_INTERNAL	 1
#define STV_HIDDEN		 2
#define STV_PROTECTED	 3

#define ELF32_ST_VISIBILITY(o) ((o)&3)

#define ELF32_R_SYM(x)		((x) >> 8)
#define ELF32_R_TYPE(x) 	((uint8_t)((x)&0xff))
#define ELF32_R_INFO(s,t)	(((s)<<8) | ((t)&0xff))

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
	Elf32_Shdr	*shdrs;
	uint32_t   	nshdrs;
	const char 	*strtab;
	uint32_t    strtablen;
	uint32_t	idx;
} *Txx_Elf32_Shtab;

static inline const char *
txx_elf32_get_section_name(Txx_Elf32_Shtab stab, uint32_t index)
{
	return &stab->strtab[stab->shdrs[index].sh_name];
}

typedef struct {
	Elf32_Sym	*syms;
	uint32_t    nsyms;
	const char *strtab;
	uint32_t    strtablen;
	uint32_t	idx;
} *Txx_Elf32_Symtab;

typedef struct _Elf_Stream *Elf_Stream;

Elf_Stream
txx_newstrm(char *name, FILE *f);

void
txx_set_errstrm(FILE *f);

int
txx_seek(Elf_Stream s, Elf32_Off where);

void
txx_delstrm(Elf_Stream s);

int
txx_getehdr(Elf_Stream s, Elf32_Ehdr *pehdr);

int
txx_getshdr(Elf_Stream s, Elf32_Shdr *pshdr);

int
txx_getsym(Elf_Stream s, Elf32_Sym *psym);

void *
txx_getscn(Elf_Stream s, Elf32_Shdr *psect, void *data, Elf32_Off offset, Elf32_Word len);

Elf32_Word *
txx_getgrp(Elf_Stream s, Elf32_Shdr *psect, Elf32_Word *data);

void
txx_delshtab(Txx_Elf32_Shtab sht);

Txx_Elf32_Shtab
txx_getshtab(Elf_Stream s, Elf32_Ehdr *pehdr);

const char *
txx_sec_name(Txx_Elf32_Shtab sht, Elf32_Shdr *shdr);

const char *
txx_sym_name(Txx_Elf32_Symtab symt, Elf32_Sym *sym);

void
txx_delsymtab(Txx_Elf32_Symtab symtab);

Txx_Elf32_Symtab
txx_getsymtab(Elf_Stream s, Txx_Elf32_Shtab shtab);

long
txx_find_symhdrs(Elf_Stream s, Txx_Elf32_Shtab shtab, Elf32_Shdr **psymsh, Elf32_Shdr **pstrsh);

void
txx_dump_ehdr(FILE *f, Elf32_Ehdr *pehdr);

#define FMT_SHORT  0
#define FMT_LONG   1
#define FMT_COMPAT 2

void 
txx_dump_shdr(FILE *f, Elf32_Shdr *pshdr, int format);

void
txx_dump_shtab(FILE *f, Txx_Elf32_Shtab shtab, int format);

void
txx_dump_symtab(FILE *f, Txx_Elf32_Symtab symtab, Txx_Elf32_Shtab shtab, int format);

int
txx_dump_groups(FILE *f, Elf_Stream s, Txx_Elf32_Shtab shtab, Txx_Elf32_Symtab symtab);

#ifdef __cplusplus
}
#endif


#endif
