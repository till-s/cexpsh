#ifndef CEXP_HDR1_H
#define CEXP_HDR1_H

typedef unsigned long (*CexpFuncPtr)();

typedef struct CexpSymRec_ {
	char		*name;
	int		type;
	int		size;
	union	{
		CexpFuncPtr	func;
		unsigned long	*addr;
	}			val;
} CexpSymRec, *CexpSym;

typedef struct CexpSymTblRec_ {
	unsigned long	nentries;
	CexpSym		syms;
} CexpSymTblRec, *CexpSymTbl;

typedef struct CexpParserArgRec {
	CexpSymTbl	symtbl;
	unsigned char	*chpt;
} CexpParserArgRec, *CexpParserArg;

void
cexpFreeSymTbl(CexpSymTbl arg);

CexpSymTbl
cexpSlurpElf(int fd);

CexpSym
cexpSymTblLookup(CexpSymTbl t, char *name);

#ifndef _INSIDE_CEXP_Y
#include "cexp.tab.h"
#endif

#endif
