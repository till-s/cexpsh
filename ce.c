/* $Id$ */

/* 'main' program for CEXP which reads lines of input
 * and calls the parser (cexpparse()) on each of them...
 */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <regexp.h>
#include "elfsyms.h"
#include "vars.h"

#ifndef __rtems
#define HAS_GETOPT
#endif

#ifdef YYDEBUG
extern int cexpdebug;
#endif

#ifdef HAS_GETOPT
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
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
}
#endif

#ifdef DEBUG
#include <math.h>
/* create a couple of variables for testing */

/* NOTE: these names are global and likely to
 *       clash - however, I have to type them
 *       over and over during testing and hence
 *       I want them to be simple - just undef
 *       DEBUG
 */
double	f,g,h;
char	c,d,e;
int		a,b;

/* a wrapper to link a math routine for testing doubles */
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
int
lkup(char *re)
{
extern	CexpSym _cexpSymTblLookupRegex();
regexp	*rc=0;
CexpSym s;
int		ch=0;

	if (!(rc=regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return -1;
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
	return 0;
}

int
lkaddr(void *addr)
{
	cexpSymTblLkAddr(addr, 8, 0, 0);
	return 0;
}

int
main(int argc, char **argv)
{
char				*line,*prompt,*tmp;
CexpParserCtx		ctx;

#ifdef HAS_GETOPT
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
		case 'd': cexpdebug=1;
#endif
	}
}
#endif

if (!(ctx=cexpCreateParserCtx(cexpCreateSymTbl(argc>optind?argv[optind]:0)))) {
	fprintf(stderr,"Need an elf symbol table file arg\n");
	return 1;
}

fprintf(stderr,"main is at 0x%08lx\n",(unsigned long)main);

tmp = argc>0 ? argv[0] : "Cexp";
prompt=malloc(strlen(tmp)+2);
strcpy(prompt,tmp);
strcat(prompt,">");
while ((line=readline(prompt))) {
	cexpResetParserCtx(ctx,line);
	cexpparse((void*)ctx);
	add_history(line);
	free(line);
}
free(prompt);

cexpFreeParserCtx(ctx);

return 0;
}
