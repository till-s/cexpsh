/* $Id$ */
#ifndef CEXP_HELP_H
#define CEXP_HELP_H

/* Interface to Cexp's trivial help facility */

#ifdef _INSIDE_CEXP_
#include "cexp.h"
#include "cexpmod.h"
#include "cexpsyms.h"
/* magic name of help tables */
#define CEXP_HELP_TAB _cexpHelpTab
#define CEXP_HELP_TAB_NAME "_cexpHelpTab"
#endif

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
 *	HELP(name, 
 *
 * with the last entry being HELP("",,0,)
 *
 * Because the help tables are
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

#ifdef _INSIDE_CEXP_
void
cexpAddHelpToSymTab(CexpHelpTab h, CexpSymTbl t);
#endif

#endif
