#ifndef CEXP_VARS_H
#define CEXP_VARS_H
/* $Id$ */

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
cexpVarLookup(char *name, unsigned long *prval);

/* lookup a variable and set to 'val'.
 * If the 'creat' flag is passed, a new variable
 * is created and 'val' assigned to its value.
 * RETURNS nonzero value if set/create succeeds.
 */
void *
cexpVarSet(char *name, unsigned long val, int creat);

/* remove a variable
 * RETURNS: nonzero if the variable was found & deleted
 */
void *
cexpVarDelete(char *name);

#endif
