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
#include <string.h>
#if defined(__rtems) && !defined(RTEMS_TODO_DONE)
#include "rtems-hackdefs.h"
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef MDBG
#include <mdbg.h>
#endif

/* on RTEMS, this is not always 0, due to strange redirection */
#define STDIN_FD fileno(stdin)

#if defined(USE_GNU_READLINE) /* (recommended, but not reentrant :-() */
#include <readline/readline.h>
#include <readline/history.h>
/* avoid reading curses or terminfo headers */
extern int tgetnum();
#define  readline_r(prompt,context) readline(prompt)
#else  /* dont use READLINE  */

#define add_history(line) do {} while (0)
#define tgetnum(arg) -1

#if defined(USE_RTEMS_SHELL) && defined(__rtems)


extern shell_scanline(char *line, int size, FILE *in, FILE *out);

static char *readline_r(char *prompt, void *context)
{
char *rval=malloc(500);
int	 l;
	if (prompt)
		fputs(prompt,stdout);
	*rval=0;
	if (!shell_scanline(rval,500,stdin,stdout)) {
		free (rval);
		rval=0;
	}
	return rval;
}
#elif defined(USE_TECLA)

#include <libtecla.h>

static char *readline_r(char *prompt, GetLine *gl)
{
char	*rval=0;
char	*l;
int		len;
	if ((l=gl_get_line(gl,prompt,NULL,0)) && (len=strlen(l)) > 0) {
		rval=strdup(l);
	}
	return rval;
}

#else
static char *readline_r(char *prompt, void *context)
{
int ch;
	char *rval=malloc(500),*cp;
	if (prompt)
		fputs(prompt,stdout);

	for (cp=rval; cp<rval+500-1 && (ch=getchar())>=0;) {
			switch (ch) {
				case '\n': *cp=0; return rval;
				case '\b':
					if (cp>rval) {
						cp--;
						fputs("\b ",stdout);
					}
					break;
				default:
					*cp++=ch;
					break;
			}
	}
	return rval;	
}
#endif
#endif

#include <spencer_regexp.h>
#include "cexpmod.h"
#include "vars.h"
#include "context.h"

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
spencer_regexp	*rc=arg;
	if (spencer_regexec(rc,name)) {
		cexpTAPrintInfo(a,f);
		fprintf(f,": %s\n",name);
	}
	return 0;
}

/* alternate entries for the lookup functions */
int
lkup(char *re)
{
extern	CexpSym _cexpSymLookupRegex();
spencer_regexp	*rc=0;
CexpSym 		s;
CexpModule		m;
int				ch=0,tsaved=0;
int				nl;
struct termios	tatts,rawatts;
#ifndef USE_TECLA
struct winsize	win;
#endif

	if (!(rc=spencer_regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return -1;
	}

#ifdef USE_TECLA
	{
	GlTerminalSize ts=gl_terminal_size(cexpContextGetCurrent()->gl, 80, 24);
	nl = ts.nline;
	}
#else
	nl = ioctl(STDIN_FD, TIOCGWINSZ,&win) ? tgetnum("li") : win.ws_row;
#endif

	if (nl<=0)
		nl=24;

	/* TODO should acquire the module readlock around this ?? */
	for (s=0,m=0; !ch && (s=_cexpSymLookupRegex(rc,nl-1,s,0,&m));) {
		unsigned char ans;
		if (!tsaved) {
			tsaved=1;
			if (tcgetattr(STDIN_FD,&tatts)) {
				tsaved=-1;
			} else {
				rawatts=tatts;
				rawatts.c_lflag    &= ~ICANON;
				rawatts.c_cc[VMIN] =  1;
			}
		}
		printf("More (Y/n)?:"); fflush(stdout);
		if (tsaved>0)
			tcsetattr(STDIN_FD,TCSANOW,&rawatts);
		read(STDIN_FD,&ans,1);
		ch=ans;
		if ('Y'==toupper(ch) || '\n'==ch || '\r'==ch)
			ch=0;
		if (tsaved>0)
			tcsetattr(STDIN_FD,TCSANOW,&tatts);
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
int				rval=-1;
CexpModule		mod;

	if (!f) f=stdout;

	if ((v.s=cexpSymLookup(name,&mod))) {
		fprintf(f,"Module '%s' Symbol Table:\n", cexpModuleName(mod));
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
	cexpSymLkAddr(addr, 8, stdout, 0);
	return 0;
}

/* This initialization code should be called exactly once */
int
cexpInit(void *arg)
{
static int done=0;
	if (!done) {
		cexpModuleInitOnce();
		cexpVarInitOnce();
		cexpContextInitOnce();
		done=1;
	}
	return 1;
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
cexp_kill(int doWhat)
{
	longjmp(cexpContextGetCurrent()->jbuf,doWhat);
}

int
cexp_main(int argc, char **argv)
{
	return cexp_main1(argc, argv, 0);
}

/* A stack of Cexp contexts. The purpose of this is
 * to provide unrelated code (such as the disassembler
 * or the above 'kill' routine) access to the main
 * routine's stack frame (i.e. the essential parts of it
 * which are part of the CexpContextRec) in a re-entrant
 * way.
 * Cexp may invoke itself recursively (e.g. to process
 * scripts) and the context info of the nested running
 * instances are linked together using the context's
 * 'next' field.
 * In a multithreaded environment, the global variable
 * 'cexpCurrentContext' must be maintained on a per-thread
 * basis. This requires the use of special OS magic (see
 * context.h).
 */
CexpContextOSD cexpCurrentContext=0;

int
cexp_main1(int argc, char **argv, void (*callback)(int argc, char **argv, CexpContext ctx))
{
CexpContextRec		context;	/* the public parts of this instance's context */
FILE				*scr=0;
char				*line=0,*prompt=0,*tmp;
char				*symfile=0, *script=0;
CexpParserCtx		ctx=0;
int					rval=CEXP_MAIN_INVAL_ARG;
MyGetOptCtxtRec		oc={0}; /* must be initialized */
int					opt;
#ifdef USE_TECLA
#define	rl_context  context.gl
#else
#define rl_context  0
#endif
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

/* make sure vital code is initialized */

{
	static int initialized=0;
	cexpContextRunOnce(&initialized, cexpInit);
}

if (!cexpSystemModule) {
	if (!symfile)
		fprintf(stderr,"Need a symbol file argument\n");
	else if (!cexpModuleLoad(symfile,"SYSTEM"))
		fprintf(stderr,"Unable to load system symbol table\n");
	if (!cexpSystemModule) {
		usage(argv[0]);
		return CEXP_MAIN_NO_SYMS;
	}
}

#ifdef USE_MDBG
mdbgInit();
#endif

/* initialize the public context */
context.next=0;
#ifdef HAVE_BFD_DISASSEMBLER
{
	extern void cexpDisassemblerInit();
	cexpDisassemblerInit(&context.dinfo, stdout);
}
#endif

if (!cexpContextGetCurrent()) {
	/* topmost frame */
#ifdef USE_TECLA
	context.gl = new_GetLine(200,2000);
	if (!context.gl) {
		fprintf(stderr,"Unable to create line editor\n");
		return CEXP_MAIN_NO_MEM;
	}
#endif
	/* register first instance running in this thread's context; */
	cexpContextRegister();
} else {
#ifdef USE_TECLA
	/* re-use caller's line editor */
	context.gl = cexpContextGetCurrent()->gl;
#endif
}
/* push our frame to the top */
context.next=cexpContextGetCurrent();
cexpContextSetCurrent(&context);

do {
	if (!(ctx=cexpCreateParserCtx())) {
		fprintf(stderr,"Unable to create parser context\n");
		usage(argv[0]);
		rval = CEXP_MAIN_NO_MEM;
		goto cleanup;
	}

	if (!(rval=setjmp(context.jbuf))) {
		/* call them back to pass the jmpbuf */
		if (callback)
			callback(argc, argv, &context);
	
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
			while ((line=readline_r(prompt,rl_context))) {
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

/* cexpContextSetCurrent() returns its argument for convenience */
if ( ! cexpContextSetCurrent(cexpContextGetCurrent()->next) ) {
	/* we'll exit the topmost instance */
#ifdef USE_TECLA
	del_GetLine(context.gl);
#endif
	cexpContextUnregister();
}

return rval;
}
