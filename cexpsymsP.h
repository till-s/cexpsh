/* $Id$ */
/* private interface to cexp symbols (only seen by code implementing the symboltable) */
#ifndef CEXP_CEXPSYMS_P_H
#define CEXP_CEXPSYMS_P_H
#include "cexpsyms.h"

/* our implementation of the symbol table holds more information
 * that we make public
 */
typedef struct CexpSymTblRec_ {
	unsigned long	nentries;
	CexpSym			syms; 		/* symbol table, sorted in ascending order (key=name) */
	char			*strtbl;	/* string table */
	CexpSym			*aindex;	/* an index sorted to ascending addresses */
	CexpSymTbl		next;		/* linked list of tables */
} CexpSymTblRec;

int
_cexp_addrcomp(const void *a, const void *b);
int
_cexp_namecomp(const void *a, const void *b);

char *
rshLoad(char *host, char *user, char *cmd);

/* Determine if we want to keep the external symbol 'ext_sym'.
 * Must return a pointer to the symbol name if YES and 0 if NO
 */
typedef const char * (*CexpSymFilterProc)(void *ext_sym, void *closure);

/* Assign the internal representation fields to 'intSym' */
typedef void (CexpSymAssignProc)(void *ext_sym, CexpSym int_sym, void *closure);

/* create a Cexp symbol table from external representation */
CexpSymTbl
cexpCreateSymTbl(
	void *syms,						/* pointer to external symtab */
	int symSize,					/* size of one slot */
	int nsyms,						/* number of syms in external tab */
	CexpSymFilterProc filter,		/* routine to filter external syms */
	CexpSymAssignProc assign,		/* routine to assign the relevant fields to internal representation */
	void *closure					/* aux pointer passed to filter/assign */
	);

/* release all resources associated with the symbol table
 * (as well as the CexpSymTblRec itself).
 *  *tbl is set to 0.
 */
void
cexpFreeSymTbl(CexpSymTbl *tbl);

/* do a binary search for a symbol's aindex number */
int
cexpSymTblLkAddrIdx(void *addr, int margin, FILE *f, CexpSymTbl t);

#define LOAD_CHUNK    2000

#endif
