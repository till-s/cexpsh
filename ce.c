/* $Id$ */

/* 'main' program for CEXP which reads lines of input
 * and calls the parser (yyparse()) on each of them...
 */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

#include "cexp.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <regexp.h>

#ifdef YYDEBUG
extern int yydebug;
#endif

static void
usage(char *nm)
{
	fprintf(stderr, "usage: %s [-h]",nm);
#ifdef YYDEBUG
	fprintf(stderr, " [-d]");
#endif
	fprintf(stderr," <ELF symbol file>\n");
	fprintf(stderr, "       C expression parser\n");
	fprintf(stderr, "       -h print this message\n");
#ifdef YYDEBUG
	fprintf(stderr, "       -d enable parser debugging messages\n");
#endif
	fprintf(stderr, "       Author: Till Straumann <Till.Straumann@TU-Berlin.de>\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
}

#define DEBUG
#ifdef DEBUG
#include <math.h>
double f,g,h;
char c,d,e;
int a,b;

void root(double * res, double n)
{*res= sqrt(n);}
#endif

#ifdef __rtems
#define main cexp_main
#define optind 1
#endif

static void *
varprint(char *name, CexpTypedVal v, void *arg)
{
FILE	*f=stdout;
regexp	*rc=arg;
	if (regexec(rc,name)) {
		cexpTVPrintInfo(v,f);
		fprintf(f,": %s\n",name);
	}
	return 0;
}

/* alternate entries for the lookup functions */
void
lkup(char *re)
{
extern	CexpSym _cexpSymTblLookupRegex();
regexp	*rc=0;
CexpSym s;
int		ch=0;

	if (!(rc=regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return;
	}

	for (s=0; !ch && (s=_cexpSymTblLookupRegex(rc,25,s,0,0));) {
		char *line;
		line=readline("More (Y/n)?");
		ch=line[0];
		free(line);
		if ('Y'==toupper(ch)) ch=0;
	}
	printf("\nUSER VARIABLES:\n");
	cexpVarWalk(varprint,(void*)rc);

	if (rc) free(rc);
}

void
lkaddr(void *addr)
{
	cexpSymTblLkAddr(addr, 8, 0, 0);
}
int
main(int argc, char **argv)
{
char				*line;
CexpParserCtx		ctx;

#ifndef __rtems
int					opt;
char				optstr[]={
						'h',

#ifdef YYDEBUG
						'd',
#endif
						'\0'
					};

while ((opt=getopt(argc, argv, optstr))>=0) {
	switch (opt) {
		default:  fprintf(stderr,"Unknown Option %c\n",opt);
		case 'h': usage(argv[0]); return(1);
#ifdef YYDEBUG
		case 'd': yydebug=1;
#endif
	}
}
#endif

if (!(ctx=cexpCreateParserCtx(cexpCreateSymTbl(argc>optind?argv[optind]:0)))) {
	fprintf(stderr,"Need an elf symbol table file arg\n");
	return 1;
}

fprintf(stderr,"main is at 0x%08lx\n",(unsigned long)main);

while ((line=readline("Cexpr>"))) {
	cexpResetParserCtx(ctx,line);
	yyparse((void*)ctx);
	add_history(line);
	free(line);
}

cexpFreeParserCtx(ctx);

return 0;
}
