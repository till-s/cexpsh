/* private interface to cexp symbols (only seen by code implementing the symboltable) */

/*
 * Copyright 2002, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */


#ifndef CEXP_CEXPSYMS_P_H
#define CEXP_CEXPSYMS_P_H
#include "cexpsyms.h"
#include <cexp_regex.h>

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

/* a semi-public routine which takes a precompiled regexp.
 * The reason this is not public is that we try to keep
 * the public from having to deal with/know about the regexp
 * implementation, i.e. which variant, which headers etc.
 */
CexpSym
_cexpSymTblLookupRegex(cexp_regex *rc, int *pmax, CexpSym s, FILE *f, CexpSymTbl t);

#define LOAD_CHUNK    2000

#endif
