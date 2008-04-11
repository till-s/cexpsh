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

/**************************************************/
/*  SYMBOLS                                       */
/**************************************************/

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

#define STV_DEFAULT		 0
#define STV_INTERNAL	 1
#define STV_HIDDEN		 2
#define STV_PROTECTED	 3

#define ELF32_ST_VISIBILITY(o) ((o)&3)

typedef struct {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	uint8_t		st_info;
	uint8_t		st_other;
	Elf32_Half	st_shndx;
} Elf32_Sym;

/**************************************************/
/*  RELOCATION RECORDS                            */
/**************************************************/

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

/**************************************************/
/* ANYTHING BELOW HERE IS DEFINED BY THIS LIBRARY */
/* AND NOT BY THE ELF FILE FORMAT.                */
/**************************************************/


/* A Section Header Table */
typedef struct {
	Elf32_Shdr	*shdrs;       /* Array of Shdrs    */
	uint32_t   	nshdrs;       /* number of entries */
	const char 	*strtab;      /* associated strtab */
	uint32_t    strtablen;
	uint32_t	idx;          /* SH idx of strtab  */
} *Pmelf_Elf32_Shtab;

typedef struct {
	Elf32_Sym	*syms;        /* Array of symbols  */
	uint32_t    nsyms;        /* number of entries */
	const char *strtab;       /* associated strtab */
	uint32_t    strtablen;
	uint32_t	idx;          /* SH idx of strtab  */
} *Pmelf_Elf32_Symtab;

/* Stream (file) where to read from; we hide the
 * details so that other implementations could be
 * provided.
 */
typedef struct _Elf_Stream *Elf_Stream;

static inline const char *
pmelf_elf32_get_section_name(Pmelf_Elf32_Shtab stab, uint32_t index)
{
	return &stab->strtab[stab->shdrs[index].sh_name];
}

/* Create a new stream; if 'name' is given then
 * the named file is opened and used for the stream.
 * Alternatively, an open FILE stream may be passed.
 * The name is unused in this case.
 * 
 * RETURNS: new stream of NULL on error.
 *
 * NOTE:    if an open 'FILE' is passed and creating
 *          the stream fails the FILE is *not* closed.
 */
Elf_Stream
pmelf_newstrm(char *name, FILE *f);

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
pmelf_seek(Elf_Stream s, Elf32_Off where);

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
pmelf_getehdr(Elf_Stream s, Elf32_Ehdr *pehdr);

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
pmelf_getshdr(Elf_Stream s, Elf32_Shdr *pshdr);

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
pmelf_getsym(Elf_Stream s, Elf32_Sym *psym);

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
pmelf_getscn(Elf_Stream s, Elf32_Shdr *psect, void *data, Elf32_Off offset, Elf32_Word len);


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
pmelf_getgrp(Elf_Stream s, Elf32_Shdr *psect, Elf32_Word *data);

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
Pmelf_Elf32_Shtab
pmelf_getshtab(Elf_Stream s, Elf32_Ehdr *pehdr);

/*
 * Destroy section header table and release memory
 */
void
pmelf_delshtab(Pmelf_Elf32_Shtab sht);

/*
 * Convenience routine: retrieve section name as a
 * string.
 *
 * RETURNS: section name or NULL if parameters are
 *          invalid (e.g., index into strtab found
 *          in shdr is out of bounds).
 */
const char *
pmelf_sec_name(Pmelf_Elf32_Shtab sht, Elf32_Shdr *shdr);

/*
 * Convenience routine: retrieve symbol name as a
 * string.
 *
 * RETURNS: symbol name or NULL if parameters are
 *          invalid (e.g., index into strtab found
 *          in sym is out of bounds).
 */
const char *
pmelf_sym_name(Pmelf_Elf32_Symtab symt, Elf32_Sym *sym);

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
Pmelf_Elf32_Symtab
pmelf_getsymtab(Elf_Stream s, Pmelf_Elf32_Shtab shtab);

/*
 * Destroy symbol table and release memory
 */
void
pmelf_delsymtab(Pmelf_Elf32_Symtab symtab);

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
long
pmelf_find_symhdrs(Elf_Stream s, Pmelf_Elf32_Shtab shtab, Elf32_Shdr **psymsh, Elf32_Shdr **pstrsh);

/* Dump contents of file header to FILE in readable form */
void
pmelf_dump_ehdr(FILE *f, Elf32_Ehdr *pehdr);

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
pmelf_dump_shdr(FILE *f, Elf32_Shdr *pshdr, int format);

/*
 * Dump contents of section header table in readable
 * form to FILE. If format == FMT_COMPAT the listing is
 * identical to the output of 'readelf -S' (except for some
 * header and footer lines).
 */
void
pmelf_dump_shtab(FILE *f, Pmelf_Elf32_Shtab shtab, int format);

/*
 * Dump contents of symbol table in readable form to FILE.
 * If format == FMT_COMPAT the listing is identical to the
 * output of 'readelf -s' (except for some header and footer
 * lines).
 */
void
pmelf_dump_symtab(FILE *f, Pmelf_Elf32_Symtab symtab, Pmelf_Elf32_Shtab shtab, int format);

/*
 * Dump contents of all section groups to FILE in readable
 * form, compatible with 'readelf -g'.
 *
 * RETURNS:  number of section groups on success or a value
 *           less than zero if an error was found.
 */
int
pmelf_dump_groups(FILE *f, Elf_Stream s, Pmelf_Elf32_Shtab shtab, Pmelf_Elf32_Symtab symtab);

#ifdef __cplusplus
}
#endif


#endif
