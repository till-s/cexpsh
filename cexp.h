#ifndef CEXP_PUBLIC_HDR_H
#define CEXP_PUBLIC_HDR_H

/* $Id$ */

/* public interface to the 'cexp' C Expression Parser and symbol table utility */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002  - this line must not be removed*/
/* License: GPL, for details see http:www.gnu.org - this line must not be removed */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CexpSymTblRec_ *CexpSymTbl;

typedef struct CexpParserCtxRec_ *CexpParserCtx;

/* Symbol table management */

/* the global system image symbol table */
extern CexpSymTbl cexpSysSymTbl;

/* Read an ELF file's ".symtab" section and create an
 * internal symbol table representation.
 * The first call to cexpCreateSymTbl() will also set
 * the global 'cexpSysSymTbl'.
 * If the file name argument is NULL, the routine
 * returns cexpSysSymTbl.
 *
 * The ELF file may be the system's object file
 * itself (needs more memory during execution of
 * this routine) or a stripped down version containing
 * only the symbol table. Such a stripped down
 * symbol file may be created with the 'xsyms' 
 * utility.
 */
CexpSymTbl
cexpCreateSymTbl(char *elfFileName);

/* free a symbol table (*ps).
 * Note that if *ps is the sysSymTbl,
 * it will only be released if
 * ps==sysSymTbl. I.e:
 *
 * CexpSymTbl p=cexpSysSymTbl;
 * cexpFreeSymTbl(&p);
 * 
 * will _not_ free up the cexpSysSymTbl
 * but merely set *p to zero. However,
 * cexpFreeSymTbl(&sysSymTbl) _will_
 * release the global table.
 */
void
cexpFreeSymTbl(CexpSymTbl *psymtbl);

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
cexpCreateParserCtx(CexpSymTbl t);

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

/* Release a parser context. This frees up the
 * associated symbol table UNLESS it is the
 * global system symbol table which must be
 * deleted with an explicit call to 
 * cexpFreeSymTbl(&sysSymTbl); (see above).
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
 *      / * Note: here, we try to re-use the system symTbl if
 *        *       it exists already.
 *        * /
 *      CexpParserCtx ctx=cexpCreateParserCtx(
 *                            cexpSysSymTbl ? cexpSysSymTbl :
 *                            cexpCreateSymTbl(filename));
 *		char        *line;
 *
 *           while ((line=readline("prompt>"))) {
 *                cexpResetParserCtx(ctx,line);
 *                cexpparse(ctx);
 *                free(line);
 *           }
 *           cexpFreeParserCtx(ctx);
 *           / * optionally release the global symbol table
 *             * otherwise, it may be left in place for reuse
 *             * by another instance of this code...
 *             * /
 *           cexpFreeSymTbl(&cexpSysSymTbl);
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
/* error return values: */
#define CEXP_MAIN_INVAL_ARG	1	/* invalid option / argument */
#define CEXP_MAIN_NO_SYMS	2	/* unable to open symbol table */
#define CEXP_MAIN_NO_SCRIPT	3	/* unable to open script file */

#ifdef __cplusplus
};
#endif

#endif
