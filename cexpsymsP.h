/* $Id$ */
/* private interface to cexp symbols (only seen by code implementing the symboltable) */
#ifndef CEXP_CEXPSYMS_P_H
#define CEXP_CEXPSYMS_P_H
#include "cexpsyms.h"

/* our implementation of the symbol table holds more information
 * that we make public
 */
typedef struct PrivSymTblRec_ {
	/* NOTE: the stab field MUST be first, so we can cast pointers around */
	CexpSymTblRec	stab;	/* symbol table, sorted in ascending order (key=name) */
	char		*strtbl;	/* string table */
	CexpSym		*aindex;	/* an index sorted to ascending addresses */
} PrivSymTblRec, *PrivSymTbl;

int
_cexp_addrcomp(const void *a, const void *b);
int
_cexp_namecomp(const void *a, const void *b);

char *
rshLoad(char *host, char *user, char *cmd);

#define LOAD_CHUNK    2000

/* flags associated with symbols */
#define CEXP_SYMFLG_GLBL	(1<<0) /* a global symbol */
#define CEXP_SYMFLG_WEAK	(1<<1) /* a weak symbol   */

#endif
