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

/* 'magic' names; if symbols with these names are found in
 * an object file, pointers should be stored (by the object
 * loader code) to the iniCallback and finiCallback fields
 * below...
 */
#define	CEXPMOD_INITIALIZER_SYM	"_cexpModuleInitialize"
#define CEXPMOD_FINALIZER_SYM   "_cexpModuleFinalize"

typedef struct CexpModuleRec_ {
	CexpSymTbl			symtbl;
	char				*name;
	CexpModule			next;
	ModuleId 			id;			/* unique ID */
	void				*memSeg;	/* actual memory */
	unsigned long		memSize;	/* size of the loaded stuff */
	unsigned long		text_vma;	/* where the text segment was loaded */
	BITMAP_DECLARE(neededby);		/* bitmap module ids that depend on this one */
	BITMAP_DECLARE(needs);			/* bitmap of module ids this module depends on */
	VoidFnPtr			*ctor_list;
	unsigned			nCtors;
	VoidFnPtr			*dtor_list;
	unsigned			nDtors;
	void				(*cleanup)(CexpModule thismod);
									/* cleanup routine is invoked after destructors NOTE:
									 * this routine is intended for use by the object code
									 * module whereas the 'finiCallback()' below is used
									 * to point to code in the loaded module.
									 */
	void				*modPvt;	/* lowlevel object format private data */
	void				(*iniCallback)(CexpModule thismod);
									/* optional (non-C++) initialization routine; called
									 * after constructors
									 */
	int					(*finiCallback)(CexpModule thismod);
									/* optional (non-C++) finalizer; called before destructors.
									 * If 'finiCallback()' returns a non-zero value, this is
									 * interpreted as REJECTING the unload operation and the
									 * module is left untouched. This can e.g. be used to
									 * prevent a 'in-use' driver from being unloaded.
									 */
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
