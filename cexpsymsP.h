/* private interface to cexp symbols (only seen by code implementing the symboltable) */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *    Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 * 	  Stanford Linear Accelerator Center, Stanford University.
 *
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 *     the Stanford Linear Accelerator Center, Stanford University,
 * 	   under Contract DE-AC03-76SFO0515 with the Department of Energy.
 * 
 * Government disclaimer of liability
 * ----------------------------------
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied, or
 * assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately owned
 * rights.
 * 
 * Stanford disclaimer of liability
 * --------------------------------
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * Stanford disclaimer of copyright
 * --------------------------------
 * Stanford University, owner of the copyright, hereby disclaims its
 * copyright and all other rights in this software.  Hence, anyone may
 * freely use it for any purpose without restriction.  
 * 
 * Maintenance of notices
 * ----------------------
 * In the interest of clarity regarding the origin and status of this
 * SLAC software, this and all the preceding Stanford University notices
 * are to remain affixed to any copy or derivative of this software made
 * or distributed by the recipient and are to be affixed to any copy of
 * software made or distributed by the recipient that contains a copy or
 * derivative of this software.
 * 
 * SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 

#ifndef CEXP_CEXPSYMS_P_H
#define CEXP_CEXPSYMS_P_H
#include "cexpsyms.h"
#include <cexp_regex.h>

/* our implementation of the symbol table holds more information
 * that we make public
 */
typedef struct CexpStrTblRec_ {
	char                  *chars;
	struct CexpStrTblRec_ *next;
} CexpStrTblRec, *CexpStrTbl;

typedef struct CexpSymTblRec_ {
	unsigned long	nentries;
	unsigned long   size;
	CexpSym			syms; 		/* symbol table, sorted in ascending order (key=name) */
	CexpStrTbl      strtbl;
	CexpSym			*aindex;	/* an index sorted to ascending addresses */
	CexpSymTbl		next;		/* linked list of tables */
} CexpSymTblRec;

int
_cexp_addrcomp(const void *a, const void *b);
int
_cexp_namecomp(const void *a, const void *b);

char *
rshLoad(char *host, char *user, char *cmd, long *size_p);

/* Determine if we want to keep the external symbol 'ext_sym'.
 * Must return a pointer to the symbol name if YES and 0 if NO
 */
typedef const char * (*CexpSymFilterProc)(void *ext_sym, void *closure);

/* Assign the internal representation fields to 'intSym' */
typedef void (CexpSymAssignProc)(void *ext_sym, CexpSym int_sym, void *closure);

/* Allocate a new symbol table capable of holding 'n_entries' slots 
 * (n_entries may be zero if a pre-fabricated symbol array is installed
 * into this table).
 *
 * RETURNS: symbol table or NULL on failure.
 */
CexpSymTbl
cexpNewSymTbl(unsigned n_entries);

/* Sort symbols and addresses */
void
cexpSortSymTbl(CexpSymTbl stbl);

/* Build sorted index of addresses
 * RETURNS 0 on success, nonzero on error (no memory of index table)
 */
int
cexpIndexSymTbl(CexpSymTbl stbl);

/* Convert 'nsyms' external symbols to internal representation and add to table;
 * 
 * NOTES: - 'stbl' may be NULL in which case a new table is allocated.
 *        - routine does not sort entries (the idea is that multiple external
 *          tables are added and eventually sorted).
 *        - symbols should only be looked up *after* all necessary calls
 *          of this routine are executed and the symbol table sorted.
 *          It is NOT SAFE to change the structure of the table once
 *          users hold references into the table.
 *
 *        - if the CEXP_SYMTBL_FLAG_NO_STRCPY is set in 'flags' then
 *          no copy of the symbol name is made. It is assumed that the
 *          external symbol table holds static strings which are safe
 *          to be re-used.
 */

#define CEXP_SYMTBL_FLAG_NO_STRCPY  (1<<0)

CexpSymTbl
cexpAddSymTbl(CexpSymTbl stbl, void *syms, int symSize, int nsyms, CexpSymFilterProc filter, CexpSymAssignProc assign, void *closure, unsigned flags);

/* create and sort a Cexp symbol table from external representation */
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
