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
#ifndef PMBFD_H
#define PMBFD_H

#include <stdio.h>
#include <stdint.h>

#define _PMBFD_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bfd_arch_info_type bfd_arch_info_type;

typedef unsigned long bfd_vma;
typedef          long bfd_signed_vma;
typedef unsigned long bfd_size_type;
typedef unsigned long symvalue;

/*
 * Here's a trick: 
 * BFD ansidecl.h defines PTR void*
 * and it is sometimes included before bfd.h
 * sometimes after.
 * If we define PTR here then ansidecl.h included
 * after bfd.h would redefine it.
 * By making this a typedef, including ansidecl.h
 * afterwards does no harm. OTOH, if ansidecl.h was
 * included before us then its definition masks
 * our typedef ;-)
 */
#ifndef PTR
typedef void *PTR;
#endif

/*
 * we do need to define TRUE/FALSE namely if
 * opcodes is used together with pmbfd...
 */
#define TRUE  1
#define FALSE 0

typedef long          file_ptr;

typedef unsigned int flagword;
typedef uint8_t      bfd_byte;

struct bfd;

typedef struct bfd bfd; 

/* Symbol attributes */
#define BSF_KEEP			0x0020
#define BSF_SECTION_SYM		0x0100
#define BSF_FUNCTION		0x0010
#define BSF_OLD_COMMON		0x0200
/* NOTE: BSF_OBJECT definition differs from BFD
 *       so that we can store the flags in a
 *       16-bit word.
 *
#define BSF_OBJECT			0x10000
 */
#define BSF_OBJECT			0x1000
#define BSF_GLOBAL			0x0002
#define BSF_LOCAL			0x0001
#define BSF_WEAK			0x0080

/* The following are not neede by cexp but
 * exist for testing against objdump -t
 */
#define BSF_FILE			0x4000
#define BSF_DEBUGGING		0x0008

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
	const char      *name; /* Beware; two asymbols may point to the same name */
	/* NOTE: To make 'asymbol' smaller the 'val' field has slightly
	 *       different semantics than demanded by BFD (BFD stores the
	 *       symbol size of common symbols in 'value').
	 *       Compatible semantics are obtained by accessing the value
	 *       via the 'bfd_asymbol_value() / pmbfd_asymbol_set_value()'
	 *       routines. 
	 *       The member name differs from BFD's so we can detect attempts
	 *       to tamper with 'value' w/o going through the proper access
	 *       macros/routines.
	 */
	symvalue        val;
	bfd_size_type   size;
	uint16_t        flags;
	uint16_t        secndx;
} asymbol;

#define bfd_asymbol_name(s) ((s)->name)

asection *
elf_next_in_group(asection *);

int
elf_get_size(bfd *abfd, asymbol *asym);

unsigned
elf_get_align(bfd *abfd, asymbol *asym);

#define align_power(addr, align)    \
  (((addr) + ((bfd_vma) 1 << (align)) - 1) & ((bfd_vma) -1 << (align)))

extern asection *bfd_abs_section_ptr;
extern asection *bfd_und_section_ptr;
extern asection *bfd_com_section_ptr;

enum bfd_architecture {
	bfd_arch_unknown,
	bfd_arch_obscure,
	bfd_arch_m68k         =  2,
#define bfd_mach_m68000                         1
#define bfd_mach_m68008                         2
#define bfd_mach_m68010                         3
#define bfd_mach_m68020                         4
#define bfd_mach_m68030                         5
#define bfd_mach_m68040                         6
#define bfd_mach_m68060                         7
#define bfd_mach_cpu32                          8
#define bfd_mach_mcf_isa_a_nodiv                9
#define bfd_mach_mcf_isa_a                     10
#define bfd_mach_mcf_isa_a_mac                 11
#define bfd_mach_mcf_isa_a_emac                12
#define bfd_mach_mcf_isa_aplus                 13
#define bfd_mach_mcf_isa_aplus_mac             14
#define bfd_mach_mcf_isa_aplus_emac            15
#define bfd_mach_mcf_isa_b_nousp               16
#define bfd_mach_mcf_isa_b_nousp_mac           17
#define bfd_mach_mcf_isa_b_nousp_emac          18
#define bfd_mach_mcf_isa_b                     19
#define bfd_mach_mcf_isa_b_mac                 20
#define bfd_mach_mcf_isa_b_emac                21
#define bfd_mach_mcf_isa_b_float               22
#define bfd_mach_mcf_isa_b_float_mac           23
#define bfd_mach_mcf_isa_b_float_emac          24
	bfd_arch_i386         =  9,
#define bfd_mach_i386_i386	                    1
#define bfd_mach_i386_i8086                     2
#define bfd_mach_i386_i386_intel_syntax         3
#define bfd_mach_x86_64                        64
#define bfd_mach_x86_64_intel_syntax           65
	bfd_arch_powerpc      = 21,
#define bfd_mach_ppc                           32
#define bfd_mach_ppc64                         64
#define bfd_mach_ppc_403                      403
#define bfd_mach_ppc_403gc                   4030
#define bfd_mach_ppc_505                      505
#define bfd_mach_ppc_601                      601
#define bfd_mach_ppc_602                      602
#define bfd_mach_ppc_603                      603
#define bfd_mach_ppc_ec603e                  6031
#define bfd_mach_ppc_604                      604
#define bfd_mach_ppc_620                      620
#define bfd_mach_ppc_630                      630
#define bfd_mach_ppc_750                      750
#define bfd_mach_ppc_860                      860
#define bfd_mach_ppc_a35                       35
#define bfd_mach_ppc_rs64ii                   642
#define bfd_mach_ppc_rs64iii                  643
#define bfd_mach_ppc_7400                    7400
#define bfd_mach_ppc_e500                     500
	bfd_arch_rs6000        = 22
#define bfd_mach_rs6lk                       6000
#define bfd_mach_rs6lk_rs1                   6001
#define bfd_mach_rs6lk_rsc                   6002
#define bfd_mach_rs6lk_rs2                   6003
};

enum bfd_flavour {
	bfd_target_unknown_flavour = 0,
	bfd_target_elf_flavour = 5
};

enum bfd_endian {
	BFD_ENDIAN_BIG,
	BFD_ENDIAN_LITTLE,
	BFD_ENDIAN_UNKNOWN
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

enum bfd_architecture bfd_get_arch(bfd *abfd);

unsigned long bfd_get_mach(bfd *abfd);

int
bfd_big_endian(bfd *abfd);

int
bfd_little_endian(bfd *abfd);

bfd*
bfd_asymbol_bfd(asymbol *sym);

bfd_vma
bfd_asymbol_value(asymbol *sym);

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

#define HAS_SYMS	0x10

flagword
bfd_get_file_flags(bfd *abfd);

enum bfd_flavour
bfd_get_flavour(bfd *abfd);

long
bfd_get_reloc_upper_bound(bfd *abfd, asection *sect);

asection *
bfd_get_section(asymbol *sym);

/* Note: this actually returns ld(alignment)! */
unsigned int
bfd_get_section_alignment(bfd *abfd, asection *sect);

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

bfd *
bfd_openr(const char *fname, const char *target);

void
bfd_perror(const char *msg);

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

/*
 * The following types and routines are NOT in BFD -- they
 * are added for ease of portability
 */

/* The code where relocations are dealt with in bfdstuff is
 * very concentrated.
 * We can be more efficient and save memory if we don't emulate
 * the BFD API.
 * Hence we provide an alternative.
 */

/* Table with relocations */
typedef struct pmbfd_areltab pmbfd_areltab;

typedef union  pmbfd_arelent pmbfd_arelent;

/* generate reloc table */
long
pmbfd_canonicalize_reloc(bfd *abfd, asection *sec, pmbfd_areltab *tab, asymbol **syms);

/* iterator over reloc table */
pmbfd_arelent *
pmbfd_reloc_next(bfd *abfd, pmbfd_areltab *tab, pmbfd_arelent *prev);


bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *reloc, asymbol *sym, asection *input_section);

int
pmbfd_reloc_get_sym_idx(bfd *abfd, pmbfd_arelent *reloc);

bfd_size_type
pmbfd_reloc_get_address(bfd *abfd, pmbfd_arelent *reloc);

const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *reloc);

symvalue
pmbfd_asymbol_set_value(asymbol *sym, symvalue v);

/* experimental stuff to keep libopcodes happy */
#define BFD_DEFAULT_TARGET_SIZE  32

#define CONST_STRNEQ(s1,s2) (0 == strncmp((s1),(s2),sizeof(s2)-1))
static inline int
sprintf_vma(char *s, bfd_vma vma)
{
	return sprintf(s,"0x%08lx",vma);
}

/* Byte swapping */

/* Read big endian into host order    */
bfd_vma bfd_getb32(const void *p);
/* Read little endian into host order */
bfd_vma bfd_getl32(const void *p);

#define bfd_octets_per_byte(abfd) (1)


/* CPU-specific instruction sets      */
unsigned bfd_m68k_mach_to_features (int);

/* Not in BFD; implemented so we can create a 'objdump'-compatible printout */
file_ptr
pmbfd_get_section_filepos(bfd *abfd, asection *section);

/* Not in BFD; create object attributes tables (see pmelf.h)
 * from a '.gnu.attributes' section.
 *
 * RETURNS: attribute set or NULL (if no '.gnu.attributes'
 *          section is present or if it could not be parsed.
 *
 * NOTE:    the caller is responsible for releasing resources
 *          by calling pmelf_destroy_attribute_set() when the
 *          table is no longer used.
 *
 *          The return value is really a (Pmelf_attribute_set *).
 */
void *
pmbfd_get_file_attributes(bfd *abfd);

#ifdef __cplusplus
};
#endif

#endif
