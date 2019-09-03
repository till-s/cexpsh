#ifndef CEXP_CEXPSYMS_H
#define CEXP_CEXPSYMS_H

/* Interface to the symbol table */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

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

#include <stdio.h>	/* print info routine */

#include "cexp.h"
#include "ctyps.h"

typedef struct CexpSymRec_ {
	const char			*name;
	CexpTypedAddrRec	value;
/* ^^^ fields above here are used by the 'gencore' [host] utility which must
 *     be adapted if anything is changed; see cexpmodP.h for details
 */
	int					size;
	unsigned			flags;
	char				*help;
} CexpSymRec;

/* flags associated with symbols */
#define CEXP_SYMFLG_GLBL		(1<<0) /* a global symbol */
#define CEXP_SYMFLG_WEAK		(1<<1) /* a weak symbol   */
#define CEXP_SYMFLG_MALLOC_HELP	(1<<3) /* whether the help info is static or malloc()ed */
#define CEXP_SYMFLG_SECT		(1<<4) /* a section (name) symbol */


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
cexpSymTblLookupRegex(char *re, int *pmax, CexpSym s, FILE *f, CexpSymTbl t);

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
