/* $Id$ */
#ifndef CEXP_MODULE_H
#define CEXP_MODULE_H

/* Interface to cexp modules */

#include "cexp.h"
#include "cexpsyms.h"

/* search for a name in all module's symbol tables */
CexpSym
cexpSymLookup(const char *name, CexpModule *pmod);

/* search for an address in all modules */
CexpSym
cexpSymLkAddr(void *addr, int margin, FILE *f, CexpModule *pmod);

/* return a module's name (string owned by module code) */
char *
cexpModuleName(CexpModule mod);

/* list the IDs of modules whose name matches a pattern
 * to file 'f' (stdout if NULL).
 *
 * RETURNS: First module ID found, NULL on no match.
 */
CexpModule
cexpModuleFindByName(char *pattern, FILE *f);

/* Dump info about a module to 'f' (stdout if NULL)
 * If NULL is passed for the module ID, info about
 * all modules is given.
 */
int
cexpModuleInfo(CexpModule mod, FILE *f);

/* initialize vital stuff; must be called at program startup */
void
cexpModuleInit(void);

#endif
