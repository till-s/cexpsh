#ifndef CEXP_PUBLIC_HDR_H
#define CEXP_PUBLIC_HDR_H

/* $Id$ */

/* public interface to the 'cexp' C Expression Parser and symbol table utility */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002  - this line must not be removed*/
/* License: GPL, for details see http:www.gnu.org - this line must not be removed */

#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Managing modules (object code and symbols) */
typedef struct CexpModuleRec_	*CexpModule;

/* you may use this to check if the system module
 * (i.e. system symbol table) has been loaded
 * already...
 */
extern  CexpModule	cexpSystemModule;

/* load an object file and register it as 'module_name'
 * 'module_name' may be 0 in which case the filename
 * will be used as the module name.
 *
 * RETURNS: 0 on success, nonzero on failure
 */

int
cexpModuleLoad(char *file_name, char *module_name);

/* unload a module */
int
cexpModuleUnload(char *module_name);

typedef struct CexpParserCtxRec_	*CexpParserCtx;

/* The C expression parser */

/* create and initialize a parser context
 * 
 * NOTE: Essentially because bison does not
 *       pass the parser argument/context
 *       to the error messenger (yyerror()),
 *       it is not possible to pass that routine
 *       a file descriptor.
 *       Hence, all printing (of the evaluated 
 *       expression etc.) is done to stdout,
 *       and all error messages go to stderr.
 *       Before calling the parser, you may
 *       try to redirect stdout/stderr...
 */

CexpParserCtx
cexpCreateParserCtx(void);

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
cexpResetParserCtx(CexpParserCtx ctx, char *linebuf);

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
lkaddr(void *addr);

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

int
cexp_main1(int argc, char **argv, void (*callback)(int argc, char **argv, jmp_buf *env));

/* kill a main loop. This should only be called by
 * low level exception code for aborting the main loop
 *
 * ctxt is a pointer to a main loop's context.
 * The 'doWhat' argument may be either 0 or
 * CEXP_MAIN_KILLED. In the former case, 'cexp'
 * will cleanup, recover and resume execution,
 * otherwise it will return after cleaning up.
 */
void
cexp_kill(jmp_buf *ctxt, int doWhat);

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
