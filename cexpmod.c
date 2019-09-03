
/* Implementation of cexp modules */

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <cexp_regex.h>
#include <stdlib.h>
#include <string.h>
/* FIXME: would like to use uintptr_t but some RTEMS versions
 *        make this long long (64-bit) even on a 32-bit machine :-(
 *        which makes the output look bad. Hopefully this can
 *        be fixed in the future...
#include <inttypes.h>
 */
typedef unsigned long myuintptr_t;
typedef          long myintptr_t;
#define MYPRIxPTR "lx"

#include "cexpmodP.h"
#include "cexpsymsP.h"
#include "cexplock.h"
#define _INSIDE_CEXP_
#include "cexpHelp.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef USE_PMBFD
#include <pmelf.h>
#endif

#ifdef HAVE_BFD_DISASSEMBLER
/* Oh well; rtems/score/types.h defines boolean and bfd
 * defines boolean as well :-( Can't you people use names
 * less prone to clashes???
 * We redefine bfd's boolean here
 */
#define  boolean bfdddd_bbboolean
#include <bfd.h>
#undef   boolean
extern void cexpDisassemblerInstall(bfd *abfd);
#endif

CexpModule cexpSystemModule=0;

/* change this whenever the layout of internal data changes */
char *
cexpMagicString = CEXPMOD_MAGIC;

static CexpRWLockRec	_rwlock={0};
#define __RLOCK()	cexpReadLock(&_rwlock)
#define __RUNLOCK()	cexpReadUnlock(&_rwlock)
#define __WLOCK()	cexpWriteLock(&_rwlock)
#define __WUNLOCK()	cexpWriteUnlock(&_rwlock)

void
cexpModuleInitOnce(void)
{
#ifndef NO_THREAD_PROTECTION
	if (!_rwlock.mutex) {
		cexpRWLockInit(&_rwlock);
	}
#endif
}

/* Search predecessor of a module in the list.
 * This should be called with the lock held!
 */

static CexpModule
predecessor(CexpModule mod)
{
register CexpModule pred;

	if (!mod) return 0;

	for (pred=cexpSystemModule; pred && mod!=pred->next; pred=pred->next)
			/* nothing else to do */;
	return pred;
}

/* verify the validity of a handle;
 * should be called with the lock held
 */
static int
modIsStale(CexpModule mod)
{
register CexpModule m;

	for (m=cexpSystemModule; m && m!=mod; m=m->next)
			/* nothing else to do */;
	return m==0;
}


/* search for a name in all module's symbol tables */
CexpSym
cexpSymLookup(const char *name, CexpModule *pmod)
{
CexpModule	m;
CexpSym		rval=0;
int			index;

	__RLOCK();

	for (m=cexpSystemModule, index=0; m; m=m->next, index++) {
		if ((rval=cexpSymTblLookup(name,m->symtbl)))
			break;
	}
	if (pmod)
		*pmod=m;

	__RUNLOCK();

	return rval;
}

static int addrInModule(void *addr, CexpModule m)
{
CexpSegment s;
CexpSymTbl  t;
	for ( s = m->segs; s->name; s++ ) {
		if ( ! s->chunk ) {
			if ( m == cexpSystemModule ) {
				t = m->symtbl;
				/* assume system module is a single chunk (not allocated though) */
				return addr >= (void*)t->aindex[0]->value.ptv && addr <= (void*)t->aindex[t->nentries-1]->value.ptv;
			}
			continue;
		}

		if ( (char*)addr >= (char*)s->chunk && (char*)addr < (char*)s->chunk + s->size )
			return 1;
	}
	return 0;
}

/* search for (the closest) address in all modules giving its
 * aindex
 *
 * RETURNS: -1 if not found (i.e. within the boundaries of) any module.
 */
int
cexpSymLkAddrIdx(void *addr, int margin, FILE *f, CexpModule *pmod)
{
CexpModule	m;
int			rval=-1;
CexpSymTbl	t;

	__RLOCK();

	for (m=cexpSystemModule; m; m=m->next) {
		t=m->symtbl;
		if ( !addrInModule(addr, m) )
			continue;
		if (f)
			fprintf(f,"=====  In module '%s' =====:\n",m->name);
		if ((rval=cexpSymTblLkAddrIdx(addr,margin,f,t)) >= 0)
			break;
	}
	if (pmod)
		*pmod=m;

	__RUNLOCK();

	return rval;
}

/* search for an address in all modules */
CexpSym
cexpSymLkAddr(void *addr, int margin, FILE *f, CexpModule *pmod)
{
int			i;
CexpModule	m;

	if (!pmod)
		pmod = &m;
	i=cexpSymLkAddrIdx(addr,margin,f,pmod);

	return i >= 0 ? (*pmod)->symtbl->aindex[i] : 0;
}


int
cexpAddrFind(void **addr, char *buf, int size)
{
CexpSym s;
CexpModule m;
	s=cexpSymLkAddr(*addr,0,0,&m);
	if (!s)
		return -1;
	if (size>0) {
		snprintf(buf,size,"%s:%s",cexpModuleName(m),s->name);
		/* don't know if snprintf puts a 0 there */
		buf[size-1]=0;
	}
	*addr=s->value.ptv;
	return 0;
}

/* return a module's name (string owned by module code) */
char *
cexpModuleName(CexpModule mod)
{
	return mod->name;
}

/* see comments in cexpsyms.c about this routine. The version
 * here is just a wrapper for looping over modules
 */
CexpSym
_cexpSymLookupRegex(cexp_regex *rc, int *pmax, CexpSym s, FILE *f, CexpModule *pmod)
{
CexpModule	m=0,mfound;
int			max=24;

	if (!pmax)	pmax=&max;

	if (pmod)	{
		/* start at module/symbol passed in */
		m=*pmod;
		if (s && !(++s)->name) {
			/* was the last one */
			s=0;
			if (!(m=m->next))
				return 0;
		}
	} else  {
		s=0;
	}
	if (!m)	m=cexpSystemModule;
    
	__RLOCK();

	if (modIsStale(m)) {
		__RUNLOCK();
		fprintf(f ? f : stderr,"Got a stale module handle; giving up...\n");
		return 0;
	}

	for (; m; m=m->next) {
    	if (!s) s=m->symtbl->syms;
		mfound=0;
		while (s->name && *pmax) {
      		if (cexp_regexec(rc,s->name)) {
				if (!mfound) {
					if (f) fprintf(f,"=====  In module '%s' (0x%08"MYPRIxPTR") =====:\n",m->name, (myuintptr_t)m);
					mfound=m; /* print module name only once */
					(*pmax)--;
				}
				if (f) cexpSymPrintInfo(s,f);
				if (--(*pmax) <= 0) {
					if (pmod)
						*pmod=m;

					__RUNLOCK();
					return s;
				}
			}
			s++;
		}
		s=0;
	}

	__RUNLOCK();

    return 0;
}

static void
bitmapInfo(FILE *f, BitmapWord *bm)
{
CexpModule m;

	for (m=cexpSystemModule; m; m=m->next) {
		if (BITMAP_TST(bm, m->id))
			fprintf(f," %s",m->name);
	}
}

CexpModule
cexpModIterate(CexpModule mod, FILE *f, void mcallback(CexpModule m, FILE *f, void *arg), void *arg)
{
CexpModule m = mod ? mod : cexpSystemModule;

	if (!f) f=stdout;

	__RLOCK();

	if (modIsStale(m)) {
		__RUNLOCK();
		fprintf(f,"Got a stale module handle; giving up...\n");
		return 0;
	}

	for (; m; m=m->next) {
		mcallback(m, f, arg);
		if (mod) {
			mod = mod->next;
			break; /* info only for the particular module requested */
		}
	}

	__RUNLOCK();

	return mod;
}

static void
modPrintInfo(CexpModule m, FILE *f, void *closure)
{
myintptr_t	level = (myintptr_t)closure;
CexpSym	*psects;
CexpSegment s;
	fprintf(f,"Module '%s' (0x%08"MYPRIxPTR"):\n",
				m->name, (myuintptr_t)m);
	if ( level > 0 )
		fprintf(f,"Full path '%s'\n",m->fileName);
	if ( level > 1 ) {
		fprintf(f,"  %li symbol table entries\n",
						m->symtbl->nentries);
		fprintf(f,"  %li bytes of memory allocated to binary\n",
						m->memSize);
		if ( m->segs ) {
			fprintf(f,"Memory segment layout:\n");
			for ( s=m->segs; s->name; s++ ) {
				fprintf(f,"  %20s@0x%08lx (0x%08lx/%lu bytes)\n",
						s->name,
						(unsigned long)s->chunk,
						s->size, s->size);
			}
		}
	}
	fprintf(f,"  Text starts at: 0x%08x\n",
					(unsigned)m->text_vma);
	if ( level > 0 ) {
		fprintf(f,"  Needs:"); bitmapInfo(f,m->needs); fputc('\n',f);
		fprintf(f,"  Needed by:"); bitmapInfo(f,m->neededby); fputc('\n',f);
	}
	if ( level > 2 && ( psects = m->section_syms ) ) {
		fprintf(f,"  Section load info:\n");
		for ( ; *psects; psects++ )
			fprintf(f,"  @0x%08"MYPRIxPTR": %s\n", (myuintptr_t)(*psects)->value.ptv, (*psects)->name);
	}
}

CexpModule
cexpModuleInfo(CexpModule mod, int level, FILE *f)
{
	return cexpModIterate(mod, f, modPrintInfo, (void*)(myintptr_t)level);
}

static void
modPrintGdbSects(CexpModule m, FILE *f, void *closure)
{
char	*prefix = (char*)closure;
CexpSym	*psects;

	if ( m == cexpSystemModule )
		return;

	if ( !prefix )
		prefix="add-symbol-file ";
	else if ( ((myintptr_t)-1) == (myintptr_t)prefix )
		prefix=0;

	fprintf(f,"%s%s 0x%08lx", prefix ? prefix : "", m->name, m->text_vma);
	if ( ( psects = m->section_syms ) ) {
		for ( ; *psects; psects++ )
			fprintf(f," -s%s 0x%08"MYPRIxPTR, (*psects)->name, (myuintptr_t)(*psects)->value.ptv);
	}
	fputc('\n',f);
}

CexpModule
cexpModuleDumpGdbSectionInfo(CexpModule mod, char *prefix, FILE *feil)
{
	return cexpModIterate(mod, feil, modPrintGdbSects, (void*)prefix);
}

CexpModule
cexpModuleFindByName(char *needle, FILE *f)
{
cexp_regex	*rc=0;
CexpModule	m,found=0;

	if (!f)
		f=stdout;
	else if (CEXP_FILE_QUIET == f)
		f=0;

	if (!(rc=cexp_regcomp(needle))) {
		fprintf(stderr,"unable to compile regexp '%s'\n", needle);
		return 0;
	}

	__RLOCK();

	for (m=cexpSystemModule; m; m=m->next) {
		if (cexp_regexec(rc,m->name)) {
			/* record first item found */
			if (!found)
				found=m;
			if (f)
				fprintf(f,"0x%08"MYPRIxPTR": %s\n",(myuintptr_t)m, m->name);
			else
				break;
		}
	}

	__RUNLOCK();

	cexp_regfree(rc);
	return found;
}

#ifdef USE_LOADER
int
cexpModuleUnload(CexpModule mod)
{
int			i;
CexpModule	pred,m;
CexpSegment s;

	__WLOCK();

	/* is mod in the list at all ? */
	pred=predecessor(mod);

	if (mod && mod==cexpSystemModule) {
		fprintf(stderr,"Cannot unload system symbol table\n");
		goto cleanup;
	}

	if (!mod || !pred) {
		fprintf(stderr,"Cannot unload: bad module handle\n");
		goto cleanup;
	}

	for (i=0; i<BITMAP_DEPTH; i++) {
		if (mod->neededby[i]) {
			fprintf(stderr,"Cannot unload %s; still needed by:", mod->name);
			bitmapInfo(stderr,mod->neededby);
			fputc('\n',stderr);
			goto cleanup;
		}
	}

	if (mod->finiCallback && mod->finiCallback(mod)) {
		fprintf(stderr,"Unload rejected by module finalizer!\n");
		/* unload rejected by module */
		goto cleanup;
	}


	/* remove from dependency bitmaps */
	for (m=cexpSystemModule; m; m=m->next)
		BITMAP_CLR(m->neededby, mod->id);

	/* call destructors */
	{
		int i;
		for (i=0; i<mod->nDtors; i++) {
			if (mod->dtor_list[i])
				mod->dtor_list[i]();
		}
	}

	/* call module cleanup routine */
	if (mod->cleanup)
		mod->cleanup(mod);

	/* remove from list */
	pred->next=mod->next;
	mod->next=0;

	__WUNLOCK();

	for ( s = mod->segs; s->name; s++ ) {

		if ( ! s->chunk )
			continue;

#ifdef HAVE_SYS_MMAN_H
		/* HACK: NEVER remove 'x' attribute. Since our memory segments are not
		 *       page-aligned it could be that we revoke 'x' for parts of memory
		 *       that are located outside of our segments which may still be
		 *       live, executable modules.
		 */
		if ( 0 ) {
			unsigned long nsiz, pgbeg, pgmsk;
			pgmsk  = getpagesize()-1;
			pgbeg  = (unsigned long)s->chunk;
			pgbeg &= ~pgmsk;
			nsiz   = s->size + (unsigned long)s->chunk - pgbeg; 
			nsiz   = (nsiz + pgmsk) & ~pgmsk;
			if ( mprotect((void*)pgbeg, nsiz, PROT_READ | PROT_WRITE) )
				perror("ERROR -- mprotect(PROT_READ|PROT_WRITE)");
		}
#endif
		memset(s->chunk, 0, s->size);
	}

	/* could flush the caches here */

	cexpModuleFree(&mod);

	return 0;

cleanup:
	__WUNLOCK();
	return -1;
}
#endif

static void
addDependencies(CexpModule nmod)
{
CexpModule m;
	for (m=cexpSystemModule; m; m=m->next) {
		if (BITMAP_TST(nmod->needs,m->id))
			BITMAP_SET(m->neededby,nmod->id);
	}
}

/* NOTE: the caller of this helper must hold the module lock
 *
 * This routine is NOT reentrant.
 *
 * RETURNS: new unused module id, -1 on error
 */

static int
idAlloc(CexpModule mod)
{
static ModuleId id=MAX_NUM_MODULES-1;
ModuleId	new_id;
BITMAP_DECLARE(used);
CexpModule	m;

	memset(used,0,sizeof(used));

	/* mark all used ids */
	for (m=cexpSystemModule; m; m=m->next) {
			BITMAP_SET(used,m->id);
	}

	new_id = id;
	do {
		new_id = (new_id + 1) % MAX_NUM_MODULES;
		if (new_id == id)
			return -1;
	} while (BITMAP_TST(used,new_id));

	mod->id=id=new_id;

	return 0;
}

static CexpSym cexpNoBuiltinSymbols = 0;
char           *cexpBuiltinCpuArch  = 0;

extern CexpSym cexpSystemSymbols __attribute__((weak, alias("cexpNoBuiltinSymbols")));
#ifdef USE_PMBFD
static unsigned      cexpNoBuiltinAttributesSize = 0;

extern const uint8_t *cexpSystemAttributes    __attribute__((weak));
extern unsigned      cexpSystemAttributesSize __attribute__((weak, alias("cexpNoBuiltinAttributesSize")));
#endif

static int
cexpLoadBuiltinSymtab(CexpModule nmod)
{
int nsyms, nsect_syms, i;
CexpSym s;
	if ( !cexpSystemSymbols ) {
		fprintf(stderr,"No builtin symbol list found\n");
		return -1;
	}

	for ( nsect_syms = nsyms=0; cexpSystemSymbols[nsyms].name; nsyms++ ) {
		if ( cexpSystemSymbols[nsyms].flags & CEXP_SYMFLG_SECT )
			nsect_syms++;
	}

	if ( !(nmod->symtbl = cexpCreateSymTbl(cexpSystemSymbols, sizeof(*cexpSystemSymbols), nsyms, 0, 0, 0)) ) {
		fprintf(stderr,"Reading builtin system symbol table failed\n");
		return -1;
	}

	nmod->text_vma = 0xdeadbeef;

	nmod->section_syms = malloc( (nsect_syms + 1) * sizeof(CexpSym) );
	for ( i = 0, s = cexpSystemSymbols, nsect_syms = 0; i<nsyms; i++, s++ ) {
		if ( s->flags & CEXP_SYMFLG_SECT ) {
			nmod->section_syms[nsect_syms++] = s;
			if ( !strcmp(".text",s->name) ) {
				nmod->text_vma = (unsigned long)s->value.ptv;
			}
		}
		/* guess type */
		if ( TVoid == s->value.type )
			s->value.type = cexpTypeGuessFromSize(s->size);
	}
	nmod->section_syms[nsect_syms] = 0; /* tag end */

#if defined(USE_PMBFD) && defined(USE_LOADER)
	if ( cexpSystemAttributesSize > 0 ) {
		bfd_init();
		nmod->fileAttributes = pmelf_read_attribute_set(cexpSystemAttributes, cexpSystemAttributesSize, 0, "SYSTEM");
	}
#endif

#ifdef HAVE_BFD_DISASSEMBLER
	/* finding our BFD architecture turns out to be non-trivial! */
	{
	bfd *abfd = 0; /* keep compiler happy */
	const bfd_arch_info_type *ai = 0;
	char	      *tn = 0;
	const char **tgts = 0;
#ifndef USE_PMBFD
		/* must open a BFD for the default target */
		if ( !(tn=malloc(L_tmpnam)) || !tmpnam(tn) || !(abfd=bfd_openw(tn,"default")) ) {
			fprintf(stderr,"cexpLoadBuiltinSymtab(): unable to open a dummy BFD for determining disassembler arch\n");
			goto cleanup;
		}
		if ( cexpBuiltinCpuArch ) {
			if ( !(ai=bfd_scan_arch(cexpBuiltinCpuArch)) )
				fprintf(stderr,"cexpLoadBuiltinSymtab(): User supplied CPU Architecture '%s' invalid; trying default...\n",
						cexpBuiltinCpuArch);
		}
		if ( !ai && (tgts=bfd_target_list()) && *tgts && (cexpBuiltinCpuArch = strrchr(*tgts,'-')) ) {
			ai = bfd_scan_arch(++cexpBuiltinCpuArch);
		}
		if ( !ai ) {
			cexpBuiltinCpuArch = 0;
			goto cleanup;
		}

		abfd->arch_info = ai;
#else
		/*
		 * pmbfd can deal with the NULL BFD that the opcode
		 * library passes...
		 */
#endif
		cexpDisassemblerInstall(abfd);

cleanup:
#ifndef USE_PMBFD
		if ( !ai )
			fprintf(stderr,"Unable to determine target CPU architecture -- skipping disassembler installation\n");
#endif
		if ( abfd )
			bfd_close_all_done(abfd);
		if ( tn ) {
			unlink(tn);
			free(tn);
		}
		free(tgts);
	}
#endif
	return 0;
}


CexpModule
cexpModuleLoad(char *filename, char *modulename)
{
CexpModule m,tail,nmod,rval=0;
char       *slash = filename ? strrchr(filename,'/') : 0;

	if (slash)
		slash++;

	if (!modulename)
		modulename=slash ? slash : filename;

	if (!modulename)
		modulename="SYSTEM-BUILTIN";

	if (!(nmod=(CexpModule)malloc(sizeof(*nmod))))
		return 0;

	memset(nmod,0,sizeof(*nmod));

	__WLOCK();

	for (m=cexpSystemModule, tail=0; m; m=m->next) {
		tail=m;
		if (!strcmp(m->name,modulename)) {
			fprintf(stderr,
				"ERROR: a module '%s' exists already\n",
				modulename);
			goto cleanup;
		}
	}

	if (idAlloc(nmod)) {
		fprintf(stderr,
			"Unable to allocate a module ID (more than %i loaded?)\n",
			MAX_NUM_MODULES);
		goto cleanup;
	}

	if (!(nmod->name=(char*)malloc(strlen(modulename)+1))) {
			goto cleanup;
	}
	strcpy(nmod->name,modulename);

	if ( filename ) {
		if (cexpLoadFile(filename,nmod)) {
			goto cleanup;
		}
	} else {
		if (cexpLoadBuiltinSymtab(nmod)) {
			goto cleanup;
		}
	}

	/* add help tables */
	{
	CexpSym found;
	int		max;
	cexp_regex	*rc;
		assert(rc=cexp_regcomp("^"CEXP_HELP_TAB_NAME));
		for (found=0,max=1; (found=_cexpSymTblLookupRegex(rc,&max,found,0,nmod->symtbl)); found++,max=1) {
			cexpAddHelpToSymTab((CexpHelpTab)found->value.ptv, nmod->symtbl);
		}
		cexp_regfree(rc);
	}

#ifdef HAVE_SYS_MMAN_H
	if ( nmod->segs ) {
	CexpSegment s;

	for ( s=nmod->segs; s->name; s++ ) {
		/* make executable */
		if ( s->chunk ) {
			unsigned long nsiz, pgbeg, pgmsk;
			pgmsk  = getpagesize()-1;
			pgbeg  = (unsigned long)s->chunk;
			pgbeg &= ~pgmsk;
			nsiz   = s->size + (unsigned long)s->chunk - pgbeg; 
			nsiz   = (nsiz + pgmsk) & ~pgmsk;
			if ( mprotect((void*)pgbeg, nsiz, PROT_READ | PROT_WRITE | PROT_EXEC) )
				perror("ERROR -- mprotect(PROT_READ|PROT_WRITE|PROT_EXEC)");
		}
	}
	}
#endif

		/* call the constructors */
	{
		int i;
		for (i=0; i<nmod->nCtors; i++) {
			if (nmod->ctor_list[i])
				nmod->ctor_list[i]();
		}
		/* the constructors are not really needed
		 * anymore...
		 */
		free(nmod->ctor_list);
		nmod->ctor_list=0;
		nmod->nCtors=0;
	}

	/* call 'non-C++' constructor */
	if (nmod->iniCallback)
		nmod->iniCallback(nmod);

	addDependencies(nmod);

	/* chain to the list of modules */
	if (tail)
		tail->next=nmod;
	else
		cexpSystemModule=nmod;
	rval=nmod;
	nmod=0;

cleanup:
	__WUNLOCK();

	if (nmod) {
		cexpModuleFree(&nmod);
		return 0;
	}

	return rval;
}

void
cexpModuleFree(CexpModule *mp)
{
CexpModule mod=*mp;
	if (mod) {
		assert( ! mod->next);
		free(mod->name);
#ifdef USE_LOADER
		cexpSegsDelete(mod->segs);
#else
		if ( mod->segs ) {
			fprintf(stderr,"WARNING: we shouldn't get here -- memory leak at %s:%u\n",__FILE__,__LINE__);
		}
#endif
		free(mod->ctor_list);
		free(mod->dtor_list);
		free(mod->section_syms);
		free(mod->fileName);
		cexpFreeSymTbl(&mod->symtbl);
		free(mod);
#ifdef USE_PMBFD
		if (mod->fileAttributes)
			pmelf_destroy_attribute_set(mod->fileAttributes);
#endif
	}
	*mp=mod;
}
