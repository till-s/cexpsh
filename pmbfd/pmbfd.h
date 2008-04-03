#ifndef PMBFD_H
#define PMBFD_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bfd_arch_info_type bfd_arch_info_type;

#ifndef BFD64
typedef unsigned long bfd_vma;
typedef unsigned long bfd_size_type;
typedef unsigned long symvalue;
#endif

typedef void          *PTR;

typedef long          file_ptr;

typedef unsigned int flagword;

struct bfd;

typedef struct bfd bfd; 

/* Symbol attributes */
#define BSF_KEEP			0x00000020
#define BSF_SECTION_SYM		0x00000100
#define BSF_FUNCTION		0x00000010
#define BSF_OLD_COMMON		0x00000200
#define BSF_OBJECT			0x00010000
#define BSF_GLOBAL			0x00000002
#define BSF_LOCAL			0x00000001
#define BSF_WEAK			0x00000080

#define SEC_ALLOC			0x00000001
#define SEC_RELOC			0x00000004
#define SEC_GROUP			0x04000000
#define SEC_LINK_ONCE		0x00020000
#define SEC_LINK_DUPLICATES	0x00040000

/* The following are not needed by cexp but defined
 * for testing against BFD objdump -h..
 */
#define SEC_LOAD			0x00000002
#define SEC_READONLY		0x00000008
#define SEC_CODE			0x00000010
#define SEC_DATA			0x00000020
#define SEC_ROM				0x00000040
#define SEC_CONSTRUCTOR		0x00000080
#define SEC_HAS_CONTENTS	0x00000100
#define SEC_DEBUGGING		0x00002000
#define SEC_EXCLUDE			0x00008000
#define SEC_LINK_DUPLICATES_DISCARD		0x00000000
#define SEC_LINK_DUPLICATES_ONE_ONLY	0x00080000
#define SEC_LINK_DUPLICATES_SAME_SIZE	0x00100000
#define SEC_LINK_DUPLICATES_SAME_CONTENTS \
	(SEC_LINK_DUPLICATES_ONE_ONLY | SEC_LINK_DUPLICATES_SAME_SIZE)
#define SEC_MERGE			0x01000000
#define SEC_STRINGS			0x02000000

typedef struct sec asection;

typedef struct {
	const char *name; /* Beware; two asymbols may point to the same name */
	flagword flags;
	symvalue value;
	asection *section;
	/* ELF symbol extension */
	struct {
		unsigned long st_size;
	} internal_elf_sym;
} asymbol;

typedef asymbol elf_symbol_type;

#define bfd_asymbol_name(s) ((s)->name)


/* type of relocation */
typedef struct {
	char *name;
} reloc_howto_type;

typedef struct {
	asymbol          **sym_ptr_ptr;
	reloc_howto_type *howto;
	bfd_size_type     address;
	int dummy;
} arelent;

asection *
elf_next_in_group(asection *);

elf_symbol_type *
elf_symbol_from(bfd *abfd, asymbol *sp);

extern asection *bfd_abs_section_ptr;

enum bfd_architecture { dumm1, dumm2 }; 

enum bfd_flavour {
	bfd_target_elf_flavour = 5
};


typedef int bfd_boolean;

typedef enum bfd_format {
	bfd_unknown = 0,
	bfd_object  = 1,
	bfd_archive = 2,
	bfd_core    = 3
} bfd_format;

typedef enum bfd_reloc_status {
	bfd_reloc_ok = 0,
	bfd_reloc_overflow,
	bfd_reloc_outofrange, /* section doesn't contain address */
	bfd_reloc_continue,
	bfd_reloc_notsupported,
	bfd_reloc_other,
	bfd_reloc_undefined,  /* reloc. against undef. symbol    */
	bfd_reloc_dangerous
} bfd_reloc_status_type;

bfd*
bfd_asymbol_bfd(asymbol *sym);

bfd_vma
bfd_asymbol_value(asymbol *sym);

long
bfd_canonicalize_reloc(bfd *abfd, asection *sec, arelent **loc, asymbol **syms);

long
bfd_canonicalize_symtab(bfd *abfd, asymbol**);

bfd_boolean
bfd_check_format(bfd *abfd, bfd_format format);

bfd_boolean
bfd_close_all_done(bfd *abfd);

#if 0
bfd_boolean
bfd_discard_group(bfd *, asection *);
#endif

enum bfd_architecture
bfd_get_arch(bfd *abfd);

#define HAS_SYMS	0x10

flagword
bfd_get_file_flags(bfd *abfd);

enum bfd_flavour
bfd_get_flavour(bfd *abfd);

unsigned long
bfd_get_mach(bfd *abfd);

long
bfd_get_reloc_upper_bound(bfd *abfd, asection *sect);

asection *
bfd_get_section(asymbol *sym);

/* Note: this actually returns ld(alignment)! */
unsigned int
bfd_get_section_alignment(bfd *abfd, asection *sect);

file_ptr
bfd_get_section_filepos(bfd *abfd, asection *section);

bfd_size_type
bfd_get_section_size(asection *section);

bfd_boolean
bfd_get_section_contents(bfd *abfd, asection *section, void *location, file_ptr offset, bfd_size_type count);

flagword
bfd_get_section_flags(bfd *abfd, asection *sect);

const char *
bfd_get_section_name(bfd *bfd, asection *sect);

bfd_vma
bfd_get_section_vma(bfd *bfd, asection *sect);

bfd_vma
bfd_get_section_lma(bfd *bfd, asection *sect);

long
bfd_get_symtab_upper_bound(bfd *bfd);

char *
bfd_get_target(bfd *abfd);

char *
bfd_get_unique_section_name(bfd *abfd, const char *template, int *count);

void
bfd_init(void);

int
bfd_is_abs_section(asection *sect);

int
bfd_is_com_section(asection *sect);

int
bfd_is_und_section(asection *sect);

asymbol *
bfd_make_empty_symbol(bfd *bfd);

asection *
bfd_make_section(bfd *, const char *name);

void
bfd_map_over_sections(bfd *abfd, void (*f)(bfd *abfd, asection *sect, void *closure), void *closure);

bfd *
bfd_openstreamr(const char *fname, const char *target, FILE *f);

bfd_reloc_status_type
bfd_perform_relocation(bfd *abfd, arelent *relent, void *data, asection *input_section, bfd *output_bfd, char **p_err_msg);

void
bfd_perror(const char *msg);

const char *
bfd_printable_arch_mach(enum bfd_architecture arch, unsigned long mach);

const char *
bfd_printable_name(bfd *abfd);

const bfd_arch_info_type *
bfd_scan_arch(const char *str);

bfd_boolean
bfd_set_section_alignment(bfd *abfd, asection *sect, unsigned int val);

bfd_size_type
bfd_section_size(bfd *abfd, asection *sect);

bfd_boolean
bfd_set_section_flags(bfd *abfd, asection *sect, flagword flags);

bfd_boolean
bfd_set_section_size(bfd *abfd, asection *sect, bfd_size_type val);

bfd_boolean
bfd_set_section_vma(bfd *abfd, asection *sect, bfd_vma vma);

asection *
bfd_set_section(asymbol *sym, asection *sect);

#define xmalloc  malloc
#define xrealloc realloc

#define align_power(addr, align)    \
  (((addr) + ((bfd_vma) 1 << (align)) - 1) & ((bfd_vma) -1 << (align)))

#ifdef __cplusplus
};
#endif

#endif
