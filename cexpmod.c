/* $Id$ */

/* Implementation of cexp modules */

/*
 * Copyright 2002, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */

#include <stdio.h>
#include <assert.h>
#include <cexp_regex.h>
#include <stdlib.h>
#include <string.h>

#include "cexpmodP.h"
#include "cexpsymsP.h"
#include "cexplock.h"

CexpModule cexpSystemModule=0;

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
		if ( addr < (void*)t->aindex[0]->value.ptv  ||
		     addr > (void*)t->aindex[t->nentries-1]->value.ptv )
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
					if (f) fprintf(f,"=====  In module '%s' (0x%08x) =====:\n",m->name, (unsigned)m);
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

int
cexpModuleInfo(CexpModule mod, FILE *f)
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
		fprintf(f,"Module '%s' (0x%08x):\n",
						m->name, (unsigned)m);
		fprintf(f,"  %li symbol table entries\n",
						m->symtbl->nentries);
		fprintf(f,"  %li bytes of memory allocated to binary\n",
						m->memSize);
		fprintf(f,"  Text starts at: 0x%08x\n",
						(unsigned)m->text_vma);
		fprintf(f,"  Needs:"); bitmapInfo(f,m->needs); fputc('\n',f);
		fprintf(f,"  Needed by:"); bitmapInfo(f,m->neededby); fputc('\n',f);
		if (mod)
			break; /* info only for the particular module requested */
	}

	__RUNLOCK();

	return 0;
}

CexpModule
cexpModuleFindByName(char *needle, FILE *f)
{
cexp_regex	*rc=0;
CexpModule	m,found=0;

	if (!f)
		f=stdout;

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
			fprintf(f,"0x%08x: %s\n",(unsigned)m, m->name);
		}
	}

	__RUNLOCK();

	cexp_regfree(rc);
	return found;
}

int
cexpModuleUnload(CexpModule mod)
{
int			i;
CexpModule	pred,m;

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
			"Unable to allocate a module ID (more than %i loaded?)\n",
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
		free(mod->memSeg);
		free(mod->ctor_list);
		free(mod->dtor_list);
		cexpFreeSymTbl(&mod->symtbl);
		free(mod);
	}
	*mp=mod;
}
