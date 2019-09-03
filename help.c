#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* help information for important internal 'cexp' routines */

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

#include "cexpmod.h"
#include "cexpsymsP.h"
#include "cexp.h"
#define _INSIDE_CEXP_
#include "cexpHelp.h"

#ifdef HAVE_BFD_DISASSEMBLER
extern int cexpDisassemble();
#define DISAS_HELP "    cexpDisassemble      - disassemble memory\n"
#else
#define DISAS_HELP
#endif

#ifdef HAVE_TECLA
extern int cexpResizeTerminal();
#endif

CEXP_HELP_TAB_BEGIN(cexp)
	HELP(
"Search the system symbol table and the user variables\n\
for a regular expression.  Info about the symbol will be\n\
printed on stdout.",
		int,
		lkup,(char *regexp_pattern)
	),
	HELP(
"Search for an address in the system symbol table and\n\
print a range (howmany) of symbols close to the address\n\
of interest to stdout. Howmany defaults to +-5 if passed 0",
		int,
		lkaddr,(void *addr, int howmany)
	),
	HELP(
"Load an object module (module_name may be NULL), returns ID",
		CexpModule,
		cexpModuleLoad,(char *file_name, char *module_name)
	),

#ifdef USE_LOADER
	HELP(
"Unload a module (pass handle, RETURNS: 0 on success)",
		int,
		cexpModuleUnload,(CexpModule moduleHandle)
	),
#endif
	HELP(
"Return a module's name (string owned by module code)",
		char*,
		cexpModuleName,(CexpModule moduleID)
	),
	HELP(
"List the IDs of modules whose name matches a pattern\n\
to file 'f' (stdout if NULL).\n\
RETURNS: First module ID found, NULL on no match.",
		CexpModule,
		cexpModuleFindByName,(char *pattern, FILE *f)
	),
	HELP(
"Dump info about a module to 'f' (stdout if NULL).\n\
If NULL is passed for the module ID, info about all\n\
modules is given. 'level' selects Info verbosity:\n\
  0: print name and load address of text section\n\
  1: add module dependency info\n\
  2: add memory requirements info\n\
  3: add load addresses/names for all sections\n\
RETURNS: mod->next",
		int,
		cexpModuleInfo,(CexpModule mod, int level, FILE *f)
	),
	HELP(
"Dump info about a module's section addresses in a \n\
format suitable to GDB to 'f' (stdout if NULL).\n\
If NULL is passed for the module ID, info about all\n\
modules is given. Default prefix (if NULL) is\n\
'add-symbol-file'\n\
RETURNS: mod->next",
		int,
		cexpModuleDumpGdbSectionInfo, (CexpModule mod, char *prefix, FILE *feil)
	),
	HELP(
"The main interpreter loop, it can be registered with a shell...",
		int,
		cexp_main,(int argc, char **argv)
	),
	HELP(
"A few Cexp builtin routines are:\n\n\
    lkup                   - lookup a symbol\n\
    lkaddr                 - find the address closest to a symbol\n\
    cexpModuleLoad         - load an object file\n"
#ifdef USE_LOADER
"    cexpModuleUnload       - remove a module from the running system\n"
#endif
"\
    cexpModuleName         - return a module name given its handle\n\
    cexpModuleFindByName   - find a module given its name\n\
    cexpModuleInfo         - dump info about one or all modules\n"
	DISAS_HELP
"    cexpsh(\"scriptfile\")   - run cexp recursively - e.g. for evaluating a script\n\n\
Use 'symbol.help(level)' for getting info about a symbol:\n\n\
    lkup.help(1)\n\n\
Type a C expression, e.g.\n\n\
        printf(\"hello %s\\n\",\"cruelworld\" + 5)\n",
		int,
		cexpsh,(char* cmdline)
	),
#ifdef HAVE_BFD_DISASSEMBLER
	HELP(
"Disassemble 'n' lines (defaults to 10 if 0) from 'addr'.\n\
If 'addr' is passed NULL, disassembly resumes where the\n\
last call to cexpDisassemble() stopped.\n\
Parameter 'di' should be set to NULL (automatically determined).",
		int,
		cexpDisassemble,(void *addr, int n, disassemble_info *di)
	),
#endif
#ifdef HAVE_TECLA
	HELP(
"Resize the terminal, i.e. try to query the terminal for its size\n\
and notify TECLA of the new size. If nothing works, you may pass a\n\
default size (struct {int row,col;} *psize) in the optional argument.\n",
		int,
		cexpResizeTerminal,(void *psize)
	),
#endif
CEXP_HELP_TAB_END

void
cexpAddHelpToSymTab(CexpHelpTab h, CexpSymTbl t)
{
CexpSym found;
int		i;
	for (; h->addr; h++) {
		/* scan identical addresses skipping section symbols */
		for ( i=cexpSymTblLkAddrIdx(h->addr,0,0,t);
			  i>=0 && (found = t->aindex[i])->value.ptv == h->addr;
		      i-- ) {
			if (CEXP_SYMFLG_GLBL == (found->flags & (CEXP_SYMFLG_GLBL|CEXP_SYMFLG_SECT))) {
				found->help=h->info.text;
				break;
			}
		}
	}
}
