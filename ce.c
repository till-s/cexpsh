#include "cexp.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

extern CexpSymTbl cexpSysSymTbl;

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

int
main(int argc, char **argv)
{
int					fd,opt;
char				*line;
CexpParserArgRec	arg={0};
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

memset(&arg,0,sizeof(arg));

if (argc-optind<1 || (fd=open(argv[optind],O_RDONLY))<0) {
	fprintf(stderr,"Need an elf symbol table file arg\n");
	return 1;
}

fprintf(stderr,"main is at 0x%08x\n",(unsigned long)main);

cexpSysSymTbl=arg.symtbl=cexpSlurpElf(fd);

close(fd);

if (!arg.symtbl) {
	fprintf(stderr,"Error while reading symbol table\n");
	return 1;
}

while ((line=readline("Cexpr>"))) {
	int i;
	arg.chpt=line;
	yyparse((void*)&arg);
	add_history(line);
	free(line);
	while (arg.lstLen)
			free(arg.lineStrTbl[--arg.lstLen]);
}

cexpFreeSymTbl(arg.symtbl);

return 0;
}
