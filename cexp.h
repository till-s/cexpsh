#ifndef CEXP_HDR1_H
#define CEXP_HDR1_H


#include <stdio.h>

#include "ctyps.h"

typedef struct CexpSymRec_ {
	char			*name;
	CexpTypedValRec	value;
	int				size;
} CexpSymRec, *CexpSym;

typedef struct CexpSymTblRec_ {
	unsigned long	nentries;
	CexpSym			syms;
} CexpSymTblRec, *CexpSymTbl;

typedef struct CexpParserCtxRec {
	CexpSymTbl		symtbl;
	unsigned char	*chpt;
	char			*lineStrTbl[10];	/* allow for 10 strings on one line of input */
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

/* do a binary search for an address */
CexpSym
cexpSymTblLkAddr(void *addr, int margin, FILE *f, CexpSymTbl t);

/* lookup a regular expression */
CexpSym
cexpSymTblLookupRegex(char *re, int max, CexpSym s, FILE *f, CexpSymTbl t);

CexpParserCtx
cexpCreateParserCtx(CexpSymTbl t);

void
cexpResetParserCtx(CexpParserCtx ctx, char *linebuf);

void
cexpFreeParserCtx(CexpParserCtx ctx);

/* parse a line of input; the line buffer is part of
 * the context that must be passed to this routine
 */
int
yyparse(void*); /* pass a CexpParserCtx pointer */

#define YYDEBUG	1

#ifndef _INSIDE_CEXP_Y
#include "cexp.tab.h"
#endif

#endif
