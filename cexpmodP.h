/* $Id$ */
#ifndef CEXP_MODULE_PRIVATE_H
#define CEXP_MODULE_PRIVATE_H

/* Private interface to cexp modules */

#include "cexpmod.h"

/* implementation of a module */

typedef struct CexpModuleRec_ {
	CexpSymTbl	symtbl;
	char		*name;
	CexpModule	next;
} CexpModuleRec;

/* This routine must be provided by the underlying
 * object file handling. It is responsible for
 * allocating all of the necessary members of the
 * new modules (except for the name)
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
