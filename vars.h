#ifndef CEXP_VARS_H
#define CEXP_VARS_H
/* $Id$ */

#include "ctyps.h"

/* Interface to CEXP variables */

/* initialize the facility */
void
cexpVarInit(void);

/* destroy all variables */
void
cexpVarsFlush(void);

/* lookup a variable
 * If the variable is not found, the return value
 * is 0 and the returned value is invalid.
 */
void *
cexpVarLookup(char *name, CexpTypedVal prval);

/* lookup a variable and set to 'val'.
 * If the 'creat' flag is passed, a new variable
 * is created and 'val' assigned to its value.
 * RETURNS nonzero value if set/create succeeds.
 */
void *
cexpVarSet(char *name, CexpTypedVal val, int creat);

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
