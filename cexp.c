/* $Id$ */

/* 'main' program for CEXP which reads lines of input
 * and calls the parser (cexpparse()) on each of them...
 */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_GNU_READLINE /* (recommended) */
#include <readline/readline.h>
#include <readline/history.h>
/* avoid reading curses or terminfo headers */
extern int tgetnum();
#else  /* dont use READLINE  */
#define tgetnum(arg) -1
#define add_history(line) do {} while (0)
#define readline my_readline
static char *my_readline(char *prompt)
{
	char *rval=malloc(500);
	if (prompt)
		fputs(prompt,stdout);
	if (!fgets(rval,500,stdin)) {
		free(rval);
		rval=0;
	}
	return rval;	
}
#endif

#include <regexp.h>
#include "elfsyms.h"
#include "vars.h"

#include "getopt/mygetopt_r.h"

#include "builddate.c"

#ifdef YYDEBUG
extern int cexpdebug;
#endif

static void
usage(char *nm)
{
	fprintf(stderr, "usage: %s [-h] [-v]",nm);
#ifdef YYDEBUG
	fprintf(stderr, " [-d]");
#endif
	fprintf(stderr," [-s <ELF symbol file>]");
	fprintf(stderr," [<script file>]\n");
	fprintf(stderr, "       C expression parser and symbol table utility\n");
	fprintf(stderr, "       -h print this message\n");
	fprintf(stderr, "       -v print version information\n");
#ifdef YYDEBUG
	fprintf(stderr, "       -d enable parser debugging messages\n");
#endif
	fprintf(stderr, "       Author: Till Straumann <Till.Straumann@TU-Berlin.de>\n");
	fprintf(stderr, "       Licensing: GPL (http://www.gnu.org)\n");
}

static void
version(char *nm)
{
	fprintf(stderr,"%s release $Name$, build date %s\n",nm,cexp_build_date);
}

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

static void *
varprint(char *name, CexpTypedAddr a, void *arg)
{
FILE	*f=stdout;
regexp	*rc=arg;
	if (regexec(rc,name)) {
		cexpTAPrintInfo(a,f);
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
int		nl=tgetnum("li");

	if (nl<=0)
		nl=25;

	if (!(rc=regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return -1;
	}

	for (s=0; !ch && (s=_cexpSymTblLookupRegex(rc,nl,s,0,0));) {
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
whatis(char *name, FILE *f)
{
union {
CexpSym			s;
CexpTypedAddr	ptv;
} v;
int rval=-1;

	if (!f) f=stdout;

	if ((v.s=cexpSymTblLookup(name,0))) {
		fprintf(f,"System Symbol Table:\n");
		cexpSymPrintInfo(v.s,f);
		rval=0;
	}
	if ((v.ptv=cexpVarLookup(name,0))) {
		fprintf(f,"User Variable:\n");
		cexpTAPrintInfo(v.ptv,stdout);
		fprintf(f,": %s\n",name);
		rval=0;
	}
	return rval;
}

int
lkaddr(void *addr)
{
	cexpSymTblLkAddr(addr, 8, 0, 0);
	return 0;
}

/* a wrapper to call cexp main with a variable arglist */
int
cexp(char *arg0,...)
{
va_list ap;
int		argc=0;
char	*argv[10]; /* limit to 10 arguments */

	va_start(ap,arg0);

	argv[argc]="cexp_main";
	if (arg0) {
		argv[++argc]=arg0;

		while ((argv[++argc]=va_arg(ap,char*))) {
			if (argc >= sizeof(argv)/sizeof(argv[0])) {
				fprintf(stderr,"cexp: too many arguments\n");
				va_end(ap);
				return CEXP_MAIN_INVAL_ARG;
			}
		}
		argc--; /* strip 0 terminator */
	}

	va_end(ap);
	return cexp_main(++argc,argv);
}

/* This is meant to be called from low-level
 * exception handlers to abort execution.
 * If it is called while 'cexp_main' is not
 * in its main loop (i.e. the context is invalid)
 * bad things will happen. However, things can't
 * probably get much worse...
 */
void
cexp_kill(jmp_buf *ctxt, int doWhat)
{
	longjmp(*ctxt,doWhat);
}

int
cexp_main(int argc, char **argv)
{
	return cexp_main1(argc, argv, 0);
}

int
cexp_main1(int argc, char **argv, void (*callback)(int argc, char **argv, jmp_buf *env))
{
FILE				*scr=0;
char				*line=0,*prompt=0,*tmp;
char				*symfile=0, *script=0;
CexpParserCtx		ctx=0;
int					rval=CEXP_MAIN_INVAL_ARG;
MyGetOptCtxtRec		oc={0}; /* must be initialized */
int					opt, doWhat;	
jmp_buf				mainContext;
char				optstr[]={
						'h',
						'v',
						's',':',
#ifdef YYDEBUG
						'd',
#endif
						'\0'
					};

while ((opt=mygetopt_r(argc, argv, optstr,&oc))>=0) {
	switch (opt) {
		default:  fprintf(stderr,"Unknown Option %c\n",opt);
		case 'h': usage(argv[0]);
		case 'v': version(argv[0]);
		return 0;

#ifdef YYDEBUG
		case 'd': cexpdebug=1;
		break;
#endif
		case 's': symfile=oc.optarg;
		break;
	}
}

if (argc>oc.optind)
	script=argv[oc.optind];

do {
	if (!(ctx=cexpCreateParserCtx(cexpCreateSymTbl(symfile)))) {
		fprintf(stderr,"Need an elf symbol table file arg\n");
		usage(argv[0]);
		return CEXP_MAIN_NO_SYMS;
	}

	if (!(rval=setjmp(mainContext))) {
		/* call them back to pass the jmpbuf */
		if (callback)
			callback(argc, argv, &mainContext);
	
		if (script) {
			char buf[500]; /* limit line length to 500 chars :-( */
			if (!(scr=fopen(script,"r"))) {
				perror("opening scriptfile");
				rval=CEXP_MAIN_NO_SCRIPT;
				goto cleanup;
			}
			while (fgets(buf,sizeof(buf),scr)) {
				cexpResetParserCtx(ctx,buf);
				cexpparse((void*)ctx);
			}
			fclose(scr);
			scr=0;
		} else {
			tmp = argc>0 ? argv[0] : "Cexp";
			prompt=malloc(strlen(tmp)+2);
			strcpy(prompt,tmp);
			strcat(prompt,">");
			while ((line=readline(prompt))) {
				if (*line) {
					/* skip empty lines */
					cexpResetParserCtx(ctx,line);
					cexpparse((void*)ctx);
					add_history(line);
				}
				free(line); line=0;
			}
		}
		
	} else {
			/* setjmp passes 0: first time
			 *               1: longjmp(buf,0) or longjmp(buf,1)
			 *           other: longjmp(buf,other)
			 */
			rval = (rval<2 ? -1 : CEXP_MAIN_KILLED);
	}
	
cleanup:
		script=0;	/* become interactive if script is killed */
		free(prompt); 			prompt=0;
		free(line);   			line=0;
		cexpFreeParserCtx(ctx); ctx=0;
		if (scr) fclose(scr);	scr=0;
	
} while (-1==rval);

return rval;
}
