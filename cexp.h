#ifndef CEXP_PUBLIC_HDR_H
#define CEXP_PUBLIC_HDR_H

/* $Id$ */

/* public interface to the 'cexp' C Expression Parser and symbol table utility */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002  - this line must not be removed*/

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

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Depending on the threading environment, this
 * routine should be called prior to invoking
 * the first instance of 'cexp'.
 * Under the following circumstances, it is safe
 * to omit this step:
 *
 *  - in a single threaded environment
 *  - when configured with USE_EPICS_OSI
 *  - if an RTEMS initialization task starts
 *    the first instance of cexp.
 *
 * Otherwise, there exists a (minor) race
 * condition for multiple tasks initializing
 * vital structures in parallel.
 *
 * The routine returns 1 for convenience, so
 * it could be used in a C++ static constructor.
 */

typedef void (*CexpSigHandlerInstallProc)(void (*handler)(int));

int
cexpInit(CexpSigHandlerInstallProc sigHandlerInstaller);

/* Managing modules (object code and symbols) */
typedef struct CexpModuleRec_	*CexpModule;
typedef struct CexpSymRec_	*CexpSym;

/* you may use this to check if the system module
 * (i.e. system symbol table) has been loaded
 * already...
 */
extern  CexpModule	cexpSystemModule;

/* load an object file and register it as 'module_name'
 * 'module_name' may be 0 in which case the filename
 * will be used as the module name.
 *
 * RETURNS: module handle on success, 0 on failure
 */

CexpModule
cexpModuleLoad(char *file_name, char *module_name);

/* unload a module */
int
cexpModuleUnload(CexpModule moduleHandle);

/* return a module's name (string owned by module code) */
char *
cexpModuleName(CexpModule mod);

#define CEXP_FILE_QUIET ((FILE*) -1 )

/* list the IDs of modules whose name matches a pattern
 * to file 'f' (stdout if NULL, quiet if FILE_QUIET).
 *
 * RETURNS: First module ID found, NULL on no match.
 */
CexpModule
cexpModuleFindByName(char *pattern, FILE *f);

/* Dump info about a module to 'f' (stdout if NULL)
 * If NULL is passed for the module ID, info about
 * all modules is given.
 * Info of different levels is given:
 *  level 0: module name and text address
 *  level 1: level 0 info and module dependencies
 *  level 2: level 1 info and memory usage info
 *  level 3: level 2 info and section address info
 *
 * RETURNS: mod->next (or NULL if mod==NULL).
 */
CexpModule
cexpModuleInfo(CexpModule mod, int level, FILE *feil);

/* Dump section info for a module (or all modules
 * if mod==NULL) in a format useful to GDB to 'f'
 * (stdout if NULL). Lines in the format
 *
 *  'prefix' 'module_name' 'text_address' { -s 'section_name' 'section_address' }
 *
 * are printed. The default prefix (if NULL is passed)
 * is 'add-symbol-file'.
 *
 * RETURNS: mod->next or NULL.
 */
CexpModule
cexpModuleDumpGdbSectionInfo(CexpModule mod, char *prefix, FILE *feil);

/* search for a name in all module's symbol tables
 *
 * RETURNS: opaque symbol handle / NULL if not found
 *          A handle for the module defining the symbol
 *          is returned in *pmod. It is OK to pass
 *          pmod==NULL if the module handle is not needed.
 *
 * NOTE:    This routine looks for an exact match. If you
 *          want to search for a regular expression, use the
 *          "semi-public" _cexpSymLookupRegex() routine which
 *          requires knowledge of the regex library implementation.
 */
CexpSym
cexpSymLookup(const char *name, CexpModule *pmod);

/* Search for an address in all modules. Symbol info is printed
 * to file 'f' for +/- 'margin' symbols in the vicinity of 'addr'.
 *
 * It is OK to pass 'f'==NULL in which case no info is printed.
 *
 * RETURNS: symbol handle for the symbol closest to 'addr';
 *          module handle in *pmod.
 */
CexpSym
cexpSymLkAddr(void *addr, int margin, FILE *f, CexpModule *pmod);

/* Symbol value/name access */
const char *
cexpSymName(CexpSym sym);

void *
cexpSymValue(CexpSym sym);

/* The C expression parser */
typedef struct CexpParserCtxRec_	*CexpParserCtx;

/* create and initialize a parser context
 * 
 * NOTE: Essentially because bison does not
 *       pass the parser argument/context
 *       to the error messenger (yyerror()),
 *       it is not possible to pass that routine
 *       a file descriptor.
 *       Hence, all error printing is done to stderr,
 *       Before calling the parser, you may
 *       try to redirect stderr...
 *       All normal output (i.e. the evaluated expression)
 *       is sent to 'f'. 'f' may be passed NULL in
 *       which case all normal output is discarded.
 */

CexpParserCtx
cexpCreateParserCtx(FILE *f);

/* reset a parser context; this routine
 * must be called before calling the parser
 * itself. It properly sets up the context
 * for parsing one line of input.
 * A pointer to the '\0' terminated input line 
 * must be provided.
 *
 * Most likely, you want to create the line
 * using GNU 'readline'...
 *
 * NOTE: if linebuf was malloc()ed, it is
 * the user's responsibility to release it
 * _after_ parsing the line.
 */
void
cexpResetParserCtx(CexpParserCtx ctx, const char *linebuf);

/* Release a parser context.
 * 
 * NOTE: UVARS (user variables) are NOT part of
 *       the parser context - they are currently
 *       managed globally.
 *
 */
void
cexpFreeParserCtx(CexpParserCtx ctx);

/* parse a line of input; the line buffer is part of
 * the context that must be passed to this routine
 *
 * A typical calling sequence for the CEXP utility
 * is as follows:
 *
 *      / * Note: the system module/symbol table must be
 *        *       loaded prior to calling this.
 *        * /
 *      CexpParserCtx ctx=cexpCreateParserCtx();
 *		char        *line;
 *
 *           while ((line=readline("prompt>"))) {
 *                cexpResetParserCtx(ctx,line);
 *                cexpparse(ctx);
 *                free(line);
 *           }
 *           cexpFreeParserCtx(ctx);
 *
 *      / * optionally release the global symbol table
 *        * at thie point. Otherwise, it may be left in
 *        * place for reuse by another instance of this
 *        * code...
 *        * /
 */

#ifndef _INSIDE_CEXP_Y
/* pass a CexpParserCtx pointer, this is the public interface
 * NOTE: this requires the BISON '%pure_parser' extension.
 */
int
cexpparse(CexpParserCtx ctx);
#else
/* private interface for bison generated code to which
 * the argument is opaque.
 */
int
cexpparse(void*);
#endif

/* two routines mimicking vxWorks utilities. The names
 * are simple to type...
 */

/* search the system symbol table and the user variables
 * for a regular expression.
 * Info about the symbol will be printed on stdout
 */
int
lkup(char *pattern);

/* search for an address in the system symbol table and
 * print a range of symbols close to the address of
 * interest to stdout.
 */

int
lkaddr(void *addr, int howmany);

/* get info about an address into a buffer without
 * having to know about symbol implementation etc.
 *
 * RETURNS 0 on success, -1 if no close symbol
 * could be found.
 *
 * The routine prints 'module' ':' 'symbol' 
 * into the buffer. Symbol is the closest
 * symbol matching '*paddr'. Closest means 'rounded
 * down', i.e. the name of the routine or
 * variable that encompasses '*paddr'.
 * 
 * *paddr is updated to hold the exact symbol
 * address, so the caller can calculate the
 * difference using this routine.
 */
int
cexpAddrFind(void **addr, char *buf, int size);

/* a wrapper to call cexp main with a variable arglist
 * NOTE: arg0 is automatically set to "cexp_main", hence
 *       'arg1' is the first 'real' argument.
 */
int
cexp(char *arg1,...);

/* the main interpreter loop, it can be registered
 * with a shell...
 */
int
cexp_main(int argc, char **argv);

/* alternate entry point. This one 
 * allows for storing a 'kill environment'.
 * A callback routine will be invoked
 * once the internal jmpbuf is initialized.
 */

/* A main routine context (for doing special magic) */
typedef struct CexpContextRec_ *CexpContext;

int
cexp_main1(int argc, char **argv, void (*callback)(int argc, char **argv, CexpContext ctx));

/* kill a main loop. This should only be called by
 * low level exception code for aborting the main loop
 *
 * The 'doWhat' argument may be either 0 or
 * CEXP_MAIN_KILLED. In the former case, 'cexp'
 * will cleanup, recover and resume execution,
 * otherwise it will return after cleaning up.
 */
void
cexp_kill(int doWhat);

/*
 * If this pointer is non-null (can be set by
 * the application; cexpInit() will automatically
 * install 'signal' here on systems which do have
 * 'signal()'
 */
extern CexpSigHandlerInstallProc cexpSigHandlerInstaller;

/* error return values: */
#define CEXP_MAIN_INVAL_ARG	1	/* invalid option / argument */
#define CEXP_MAIN_NO_SYMS	2	/* unable to open symbol table */
#define CEXP_MAIN_NO_SCRIPT	3	/* unable to open script file */
#define CEXP_MAIN_KILLED	4	/* main loop was killed */
#define CEXP_MAIN_NO_MEM	5	/* no memory */

#ifdef __cplusplus
};
#endif

#endif
