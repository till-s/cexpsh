#ifndef CEXP_HDR1_H
#define CEXP_HDR1_H

typedef unsigned long (*CexpFuncPtr)();

typedef struct CexpSymRec_ {
	char			*name;
	int				type;
	int				size;
	union	{
		CexpFuncPtr		func;
		unsigned long	*addr;
	}				val;
} CexpSymRec, *CexpSym;

typedef struct CexpSymTblRec_ {
	unsigned long	nentries;
	CexpSym			syms;
} CexpSymTblRec, *CexpSymTbl;

typedef struct CexpParserCtxRec {
	CexpSymTbl		symtbl;
	unsigned char	*chpt;
	char			*lineStrTbl[10];	/* allow for 10 strings on one line of input */
	unsigned int	lstLen;
	unsigned long	evalInhibit;
} CexpParserCtxRec, *CexpParserCtx;

/* Symbol table management */

/* the global system image symbol table */
extern CexpSymTbl cexpSysSymTbl;

/* Read an ELF file's ".symtab" section and create an
 * internal symbol table representation.
 * The first call to cexpCreateSymTbl() will also set
 * the global 'cexpSysSymTbl'.
 * If the file name argument is NULL, the routine
 * returns cexpSysSymTbl.
 */
CexpSymTbl
cexpCreateSymTbl(char *symFileName);

/* free a symbol table (*ps).
 * Note that if *ps is the sysSymTbl,
 * it will only be released if
 * ps==sysSymTbl. I.e:
 *
 * CexpSymTbl p=cexpSysSymTbl;
 * cexpFreeSymTbl(&p);
 * 
 * will _not_ free up the cexpSysSymTbl
 * but merely set *p t0 zero. However,
 * cexpFreeSymTbl(&sysSymTbl) _will_
 * release the global table.
 */
void
cexpFreeSymTbl(CexpSymTbl *psymtbl);

CexpSymTbl
cexpSlurpElf(int fd);

CexpSym
cexpSymTblLookup(char *name, CexpSymTbl t);

#define YYDEBUG	1

#ifndef _INSIDE_CEXP_Y
#include "cexp.tab.h"
#endif

#endif
