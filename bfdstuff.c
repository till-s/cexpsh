#include <bfd.h>
#include <libiberty.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cexp.h"
#include "cexpmodP.h"
#include "cexpsymsP.h"

#ifdef USE_MDBG
#include <mdbg.h>
#endif

#ifdef USE_ELF_STUFF
#include "elf-bfd.h"
#endif

#define CACHE_LINE_SIZE 32

/* an output segment description */
typedef struct SegmentRec_ {
	PTR				chunk;		/* pointer to memory */
	unsigned long	vmacalc;	/* working counter */
	unsigned long	size;
	unsigned		attributes; /* such as 'read-only' etc.; currently unused */
} SegmentRec, *Segment;

#define NUM_SEGS 1
#define ONLY_SEG (assert(NUM_SEGS==1),0)

typedef struct LinkDataRec_ {
	SegmentRec		segs[NUM_SEGS];
	asymbol			**st;
	CexpSymTbl		cst;
	int				errors;
	BitmapWord		*depend;
} LinkDataRec, *LinkData;

static CexpSymTbl
buildCexpSymtbl(asymbol **asyms, int num_new_commons);

/* how to decide where a particular section should go */
static Segment
segOf(LinkData ld, asection *sect)
{
	/* multiple sections not supported (yet) */
	return &ld->segs[ONLY_SEG];
}

/* determine the alignment power of a common symbol
 * (currently only works for ELF)
 */
#ifdef USE_ELF_STUFF
static __inline__ int
get_align_pwr(bfd *abfd, asymbol *sp)
{
register unsigned long rval=0,tst;
elf_symbol_type *esp;
	if (esp=elf_symbol_from(abfd, sp))
		for (tst=1; tst<esp->internal_elf_sym.st_size; rval++)
			tst<<=1;
	return rval;
}

/* we sort in descending order; hence the routine must return b-a */
static int
align_size_compare(const void *a, const void *b)
{
elf_symbol_type *espa, *espb;
asymbol			*spa=*(asymbol**)a;
asymbol			*spb=*(asymbol**)b;

	return
		((espa=elf_symbol_from(bfd_asymbol_bfd(spa),spa)) &&
	     (espb=elf_symbol_from(bfd_asymbol_bfd(spb),spb)))
		? (espb->internal_elf_sym.st_size - espa->internal_elf_sym.st_size)
		: 0;
}
#else
#define get_align_pwr(abfd,sp)	0
#define align_size_compare		0
#endif

static const char *
filter(void *ext_sym, void *closure)
{
asymbol *asym=*(asymbol**)ext_sym;

		if ( !(BSF_KEEP & asym->flags) ||
			 !((BSF_FUNCTION|BSF_OBJECT) & asym->flags) )
			return 0;
		return bfd_asymbol_name(asym);
}

static void
assign(void *ext_sym, CexpSym cesp, void *closure)
{
asymbol		*asym=*(asymbol**)ext_sym;
int			isnewcommon = ext_sym < closure;
int			s;
CexpType	t=TVoid;
#ifdef USE_ELF_STUFF
elf_symbol_type *elfsp=elf_symbol_from(0 /*lucky hack: unused by macro */,asym);
#define ELFSIZE(elfsp)	((elfsp)->internal_elf_sym.st_size)
#else
#define		elfsp 0
#define	ELFSIZE(elfsp)	(0)
#endif

printf("TSILL, adding %s\n",bfd_asymbol_name(asym));


		s = elfsp ? ELFSIZE(elfsp) : 0;

		if (BSF_FUNCTION & asym->flags) {
			t=TFuncP;
			if (0==s)
				s=sizeof(void(*)());
		} else {
			/* must be BFS_OBJECT */
			if (isnewcommon) {
				/* value holds the size */
				s = bfd_asymbol_value(asym);
			}
			if (CEXP_BASE_TYPE_SIZE(TUCharP) == s) {
				t=TUChar;
			} else if (CEXP_BASE_TYPE_SIZE(TUShortP) == s) {
				t=TUShort;
			} else if (CEXP_BASE_TYPE_SIZE(TULongP) == s) {
				t=TULong;
			} else if (CEXP_BASE_TYPE_SIZE(TDoubleP) == s) {
				t=TDouble;
			} else if (CEXP_BASE_TYPE_SIZE(TFloatP) == s) {
				/* if sizeof(float) == sizeof(long), long has preference */
				t=TFloat;
			} else {
				/* if it's bigger than double, leave it (void*) */
			}
		}

		cesp->size = s;
		cesp->value.type = t;
		/* store a pointer to the external symbol, so we have
		 * a handle even after the internal symbols are sorted
		 */
		cesp->value.ptv  = (CexpVal)asym;

		if (asym->flags & BSF_GLOBAL)
			cesp->flags |= CEXP_SYMFLG_GLBL;
		if (asym->flags & BSF_WEAK)
			cesp->flags |= CEXP_SYMFLG_WEAK;

		/* can only set the value after relocation */
}

/* call this after relocation to assign the internal
 * symbol representation their values
 */
void
cexpSymTabSetValues(CexpSymTbl cst)
{
CexpSym	cesp;
	for (cesp=cst->syms; cesp->name; cesp++) {
		asymbol *sp=(asymbol*)cesp->value.ptv;
		cesp->value.ptv=(CexpVal)bfd_asymbol_value(sp);
	}
}

static void
s_count(bfd *abfd, asection *sect, PTR arg)
{
Segment		seg=segOf((LinkData)arg, sect);
	printf("Section %s, flags 0x%08x\n",
			bfd_get_section_name(abfd,sect), sect->flags);
	printf("size: %i, alignment %i\n",
			bfd_section_size(abfd,sect),
			(1<<bfd_section_alignment(abfd,sect)));
	if (SEC_ALLOC & sect->flags) {
		seg->size+=bfd_section_size(abfd,sect);
		seg->size+=(1<<bfd_get_section_alignment(abfd,sect));
	}
}

static void
s_setvma(bfd *abfd, asection *sect, PTR arg)
{
Segment		seg=segOf((LinkData)arg, sect);

	if (SEC_ALLOC & sect->flags) {
		seg->vmacalc=align_power(seg->vmacalc, bfd_get_section_alignment(abfd,sect));
		printf("%s allocated at 0x%08x\n",
				bfd_get_section_name(abfd,sect),
				seg->vmacalc);
		bfd_set_section_vma(abfd,sect,seg->vmacalc);
		seg->vmacalc+=bfd_section_size(abfd,sect);
		sect->output_section = sect;
	}
}


static void
s_reloc(bfd *abfd, asection *sect, PTR arg)
{
LinkData	ld=(LinkData)arg;
int			i;
long		err;
char		buf[1000];

	if ( ! (SEC_ALLOC & sect->flags) )
		return;

	/* read section contents to its memory segment
	 * NOTE: this automatically clears section with
	 *       ALLOC set but with no CONTENTS (such as
	 *       bss)
	 */
	bfd_get_section_contents(
		abfd,
		sect,
		(PTR)bfd_get_section_vma(abfd,sect),
		0,
		bfd_section_size(abfd,sect)
	);

	/* if there are relocations, resolve them */
	if ((SEC_RELOC & sect->flags)) {
		arelent **cr=0,r;
		long	sz;
		sz=bfd_get_reloc_upper_bound(abfd,sect);
		if (sz<=0) {
			fprintf(stderr,"No relocs for section %s???\n",
					bfd_get_section_name(abfd,sect));
			return;
		}
		/* slurp the relocation records; build a list */
		cr=(arelent**)xmalloc(sz);
		sz=bfd_canonicalize_reloc(abfd,sect,cr,ld->st);
		if (sz<=0) {
			fprintf(stderr,"ERROR: unable to canonicalize relocs\n");
			free(cr);
			return;
		}
		for (i=0; i<sect->reloc_count; i++) {
			arelent *r=cr[i];
			printf("relocating (%s=",
					bfd_asymbol_name(*(r->sym_ptr_ptr))
					);
			if (bfd_is_und_section(bfd_get_section(*r->sym_ptr_ptr))) {
				CexpModule mod;
				CexpSym ts=cexpSymLookup(bfd_asymbol_name(*(r->sym_ptr_ptr)),&mod);
				asymbol *sp;
				if (ts) {
				/* Resolved reference; replace the symbol pointer
				 * in this slot with a new asymbol holding the
				 * resolved value.
				 */
				sp=*r->sym_ptr_ptr=bfd_make_empty_symbol(abfd);
				/* copy name pointer */
				bfd_asymbol_name(sp) = bfd_asymbol_name(ts);
				sp->value=(symvalue)ts->value.ptv;
				sp->section=bfd_abs_section_ptr;
				sp->flags=BSF_GLOBAL;
				/* mark the referenced module in the bitmap */
				assert(mod->id < MAX_NUM_MODULES);
				BITMAP_SET(ld->depend,mod->id);
				} else {
					fprintf(stderr,"Unresolved symbol: %s\n",bfd_asymbol_name(sp));
					ld->errors++;
		continue;
				}
			}
			printf("0x%08x)->0x%08x\n",
			bfd_asymbol_value(*(r->sym_ptr_ptr)),
			r->address);

			if ((err=bfd_perform_relocation(
				abfd,
				r,
				(PTR)bfd_get_section_vma(abfd,sect),
				sect,
				0 /* output bfd */,
				0))) {
				bfd_perror("Relocation failed");
				ld->errors++;
			}
		}
		free(cr);
	}
}

/* resolve undefined and common symbol references;
 * The routine also rearranges the symbol table for
 * all common symbols that must be created to be located
 * at the beginning of the symbol list.
 *
 * RETURNS:
 *   value >= 0 : OK, number of new common symbols
 *   value <0   : -number of errors (unresolved refs or multiple definitions)
 *
 * SIDE EFFECTS:
 *   - all new common symbols moved to the beginning of the table
 *   - resolved undef or common slots are replaced by new symbols
 *     pointing to the ABS section
 *   - KEEP flag is set for all globals defined by this object.
 *   - raise a bit for each module referenced by this new one.
 */
static int
resolve_syms(bfd *abfd, asymbol **syms, BitmapWord *depend)
{
asymbol		*sp;
int			i,num_new_commons=0,errs=0;
CexpModule	mod;

	/* resolve undefined and common symbols;
	 * find name clashes
	 */
	for (i=0; sp=syms[i]; i++) {
		asection *sect=bfd_get_section(sp);
		CexpSym	ts;

		/* we only care about global symbols
		 * (NOTE: undefined symbols are neither local
		 *        nor global)
		 */
		if ( (BSF_LOCAL & sp->flags) ) {
			continue;
		}

		ts=cexpSymLookup(bfd_asymbol_name(sp), &mod);

		if (bfd_is_und_section(sect)) {
			/* do some magic on symbols which _do_ have a
			 * type. These are probably sitting in a shared
			 * library but _do_ have a valid value.
			 */
			if (!ts && bfd_asymbol_value(sp) && (BSF_FUNCTION & sp->flags)) {
					sp->flags |= BSF_KEEP;
			}
			/* resolve references at relocation time */
#if 0
			if (ts) {
				/* Resolved reference; replace the symbol pointer
				 * in this slot with a new asymbol holding the
				 * resolved value.
				 */
				sp=syms[i]=bfd_make_empty_symbol(abfd);
				/* copy name pointer */
				bfd_asymbol_name(sp) = bfd_asymbol_name(ts);
				sp->value=(symvalue)ts->value.ptv;
				sp->section=bfd_abs_section_ptr;
				sp->flags=BSF_GLOBAL;
				/* mark the referenced module in the bitmap */
				assert(mod->id < MAX_NUM_MODULES);
				BITMAP_SET(depend,mod->id);
			} else {
				fprintf(stderr,"Unresolved symbol: %s\n",bfd_asymbol_name(sp));
				errs++;
			}
#endif
		}
		else if (bfd_is_com_section(sect)) {
			if (ts) {
				/* use existing value of common sym */
				sp = bfd_make_empty_symbol(abfd);

				/* TODO: check size and alignment */

				/* copy pointer to old name */
				bfd_asymbol_name(sp) = bfd_asymbol_name(syms[i]);
				sp->value=(symvalue)ts->value.ptv;
				sp->section=bfd_abs_section_ptr;
				sp->flags=BSF_GLOBAL;
				/* mark the referenced module in the bitmap */
				assert(mod->id < MAX_NUM_MODULES);
				BITMAP_SET(depend,mod->id);
			} else {
				/* it's the first definition of this common symbol */
				asymbol *swap;

				/* we'll have to add it to our internal symbol table */
				sp->flags |= BSF_KEEP;

				/* this is a new common symbol; we move all of these
				 * to the beginning of the 'st' list
				 */
				swap=syms[num_new_commons];
				syms[num_new_commons++]=sp;
				sp=swap;
			}
			syms[i]=sp; /* use new instance */
		} else {
			if (ts) {
				fprintf(stderr,"Symbol '%s' already exists\n",bfd_asymbol_name(sp));

				errs++;
				/* TODO: check size and alignment; allow multiple
				 *       definitions??? - If yes, we have to track
				 *		 the dependencies also.
				 */
			} else {
				/* new symbol defined by the loaded object; account for it */
				/* mark for second pass */
				sp->flags |= BSF_KEEP;
			}
		}
	}

	return errs ? -errs : num_new_commons;
}

/* make a dummy section holding the new common data introduced by
 * the newly loaded file.
 *
 * RETURNS: 0 on failure, nonzero on success
 */
static int
make_new_commons(bfd *abfd, asymbol **syms, int num_new_commons)
{
unsigned long	i,val;
asection	*csect;

	if (num_new_commons) {
		/* make a dummy section for new common symbols */
		csect=bfd_make_section(abfd,bfd_get_unique_section_name(abfd,".dummy",0));
		if (!csect) {
			bfd_perror("Creating dummy section");
			return -1;
		}
		csect->flags |= SEC_ALLOC;

		/* our common section alignment is the maximal alignment
		 * found during the sorting process which is the alignment
		 * of the first element...
		 */
		bfd_section_alignment(abfd,csect) = get_align_pwr(abfd,syms[0]);

		/* set new common symbol values */
		for (val=0,i=0; i<num_new_commons; i++) {
			asymbol *sp;
			int tsill;

			sp = bfd_make_empty_symbol(abfd);

			val=align_power(val,(tsill=get_align_pwr(abfd,syms[i])));
printf("TSILL align_pwr %i\n",tsill);
			/* copy pointer to old name */
			bfd_asymbol_name(sp) = bfd_asymbol_name(syms[i]);
			sp->value=val;
			sp->section=csect;
			sp->flags=syms[i]->flags;
			val+=syms[i]->value;
			syms[i] = sp;
		}
		
		bfd_set_section_size(abfd, csect, val);
	}
	return 0;
}

static int
slurp_symtab(bfd *abfd, LinkData ld)
{
asymbol 		**asyms=0;
long			i,nsyms;
long			num_new_commons;

	if (!(HAS_SYMS & bfd_get_file_flags(abfd))) {
		fprintf(stderr,"No symbols found\n");
		return -1;
	}
	if ((i=bfd_get_symtab_upper_bound(abfd))<0) {
		fprintf(stderr,"Fatal error: illegal symtab size\n");
		return -1;
	}
	if (i) {
		asyms=(asymbol**)xmalloc(i);
	}
	nsyms= i/sizeof(asymbol*) - 1;
	if (bfd_canonicalize_symtab(abfd,asyms) <= 0) {
		bfd_perror("Canonicalizing symtab");
		goto cleanup;
	}

	/* do a sanity check */


	if ((num_new_commons=resolve_syms(abfd,asyms,ld->depend))<0) {
		goto cleanup;
	}
		
	/*
	 *	sort st[0]..st[num_new_commons-1] by alignment
	 */
	if (align_size_compare && num_new_commons)
		qsort((void*)asyms, num_new_commons, sizeof(*asyms), align_size_compare);

	/* Now, everything is in place to build our internal symbol table
	 * representation.
	 * We cannot do this later, because the size information will be lost.
	 * However, we cannot provide the values yet; this can only be done
	 * after relocation has been performed.
	 */

	ld->cst=cexpCreateSymTbl(asyms, sizeof(*asyms), nsyms, filter, assign, asyms+num_new_commons);

	if (0!=make_new_commons(abfd,asyms,num_new_commons))
		goto cleanup;

	ld->st=asyms;
	asyms=0;

cleanup:
	if (asyms) {
		free(asyms);
		return -1;
	}

	return 0;
}

static int
flushCache(LinkData ld)
{
#if defined(__PPC__) || defined(__PPC) || defined(_ARCH_PPC) || defined(PPC)
int i,j;
	for (i=0; i<NUM_SEGS; i++) {
		for (j=0; j<= ld->segs[i].size; j+=CACHE_LINE_SIZE)
			__asm__ __volatile__(
				"dcbf %0, %1\n"	/* flush out one data cache line */
				"icbi %0, %1\n" /* invalidate cached instructions for this line */
				::"b"(ld->segs[i].chunk),"r"(j));
	}
	/* enforce flush completion and discard preloaded instructions */
	__asm__ __volatile__("sync; isync");
#endif
}


/* the caller of this routine holds the module lock */
int
cexpLoadFile(char *filename, CexpModule mod)
{
bfd 			*abfd=0;
LinkDataRec		ldr;
int				rval=1,i,j;
CexpSym			sane;

	memset(&ldr,0,sizeof(ldr));

	ldr.depend = mod->needs;

	/* basic check for our bitmaps */
	assert( (1<<LD_WORDLEN) <= sizeof(BitmapWord)*8 );

	if (!filename) {
		fprintf(stderr,"Need filename arg\n");
		goto cleanup;
	}

	bfd_init();
#ifdef USE_MDBG
	mdbgInit();
#endif

	if ( ! (abfd=bfd_openr(filename,0)) ) {
		bfd_perror("Opening object file");
		goto cleanup;
	}
	if (!bfd_check_format(abfd, bfd_object)) {
		bfd_perror("Checking format");
		goto cleanup;
	}

	if (slurp_symtab(abfd,&ldr)) {
		fprintf(stderr,"Error creating symbol table\n");
		goto cleanup;
	}

	/* the first thing to be loaded must be the system
	 * symbol table. Reject to load anything...
	 */
	if ( ! (0==cexpSystemModule) ) {

	ldr.errors=0;
	bfd_map_over_sections(abfd, s_count, &ldr);

	/* allocate segment space */
	for (i=0; i<NUM_SEGS; i++)
		ldr.segs[i].vmacalc=(unsigned long)ldr.segs[i].chunk=xmalloc(ldr.segs[i].size);

	ldr.errors=0;
	bfd_map_over_sections(abfd, s_setvma, &ldr);

	ldr.errors=0;
memset(ldr.segs[ONLY_SEG].chunk,0xee,ldr.segs[ONLY_SEG].size); /*TSILL*/
	bfd_map_over_sections(abfd, s_reloc, &ldr);
	if (ldr.errors)
		goto cleanup;

	}

	cexpSymTabSetValues(ldr.cst);

	/* system symbol table sanity check */
	if ((sane=cexpSymTblLookup("cexpLoadFile",ldr.cst))) {
		/* this must be the system table */
		extern void *_edata, *_etext;
		
        /* it must be the main symbol table */
        if ( sane->value.ptv==(CexpVal)cexpLoadFile     &&
       	     (sane=cexpSymTblLookup("_etext",ldr.cst))  &&
			 sane->value.ptv==(CexpVal)&_etext          &&
             (sane=cexpSymTblLookup("_edata",ldr.cst))  &&
			 sane->value.ptv==(CexpVal)&_edata ) {

        	/* OK, sanity test passed */

		} else {
			fprintf(stderr,"SANITY CHECK FAILED! Stale symbol file?\n");
			goto cleanup;
		}
	}


	flushCache(&ldr);

	/* TODO: call constructors */


	/* move the relevant data over to the module */
	mod->memSeg  = ldr.segs[ONLY_SEG].chunk;
	mod->memSize = ldr.segs[ONLY_SEG].size;
	ldr.segs[ONLY_SEG].chunk=0;

	mod->symtbl  = ldr.cst;
	ldr.cst=0;

	rval=0;

cleanup:
	if (ldr.st) free(ldr.st);

	if (ldr.cst)
		cexpFreeSymTbl(&ldr.cst);
	if (abfd) bfd_close_all_done(abfd);
	for (i=0; i<NUM_SEGS; i++)
		if (ldr.segs[i].chunk) free(ldr.segs[i].chunk);

#ifdef USE_MDBG
	printf("Memory leaks found: %i\n",mdbgPrint(0,0));
#endif
	return rval;
}
