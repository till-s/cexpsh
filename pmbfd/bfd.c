#include "pmbfd.h"
#include "pmelf.h"

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

struct sec {
	const char   *name;
	bfd_size_type size;
	bfd_vma       vma;
	unsigned      align_power;
	flagword      flags;
	Elf32_Shdr   *shdr;
	asection     *grp_next;
};

#define SECCHUNKSZ	20

struct secchunk {
	asection	secs[SECCHUNKSZ];
	int         avail;
	struct secchunk *next;
};

static const bfd_arch_info_type myarch = {
	arch_name:
#if defined(__PPC__)
	"elf32-powerpc"
#elif defined(__i386__)
	"elf32-i386"
#elif defined(__m68k__)
	"elf32-m68k"
#else
#error "Undefined architecture"
#endif
};

/* find first bit set in x */

static unsigned ldz(uint32_t x)
{
unsigned rval = 0;
int      i;
static uint32_t msk[]={
	0xffff0000,
	0xff00ff00,
	0xf0f0f0f0,
	0xcccccccc,
	0xaaaaaaaa
};
	for ( i=0; i<sizeof(msk)/sizeof(msk[0]); i++ ) {
		rval = ( rval << 1 );
		if ( x & msk[i] ) {
			x &= msk[i];
			rval ++;
		} else {
			x &= ~msk[i];
		}
	}
	return rval;
}

/* Index into strtabs */
#define SYMSTRTAB	0

struct bfd {
	Elf_Stream      s;
	Elf32_Ehdr      ehdr;
	const char      *arch;
	asection        *sects;
	uint32_t        nsects;
	asymbol         *syms;
	uint32_t        nsyms;
	const char      **strtabs;
	uint32_t        nstrtabs;
	Txx_Elf32_Shtab	shtab;
	Elf32_Shdr      *symsh;	    /* sh of symbol table           */
	Elf32_Shdr		*symstrs;	/* sh of symbol table stringtab */
	struct secchunk	*secmemh;
	struct secchunk *secmemt;
};

const char *
bfd_printable_name(bfd *abfd)
{
	return abfd->arch;
}


static int inited = 0;

static bfd thebfd = { 0 };

static struct sec the_und_sec = {
	name: "UND",
};

static struct sec the_abs_sec = {
	name: "ABS",
};

static struct sec the_com_sec = {
	name: "COM",
};


asection *bfd_und_section_ptr = &the_und_sec;
asection *bfd_abs_section_ptr = &the_abs_sec;
asection *bfd_com_section_ptr = &the_com_sec;


asection * secget(bfd *abfd)
{
struct secchunk *c;

	if ( !abfd->secmemt || abfd->secmemt->avail >= SECCHUNKSZ ) {
		if ( ! (c=calloc(1,sizeof(*c))) ) 
			return 0;
		c->avail = 0;
		c->next  = 0;
		if ( abfd->secmemt )
			abfd->secmemt->next = c;
		abfd->secmemt       = c;
		if ( !abfd->secmemh )
			abfd->secmemh = c;
	}
	return &abfd->secmemt->secs[abfd->secmemt->avail++];
}

/* initialize bfd with 'n' (empty) asections */
int
secinit(bfd *abfd, unsigned n)
{
struct secchunk *c;

	while ( n > 0 ) {
		if ( ! (c = calloc(1, sizeof(*c))) ) {
			return -1;
		}
		c->avail = n > SECCHUNKSZ ? SECCHUNKSZ : n;

		if ( abfd->secmemt )
			abfd->secmemt->next = c;
		abfd->secmemt = c;
		if ( !abfd->secmemh )
			abfd->secmemh = c;

		n -= c->avail;
	}
	return 0;
}

asection * shdr2sec(bfd *abfd, Elf32_Word ndex)
{
struct secchunk *c;
	switch ( ndex ) {
		case SHN_ABS:    return bfd_abs_section_ptr;
		case SHN_UNDEF:  return bfd_und_section_ptr;
		case SHN_COMMON: return bfd_com_section_ptr;
		default: break;
	}
	c = abfd->secmemh;

	while ( c ) {
		if ( !c->avail )
			return 0;
		if ( ndex < c->avail )
			return &c->secs[ndex];
		ndex -= c->avail;
		c     = c->next;
	}

	return 0;
}

asection *
elf_next_in_group(asection *sec)
{
	return sec->grp_next;
}

elf_symbol_type *
elf_symbol_from(bfd *abfd, asymbol *sp);

bfd*
bfd_asymbol_bfd(asymbol *sym)
{
	return &thebfd;
}

bfd_vma
bfd_asymbol_value(asymbol *sym);

long
bfd_canonicalize_reloc(bfd *abfd, asection *sec, arelent **loc, asymbol **syms);

long
bfd_canonicalize_symtab(bfd *abfd, asymbol**);

bfd_boolean
bfd_check_format(bfd *abfd, bfd_format format)
{
	switch ( format ) {
		case bfd_object:
			if ( ET_REL == abfd->ehdr.e_type )
				return BFD_TRUE;
			break;
		case bfd_core:
			if ( ET_CORE == abfd->ehdr.e_type )
				return BFD_TRUE;
			break;
		default:
			break;
	}
	return BFD_FALSE;
}

bfd_boolean
bfd_close_all_done(bfd *abfd)
{
struct secchunk   *c,*cn;

	if ( abfd->s ) {
		txx_delstrm(abfd->s);
		abfd->s = 0;
	}
	if ( abfd->shtab ) {
		txx_delshtab(abfd->shtab);
		abfd->shtab = 0;
	}
	while ( abfd->nstrtabs > 0 ) {
		abfd->nstrtabs--;
		free( (void*)abfd->strtabs[ abfd->nstrtabs ] );
	}
	free( abfd->strtabs );
	abfd->strtabs = 0;
	for ( c=abfd->secmemh; c;  ) {
		cn = c->next;
		free(c);
		c  = cn;
	}
	abfd->secmemh = thebfd.secmemt = 0;

	return BFD_TRUE;
}

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

bfd_boolean
bfd_get_section_contents(bfd *abfd, asection *section, void *location, file_ptr offset, bfd_size_type count);

file_ptr
bfd_get_section_filepos(bfd *abfd, asection *section)
{
	return section->shdr ? section->shdr->sh_offset : -1;
}

flagword
bfd_get_section_flags(bfd *abfd, asection *sect)
{
	return sect->flags;
}

const char *
bfd_get_section_name(bfd *bfd, asection *sect)
{
	return sect->name;
}

bfd_vma
bfd_get_section_vma(bfd *bfd, asection *sect)
{
	return sect->vma;
}

bfd_vma
bfd_get_section_lma(bfd *bfd, asection *sect)
{
	return 0x0; /* would need to look into phdr... */
}

bfd_size_type
bfd_get_section_size(asection *sect)
{
	return sect->size;
}

unsigned
bfd_get_section_alignment(bfd *bfd, asection *sect)
{
	return sect->align_power;
}




long
bfd_get_symtab_upper_bound(bfd *bfd);

char *
bfd_get_target(bfd *abfd)
{
	switch ( abfd->ehdr.e_machine ) {
		case EM_PPC:	return "elf32-powerpc";
		case EM_68K:	return "elf32-m68k";
		case EM_386:	return "elf32-i386";
		default:
			break;
	}
	return "unknown";
}

char *
bfd_get_unique_section_name(bfd *abfd, const char *template, int *count);

void
bfd_init(void)
{
	if ( !inited ) {
		memset( &thebfd, 0, sizeof(thebfd));
		inited = 1;
	}
}

int
bfd_is_abs_section(asection *sect);

int
bfd_is_com_section(asection *sect);

int
bfd_is_und_section(asection *sect);

asymbol *
bfd_make_empty_symbol(bfd *bfd);

asection *
bfd_make_section(bfd *abfd, const char *name)
{
asection *rval;
	if ( (rval = secget(abfd)) ) {
		rval->name = name;
	}
	return rval;
}

void
bfd_map_over_sections(bfd *abfd, void (*f)(bfd *abfd, asection *sect, void *closure), void *closure)
{
struct secchunk *c;
int              i;
	for ( c = abfd->secmemh; c; c=c->next ) {
		for ( i=0; i<c->avail; i++ ) {
			if ( c->secs[i].flags != SEC_BOGUS )
				f(abfd, c->secs+i, closure);
		}
	}
}

struct elf2bfddat {
	Elf32_Word	idx;
	int         err;
};

static void elf2bfdsec(bfd *abfd, asection *sec, void *closure)
{
struct elf2bfddat *p    = closure;
Elf32_Shdr        *shdr = &abfd->shtab->shdrs[p->idx];
asection          *s;
Elf32_Word        *grp;
Elf32_Word        n;
Elf32_Sym         sym;
int               i;
flagword          flags = 0;

	/* Some ELF sections don't make it into the bfd section table; nevertheless,
	 * allocate an asection for every ELF section so that we can easily
	 * find the associated asection given the ELF index...
	 */

	/* mark as bogus */
	sec->flags       = SEC_BOGUS;

	sec->name        = &abfd->shtab->strtab[shdr->sh_name];
	sec->size        = shdr->sh_size;
	sec->vma         = shdr->sh_addr;
	sec->align_power = ldz(shdr->sh_addralign);
	sec->shdr        = shdr;
	sec->grp_next    = 0;

	p->idx++;

	switch ( shdr->sh_type ) {
		case SHT_NULL:	return;

		case SHT_PROGBITS:
		case SHT_NOBITS:
		case SHT_HASH:
		case SHT_NOTE:
		case SHT_INIT_ARRAY:
		case SHT_FINI_ARRAY:
		case SHT_PREINIT_ARRAY:
		case SHT_DYNAMIC: /* FIXME: extra action needed for this one ? -- should not be used by cexp anyways */
						break;

		case SHT_SYMTAB:
						/* pmelf already verified that there is one and only one symtab */
						return;

		case SHT_DYNSYM:
						/* hmm - not sure what to do here (should not be used by cexp) */
						return;

		case SHT_SYMTAB_SHNDX:
						ERRPR("unsupported SHT_SYMTAB_SHNDX section found\n");
						p->err = -1;
						return;	

		case SHT_STRTAB:
						/* treat as normal section if not one of our recognized strtabs */
						if (   (shdr - abfd->shtab->shdrs) == abfd->ehdr.e_shstrndx
								|| (shdr                   == abfd->symstrs)
						   )
							return;
						break;

		case SHT_REL:
		case SHT_RELA: /* TODO: deal with relocations */
						return;

		case SHT_GROUP:
						grp = txx_getgrp(abfd->s, shdr, 0);
						if ( !grp ) {
							p->err = 1;
							return;
						}
						if ( GRP_COMDAT & grp[0] ) {
							n = shdr->sh_size / sizeof(grp[0]);
							for (i=1, s=sec; i<n; i++, s=s->grp_next) {
								s->grp_next = shdr2sec(abfd, grp[i]);
							}
							/* BFD provides a closed 'loop' */
							s->grp_next = sec;
							flags |= SEC_LINK_ONCE | SEC_LINK_DUPLICATES_DISCARD;
						}
						free(grp);
						/* asection name is group signature */
						if ( shdr->sh_link != abfd->symsh - abfd->shtab->shdrs ) {
							ERRPR("Group name symbol not in one-and-only symtab!\n");
							p->err = 1;
							return;
						}

						/* need to read the symbol */
						if ( txx_seek(abfd->s, abfd->symsh->sh_offset + sizeof(sym)*shdr->sh_info) ) {
							bfd_perror("unable to seek to group signature symbol");
							p->err = 1;
							return;
						}

						if ( txx_getsym(abfd->s, &sym) ) {
							bfd_perror("unable to read group signature symbol");
							p->err = 1;
							return;
						}

						sec->name   = &abfd->strtabs[SYMSTRTAB][sym.st_name];
						flags |= SEC_GROUP | SEC_EXCLUDE;
						break;

		default:
						ERRPR("Unknown section type 0x%"PRIx32" found\n", shdr->sh_type);
						p->err = -1;
						return;	
	}

	if ( shdr->sh_type != SHT_NOBITS )
		flags |= SEC_HAS_CONTENTS;

	if ( shdr->sh_flags & SHF_ALLOC ) {
		flags |= SEC_ALLOC;
		if ( shdr->sh_type != SHT_NOBITS )
			flags |= SEC_LOAD;
	}
	
	if ( ! (shdr->sh_flags & SHF_WRITE) ) {
		flags |= SEC_READONLY;
	}

	if ( shdr->sh_flags & SHF_EXECINSTR ) {
		flags |= SEC_CODE;
	} else if ( flags & SEC_LOAD ) {
		flags |= SEC_DATA;
	}

	if ( shdr->sh_flags & SHF_MERGE ) {
		flags |= SEC_MERGE;
		if ( shdr->sh_flags & SHF_STRINGS )
			flags |= SEC_STRINGS;
	}

	if ( ! (flags & SEC_ALLOC) ) {
		if (   !strncmp(sec->name, ".debug", 6) 
		    || !strncmp(sec->name, ".gnu.linkonce.wi.", 17)
		    || !strncmp(sec->name, ".line", 5)
		    || !strncmp(sec->name, ".stab", 5)
			)
			flags |= SEC_DEBUGGING;
	}

	if ( !strncmp( sec->name, ".gnu.linkonce", 13) )
		flags |= SEC_LINK_ONCE | SEC_LINK_DUPLICATES_DISCARD;

	sec->flags = flags;
}

static int
mark_relocs(bfd *abfd)
{
uint32_t   n;
asection   *tsec;
Elf32_Shdr *shdr;

	for ( n = 1, shdr = abfd->shtab->shdrs + n; n<abfd->shtab->nshdrs; n++, shdr++ ) {
		switch ( shdr->sh_type ) {
			case SHT_REL:
			case SHT_RELA:
				break;
			default:
				continue;
		}


		if ( ! (tsec = shdr2sec(abfd, shdr->sh_info)) ) {
			ERRPR("pmbfd: no asection found associated with REL/RELA section %s (sh_info: %d)\n",
				&abfd->shtab->strtab[shdr->sh_name],
				shdr->sh_info);
			return -1;
		}
		if ( SEC_BOGUS == tsec->flags ) {
			ERRPR("pmbfd: REL/RELA section %s (sh_info: %d) targeting BOGUS section %s\n",
				&abfd->shtab->strtab[shdr->sh_name],
				shdr->sh_info,
				tsec->name);
			return -1;
		}
		tsec->flags |= SEC_RELOC;
	}
	return 0;
}


bfd *
bfd_openstreamr(const char *fname, const char *target, FILE *f)
{
long              nsyms;
const char        *strs;
struct elf2bfddat d;
struct bfd        *abfd = &thebfd;

	if ( ! inited ) {
		ERRPR("pmbfd: not initialized\n");
		return 0;
	}

	if ( abfd->s ) {
		ERRPR("pmbfd: cannot open multiple streams at once\n");
		return 0;
	}
	if ( target ) {
		ERRPR("bfd_openstreamr(): 'target' arg not supported\n");
		return 0;
	}
	if ( ! (abfd->s = txx_newstrm(0, f)) )
		return 0;

	if ( txx_getehdr(abfd->s, &abfd->ehdr) ) {
		goto cleanup;
	}

	if ( ! (abfd->shtab = txx_getshtab(abfd->s, &abfd->ehdr)) ) {
		goto cleanup;
	}

	if ( (nsyms = txx_find_symhdrs(abfd->s, abfd->shtab, &abfd->symsh, &abfd->symstrs)) < 0 ) {
		goto cleanup;
	}

	/* get string table */
	if ( ! (strs = txx_getscn( abfd->s, abfd->symstrs, 0)) ) {
		goto cleanup;
	}

	if ( ! (abfd->strtabs = malloc( sizeof(strs))) ) {
		free((void*)strs);
		goto cleanup;
	}

	/* SYMSTRTAB */
	abfd->strtabs[abfd->nstrtabs++] = strs;
	
	/* make section table */

	if ( secinit(abfd, abfd->shtab->nshdrs) ) {
		ERRPR("Not enough memory for bfd section table\n");
		goto cleanup;
	}

	/* at this point we ONLY have ELF sections so we can use
	 * bfd_map_over_sections
	 */
	d.idx = 0;
	d.err = 0;
	bfd_map_over_sections(abfd, elf2bfdsec, &d);
	if ( d.err )
		goto cleanup;

	if ( mark_relocs(abfd) )
		goto cleanup;


	return abfd;

cleanup:

	bfd_close_all_done(abfd);
	return 0;
}

bfd_reloc_status_type
bfd_perform_relocation(bfd *abfd, arelent *relent, void *data, asection *input_section, bfd *output_bfd, char **p_err_msg);

void
bfd_perror(const char *msg)
{
	perror(msg);
}

const char *
bfd_printable_arch_mach(enum bfd_architecture arch, unsigned long mach);

const bfd_arch_info_type *
bfd_scan_arch(const char *str)
{
	return str && myarch.arch_name && !strcasecmp(myarch.arch_name, str) ? &myarch : 0;
}

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
