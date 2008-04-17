/* $Id$ */
#ifndef CEXP_HELP_H
#define CEXP_HELP_H

/* Interface to Cexp's trivial help facility */

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

#ifdef _INSIDE_CEXP_
#include "cexp.h"
#include "cexpmod.h"
#include "cexpsyms.h"
#endif

/* magic name of help tables */
/* Macro magic (see below) doesn't work well -> hardcode in multiple places :-(
#define CEXP_HELP_TAB _cexpHelpTab
*/
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
 * CEXP_HELP_TAB_BEGIN(facility)
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
#define CEXP_HELP_TAB_BEGIN(facility) \
	CexpHelpTabRec _cexpHelpTab ## facility []\
   	__attribute__((unused))\
	={
#else
#define CEXP_HELP_TAB_BEGIN(facility) \
	CexpHelpTabRec _cexpHelpTab ## facility []\
	={
#endif

#define CEXP_HELP_TAB_END \
	HELP("",,0,)};

#ifdef _INSIDE_CEXP_
void
cexpAddHelpToSymTab(CexpHelpTab h, CexpSymTbl t);
#endif

#endif
