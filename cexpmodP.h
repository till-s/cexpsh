/* $Id$ */
#ifndef CEXP_MODULE_PRIVATE_H
#define CEXP_MODULE_PRIVATE_H

/* Private interface to cexp modules */

#include "cexpmod.h"

/* implementation of a module */
#define MAX_NUM_MODULES 256
#define LD_WORDLEN		5
#define BITMAP_DEPTH	((MAX_NUM_MODULES)>>(LD_WORDLEN))
#define BITMAP_SET(bm,bitno) (((bm)[(bitno)>>LD_WORDLEN]) |= (1<<((bitno)&((1<<LD_WORDLEN)-1))))
#define BITMAP_CLR(bm,bitno) (((bm)[(bitno)>>LD_WORDLEN]) &= ~(1<<((bitno)&((1<<LD_WORDLEN)-1))))
#define BITMAP_TST(bm,bitno) (((bm)[(bitno)>>LD_WORDLEN]) &  (1<<((bitno)&((1<<LD_WORDLEN)-1))))
#define BITMAP_DECLARE(bm)	BitmapWord bm[BITMAP_DEPTH]

typedef	unsigned long	BitmapWord;
typedef short			ModuleId;	/* Id < 0 means INVALID */

typedef void			(*VoidFnPtr)(void);

typedef struct CexpModuleRec_ {
	CexpSymTbl			symtbl;
	char				*name;
	CexpModule			next;
	ModuleId 			id;			/* unique ID */
	void				*memSeg;	/* actual memory */
	unsigned long		memSize;	/* size of the loaded stuff */
	BITMAP_DECLARE(neededby);		/* bitmap module ids that depend on this one */
	BITMAP_DECLARE(needs);			/* bitmap of module ids this module depends on */
	VoidFnPtr			*ctor_list;
	unsigned			nCtors;
	VoidFnPtr			*dtor_list;
	unsigned			nDtors;
} CexpModuleRec;

/* This routine must be provided by the underlying
 * object file handling. It is responsible for
 * allocating all of the necessary members of the
 * new modules (except for the name).
 *
 * In particular, this routine must raise the
 * bits for all module dependencies in the 'needs'
 * bitmap:
 *    if (need(some_module))
 *      BITMAP_SET(this_module->needs,some_module->id);
 */
int
cexpLoadFile(char *filename, CexpModule new_module);

/* Release all data structures associated with *pmod
 *
 * NOTE: this must only be called once the module
 *       has been dequeued from the list
 */
void
cexpModuleFree(CexpModule *pmod);

#endif
