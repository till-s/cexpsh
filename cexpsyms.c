
/* generic symbol table handling */

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
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cexp_regex.h>

#include "cexpsymsP.h"
#include "cexpmod.h"
#include "vars.h"
/* NOTE: DONT EDIT 'cexp.tab.h'; it is automatically generated by 'bison' */
#include "cexp.tab.h"

#ifndef LINKER_VERSION_SEPARATOR
#define LINKER_VERSION_SEPARATOR '@'
#endif

/* compare the name of two symbols 
 * (in a not very fancy way)
 * The clue is to handle the symbol
 * versioning in an appropriate way...
 */
int
_cexp_namecomp(const void *key, const void *cmp)
{
CexpSym				sa=(CexpSym)key;
CexpSym				sb=(CexpSym)cmp;
#if	LINKER_VERSION_SEPARATOR
register const char	*k=sa->name, *c=sb->name;

	while (*k) {
		register int	rval;
		if ((rval=*k++-*c++))
			return rval; /* this also handles the case where *c=='\0' */
		
	}
	/* key string is exhausted */
	return !*c || LINKER_VERSION_SEPARATOR==*c ? 0 : -1;
#else
	return strcmp(sa->name, sb->name);
#endif
}

/* compare the 'values' of two symbols, i.e. the addresses
 * they represent.
 */
int
_cexp_addrcomp(const void *a, const void *b)
{
	CexpSym *sa=(CexpSym*)a;
	CexpSym *sb=(CexpSym*)b;
	/* pointers may be larger than 'int' ! */
	if ( (*sa)->value.ptv > (*sb)->value.ptv )
		return 1;
	else if ( (*sa)->value.ptv < (*sb)->value.ptv )
		return -1;
	else
		return 0;
}

CexpSym
cexpSymTblLookup(const char *name, CexpSymTbl t)
{
CexpSymRec key;
	key.name = name;
	return (CexpSym)bsearch((void*)&key,
				t->syms,
				t->nentries,
				sizeof(*t->syms),
				_cexp_namecomp);
}

/* a semi-public routine which takes a precompiled regexp.
 * The reason this is not public is that we try to keep
 * the public from having to deal with/know about the regexp
 * implementation, i.e. which variant, which headers etc.
 */
CexpSym
_cexpSymTblLookupRegex(cexp_regex *rc, int *pmax, CexpSym s, FILE *f, CexpSymTbl t)
{
CexpSym	found=0;
int		max=24;

	if (!pmax)
		pmax=&max;

	if (!s)		s=t->syms;

	while (s->name && *pmax) {
		if (cexp_regexec(rc,s->name)) {
			if (f) cexpSymPrintInfo(s,f);
			(*pmax)--;
			found=s;
		}
		s++;
	}

	return s->name ? found : 0;
}

CexpSym
cexpSymTblLookupRegex(char *re, int *pmax, CexpSym s, FILE *f, CexpSymTbl t)
{
CexpSym		found;
cexp_regex	*rc;
int			max=24;

	if (!pmax)
		pmax=&max;

	if (!(rc=cexp_regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return 0;
	}
	found=_cexpSymTblLookupRegex(rc,pmax,s,f,t);

	if (rc) cexp_regfree(rc);
	return found;
}

CexpSymTbl
cexpNewSymTbl(unsigned n_entries)
{
CexpSymTbl rval;

	if ( ! (rval = calloc(1, sizeof(*rval))) )
		return 0;

	if ( n_entries ) {
		if ( ! (rval->syms = malloc(sizeof(rval->syms[0])*(n_entries + 1))) )
			goto cleanup;
	}

	rval->size = n_entries;

	return rval;
	
cleanup:
	if ( rval ) {
		free(rval->syms);
	}
	return 0;
}

void
cexpSortSymTbl(CexpSymTbl stbl)
{
int fr,to;
const char *old;

	if ( 0 == stbl->nentries )
		return;

	qsort((void*)stbl->syms,
		stbl->nentries,
		sizeof(*stbl->syms),
		_cexp_namecomp);

	/* Sometimes the same symbol is present in an executable's symbol table AND 
	 * dynamic symbol table. Eliminate redundant entries simply by wasting memory...
	 */

	/* Add a marker to the end of the table (using the extra element there)
	 * to make sure the while loop breaks.
	 */
	old = stbl->syms[stbl->nentries].name;
	/* Use an invalid symbol as a marker */
	stbl->syms[stbl->nentries].name = "+%2W";

	for ( fr=1, to=0; fr<stbl->nentries; fr++ ) {
		while ( 0 == _cexp_namecomp( &stbl->syms[to], &stbl->syms[fr] ) ) {
#ifdef DEBUG /* There seem to be redundant symbols (plt and executable); don't warn for now */
			if ( _cexp_addrcomp( &stbl->syms[to], &stbl->syms[fr] ) ) {
				fprintf(stderr,"WARNING: Eliminating redundant symbol '%s' but values differ (%p vs. %p [eliminated])\n",
				               stbl->syms[to].name,
				               stbl->syms[to].value.ptv,
				               stbl->syms[fr].value.ptv);
			}
#endif
			fr++;
		}
		stbl->syms[++to] = stbl->syms[fr];
	}

	stbl->syms[stbl->nentries].name = old;

	stbl->nentries = to + 1;
}


CexpSymTbl
cexpAddSymTbl(CexpSymTbl stbl, void *syms, int symSize, int nsyms, CexpSymFilterProc filter, CexpSymAssignProc assign, void *closure, unsigned flags)
{
char		*sp,*dst;
const char	*symname;
CexpSymTbl	rval;
CexpSym		cesp;
int			n,nDstSyms,nDstChars;
CexpStrTbl  strtbl = 0;

	if ( stbl ) {
		rval = stbl;
	} else {
		if (! (rval=(CexpSymTbl)calloc(1, sizeof(*rval))))
			return 0;
	}

	if ( filter && assign ) {

		if ( rval->syms && 0 == rval->size ) {
			/* cannot add to a table that we didn't create */
			return 0;
		}

		/* count the number of valid symbols */
		if ( (flags & CEXP_SYMTBL_FLAG_NO_STRCPY) ) {
			for (sp=syms,n=0,nDstSyms=0; n<nsyms; sp+=symSize,n++) {
				if ((symname=filter(sp,closure))) {
					nDstSyms++;
				}
			}
			nDstChars = 0;
		} else {
			for (sp=syms,n=0,nDstSyms=0,nDstChars=0; n<nsyms; sp+=symSize,n++) {
				if ((symname=filter(sp,closure))) {
					nDstChars+=strlen(symname)+1;
					nDstSyms++;
				}
			}
		}

		/* create our copy of the symbol table - the object format contains
		 * many things we're not interested in and also, it's not
		 * sorted...
		 */

		if ( nDstSyms > rval->size - rval->nentries ) {
			/* allocate all the table space */
			if ( ! (rval->syms=(CexpSym)realloc(rval->syms, sizeof(rval->syms[0])*(rval->nentries + nDstSyms + 1))) )
				goto cleanup;

			rval->size = rval->nentries + nDstSyms;
		}

		if ( nDstChars ) {
			if ( !(strtbl = malloc(sizeof(*strtbl))) )
				goto cleanup;
			if ( !(strtbl->chars = malloc(nDstChars)) ) {
				free( strtbl );
				strtbl = 0;
				goto cleanup;
			}
			strtbl->next = rval->strtbl;
			rval->strtbl = strtbl;
		}
	
		/* now copy the relevant stuff */
		dst = strtbl ? strtbl->chars : 0;
		for (sp=syms,n=0,cesp=rval->syms + rval->nentries; n<nsyms; sp+=symSize,n++) {
			if ((symname=filter(sp,closure))) {
					memset(cesp,0,sizeof(*cesp));
					if ( (flags & CEXP_SYMTBL_FLAG_NO_STRCPY) ) {
						cesp->name=symname;
					} else {
						/* copy the name to the string table and put a pointer
						 * into the symbol table.
						 */
						cesp->name=dst;
						while ((*(dst++)=*(symname++)))
							/* do nothing else */;
					}
					cesp->flags = 0;
	
					assign(sp,cesp,closure);
	
				
					cesp++;
			}
		}
		/* mark the last table entry */
		cesp->name=0;
	} else { /* no filter or assign callback -- they pass us a list of symbols in already */
		if ( rval->syms ) {
			/* cannot add an existing list of symbols to another */
			return 0;
		}
		nDstSyms   = nsyms;
		rval->syms = syms;
	}

	rval->nentries += nDstSyms;

	return rval;

cleanup:
	if ( stbl ) {
		/* leave old table alone */
	} else {
		cexpFreeSymTbl(&rval);
	}
	return 0;
}

CexpSymTbl
cexpCreateSymTbl(void *syms, int symSize, int nsyms, CexpSymFilterProc filter, CexpSymAssignProc assign, void *closure)
{
CexpSymTbl rval;

	if ( (rval = cexpAddSymTbl( 0, syms, symSize, nsyms, filter, assign, closure, 0)) ) {
		cexpSortSymTbl( rval );
	}
	return rval;
}

/* (Re-) Build sorted index of addresses */
int
cexpIndexSymTbl(CexpSymTbl t)
{
int i;

	t->aindex = (CexpSym*)realloc(t->aindex, t->nentries * sizeof(*t->aindex));

	if ( t->nentries && ! t->aindex )
		return -1;

	for ( i = 0; i < t->nentries; i++ ) {
		t->aindex[i] = &t->syms[i];
	}
	qsort((void*)t->aindex,
		t->nentries,
		sizeof(*t->aindex),
		_cexp_addrcomp);

	return 0;
}

char *
cexpSymGetHelp(CexpSym s)
{
	if ( ! s )
		return 0;

	return (s->flags & CEXP_SYMFLG_HAS_XTRA) ? s->xtra.info->help : s->xtra.help;
}

void
cexpSymSetHelp(CexpSym s, char *h, int isMalloced)
{
	if ( (s->flags & CEXP_SYMFLG_MALLOC_HELP) ) {
		free( cexpSymGetHelp( s ) );
	}
	if ( (s->flags & CEXP_SYMFLG_HAS_XTRA) ) {
		s->xtra.info->help = h;
	} else {
		s->xtra.help = h;
	}
	if ( isMalloced )
		s->flags |=  CEXP_SYMFLG_MALLOC_HELP;
	else
		s->flags &= ~CEXP_SYMFLG_MALLOC_HELP;
}

void
cexpFreeSymTbl(CexpSymTbl *pt)
{
CexpSymTbl	st=*pt;
CexpSym		s;
CexpStrTbl  strs;
int			i;
	if (st) {
		/* release help info */
		for (s=st->syms, i=0;  i<st->nentries; i++,s++) {
			if (s->flags & CEXP_SYMFLG_MALLOC_HELP) {
				free( cexpSymGetHelp( s ) );
			}
			if ( (s->flags & CEXP_SYMFLG_HAS_XTRA) ) {
				free( s->xtra.info );
			}
		}
		free(st->syms);
		while ( (strs = st->strtbl) ) {
			st->strtbl = strs->next;
			free(strs->chars);
			free(strs);
		}
		free(st->aindex);
		free(st);
	}
	*pt=0;
}

int
cexpSymPrintInfo(CexpSym s, FILE *f)
{
int			i=0,k;
CexpType	t=s->value.type;

	if (!f) f=stdout;

	i+=fprintf(f,"%10p[%6d]: ",
			(void*)s->value.ptv,
			s->size);
	if (!CEXP_TYPE_FUNQ(t)) {
		i+=cexpTAPrintInfo(&s->value, f);
	} else {
		for (k=0; k<30; k++)
			fputc(' ',f);
		i+=k;
		i+=fprintf(f,"%s",cexpTypeInfoString(t));
	}
	while (i++<50)
		fputc(' ',f);
	i+=fprintf(f,"%s\n", s->name);
	return i;
}

/* do a binary search for an address returning its aindex number;
 * if multiple entries exist, return the highest index.
 */
int
cexpSymTblLkAddrIdx(void *addr, int margin, FILE *f, CexpSymTbl t)
{
int			lo,hi,mid;

	lo=0; hi=t->nentries-1;
		
	while (lo < hi) {
		mid=(lo+hi+1)>>1; /* round up */
		if (addr < (void*)t->aindex[mid]->value.ptv)
			hi = mid-1;
		else
			lo = mid;
	}
	
	mid=lo;

	if (f) {
		lo=mid-margin; if (lo<0) 		 	lo=0;
		hi=mid+margin; if (hi>=t->nentries)	hi=t->nentries-1;
		while (lo<=hi)
			cexpSymPrintInfo(t->aindex[lo++],f);
	}
	return mid;
}

CexpSym
cexpSymTblLkAddr(void *addr, int margin, FILE *f, CexpSymTbl t)
{
	return t->aindex[cexpSymTblLkAddrIdx(addr,margin,f,t)];
}

/* currently, we have only very rudimentary support; just enough
 * for 'HELP'
 */
static char *
symHelp(CexpTypedVal returnVal, CexpSym sym, va_list ap)
{
CexpTypedVal v;
char *newhelp=0;
int  verbose=0;

	returnVal->type=TUCharP;
	returnVal->tv.p=cexpSymGetHelp(sym);

	if ((v=va_arg(ap,CexpTypedVal))) {
		switch (v->type) {
			case TUCharP:
				newhelp = v->tv.p;
				break;
			case TUInt:
				verbose = v->tv.i;
				break;
			case TULong:
				verbose = v->tv.l;
				break;
			default:
				return "Cexp Help: Warning, invalid argument";

		}
	}
	
	if (newhelp) {
		if (sym->flags & CEXP_SYMFLG_MALLOC_HELP)
			free( cexpSymGetHelp(sym) );
#if defined(CONFIG_STRINGS_LIVE_FOREVER) && 0 /* might come from another module; we better make a copy */
		/* the help storage is probably an 'eternal' string */
		cexpSymSetHelp( sym, newhelp, 0 );
#else
		cexpSymSetHelp( sym, strdup( newhelp ), CEXP_SYMFLG_MALLOC_HELP );
#endif
	} else {
		char *help;
		if (verbose || ! (help = cexpSymGetHelp(sym))) {
			CexpSym		s;
			CexpModule	m;
			if ((s=cexpSymLkAddr(sym->value.ptv,0,0,&m)) &&
				 s==sym && m) {
				fprintf(stdout,"In module %s:\n",cexpModuleName(m));
			} else if ((s=cexpVarLookup(sym->name,0)) &&
						s==sym) {
				fprintf(stdout,"User Variable:\n");
			}
			cexpSymPrintInfo(sym,stdout);
		}
		if (help)
			fprintf(stdout,"%s\n",help);
		else
			fprintf(stdout,"No help available\n");
	}
	return 0;
}

char *
cexpSymMember(CexpTypedVal returnVal, CexpSym sym, char *mname, ...)
{
char 	*rval="member not implemented";
va_list ap;

	va_start(ap, mname);

	if (!strcmp("help",mname)) {
		rval=symHelp(returnVal, sym, ap);
	}

	va_end(ap);
	return rval;
}

const char *
cexpSymName(CexpSym s)
{
	return s ? s->name : 0;
}

void *
cexpSymValue(CexpSym s)
{
	return s ? (void*)s->value.ptv : 0;
}
