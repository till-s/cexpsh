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

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

/* License: GPL, http://www.gnu.org */

/* Implementation of CEXP variables, currently
 * just a linked list. The number of user generated
 * variables is probably small and performance is
 * not an issue.
 *
 * However, the implementation of the variables / string table
 * is hidden within this file and may easily be changed.
 */

/* use a global lock to keep the lists consistent */

/* TODO: if ever deemed necessary, a multiple read / single write
 *       locking scheme could be implemented...
 */
#ifdef __rtems
#include <rtems.h>
static rtems_id _varlock;
#define __LOCK		rtems_semaphore_obtain(_varlock,RTEMS_WAIT,RTEMS_NO_TIMEOUT)
#define __UNLOCK	rtems_semaphore_release(_varlock)
/* IMPORTANT: use a standard (not simple) binary semaphore that may nest */
static inline void
 __LCKINI(void)
{
	rtems_semaphore_create(
		rtems_build_name('c','x','l','k'),
		1,/*initial count*/
		RTEMS_FIFO|RTEMS_BINARY_SEMAPHORE,
		0,
		&_varlock);
}
#elif defined(NO_THREAD_PROTECTION)
#define __LOCK
#define __UNLOCK
#define __LCKINI() do {} while(0)
#else
#error "thread protection not implemented for this target system"
#endif

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

typedef int (*LhComp)(lh el, void *what);

static lh
lhrFindN_LOCK(lh where, void *what, LhComp comp, lh *succ)
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
	char			str[0];	/* name space is allocated contiguous */
} CexpStrRec, *CexpStr;

/* Cexp variables */
typedef struct CexpVarRec_ {
	lhR					head;
	CexpTypedAddrRec	pv;
	CexpValU			val;
	CexpStr				name;
} CexpVarRec , *CexpVar;

/* initialize the list anchor's 'n' field to 1 
 * to indicate that the 'library' has not been
 * initialized...
 */
static CexpVarRec gblList={{(void*)0},{0},{0},(CexpStr)1};
static CexpStrRec strTab ={{0}};

void
cexpVarInit(void)
{
/* initialize the global lock */
__LCKINI();
__LOCK;
	/* use gblList.name as an indicator for the
	 * very first call...
	 */
	if (gblList.name)
		memset(&gblList,0,sizeof(gblList));
__UNLOCK;
}

/* destroy all variables */
void
cexpVarsFlush(void)
{
	lhrFlushN_LOCK(gblList.head.p);
		gblList.head=(lhR){0};
	__UNLOCK;
}

/*
 * NOTE: there is no corresponding 'cexpStrsFlush()'
 *       because the string table is a 'static' object,
 *       i.e. strings, once allocated, live forever...
 */       

static int
strcomp(lh el, void *what)
{
char *name=what;
	return strcmp(&(((CexpStr)el)->str[0]),name);
}

static CexpStr
strFindN_LOCK(char *chpt, lh *succ)
{
	return  (CexpStr)lhrFindN_LOCK(&strTab.head,chpt,strcomp,succ);
}

static int
varcomp(lh el, void *what)
{
char *name=what;
	return strcmp(&(((CexpVar)el)->name->str[0]),name);
}

/* find a variable, return with a held lock */
static CexpVar
findN_LOCK(char *name, lh *succ)
{
	return (CexpVar)lhrFindN_LOCK(&gblList.head,name,varcomp,succ);
}


/* lookup a variable 
 * If the 'creat' flag is passed, a new variable
 * is created.
 * RETURNS nonzero value if set/create succeeds.
 */

CexpTypedAddr
cexpVarLookup(char *name, int creat)
{
CexpVar v,where;
CexpVar n;
CexpStr s;

	if (creat) {
		/* (avoid calling malloc from locked section) */
		n=(CexpVar)malloc(sizeof(*n));
		s=(CexpStr)malloc(sizeof(*s) + strlen(name)+1);
		n->head=(lhR){0};
		s->head=(lhR){0};
	} else {
		n=0;
		s=0;
	}

	if (!(v=findN_LOCK(name,(lh*)&where)) && creat && n && s) {
		CexpStr t,q;
		/* create string */
		if (!(t=strFindN_LOCK(name,(lh*)&q))) {
			lhrAdd(s,(lh)q);
			strcpy(&s->str[0],name);
			t=s; s=0;
		}
		__UNLOCK;
		/* create variable / add to list */
		lhrAdd(n,(lh)where);
		n->name=t;
		n->pv.type=TVoid;
		n->pv.ptv=&n->val;
		v=n; n=0;
	}
	__UNLOCK;
	if (n) free(n);
	if (s) free(s);
	return v ? &v->pv : 0;
}

/* lookup / create a string */

char *
cexpStrLookup(char *name, int creat)
{
CexpStr s,t,q;

	if (creat) {
		/* (avoid calling malloc from locked section) */
		s=(CexpStr)malloc(sizeof(*s) + strlen(name)+1);
		s->head=(lhR){0};
	} else {
		s=0;
	}

	/* create string ? */
	if (!(t=strFindN_LOCK(name,(lh*)&q)) && creat && s) {
		lhrAdd(s,(lh)q);
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
CexpVar v,p;
	if ((v=findN_LOCK(name,(lh*)&p))) {
#ifdef DOUBLE_LINKED
		lhrDel(v);
#else
		p->head.p=v->head.p;
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
		if ((rval=walker(&v->name->str[0], &v->pv, usrArg)))
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
			printf("%10s 0x%8lx ",&v->name->str[0], v->pv.ptv->l);
	}

#ifdef DOUBLE_LINKED
	printf("\n\nforward: \n");
	for (; n && n->head.n; n=(CexpVar)n->head.n) {
			printf("%10s 0x%8lx ",&n->name->str[0], n->pv.ptv->l);
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
CexpTypedVal val;
int	ch;

  cexpVarInit();

  {
	CexpVar vv;
		  val=cexpVarLookup("hallo",1);
			vv = (CexpVar)((unsigned long)val - OffsetOf(CexpVar,value));
		  printf("CexpVar size: %i, & 0x%08lx, &name: %08lx\n",
						  sizeof(*vv), (unsigned long)vv, (unsigned long)&vv->name->str[0]);
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
					val->type=TULong;
					val->tv.l=value;
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
							val ? "" : "not ", val->tv.l);
				}
			break;
	}
	add_history(line);
  }
  return 0;
}
#endif
#endif
