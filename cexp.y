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

#define CHECK(cexpTfuncall) do { const char *e=(cexpTfuncall);\
								 if (e) { yyerror(e); YYERROR; } } while (0)

/* acceptable characters for identifiers - must not
 * overlap with operators
 */
#define ISIDENTCHAR(ch) ('_'==(ch) || '.'==(ch) || '@'==(ch))

#define LEXERR	-1
void yyerror();
int  yylex();

%}
%pure_parser

%union {
	CexpTypedValRec		val;
	CexpSym				sym;	/* a symbol table entry */
	struct			{
		CexpTypedValRec	val;
		char	        *name;
	}					uvar;	/* copy of a UVAR */
	unsigned long		ul;
}

%token <val>	NUMBER
%token <val>    STR_CONST
%token <val>	IDENT		/* an undefined identifier */
%token <sym>	FUNC VAR
%token <uvar>  	UVAR		/* user variable */
%token			KW_CHAR		/* keyword 'char' */
%token			KW_SHORT	/* keyword 'short' */
%token			KW_LONG		/* keyword 'long' */
%token			KW_DOUBLE	/* keyword 'long' */

%type  <val>	exp
%type  <val>	binexp
%type  <ul>		or
%type  <ul>		and
%type  <val>	unexp
%type  <val>	lval
%type  <val>  	ptr
%type  <val>	call
%type  <val>	funcp
/*
%type  <val>    cast

%type  <sym>	var

*/

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
	|		exp '\n'
					{ CexpType t=$1.type;
						if (CEXP_TYPE_FPQ($1.type)) {
							CHECK(cexpTypeCast(&$1,TDouble,0));
							printf("%lf (type 0x%04x\n",$1.tv.d,t);
						}else {
							CHECK(cexpTypeCast(&$1,TULong,0));
							printf("0x%08lx (%ld)\n",$1.tv.l,$1.tv.l);
						}
					}
;

exp:	binexp 
					{ $$ = $1; }
	|	lval '=' exp
					{ $$=$3; EVAL(CHECK(cexpTVAssign(&$1, &$3))); }
/*
	|	IDENT '=' exp
					{ CexpTypedValRec v;
					  $$=$3;
					  v.tv.p=(void*) &$1.tv;
					  v.type=CEXP_TYPE_BASE2PTR($1.type);
					  if (CEXP_TYPE_SIZE($3.val.type) > CEXP_TYPE_SIZE($3.val.type)) {
							yyerror("type mismatch in assignment");
							YYERROR;
					  }
					  EVAL(if (!cexpVarSet($1,$3,1/*allow creation*TSILL/)) {	\
								yyerror("unable to add new user variable");	\
								YYERROR; 								\
							});
					}
	|	UVAR  '=' exp
					{ $$=$3;		EVAL(if (!cexpVarSet($1.name,$3,1/*allow creation*TSILL/)) {	\
												yyerror("unable to set user variable");	\
												YYERROR; 								\
											}); }
*/

;

binexp:	unexp
	|	or  binexp	%prec OR
					{ $$.tv.l = $1 || cexpTVTrueQ(&$2);
					  $$.type = TULong;
					  POPEVAL; }
	|	and binexp	%prec AND
					{ $$.tv.l = $1 && cexpTVTrueQ(&$2);
					  $$.type = TULong;
					  POPEVAL; }
	|	binexp '|' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OOr)); }
	|	binexp '^' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OXor)); }
	|	binexp '&' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OAnd)); }
	|	binexp NE binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,ONe)); }
	|	binexp EQ binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OEq)); }
	|	binexp '>' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OGt)); }
	|	binexp '<' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OLt)); }
	|	binexp LE binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OLe)); }
	|	binexp GE binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OGe)); }
	|	binexp SHL binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OShL)); }
	|	binexp SHR binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OShR)); }
	|	binexp '+' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OAdd)); }
	|	binexp '-' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OSub)); }
	|	binexp '*' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OMul)); }
	|	binexp '/' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,ODiv)); }
	|	binexp '%' binexp
					{ CHECK(cexpTVBinOp(&$$,&$1,&$3,OMod)); }
;

or:		binexp OR
					{ $$=cexpTVTrueQ(&$1); PSHEVAL($$); }
;
	
and:	binexp AND
					{ $$=cexpTVTrueQ(&$1); PSHEVAL( ! $$); }
;
	

unexp:	VAR
					{ CHECK(cexpTVPtrDeref(&$$,&$1->value)); }
	|	UVAR
					{ $$=$1.val; }
	|	NUMBER
					{ $$=$1; }
	|	'(' exp ')'
					{ $$=$2; }
	|   '!' unexp
					{ $$.tv.l = ! cexpTVTrueQ(&$2); }
	|   '~' unexp
					{ CHECK(cexpTVUnOp(&$$,&$2,OCpl)); }
	|   '-' unexp %prec NEG
					{ CHECK(cexpTVUnOp(&$$,&$2,ONeg)); }
	|	'(' KW_CHAR ')' unexp %prec CAST
					{ $$=$4; CHECK(cexpTypeCast(&$$,TUChar,CNV_FORCE)); }
	|	'(' KW_SHORT ')' unexp %prec CAST
					{ $$=$4; CHECK(cexpTypeCast(&$$,TUShort,CNV_FORCE)); }
	|	'(' KW_LONG ')' unexp %prec CAST
					{ $$=$4; CHECK(cexpTypeCast(&$$,TULong,CNV_FORCE)); }
	|	'(' KW_DOUBLE ')' unexp %prec CAST
					{ $$=$4; CHECK(cexpTypeCast(&$$,TDouble,CNV_FORCE)); }
	|	ptr
					{ $$=$1; }
	|	'*' ptr %prec DEREF
					{ CHECK(cexpTVPtrDeref(&$$, &$2)); }
	|	call
					{ $$=$1; }
/*
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
	|	cast
					{ $$=$1; }
*/
;

lval:	VAR			{ $$=$1->value; }
	|   '*' ptr %prec DEREF
					{ $$=$2; }
	|	'(' KW_CHAR ')' VAR %prec CAST
					{ $$=$4->value; CHECK(cexpTypeCast(&$$,TUCharP,CNV_FORCE)); }
	|	'(' KW_SHORT ')' VAR %prec CAST
					{ $$=$4->value; CHECK(cexpTypeCast(&$$,TUShortP,CNV_FORCE)); }
	|	'(' KW_LONG ')' VAR %prec CAST
					{ $$=$4->value; CHECK(cexpTypeCast(&$$,TULongP,CNV_FORCE)); }
	|	'(' KW_DOUBLE ')' VAR %prec CAST
					{ $$=$4->value; CHECK(cexpTypeCast(&$$,TDoubleP,CNV_FORCE)); }
;

ptr:	'&' VAR %prec ADDR
					{ $$=$2->value; }
	|	'(' KW_CHAR '*' ')' unexp %prec CAST
					{ $$=$5; CHECK(cexpTypeCast(&$$,TUCharP,CNV_FORCE)); }
	|	'(' KW_SHORT '*' ')' unexp %prec CAST
					{ $$=$5; CHECK(cexpTypeCast(&$$,TUShortP,CNV_FORCE)); }
	|	'(' KW_LONG '*' ')' unexp %prec CAST
					{ $$=$5; CHECK(cexpTypeCast(&$$,TULongP,CNV_FORCE)); }
	|	'(' KW_DOUBLE '*' ')' unexp %prec CAST
					{ $$=$5; CHECK(cexpTypeCast(&$$,TDoubleP,CNV_FORCE)); }
	|	STR_CONST
					{ $$=$1; }
	|	funcp
					{ $$=$1; }
;

/*
var:	CHAR_VAR	{ $$=$1; }
	|	SHORT_VAR	{ $$=$1; }
	|	VAR			{ $$=$1; }
;

clval:	CHAR_VAR
					{ $$=(unsigned char*)$1->val.addr; }
	|	'(' KW_CHAR ')' var %prec CAST
					{ $$=(unsigned char*)$4->val.addr; }
	|	'*' cptr %prec DEREF
					{ $$=$2; }
;

slval:	SHORT_VAR
					{ $$=(unsigned short*)$1->val.addr; }
	|	'(' KW_SHORT ')' var %prec CAST
					{ $$=(unsigned short*)$4->val.addr; }
	|	'*' sptr %prec DEREF
					{ $$=$2; }
;


llval:	VAR
					{ $$=$1->val.addr; }
	|	'(' KW_LONG ')' var %prec CAST
					{ $$=$4->val.addr; }
	|	'*' lptr %prec DEREF
					{ $$=$2; }
;

cptr:	'&' CHAR_VAR %prec ADDR
					{ $$=(unsigned char*)$2->val.addr; }
	|	'(' KW_CHAR '*' ')' unexp %prec CAST
					{ $$=(unsigned char*)$5; }
	|	STR_CONST
					{ $$=$1; }
;

sptr:	'&' SHORT_VAR %prec ADDR
					{ $$=(unsigned short*)$2->val.addr; }
	|	'(' KW_SHORT '*' ')' unexp %prec CAST
					{ $$=(unsigned short*)$5; }
;
*/

/* NOTE: for now, we consider it not legal to
 *       deal with pointers to UVARs
 */
/*
lptr:	'&' VAR	 %prec ADDR
					{ $$=$2->val.addr; }
	|	'(' KW_LONG '*' ')' unexp %prec CAST
					{ $$=(unsigned long*)$5; }
	|	funcp
					{ $$=(unsigned long*)$1; }
	|	'&' funcp %prec ADDR
					{ $$=(unsigned long*)$2; }
;
*/

funcp:	FUNC	
					{ assert($1->value.type==TFuncP);
					  $$=$1->value; }
;	


call:	funcp '(' ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,0))); }
	|	funcp '(' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,0))); }
	|	funcp '(' exp ',' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,0))); }
	|	funcp '(' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,0))); }
	|	funcp '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,0))); }
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

/* helper to save typing */
static int
prerr(void)
{
	yyerror("yylex: buffer overflow");
	return LEXERR;
}

static int
scanfrac(char *buf, char *chpt, int size, YYSTYPE *rval, CexpParserCtx pa)
{
int hasE=0;
	/* first, we put ch to the buffer */
	*(chpt++)=(char)ch; size--; /* assume it's still safe */
	getch();
	do {
		while(isdigit(ch) && size) {
			*(chpt++)=(char)ch; if (!--size) return prerr();
			getch();
		}
		if (toupper(ch)=='E' && !hasE) {
			*(chpt++)=(char)'E'; if (!--size) return prerr();
			getch();
			if ('-'==ch || '+'==ch) {
				*(chpt++)=(char)ch; if (!--size) return prerr();
				getch();
			}
			hasE=1;
		} else {
	break; /* the loop */
		}
	} while (1);
	*chpt=0;
	rval->val.type=TDouble;
	rval->val.tv.d=strtod(buf,&chpt);
	return *chpt ? LEXERR : NUMBER;
}

int
yylex(YYSTYPE *rval, void *arg)
{
unsigned long	num;
CexpParserCtx 	pa=arg;
char sbuf[80], limit=sizeof(sbuf)-1;
char *chpt;

	while (' '==ch || '\t'==ch)
		getch();

	if (isdigit(ch)) {
		/* a number */
		num=0;
		chpt=sbuf;
		if ('0'==ch) {
			
			/* hex, octal or fractional */
			*(chpt++)=(char)ch; limit--;
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
			} else if ('.'==ch) {
				/* a decimal number */
				return scanfrac(sbuf,chpt,limit,rval,pa);
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
				*(chpt++)=(char)ch; limit--;
				num=10*num+(ch-'0');
				getch();
			} while (isdigit(ch) && limit);
			if (!limit) {
				return prerr();
			}
			if ('.'==ch) {
				/* it's a fractional number */
				return scanfrac(sbuf,chpt,limit,rval,pa);
			}
		}
		rval->val.tv.l=num;
		rval->val.type=TULong;
		return NUMBER;
	} else if ('.'==ch) {
		/* also a fractional number */
		return scanfrac(sbuf,sbuf,limit,rval,pa);
	} else if (isalpha(ch) || ISIDENTCHAR(ch)) {
		/* slurp in an identifier */
		chpt=sbuf;
		do {
			*(chpt++)=ch;
			getch();
		} while ((isalnum(ch)||ISIDENTCHAR(ch)) && (--limit > 0));
		*chpt=0;
		if (!limit)
			return prerr();
		/* is it one of the type cast keywords? */
		if (!strcmp(sbuf,"char"))
			return KW_CHAR;
		else if (!strcmp(sbuf,"short"))
			return KW_SHORT;
		else if (!strcmp(sbuf,"long"))
			return KW_LONG;
		else if (!strcmp(sbuf,"double"))
			return KW_DOUBLE;
		else if ((rval->sym=cexpSymTblLookup(sbuf, pa->symtbl)))
			return CEXP_TYPE_OBJQ(rval->sym->value.type) ? VAR : FUNC;
		else if (cexpVarLookup(sbuf, &rval->uvar.val)) {
			return (rval->uvar.name=lstAddString(pa,sbuf)) ? UVAR : LEXERR;
		}

		/* it's a currently undefined symbol */
		rval->val.type=TUCharP;
		return (rval->val.tv.p=lstAddString(pa,sbuf)) ? IDENT : LEXERR;
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
				rval->val.type=TUCharP;
				return (rval->val.tv.p=lstAddString(pa,sbuf)) ? STR_CONST : LEXERR;
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
