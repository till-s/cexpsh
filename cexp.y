%{
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#define _INSIDE_CEXP_Y
#include "cexp.h"
#undef  _INSIDE_CEXP_Y
#include "vars.h"

#define BOOLTRUE	1
#define YYPARSE_PARAM	parm
#define YYLEX_PARAM		parm
#define YYERROR_VERBOSE

#define EVAL_INH	 (((CexpParserCtx)YYPARSE_PARAM)->evalInhibit)
#define PSHEVAL(inh) do { EVAL_INH<<=1; if (inh) EVAL_INH++; } while(0)
#define POPEVAL      do { EVAL_INH>>=1; } while(0)
#define EVAL(stuff)  if (! EVAL_INH ) do { stuff; } while (0)

#define FILL_FN_ARGS
#ifdef  FILL_FN_ARGS
#define	FILLARG 0
#define FILLER  ,FILLARG
#else
#define FILLARG
#define FILLER
#endif

/* acceptable characters for identifiers - must not
 * overlap with operators
 */
#define ISIDENTCHAR(ch) ('_'==(ch) || '.'==(ch) || '@'==(ch))

#define FILL1	FILLER
#define FILL2	FILL1 FILLER
#define FILL3	FILL2 FILLER
#define FILL4	FILL3 FILLER
#define FILL5	FILL4 FILLER
#define FILL6	FILL5 FILLER
#define FILL7	FILL6 FILLER
#define FILL8	FILL7 FILLER
#define FILL9	FILL8 FILLER
#define FILL10  FILLARG FILL9

#define LEXERR	-1
void yyerror();
int  yylex();
%}
%pure_parser

%union {
	unsigned long	num;	/* a number */
	unsigned char	*caddr;	/* an byte address */
	unsigned short	*saddr;	/* an short address */
	unsigned long	*laddr;	/* an long address */
	CexpFuncPtr		func;	/* function  pointer */
	CexpSym			sym;	/* a symbol table entry */
	struct			{
		char			*name;
		unsigned long	val;	
	}				uvar;	/* copy of a UVAR */
}


%token <num>	NUMBER
%token <caddr>  STR_CONST
%token <caddr>	IDENT		/* an undefined identifier */
%token <uvar>  	UVAR		/* user variable */
/* NOTE: elfsym.c relies on the order of FUNC..VAR */
%token <sym>	FUNC LABEL CHAR_VAR SHORT_VAR VAR
%token			CHAR_CAST	/* keyword 'char' */
%token			SHORT_CAST	/* keyword 'short' */
%token			LONG_CAST	/* keyword 'long' */
%type  <num>	exp
%type  <num>	binexp
%type  <num>	or
%type  <num>	and
%type  <num>	unexp
%type  <caddr>	clval
%type  <saddr>	slval
%type  <laddr>	llval
%type  <caddr>  cptr
%type  <saddr>  sptr
%type  <laddr>	lptr
%type  <sym>	var
%type  <func>	funcp
%type  <num>	call


%right			'?' ':'
%right			'='
%left			OR
%left			AND
%left			'|'
%left			'^'
%left			'&'
%left			EQ NE
%left			'<' '>' LE GE
%left			SHL SHR
%left			'-' '+'
%left			'*' '/' '%'
%right			'!' '~' NEG CAST ADDR DEREF
%left			CALL

%%

input:	line		{ YYACCEPT; }

line:	'\n'
	|		exp '\n' { printf("0x%08lx (%ld)\n",$1,$1); }
;

exp:	binexp 
					{ $$ = $1; }
	|	clval '=' exp
					{ $$=$3&0xff; 	EVAL(*(unsigned char*)$1  = (unsigned char)$$); }
	|	slval '=' exp
					{ $$=$3&0xffff; EVAL(*(unsigned short*)$1 = (unsigned short)$$); }
	|	llval '=' exp
					{ $$=$3; 		EVAL(*$1 = $$); }
	|	IDENT '=' exp
					{ $$=$3;		EVAL(if (!cexpVarSet($1,$3,1/*allow creation*/)) {	\
												yyerror("unable to add new variable");	\
												YYERROR; 								\
											}); }
	|	UVAR  '=' exp
					{ $$=$3;		EVAL(if (!cexpVarSet($1.name,$3,1/*allow creation*/)) {	\
												yyerror("unable to add new variable");	\
												YYERROR; 								\
											}); }

;

binexp:	unexp
	|	or  binexp	%prec OR
					{ $$=($1||$2); POPEVAL; }
	|	and binexp	%prec AND
					{ $$=($1&&$2); POPEVAL; }
	|	binexp '|' binexp
					{ $$=$1|$3; }
	|	binexp '^' binexp
					{ $$=$1^$3; }
	|	binexp '&' binexp
					{ $$=$1&$3; }
	|	binexp NE binexp
					{ $$=($1!=$3 ? BOOLTRUE : 0); }
	|	binexp EQ binexp
					{ $$=($1==$3 ? BOOLTRUE : 0); }
	|	binexp '>' binexp
					{ $$=($1>$3 ? BOOLTRUE : 0); }
	|	binexp '<' binexp
					{ $$=($1<$3 ? BOOLTRUE : 0); }
	|	binexp LE binexp
					{ $$=($1<=$3 ? BOOLTRUE : 0); }
	|	binexp GE binexp
					{ $$=($1>=$3 ? BOOLTRUE : 0); }
	|	binexp SHL binexp
					{ $$=$1<<$3; }
	|	binexp SHR binexp
					{ $$=$1>>$3; }
	|	binexp '+' binexp
					{ $$=$1+$3; }
	|	binexp '-' binexp
					{ $$=$1-$3; }
	|	binexp '*' binexp
					{ $$=$1*$3; }
	|	binexp '/' binexp
					{ $$=$1/$3; }
	|	binexp '%' binexp
					{ $$=$1%$3; }
;

or:		binexp OR
					{ $$=$1; PSHEVAL($$); }
;
	
and:	binexp AND
					{ $$=$1; PSHEVAL( ! $$); }
;
	

unexp:	VAR
					{ $$=*(unsigned long*)$1->val.addr; }
	|	UVAR
					{ $$=$1.val; }
	|	LABEL		
					{ $$=(unsigned long)$1->val.addr; }
	|	NUMBER
					{ $$=$1; }
	|	'(' exp ')'
					{ $$=$2; }
	|   '!' unexp
					{ $$=!$2; }
	|   '~' unexp
					{ $$=~$2; }
	|   '-' unexp %prec NEG
					{ $$=-$2; }
	|	'(' CHAR_CAST ')' unexp
					{ $$=$4 & 0xff; }
	|	'(' SHORT_CAST ')' unexp
					{ $$=$4 & 0xffff; }
	|	'(' LONG_CAST ')' unexp
					{ $$=$4; }
	|	cptr
					{ $$=(unsigned long)$1; }
	|	sptr
					{ $$=(unsigned long)$1; }
	|	lptr
					{ $$=(unsigned long)$1; }
	|	'*' cptr %prec DEREF
					{ $$=*(unsigned char*)$2; }
	|	'*' sptr %prec DEREF
					{ $$=*(unsigned short*)$2; }
	|	'*' lptr %prec DEREF
					{ $$=*$2; }
	|	call
					{ $$=$1; }
;

var:	CHAR_VAR	{ $$=$1; }
	|	SHORT_VAR	{ $$=$1; }
	|	VAR			{ $$=$1; }
;

clval:	CHAR_VAR
					{ $$=(unsigned char*)$1->val.addr; }
	|	'(' CHAR_CAST ')' var %prec CAST
					{ $$=(unsigned char*)$4->val.addr; }
	|	'*' cptr %prec DEREF
					{ $$=$2; }
;

slval:	SHORT_VAR
					{ $$=(unsigned short*)$1->val.addr; }
	|	'(' SHORT_CAST ')' var %prec CAST
					{ $$=(unsigned short*)$4->val.addr; }
	|	'*' sptr %prec DEREF
					{ $$=$2; }
;


llval:	VAR
					{ $$=$1->val.addr; }
	|	'(' LONG_CAST ')' var %prec CAST
					{ $$=$4->val.addr; }
	|	'*' lptr %prec DEREF
					{ $$=$2; }
;

cptr:	'&' CHAR_VAR %prec ADDR
					{ $$=(unsigned char*)$2->val.addr; }
	|	'(' CHAR_CAST '*' ')' unexp %prec CAST
					{ $$=(unsigned char*)$5; }
	|	STR_CONST
					{ $$=$1; }
;

sptr:	'&' SHORT_VAR %prec ADDR
					{ $$=(unsigned short*)$2->val.addr; }
	|	'(' SHORT_CAST '*' ')' unexp %prec CAST
					{ $$=(unsigned short*)$5; }
;

/* NOTE: for now, we consider it not legal to
 *       deal with pointers to UVARs
 */
lptr:	'&' VAR	 %prec ADDR
					{ $$=$2->val.addr; }
	|	'(' LONG_CAST '*' ')' unexp %prec CAST
					{ $$=(unsigned long*)$5; }
	|	funcp
					{ $$=(unsigned long*)$1; }
	|	'&' funcp %prec ADDR
					{ $$=(unsigned long*)$2; }
;

funcp:	FUNC	
					{ $$=$1->val.func; }
;	

call:	funcp '(' ')'
		%prec CALL	{	EVAL($$=$1(FILL10)); }
	|	funcp '(' exp ')'
		%prec CALL	{	EVAL($$=$1($3 FILL9)); }
	|	funcp '(' exp ',' exp ')'
		%prec CALL	{	EVAL($$=$1($3,$5 FILL8)); }
	|	funcp '(' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7 FILL7)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9 FILL6)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9,$11 FILL5)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9,$11,$13 FILL4)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9,$11,$13,$15 FILL3)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9,$11,$13,$15,$17 FILL2)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9,$11,$13,$15,$17,$19 FILL1)); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL($$=$1($3,$5,$7,$9,$11,$13,$15,$17,$19,$21)); }
;

	
%%


/* add a string to the line string table returning its index
 * RETURNS a negative number on error
 */
unsigned char *
lstAddString(CexpParserCtx env, char *string)
{
unsigned char	*rval=0;
char			**chppt;
int				i;
	for (i=0,chppt=env->lineStrTbl;
		 i<sizeof(env->lineStrTbl)/sizeof(env->lineStrTbl[0]);
		 i++,chppt++) {
		if (*chppt) {
			if  (strcmp(string,*chppt))			continue;
			else /* string exists already */	return *chppt;
		}
		/* string exists already */
		if ((rval=malloc(strlen(string)+1))) {
			*chppt=rval;
			strcpy(rval,string);
			return rval;
		}
	}
	fprintf(stderr,"Cexp: Line String Table exhausted\n");
	return 0;
}

#define ch (*pa->chpt)
#define getch() do { (pa->chpt)++;} while(0)

int
yylex(YYSTYPE *rval, void *arg)
{
unsigned long	num;
CexpParserCtx 	pa=arg;
char sbuf[80], limit=sizeof(sbuf)-1;

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
		/* slurp in an identifier */
		char *chpt=sbuf;
		do {
			*(chpt++)=ch;
			getch();
		} while ((isalnum(ch)||ISIDENTCHAR(ch)) && (--limit > 0));
		*chpt=0;
		/* is it one of the type cast keywords? */
		if (!strcmp(sbuf,"char"))
			return CHAR_CAST;
		else if (!strcmp(sbuf,"short"))
			return SHORT_CAST;
		else if (!strcmp(sbuf,"long"))
			return LONG_CAST;
		else if ((rval->sym=cexpSymTblLookup(sbuf, pa->symtbl)))
			return rval->sym->type;
		else if (cexpVarLookup(sbuf, &rval->uvar.val)) {
			return (rval->uvar.name=lstAddString(pa,sbuf)) ? UVAR : LEXERR;
		}

		/* it's a currently undefined symbol */
		return (rval->caddr=lstAddString(pa,sbuf)) ? IDENT : LEXERR;
	} else if ('"'==ch) {
		/* generate a character constant */
		char *dst;
		dst=sbuf-1;
		do {
		skipit:	
			dst++; limit--;
			getch();
			*dst=ch;
			if ('\\'==ch) {
				getch();
				switch (ch) {
					case 'n':	*dst='\n'; goto skipit;
					case 'r':	*dst='\r'; goto skipit;
					case 't':	*dst='\t'; goto skipit;
					case '"':	*dst='"';  goto skipit;
					case '\\':	           goto skipit;
					case '0':	*dst=0;    goto skipit;
					default:
						dst++; limit--; *dst=ch;
						break;
				}
			}
			if ('"'==ch) {
				*dst=0;
				getch();
				return (rval->caddr=lstAddString(pa,sbuf)) ? STR_CONST : LEXERR;
			}
		} while (ch && limit>2);
		return LEXERR;
	} else {
		long rv=ch;
		if (rv) getch();

		/* it's any kind of 'special' character such as
		 * an operator etc.
		 */

		/* check for 'double' character operators '&&' '||' '<<' '>>' '==' '!=' '<=' '>=' */
		switch (ch) { /* the second character */
			default: break;

			case '&': if ('&'==rv) rv=AND; break;
			case '|': if ('|'==rv) rv=OR;  break;

			case '<': if ('<'==rv) rv=SHL; break;
			case '>': if ('>'==rv) rv=SHR; break;

			case '=':
				switch (rv) {
					default: break;
					case '=': rv=EQ;	break;
					case '!': rv=NE;	break;
					case '<': rv=LE;	break;
					case '>': rv=GE;	break;
				}
			break;
		}
		if (rv>255) getch(); /* skip second char */
		/* yyparse cannot deal with '\0' chars, so we translate it back to '\n'...*/
		return rv ? rv : '\n';
	}
	return 0; /* seems to mean ERROR/EOF */
}

/* re-initialize a parser context to parse 'buf';
 * If called with a NULL argument, a new
 * context is created and initialized.
 * Note that the ELF symbol file name is only needed
 * if a new symbol table is created. 
 * As a side effect, the system table (cexpSysSymTbl)
 * is initialized if and only if it has not been
 * created before. OTOH, if the file name is omitted (NULL)
 * and there is a sysSymTbl, that one is used.
 * RETURNS: initialized context
 */

static void
releaseStrings(CexpParserCtx ctx)
{
int			i;
char		**chppt;
	/* make sure the variable facility is initialized */
	cexpVarInit();

	/* release the line string table */
	for (i=0,chppt=ctx->lineStrTbl;
		 i<sizeof(ctx->lineStrTbl)/sizeof(ctx->lineStrTbl[0]);
		 i++,chppt++) {
		if (*chppt) {
			free(*chppt);
			*chppt=0;
		}
	}
}

CexpParserCtx
cexpCreateParserCtx(CexpSymTbl t)
{
CexpParserCtx	ctx=0;

	/* make sure the variable facility is initialized */
	cexpVarInit();

	if (t) {
		assert(ctx=(CexpParserCtx)malloc(sizeof(*ctx)));
		memset(ctx,0,sizeof(*ctx));
		ctx->symtbl = t;
	}
	return ctx;
}

void
cexpResetParserCtx(CexpParserCtx ctx, char *buf)
{
	ctx->chpt=buf;
	ctx->evalInhibit=0;
	releaseStrings(ctx);
}

void
cexpFreeParserCtx(CexpParserCtx ctx)
{
	cexpFreeSymTbl(&ctx->symtbl);
	releaseStrings(ctx);
}

void
yyerror(char*msg)
{
fprintf(stderr,"Cexp syntax error: %s\n",msg);
}
