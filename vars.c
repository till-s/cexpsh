/* $Id$ */
#include "vars.h"
#include <assert.h>
#include <stdlib.h>
#ifdef TEST_VARS
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* Implementation of CEXP variables, currently
 * just a linked list. The number of user generated
 * variables is probably small and performance is
 * not an issue.
 */

/* use a global lock to keep the lists consistent */
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

static void 
lhrAdd(void *e, void *l)
{
lh el=e, where=l;

	/* this fails most probably due to missing to call
	 * cexpVarInit()
	 */
	assert(el->p==0 && where->n==0);
	__LOCK;
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
findN_LOCK(char *name)
{
CexpVar v;
	__LOCK;
		/* walk backwards; the lhrAdd adds elements 'before' the list head in t' */
		for (v=(CexpVar)gblList.head.p; v && strcmp(v->name,name);)
			   	v=(CexpVar)v->head.p;
	return v;
}


void *
cexpVarLookup(char *name, CexpTypedVal prval)
{
CexpVar v;
	if ((v=findN_LOCK(name))) *prval=v->value;
	__UNLOCK;
	return v;
}


/* lookup a variable and set to 'val'.
 * If the 'creat' flag is passed, a new variable
 * is created and 'val' assigned to its value.
 * RETURNS nonzero value if set/create succeeds.
 */
void *
cexpVarSet(char *name, CexpTypedVal val, int creat)
{
CexpVar v;
CexpVar n=(CexpVar)malloc(sizeof(*n) + strlen(name)+1);
		/* (avoid calling malloc from locked section) */
	n->head.p=n->head.n=0;

	if ((v=findN_LOCK(name))) {
		/* variable found, write its value */
		v->value=*val;
	} else if (creat && n) {
		/* create variable / add to list */
		lhrAdd(n,&gblList.head);
		strcpy(n->name, name);
		n->value=*val;
		v=n; n=0;
	}
	__UNLOCK;
	if (n) free(n);
	return v;
}

/* remove a variable
 * RETURNS: nonzero if the variable was found & deleted
 */
void *
cexpVarDelete(char *name)
{
CexpVar v;
	if ((v=findN_LOCK(name)))
		lhrDel(v);
	__UNLOCK;
	/* paranoia to make dangling pointers more likely to crash */
	if (v) {
		memset(v,0,sizeof(*v));
		return (void*)0xdeadbeef;
	}
	return 0;
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
CexpTypedValRec v;

  cexpVarInit();

  {
		  CexpVar v=cexpVarSet("hallo",0xdeadbeef,1);
		  printf("CexpVar size: %i, & 0x%08x, &name: %08x\n",
						  sizeof(*v), v, v->name);
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
				v.type=TULong;
				v.tv.l=value;
				i=cexpVarSet(name,&v,f);
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
					v.tv.l=0xdeadbeef;
					i=cexpVarLookup(name,&v);
					printf("Var %sfound: 0x%x\n",
							i ? "" : "not ", v.tv.l);
				}
			break;
	}
	add_history(line);
  }
}
#endif
