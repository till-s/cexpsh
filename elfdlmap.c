
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <features.h>

#include <stdlib.h>

#include <elfdlmap.h>

#if defined(HAVE_LINK_H) && defined(HAVE_DL_ITERATE_PHDR)

#include <stdio.h>

#include <link.h>

/* 1: basic info
 * 2: dt_tag info
 * 3: individual hash bucket info
 */
#define DLMAP_DEBUG 0

typedef struct Elf_GnuHashHdr {
	Elf32_Word nbuckets;
	Elf32_Word symndx;
	Elf32_Word maskwords;
	Elf32_Word shift2;
} Elf_GnuHashHdr;

#define MSK_SYMTAB   (1<<0)
#define MSK_STRTAB   (1<<1)
#define MSK_HASH     (1<<2)
#define MSK_GNUHASH  (1<<3)

static int
cb(struct dl_phdr_info *info, size_t info_len, void *closure)
{
CexpLinkMap    **tailpp = closure;
CexpLinkMap    map;
ElfW(Dyn)      *dyn, *dyns;
ElfW(Addr)     off = info->dlpi_addr;
ElfW(Addr)     uoff;

ElfW(Sym)      *symtab   = 0; /* silence compiler warning */
const char     *strtab   = 0; /* silence compiler warning */
void           *hash     = 0; /* silence compiler warning */
void           *gnu_hash = 0; /* silence compiler warning */
Elf_GnuHashHdr hhdr;
unsigned long  ndsyms;
int            i;
unsigned       msk;

#if DLMAP_DEBUG > 0
	printf("Object %s @ %p\n", info->dlpi_name, (void*)info->dlpi_addr);
#endif

	/* Find dynamic section */
	dyn = 0;
	for ( i = 0; i < info->dlpi_phnum; i++ ) {
		if ( PT_DYNAMIC == info->dlpi_phdr[i].p_type ) {
			dyn = (ElfW(Dyn)*)(info->dlpi_phdr[i].p_vaddr + off);
#if DLMAP_DEBUG > 0
			printf("Dynamic section @ %p\n", (void*)dyn);
#endif
			break;
		}
	}

	if ( ! dyn ) {
		fprintf(stderr,"cexpLinkMapBuild() - No dynamic section found for '%s'\n", info->dlpi_name);
		return -1;
	}

	msk      = 0;

	dyns = 0;
	/* Now scan the dynamic section for things we need... */
	while (  DT_NULL != dyn->d_tag ) {
		switch ( dyn->d_tag ) {
			case DT_SYMTAB:
				symtab = (void*)(dyn->d_un.d_ptr);
				/* remember dynamic symtab entry */ 
#if DLMAP_DEBUG > 1
				printf("Symtab found @%p\n", (void*)symtab);
#endif
				dyns     = dyn;
				msk     |= MSK_SYMTAB;
				break;
			case DT_STRTAB:
				strtab   = (const char*)(dyn->d_un.d_ptr);
#if DLMAP_DEBUG > 1
				printf("Strtab found @%p\n", (void*)strtab);
#endif
				msk     |= MSK_STRTAB;
				break;
			case DT_GNU_HASH:
				gnu_hash = (void*)(dyn->d_un.d_ptr);
#if DLMAP_DEBUG > 1
				printf("GNU HASH found @%p\n", (void*)gnu_hash);
#endif
				msk     |= MSK_GNUHASH;
				break;
			case DT_HASH:
				hash     = (void*)(dyn->d_un.d_ptr);
#if DLMAP_DEBUG > 1
				printf("HASH found @%p\n", (void*)hash);
#endif
				msk     |= MSK_HASH;
				break;

			default:
				break;

		}
		dyn++;
	}

	/* Check if we have what we need */
	if ( ! (MSK_SYMTAB & msk) ) {
		fprintf(stderr,"cexpLinkMapBuild() - No dynamic symbols found for '%s'\n", info->dlpi_name);
		return -1;
	}

	if ( ! (MSK_STRTAB & msk) ) {
		fprintf(stderr,"cexpLinkMapBuild() - No dynamic stringtab found for '%s'\n", info->dlpi_name);
		return -1;
	}

	if ( ! ((MSK_HASH | MSK_GNUHASH) & msk) ) {
		/* unable to determine symbol count w/o section headers nor hash tables 
		 * and section header may not be mapped.
		 */
		fprintf(stderr,"cexpLinkMapBuild() - No hash table found for '%s'\n", info->dlpi_name);
		return -1;
	}

	/* Here comes some uglyness - it seems glibc's dynamic linker
	 * already fixed up pointers in the DYNAMIC section to point
	 * to real virtual addresses. EXCEPT in the case of a vdso
	 * object which it has NOT fixed up (probably can't write there
	 * anyways).
	 * uClibc-0.9.33 OTOH doesn't seem to ever fix up the DYNAMIC
	 * section.
	 */
#ifdef __UCLIBC__
	uoff = off;
#if DLMAP_DEBUG > 0
	printf("UCLIBC fixup 0x%lx\n", uoff);
#endif
#else
	uoff  = 0;


	/* Use ugly heuristics to guess if the linker has 'fixed up' the pointers
	 * in the dynamic section; glibc seems to do so, uclibc does not. Also,
	 * glibc does not fix up the 'vdso's dynamic section...
	 */
	for ( i = 0; i < info->dlpi_phnum; i++) {
		if ( PT_LOAD != info->dlpi_phdr[i].p_type )
			continue;
		if (   info->dlpi_phdr[i].p_vaddr <= dyns->d_un.d_ptr
		    && dyns->d_un.d_ptr + sizeof(ElfW(Sym)) < info->dlpi_phdr[i].p_vaddr + info->dlpi_phdr[i].p_memsz ) {
			/* It's pointing into one of the segments; could still have been fixed up,
			 * theoretically - should we do some more tests ? ...
			 */
#if DLMAP_DEBUG > 0
			printf("GLIBC fixup hack due to PHDR #%i (vaddr %p - %p)\n",
			       i,
			       (void*)info->dlpi_phdr[i].p_vaddr,
			       (void*)(info->dlpi_phdr[i].p_vaddr + info->dlpi_phdr[i].p_memsz)
			);
#endif
			uoff = off;
			break;
		}
	}
#if DLMAP_DEBUG > 0
	printf("GLIBC fixup 0x%lx\n", uoff);
#endif
#endif
	symtab = ((void*)symtab) + uoff;
	strtab = ((void*)strtab) + uoff;
	if ( (MSK_HASH & msk) ) {
		hash = ((void*)hash) + uoff;
	}
	if ( (MSK_GNUHASH & msk) ) {
		gnu_hash = ((void*)gnu_hash) + uoff;
	}

	/* Now we have to go through some pains to find the number of symbols.
	 * Unfortunately this info is not contained in the dynamic section :-(
	 * and other ELF info (such as the section headers) may not be mapped
	 * into the program's address space so we can't use section headers.
	 *
	 * It is, however, possible to extract the number of symbols from
	 * the hash table info...
	 * 
	 * Note that both, GNU_HASH and HASH may be present. Check for
	 * GNU_HASH first to make sure hhdr is valid when computing first_sym.
	 */
	if ( (MSK_GNUHASH & msk) ) {
		/* read the header */
		hhdr = *(Elf_GnuHashHdr*)gnu_hash;

#if DLMAP_DEBUG > 0
		printf("GNU Hash Hdr %u buckets\n", hhdr.nbuckets);
		printf("GNU Hash Hdr %u symndx\n",  hhdr.symndx);
#endif

		gnu_hash += sizeof(hhdr);

		/* skip filter */
		gnu_hash += hhdr.maskwords * sizeof(ElfW(Addr));

#if DLMAP_DEBUG > 2
		for ( i=0; i<hhdr.nbuckets; i++ ) {
			printf("GNU Bucket %u: %u\n", i, ((Elf32_Word*)gnu_hash)[i]);
		}
#endif

		ndsyms = 0;

		/* find the last chain and follow it to the end */
		for ( i = hhdr.nbuckets - 1; i >=0 && 0 == (ndsyms = ((Elf32_Word*)gnu_hash)[i]); i--)
			/* nothing else to do */;

		/* skip the bucket area */
		gnu_hash += hhdr.nbuckets * sizeof(Elf32_Word);

		if ( ndsyms >= hhdr.symndx ) {
			i = ndsyms - hhdr.symndx;

			while ( 0 == (((Elf32_Word*)gnu_hash)[i] & 1 ) )
				i++;
			/* found last index; number of symbols is one more */
			i++;
		} else {
			i=0;
		}
		ndsyms = i+hhdr.symndx;

#if DLMAP_DEBUG > 0
		printf("%lu dynamic symbols from GNU_HASH\n", ndsyms);
#endif
	} else if ( (MSK_HASH & msk) ) {
		/* This is easy; the number of chains equals the number of symbols */
		ndsyms = ((Elf32_Word*)hash)[1];
#if DLMAP_DEBUG > 0
		printf("%lu dynamic symbols from HASH\n", ndsyms);
#endif
	} else {
		fprintf(stderr, "cexpLinkMapBuild() - internal error; should not get here (%s)\n", info->dlpi_name);
		return -1;
	}

	map           = calloc(1, sizeof(*map));
	map->flags    = CEXP_LINK_MAP_STATIC_STRINGS;
	map->elfsyms  = symtab;
	map->strtab   = strtab;
	map->name     = info->dlpi_name;
	map->nsyms    = ndsyms;
	map->offset   = off;
	/* if there is a gnu hashtable then         */
    /* all undefined or local symbols are first */
	map->firstsym = (MSK_GNUHASH & msk) ? hhdr.symndx : 0;
#if DLMAP_DEBUG > 0
	printf("Firstsym: %lu\n", map->firstsym);
#endif

	/* enqueue at tail */
	**tailpp = map;
	*tailpp  = &map->next;
	return 0;
}

#endif

CexpLinkMap
cexpLinkMapBuild(const char *name, void *parm)
{
CexpLinkMap rval = 0;
#if defined(HAVE_LINK_H) && defined(HAVE_DL_ITERATE_PHDR)
CexpLinkMap *tailp = &rval;
	if ( dl_iterate_phdr( cb, &tailp ) ) {
		/* something went wrong */
		cexpLinkMapFree(rval);
		rval = 0;
	}
#endif
	return rval;
}

void
cexpLinkMapFree(CexpLinkMap m)
{
CexpLinkMap mn;

	while ( m ) {
		mn = m->next;
		free( m );
		m = mn;
	}
}

