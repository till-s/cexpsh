/* $Id$ */
#ifndef CEXP_MODULE_H
#define CEXP_MODULE_H

/* Interface to cexp modules */

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

#include "cexp.h"
#include "cexpsyms.h"

/* search for a name in all module's symbol tables */
CexpSym
cexpSymLookup(const char *name, CexpModule *pmod);

/* search for an address in all modules */
CexpSym
cexpSymLkAddr(void *addr, int margin, FILE *f, CexpModule *pmod);

/* search for an address in all modules giving its aindex 
 * to the *pmod's aindex table
 *
 * RETURNS: aindex or -1 if the address is not within the
 *          boundaries of any module.
 */
int
cexpSymLkAddrIdx(void *addr, int margin, FILE *f, CexpModule *pmod);

/* return a module's name (string owned by module code) */
char *
cexpModuleName(CexpModule mod);

/* list the IDs of modules whose name matches a pattern
 * to file 'f' (stdout if NULL).
 *
 * RETURNS: First module ID found, NULL on no match.
 */
CexpModule
cexpModuleFindByName(char *pattern, FILE *f);

/* Dump info about a module to 'f' (stdout if NULL)
 * If NULL is passed for the module ID, info about
 * all modules is given.
 */
int
cexpModuleInfo(CexpModule mod, FILE *f);

/* initialize vital stuff; must be called excactly ONCE at program startup */
void
cexpModuleInitOnce(void);

#endif
