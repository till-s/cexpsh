#include "cexp.h"
#include <fcntl.h>
#include <stdio.h>

CexpSymTbl sysSymTbl=0;

int
main(int argc, char **argv)
{
int					fd;
char				*line;
CexpParserArgRec	arg={0};

memset(&arg,0,sizeof(arg));

if (argc<2 || (fd=open(argv[1],O_RDONLY))<0) {
	fprintf(stderr,"Need an elf symbol table file arg\n");
	return 1;
}

sysSymTbl=arg.symtbl=cexpSlurpElf(fd);

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
