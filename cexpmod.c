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
					fprintf(f,"=====  In module '%s' (id 0x%08x) =====:\n",m->name, m->id);
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

int
cexpModuleInfo(FILE *f)
{
CexpModule m;


	if (!f) f=stdout;

	__RLOCK();

	for (m=cexpSystemModule; m; m=m->next) {
		fprintf(f,"Module '%s':\n",m->name);
		fprintf(f,"  %i symbol table entries\n",m->symtbl->nentries);
		fprintf(f,"  %i bytes of memory allocated to binary\n",m->memSize);
	}

	__RUNLOCK();

	return 0;
}

int
cexpModuleLoad(char *filename, char *modulename)
{
CexpModule m,tail,nmod;

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

	if (!(nmod->name=(char*)malloc(strlen(modulename)+1))) {
			goto cleanup;
	}
	strcpy(nmod->name,modulename);

	if (cexpLoadFile(filename,nmod)) {
		goto cleanup;
	}

	/* chain to the list of modules */
	if (tail)
		tail->next=nmod;
	else
		cexpSystemModule=nmod;
	nmod=0;

cleanup:
	__WUNLOCK();

	if (nmod) {
		cexpModuleFree(&nmod);
		return -1;
	}

	return 0;
}

void
cexpModuleFree(CexpModule *mp)
{
CexpModule mod=*mp;
	if (mod) {
		assert( ! mod->next);
		free(mod->name);
		free(mod->memSeg);
		cexpFreeSymTbl(&mod->symtbl);
		free(mod);
	}
	*mp=mod;
}
