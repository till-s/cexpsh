#include "cexp.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

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

int a,b,c,d,e;

int
main(int argc, char **argv)
{
int					fd,opt;
char				*line;
CexpParserCtx		ctx;
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

if (argc-optind<1 || !(ctx=cexpCreateParserCtx(argv[optind]))) {
	fprintf(stderr,"Need an elf symbol table file arg\n");
	return 1;
}

fprintf(stderr,"main is at 0x%08x\n",(unsigned long)main);

while ((line=readline("Cexpr>"))) {
	cexpResetParserCtx(ctx,line);
	yyparse((void*)ctx);
	add_history(line);
	free(line);
}

cexpFreeParserCtx(ctx);

return 0;
}
