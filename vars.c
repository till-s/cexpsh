/* $Id$ */
#include "vars.h"
#include <assert.h>
#include <stdlib.h>
#ifdef TEST_VARS
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

/* License: GPL, http://www.gnu.org */

/* Implementation of CEXP variables, currently
 * just a linked list. The number of user generated
 * variables is probably small and performance is
 * not an issue.
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
	struct lhR_	*n, *p;
} lhR, *lh;

static int		  walking=0; /* detect nasty walkers */

static void 
lhrAdd(void *e, void *l)
{
lh el=e, where=l;

	assert(el->p==0 && el->n==0);
	__LOCK;
	assert(!walking);
	el->n = where;
	el->p = where->p;
	if (where->p) where->p->n=el;
	where->p  = el;
	__UNLOCK;
}

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

typedef struct CexpVarRec_ {
	lhR				head;
	CexpTypedValRec	value;
	char			name[0];	/* name space is allocated contiguous */
} CexpVarRec , *CexpVar;

/* initialize the list anchor's 'n' field to 1 
 * to indicate that the 'library' has not been
 * initialized...
 */
static CexpVarRec gblList={{(void*)1},};

void
cexpVarInit(void)
{
/* initialize the global lock */
__LCKINI();
__LOCK;
	/* use gblList.vars.head as an indicator for the
	 * very first call...
	 */
	if (gblList.head.n)
		memset(&gblList,0,sizeof(gblList));
__UNLOCK;
}

/* destroy all variables */
void
cexpVarsFlush(void)
{
lh v,p;
	__LOCK;
	for (v=gblList.head.p; v;) {
		p=v; v=p->p;
		free(p);
	}
	gblList.head.p=gblList.head.n=0;
	__UNLOCK;
}

/* find a variable, return with a held lock */
static CexpVar
findN_LOCK(char *name, lh *succ)
{
lh		p,v;
int		missed=1;
	__LOCK;
		/* walk backwards; the lhrAdd adds elements 'before' the list head in t' */
		for (v=gblList.head.p, p=&gblList.head;
			 v && (missed=strcmp(((CexpVar)v)->name,name))<0;) {
			   	p=v; v=v->p;
		}
	if (succ) *succ=p;
	return (CexpVar)(missed ? 0 : v);
}


/* lookup a variable 
 * If the 'creat' flag is passed, a new variable
 * is created.
 * RETURNS nonzero value if set/create succeeds.
 *         *creat is set to a nonzero value if
 *         the variable was created as a result
 *         of this call;
 */

CexpTypedVal
cexpVarLookup(char *name, int creat)
{
CexpVar v,where;
CexpVar n=(CexpVar)malloc(sizeof(*n) + strlen(name)+1);
		/* (avoid calling malloc from locked section) */
	n->head.p=n->head.n=0;

	if (!(v=findN_LOCK(name,(lh*)&where)) && creat && n) {
		/* create variable / add to list */
		lhrAdd(n,(lh)where);
		strcpy(n->name, name);
		n->value.type=TVoid;
		n->value.tv.p=0;
		v=n; n=0;
	}
	__UNLOCK;
	if (n) free(n);
	return v ? &v->value : 0;
}

/* remove a variable
 * RETURNS: nonzero if the variable was found & deleted
 */
void *
cexpVarDelete(char *name)
{
CexpVar v;
	if ((v=findN_LOCK(name,0)))
		lhrDel(v);
	__UNLOCK;
	/* paranoia to make dangling pointers more likely to crash */
	if (v) {
		memset(v,0,sizeof(*v));
		return (void*)0xdeadbeef;
	}
	return 0;
}

void *
cexpVarWalk(CexpVarWalker walker, void *usrArg)
{
CexpVar v;
void	*rval=0;
	__LOCK;
	walking++;
	for (v=(CexpVar)gblList.head.p; v; v=(CexpVar)v->head.p) {
		if ((rval=walker(v->name, &v->value, usrArg)))
			break;
	}
	walking--;
	__UNLOCK;
	return rval;
}

#ifdef TEST_VARS
static void
varPrintList(void)
{
CexpVar v,n=(CexpVar)gblList.head.n;
	printf("\nreverse: \n");
	for (v=(CexpVar)gblList.head.p; v; n=v, v=(CexpVar)v->head.p) {
			printf("%10s 0x%8x ",v->name, v->value.tv.l);
	}
	printf("\n\nforward: \n");
	for (; n && n->head.n; n=(CexpVar)n->head.n) {
			printf("%10s 0x%8x ",n->name, n->value.tv.l);
	}
	printf("\n");
}

int
main(int argc, char **argv)
{
char *line=0;
char *name,*v;
unsigned long value,f;
void *i;
CexpTypedValRec val;

  cexpVarInit();

  {
	CexpVar vv;
		  val.type=TULong; val.tv.l=0xdeadbeef;
		  vv=cexpVarSet("hallo",&val,1);
		  printf("CexpVar size: %i, & 0x%08x, &name: %08x\n",
						  sizeof(*vv), vv, vv->name);
  }

  while (free(line),line=readline("Vars Tst>")) {

	for (name=line+1; *name && isspace(*name); name++);
	for (v=name; *v && !isspace(*v); v++);
	if (*v) {
		*v=0;
		for (v++;*v && isspace(*v); v++);
		if (*v) sscanf(v,"%i",&value);
	}
	f=0;

	switch (toupper(line[0])) {
			default:  fprintf(stderr,"unknown command\n");
			continue; /* dont add bad commands to history */

			case 'F': cexpVarsFlush();
			break;

			case 'P': varPrintList();
			break;

			case 'A': f=1;/* find and add */
			case 'S':     /* find and set (dont add) */
				if (!*name || !*v) {
					fprintf(stderr,"missing name and/or value\n");
					break;
				}
				val.type=TULong;
				val.tv.l=value;
				i=cexpVarSet(name,&val,f);
			  	printf("\n%s %s\n",
						f?"adding":"setting",
						i?"success":"failure");
			break;

			case 'L':
			case 'D':
				if (!*name) {
						fprintf(stderr,"missing name\n");
						break;
				}
				if ('D'==line[0]) {
					printf("Deleting %s\n",
							cexpVarDelete(name) ? "success" : "failure");
				} else {
					val.tv.l=0xdeadbeef;
					i=cexpVarLookup(name,&val);
					printf("Var %sfound: 0x%x\n",
							i ? "" : "not ", val.tv.l);
				}
			break;
	}
	add_history(line);
  }
}
#endif
