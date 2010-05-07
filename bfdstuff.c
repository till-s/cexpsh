/* bfdstuff.c,v 1.51 2004/12/02 21:16:37 till Exp */

/* Cexp interface to object/symbol file using BFD; runtime loader support */

/* NOTE: read the comment near the end of this file about C++ exception handling */

/* IMPORTANT LICENSING INFORMATION:
 *
 *  linking this code against 'libbfd'/ 'libopcodes'
 *  may be subject to the GPL conditions.
 *  (This file itself is distributed under the more
 *  liberal 'SLAC' license)
 *
 */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *    Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 * 	  Stanford Linear Accelerator Center, Stanford University.
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
 * SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 

/*#include <libiberty.h>*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _INSIDE_CEXP_
#include "cexp.h"
#include "cexpmodP.h"
#include "cexpsymsP.h"
#include "cexpsegsP.h"
#include "cexpHelp.h"

/* Oh well; rtems/score/ppctypes.h defines boolean and bfd
 * defines boolean as well :-( Can't you people use names
 * less prone to clashes???
 * We redefine bfd's boolean here
 */
#define  boolean bfdddd_bbboolean
#ifdef USE_PMBFD
#include "pmbfd.h"
#include "pmelf.h"
#define reloc_get_address(abfd, r) pmbfd_reloc_get_address(abfd, r)
#define reloc_get_name(abfd, r)    pmbfd_reloc_get_name(abfd, r)
#define bfd_asymbol_set_value(s,v) pmbfd_asymbol_set_value(s,v)
#undef HAVE_ELF_BFD_H
#else
#include <bfd.h>
#include <libiberty.h>

#ifdef HAVE_ELF_BFD_H
#include "elf-bfd.h"
#endif
#undef   boolean

#define reloc_get_address(abfd, r)       ((r)->address)
#define reloc_get_name(abfd,r)           ((r)->howto->name)
#define bfd_asymbol_set_value(s,v)       ((s)->value = (v))
#define bfd_set_output_section(s, os)    ((s)->output_section = (os))

#endif

#define NumberOf(arr) (sizeof(arr)/sizeof(arr[0]))

#define	DEBUG_COMMON	(1<<0)
#define DEBUG_SECT		(1<<1)
#define DEBUG_RELOC		(1<<2)
#define DEBUG_SYM		(1<<3)
#define DEBUG_CDPRI		(1<<4)

#define DEBUG			(0)

#include "cexp_regex.h"

/* magic symbol names for C++ support; probably gcc specific */
#define CTOR_DTOR_PATTERN		"^_+GLOBAL_[_.$][ID][_.$]"
/* static/global CTOR/DTOR has no init_priority (highest priority is 1) */
#define INIT_PRIO_NONE			1000000
#ifdef OBSOLETE_EH_STUFF	/* old suselinux-ppc modified gcc needed that */
#define EH_FRAME_BEGIN_PATTERN	"__FRAME_BEGIN__"
#define EH_FRAME_END_PATTERN	"__FRAME_END__"
#endif
#define EH_SECTION_NAME			".eh_frame"
#define TEXT_SECTION_NAME		".text"
#define DSO_HANDLE_NAME			"__dso_handle"

/* this probably only makes sense on ELF */
#if defined(HAVE_ELF_BFD_H) || defined(_PMBFD_)
/* filter dangerous sections; we do ont support shared objects */
#define FIXUP_SECTION_NAME		".fixup"
#define GOT_SECTION_NAME		".got"
#define GOT2_SECTION_NAME		".got2"
#endif

#define ISELF(abfd)	(bfd_get_flavour((abfd)) == bfd_target_elf_flavour)

/* using one static variable for the pattern matcher is
 * OK since the entire cexpLoadFile() routine is called
 * with the WRITE_LOCK held, so mutex is guaranteed for
 * the ctorCtorRegexp.
 */
static  cexp_regex	*ctorDtorRegexp=0;

/*
 * Allow to override the object-attribute matcher
 */
int _cexpForceIgnoreObjAttrMismatches = 0;

#ifdef HAVE_BFD_DISASSEMBLER
/* as a side effect, this code gives access to a disassembler which
 * itself is implemented by libopcodes
 */
extern void	cexpDisassemblerInstall(bfd *abfd);
#else
#define		cexpDisassemblerInstall(a) do {} while(0)
#endif

/* this is PowerPC specific; note that some architectures
 * (mpc860) have smaller cache lines. Setting this to a smaller
 * value than the actual cache line size is safe and performance
 * is not an issue here
 */
#if defined(__PPC__) || defined(__PPC) || defined(_ARCH_PPC) || defined(PPC)
#    define CACHE_LINE_SIZE 16
#    define FLUSHINVAL_LINE(addr) \
		__asm__ __volatile__( \
			"dcbf 0, %0\n"	/* flush out one data cache line */ \
			"icbi 0, %0\n"	/* invalidate cached instructions for this line */ \
		::"r"(addr))
/* enforce flush completion and discard preloaded instructions */
#    define FLUSHFINISH() __asm__ __volatile__("sync; isync")
#elif defined(__mc68000__) || defined(__mc68000) || defined(mc68000) || defined(__m68k__)
#    define CACHE_LINE_SIZE 16
#  if defined(__rtems__)
extern void _CPU_cache_flush_1_data_line(void *addr);
extern void _CPU_cache_invalidate_1_instruction_line(void *addr);
#    define FLUSHINVAL_LINE(addr) \
		do { \
			_CPU_cache_flush_1_data_line(addr); \
			_CPU_cache_invalidate_1_instruction_line(addr); \
		} while (0)
#    define FLUSHFINISH() do {} while (0)
#  else
/* m68k cache flush instructions are only available in supervisor mode;
 * PLUS they operate on physical addresses :-(
 */
# error("don't know how to flush/invalidate cache on this system")
#  endif /* defined __rtems__ */
#endif

/* data we need for linking */
typedef struct LinkDataRec_ {
	bfd				*abfd;	
	CexpSegment     segs;
	int             nsegs;
	asymbol			**st;
	asymbol			***new_commons;
	long            num_new_commons;
	char			*dummy_section_name;
	CexpSymTbl		cst;
	int				errors;
	int				num_alloc_sections;
	int				num_section_names;	/* differs because of linkonce sections */
	BitmapWord		*depend;
	int				nCtors;
	int				nDtors;
#ifdef OBSOLETE_EH_STUFF
	asymbol			*eh_frame_b_sym;
	asymbol			*eh_frame_e_sym;
#else
	asection		*eh_section;
#endif
	unsigned long	text_vma;
	asection		*text;
	asection		*eh;
	void			*iniCallback;
	void			*finiCallback;
	CexpModule		module;
} LinkDataRec, *LinkData;

/* forward declarations */

/* Hmm - c++ exception handling is _highly_ compiler and version and 
 * target dependent :-(.
 *
 * It is very unlikely that exception handling works with anything other
 * than gcc.
 *
 * Gcc (2.95.3) seems to implement (at least) two flavours of exception
 * handling: longjump and exception frame tables. The latter requires
 * the dynamic loader to load and register the exception frame table, the
 * former is transparent.
 * Unfortunately, the __register_frame()/__deregister_frame() routines
 * are absent on targets which do not support this kind of exception handling.
 * 
 * Therefore, we use the CEXP symbol table itself to retrieve the actual
 * routines __register_frame() and __deregister_frame(). Our private pointers
 * are initialized when we read the system symbol table.
 */
static void (*my__register_frame)(void*)=0;
static void (*my__deregister_frame)(void*)=0;

/* Support for __cxa_atexit() */
static void (*my__cxa_finalize)(/*dso_handle*/ void*)=0;
static CexpSym	my__dso_handle = 0;

static asymbol *
asymFromCexpSym(bfd *abfd, CexpSym csym, BitmapWord *depend, CexpModule mod);

static void
bfdCleanupCallback(CexpModule);

static void
bfdstuff_complete_init(CexpSymTbl);

/* how to decide where a particular section should go */
static CexpSegment
segOf(LinkData ld, asection *sect)
{
	return cexpSegsMatch(ld->segs, ld->abfd, sect);
}

/* determine the alignment power of a common symbol
 * (currently only works for ELF)
 */
#if defined(HAVE_ELF_BFD_H)
static inline unsigned
elf_get_align(bfd *abfd, asymbol *sp)
{
elf_symbol_type *esp;
unsigned         rval;
	if ( (esp = elf_symbol_from(abfd, sp)) && (rval = esp->internal_elf_sym.st_value) ) {
		return rval;
	}
	return  1;
}

static inline int
elf_get_size(bfd *abfd, asymbol *asym)
{
elf_symbol_type *elfsp= ISELF(abfd) ? elf_symbol_from(abfd, asym) : 0;
	return elfsp ? elfsp->internal_elf_sym.st_size : 0 ;
}

#elif defined(_PMBFD_)
/* exported by pmbfd.h */
#else
#define elf_get_align(abfd,sp)		1
#define elf_get_size(abfd,sp)       0
#endif

#if defined(HAVE_ELF_BFD_H) || defined(_PMBFD_)
/* we sort in descending order; hence the routine must return b-a */
static int
align_size_compare(const void *a, const void *b)
{
asymbol			*spa=**(asymbol***)a;
asymbol			*spb=**(asymbol***)b;

unsigned algn_a, algn_b;

	algn_a = elf_get_align(bfd_asymbol_bfd(spa), spa);
	algn_b = elf_get_align(bfd_asymbol_bfd(spb), spb);

	return algn_b - algn_a;
}
#endif

static inline int
elf_get_align_pwr(bfd *abfd, asymbol *sp)
{
register unsigned long rval=0,tst;
	for (tst=1; tst < elf_get_align(abfd, sp); rval++)
			tst<<=1;
	return rval;
}


/* determine if a given symbol is a constructor or destructor 
 * by matching to a magic pattern.
 *
 * RETURNS: 1 if the symbol is a constructor
 *         -1 if it is a destructor
 *          0 otherwise
 */
static inline int
isCtorDtor(asymbol *asym, int quiet, int *pprio)
{
	/* From bfd/linker.c: */
	/* "A constructor or destructor name starts like this:
	   _+GLOBAL_[_.$][ID][_.$] where the first [_.$] and
	   the second are the same character (we accept any
	   character there, in case a new object file format
	   comes along with even worse naming restrictions)."  */

	if (cexp_regexec(ctorDtorRegexp,bfd_asymbol_name(asym))) {
		register const char *tail = ctorDtorRegexp->endp[0];
		/* read the priority */
		if (pprio) {
			if (isdigit(*tail))
				*pprio = atoi(tail);
			else
				*pprio = INIT_PRIO_NONE;
		}
		switch (*(tail-2)) {
			case 'I':	return  1; /* constructor */
			case 'D':	return -1; /* destructor  */
			default:
				if (!quiet) {
					fprintf(stderr,"WARNING: unclassified BSF_CONSTRUCTOR symbol;");
					fprintf(stderr," ignoring '%s'\n",bfd_asymbol_name(asym));
				}
			break;
		}
	}
	return 0;
}

/* this routine defines which symbols are retained by Cexp in its
 * internal symbol table
 */
static const char *
filter(void *ext_sym, void *closure)
{
LinkData	ld   =closure;
asymbol		*asym=*(asymbol**)ext_sym;

		ld = ld;
		/*
		 * silence warning about unused var (bfd_get_section_name macro
		 * does currently not use the first argument).
		 */

		if ( !(BSF_KEEP & asym->flags) )
			return 0;
		return BSF_SECTION_SYM & asym->flags ? 
					bfd_get_section_name(ld->abfd,bfd_get_section(asym)) :
					bfd_asymbol_name(asym);
}

/* this routine is responsible for converting external (BFD) symbols
 * to their internal representation (CexpSym). Only symbols who have
 * passed the filter() above will be seen by assign().
 */
static void
assign(void *ext_sym, CexpSym cesp, void *closure)
{
LinkData	ld=(LinkData)closure;
asymbol		*asym=*(asymbol**)ext_sym;
int			s;
CexpType	t=TVoid;

#if DEBUG & DEBUG_SYM
		printf("assigning symbol: %s\n",bfd_asymbol_name(asym));
#endif

		s = elf_get_size(ld->abfd, asym);

		if (BSF_FUNCTION & asym->flags) {
			t=TFuncP;
			if (0==s)
				s=sizeof(void(*)());
		} else if (BSF_SECTION_SYM & asym->flags) {
			/* section name: leave it void* */
		} else {
			/* must be BSF_OBJECT */
			if (asym->flags & BSF_OLD_COMMON) {
				/* value holds the size */
				s = bfd_asymbol_value(asym);
			}
			t = cexpTypeGuessFromSize(s);
		}
		/* last attempt: if there is no size info and no flag set,
		 * at least look at the section; functions are in the text
		 * section...
		 */
		if (TVoid == t &&
			! (asym->flags & (BSF_FUNCTION | BSF_OBJECT | BSF_SECTION_SYM)) &&
			0 == s) {
			if (ld->text && ld->text == bfd_get_section(asym))
				t=TFuncP;
		} 

		cesp->size = s;
		cesp->value.type = t;
		/* We can only set the real value after relocation.
		 * Therefore, store a pointer to the external symbol,
		 * so we have a handle even after the internal symbols
		 * are sorted. Note that the aindex[] built by cexpCreateSymTbl()
		 * will be incorrect and has to be re-sorted further 
		 * down the line.
		 */
		cesp->value.ptv  = ext_sym;

		if (asym->flags & BSF_GLOBAL)
			cesp->flags |= CEXP_SYMFLG_GLBL;
		if (asym->flags & BSF_WEAK)
			cesp->flags |= CEXP_SYMFLG_WEAK;
		if (asym->flags & BSF_SECTION_SYM)
			cesp->flags |= CEXP_SYMFLG_SECT;
}

/* call this after relocation to assign the internal
 * symbol representation their values
 */
void
cexpSymTabSetValues(CexpSymTbl cst)
{
CexpSym	cesp;
	for (cesp=cst->syms; cesp->name; cesp++) {
		asymbol *sp=*(asymbol**)cesp->value.ptv;
		cesp->value.ptv=(CexpVal)bfd_asymbol_value(sp);
	}

	/* resort the address index */
	qsort((void*)cst->aindex,
			cst->nentries,
			sizeof(*cst->aindex),
			_cexp_addrcomp);
}

/* All 's_xxx()' routines defined here are for use with
 * bfd_map_over_sections()
 */

/* compute the size requirements for a section */
static void
s_count(bfd *abfd, asection *sect, PTR arg)
{
LinkData	ld    = (LinkData)arg;
CexpSegment	seg   = segOf(ld, sect);
flagword	flags = bfd_get_section_flags(abfd,sect);
const char	*secn = bfd_get_section_name(abfd,sect);
#if DEBUG & DEBUG_SECT
	printf("Section %s, flags 0x%08x\n", secn, flags);
	printf("size: %lu, alignment %i\n",
			(unsigned long)bfd_section_size(abfd,sect),
			(1<<bfd_get_section_alignment(abfd,sect)));
#endif
	if (SEC_ALLOC & flags) {
		seg->size+=(1<<bfd_get_section_alignment(abfd,sect))-1;
		seg->size+=bfd_section_size(abfd,sect);
		/* exception handler frame tables have to be 0 terminated and
		 * we must reserve a little extra space.
		 */
		if ( ! (SEC_LINK_ONCE & flags) ) {
			/* speed up the name comparison by skipping the numerous
			 * linkonce sections - we assume that none of the ones
			 * we check for below here are LINKONCE
			 */
			if ( !strcmp(secn, EH_SECTION_NAME) ) {
#ifdef OBSOLETE_EH_STUFF
				asymbol *asym;
				/* create a symbol to tag the terminating 0 */
				asym=ld->eh_frame_e_sym=bfd_make_empty_symbol(abfd);
				bfd_asymbol_name(asym) = EH_FRAME_END_PATTERN;
				asym->value=(symvalue)bfd_section_size(abfd,sect);
				asym->section=sect;
#else
				ld->eh_section = sect;
#endif
				/* allocate space for a terminating 0 */
				seg->size+=sizeof(long);
			}
#if defined(HAVE_ELF_BFD_H) || defined(_PMBFD_)
			/* filter dangerous sections; we do not support shared objects
			 * do this in s_count and not earlier because we allow a got
			 * in the main system to support linux/solaris & friends...
			 */
			else if (bfd_section_size(abfd,sect) > 0) {
				if ( !strcmp(FIXUP_SECTION_NAME, secn) ||
				     !strcmp(GOT_SECTION_NAME, secn)   ||
				     !strcmp(GOT2_SECTION_NAME, secn) ) {
					if ( !ld->errors++ ) {
						fprintf(stderr,
								"Uh oh - a `%s` section should not be present"
								" - did you compile with -fPIC against my advice?\n",
								secn);
					}
				}
			}
#endif
		}
	}
}

/* VMA of -1 means 'invalid' */
static int
check_get_section_vma(bfd *abfd, asection *sect, unsigned long *p_vma)
{
	return (unsigned long)-1 == (*p_vma = bfd_get_section_vma(abfd, sect)) ? 1 : 0;
}

/* compute the section's vma for loading */

static void
s_setvma(bfd *abfd, asection *sect, PTR arg)
{
CexpSegment	seg=segOf((LinkData)arg, sect);
LinkData	ld=(LinkData)arg;

	if (SEC_ALLOC & bfd_get_section_flags(abfd, sect)) {

		if ( 0 == bfd_section_size(abfd, sect) )
				return;

		if ( (unsigned long)-1 == seg->vmacalc ) {
			/* we get here if we try to use a segment that
			 * is not initialized or has size zero.
			 */
			fprintf(stderr,"Internal Error: trying to use segment of size 0\n");
			ld->errors++;
			return;
		}
		seg->vmacalc=align_power(seg->vmacalc, bfd_get_section_alignment(abfd,sect));
#if DEBUG & DEBUG_SECT
		printf("%s allocated at 0x%08lx\n",
				bfd_get_section_name(abfd,sect),
				(unsigned long)seg->vmacalc);
#endif
		bfd_set_section_vma(abfd,sect,seg->vmacalc);
		seg->vmacalc+=bfd_section_size(abfd,sect);
		if
#ifdef OBSOLETE_EH_STUFF
		   ( ld->eh_frame_e_sym && bfd_get_section(ld->eh_frame_e_sym) == sect )
#else
		   ( ld->eh_section && ld->eh_section == sect )
#endif
		{
			/* allocate space for a terminating 0 */
			seg->vmacalc+=sizeof(long);
		}
#if defined bfd_set_output_section
		/* Only needed with real BFD */
		bfd_set_output_section(sect, sect);
#endif
		if (ld->text && sect == ld->text) {
			if ( check_get_section_vma(abfd,sect, &ld->text_vma) )
				ld->text_vma = 0;
		}
	}
}

/* just count the number of section names we have to remember
 * (debugger support)
 */
static void
s_nsects(bfd *abfd, asection *sect, PTR arg)
{
LinkData	ld=(LinkData)arg;
	if ( SEC_ALLOC & bfd_get_section_flags(ld->abfd, sect) ) {
		if ( 0 == bfd_section_size(ld->abfd, sect) ) {
			/* Effectively remove zero-sized sections */
			bfd_set_section_flags( ld->abfd, sect, bfd_get_section_flags( ld->abfd, sect ) & ~SEC_ALLOC);
		} else {
			ld->num_section_names++;
		}
	}
}

/* find basic sections and the number of sections which are
 * actually allocated and resolve gnu.linkonce.xxx sections
 */
static void
s_basic(bfd *abfd, asection *sect, PTR arg)
{
LinkData	ld=(LinkData)arg;
flagword	flags=bfd_get_section_flags(ld->abfd, sect);
	if (!strcmp(TEXT_SECTION_NAME,bfd_get_section_name(abfd,sect))) {
		ld->text = sect;
	}
	if ( SEC_ALLOC & flags ) {
		const char *secn=bfd_get_section_name(ld->abfd,sect);

		/* temporarily store a pointer to the name */
		ld->module->section_syms[ld->num_section_names++] = (CexpSym)secn;
	}
}

static void
s_eliminate_linkonce(bfd *abfd, asection *sect, PTR arg)
{
LinkData	ld=(LinkData)arg;
flagword	flags=bfd_get_section_flags(ld->abfd, sect);

	if ( SEC_LINK_ONCE & flags ) {
		const char *secn=bfd_get_section_name(ld->abfd,sect);

		if (cexpSymLookup(secn, 0)) {
			/* a linkonce section with this name had been loaded already;
			 * discard this one...
			 */
			if ( (SEC_LINK_DUPLICATES & flags) )
				fprintf(stderr,"Warning: section '%s' exists; discarding...\n", secn);

			/* discard this section (older gcc creating .gnu.linkonce.xxx sections) */
#if DEBUG & DEBUG_SECT
			if ( SEC_ALLOC & flags ) {
				printf("Removing linkonce section '%s'\n", secn);
			}
#endif
			bfd_set_section_flags( abfd, sect, bfd_get_section_flags( abfd, sect ) & ~SEC_ALLOC);

			if ( (SEC_GROUP & flags) ) {
				/* more recent gcc using ELF section groups:
				 * remove the entire group; this relies on the fact
				 * that group sections must occur first, i.e., before
				 * the group contents
				 */
#if 0			/* bfd_discard_group() unfortunately not implemented; generic version
                   does nothing */
				bfd_discard_group(abfd, sect);
				
#else
#if defined(HAVE_ELF_BFD_H) || defined(_PMBFD_)
				if ( ISELF(ld->abfd) ) {
					asection *s,*frst;

					/* group seems to be a circular list pointed to
					 * by the group section
					 */
					frst = s = elf_next_in_group(sect);
					while ( s ) {
#if DEBUG & DEBUG_SECT
						printf("Removing linkonce group member '%s'\n",bfd_get_section_name(ld->abfd, s));
#endif
						bfd_set_section_flags( ld->abfd, s, bfd_get_section_flags( ld->abfd, s ) & ~SEC_ALLOC );
						if ( (s = elf_next_in_group(s)) == frst )
							break;
					}
				}
#endif
#endif
			}

		}
	}
	if ( SEC_ALLOC & bfd_get_section_flags(abfd, sect) )
		ld->num_alloc_sections++;
}

/* Extracted from bfd.h; unfortunately conversion of relocation error
 * codes to message strings is not available in libbfd  (unless I missed it)
 */
static char *reloc_err_msg[]={
	"ok (no error)",
	"The relocation was performed, but there was an overflow",
	"The address to relocate was not within the section supplied",
	"'continue' (Used by special functions)",
	"Unsupported relocation size/type requested",
  	"bfd_reloc_other",
	"The symbol to relocate against was undefined",
	"The relocation was performed, but may not be ok/dangerous"
};


/* read the section contents and process the relocations.
 */
static void
s_reloc(bfd *abfd, asection *sect, PTR arg)
{
LinkData	  ld=(LinkData)arg;
int			  i;
long		  err;
unsigned long vma;

	if ( ! (SEC_ALLOC & bfd_get_section_flags(abfd, sect) ) )
		return;

	if ( check_get_section_vma(abfd, sect, &vma) ) {
		fprintf(stderr,"Internal Error: trying to load relocations into non-existing memory segment\n");
		ld->errors++;
		return;
	}

	/* read section contents to its memory segment
	 * NOTE: this automatically clears section with
	 *       ALLOC set but with no CONTENTS (such as
	 *       bss)
	 */
	if (!bfd_get_section_contents(
				abfd,
				sect,
				(PTR)vma,
				0,
				bfd_section_size(abfd,sect))) {
		bfd_perror("reading section contents");
		ld->errors++;
		return;
	}

	/* if there are relocations, resolve them */
	if ( (SEC_RELOC & bfd_get_section_flags(abfd, sect)) ) {
		asection *symsect;
#ifndef _PMBFD_
		arelent **cr=0;
		arelent *r;
#else
		pmbfd_areltab *cr=0;
		pmbfd_arelent *r;
#endif
		long	sz;
		sz=bfd_get_reloc_upper_bound(abfd,sect);
		if (sz<=0) {
			fprintf(stderr,"No relocs for section %s???\n",
					bfd_get_section_name(abfd,sect));
			ld->errors++;
			return;
		}
		/* slurp the relocation records; build a list */
		cr=xmalloc(sz);
		/*
		 * API of bfd_canonicalize_reloc() differs from
		 * pmbfd_canonicalize_reloc(). We want to emphasize
		 * that, hence we don't simply redefine bfd_canonicalize_reloc()
		 */
#ifndef _PMBFD_
		sz=bfd_canonicalize_reloc(abfd,sect,cr,ld->st);
#else
		sz=pmbfd_canonicalize_reloc(abfd,sect,cr,ld->st);
#endif
		if (sz<=0) {
			fprintf(stderr,"ERROR: unable to canonicalize relocs\n");
			free(cr);
			ld->errors++;
			return;
		}
		for (i=0, r=0; i<sz; i++) {
			asymbol **ppsym;
#ifndef _PMBFD_
			r=cr[i];
			ppsym = r->sym_ptr_ptr;
#else
			r        = pmbfd_reloc_next(abfd, cr, r);

			{
			long idx = pmbfd_reloc_get_sym_idx(abfd, r);

				if ( idx < 0 ) {
					union {
						unsigned long l;
						char          c[sizeof(unsigned long)];
					} *p = (void*)r;
					fprintf(stderr,"Bad symbol index in reloc detected (section %s)\n",
							bfd_get_section_name(abfd,sect));
					fprintf(stderr,"[%u] 0x%08lx: 0x%08lx 0x%08lx\n",
						i, p->l, (p+1)->l, (p+2)->l);
					ld->errors++;
					continue;
				}
				ppsym = ld->st + idx;
			}
#endif
			symsect=bfd_get_section(*ppsym);

			if ( bfd_is_und_section(symsect) ) {
				/* reloc references an undefined symbol which we have to look-up */
				CexpModule	mod;
				asymbol		*sp;
				CexpSym		ts;

				ts=cexpSymLookup(bfd_asymbol_name((sp=*ppsym)),&mod);
				if (ts) {
					if (my__dso_handle == ts) {
						/* they are looking for __dso_handle; give them
						 * our module handle.
						 *
						 * We assume the system module has its own
						 * __dso_handle symbol and hence we can simply
						 * compare the symbol handles.
						 *
						 * The pathological case of a module asking for
						 * __dso_handle and a g++ version which supports
						 * __cxa_atexit/__cxa_finalize() but does NOT
						 * define its own __dso_handle will fail.
						 * (assert() when the symbols are looked up)
						 *
						 * We will get an undefined symbol reference
						 * if the module asks for __dso_handle but libgcc has
						 * no __cxa_finalize() and no __dso_handle.
						 *
						 * We will get this error if the module asks for
						 * __dso_handle but libc has no __cxa_finalize
						 */
						if ( !my__cxa_finalize ) {
							fprintf(stderr,
								"Error: found '"DSO_HANDLE_NAME"' but not '__cxa_finalize'\n");
							fprintf(stderr,
								"       No '__cxa_atexit' support!\n");
							ld->errors++;
		continue;
						}
						sp = bfd_make_empty_symbol(abfd);
						/* copy pointer to name */
						bfd_asymbol_name(sp) = bfd_asymbol_name(ts);
						bfd_asymbol_set_value(sp, (symvalue)ld->module);
						bfd_set_section(sp, bfd_abs_section_ptr);
						sp->flags   = BSF_LOCAL;
					} else {
						/* Resolved reference; replace the symbol pointer
						 * in this slot with a new asymbol holding the
						 * resolved value.
						 */
						sp=asymFromCexpSym(abfd,ts,ld->depend,mod);
					}
					*ppsym  = sp;
					symsect = bfd_get_section(*ppsym);
				} else {
					fprintf(stderr,"Unresolved symbol: %s\n",bfd_asymbol_name(sp));
					ld->errors++;
		continue;
				}
			}
			/* Ignore relocs that reference dropped linkonce sections */
			/* FIXME: We really should have a dedicated flag (and not overload SEC_ALLOC)
			 *        to indicate that we deal with a dropped linkonce. Unfortunately,
			 *        in BFD, there is no flag available to the user...
			 */
			else if ( !(SEC_ALLOC & bfd_get_section_flags(abfd, symsect) ) && ! bfd_is_abs_section(symsect) && !bfd_is_com_section(symsect) ) {
				/* FIXME: should we add a paranoia check that the 'addend' is zero? gcc only skips
				 *        NULL relocs in .eh_frame if the relocation value is zero.
				 */
#if ! (DEBUG & DEBUG_RELOC)
				/* reloc referencing a symbol in a dropped linkonce should not happen unless from .eh_frame;
				 * warn about it:
				 */
				if ( sect != ld->eh_section )
#endif
				{
					fprintf(stderr, "WARNING:\n");
					fprintf(stderr, "Ignoring/skipping reloc   [0x%08lx = %s@%s]\n",
					                (unsigned long)bfd_asymbol_value(*ppsym),
					                bfd_asymbol_name(*ppsym),
					                bfd_get_section_name(abfd, symsect));
					fprintf(stderr, "  [IN DROPPED LINKONCE] => 0x%08lx@%s (sym_ptr_ptr = 0x%08lx)\n",
					                (unsigned long)reloc_get_address(abfd, r),
					                bfd_get_section_name(abfd, sect),
					                (unsigned long)ppsym
					       );
				}
			continue;
			}

#if DEBUG & DEBUG_RELOC
			printf("relocating [0x%08lx = %s@%s]\n",
			        (unsigned long)bfd_asymbol_value(*ppsym),
					bfd_asymbol_name(*ppsym),
			        bfd_get_section_name(abfd, symsect)
			      );
			printf("         => 0x%08lx@%s (sym_ptr_ptr = 0x%08lx)\n",
			       (unsigned long)reloc_get_address(abfd, r),
			       bfd_get_section_name(abfd, sect),
			       (unsigned long)ppsym
			      );
#endif
#ifndef _PMBFD_
			err=bfd_perform_relocation(
				abfd,
				r,
				(PTR)vma,
				sect,
				0 /* output bfd */,
				0
				);
#else
			err=pmbfd_perform_relocation(
				abfd,
				r,
				*ppsym,
				sect
				);
#endif
			if ( err ) {
				fprintf(stderr,
					"Relocation of type '%s'@0x%08lx[%s] ->'%s' failed: %s (check compiler flags!)\n",
					reloc_get_name(abfd, r),
					reloc_get_address(abfd, r),
					bfd_get_section_name(abfd,sect),
					bfd_asymbol_name(*ppsym),
					err >= NumberOf(reloc_err_msg) ? "unknown error" : reloc_err_msg[err]);
				ld->errors++;
			}
		}
		free(cr);
	}
}

typedef struct CtorSymRec_ {
	asymbol	*sym;	/* the symbol */
	int	pri;	/* priority we have extracted already */
} CtorSymRec, *CtorSym;

static int
ctor_cmp(const void *a, const void *b)
{
register int	 diff;
register CtorSym sa = (CtorSym)a;
register CtorSym sb = (CtorSym)b;

	/* priorities are in inverse order */
	if ( (diff = sa->pri - sb->pri ) )
		return diff;

	/* same priority -- enforce keeping the current
	 * ordering
	 */
	return sa-sb;
}


/* build the module's static constructor and
 * destructor lists.
 */
static void
buildCtorsDtors(LinkData ld)
{
asymbol		**asymp;
CtorSym		ctorsyms = 0;
CtorSym		dtorsyms = 0;
int			i,j;
CexpModule	mod = ld->module;

	if (ld->nCtors) {
		mod->ctor_list=(VoidFnPtr*)xmalloc(sizeof(*mod->ctor_list) * (ld->nCtors));
		memset(mod->ctor_list,0,sizeof(*mod->ctor_list) * (ld->nCtors));
		ctorsyms=(CtorSym)xmalloc(sizeof(*ctorsyms) * ld->nCtors);
	}
	if (ld->nDtors) {
		mod->dtor_list=(VoidFnPtr*)xmalloc(sizeof(*mod->dtor_list) * (ld->nDtors));
		memset(mod->dtor_list,0,sizeof(*mod->dtor_list) * (ld->nDtors));
		dtorsyms=(CtorSym)xmalloc(sizeof(*dtorsyms) * ld->nDtors);
	}

	for (asymp=ld->st; *asymp; asymp++) {
		int	pri;
		switch (isCtorDtor(*asymp,0/*quiet*/, &pri)) {
			case 1:
				assert(mod->nCtors<ld->nCtors);
				ctorsyms[mod->nCtors].sym = *asymp;
				ctorsyms[mod->nCtors].pri = pri;
				mod->nCtors++;
			break;

			case -1:
				assert(mod->nDtors<ld->nDtors);
				dtorsyms[mod->nDtors].sym = *asymp;
				dtorsyms[mod->nDtors].pri = pri;
				mod->nDtors++;
			break;

			default:
			break;
		}
	}
	assert(mod->nCtors == ld->nCtors);
	assert(mod->nDtors == ld->nDtors);

	/* sort the ctors/dtors to support ordered initialization/destruction */
	if (ld->nCtors)
		qsort(ctorsyms, ld->nCtors, sizeof(*ctorsyms), ctor_cmp); 
	if (ld->nDtors)
		qsort(dtorsyms, ld->nDtors, sizeof(*dtorsyms), ctor_cmp); 

	/* store the symbol values in the module's ctor/dtor lists */
	for (i=0; i<ld->nCtors; i++) {
		mod->ctor_list[i]=(VoidFnPtr)bfd_asymbol_value(ctorsyms[i].sym);
#if (DEBUG) & DEBUG_CDPRI
		printf("Ctor %.40s at priority %i\n",
				bfd_asymbol_name(ctorsyms[i].sym),
				ctorsyms[i].pri);
#endif
	}
	/* inverse the order of calling the destructors */
	for (i=0, j=ld->nDtors-1; i<ld->nDtors; i++,j--) {
		mod->dtor_list[i]=(VoidFnPtr)bfd_asymbol_value(dtorsyms[j].sym);
#if (DEBUG) & DEBUG_CDPRI
		printf("Dtor %.40s at priority %i\n",
				bfd_asymbol_name(dtorsyms[i].sym),
				dtorsyms[i].pri);
#endif
	}

	free(ctorsyms);
	free(dtorsyms);
}

/* convert a CexpSym to a (temporary) BFD symbol and update module
 * dependencies
 */
static asymbol *
asymFromCexpSym(bfd *abfd, CexpSym csym, BitmapWord *depend, CexpModule mod)
{
asymbol *sp = bfd_make_empty_symbol(abfd);
	/* TODO: check size and alignment */

	/* copy pointer to name */
	bfd_asymbol_name(sp) = csym->name;
	bfd_asymbol_set_value(sp, (symvalue)csym->value.ptv);
	bfd_set_section(sp, bfd_abs_section_ptr);
	sp->flags=BSF_GLOBAL;
	/* mark the referenced module in the bitmap */
	assert(mod->id < MAX_NUM_MODULES);
	BITMAP_SET(depend,mod->id);
	return sp;
}

static void
add_new_common(LinkData ld, asymbol *sp, asymbol **slot)
{
	/* we'll have to add it to our internal symbol table */
	sp->flags |= BSF_KEEP|BSF_OLD_COMMON;

	/* 
	 * we must not change the order of the symbol list;
	 * _elf_bfd_canonicalize_reloc() relies on the order being
	 * preserved.
	 * Therefore, we create an array of pointers which
	 * we can sort in any desired way...
	 */
	ld->new_commons=(asymbol***)xrealloc(ld->new_commons,
			sizeof(*ld->new_commons)*(ld->num_new_commons+1));
	ld->new_commons[ld->num_new_commons] = slot;
	*slot = sp;
	ld->num_new_commons++;
}

/* resolve undefined and common symbol references;
 * The routine also creates a list of
 * all common symbols that need to be created.
 *
 * RETURNS:
 *   value >= 0 : OK, number of new common symbols
 *   value <  0 : -number of errors (unresolved refs or multiple definitions)
 *
 * SIDE EFFECTS:
 *   - resolved undef or common slots are replaced by new symbols
 *     pointing to the ABS section
 *   - KEEP flag is set for all globals defined by this object.
 *   - KEEP flag is set for all section name symbols, except for
 *     non-ALLOCed linkonce sections (these had already been loaded
 *     as part of other modules).
 *   - raise a bit for each module referenced by this new one.
 */
static int
resolve_syms(bfd *abfd, asymbol **syms, LinkData ld)
{
asymbol		*sp;
int			i,errs=0;

	ld->num_new_commons = 0;

	/* resolve undefined and common symbols;
	 * find name clashes
	 */
	for (i=0; (sp=syms[i]); i++) {
		asection	*sect=bfd_get_section(sp);
		CexpSym		ts;
		int			res;
		CexpModule	mod;
		const char	*symname;

#if DEBUG  & DEBUG_SYM
		printf("scanning SYM (flags 0x%08lx, value 0x%08lx) '%s'\n",
						(unsigned long)sp->flags,
						(unsigned long)bfd_asymbol_value(sp),
						bfd_asymbol_name(sp));
		{
		unsigned long vma;
		if ( check_get_section_vma(abfd, sect, &vma) ) {
			printf("         section vma INVALID in %s\n",
					bfd_get_section_name(abfd,sect));
			
		} else {
			printf("         section vma 0x%08lx in %s\n",
						vma,
						bfd_get_section_name(abfd,sect));
		}
		}
#endif
		/* count constructors/destructors */
		if ((res=isCtorDtor(sp,1/*warn*/,0/*don't care about priority*/))) {
			if (res>0)
				ld->nCtors++;
			else
				ld->nDtors++;
		}

#ifdef OBSOLETE_EH_STUFF
		/* mark end with 32-bit 0 if this symbol is contained
 		 * in a ".eh_frame" section
		 */
		if (!strcmp(EH_FRAME_BEGIN_PATTERN, bfd_asymbol_name(sp))) {
			ld->eh_frame_b_sym=sp;

		/* check for magic initializer/finalizer symbols */
		} else
#endif
		if (!strcmp(CEXPMOD_INITIALIZER_SYM, bfd_asymbol_name(sp))) {

			/* set the local flag; we dont want these in our symbol table */
			sp->flags |= BSF_LOCAL;
			/* store a pointer to the internal symbol table slot; we lookup the
			 * value after relocation
			 */
			if (ld->iniCallback) {
				fprintf(stderr,
						"Only one '%s' may be present in a loaded object\n",
						CEXPMOD_INITIALIZER_SYM);
				errs++;
			} else {
				ld->iniCallback=syms+i;
			}

		} else if (!strcmp(CEXPMOD_FINALIZER_SYM, bfd_asymbol_name(sp))) {
			sp->flags |= BSF_LOCAL;
			if (ld->finiCallback) {
				fprintf(stderr,
						"Only one '%s' may be present in a loaded object\n",
						CEXPMOD_FINALIZER_SYM);
				errs++;
			} else {
				ld->finiCallback=syms+i;
			}
		}


		/* we only care about global symbols
		 * (NOTE: undefined symbols are neither local
		 *        nor global)
		 * The only exception are the name symbols of allocated sections
		 * - there may be quite a lot of them ("gnu.linkonce.x.yyy") in a
		 * large C++ object...
		 */

		if ( BSF_LOCAL & sp->flags ) {
			if ( BSF_SECTION_SYM & sp->flags ) {
				/* keep allocated section names in CEXP's symbol table so we can
				 * tell the debugger their load address.
				 *
				 * NOTE: The already loaded modules have been scanned for
				 *       gnu.linkonce.xxx sections in 's_basic()' where
				 *		 redundant occurrencies have been de-SEC_ALLOCed
				 *		 from this module.
				 */
				if ( (SEC_ALLOC & bfd_get_section_flags(abfd, sect)) ) {
						sp->flags|=BSF_KEEP;
				}
			}
			continue; /* proceed with the next symbol */
		}

		/* non-local, i.e. global or undefined */
		symname = bfd_asymbol_name(sp);

		if ( BSF_THREAD_LOCAL & sp->flags ) {
			fprintf(stderr,"Warning: ignoring thread-local symbol '%s'; no TLS support\n", symname);
			continue; /* proceed with the next symbol */
		}

		ts=cexpSymLookup(symname, &mod);

		if (bfd_is_und_section(sect)) {
			/* do some magic on undefined symbols which do have a
			 * type. These are probably sitting in a shared
			 * library and _do_ have a valid value. (This is the
			 * case when loading the symbol table e.g. on a 
			 * linux platform.)
			 */
			if (!ts && bfd_asymbol_value(sp) && (BSF_FUNCTION & sp->flags)) {
				sp->flags |= BSF_KEEP;
				/* resolve references at relocation time */
			} else if ( !ts && (BSF_WEAK & sp->flags) ) {
				/* handle weak undef. like a common symbol which
				 * we found for the first time.
				 */
				add_new_common(ld, sp, syms+i);
			}
		}
		else if (bfd_is_com_section(sect)) {
			if (ts) {
				int newsz;

				/* There are not weak common symbols so if
				 * it is weak then it is so due to us earlier
				 * linking a weak undefined symbol.
				 */
				const char *symcat = "COMMON";

				if ( CEXP_SYMFLG_WEAK & ts->flags ) {
					symcat = "(weak undefined)";
				}
				/* make sure existing symbol meets size and alignment requirements
				 * of new one.
				 */
				if ( ts->size != (newsz = elf_get_size(abfd, sp)) ) {
					if ( ts->size > newsz ) {
						fprintf(stderr,"Warning: Existing %s symbol '%s' larger than space requested by loadee\n", symcat, ts->name);
					} else {
						fprintf(stderr,"Error: Existing %s symbol '%s' smaller than space requested by loadee\n", symcat, ts->name);
						errs ++;
					}
				}
				if ( (unsigned long)ts->value.ptv & (elf_get_align(abfd, sp) -1 ) ) {
					fprintf(stderr,"Existing %s symbol %s doesn't meet alignment requirement of the loadee\n", symcat, ts->name); 
					errs++;
				}
				/* use existing value of common sym */
				sp = asymFromCexpSym(abfd,ts,ld->depend,mod);

				syms[i]=sp; /* use new instance */
			} else {

				/* it's the first definition of this common symbol,
				 * we'll have to add it to our internal symbol table
				 */

				add_new_common(ld, sp, syms+i);
			}
		} else {
			/* any other symbol, i.e. neither undefined nor common */
			if (ts) {
				if (sp->flags & BSF_WEAK) {
					/* use existing instance */
					sp=syms[i]=asymFromCexpSym(abfd,ts,ld->depend,mod);
				} else {
					fprintf(stderr,"Symbol '%s' already exists",symname);
					errs++;
					/* TODO: check size and alignment; allow multiple
					 *       definitions??? - If yes, we have to track
					 *		 the dependencies also.
					 */
					/* TODO: we should allow a new 'strong' symbol to
					 *       override an existing 'weak' one. This is
					 *       hard to do however because we would have
					 *       to change all old relocations referring
					 *       to the existing weak definition!
					 */
					if ( ts->flags & CEXP_SYMFLG_WEAK ) {
						fprintf(stderr," (as a weak symbol, but I cannot re-link already loaded objects, sorry)");
					}
					fputc('\n',stderr);
				}
			} else {
				/* new symbol defined by the loaded object; account for it */
				/* mark for second pass */
				sp->flags |= BSF_KEEP;
			}
		}
	}

	return errs ? -errs : ld->num_new_commons;
}

/* make a dummy section holding the new common data objects introduced by
 * the newly loaded file.
 *
 * RETURNS: 0 on failure, nonzero on success
 */
static int
make_new_commons(bfd *abfd, LinkData ld)
{
unsigned long	i,val;
asection	*csect;

	if (ld->num_new_commons) {
		/* make a dummy section for new common symbols */
		ld->dummy_section_name = bfd_get_unique_section_name(abfd,".dummy",0);
		csect=bfd_make_section(abfd, ld->dummy_section_name);
		if (!csect) {
			bfd_perror("Creating dummy section");
			return -1;
		}
		bfd_set_section_flags( abfd, csect, bfd_get_section_flags( abfd, csect ) | SEC_ALLOC );
		/* our common section alignment is the maximal alignment
		 * found during the sorting process which is the alignment
		 * of the first element...
		 */
		bfd_set_section_alignment(abfd, csect, elf_get_align_pwr(abfd,*ld->new_commons[0]));

		/* set new common symbol values */
		for (val=0,i=0; i<ld->num_new_commons; i++) {
			asymbol *sp;
			int apwr;

			sp = bfd_make_empty_symbol(abfd);

			/* to minimize fragmentation, the new commons have been sorted
			 * in decreasing order according to their alignment requirements
			 * (probably not supported on formats other than ELF)
			 */
			apwr = elf_get_align_pwr(abfd, *ld->new_commons[i]);
			val=align_power(val,apwr);
#if DEBUG & DEBUG_COMMON
			printf("New common: %s; align_pwr %i\n",bfd_asymbol_name(*ld->new_commons[i]),apwr);
#endif
			/* copy pointer to old name */
			bfd_asymbol_name(sp) = bfd_asymbol_name(*ld->new_commons[i]);
			bfd_asymbol_set_value(sp, val);
			bfd_set_section(sp, csect);
			sp->flags=(*ld->new_commons[i])->flags;
			val+=bfd_asymbol_value(*ld->new_commons[i]);
			*ld->new_commons[i] = sp;
		}
		
		bfd_set_section_size(abfd, csect, val);
	}
	return 0;
}

/* this routine does the following:
 *
 *  - build the external (BFD) symtab
 *  - append external symbols for the names of new
 *    (i.e. not already loaded) 'linkonce' sections
 *    to the BFD symtab.
 *  - resolve undefined and already loaded common symbols
 *    (i.e. substitute their asymbol by a new one using
 *    the known values). This is actually performed by
 *    resolve_syms().
 *  - create the internal representation (CexpSym) of
 *    the symbol table.
 *  - make newly introduced common symbols. This is done
 *    by first creating a new section, sorting the common
 *    syms by size in reverse order (for meeting alignment
 *    requirements) and generating clones for them 
 *    associated with the new section.
 *
 * NOTE: Loading the initial symbol table with this routine
 *       relies on the assumption that there are no
 *        COMMON symbols present.
 */

static int
slurp_symtab(bfd *abfd, LinkData ld)
{
asymbol 			**asyms=0;
long				i,nsyms;
long				num_new_commons;

	if (!(HAS_SYMS & bfd_get_file_flags(abfd))) {
		fprintf(stderr,"No symbols found\n");
		return -1;
	}
	if ((i=bfd_get_symtab_upper_bound(abfd))<0) {
		fprintf(stderr,"Fatal error: illegal symtab size\n");
		return -1;
	}

	/* Allocate space for the symbol table  */
	if (i) {
		asyms=(asymbol**)xmalloc((i));
	}
	if ( (nsyms = bfd_canonicalize_symtab(abfd,asyms)) <= 0) {
		bfd_perror("Canonicalizing symtab");
		goto cleanup;
	}

	if ( (num_new_commons=resolve_syms(abfd,asyms,ld)) <0 ) {
		goto cleanup;
	}

#if defined(HAVE_ELF_BFD_H) || defined(_PMBFD_)
	/*
	 *	sort the list of new_commons by alignment
	 */
	if (ISELF(abfd) && num_new_commons)
		qsort(
			(void*)ld->new_commons,
			num_new_commons,
			sizeof(*ld->new_commons),
			align_size_compare);
#endif
	
	/* Now, everything is in place to build our internal symbol table
	 * representation.
	 * We cannot do this later, because the size information will be lost.
	 * However, we cannot provide the values yet; this can only be done
	 * after relocation has been performed.
	 */

	ld->cst=cexpCreateSymTbl(asyms,
							 sizeof(*asyms),
							 nsyms,
							 filter, assign, ld);

	if (0!=make_new_commons(abfd,ld))
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


static void
flushCache(LinkData ld)
{
#if defined(CACHE_LINE_SIZE)
int	i;
char	*start, *end;
	for (i=0; i<ld->nsegs; i++) {
		if (ld->segs[i].size) {
			start=(char*)ld->segs[i].chunk;
			/* align back to cache line */
			start-=(unsigned long)start % CACHE_LINE_SIZE;
			end  =(char*)ld->segs[i].chunk+ld->segs[i].size;

			for (; start<end; start+=CACHE_LINE_SIZE)
				FLUSHINVAL_LINE(start);
		}
	}
	/* enforce flush completion and discard preloaded instructions */
	FLUSHFINISH();
#endif
}


/* the caller of this routine holds the module lock */ 
int
cexpLoadFile(char *filename, CexpModule mod)
{
LinkDataRec						ldr;
int								rval=1,i;
CexpSym							sane, *psym;
FILE							*f=0;
void							*ehFrame=0;
#ifndef OBSOLETE_EH_STUFF
unsigned long                   eh_vma;
#endif
#ifdef __rtems__
char							tmpfname[30]={
		'/','t','m','p','/','m','o','d','X','X','X','X','X','X',
		0};
#else
#define tmpfname 0
#endif

char							*targ;
char							*thename = 0;

#ifdef USE_PMBFD
Pmelf_attribute_set             *obj_atts = 0;
CexpModule                      m;
#endif

	/* clear out the private data area; the cleanup code
	 * relies on this...
	 */
	memset(&ldr,0,sizeof(ldr));

	ldr.depend = mod->needs;
	ldr.module = mod;

	if ( (ldr.nsegs = cexpSegsInit(&ldr.segs)) < 1 ) {
		fprintf(stderr,"Unable to allocate segment table -- no memory?\n");
		goto cleanup;
	}

	/* basic check for our bitmaps */
	assert( (1<<LD_WORDLEN) <= sizeof(BitmapWord)*8 );

	if (!filename) {
		fprintf(stderr,"Need filename arg\n");
		goto cleanup;
	}

	bfd_init();

	/* make sure initialization is complete in case the system symtab was
	 * read using another method
	 */
	if (cexpSystemModule) 
		bfdstuff_complete_init(cexpSystemModule->symtbl);

	/* lazy init of regexp pattern */
	if (!ctorDtorRegexp)
		ctorDtorRegexp=cexp_regcomp(CTOR_DTOR_PATTERN);

	thename = malloc(MAXPATHLEN);

	f = cexpSearchFile(getenv("PATH"), &filename, thename, tmpfname);

	if ( !f ) {
		perror("opening object file (check PATH)");
		goto cleanup;
	}


	if ( ! (ldr.abfd=bfd_openstreamr(filename,0,f)) ) {
		bfd_perror("Opening BFD on object file stream");
		goto cleanup;
	}

	/* ldr.abfd now holds the descriptor and bfd_close_all_done() will release it */
	f=0;

	if (!bfd_check_format(ldr.abfd, bfd_object)) {
		bfd_perror("Checking format");
		goto cleanup;
	}

	targ = bfd_get_target(ldr.abfd);

	if ( !bfd_scan_arch(bfd_printable_name(ldr.abfd)) ) {
		fprintf(stderr,"Architecture mismatch: refuse to load a '%s/%s' module\n",
							targ,
							bfd_printable_name(ldr.abfd));
		goto cleanup;
	}


	/*
	 * Check compatibility as described by a '.gnu.attributes'
	 * section. Do this only if we are building for pmbfd
	 * and if pmelf has an implementation for the host CPU/ABI.
	 */
#if defined(USE_PMBFD) && defined(PMELF_ATTRIBUTE_VENDOR)
	obj_atts = pmbfd_get_file_attributes(ldr.abfd);
	/* FIXME: should we warn if no attributes could be
	 *        constructed? Maybe an older gcc simply
	 *        didn't create a .gnu.attributes section...
	 */

	/* Definitely warn if system module has no attributes
	 * but loadee does...
	 */
	if ( cexpSystemModule && ! cexpSystemModule->fileAttributes && obj_atts ) {
		fprintf(stderr,
		        "Warning: system module has no file attributes but loadee\n"
		        "has a '.gnu.attributes' section; IGNORING possible ABI\n"
				"incompatibilities\n");
		pmelf_destroy_attribute_set(obj_atts);
		obj_atts = 0;
	}
	
	/* Check compatibility of this module with all previously
	 * loaded ones. Should be OK even if we are just loading
	 * the symbol (aka system module).
	 */
	for ( m = cexpSystemModule; m; m = m->next ) {
		if ( pmelf_match_attribute_set(m->fileAttributes, obj_atts) ) {
			fprintf(stderr,
			        "Mismatch of object file attributes (ABI) with module '%s' found\n",
					m->name);
			if ( _cexpForceIgnoreObjAttrMismatches ) {
				fprintf(stderr,"WARNING: forced to ignore mismatch\n");
			} else {
				goto cleanup;
			}
		}
	}
#endif

	/* Get number of all ALLOC sections to compute the
	 * number of all section names we remember (for debugger
	 * support).
	 */
	bfd_map_over_sections(ldr.abfd, s_nsects, &ldr);
	if (!(ldr.module->section_syms =
			malloc( sizeof(*ldr.module->section_syms)
				    * (ldr.num_section_names + 1) )) ) {
		fprintf(stderr,"No memory\n");
		goto cleanup;
	}
	ldr.num_section_names = 0; /* reset index to be reused in s_basic */
	

	/* Get basic section info and filter linkonce sections */
	bfd_map_over_sections(ldr.abfd, s_basic, &ldr);
	ldr.module->section_syms[ldr.num_section_names]=0;

	/* Eliminate linkonce sections that had already been linked as part
	 * of previous modules.
	 * This also counts the final number of sections to allocate/load
	 * (after elimination of linkonce-s).
	 */
	bfd_map_over_sections(ldr.abfd, s_eliminate_linkonce, &ldr);

	if (slurp_symtab(ldr.abfd,&ldr)) {
		fprintf(stderr,"Error creating symbol table\n");
		goto cleanup;
	}

	/* the first thing to be loaded must be the system
	 * symbol table. Reject to load anything in this case
	 * which allows for using the executable as the
	 * symbol file.
	 */
	if ( ! (0==cexpSystemModule) ) {

		/* compute the memory space requirements */
		ldr.errors = 0;
		bfd_map_over_sections(ldr.abfd, s_count, &ldr);
		if (ldr.errors)
			goto cleanup;

		/* allocate segment space */
		if ( cexpSegsAlloc(ldr.segs) ) {
			fprintf(stderr,"Unable to allocate memory segments\n");
			goto cleanup;
		}
	
		for (i=0; i<ldr.nsegs; i++) {
			unsigned long chunk = (unsigned long)ldr.segs[i].chunk;

			ldr.segs[i].vmacalc= chunk ? chunk : (unsigned long)-1;

if ( chunk ) memset(ldr.segs[i].chunk, 0xee,ldr.segs[i].size); /*TSILL*/
		}

		/* compute and set the base addresses for all sections to be allocated */
		ldr.errors=0;
		bfd_map_over_sections(ldr.abfd, s_setvma, &ldr);
		if ( ldr.errors )
			goto cleanup;

		ldr.errors=0;
		bfd_map_over_sections(ldr.abfd, s_reloc, &ldr);
		if (ldr.errors)
			goto cleanup;

	} else {
		/* it's the system symtab - there should be no real COMMON symbols */
		for ( i=0; i<ldr.num_new_commons; i++ )
			assert( BSF_WEAK & (*ldr.new_commons[i])->flags );
		if ( ldr.text )
			ldr.text_vma=bfd_get_section_vma(ldr.abfd,ldr.text);
	}

	cexpSymTabSetValues(ldr.cst);

	/* record the section names */
	for ( psym = ldr.module->section_syms; *psym; psym++ ) {
		CexpSym s, *tsym;
		/* If no section name is found in this module's symtab
		 * then it must be a previously loaded linkonce...
		 */
		if ( (s = cexpSymTblLookup( (char*)*psym, ldr.cst )) )
			*psym = s;
		else  {
			s = cexpSymLookup( (char*)*psym, 0 );
			if ( !s ) {
				fprintf(stderr,"WARNING: no existing symtab entry found for section '%s'\n", (char*)*psym);
				/* remove this slot */
				tsym = psym +1;
				while ( (*(tsym-1) = *tsym) )
					tsym++;
				psym--;
				ldr.num_section_names--;
			} else {
				*psym = s;
			}
		}
	}

	/* system symbol table sanity check */
	if ((sane=cexpSymTblLookup("cexpLoadFile",ldr.cst))) {
		/* this must be the system table */
		extern char _edata[], _etext[];

        /* it must be the main symbol table */
        if ( sane->value.ptv==(CexpVal)cexpLoadFile     &&
       	     (sane=cexpSymTblLookup("_etext",ldr.cst))  &&
			 (char*)sane->value.ptv==_etext           &&
             (sane=cexpSymTblLookup("_edata",ldr.cst))  &&
			 (char*)sane->value.ptv==_edata ) {

        	/* OK, sanity test passed */

			/* as a side effect, since we have an open BFD, we
			 * use it to fetch a disassembler for the target
			 */
			cexpDisassemblerInstall(ldr.abfd);

		} else {
			fprintf(stderr,"SANITY CHECK FAILED! Stale symbol file?\n");
			goto cleanup;
		}
	}

	/* Another sanity check */
#ifndef OBSOLETE_EH_STUFF
	eh_vma = 0;
	if (cexpSystemModule && ldr.eh_section) {
		if ( check_get_section_vma(ldr.abfd, ldr.eh_section, &eh_vma) ) {
			fprintf(stderr,"Internal Error: Have eh_section but an empty memory segment\n");;
			goto cleanup;
		}
	}
#endif

	/* FROM HERE ON, NO CLEANUP MUST BE DONE */


	/* the following steps are not needed when
	 * loading the system symbol table
	 */
	if (cexpSystemModule) {
		/* if there is an exception frame table, we
		 * have to register it with libgcc. If it was
		 * in its own section, we also must write a terminating 0
		 */
#ifdef OBSOLETE_EH_STUFF
		if (ldr.eh_frame_b_sym) {
			if ( ldr.eh_frame_e_sym ) {
				/* eh_frame was in its own section; hence we have to write a terminating 0
				 */
				*(void **)bfd_asymbol_value(ldr.eh_frame_e_sym)=0;
			}
			assert(my__register_frame);
			ehFrame=(void*)bfd_asymbol_value(ldr.eh_frame_b_sym);
			my__register_frame(ehFrame);
		}
#else
		if (ldr.eh_section) {
			ehFrame = (void*)eh_vma;
			/* write terminating 0 to eh_frame */
			*(void**)( (ehFrame) + bfd_section_size(ldr.abfd, ldr.eh_section) ) = 0;
			assert(my__register_frame);
			my__register_frame(ehFrame);
		}
#endif
		/* build ctor/dtor lists */
		if (ldr.nCtors || ldr.nDtors) {
			buildCtorsDtors(&ldr);
		}
	} else {
		bfdstuff_complete_init(ldr.cst);
	}

	flushCache(&ldr);

	/* move the relevant data over to the module */
	mod->segs    = ldr.segs;
	ldr.segs     = 0;

	/* Add up sizes */
	mod->memSize = 0;
	for ( i=0; i<ldr.nsegs; i++ )
		mod->memSize += mod->segs[i].size;

	mod->symtbl  = ldr.cst;
	ldr.cst=0;

	if (ldr.iniCallback) {
		/* it's still an asymbol**; we now lookup the value */
		mod->iniCallback=(void(*)(CexpModule)) bfd_asymbol_value((*(asymbol**)ldr.iniCallback));
	}
	if (ldr.finiCallback) {
		mod->finiCallback=(int(*)(CexpModule)) bfd_asymbol_value((*(asymbol**)ldr.finiCallback));
	}

	mod->cleanup  = bfdCleanupCallback;
	mod->text_vma = ldr.text_vma;

	mod->fileName = thename;
	thename       = 0;

	/* store the frame info in the modPvt field */
	mod->modPvt   = ehFrame;
	ehFrame       = 0;

#ifdef USE_PMBFD
	mod->fileAttributes = obj_atts;
	obj_atts      = 0;
#endif

	rval          = 0;

	/* fall through to release unused resources */
cleanup:
	if (ldr.st)
		free(ldr.st);
	if (ldr.new_commons)
		free(ldr.new_commons);
	if (ehFrame) {
		assert(my__deregister_frame);
		my__deregister_frame(ehFrame);
	}

	free(thename);

#if defined(USE_PMBFD) && defined(PMELF_ATTRIBUTE_VENDOR)
	if (obj_atts)
		pmelf_destroy_attribute_set(obj_atts);
#endif

	if (ldr.cst)
		cexpFreeSymTbl(&ldr.cst);

	if (ldr.abfd)
		bfd_close_all_done(ldr.abfd);

	if (f) {
		fclose(f);
	}
#ifdef __rtems__
	if (filename==tmpfname)
		unlink(tmpfname);
#endif

	if (ldr.dummy_section_name)
		free(ldr.dummy_section_name);

	cexpSegsDelete(ldr.segs);

	return rval;
}

static void
bfdstuff_complete_init(CexpSymTbl cst)
{
static int done = 0;
CexpSym s;
	if ( done )
		return;
	/* initialize the my__[de]register_frame() pointers if possible */
	if ((s=cexpSymTblLookup("__register_frame",cst)))
		my__register_frame=(void(*)(void*))s->value.ptv;
	if ((s=cexpSymTblLookup("__deregister_frame",cst)))
		my__deregister_frame=(void(*)(void*))s->value.ptv;
	if ((s=cexpSymTblLookup("__cxa_finalize",cst)))
		my__cxa_finalize=(void(*)(void*))s->value.ptv;
	my__dso_handle=cexpSymTblLookup(DSO_HANDLE_NAME,cst);

	/* Handle special cases */
	if ( my__cxa_finalize && !my__dso_handle ) {
		assert(!"TODO: should create dummy '"DSO_HANDLE_NAME"' in system symbol table");
	}
	done = 1;
}

static void
bfdCleanupCallback(CexpModule mod)
{
	if (my__cxa_finalize) {
		my__cxa_finalize(mod);
	}
	/* release the frame info we had stored in the modPvt field */
	if (mod->modPvt) {
		assert(my__deregister_frame);
		my__deregister_frame(mod->modPvt);
	}
}
