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

#endif
