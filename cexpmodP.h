/* $Id$ */
#ifndef CEXP_MODULE_PRIVATE_H
#define CEXP_MODULE_PRIVATE_H

/* Private interface to cexp modules */

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

/* Version to protect the layout of CexpModuleRec, CexpSymRec, CexpTARec */
#define CEXPMOD_MAGIC	"cexp0000"

typedef struct CexpModuleRec_ {
	char				*name;
	CexpModule			next;
	CexpSym				*section_syms;	/* NULL terminated list to section address symbols
										 * pointer may be NULL if there is no list
										 */
	unsigned long		text_vma;		/* where the text segment was loaded */
/* ^^^^ FIELDS ABOVE HERE ARE USED BY THE gencore UTILITY ^^^^^^^^^^^^^^^^
 * when changing anything, 'gencore' must be changed accordingly and the
 * magic number be changed
 */
	CexpSymTbl			symtbl;
	ModuleId 			id;			/* unique ID */
	void				*memSeg;	/* actual memory */
	unsigned long		memSize;	/* size of the loaded stuff */
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
	char				*fileName;
	void                *fileAttributes;
	                                /* compatibility attributes as described by '.gnu.attributes'
									 * section. Currently, only pmbfd supports this.
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
