/* $Id$ */

/* Implementation of cexp modules */
#include <stdio.h>
#include <assert.h>
#include <regexp.h>

#include "cexpmodP.h"
#include "cexpsymsP.h"

CexpModule cexpSystemModule=0;

#ifdef NO_THREAD_PROTECTION
#define __RLOCK() do {} while(0)
#define __RUNLOCK() do {} while(0)
#define __WLOCK() do {} while(0)
#define __WUNLOCK() do {} while(0)
#endif

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

/* search for an address in all modules */
CexpSym
cexpSymLkAddr(void *addr, int margin, FILE *f, CexpModule *pmod)
{
CexpModule	m;
CexpSym		rval=0;
CexpSymTbl	t;

	__RLOCK();

	for (m=cexpSystemModule; m; m=m->next) {
		t=m->symtbl;
		if ( addr < (void*)t->aindex[0]->value.ptv  ||
		     addr > (void*)t->aindex[t->nentries-1]->value.ptv )
			continue;
		if (f)
			fprintf(f,"=====  In module '%s' =====:\n",m->name);
		if ((rval=cexpSymTblLkAddr(addr,margin,f,t)))
			break;
	}
	if (pmod)
		*pmod=m;

	__RUNLOCK();

	return rval;
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
_cexpSymLookupRegex(regexp *rc, int max, CexpSym s, FILE *f, CexpModule *pmod)
{
CexpModule	m,mfound;

    if (max<1)  max=25;
    if (!f)     f=stdout;

	if (pmod)	{
		/* start at module/symbol passed in */
		m=*pmod;
	} else  {
		s=0;
		m=0;
	}
	if (!m)	m=cexpSystemModule;
    
	__RLOCK();

	for (; m; m=m->next) {
    	if (!s) s=m->symtbl->syms;
		mfound=0;
		while (s->name && max) {
      		if (regexec(rc,s->name)) {
				if (!mfound) {
					fprintf(f,"=====  In module '%s' (0x%08x) =====:\n",m->name, m);
					mfound=m; /* print module name only once */
				}
				cexpSymPrintInfo(s,f);
				if (0==--max) {
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

int
cexpModuleInfo(CexpModule mod, FILE *f)
{
CexpModule m;


	if (!f) f=stdout;

	__RLOCK();

	m=mod ? mod : cexpSystemModule;
	for (m=cexpSystemModule; m; m=m->next) {
		fprintf(f,"Module '%s' (0x%08x):\n",m->name,m);
		fprintf(f,"  %i symbol table entries\n",m->symtbl->nentries);
		fprintf(f,"  %i bytes of memory allocated to binary\n",m->memSize);
		fprintf(f,"  Text starts at: 0x%08x\n",m->text_vma);
		fprintf(f,"  Needs:"); bitmapInfo(f,m->needs); fputc('\n',f);
		fprintf(f,"  Needed by:"); bitmapInfo(f,m->neededby); fputc('\n',f);
		if (mod)
			break; /* info only for the particular module requested */
	}

	__RUNLOCK();

	return 0;
}

int
cexpModuleUnload(CexpModule mod)
{
int			i;
CexpModule	pred,m;

	__WLOCK();

	/* is mod in the list at all ? */
	for (pred=cexpSystemModule; pred && mod!=pred->next; pred=pred->next)
		/* nothing else to do */;

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

	memset(mod->memSeg, 0, mod->memSize);

	/* could flush the caches here */

	cexpModuleFree(&mod);

	return 0;

cleanup:
	__WUNLOCK();
	return -1;
}


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
BitmapWord  freebits;
CexpModule	m;

	memset(used,0,sizeof(used));

	/* mark all used ids */
	for (m=cexpSystemModule; m; m=m->next) {
			BITMAP_SET(used,m->id);
	}

	new_id = id;
	do {
		new_id = ++new_id % MAX_NUM_MODULES;
		if (new_id == id)
			return -1;
	} while (BITMAP_TST(used,new_id));

	mod->id=id=new_id;

	return 0;
}


CexpModule
cexpModuleLoad(char *filename, char *modulename)
{
CexpModule m,tail,nmod,rval=0;

	if (!modulename)
		modulename=filename;

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
			"Unable to allocate a module ID (more than %i loaded??)\n",
			MAX_NUM_MODULES);
		goto cleanup;
	}

	if (!(nmod->name=(char*)malloc(strlen(modulename)+1))) {
			goto cleanup;
	}
	strcpy(nmod->name,modulename);

	if (cexpLoadFile(filename,nmod)) {
		goto cleanup;
	}

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
		free(mod->memSeg);
		free(mod->ctor_list);
		free(mod->dtor_list);
		cexpFreeSymTbl(&mod->symtbl);
		free(mod);
	}
	*mp=mod;
}
