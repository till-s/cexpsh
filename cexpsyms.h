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
} CexpSymRec, *CexpSym;

/* flags associated with symbols */
#define CEXP_SYMFLG_GLBL		(1<<0) /* a global symbol */
#define CEXP_SYMFLG_WEAK		(1<<1) /* a weak symbol   */
#define CEXP_SYMFLG_PASS_CTX 	(1<<2) /* implicitely pass parser context to the function described by this symbol */


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

#endif
