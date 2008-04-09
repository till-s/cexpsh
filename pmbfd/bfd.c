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

/* Assume we have less than 64k sections */
typedef uint16_t	Secndx;

struct sec {
	const char   *name;
	bfd_size_type size;
	bfd_vma       vma;
	unsigned      align_power;
	flagword      flags;
	Elf32_Shdr   *shdr;
#define GRP_NULL	0
	Secndx       grp_next;		/* half-word */
#define RELS_NULL	0
	Secndx       rels;		    /* half-word */
};

static void
bfd_set_section_name(asection *sect, const char *name)
{
	sect->name = name;
}


#define SECCHUNKSZ       0
#define MAX_USR_SECTS    5
#define STRCHUNKSZ	   504
#define SYMCHUNKSZ      50

#if SECCHUNKSZ > 0
struct secchunk {
	asection	secs[SECCHUNKSZ];
	int         avail;
	struct secchunk *next;
};
#endif

#if SYMCHUNKSZ > 0
struct symchunk {
	asymbol	syms[SYMCHUNKSZ];
	int     avail;
	struct symchunk *next;
};
#endif

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
#if SECCHUNKSZ == 0
	asection        *sects;
	uint32_t        nsects;
	uint32_t		maxsects;
#endif
	asymbol         *syms;
	long            nsyms;
	const char      **strtabs;
	uint32_t        nstrtabs;
	uint32_t		str_avail;
	Txx_Elf32_Shtab	shtab;
	Elf32_Shdr      *symsh;	    /* sh of symbol table           */
	Elf32_Shdr		*symstrs;	/* sh of symbol table stringtab */
#if SECCHUNKSZ > 0
	struct secchunk	*secmemh;
	struct secchunk *secmemt;
#endif
	struct symchunk	*symmemh;
	struct symchunk *symmemt;
};

const char *
bfd_printable_name(bfd *abfd)
{
	return abfd->arch;
}

static int inited = 0;

static bfd thebfd = { 0 };

static struct sec the_und_sec = {
	name: "*UND*",
};

static struct sec the_abs_sec = {
	name: "*ABS*",
};

static struct sec the_com_sec = {
	name: "*COM*",
};

asection *bfd_und_section_ptr = &the_und_sec;
asection *bfd_abs_section_ptr = &the_abs_sec;
asection *bfd_com_section_ptr = &the_com_sec;

static const char *
elf_sym_name(bfd *abfd, Elf32_Sym *sym)
{
	/* assume everything is initialized */
	return sym->st_name >= abfd->symstrs->sh_size ? 0 : &abfd->strtabs[SYMSTRTAB][sym->st_name];
}

#if 1	/* currently unused; leave here in case we need it in the future */
static char *stralloc(bfd *abfd, uint32_t len)
{
char       *rval;
const char **nst;
	if ( len > STRCHUNKSZ ) {
		ERRPR("String table implementation limits string length to %u bytes\n", STRCHUNKSZ);
		return 0;
	}
	if (abfd->str_avail < len) {
		if ( ! (nst = realloc(abfd->strtabs, (abfd->nstrtabs+1) * sizeof(*abfd->strtabs))) ) {
			ERRPR("Memory allocation for string table failed\n");
			return 0;
		}
		abfd->strtabs = nst;
		abfd->nstrtabs++;
		abfd->str_avail = STRCHUNKSZ;
	}
	rval = (char *)&abfd->strtabs[abfd->nstrtabs-1][STRCHUNKSZ - abfd->str_avail];
	abfd->str_avail -= len;
	return rval;
}
#endif

#if SECCHUNKSZ > 0
static asection *
secget(bfd *abfd)
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
static int
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

#else 

static asection *
secget(bfd *abfd)
{
	return abfd->nsects < abfd->maxsects ? &abfd->sects[abfd->nsects++] : 0;
}


static int
secinit(bfd *abfd, unsigned n)
{
	if ( ! (abfd->sects = calloc( n + MAX_USR_SECTS , sizeof(asection) )) ) {
		return -1;
	}
	abfd->nsects   = n;
	abfd->maxsects = n + MAX_USR_SECTS;
	return 0;
}

#endif

static asymbol *
symget(bfd *abfd)
{
struct symchunk *c;

	if ( !abfd->symmemt || abfd->symmemt->avail >= SYMCHUNKSZ ) {
		if ( ! (c=calloc(1,sizeof(*c))) ) 
			return 0;
		c->avail = 0;
		c->next  = 0;
		if ( abfd->symmemt )
			abfd->symmemt->next = c;
		abfd->symmemt       = c;
		if ( !abfd->symmemh )
			abfd->symmemh = c;
	}
	return &abfd->symmemt->syms[abfd->symmemt->avail++];
}


static asection *
shdr2sec(bfd *abfd, Elf32_Word ndex, int return_bogus)
{
asection        *sec;
	switch ( ndex ) {
		case SHN_ABS:    return bfd_abs_section_ptr;
		case SHN_UNDEF:  return bfd_und_section_ptr;
		case SHN_COMMON: return bfd_com_section_ptr;
		default: break;
	}

#if SECCHUNKSZ > 0
	{
	struct secchunk *c;
	c = abfd->secmemh;

	while ( c ) {
		if ( !c->avail )
			return 0;
		if ( ndex < c->avail ) {
			sec = &c->secs[ndex];
			return return_bogus || SEC_BOGUS != bfd_get_section_flags(abfd, sec) ? sec : 0;
		}
		ndex -= c->avail;
		c     = c->next;
	}
	}
#else
	if ( ndex >= abfd->nsects )
		return 0;
	sec = &abfd->sects[ndex];
	if ( return_bogus || SEC_BOGUS != bfd_get_section_flags(abfd, sec) )
		return sec;
#endif

	return 0;
}

asection *
elf_next_in_group(asection *sec)
{
	return shdr2sec(&thebfd, sec->grp_next, 0);
}

bfd*
bfd_asymbol_bfd(asymbol *sym)
{
	return &thebfd;
}

bfd_vma
bfd_asymbol_value(asymbol *sym)
{
symvalue base;
asection *sec = bfd_get_section(sym);

	/* If this is a COM symbol, BFD wants the size in the value field */
	if ( bfd_is_com_section(sec) ) {
		return elf_get_size(&thebfd, sym);
	}

	base = sec ? bfd_get_section_vma(&thebfd, sec) : 0;
	return base + sym->value;
}

int
elf_get_size(bfd *abfd, asymbol *asym)
{
	return asym->internal_elf_sym.st_size;
}

unsigned
elf_get_align(bfd *abfd, asymbol *asym)
{
unsigned rval;
	/* must never return 0; minimal alignment is 1 */
	if ( bfd_is_com_section(bfd_get_section(asym)) && (rval = asym->value) )
		return rval;
	return 1;
}


long
bfd_canonicalize_symtab(bfd *abfd, asymbol** psymtab)
{
unsigned n;
Elf32_Sym esym;
asymbol   *asym;
long      rval = -1;
asection  *symsec;

	if ( !abfd->syms ) {
		if ( ! (abfd->syms = malloc(sizeof(asymbol)*(abfd->nsyms))) ) {
			ERRPR("No memory for symbol table\n");
			return -1;
		}
		/* seek skipping ELF symbol #0 */
		if ( txx_seek(abfd->s, abfd->symsh->sh_offset + sizeof(esym)) ) {
			bfd_perror("unable to seek to symbol table");
			goto bail;
		}
		for ( n = 0, asym = abfd->syms; n<abfd->nsyms; n++, asym++ ) {
			if ( txx_getsym(abfd->s, &esym) ) {
				bfd_perror("unable to read symbol table");
				goto bail;
			}
			if ( ! (asym->name = elf_sym_name(abfd, &esym)) ) {
				ERRPR("Bad symbol name; ELF index possibly out of bounds\n");
				goto bail;
			}
			asym->value                     = 
			asym->internal_elf_sym.st_value = esym.st_value;
			asym->internal_elf_sym.st_size  = esym.st_size;
			asym->flags                     = 0;

			if ( ! (symsec = shdr2sec(abfd, esym.st_shndx, 1)) ) {
				ERRPR("Symbol %s pointing to NO section (idx %"PRIu16")\n",
						asym->name, esym.st_shndx);
				goto bail;
			}

			asym->section = symsec;

			/* if the section already has a vma recalculate the offset */
			asym->value -= bfd_get_section_vma(asym);

			switch ( ELF32_ST_BIND( esym.st_info ) ) {
				default:
				break;

				case STB_LOCAL: asym->flags |= BSF_LOCAL;
				break;

				case STB_WEAK : asym->flags |= BSF_WEAK;
				break;

				case STB_GLOBAL:
					if ( esym.st_shndx != SHN_UNDEF && esym.st_shndx != SHN_COMMON )
						asym->flags |= BSF_GLOBAL;
				break;
			}

			switch ( ELF32_ST_TYPE( esym.st_info ) ) {
				case STT_FUNC:		asym->flags |= BSF_FUNCTION;				break;
				case STT_OBJECT:	asym->flags |= BSF_OBJECT;					break;
				case STT_FILE:		asym->flags |= BSF_FILE | BSF_DEBUGGING;	break;
				case STT_SECTION:
					asym->flags |= BSF_SECTION_SYM | BSF_DEBUGGING;
					/* fixup the symbol name to use the section name */
					if ( !asym->name || !*asym->name )
						asym->name = bfd_get_section_name(abfd, symsec);
				
				break;
			}

			/* point bogus sections (no BFD corresponding to ELF section was
			 * generated [-- match BFD behavior]) to ABS. Do this *after* the
			 * symbol name of section syms has been fixed up!
			 */
			if ( SEC_BOGUS == bfd_get_section_flags(abfd, symsec) )
				asym->section = bfd_abs_section_ptr;

#if 0	/**** This is now done by bfd_asymbol_value() ****/
			if ( bfd_is_com_section(asym->section) ) {
				/* Fixup the value; BFD returns the size in 'value' for
				 * common symbols (the ELF st_value field holds the
				 * alignment).
				 */
				asym->value = esym.st_size;
			}
#endif

		}

	}
	/* copy out pointers */
	for ( n=0; n<abfd->nsyms; n++ )
		psymtab[n] = &abfd->syms[n];

	/* tag with final NULL */
	psymtab[n] = 0;

	rval = abfd->nsyms;

bail:
	if ( rval < 0 ) {
		free( abfd->syms );
		abfd->syms = 0;
	}

	return rval;
}

bfd_boolean
bfd_check_format(bfd *abfd, bfd_format format)
{
	switch ( format ) {
		case bfd_object:
			if ( ET_REL == abfd->ehdr.e_type
				/* also accept ET_EXEC for linux test program ... */
				|| ET_EXEC == abfd->ehdr.e_type )
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

static void bfd_cleanup(bfd *abfd, int noclose)
{

	if ( abfd->s ) {
		txx_delstrm(abfd->s, noclose);
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
	abfd->strtabs   = 0;
	abfd->str_avail = 0;

#if SECCHUNKSZ > 0
	{
	struct secchunk   *c,*cn;

	for ( c=abfd->secmemh; c;  ) {
		cn = c->next;
		free(c);
		c  = cn;
	}
	abfd->secmemh = abfd->secmemt = 0;
	}
#else
	free( abfd->sects );
	abfd->sects    = 0;
	abfd->nsects   = 0;
	abfd->maxsects = 0;
#endif
	{
	struct symchunk   *c,*cn;

	for ( c=abfd->symmemh; c;  ) {
		cn = c->next;
		free(c);
		c  = cn;
	}
	abfd->symmemh = abfd->symmemt = 0;
	}

	free(abfd->syms);
	abfd->syms    = 0;

	abfd->nsyms   = 0;

}

bfd_boolean
bfd_close_all_done(bfd *abfd)
{
	bfd_cleanup(abfd, 0);
	return BFD_TRUE;
}


#if 0
bfd_boolean
bfd_discard_group(bfd *, asection *);
#endif

flagword
bfd_get_file_flags(bfd *abfd)
{
flagword v = 0;
/* ONLY HAS_SYMS is currently implemented */
	if ( abfd->nsyms > 0 )
		v |= HAS_SYMS;
	return v;
}

enum bfd_flavour
bfd_get_flavour(bfd *abfd)
{
	return bfd_target_elf_flavour;
}

asection *
bfd_get_section(asymbol *sym)
{
	return sym->section;
}

/* Not in BFD; implemented so we can create a 'objdump'-compatible printout */
file_ptr
pmbfd_get_section_filepos(bfd *abfd, asection *section)
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

/* Note: this actually returns ld(alignment)! */
unsigned
bfd_get_section_alignment(bfd *bfd, asection *sect)
{
	return sect->align_power;
}

long
bfd_get_symtab_upper_bound(bfd *abfd)
{
	/* BFD returns 1 entry more than needed (terminating NULL) */
	return (abfd->nsyms + 1) * sizeof(asymbol *);
}

char *
bfd_get_target(bfd *abfd)
{
	switch ( abfd->ehdr.e_machine ) {
		case EM_PPC:	return "elf32-powerpc";
		case EM_68K:	return "elf32-m68k";
		case EM_386:	return "elf32-i386";
		default:
			switch ( abfd->ehdr.e_ident[EI_DATA] ) {
				case ELFDATA2LSB: return "elf32-little";
				case ELFDATA2MSB: return "elf32-big";
				default:
				break;
			}
			break;
	}
	return "unknown";
}

char *
bfd_get_unique_section_name(bfd *abfd, const char *template, int *count)
{
static unsigned id = 0;
char *rval;
int  len;

	if ( count ) {
		if ( *count > id )
			id = *count;
		else
			*count = id;
	}

	if ( id > 999999 )
		return 0;

	len = 8;

	if ( template ) 
		len += strlen(template);

	if ( ! (rval = malloc(len)) )
		return 0;

	snprintf(rval, len, "%s.%.6u", template, id);
	rval[len-1]=0;

	id ++;

	return rval;
}

void
bfd_init(void)
{
	if ( !inited ) {
		txx_set_errstrm(stdout);
		memset( &thebfd, 0, sizeof(thebfd));
		inited = 1;
	}
}

int
bfd_is_abs_section(asection *sect)
{
	return sect == bfd_abs_section_ptr;
}

int
bfd_is_com_section(asection *sect)
{
	return sect == bfd_com_section_ptr;
}

int
bfd_is_und_section(asection *sect)
{
	return sect == bfd_und_section_ptr;
}

asection *
bfd_make_section(bfd *abfd, const char *name)
{
asection *rval;
	if ( (rval = secget(abfd)) ) {
		bfd_set_section_name(rval, name);
	}
	return rval;
}

void
bfd_map_over_sections(bfd *abfd, void (*f)(bfd *abfd, asection *sect, void *closure), void *closure)
{
int              i;
#if SECCHUNKSZ > 0
struct secchunk *c;
	for ( c = abfd->secmemh; c; c=c->next ) {
		for ( i=0; i<c->avail; i++ ) {
			if ( bfd_get_section_flags(abfd, &c->secs[i]) != SEC_BOGUS )
				f(abfd, c->secs+i, closure);
		}
	}
#else
asection *sec;
	for ( i = 0, sec = abfd->sects; i < abfd->nsects; i++, sec++ ) {
		if ( bfd_get_section_flags(abfd, sec) != SEC_BOGUS )
			f(abfd, sec, closure);
	}
#endif
}

struct elf2bfddat {
	Elf32_Word	idx;
	int         err;
};

static void elf2bfdsec(bfd *abfd, asection *sec, void *closure)
{
struct elf2bfddat *p    = closure;
Secndx            idx   = p->idx;
Elf32_Shdr        *shdr = &abfd->shtab->shdrs[idx];
asection          *s;
Elf32_Word        *grp;
Elf32_Word        n;
Elf32_Sym         sym;
int               i;
flagword          flags = 0;
const char        *sname;

	/* Some ELF sections don't make it into the bfd section table; nevertheless,
	 * allocate an asection for every ELF section so that we can easily
	 * find the associated asection given the ELF index...
	 */

	/* mark as bogus */
	bfd_set_section_flags(abfd, sec, SEC_BOGUS);

	if ( ! (sname = txx_sec_name(abfd->shtab, shdr)) ) {
		ERRPR("No section name; ELF index into shstrtab possibly out of range\n");
		p->err = 1;
		return;
	}
	bfd_set_section_name(sec, sname);
	bfd_set_section_size(abfd, sec, shdr->sh_size);
	bfd_set_section_vma(abfd, sec, shdr->sh_addr);
	bfd_set_section_alignment(abfd, sec, ldz(shdr->sh_addralign));
	sec->shdr        = shdr;
	/* must not touch grp_next since setting up groups
	 * doesn't follow the order in which we scan the array
	 * of sections.
	sec->grp_next    = GRP_NULL;
	 */


	/* initializing rels is OK since relocs are marked
	 * during a second pass anyways
	 */
	sec->rels        = RELS_NULL;

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
		case SHT_RELA:
						return;

		case SHT_GROUP:
						grp = txx_getgrp(abfd->s, shdr, 0);
						if ( !grp ) {
							p->err = 1;
							return;
						}
						if ( GRP_COMDAT & grp[0] ) {
							n = shdr->sh_size / sizeof(grp[0]);
							for (i=1, s=sec; i<n; i++) {
								/* assume groups do not include 'special' sections
								 * (like UND/ABS etc.)
								 */
								if ( 0 == grp[i] || grp[i] >= abfd->shtab->nshdrs ) {
									ERRPR("Group member section index possibly out of bounds\n");
									s->grp_next = GRP_NULL;
									p->err      = 1;
									return;
								}
								s->grp_next = grp[i];
								if ( ! (s = elf_next_in_group(s)) ) {
									ERRPR("Fatal error; should never get here\n");
									p->err      = 1;
									return;
								}
							}
							/* BFD provides a closed 'loop' */
							s->grp_next = idx;
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

						/* apparently, ld -r puts the group name into the section name
						 * and the symbol linked by the group header's sh_link
						 * has a NULL st_name :-( and is made a section symbol...
						 */
						if ( sym.st_name || ELF32_ST_TYPE(sym.st_info) != STT_SECTION ) {
							const char *esnm;
							if ( ! (esnm = elf_sym_name(abfd, &sym)) ) {
								ERRPR("Bad symbol name; index possibly out of bounds\n");
								p->err = -1;
								return;
							}
							bfd_set_section_name(sec, esnm);
						}
						flags    |= SEC_GROUP | SEC_EXCLUDE;
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
		if (   !strncmp(sname, ".debug", 6) 
		    || !strncmp(sname, ".gnu.linkonce.wi.", 17)
		    || !strncmp(sname, ".line", 5)
		    || !strncmp(sname, ".stab", 5)
			)
			flags |= SEC_DEBUGGING;
	}

	/* group members don't have the LINK_ONCE flag set; so if
	 * this section is already member of a link-once group then
	 * don't raise the LINK_ONCE flag even if the section
	 * name matches...
	 */
	if ( GRP_NULL == sec->grp_next
		 && !strncmp(sname, ".gnu.linkonce", 13) )
		flags |= SEC_LINK_ONCE | SEC_LINK_DUPLICATES_DISCARD;

	bfd_set_section_flags(abfd, sec, flags);
}

static int
mark_relocs(bfd *abfd)
{
uint32_t   n;
asection   *tsec;
Elf32_Shdr *shdr;
const char *name;
int        rval = 0;

	for ( n = 1, shdr = abfd->shtab->shdrs + n; n<abfd->shtab->nshdrs; n++, shdr++ ) {
		switch ( shdr->sh_type ) {
			case SHT_REL:
			case SHT_RELA:
				break;
			default:
				continue;
		}


		if ( ! (tsec = shdr2sec(abfd, shdr->sh_info, 0)) ) {
			if ( !(name = txx_sec_name(abfd->shtab, shdr)) )
				name = "<OUT-OF-BOUNDS>";
			ERRPR("pmbfd: no asection found associated with REL/RELA section %s (sh_info: %d)\n",
				name,
				shdr->sh_info
			);
			return -1;
		}
		if ( SEC_BOGUS == bfd_get_section_flags(abfd, tsec) ) {
			if ( !(name = txx_sec_name(abfd->shtab, shdr)) )
				name = "<OUT-OF-BOUNDS>";
			ERRPR("pmbfd: REL/RELA section %s (sh_info: %d) targeting BOGUS section %s\n",
				name,
				shdr->sh_info,
				bfd_get_section_name(abfd, tsec)
			);
			return -1;
		}
		if ( RELS_NULL != tsec->rels ) {
			ERRPR("pmbfd: section %s has more than 1 associated sections with relocations -- currently unsupported, sorry\n",
				bfd_get_section_name(abfd, tsec)
			);
			return -1;	      
		}
		tsec->rels = n;
		bfd_set_section_flags(abfd, tsec, bfd_get_section_flags(abfd, tsec) | SEC_RELOC);
		rval++;
	}
	return rval;
}


bfd *
bfd_openstreamr(const char *fname, const char *target, FILE *f)
{
const char        *strs;
struct elf2bfddat d;
struct bfd        *abfd = &thebfd;
int               nrels;

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

	abfd->arch = bfd_get_target(abfd);

	if ( ! (abfd->shtab = txx_getshtab(abfd->s, &abfd->ehdr)) ) {
		goto cleanup;
	}

	if ( (abfd->nsyms = txx_find_symhdrs(abfd->s, abfd->shtab, &abfd->symsh, &abfd->symstrs)) < 2 ) {
		goto cleanup;
	}
	/* ignore symbol #0 */
	abfd->nsyms--;

	/* get string table */
	if ( ! (strs = txx_getscn( abfd->s, abfd->symstrs, 0, 0, 0)) ) {
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

	if ( (nrels = mark_relocs(abfd)) < 0 )
		goto cleanup;

	return abfd;

cleanup:

	bfd_cleanup(abfd, 1);
	return 0;
}

void
bfd_perror(const char *msg)
{
	perror(msg);
}

const bfd_arch_info_type *
bfd_scan_arch(const char *str)
{
	return str && myarch.arch_name && !strcasecmp(myarch.arch_name, str) ? &myarch : 0;
}

bfd_boolean
bfd_set_section_alignment(bfd *abfd, asection *sect, unsigned int val)
{
	sect->align_power = val;
	return BFD_TRUE;
}

bfd_size_type
bfd_section_size(bfd *abfd, asection *sect)
{
	return sect->size;
}

bfd_boolean
bfd_set_section_flags(bfd *abfd, asection *sect, flagword flags)
{
	sect->flags = flags;
	return BFD_TRUE;
}

bfd_boolean
bfd_set_section_size(bfd *abfd, asection *sect, bfd_size_type val)
{
	sect->size = val;
	return BFD_TRUE;
}

bfd_boolean
bfd_set_section_vma(bfd *abfd, asection *sect, bfd_vma vma)
{
	sect->vma = vma;
	return BFD_TRUE;
}

asection *
bfd_set_section(asymbol *sym, asection *sect)
{
	return (sym->section = sect);
}

bfd_boolean
bfd_get_section_contents(bfd *abfd, asection *section, void *location, file_ptr offset, bfd_size_type count)
{
	if ( 0 == count )
		return BFD_TRUE;

	if ( ! (bfd_get_section_flags(abfd, section) & SEC_HAS_CONTENTS) ) {
		memset(location, 0, count);
		return BFD_TRUE;
	}

	/* test for shdr presence after checking HAS_CONTENTS so we can clear
	 * out a new common section (which has no shdr)
	 */

	if ( !section->shdr )
		return BFD_FALSE;

	return txx_getscn(abfd->s, section->shdr, location, offset, count) ? BFD_TRUE : BFD_FALSE;
}

asymbol *
bfd_make_empty_symbol(bfd *abfd)
{
	return symget(abfd);
}

struct pmbfd_areltab {
	Elf32_Word	entsz;
	Elf32_Shdr  *shdr;
	uint8_t		data[];
};

union pmbfd_arelent {
	Elf32_Rel	rel;
	Elf32_Rela	rela;
	char        raw[sizeof(Elf32_Rela)];
};

static asection *
get_reloc_sec(bfd *abfd, asection *sect)
{
asection *rels;
uint32_t sz;
	if ( RELS_NULL == sect->rels  ||  ! (rels = shdr2sec(abfd, sect->rels, 1)) ) {
		ERRPR("pmbfd: section (%s) has no relocs\n", bfd_get_section_name(abfd, sect));
		return 0;
	}
	switch ( rels->shdr->sh_type ) {
		case SHT_REL:  sz = sizeof(Elf32_Rel); break;
		case SHT_RELA: sz = sizeof(Elf32_Rela); break;

		default:
		ERRPR("pmbfd: reloc section not of type REL/RELA! (but %u)\n", rels->shdr->sh_type);
		return 0;
	}
	if ( sz != rels->shdr->sh_entsize ) {
		ERRPR("pmbfd: reloc section entry size mismatch\n");
		return 0;
	}
	return rels;
}

long
bfd_canonicalize_reloc(bfd *abfd, asection *sec, pmbfd_areltab *tab, asymbol **syms)
{
asection *rels;
long     nrels;

	if ( !(rels = get_reloc_sec(abfd, sec)) ) {
		return -1;
	}

	/* sh_entsize was checked by get_reloc_sec() */
	nrels = rels->shdr->sh_size / rels->shdr->sh_entsize;

	/* read relocations */
	if ( nrels > 0 ) {
		if ( ! (txx_getscn(abfd->s, rels->shdr, tab->data, 0, 0)) ) {
			bfd_perror("Unable to read relocations\n");	
			return -1;
		}
	}

	tab->entsz = rels->shdr->sh_entsize;
	tab->shdr  = rels->shdr;
		
	return nrels;
}

long
bfd_get_reloc_upper_bound(bfd *abfd, asection *sect)
{
asection *rels = get_reloc_sec(abfd, sect);
	if ( !rels ) 
		return -1;
	return sizeof(pmbfd_areltab) + rels->shdr->sh_size;
}

/*
 * SysvR4 relocation types for i386. Statically linked objects
 * just use R_386_32 and R_386_PC32 which makes our job really easy...
 */

#define R_386_NONE         0
#define R_386_32	       1
#define R_386_PC32	       2
#define R_386_GOT32        3
#define R_386_PLT32        4
#define R_386_COPY         5
#define R_386_GLOB_DAT     6
#define R_386_JMP_SLOT     7
#define R_386_RELATIVE     8
#define R_386_GOTOFF       9
#define R_386_GOTPC       10

/*
 * 68k relocation types
 */

#define R_68K_NONE         0
#define R_68K_32           1
#define R_68K_16           2
#define R_68K_8            3
#define R_68K_PC32         4
#define R_68K_PC16         5
#define R_68K_PC8          6
#define R_68K_GOT32        7
#define R_68K_GOT16        8
#define R_68K_GOT8         9
#define R_68K_GOT320      10
#define R_68K_GOT160      11
#define R_68K_GOT80       12
#define R_68K_PLT32       13
#define R_68K_PLT16       14
#define R_68K_PLT8        15
#define R_68K_PLT320      16
#define R_68K_PLT160      17
#define R_68K_PLT80       18
#define R_68K_COPY        19
#define R_68K_GLOB_DAT    20
#define R_68K_JMP_SLOT    21
#define R_68K_RELATIVE    22


bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf32_Word     pc = bfd_get_section_vma(abfd, input_section);
Elf32_Word     val, add;

	if ( r->rel.r_offset + sizeof(Elf32_Word) > bfd_get_section_size(input_section) )
		return bfd_reloc_outofrange;

	pc += r->rel.r_offset;

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;


	val = 0;
	switch ( ELF32_R_TYPE(r->rel.r_info) ) {
		default:
		return bfd_reloc_notsupported;

		case R_386_PC32: val = -(Elf32_Word)pc;

		case R_386_32:	 memcpy(&add, (char*)pc, sizeof(add));

						 val += add + bfd_asymbol_value(psym);
						 memcpy((char*)pc, &val, sizeof(val));
		break;
	}

	return bfd_reloc_ok;
}

bfd_reloc_status_type
pmbfd_perform_relocation_m68k(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf32_Word     pc = bfd_get_section_vma(abfd, input_section);

unsigned       sz;
int32_t        val;
int8_t         cval;
int16_t        sval;
uint32_t       pcadd=0;
int32_t        lim;

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;

	pc += r->rela.r_offset;

	/* I don't have my hands on the 68k ABI document so
	 * I don't know if there are any alignment requirements.
	 * Anyhow, if we loaded the section correctly and
	 * the compiler did the right thing this should
	 * not be an issue.
	 */

	switch ( ELF32_R_TYPE(r->rela.r_info) ) {

		default:
		return bfd_reloc_notsupported;

		case R_68K_PC8:  pcadd = pc; lim   = -0x0080; sz = 1; break;
		case R_68K_8:    pcadd =  0; lim   = -0x0100; sz = 1; break;

		case R_68K_PC16: pcadd = pc; lim   = -0x8000; sz = 2; break;
		case R_68K_16:   pcadd =  0; lim   = -0x1000; sz = 2; break;

		case R_68K_PC32: pcadd = pc; lim   = 0;       sz = 4; break;
		case R_68K_32:   pcadd =  0; lim   = 0;       sz = 4; break;
	}

	if ( r->rela.r_offset + sz > bfd_get_section_size(input_section) )
		return bfd_reloc_outofrange;

	switch (sz) {
		case 1:
			memcpy(&cval, (void*)pc, sizeof(cval));
			val = cval;
		break;

		case 2:
			memcpy(&sval, (void*)pc, sizeof(sval));
			val = sval;
		break;

		case 4:
			memcpy(&val, (void*)pc, sizeof(val));
		break;
	}

	val = val - r->rela.r_addend - pcadd;

	if ( lim && (val < lim || val > -lim - 1) )
		return bfd_reloc_overflow;

	switch (sz) {
		case 1:
			cval = val;
			memcpy((void*)pc, &cval, sizeof(cval));
		break;

		case 2:
			sval = val;
			memcpy((void*)pc, &sval, sizeof(sval));
		break;

		case 4:
			memcpy((void*)pc, &val, sizeof(val));
		break;
	}

	return bfd_reloc_ok;
}


pmbfd_arelent *
pmbfd_reloc_next(bfd *abfd, pmbfd_areltab *tab, pmbfd_arelent *prev)
{
	return (pmbfd_arelent *) (prev ? (uint8_t*)prev + tab->entsz : tab->data);
}

int
pmbfd_reloc_get_sym_idx(bfd *abfd, pmbfd_arelent *r)
{
Elf32_Word idx;

	idx = ELF32_R_SYM(r->rel.r_info);
	if ( idx == 0 || idx > abfd->nsyms )
		return -1;
	return idx-1;
}

bfd_size_type
pmbfd_reloc_get_address(bfd *abfd, pmbfd_arelent *r)
{
	return r->rel.r_offset;
}

#define namecase(rel)	case rel: return #rel;
const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *r)
{
	switch ( ELF32_R_TYPE(r->rel.r_info) ) {
		namecase( R_386_NONE     )
		namecase( R_386_32       )
		namecase( R_386_PC32     )
		namecase( R_386_GOT32    )
		namecase( R_386_PLT32    )
		namecase( R_386_COPY     )
		namecase( R_386_GLOB_DAT )
		namecase( R_386_JMP_SLOT )
		namecase( R_386_RELATIVE )
		namecase( R_386_GOTOFF   )
		namecase( R_386_GOTPC    )

		default:
		break;
	}
	return "UNKNOWN";
}

const char *
pmbfd_reloc_get_name_m68k(bfd *abfd, pmbfd_arelent *r)
{
	switch ( ELF32_R_TYPE(r->rel.r_info) ) {
		namecase( R_68K_NONE )
		namecase( R_68K_32 )
		namecase( R_68K_16 )
		namecase( R_68K_8 )
		namecase( R_68K_PC32 )
		namecase( R_68K_PC16 )
		namecase( R_68K_PC8 )
		namecase( R_68K_GOT32 )
		namecase( R_68K_GOT16 )
		namecase( R_68K_GOT8 )
		namecase( R_68K_GOT320 )
		namecase( R_68K_GOT160 )
		namecase( R_68K_GOT80 )
		namecase( R_68K_PLT32 )
		namecase( R_68K_PLT16 )
		namecase( R_68K_PLT8 )
		namecase( R_68K_PLT320 )
		namecase( R_68K_PLT160 )
		namecase( R_68K_PLT80 )
		namecase( R_68K_COPY )
		namecase( R_68K_GLOB_DAT )
		namecase( R_68K_JMP_SLOT )
		namecase( R_68K_RELATIVE )

		default:
		break;
	}
	return "UNKNOWN";
}
