#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelf.h"

#define PARANOIA_ON 0

struct _Elf_Stream {
	FILE *f;
	int  needswap;
};

#define TXX_PRE    "pmelf"
#define TXX_PRINTF(f,a...) do { if (f) fprintf(f,a); } while (0)

#define ElfNumberOf(a) (sizeof(a)/sizeof((a)[0]))

static FILE *txx_err = 0;

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

Elf_Stream
txx_newstrm(char *name, FILE *f)
{
FILE *nf = 0;
Elf_Stream s;

	if ( ! f ) {
		if ( !(f = nf = fopen(name,"r")) ) {
			return 0;	
		}
	}
	if ( ! (s = calloc(1, sizeof(*s))) ) {
		if ( nf )
			fclose(nf);
		return 0;
	}
	s->f        = f;
	return s;
}

int
txx_seek(Elf_Stream s, Elf32_Off where)
{
	return fseek(s->f, where, SEEK_SET);
}

void
txx_delstrm(Elf_Stream s)
{
	fclose(s->f);
	free(s);
}

int
txx_getehdr(Elf_Stream s, Elf32_Ehdr *pehdr)
{
uint8_t magic[4] = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };

	if ( txx_seek(s, 0) ) {
		return -1;
	}
	if ( 1 != fread(pehdr, sizeof(*pehdr), 1, s->f) ) {
		return -1;
	}
	if ( memcmp(magic, pehdr->e_ident+EI_MAG0, sizeof(magic)) ) {
		TXX_PRINTF(txx_err, TXX_PRE" error: not an ELF file\n");
		return -1;
	}
	if ( pehdr->e_ident[EI_CLASS] != ELFCLASS32 ) {
		TXX_PRINTF(txx_err, TXX_PRE" error: not an 32-bit ELF file\n");
		return -1;
	}

	if ( pehdr->e_ident[EI_VERSION] != EV_CURRENT ) {
		TXX_PRINTF(txx_err, TXX_PRE" error: not a version %i ELF file\n", EV_CURRENT);
		return -1;
	}

	if ( (s->needswap = (iamlsb() ^ (pehdr->e_ident[EI_DATA] == ELFDATA2LSB ? 1 : 0))) ) {
		e32_swap16( &pehdr->e_type      );
		e32_swap16( &pehdr->e_machine   );
		e32_swap32( &pehdr->e_version   );
		e32_swap32( &pehdr->e_entry     );
		e32_swap32( &pehdr->e_phoff     );
		e32_swap32( &pehdr->e_shoff     );
		e32_swap32( &pehdr->e_flags     );
		e32_swap16( &pehdr->e_ehsize    );
		e32_swap16( &pehdr->e_phentsize );
		e32_swap16( &pehdr->e_phnum     );
		e32_swap16( &pehdr->e_shentsize );
		e32_swap16( &pehdr->e_shnum     );
		e32_swap16( &pehdr->e_shstrndx  );
	}

	if ( pehdr->e_shentsize != sizeof( Elf32_Shdr ) ) {
		TXX_PRINTF(txx_err, TXX_PRE" error: SH size mismatch %i"PRIu16"\n", pehdr->e_shentsize);
		return -1;
	}

	if ( pehdr->e_shnum > 0 ) {
		if ( pehdr->e_shstrndx == 0 || pehdr->e_shstrndx >= pehdr->e_shnum ) {
			TXX_PRINTF(txx_err, TXX_PRE" error: shstrndx out of bounds\n");
			return -1;
		}
	}
	
	return 0;
}

int
txx_getshdr(Elf_Stream s, Elf32_Shdr *pshdr)
{
unsigned long x;
	if ( 1 != fread(pshdr, sizeof(*pshdr), 1, s->f) ) {
		return -1;
	}
	if ( s->needswap ) {
		e32_swap32( &pshdr->sh_name);
		e32_swap32( &pshdr->sh_type);
		e32_swap32( &pshdr->sh_flags);
		e32_swap32( &pshdr->sh_addr);
		e32_swap32( &pshdr->sh_offset);
		e32_swap32( &pshdr->sh_size);
		e32_swap32( &pshdr->sh_link);
		e32_swap32( &pshdr->sh_info);
		e32_swap32( &pshdr->sh_addralign);
		e32_swap32( &pshdr->sh_entsize);
	}
#ifdef PARANOIA_ON
	if ( (x = pshdr->sh_flags) & SHF_MSKSUP ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getshdr - paranoia: unsupported flags 0x%08lx\n", x);
		return -1;
	}
#if PARANOIA_ON > 0
	if ( (x = pshdr->sh_type) > SHT_MAXSUP ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getshdr - paranoia: unsupported type  0x%08lu\n", x);
		return -1;
	}
#endif
#endif
	return 0;
}

int
txx_getsym(Elf_Stream s, Elf32_Sym *psym)
{
int x;
	if ( 1 != fread(psym, sizeof(*psym), 1, s->f) ) {
		return -1;
	}
	if ( s->needswap ) {
		e32_swap32( &psym->st_name);
		e32_swap32( &psym->st_value);
		e32_swap32( &psym->st_size);
		e32_swap16( &psym->st_shndx);
	}
#ifdef PARANOIA_ON
	if ( (x = ELF32_ST_TYPE( psym->st_info )) > STT_MAXSUP ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getsym - paranoia: unsupported type %i\n", x);
		return -1;
	}
	if ( (x = ELF32_ST_BIND( psym->st_info )) > STB_MAXSUP ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getsym - paranoia: unsupported binding %i\n", x);
		return -1;
	}
#endif
	return 0;
}

void *
txx_getscn(Elf_Stream s, Elf32_Shdr *psect, void *data)
{
void *buf = 0;

	if ( txx_seek(s, psect->sh_offset) ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getscn unable to seek %s", strerror(errno));
		return 0;
	}

	if ( !data ) {
		buf = malloc(psect->sh_size);
		if ( !buf ) {
			TXX_PRINTF( txx_err, TXX_PRE"txx_getscn - no memory");
			return 0;
		}
		data = buf;
	}

	if ( psect->sh_size != fread( data, 1, psect->sh_size, s->f ) ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getscn unable to read %s", strerror(errno));
		free(buf);
		return 0;
	}

	return data;
}

Elf32_Word *
txx_getgrp(Elf_Stream s, Elf32_Shdr *psect, Elf32_Word *data)
{
Elf32_Word *buf;
int        ne,j;

	if ( SHT_GROUP != psect->sh_type )
		return 0;

	if ( psect->sh_size & (sizeof(*buf)-1) ) {
		TXX_PRINTF(txx_err, "group section size not a multiple of %u\n", sizeof(*buf));
		return 0;
	}

	ne  = psect->sh_size/sizeof(*buf);

	if ( ne < 1 ) {
		TXX_PRINTF(txx_err, "group has no members and not even a flag word\n");
		return 0;
	}

	if ( ! (buf = txx_getscn(s, psect, data)) )
		return 0;

	if ( s->needswap ) {
		for ( j=0; j < ne; j++ ) {
			e32_swap32( &buf[j] );
		}
	}

	return buf;
}

void
txx_delshtab(Txx_Elf32_Shtab sht)
{
	if ( sht ) {
		free(sht->shdrs);
		free((void*)sht->strtab);
		free(sht);
	}
}

Txx_Elf32_Shtab
txx_getshtab(Elf_Stream s, Elf32_Ehdr *pehdr)
{
Txx_Elf32_Shtab rval = 0;
uint32_t      i;
Elf32_Shdr    *strsh;

	if ( 0 == pehdr->e_shnum )
		return 0;

	if ( txx_seek(s, pehdr->e_shoff) )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	if ( !(rval->shdrs = calloc(pehdr->e_shnum, sizeof(*rval->shdrs))) )
		goto bail;

	rval->nshdrs = pehdr->e_shnum;

	/* slurp the section headers */
	for ( i=0; i<pehdr->e_shnum; i++ ) {
		if ( txx_getshdr(s, rval->shdrs + i) )
			goto bail;
	}

	/* slurp the string table    */
	strsh = &rval->shdrs[pehdr->e_shstrndx];
	if ( ! (rval->strtab = txx_getscn( s, strsh, 0 )) ) {
		TXX_PRINTF( txx_err, TXX_PRE" (shstrtab)\n");
		goto bail;
	}

	return rval;

bail:
	txx_delshtab(rval);
	return 0;
}

void
txx_delsymtab(Txx_Elf32_Symtab symtab)
{
	if ( symtab ) {
		free(symtab->syms);
		free((void*)symtab->strtab);
		free(symtab);
	}
}

long
txx_find_symhdrs(Elf_Stream s, Txx_Elf32_Shtab shtab, Elf32_Shdr **psymsh, Elf32_Shdr **pstrsh)
{
Elf32_Sym    *sym;
Elf32_Shdr   *shdr;
uint32_t     i;
Elf32_Shdr   *symsh  = 0;
Elf32_Shdr   *strsh  = 0;

	for ( i = 0, shdr = shtab->shdrs; i<shtab->nshdrs; i++, shdr++ ) {
		if ( SHT_SYMTAB == shdr->sh_type && !strcmp(".symtab", &shtab->strtab[shdr->sh_name]) ) {
			if ( symsh ) {
				TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: multiple symtabs\n");
				return -1;
			}
			symsh = shdr;
#define USE_SYM_SH_LINK
#ifdef USE_SYM_SH_LINK
			strsh = shtab->shdrs + symsh->sh_link;
#endif
		}
#ifndef USE_SYM_SH_LINK
		else if ( SHT_STRTAB == shdr->sh_type && !strcmp(".strtab", &shtab->strtab[shdr->sh_name]) ) {
			if ( strsh ) {
				TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: multiple strtabs\n");
				return -1;
			}
			strsh = shdr;
		}
#endif
	}

	if ( !symsh ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: no symtab found\n");
		return -1;
	}

	if ( !strsh ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: no strtab found\n");
		return -1;
	}

	if ( 0 == symsh->sh_size ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: zero size symtab\n");
		return -1;
	}

	if ( symsh->sh_entsize && symsh->sh_entsize != sizeof( *sym ) ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: symbol size mismatch (sh_entsize %"PRIu32"\n", symsh->sh_entsize);
		return -1;
	}

	if ( (symsh->sh_size % sizeof(*sym)) != 0 ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs: sh_size of symtab not a multiple of sizeof(Elf32_Sym)\n");
		return -1;
	}

	if ( txx_seek(s, symsh->sh_offset) ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_find_symhdrs unable to seek to symtab %s\n", strerror(errno));
		return -1;
	}

	if ( psymsh )
		*psymsh = symsh;
	if ( pstrsh )
		*pstrsh = strsh;

	return symsh->sh_size/sizeof(*sym);
}

Txx_Elf32_Symtab
txx_getsymtab(Elf_Stream s, Txx_Elf32_Shtab shtab)
{
Txx_Elf32_Symtab rval = 0;
uint32_t      i;
long          nsyms;
Elf32_Shdr   *symsh  = 0;
Elf32_Shdr   *strsh  = 0;

	if ( (nsyms = txx_find_symhdrs(s, shtab, &symsh, &strsh)) < 0 )
		return 0;

	if ( !(rval = calloc(1, sizeof(*rval))) )
		return 0;

	rval->nsyms = nsyms;

	rval->idx   = symsh - shtab->shdrs;

	if ( 0 == rval->nsyms ) {
		TXX_PRINTF( txx_err, TXX_PRE"txx_getsymtab: symtab with 0 elements\n");
		goto bail;
	}

	if ( !(rval->syms = calloc(rval->nsyms, sizeof(*rval->syms))) )
		goto bail;

	/* slurp the symbols */
	for ( i=0; i<rval->nsyms; i++ ) {
		if ( txx_getsym(s, rval->syms + i) ) {
			TXX_PRINTF( txx_err, TXX_PRE"txx_getsymtab unable to read symbol %"PRIu32": %s\n", i, strerror(errno));
			goto bail;
		}
	}

	if ( ! ( rval->strtab = txx_getscn( s, strsh,  0) ) ) {
		TXX_PRINTF( txx_err, TXX_PRE" (strtab)\n" );
		goto bail;
	}

	return rval;

bail:
	txx_delsymtab(rval);
	return 0;
}

static const char *txx_ehdr_type_s[] = {
"none",
"relocatable",
"executable",
"shared object",
"core",
"processor specific (lo)",
"processor specific (hi)"
};

static const char *txx_ehdr_machine_s(Elf32_Half idx)
{
	switch ( idx ) {
		case EM_386: return "Intel x86";
		case EM_68K: return "Motorola 68k";
		case EM_PPC: return "PowerPC";
		default: break;
	}
	return "unknown";
}

static const char *txx_ehdr_abi_s[]={
	"SYSV",
};

#define ARRSTR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : "unknown" )
#define ARRCHR(arr, idx) ( (idx) <= ElfNumberOf(arr) ? arr[idx] : '?' )

void
txx_dump_ehdr(FILE *f, Elf32_Ehdr *pehdr)
{
	fprintf( f, "File    type : %s (%u)\n", ARRSTR(txx_ehdr_type_s,    pehdr->e_type), pehdr->e_type );
	fprintf( f, "Machine type : %s\n", txx_ehdr_machine_s(pehdr->e_machine) );
	fprintf( f, "ABI          : %s (%u)\n", ARRSTR(txx_ehdr_abi_s, pehdr->e_ident[EI_OSABI]), pehdr->e_ident[EI_OSABI]);
	fprintf( f, "Version      : %"PRIu32"\n",     pehdr->e_version );

	fprintf( f, "Entry point  : 0x%08"PRIx32"\n", pehdr->e_entry );

	fprintf( f, "PH offset    : %"    PRIu32"\n", pehdr->e_phoff  );
	fprintf( f, "SH offset    : %"    PRIu32"\n", pehdr->e_shoff  );
	fprintf( f, "Flags        : 0x%08"PRIx32"\n", pehdr->e_flags  );
	fprintf( f, "EH size      : %"    PRIu16"\n", pehdr->e_ehsize );
	fprintf( f, "PH entry size: %"    PRIu16"\n", pehdr->e_phentsize );
	fprintf( f, "# of PH      : %"    PRIu16"\n", pehdr->e_phnum );
	fprintf( f, "SH entry size: %"    PRIu16"\n", pehdr->e_shentsize );
	fprintf( f, "# of SH      : %"    PRIu16"\n", pehdr->e_shnum );
	fprintf( f, "SHSTR index  : %"    PRIu16"\n", pehdr->e_shstrndx );
}

static const char *txx_shdr_type_s[] = {
	"NULL",
	"PROGBITS",
	"SYMTAB",
	"STRTAB",
	"RELA",
	"HASH",
	"DYNAMIC",
	"NOTE",
	"NOBITS",
	"REL",
	"SHLIB",
	"DYNSYM",
	"RESERVED",
	"RESERVED",
	"INIT_ARRAY",
	"FINI_ARRAY",
	"PREINIT_ARRAY",
	"GROUP",
	"SYMTAB_SHNDX"
};

static const char txx_shdr_flags_c[] = {
	'W', 'A', 'X', '?', 'M', 'S', 'I', 'L', 'O', 'G'
};

static int
prflags(char *buf, Elf32_Word flags)
{
int i,m,l;
	for ( l=i=0, m=1; i<sizeof(flags)*8 && flags; i++, m<<=1 ) {
		if ( flags & m ) {
			flags &= ~m;
			buf[l++] = ARRCHR(txx_shdr_flags_c, i);
		}
	}
	buf[l]=0;
	return l;
}

#define FMT_SHORT  0
#define FMT_LONG   1
#define FMT_COMPAT 2

void 
txx_dump_shdr(FILE *f, Elf32_Shdr *pshdr, int format)
{
char fbuf[4*sizeof(pshdr->sh_flags)+1];
const char *fmt;

	if ( FMT_COMPAT != format ) {
		fprintf( f, "(%1x) ", pshdr->sh_type );
		fmt = "%-12s";
	} else {
		fmt = "%-16s";
	}

	fprintf( f, fmt,
		ARRSTR(txx_shdr_type_s, pshdr->sh_type), pshdr->sh_type
	);

	fprintf( f, "%08"PRIx32, pshdr->sh_addr);
	if ( FMT_LONG == format ) {
		fprintf( f, " %08"PRIx32, pshdr->sh_offset);
		fprintf( f, " %08"PRIx32, pshdr->sh_size);
		fprintf( f, " %08"PRIx32, pshdr->sh_link);
		fprintf( f, " %05"PRIx32, pshdr->sh_addralign);
		fprintf( f, " %05"PRIx32, pshdr->sh_entsize);
		fprintf( f, "\n flags: 0x%08"PRIx32, pshdr->sh_flags);
		if ( pshdr->sh_flags ) {
			fprintf(f, "[");
			if ( SHF_WRITE & pshdr->sh_flags ) fprintf(f, " WRITE");
			if ( SHF_ALLOC & pshdr->sh_flags ) fprintf(f, " ALLOC");
			if ( SHF_EXECINSTR & pshdr->sh_flags ) fprintf(f, " EXECINSTR");
			if ( SHF_MERGE & pshdr->sh_flags ) fprintf(f, " MERGE");
			if ( SHF_STRINGS & pshdr->sh_flags ) fprintf(f, " STRINGS");
			if ( SHF_INFO_LINK & pshdr->sh_flags ) fprintf(f, " INFO_LINK");
			if ( SHF_LINK_ORDER & pshdr->sh_flags ) fprintf(f, " LINK_ORDER");
			if ( SHF_OS_NONCONFORMING & pshdr->sh_flags ) fprintf(f, " OS_NONCONFORMING");
			if ( SHF_GROUP & pshdr->sh_flags ) fprintf(f, " GROUP");
			fprintf(f, " ]");
		}
	} else {
		fprintf( f, " %06"PRIx32, pshdr->sh_offset);
		fprintf( f, " %06"PRIx32, pshdr->sh_size);
		fprintf( f, " %02"PRIx32, pshdr->sh_entsize);
		prflags(fbuf, pshdr->sh_flags);
		fprintf( f, "%4s",fbuf);
		fprintf( f, " %2" PRIu32, pshdr->sh_link);
		fprintf( f, " %3" PRIu32, pshdr->sh_info);
		fprintf( f, " %2" PRIu32, pshdr->sh_addralign);
	}
	fprintf(f,"\n");
}

void
txx_dump_shtab(FILE *f, Txx_Elf32_Shtab shtab, int format)
{
uint32_t i;
Elf32_Shdr *shdr;

	fprintf(f,"Section Headers:\n");
	fprintf(f,"  [Nr] %-18s%-16s%-9s%-7s%-7s%2s%4s%3s%4s%3s\n",
		"Name", "Type", "Addr", "Off", "Size", "ES", "Flg", "Lk", "Inf", "Al");

	for ( i = 0, shdr = shtab->shdrs; i<shtab->nshdrs; i++, shdr++ ) {
		fprintf( f, "  [%2u] %-18s", i, &shtab->strtab[shdr->sh_name] );
		txx_dump_shdr( f, shdr, format );
	}
	fprintf(f,"Key to Flags:\n");
	fprintf(f,"  W (write), A (alloc), X (execute), M (merge), S (strings)\n");
	fprintf(f,"  I (info), L (link order), G (group), ? (unknown)\n");
	fprintf(f,"  O (extra OS processing required)\n");
}

static const char *txx_st_bind_s[] = {
	"LOCAL",
	"GLOBAL",
	"WEAK"
};

static const char *txx_st_type_s[] = {
	"NOTYPE",
	"OBJECT",
	"FUNC",
	"SECTION",
	"FILE"
};

static const char *txx_st_vis_s[] = {
	"DEFAULT",
	"INTERNAL",
	"HIDDEN",
	"PROTECTED"
};
	

void
txx_dump_symtab(FILE *f, Txx_Elf32_Symtab symtab, Txx_Elf32_Shtab shtab)
{
uint32_t i;
Elf32_Sym *sym;
int      donestr;

	fprintf(f,"%6s: %8s%6s %-8s%-7s%-9s",
		"Num", "Value", "Size", "Type", "Bind", "Vis");
	if ( shtab )
		fprintf(f,"%-13s", "Section");
	else
		fprintf(f,"%3s", "Ndx");
	fprintf(f," Name\n");
	for ( i=0, sym = symtab->syms; i<symtab->nsyms; i++, sym++ ) {

		fprintf(f,"%6u: %08"PRIx32"%6"PRIu32" %-8s%-7s%-9s",
			i,
			sym->st_value,
			sym->st_size,
			ARRSTR(txx_st_type_s, ELF32_ST_TYPE(sym->st_info)),
			ARRSTR(txx_st_bind_s, ELF32_ST_BIND(sym->st_info)),
			ARRSTR(txx_st_vis_s,  ELF32_ST_VISIBILITY(sym->st_other))
		);

		donestr = 0;

		switch ( sym->st_shndx ) {
			case SHN_UNDEF: fprintf(f, "UND"); break;
			case SHN_ABS:	fprintf(f, "ABS"); break;
			case SHN_COMMON:fprintf(f, "COM"); break;
			default:
				if ( sym->st_shndx < SHN_LORESERVE && shtab ) {
					fprintf(f,"%-13s", &shtab->strtab[shtab->shdrs[sym->st_shndx].sh_name]);
					donestr = 1;
				} else {
					fprintf(f, "%3"PRIu16, sym->st_shndx);
				}
			break;
		}

		if ( shtab && !donestr )
			fprintf(f,"         ");

		fprintf(f, " %s\n", &symtab->strtab[sym->st_name]);
	}
}

int
txx_dump_groups(FILE *f, Elf_Stream s, Txx_Elf32_Shtab shtab, Txx_Elf32_Symtab symtab)
{
int i,j;
int ng,ne;
Elf32_Shdr *shdr;
int sz=0;
Elf32_Word *buf = 0;
Elf32_Word *nbuf;

	for ( ng = i = 0, shdr = shtab->shdrs; i < shtab->nshdrs; i++, shdr++ ) {

		if ( SHT_GROUP == shdr->sh_type ) {

			if ( shdr->sh_size > sz ) {
				free(buf); buf = 0;
				sz = shdr->sh_size;
			}

			if ( ! (nbuf = txx_getgrp(s, shdr, buf)) ) {
				break;
			}
			buf = nbuf;

			ne  = shdr->sh_size/sizeof(*buf);

			/* first entry holds flags */
			fprintf(f,"\n%-7sgroup section [%5"PRIu32"] `%s' ",
				buf[0] & GRP_COMDAT ? "COMDAT":"",
				i,
				&shtab->strtab[shdr->sh_name]
			);
			if ( symtab ) {
				if ( symtab->idx == shdr->sh_link )
					fprintf(f, "[%s]", &symtab->strtab[symtab->syms[shdr->sh_info].st_name]);
				else
					fprintf(f, "<unable to print group id symbol: symtab index mismatch>");
			} else {
				fprintf(f,"<unable to print group id symbol: no symbol table given");
			}
			fprintf(f," contains %u sections:\n", ne - 1);

			fprintf(f,"   [Index]    Name\n");

			for ( j=1; j < ne; j++ ) {
				fprintf(f,"   [%5u]   %s\n", buf[j], &shtab->strtab[shtab->shdrs[buf[j]].sh_name]);
			}

			ng++;
		}
	}

	free(buf);
	return ng;
}

void
txx_set_errstrm(FILE *f)
{
	txx_err = f;
}

