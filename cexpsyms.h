#ifndef CEXP_CEXPSYMS_H
#define CEXP_CEXPSYMS_H
/* $Id$ */

/* Interface to the symbol table */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

#include <stdio.h>	/* print info routine */

#include "cexp.h"
#include "ctyps.h"

typedef struct CexpSymRec_ {
	const char			*name;
	CexpTypedAddrRec	value;
	int					size;
	unsigned			flags;
	char				*help;
} CexpSymRec, *CexpSym;

/* flags associated with symbols */
#define CEXP_SYMFLG_GLBL		(1<<0) /* a global symbol */
#define CEXP_SYMFLG_WEAK		(1<<1) /* a weak symbol   */
#define CEXP_SYMFLG_MALLOC_HELP	(1<<3) /* whether the help info is static or malloc()ed */


typedef struct CexpSymTblRec_	*CexpSymTbl;

/* Symbol table management */

/* lookup a symbol by name */
CexpSym
cexpSymTblLookup(const char *name, CexpSymTbl t);

/* do a binary search for an address */
CexpSym
cexpSymTblLkAddr(void *addr, int margin, FILE *f, CexpSymTbl t);

/* lookup a regular expression */
CexpSym
cexpSymTblLookupRegex(char *re, int max, CexpSym s, FILE *f, CexpSymTbl t);

/* print info about a symbol to FILE */
int
cexpSymPrintInfo(CexpSym s, FILE *f);

/* Invoke member with name 'mname' on a symbol
 * The member must know how to understand the optional
 * arguments.
 *
 * Returns 0 if OK, error message on failure.
 */

char *
cexpSymMember(CexpTypedVal returnVal, CexpSym sym, char *mname, ...);

#endif
