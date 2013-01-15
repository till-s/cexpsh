/* cexp.c,v 1.42 2004/12/02 21:16:37 till Exp */

/* 'main' program for CEXP which reads lines of input
 * and calls the parser (cexpparse()) on each of them...
 */

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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#if defined(__rtems__)
#if defined(HAVE_RTEMS_H)
#include <rtems.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#else
#include "rtems-hackdefs.h"
#endif
#else  /* __rtems__ */
#include <termios.h>
#include <sys/ioctl.h>
#endif /*__rtems__*/

#ifdef HAVE_SIGNALS
#include <signal.h>
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

#if defined(USE_RTEMS_SHELL) && defined(__rtems__)

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
#elif defined(HAVE_TECLA)

#include "teclastuff.h"
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
#define LINEBUFSZ 500
#define TABSZ     4
static char *readline_r(char *prompt, void *context)
{
int ch = -1,i;
int fd;
struct termios told, tnew;
char *rval,*cp;

	if ( ! (rval=malloc(LINEBUFSZ)) )
		return 0;

	if (prompt) {
		fputs(prompt,stdout);
		fflush(stdout);
	}

	fd = fileno(stdin);
	if ( isatty(fd) && 0 ==tcgetattr(fd, &told) ) {
		tnew = told;
		/* missing in rtems :-(
		cfmakeraw(&tnew);	
		*/
		tnew.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
			            | INLCR | IGNCR | ICRNL | IXON);
		tnew.c_oflag &= ~OPOST;
		tnew.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		tnew.c_cflag &= ~(CSIZE | PARENB);
		tnew.c_cflag |= CS8;

		tnew.c_cc[VMIN]  = 1;
		tnew.c_cc[VTIME] = 0;

		tcsetattr(fd, TCSANOW, &tnew);
	} else {
		fd = -1;
	}

	for (cp=rval; cp<rval+LINEBUFSZ-1 && (ch=getchar())>=0;) {
			const char *back="\b \b";
			switch (ch) {

				case '\r': ch='\n';
					/* fall thru */
				case '\n':
					putchar('\r');
					*cp=0;
					cp = rval + LINEBUFSZ;
				break;

				case '\b':
					if (cp>rval) {
						cp--;
						fputs("\b ",stdout);
						fflush(stdout);
					} else {
						continue;
					}
				break;

				case 21: /* Ctrl-U -- kill entire line */
					while ( cp>rval ) {
						fputs("\b \b",stdout);
						cp--;
					}
					fflush(stdout);
				continue;

				case 4: /* Ctrl-D -- bail */
					ch = -1;
				goto bail;

				case '\t':
					i = TABSZ - ((cp-rval)%TABSZ);
					while ( i-->0 && cp < rval+LINEBUFSZ-1 )
						fputc(*cp++ = ' ', stdout);
				continue;

				default:
					*cp++=ch;
				break;
			}
			putchar(ch);
	}

bail:

	if ( ch < 0 ) {
		free(rval);
		rval = 0;
	}
	if ( fd >= 0 )
		tcsetattr(fd, TCSANOW, &told);
	return rval;	
}
#endif
#endif

#include <cexp_regex.h>
#include "cexpmod.h"
#include "vars.h"
#include "context.h"
#include "cexplock.h"

#include "getopt/mygetopt_r.h"

extern const char *cexp_build_date;

/* A lock for various purposes */
static CexpLock cexpGblLock;

#ifdef YYDEBUG
extern int cexpdebug;
#endif

static void
usage(const char *nm)
{
	fprintf(stderr, "usage: %s [-h] [-v]",nm);
#ifdef YYDEBUG
	fprintf(stderr, " [-d]");
#endif
	fprintf(stderr," [-s <symbol file>]");
	fprintf(stderr," [-a <cpu_arch>]");
	fprintf(stderr," [-p <prompt>]");
	fprintf(stderr," [-c <expression>]");
	fprintf(stderr," [<script file>]\n");
	fprintf(stderr, "       C expression parser and symbol table utility\n");
	fprintf(stderr, "       -h print this message\n");
	fprintf(stderr, "       -I become interactive after processing script or -c\n");
	fprintf(stderr, "       -i do not become interactive after processing script or -c\n");
	fprintf(stderr, "       -v print version information\n");
	fprintf(stderr, "       -p <prompt> set prompt\n");
	fprintf(stderr, "       -c <expression> evaluate expression and return\n");
	fprintf(stderr, "       -q quiet evaluation of scripts\n");
#ifdef HAVE_BFD_DISASSEMBLER
	fprintf(stderr, "       -a set default CPU architecture for disassembler\n");
	fprintf(stderr, "          (effective with builtin symtable only)\n");
#else
	fprintf(stderr, "       -a IGNORED (no BFD disassembler support)\n");
#endif
#ifdef YYDEBUG
	fprintf(stderr, "       -d enable parser debugging messages\n");
#endif
	fprintf(stderr, "       Author:    Till Straumann <strauman@@slac.stanford.edu\n");
	fprintf(stderr, "       Licensing: 'SLAC' license - consult the LICENSE file for details\n");
}

static void
version(const char *nm)
{
	fprintf(stderr,"This is CEXP release $Name$, build date %s\n",cexp_build_date);
}

static void
hello(void)
{
	fprintf(stderr,"Type 'cexpsh.help()' for help (no quotes)\n");
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
varprint(const char *name, CexpSym s, void *arg)
{
FILE		*f=stdout;
cexp_regex	*rc=arg;
	if (cexp_regexec(rc,name)) {
		cexpSymPrintInfo(s,f);
	}
	return 0;
}

/* alternate entries for the lookup functions */
int
lkup(const char *re)
{
extern	CexpSym _cexpSymLookupRegex();
cexp_regex		*rc=0;
CexpSym 		s;
CexpModule		m;
int				ch=0,tsaved=0;
int				nl;
struct termios	tatts,rawatts;
#ifndef HAVE_TECLA
struct winsize	win;
#endif

	if (!(rc=cexp_regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return -1;
	}

	/* TODO should acquire the module readlock around this ?? */
	for (s=0,m=0; !ch ;) {
		unsigned char ans;

#ifdef HAVE_TECLA
		{
		GlTerminalSize	ts;
		ts.nline   = 24;
		ts.ncolumn = 80;
		cexpResizeTerminal(&ts);
		nl = ts.nline;
		}
#else
		nl = ioctl(STDIN_FD, TIOCGWINSZ,&win) ? tgetnum("li") : win.ws_row;
#endif

		if (nl<=0)
			nl=24;

		nl--;

		if (!(s=_cexpSymLookupRegex(rc,&nl,s,stdout,&m)))
			break;

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
		ch = (1 == read(STDIN_FD,&ans,1) ? ans : 'Y');
		if ('Y'==toupper(ch) || '\n'==ch || '\r'==ch)
			ch=0;
		if (tsaved>0)
			tcsetattr(STDIN_FD,TCSANOW,&tatts);
	}
	printf("\nUSER VARIABLES:\n");
	cexpVarWalk(varprint,(void*)rc);

	if (rc) cexp_regfree(rc);
	return 0;
}

#if 0 /* obsoleted by 'help' */
int
whatis(char *name, FILE *f)
{
CexpSym			s;
int				rval=-1;
CexpModule		mod;

	if (!f) f=stdout;

	if ((s=cexpSymLookup(name,&mod))) {
		fprintf(f,"Module '%s' Symbol Table:\n", cexpModuleName(mod));
		cexpSymPrintInfo(s,f);
		rval=0;
	}
	if ((s=cexpVarLookup(name,0))) {
		fprintf(f,"User Variable:\n");
		cexpSymPrintInfo(s,f);
		rval=0;
	}
	return rval;
}
#endif

int
lkaddr(void *addr, int margin)
{
if (!margin)
	margin=5;
	cexpSymLkAddr(addr, margin, stdout, 0);
	return 0;
}

#ifdef HAVE_SIGNALS
#ifdef HAVE_SIGINFO

static void (*the_handler)(int) = 0;

static void
sigact(int signum, siginfo_t *p_info, void *arg)
{
	printf("SEGV address %p\n", p_info->si_addr);
	if ( the_handler )
		the_handler( signum );
}
#endif

static void
siginstall(void (*handler)(int))
{
#ifdef HAVE_SIGINFO
/* A hack (for debugging) -- we wrap the user handler
 * so that we can print the violating address.
 * This is JUST A DEBUGGING device.
 */
struct sigaction a;
	a.sa_sigaction = sigact;
	sigemptyset(&a.sa_mask);
	a.sa_flags     = SA_SIGINFO;
	sigaction(SIGSEGV, &a, 0);
	the_handler = handler;
#else
	signal(SIGSEGV, handler);
#endif
	signal(SIGBUS,  handler);
}
#endif
/* This initialization code should be called exactly once */
int
cexpInit(CexpSigHandlerInstallProc installer)
{
static int done=0;
	if (!done) {
		if ( cexpLockingInitialize() ) {
			fprintf(stderr,"Unable to initialize locking - fatal Error\n");
			fflush(stderr);
			exit(1);
		}
#ifdef HAVE_SIGNALS
		if (!installer)
			installer=siginstall;
#endif
		cexpSigHandlerInstaller=installer;
		cexpModuleInitOnce();
		cexpVarInitOnce();
		if ( cexpContextInitOnce() ) {
			fprintf(stderr,"Unable to initialize context - fatal Error\n");
			fflush(stderr);
			exit(1);
		}
		cexpLockCreate(&cexpGblLock);
		done=1;
	}
	return 1;
}

static char *cexpPrompt=0;

static int setp(const char *new_prompt, char **pres)
{
char   *p = 0;

	if ( !new_prompt || (p = strdup(new_prompt)) ) {
		free(*pres);
		*pres = p;
		return 0;
	}
	return -1;
}

int
cexpSetPrompt(int which, const char *new_prompt)
{
int  rval = -1;
CexpContext c;

	switch ( which ) {
		default: break;

		case CEXP_PROMPT_GBL:
			{

				cexpLock(cexpGblLock);

				rval = setp(new_prompt, &cexpPrompt);

				cexpUnlock(cexpGblLock);
			}
		break;

		case CEXP_PROMPT_LCL:
		case CEXP_PROMPT_THR:
			{
				cexpContextGetCurrent(&c);

				do {
					if ( (rval = setp(new_prompt, &c->prompt)) )
						return rval;
				} while ( (c=c->next) && CEXP_PROMPT_THR == which );
			}
		break;
	}

	return rval;
}

static char *checkPrompt(CexpContext ctx, char **promptp, char *fallback)
{
	char *prompt;

	if ( ! (prompt = ctx->prompt) ) {

		/* If no -p option was used nor a local prompt was set
		 * then use the global prompt
		 */
		cexpLock(cexpGblLock);
		if ( cexpPrompt ) {
			prompt = strdup(cexpPrompt);
		}
		cexpUnlock(cexpGblLock);

		if ( ! prompt ) {
			prompt = malloc(strlen(fallback)+2);
			strcpy(prompt,fallback);
			strcat(prompt,">");
		}
		free( *promptp );
		*promptp = prompt;
	}
	return prompt;
}


/* a wrapper to call cexp main with a variable arglist */
int
cexpsh(char *arg0,...)
{
va_list ap;
int		argc=0;
char	*argv[10]; /* limit to 10 arguments */
char    *p;

	va_start(ap,arg0);

	argv[argc++]="cexp_main";
	if (arg0) {
		argv[argc++]=arg0;

		while ( (p=va_arg(ap,char*)) ) {
			
			if (argc >= sizeof(argv)/sizeof(argv[0])) {
				fprintf(stderr,"cexpsh: too many arguments\n");
				va_end(ap);
				return CEXP_MAIN_INVAL_ARG;
			}

			argv[argc++]=p;
		}
	}

	va_end(ap);
	return cexp_main(argc,argv);
}

CexpSigHandlerInstallProc cexpSigHandlerInstaller=0;

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
CexpContext context;
	cexpContextGetCurrent(&context);
	if ( context )
		longjmp(context->jbuf,doWhat);
}

static void
sighandler(int signum)
{
	cexp_kill(0);
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

extern char *cexpBuiltinCpuArch;

#define ST_DEPTH 10

static char *skipsp(register char *p)
{
	while ( *p && (' '==*p || '\t'==*p ) )
		p++;
	return p; 
}

static char *getp(register char *p, char **endp)
{
char *e,*t;
char term;
int  warn = 0;

	switch ( *p++ ) {
		case '<':
			warn = 1;
		break;

		case '.':
			/* '.' must be separated by space or there must
			 * be a string terminator.
			 */
			if ( !isspace((int)*p) && '\'' != *p && '"' != *p )
				return 0;
		break;

		default: /* not a 'source script' command */
			return 0;
	}

	p = skipsp(p);
	if ( (term='\'') ==  *p || (term='"') == *p ) {
		p++;
		if ( (e = strchr(p,term)) ) {
			/* '..' or ".." commented string -- no escapes supported */
			t = e+1;
			goto check_trailing;
		}
		/* no terminator found; treat normally */
		p--;
	}
	for ( e=p; *e && !isspace((int)*e); e++ )
		;
	t = e;

check_trailing:
	/* is there trailing stuff ? */
	while ( *t ) {
		if ( !isspace((int)*t) )
			return 0;
		t++;
	}
	if ( endp )
		*endp = e;
	if ( warn )
		fprintf(stderr,"WARNING: '<' operator to 'source' scripts is deprecated -- use '.' (followed by blank) instead!\n");
	return p;	
}

static int
process_script(CexpParserCtx ctx, const char *name, int quiet)
{
int  rval = 0;
FILE *filestack[ST_DEPTH];
int  sp=-1;
char buf[500]; /* limit line length to 500 chars :-( */
int  i;
char *endp, *maybecomment;

sprintf(buf,". %s\n",name);

goto skipFirstPrint;

do {
	register char *p;

	if ( !quiet ) {
		for ( i=0; i<=sp; i++ )
			printf("  ");
		printf("%s", buf);
		fflush(stdout);
	}
skipFirstPrint:

	maybecomment = p = skipsp(buf);
    if ( (p = getp(p, &endp)) ) {
		/* tag end of extracted path */
		*endp = 0;
		/* PUSH a new script on the stack */
		if ( !quiet ) {
			for ( i=0; i<=sp; i++ )
				fputc('<',stdout);
			printf("'%s':\n",p);
		}
		if ( ++sp >= ST_DEPTH || !(filestack[sp] = fopen(p,"r")) ) {
			if ( sp >= ST_DEPTH ) {
				fprintf(stderr,"Scripts too deply nested (only %i levels supported)\n",ST_DEPTH);
				rval = CEXP_MAIN_NO_MEM;
			} else {
				rval = CEXP_MAIN_NO_SCRIPT;
				perror("opening script file");
			}
			sp--;
		}
    } else {
		/* handle simple comments as a courtesy... */
		if ( '#' != *maybecomment ) {
			cexpResetParserCtx(ctx,buf);
			cexpparse((void*)ctx);
		}
    }

    while ( sp >= 0 && !fgets(buf, sizeof(buf), filestack[sp] ) ) {
		/* EOF reached; POP script off */
		fclose(filestack[sp]);
		sp--;
    }
} while ( sp >= 0 );
return rval;
}

#ifdef HAVE_TECLA
static void redir_cb(CexpParserCtx ctx, void *uarg)
{
GetLine *gl = uarg;
	gl_change_terminal(gl, stdin, stdout, 0);
}
#else
#define redir_cb 0
#endif

int
cexp_main1(int argc, char **argv, void (*callback)(int argc, char **argv, CexpContext ctx))
{
CexpContextRec		context;	/* the public parts of this instance's context */
CexpContext			myContext;
char				*line=0, *prompt=0, *tmp;
const char			*symfile=0, *script=0, *arg_line = 0 ;
int					rval=CEXP_MAIN_INVAL_ARG, quiet=0;
MyGetOptCtxtRec		oc={0}; /* must be initialized */
int					opt;
int                 become_interactive =
#ifdef  CEXP_INTERACTIVE_DEFAULT
	CEXP_INTERACTIVE_DEFAULT
#else
	0
#endif
	;
#ifdef HAVE_TECLA
#define	rl_context  context.gl
#else
#define rl_context  0
#endif
char				optstr[]={
						'h',
						'i',
						'I',
						'v',
						's',':',
						'a',':',
						'c',':',
						'p',':',
#ifdef YYDEBUG
						'd',
#endif
						'q',
						'\0'
					};

context.prompt = 0;
context.parser = 0;

while ((opt=mygetopt_r(argc, argv, optstr,&oc))>=0) {
	switch (opt) {
		default:  fprintf(stderr,"Unknown Option %c\n",opt);
		case 'h': usage(argv[0]);
		case 'v': version(argv[0]);
			return 0;

		case 'i':
			become_interactive = 0;
		break;

		case 'I':
			become_interactive = 1;
		break;

#ifdef YYDEBUG
		case 'd': cexpdebug=1;
			break;
#endif
		case 'q': quiet=1;
			break;
		case 's': symfile=oc.optarg;
			break;
		case 'c': arg_line=oc.optarg;
			break;
		case 'a': cexpBuiltinCpuArch = oc.optarg;
			break;

		case 'p': free(context.prompt); context.prompt = strdup(oc.optarg);
			break;
	}
}

if (argc>oc.optind)
	script=argv[oc.optind];

if ( script && arg_line ) {
	fprintf(stderr,"Cannot use both: -c option and a script\n");
	return -1;
}

/* make sure vital code is initialized */

{
	static int initialized=0;
	cexpContextRunOnce(&initialized, cexpInit);
}

if (!cexpSystemModule) {
	int tried_builtin = 0;
	if (!symfile) {
		/* try to find a builtin table */
		if ( !cexpModuleLoad(0,0) ) {
			tried_builtin = 1;
			/* Try the program name as a fallback */
			symfile = argv[0];
		}
	} 
	if ( symfile && !cexpModuleLoad(symfile,"SYSTEM")) {
		if ( tried_builtin ) {
			fprintf(stderr,"Unable to load system symbol table\n");
		} else {
			fprintf(stderr,"No builtin symbol table -- need a symbol file argument\n");
		}
	}
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

cexpContextGetCurrent(&myContext);

if (!myContext) {
	/* topmost frame */
#ifdef HAVE_TECLA
	context.gl = new_GetLine(200,2000);
	if (!context.gl) {
		fprintf(stderr,"Unable to create line editor\n");
		return CEXP_MAIN_NO_MEM;
	}
	/* mute warnings about being unable to 
	 * read ~/.teclarc
	 */
	gl_configure_getline(context.gl,0,0,0);
#endif
	/* register first instance running in this thread's context; */
	cexpContextRegister();
	if (!quiet)
		hello();
} else {
#ifdef HAVE_TECLA
	/* re-use caller's line editor */
	context.gl = myContext->gl;
#endif
}
/* push our frame to the top */
context.next = myContext;
myContext	 = &context;
cexpContextSetCurrent(myContext);

/* See if there is an ancestor with a local prompt
 * and inherit
 */
if ( !context.prompt && context.next && context.next->prompt )
	context.prompt = strdup(context.next->prompt);

do {
	if (!(context.parser=cexpCreateParserCtx(quiet ? 0 : stdout, stderr, redir_cb, rl_context))) {
		fprintf(stderr,"Unable to create parser context\n");
		usage(argv[0]);
		rval = CEXP_MAIN_NO_MEM;
		goto cleanup;
	}

#ifdef HAVE_TECLA
	{
	CPL_MATCH_FN(cexpSymComplete);
	gl_customize_completion(context.gl, context.parser, cexpSymComplete);
	}
#endif

	if (cexpSigHandlerInstaller)
		cexpSigHandlerInstaller(sighandler);

	if (!(rval=setjmp(context.jbuf))) {
		/* call them back to pass the jmpbuf */
		if (callback)
			callback(argc, argv, &context);
	
		if (script) {
			if ( (rval = process_script(context.parser, script, quiet)) )
				goto cleanup;
		} else if (arg_line) {
			cexpResetParserCtx(context.parser,arg_line);
			cexpparse((void*)context.parser);
		} else {

			while ( (line=readline_r(
							checkPrompt( &context, &prompt, argc > 0 ? argv[0] : "Cexp" ),
							rl_context)) ) {
				/* skip empty lines */
				if (*line) {
					tmp = skipsp(line);
					if ( (tmp = getp(tmp, 0)) ) {
						process_script(context.parser,tmp,quiet);
					} else {
						/* interactively process this line */
						cexpResetParserCtx(context.parser,line);
						cexpparse((void*)context.parser);
						add_history(line);
					}
				}
				free(line); line=0;
			}
		}
	} else {
			fprintf(stderr,"\nOops, exception caught\n");
			/* setjmp passes 0: first time
			 *               1: longjmp(buf,0) or longjmp(buf,1)
			 *           other: longjmp(buf,other)
			 */
			rval = (rval<2 ? -1 : CEXP_MAIN_KILLED);
	}
	
cleanup:
		script=0;	   /* become interactive if script is killed     */
		arg_line=0;	   /* become interactive if expression is killed */
		free(line);   			line=0;
		free(prompt);           prompt=0;
		cexpFreeParserCtx(context.parser); context.parser=0;

		if ( become_interactive ) {
			rval = -1;
			become_interactive = 0;
		}
} while ( -1==rval );

free(context.prompt);

/* pop our stack context from the chained list anchored 
 * at the running thread
 */
myContext = myContext->next;
cexpContextSetCurrent(myContext);
if ( ! myContext ) {
	/* we'll exit the topmost instance */
#ifdef HAVE_TECLA
	del_GetLine(context.gl);
#endif
	cexpContextUnregister();
}

return rval;
}

/* file utility stuff */


#ifdef __rtems__
/* copy a file to the '/tmp' directory residing in IMFS
 * where it can be seeked etc.
 */
static FILE *
copyFileToTmp(int fd, char *tmpfname)
{
FILE		*rval=0,*infile=0,*outfile=0;
char		buf[BUFSIZ];
int			nread,nwritten;
struct stat	stbuf;

	/* a hack (rtems) to make sure the /tmp dir is present */
	if (stat("/tmp",&stbuf)) {
		mode_t old=umask(0);
		mkdir("/tmp",0777);
		umask(old);
	}

#if 0 /* sigh - there is a bug in RTEMS/IMFS; the memory of a
		 tmpfile() is actually never released, so we work around
		 this...
	   */
	/* open files; the tmpfile will be removed by the system
	 * as soon as it's closed
	 */
	if (!(outfile=tmpfile()) || ferror(outfile)) {
		perror("creating scratch file");
		goto cleanup;
	}
#else
	{
	int	fd;
	if ( (fd=mkstemp(tmpfname)) < 0 ) {
		perror("creating scratch file");
		goto cleanup;
	}
	if ( !(outfile=fdopen(fd,"w+")) ) {
		perror("opening scratch file");
		close(fd);
		unlink(tmpfname);
		goto cleanup;
	}
	}
#endif

	if (!(infile=fdopen(fd,"r")) || ferror(infile)) {
		perror("opening object file");
		goto cleanup;
	}

	/* do the copy */
	do {
		nread=fread(buf,1,sizeof(buf),infile);
		if (nread!=sizeof(buf) && ferror(infile)) {
			perror("reading object file");
			goto cleanup;
		}

		nwritten=fwrite(buf,1,nread,outfile);
		if (nwritten!=nread) {
			if (ferror(outfile))
				perror("writing scratch file");
			else
				fprintf(stderr,"Error writing scratch file");
			goto cleanup;
		}
	} while (!feof(infile));

	fflush(outfile);
	rewind(outfile);

	/* success; remap pointers and fall thru */
	rval=outfile;
	outfile=0;

cleanup:
	if (outfile) {
		fclose(outfile);
		unlink(tmpfname);
	}
	if (infile)
		fclose(infile);
	else
		close(fd);
	return rval;
}
#endif

	
/* NOTE: fullname is a buffer of at least MAXPATHLEN chars
 *       tmpfname, if non-NULL must hold a template name (mkstemp)
 */
FILE *
cexpSearchFile(const char *path, const char *fname, char **pfullname, char *tmpfname)
{
FILE        *f = 0;
const char  *col;
char        *thename=0;
char        *fullname;
#ifdef __rtems__
struct stat	dummybuf;
int		    is_on_tftpfs;
#endif

	/* Search relative file in the PATH */
	if ( !pfullname || !(fullname = *pfullname) ) {
		if ( !(thename = malloc(MAXPATHLEN)) )
			return 0;
		fullname = thename;
	}

	if ( strchr(fname,'/') || !path )
		path = "";
	else {
		/* no '/' AND path set; handle special case
		 * of empty path -> they must specify a path
		 */
		if ( !*path ) {
			path = 0;
		}
	}

	col = path ? path-1 : 0;

	while ( !f && col ) {
	path = col+1;
	/* 'path' is never NULL; either "" (absolute file or env var not set)
     * or a colon separated list.
	 * If there is no '.' or empty element in the path, the current
	 * directory is NOT searched (e.g. PATH=/a/b/c/d searches only
	 * /a/b/c/d, PATH=/a/b/c/d: searches /a/b/c/d, then '.'
	 */
	if ( (col = strchr(path,':')) ) {
		strncpy(fullname,path,col-path);
		fullname[col-path] = 0;
	} else {
		strcpy(fullname,path);
	}
	/* don't append a separator if the prefix is empty */
	if ( *fullname )
		strcat(fullname,"/");
	strcat(fullname, fname);
		
#ifdef __rtems__
	/* The RTEMS TFTPfs is strictly
	 * sequential access (no lseek(), no stat()). Hence, we copy
	 * the file to scratch file in memory (IMFS).
	 */

	/* we assume that a file we cannot stat() but open() is on TFTPfs */
	is_on_tftpfs = stat(fullname, &dummybuf) ? open(fullname,O_RDONLY) : -1;
	if ( is_on_tftpfs >= 0 && tmpfname ) {
		if ( ! (f=copyFileToTmp(is_on_tftpfs, tmpfname)) ) {
			/* file was found but couldn't make copy; this is an error */
			break;
		}
		strcpy(fullname, tmpfname);
	} else
#endif
	if (  ! (f=fopen(fullname,"r")) ) {
		if ( errno != ENOENT ) {
			/* stop searching; file seems to be there but unreadable */
			break;
		}
	} else {
		if ( pfullname && ! *pfullname ) {
			*pfullname = thename;
			thename    = 0;
		}
	}
	}
	free(thename);
	return f;
}
