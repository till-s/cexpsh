#ifndef CEXP_VARS_H
#define CEXP_VARS_H
/* $Id$ */

#include "ctyps.h"

/* Interface to CEXP variables */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

/* initialize the facility */
void
cexpVarInit(void);

/* destroy all variables */
void
cexpVarsFlush(void);

/* lookup a variable
 * If the variable is not found, the return value
 * is 0. If it is found, a pointer to its
 * data area is returned.
 * It is the users responsability to protect the
 * data area from concurrent access by multiple
 * threads. The internal locking only protects
 * the variable management.
 * If the 'creat' is nonzero the
 * variable will be created if it doesn't exist
 * already.
 */

CexpTypedVal
cexpVarLookup(char *name, int creat);

/* remove a variable
 * RETURNS: nonzero if the variable was found & deleted
 */
void *
cexpVarDelete(char *name);

/* call a user routine for every variable in lexical
 * order. The 'walk' aborts if the user routine returns
 * a nonzero value returning that same value to the
 * caller.
 *
 * NOTE: 1) in a multithreaded environment, the global
 *       variable table is locked while in
 *       cexpVarWalk(), i.e. nobody can access
 *       variables while this routine is active.
 *       2) The walker  MUST NOT add/delete variables.
 */

typedef void* (*CexpVarWalker)(char *name, CexpTypedVal v, void *usrArg);

void *
cexpVarWalk(CexpVarWalker walker, void *usrArg);

#endif
