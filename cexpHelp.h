/* $Id$ */
#ifndef CEXP_HELP_H
#define CEXP_HELP_H

/* Interface to Cexp's trivial help facility */

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

#ifdef _INSIDE_CEXP_
#include "cexp.h"
#include "cexpmod.h"
#include "cexpsyms.h"
#endif
/* magic name of help tables */
#define CEXP_HELP_TAB _cexpHelpTab
#define CEXP_HELP_TAB_NAME "_cexpHelpTab"

/* Help structure as used by the symbol table */

/* both fields may be NULL if unavailable */
typedef struct CexpHelpRec_ {
	char	*text;		/* explanation text */
/* TODO; currently, only the 'text' field is used */
#ifdef HAVE_SEPARATE_INTERFACE
	char	*interface;	/* C-style symbol declaration */
#endif
} CexpHelpRec, *CexpHelp;

/* Code providing help information must somehow
 * provide one or more static arrays of CexpHelpTabRec items.
 * The module loader then tries to pick up these variables
 * and registers all of their fields with the loaded
 * symbol table.
 * The help arrays defined by a module must be defined
 * as follows:
 *
 * CEXP_HELP_TAB_BEGIN
 *	HELP( "text", <return type>, <fn_name>, <arg prototype> ),
 *	HELP( "help about foo", void, foo, (int unused) ),
 *	...
 * CEXP_HELP_TAB_END 
 *
 */

typedef struct CexpHelpTabRec_ {
	void		*addr;
	CexpHelpRec	info;
} CexpHelpTabRec, *CexpHelpTab;

#ifdef HAVE_SEPARATE_INTERFACE
#define HELP(annot, rtn, name, args) \
	{ name, {annot,"\t"#rtn" "#name" "#args } }
#else
#define HELP(annot, rtn, name, args) \
	{ name, {"\n\t"#rtn" "#name" "#args"\n\n"annot } }
#endif

#ifdef __GNUC__
#define CEXP_HELP_TAB_BEGIN \
	static CexpHelpTabRec CEXP_HELP_TAB []\
   	__attribute__((unused))\
	={
#else
#define CEXP_HELP_TAB_BEGIN \
	static CexpHelpTabRec CEXP_HELP_TAB []\
	={
#endif

#define CEXP_HELP_TAB_END \
	HELP("",,0,)};

#ifdef _INSIDE_CEXP_
void
cexpAddHelpToSymTab(CexpHelpTab h, CexpSymTbl t);
#endif

#endif
