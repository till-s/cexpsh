/* $Id$ */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef TEST_VARS
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include "vars.h"
#include "cexplock.h"

/* Implementation of CEXP variables, currently
 * just a linked list. The number of user generated
 * variables is probably small and performance is
 * not an issue.
 *
 * However, the implementation of the variables / string table
 * is hidden within this file and may easily be changed.
 */

/* use a global lock to keep the lists consistent;
 * note that the locking scheme does _only_ keep the
 * lists consistent. It does currently _not_ protect
 * looked-up variables from becoming stale.
 */

/* TODO: if ever deemed necessary, a multiple read / single write
 *       locking scheme could be implemented...
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

static CexpLock _varlock=0;
#define __LOCK		cexpLock(_varlock)
#define __UNLOCK	cexpUnlock(_varlock)

typedef struct lhR_ {
	struct lhR_	*p;
#ifdef DOUBLE_LINKED
	struct lhR_	*n;
#endif
} lhR, *lh;

static int		  walking=0; /* detect nasty walkers */

static void 
lhrAdd(void *e, void *l)
{
lh el=e, where=l;

#ifdef DOUBLE_LINKED
	assert(el->p==0 && el->n==0);
#else
	assert(el->p==0);
#endif
	__LOCK;
	assert(!walking);
#ifdef DOUBLE_LINKED
	el->n = where;
#endif
	el->p = where->p;
#ifdef DOUBLE_LINKED
	if (where->p) where->p->n=el;
#endif
	where->p  = el;
	__UNLOCK;
}

#ifdef DOUBLE_LINKED
static void
lhrDel(void *a)
{
lh el=a;
	__LOCK;
	assert(!walking);
	if (el->n) el->n->p=el->p;
	if (el->p) el->p->n=el->n;
	el->p=el->n=0;
	__UNLOCK;
}
#endif

typedef int (*LhComp)(lh el, const void *what);

static lh
lhrFindN_LOCK(lh where, const void *what, LhComp comp, lh *succ)
{
lh		p,v;
int		missed=1;
	__LOCK;
		/* walk backwards; the lhrAdd adds elements 'before' the list head in t' */
		for (v=where->p, p=where;
			 v && (missed=comp(v,what))<0;) {
			   	p=v; v=v->p;
		}
	if (succ) *succ=p;
	return (missed ? 0 : v);
}

static void
lhrFlushN_LOCK(lh listel)
{
lh v,p;
	__LOCK;
	for (v=listel; v;) {
		p=v; v=p->p;
		free(p);
	}
}

/* Cexp (static) strings and variable names */
typedef struct CexpStrRec_ {
	lhR				head;
	char			str[1];	/* actual name space is allocated contiguous */
} CexpStrRec, *CexpStr;

/* Cexp variables */
typedef struct CexpVarRec_ {
	lhR					head;
	CexpSymRec			sym;
	CexpValU			val;
} CexpVarRec , *CexpVar;

static CexpVarRec gblList={{(void*)0},{0},{0}};
static CexpStrRec strTab ={{0}};

void
cexpVarInitOnce(void)
{
/* initialize the global lock */
if (!_varlock)
	cexpLockCreate(&_varlock);
}

/* destroy all variables */
void
cexpVarsFlush(void)
{
	lhrFlushN_LOCK(gblList.head.p);
	memset(&gblList.head,0,sizeof(gblList.head));
	__UNLOCK;
}

/*
 * NOTE: there is no corresponding 'cexpStrsFlush()'
 *       because the string table is a 'static' object,
 *       i.e. strings, once allocated, live forever...
 */       

static int
strcomp(lh el, const void *what)
{
const char *name=what;
	return strcmp(&(((CexpStr)el)->str[0]),name);
}

static CexpStr
strFindN_LOCK(const char *chpt, lh *succ)
{
	return  (CexpStr)lhrFindN_LOCK(&strTab.head,chpt,strcomp,succ);
}

static int
varcomp(lh el, const void *what)
{
const char *name=what;
	return strcmp( ((CexpVar)el)->sym.name, name );
}

/* find a variable, return with a held lock */
static CexpVar
findN_LOCK(const char *name, lh *succ)
{
	return (CexpVar)lhrFindN_LOCK(&gblList.head,name,varcomp,succ);
}


/* lookup a variable 
 * If the 'creat' flag is passed, a new variable
 * is created.
 * RETURNS nonzero value if set/create succeeds.
 */

CexpSym
cexpVarLookup(const char *name, int creat)
{
CexpVar v;
lh		where;
CexpVar n;
CexpStr s;

	if (creat) {
		/* (avoid calling malloc from locked section) */
		n=(CexpVar)calloc(1,sizeof(*n));
		s=(CexpStr)calloc(1,sizeof(*s) + strlen(name)+1);
	} else {
		n=0;
		s=0;
	}

	if (!(v=findN_LOCK(name,&where)) && creat && n && s) {
		CexpStr t;
		lh		q;
		/* create string */
		if (!(t=strFindN_LOCK(name,&q))) {
			lhrAdd(s,q);
			strcpy(&s->str[0],name);
			t=s; s=0;
		}
		__UNLOCK;
		/* create variable / add to list */
		lhrAdd(n,where);
		n->sym.value.type=TVoid;
		n->sym.value.ptv=&n->val;
		n->sym.name=&t->str[0];
		n->sym.size=sizeof(n->val);
		n->sym.flags=0;
		v=n; n=0;
	}
	__UNLOCK;
	if (n) free(n);
	if (s) free(s);
	return v ? &v->sym : 0;
}

/* lookup / create a string */

char *
cexpStrLookup(char *name, int creat)
{
CexpStr s,t;
lh		q;

	if (creat) {
		/* (avoid calling malloc from locked section) */
		s=(CexpStr)malloc(sizeof(*s) + strlen(name)+1);
		memset(&s->head,0,sizeof(s->head));
	} else {
		s=0;
	}

	/* create string ? */
	if (!(t=strFindN_LOCK(name,&q)) && creat && s) {
		lhrAdd(s,q);
		strcpy(&s->str[0],name);
		t=s; s=0;
	}
	__UNLOCK;
	if (s) free(s);
	return t ? &t->str[0] : 0;
}


/* remove a variable
 * RETURNS: nonzero if the variable was found & deleted
 */
void *
cexpVarDelete(char *name)
{
CexpVar v;
lh		p;
	if ((v=findN_LOCK(name,&p))) {
#ifdef DOUBLE_LINKED
		lhrDel(v);
#else
		p->p=v->head.p;
#endif
	}
	__UNLOCK;
	/* paranoia to make dangling pointers more likely to crash */
	if (v) {
		memset(v,0,sizeof(*v));
		free(v);
		return (void*)0xdeadbeef;
	}
	return 0;
}

/* NOTE: there is no corresponding routine for deleting
 *       strings because they are intended to live 'forever'
 */

void *
cexpVarWalk(CexpVarWalker walker, void *usrArg)
{
CexpVar v;
void	*rval=0;
	__LOCK;
	walking++;
	for (v=(CexpVar)gblList.head.p; v; v=(CexpVar)v->head.p) {
		if ((rval=walker(v->sym.name, &v->sym, usrArg)))
			break;
	}
	walking--;
	__UNLOCK;
	return rval;
}

#if defined(TEST_VARS) || defined(DEBUG)
void
varPrintList(void)
{
CexpVar v,n=0;
	printf("\nreverse: \n");
	for (v=(CexpVar)gblList.head.p; v; n=v, v=(CexpVar)v->head.p) {
			printf("%10s 0x%8lx ",v->sym.name, v->sym.value.ptv->l);
	}

#ifdef DOUBLE_LINKED
	printf("\n\nforward: \n");
	for (; n && n->head.n; n=(CexpVar)n->head.n) {
			printf("%10s 0x%8lx ",&n->sym.name, n->pv.ptv->l);
	}
#endif
	printf("\n");
}

void
strPrintList(void)
{
CexpStr s;
	printf("string tab:\n");
	for (s=(CexpStr)strTab.head.p; s; s=(CexpStr)s->head.p)
			printf("'%s'\n",&s->str[0]);
}

#define OffsetOf(structpt, field) ((unsigned long)&((structpt)0)->field - (unsigned long)((structpt)0))

#if defined(TEST_VARS)
int
vars_main(int argc, char **argv)
{
char *line=0;
char *name,*v;
unsigned long value,f;
CexpSym val;
int	ch;

  cexpVarInitOnce();

  {
	CexpVar vv;
		  val=cexpVarLookup("hallo",1);
					vv = (CexpVar)((unsigned long)val - OffsetOf(CexpVar,sym));
		  printf("CexpVar size: %i, & 0x%08lx, &name: %08lx\n",
						  sizeof(*vv), (unsigned long)vv, (unsigned long)&vv->sym.name);
  }

  while (free(line),line=readline("Vars Tst>")) {

	for (name=line+1; *name && isspace(*name); name++);
	for (v=name; *v && !isspace(*v); v++);
	if (*v) {
		*v=0;
		for (v++;*v && isspace(*v); v++);
		if (*v) sscanf(v,"%li",&value);
	}
	f=0;

	switch ((ch=toupper(line[0]))) {
			default:  fprintf(stderr,"unknown command\n");
			continue; /* dont add bad commands to history */

			case 'F': cexpVarsFlush();
			break;

			case 'P': varPrintList();
			break;

			case 'T': strPrintList();
			break;

			case 'A': f=1;/* find and add */
			case 'S':     /* find and set (dont add) */
				if (!*name || !*v) {
					fprintf(stderr,"missing name and/or value\n");
					break;
				}
				val=cexpVarLookup(name,f);
				if (val) {
					val->value.type=TULong;
					val->value.ptv->l=value;
				}
			  	printf("\n%s %s\n",
						f?"adding":"setting",
						val?"success":"failure");
			break;

			case 'L':
			case 'D':
				if (!*name) {
						fprintf(stderr,"missing name\n");
						break;
				}
				if ('D'==ch) {
					printf("Deleting %s\n",
							cexpVarDelete(name) ? "success" : "failure");
				} else {
					val=cexpVarLookup(name,0);
					printf("Var %sfound: 0x%lx\n",
							val ? "" : "not ", val->value.ptv->l);
				}
			break;
	}
	add_history(line);
  }
  return 0;
}
#endif
#endif
