#ifndef CEXP_VARS_H
#define CEXP_VARS_H
/* $Id$ */

#include "cexpsyms.h"

/* Interface to CEXP variables */

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


/* initialize the facility: this routine must exactly be called ONCE */
void
cexpVarInitOnce(void);

/* destroy all variables */
void
cexpVarsFlush(void);

/* lookup a variable
 * If the variable is not found, the return value
 * is 0. If it is found, a pointer to its
 * data area is returned.
 * It is the users responsability to protect the
 * data area from concurrent access by multiple
 * threads. The internal locking only protects
 * the variable management.
 * If the 'creat' is nonzero the
 * variable will be created if it doesn't exist
 * already.
 */

CexpSym
cexpVarLookup(const char *name, int creat);

/* remove a variable
 * RETURNS: nonzero if the variable was found & deleted
 *
 * NOTE: the comments about cexpVarLookup in a multithreaded
 *       environment. Variables exist in a global namespace,
 *       so multiple threads may use them. However, if they
 *       do so, they must make sure not to use pointers to
 *       'CexpTypedVal's of deleted variables.
 */
void *
cexpVarDelete(char *name);

/* call a user routine for every variable in lexical
 * order. The 'walk' aborts if the user routine returns
 * a nonzero value returning that same value to the
 * caller.
 *
 * NOTE: 1) in a multithreaded environment, the global
 *       variable table is locked while in
 *       cexpVarWalk(), i.e. nobody can access
 *       variables while this routine is active.
 *       2) The walker  MUST NOT add/delete variables.
 */

typedef void* (*CexpVarWalker)(const char *name, CexpSym a, void *usrArg);

void *
cexpVarWalk(CexpVarWalker walker, void *usrArg);

/* string table management
 * There exists one global 'string table', where static
 * strings are registered. There is no way of deleting
 * strings, so it is safe to assume they live 'forever'
 */

/* lookup a string optionally adding it to the
 * table.
 */
char *
cexpStrLookup(char *name, int creat);

#endif
