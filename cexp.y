%{
#include <stdio.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#define _INSIDE_CEXP_Y
#include "cexp.h"
#undef  _INSIDE_CEXP_Y
#define BOOLTRUE	1
#define YYPARSE_PARAM		parm
#define YYLEX_PARAM		parm
#define YYERROR_VERBOSE
%}
%pure_parser

%union {
	unsigned long	num;	/* a number */
	unsigned char	*caddr;	/* an byte address */
	unsigned short	*saddr;	/* an short address */
	unsigned long	*laddr;	/* an long address */
	unsigned char	*id;	/* an undefined identifier */
	CexpFuncPtr		func;	/* function  pointer */
	CexpSym			sym;	/* a symbol table entry */
}


%token <num>	NUMBER
%token <caddr>  CHAR_CONST
%token <id>		IDENT
%token <sym>	VAR FUNC CHAR_VAR SHORT_VAR LABEL
%token			CHAR_CAST	/* keyword 'char' */
%token			SHORT_CAST	/* keyword 'short' */
%token			LONG_CAST	/* keyword 'long' */
%type  <num>	exp			/* expression */
%type  <caddr>	clval
%type  <caddr>  cptr
%type  <saddr>	slval
%type  <saddr>  sptr
%type  <laddr>	llval
%type  <laddr>  lptr
%type  <num>	bool
%type  <func>	funcp
%type  <num>	call
%type  <sym>	var

%left			NONE
%right			'='
%left			'|'
%left			'^'
%left			'&'
%left			EQ NE
%left			'<' '>' LE GE
%left			SHFT
%left			'-' '+'
%left			'*' '/' '%'
%right			'!' '~' NEG CAST ADDR DEREF
%left			CALL

%%

input:	line		{ YYACCEPT; }

line:	'\n'
	|	exp '\n' { printf("0x%08x (%i)\n",$1,$1); }
;

exp:	NUMBER					{ $$=$1; }
	|	LABEL					{ $$=(unsigned long)$1->val.addr; }
	|	cptr					{ $$=(unsigned long)$1; }
	|	sptr					{ $$=(unsigned long)$1; }
	|	lptr					{ $$=(unsigned long)$1; }
	|	clval	%prec NONE		{ $$=*$1; }
	|	slval	%prec NONE		{ $$=*$1; }
	|	llval	%prec NONE		{ $$=*$1; }
	|	clval '=' exp			{ $$=$3; *$1=(unsigned char)$3; }
	|	slval '=' exp			{ $$=$3; *$1=(unsigned short)$3; }
	|	llval '=' exp			{ $$=$3; *$1=(unsigned long)$3; }
	|	exp '|' exp				{ $$=$1|$3; }
	|	exp '^' exp				{ $$=$1^$3; }
	|	exp '&' exp				{ $$=$1&$3; }
	|	bool					{ $$=$1; }
	|   exp '<' '<' exp %prec SHFT { $$=($1<<$4); }
	|   exp '>' '>' exp %prec SHFT { $$=($1>>$4); }
	|	exp '+' exp				{ $$=$1+$3; }
	|	exp '-' exp				{ $$=$1+$3; }
	|	exp '*' exp				{ $$=$1+$3; }
	|	exp '/' exp				{ $$=$1+$3; }
	|	exp '%' exp				{ $$=$1+$3; }
	|	'-' exp %prec NEG 		{ $$=-$2; }
	|	'~' exp 				{ $$=~$2; }
	|	'(' exp ')'				{ $$=$2; }
	|	call					{ $$=$1; }
;

funcp:	FUNC			{ $$=$1->val.func; }
;	

call:	funcp '(' ')'
		%prec CALL	{	$$=$1(); }
	|	funcp '(' exp ')'
		%prec CALL	{	$$=$1($3); }
	|	funcp '(' exp ',' exp ')'
		%prec CALL	{	$$=$1($3,$5); }
	|	funcp '(' exp ',' exp ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9,$11); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9,$11,$13); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9,$11,$13,$15); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9,$11,$13,$15,$17); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9,$11,$13,$15,$17,$19); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	$$=$1($3,$5,$7,$9,$11,$13,$15,$17,$19,$21); }
;

bool: 	'!' exp			{ $$=($2==0 ? BOOLTRUE : 0); }
	|	exp '<' exp		{ $$=($1<$3 ? BOOLTRUE : 0); }
	|	exp '<' '=' exp	%prec LE	{ $$=($1<=$4 ? BOOLTRUE : 0); }
	|	exp '=' '=' exp	%prec EQ	{ $$=($1==$4 ? BOOLTRUE : 0); }
	|	exp '!' '=' exp	%prec NE	{ $$=($1!=$4 ? BOOLTRUE : 0); }
	|	exp '>' '=' exp	%prec GE	{ $$=($1>=$4 ? BOOLTRUE : 0); }
	|	exp '>' exp		{ $$=($1>$3 ? BOOLTRUE : 0); }
;

var:	CHAR_VAR		{ $$=$1; }
	|	SHORT_VAR		{ $$=$1; }
	|	VAR				{ $$=$1; }
;


clval:	CHAR_VAR								{ $$=(unsigned char*)$1->val.addr; }
	|	'(' CHAR_CAST ')' var %prec CAST		{ $$=(unsigned char*)$4->val.addr; }
	|	'*' cptr %prec DEREF					{ $$=$2; }
;
slval:	SHORT_VAR								{ $$=(unsigned short*)$1->val.addr; }
	|	'(' SHORT_CAST ')' var %prec CAST		{ $$=(unsigned short*)$4->val.addr; }
	|	'*' sptr %prec DEREF					{ $$=$2; }
;
llval:	VAR										{ $$=$1->val.addr; }
	|	'(' LONG_CAST ')' var %prec CAST		{ $$=$4->val.addr; }
	|	'*' lptr %prec DEREF					{ $$=$2; }
;

cptr:	'&' CHAR_VAR %prec ADDR					{ $$=(unsigned char*)$2->val.addr; }
	|	'(' CHAR_CAST '*' ')' exp %prec CAST	{ $$=(unsigned char*)$5; }
	|	CHAR_CONST								{ $$=$1 }
;

sptr:	'&' SHORT_VAR %prec ADDR				{ $$=(unsigned short*)$2->val.addr; }
	|	'(' SHORT_CAST '*' ')' exp %prec CAST	{ $$=(unsigned short*)$5; }
;

lptr:	'&' VAR	 %prec ADDR						{ $$=$2->val.addr; }
	|	'(' LONG_CAST '*' ')' exp %prec CAST 	{ $$=(unsigned long*)$5; }
	|	FUNC									{ $$=(unsigned long*)$1; }
;

		
%%
#if 0
static unsigned long tstvar=0xdeadbeef;

static unsigned long
tstfunc(unsigned long arg)
{
	return arg;
}

static CexpSymRec tstStab[]={
	{
		"var", VAR, {addr: &tstvar }
	},
	{
		"func", FUNC, {func: tstfunc}
	},
	{
		0, 0, {0}
	},
};

static CexpSym
lookup(char *name)
{
CexpSym rval;
	for (rval=tstStab; rval->name; rval++)
		if (!strcmp(rval->name, name))
			return rval;
	return 0;
}
#endif



#define ch (*pa->chpt)
#define getch() do { (pa->chpt)++;} while(0)

int
yylex(YYSTYPE *rval, void *arg)
{
unsigned long	num;
CexpParserArg 	pa=arg;

	while (' '==ch || '\t'==ch)
		getch();

	if (isdigit(ch)) {
		/* a number */
		num=0;
		if ('0'==ch) {
			/* hex or octal */
			getch();
			if ('x'==ch) {
				/* a hex number */
				getch();
				while (isxdigit(ch)) {
					num<<=4;
					if (ch>='a')		num+=ch-'a'+10;
					else if (ch>='A')	num+=ch-'A'+10;
					else				num+=ch-'0';
					getch();
				}
			} else {
				/* OK, it's octal */
				while ('0'<=ch && ch<'8') {
					num<<=3;
					num+=ch-'0';
					getch();
				}
			}
		} else {
			/* so it must be base 10 */
			do {
				num=10*num+(ch-'0');
				getch();
			} while (isdigit(ch));
		}
		rval->num=num;
		return NUMBER;
	} else if (isalpha(ch)) {
		char idbuf[80], limit=sizeof(idbuf)-1;
		/* slurp in an identifier */
		char *chpt=idbuf;
		do {
			*(chpt++)=ch;
			getch();
		} while (isalnum(ch) && (--limit > 0));
		*chpt=0;
		/* is it one of the type cast keywords? */
		if (!strcmp(idbuf,"char"))
			return CHAR_CAST;
		else if (!strcmp(idbuf,"short"))
			return SHORT_CAST;
		else if (!strcmp(idbuf,"long"))
			return LONG_CAST;
		else if (rval->sym=cexpSymTblLookup(pa->symtbl, idbuf))
			return rval->sym->type;

		/* it's a currently undefined symbol */
		rval->id=idbuf;
		return IDENT;
	} else if ('"'==ch) {
		/* generate a character constant */
		/* TODO allocate new string */
		do {
			getch();
			*dst=ch;
			if ('\'==ch) {
				getch();
				switch (ch) {
					case 'n':	*dst='\n'; break;
					case 'r':	*dst='\r'; break;
					case 't':	*dst='\t'; break;
					case '"':	*dst='"';  break;
					case '\':	           break;
					case '0':	*dst=0;    break;
					default:
						dst++; *dst=ch;
						break;
				}
			}
		} while (ch && '"'!=ch);
	} else {
		/* it's any kind of 'special' character such as
		 * an operator etc.
		 */
		long rv=ch;
		if (rv) getch();
		/* yyparse cannot deal with '\0' chars, so we translate it back to '\n'...*/
		return rv ? rv : '\n';
	}
	return 0; /* seems to mean ERROR */
}

int
yyerror(char*msg)
{
fprintf(stderr,"Cexp syntax error: %s\n",msg);
}

