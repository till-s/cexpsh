%{
/* $Id$ */
/* Grammar definition and lexical analyzer for Cexp */
/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */
/* License: GPL, consult http://www.gnu.org for details */
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#define _INSIDE_CEXP_Y
#include "elfsyms.h"
#undef  _INSIDE_CEXP_Y
#include "vars.h"

/* not letting them live makes not much sense */
#ifndef CONFIG_STRINGS_LIVE_FOREVER
#define CONFIG_STRINGS_LIVE_FOREVER
#endif

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

typedef struct CexpParserCtxRec_ {
	CexpSymTbl		symtbl;
	unsigned char	*chpt;
	char			*lineStrTbl[10];	/* allow for 10 strings on one line of input */
	CexpTypedValRec	rval;
	unsigned long	evalInhibit;
} CexpParserCtxRec;

static CexpTypedAddr
varCreate(char *name, CexpType type)
{
CexpTypedAddr rval;
	if (!(rval=cexpVarLookup(name,1)/*allow creation*/)) {
		yyerror("unable to add new user variable");
		return 0;
	}
	rval->type = type;
	if (CEXP_TYPE_PTRQ(type))
		rval->ptv->p=0;
	else switch(type) {
		case TUChar:	rval->ptv->c=0;		break;
		case TUShort:	rval->ptv->s=0;		break;
		case TULong:	rval->ptv->l=0;		break;
		case TFloat:	rval->ptv->f=0.0;	break;
		case TDouble:	rval->ptv->d=0.0;	break;
		default:
			assert(!"unknown type");
	}
	return rval;
}

%}
%pure_parser

%union {
	CexpTypedValRec				val;
	CexpTypedAddrRec			lval;
	CexpSym						sym;	/* a symbol table entry */
	CexpType					typ;
	struct			{
		CexpTypedAddr		plval;
		char	        	*name;
	}							uvar;	/* copy of a UVAR */
	struct			{
		CexpTypedAddrRec	lval;
		CexpBinOp			op;
	}							fixexp;
	unsigned long				ul;
}

%token <val>	NUMBER
%token <val>    STR_CONST
%token <sym>	FUNC VAR
%token <uvar>	IDENT		/* an undefined identifier */
%token <uvar>  	UVAR		/* user variable */
%token			KW_CHAR		/* keyword 'char' */
%token			KW_SHORT	/* keyword 'short' */
%token			KW_LONG		/* keyword 'long' */
%token			KW_DOUBLE	/* keyword 'long' */
%token <typ>	MODOP		/* +=, -= & friends */

%type  <val>	redef line
%type  <val>	commaexp
%type  <val>	exp
%type  <val>	binexp
%type  <ul>		or
%type  <ul>		and
%type  <val>	unexp
%type  <fixexp> postfix prefix
%type  <lval>	lval
%type  <val>	call
%type  <val>	funcp
%type  <val>	castexp

%type  <typ>	fpcast pcast cast typeid fptype

%type  <lval>   clvar lvar

%nonassoc	NONE
%left		','
%right		'?' ':'
%right		'=' MODOP
%left		OR
%left		AND
%left		'|'
%left		'^'
%left		'&'
%left		EQ NE
%left		'<' '>' LE GE
%left		SHL SHR
%left		'-' '+'
%left		'*' '/' '%'
%right		CAST
%right		VARCAST
%right		DEREF
%right		ADDR
%right		PREFIX
%left		MM
%left		PP
%right		NEG
%right		'~'
%right		'!'
%left		CALL

%%

input:	line		{ CexpParserCtx pa=parm; pa->rval=$1; YYACCEPT; }

redef:	typeid UVAR
					{ EVAL($2.plval->type = $1;); CHECK(cexpTA2TV(&$$,$2.plval)); }
	| 	typeid '*' UVAR
					{ EVAL($3.plval->type = CEXP_TYPE_BASE2PTR($1);); CHECK(cexpTA2TV(&$$,$3.plval)); }
	| 	fptype '(' '*' UVAR ')' '(' ')'
					{ EVAL($4.plval->type = $1); CHECK(cexpTA2TV(&$$,$4.plval)); }
	|	typeid IDENT
					{ EVAL(if (!($2.plval = varCreate($2.name, $1))) YYERROR; );
					  CHECK(cexpTA2TV(&$$,$2.plval));
					}
	| 	typeid '*' IDENT
					{ EVAL(if (!($3.plval = varCreate($3.name, CEXP_TYPE_BASE2PTR($1)))) YYERROR; );
					  CHECK(cexpTA2TV(&$$,$3.plval));
					}
	| 	fptype '(' '*' IDENT ')' '(' ')'
					{ EVAL(if (!($4.plval = varCreate($4.name, $1))) YYERROR; );
					  CHECK(cexpTA2TV(&$$,$4.plval));
					}
;

commaexp:	exp
	|	commaexp ',' exp
					{ $$=$3; }
;

line:	'\n'
					{	$$.type=TVoid; }
	|	IDENT '\n'
					{
						$$.type=TVoid;
						yyerror("unknown symbol/variable; '=' expected");
						YYERROR;
					}
	|	redef '\n'
	|	commaexp '\n'
					{	$$=$1;
						if (CEXP_TYPE_FPQ($1.type)) {
							CHECK(cexpTypeCast(&$1,TDouble,0));
							printf("%f\n",$1.tv.d);
						}else {
							if (TUChar==$1.type) {
								unsigned char c=$1.tv.c,e=0;
								printf("0x%02x (%d)",c,c);
								switch (c) {
									case 0:	    e=1; c='0'; break;
									case '\t':	e=1; c='t'; break;
									case '\r':	e=1; c='r'; break;
									case '\n':	e=1; c='n'; break;
									case '\f':	e=1; c='f'; break;
									default: 	break;
								}
								if (isprint(c)) {
									fputc('\'',stdout);
									if (e) fputc('\\',stdout);
									fputc(c,stdout);
									fputc('\'',stdout);
								}
								fputc('\n',stdout);
							} else {
								CHECK(cexpTypeCast(&$1,TULong,0));
								printf("0x%08lx (%ld)\n",$1.tv.l,$1.tv.l);
							}
						}
					}
;

exp:	binexp 
	|   lval  '=' exp
					{ $$=$3; EVAL(CHECK(cexpTVAssign(&$1, &$3))); }
	|   lval  MODOP exp
					{ EVAL( \
						CHECK(cexpTA2TV(&$$,&$1)); \
						CHECK(cexpTVBinOp(&$$, &$$, &$3, $2)); \
						CHECK(cexpTVAssign(&$1,&$$)); \
					  );
					}
	|   IDENT '=' exp
					{ $$=$3; EVAL(if (!($1.plval=varCreate($1.name,$3.type))) {	\
									YYERROR; 								\
								}\
								CHECK(cexpTVAssign($1.plval, &$3)); );
					}
;

binexp:	castexp
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

prefix:	MM lval	%prec PREFIX
					{ $$.lval=$2; $$.op=OSub; }
	|	PP lval %prec PREFIX
					{ $$.lval=$2; $$.op=OAdd; }
;

	
postfix: lval MM
					{ $$.lval=$1; $$.op=OSub; }
	|	lval PP
					{ $$.lval=$1; $$.op=OAdd; }
	|	lval %prec NONE
					{ $$.lval=$1; $$.op=ONoop; }
;

unexp:
/*	VAR
					{ CHECK(cexpTVPtrDeref(&$$,&$1->value)); }
*/
		NUMBER
	|	STR_CONST
	|	call
	|	'!' castexp
					{ $$.type=TULong; $$.tv.l = ! cexpTVTrueQ(&$2); }
	|	'~' castexp
					{ CHECK(cexpTVUnOp(&$$,&$2,OCpl)); }
	|	'-' castexp %prec NEG
					{ CHECK(cexpTVUnOp(&$$,&$2,ONeg)); }
/*
	|	'*' castexp %prec DEREF
					{ CHECK(cexpTVPtrDeref(&$$, &$2)); }
*/
	|	'&' VAR %prec ADDR
					{ CHECK(cexpTVPtr(&$$, &$2->value)); }
	|	'&' UVAR %prec ADDR
					{ CHECK(cexpTVPtr(&$$, $2.plval)); }
;

lvar:	 VAR				
					{ $$=$1->value; }
	|	UVAR		
					{ $$=*$1.plval; }
	|	'(' lval ')'
					{ $$=$2; }
;

clvar:	lvar			%prec NONE
	|	'(' typeid ')' clvar	%prec VARCAST
					{ CexpTypedValRec v;
						v.type=$4.type; v.tv=*$4.ptv;
						CHECK(cexpTypeCast(&v,$2,1));
						$$=$4;
						$$.type=$2;
					}
;

lval: 	clvar
		|   '*' castexp %prec DEREF
					{ if (!CEXP_TYPE_PTRQ($2.type) || CEXP_TYPE_FUNQ($2.type)) {
						yyerror("not a valid lval address");
						YYERROR;
					  }
					  $$.type=CEXP_TYPE_PTR2BASE($2.type);
					  $$.ptv=(CexpVal)$2.tv.p;
					}
/*
	|   	castexp
	|	cast VAR %prec CAST
					{ $$=$2->value; CHECK(cexpTypeCast(&$$,$1,0)); }
*/
;

typeid:	KW_CHAR
					{ $$=TUChar; }
	|	KW_SHORT
					{ $$=TUShort; }
	|	KW_LONG
					{ $$=TULong; }
	|	KW_DOUBLE
					{ $$=TDouble; }
;

cast:	'(' typeid  ')'
					{ $$=$2; }
;

pcast:
		'(' typeid	'*'	 ')'
					{ $$=CEXP_TYPE_BASE2PTR($2); }
;

fptype:	typeid
					{ switch ($1) {
						default:
							yyerror("invalid type for function pointer cast");
						YYERROR;

						case TDouble:
							$$=TDFuncP;
						break;

						case TULong:
							$$=TFuncP;
						break;
					  }
					}
;

fpcast:
		'(' fptype '(' '*' ')' '(' ')' ')'
					{ $$=$2; }
;

funcp:	FUNC	
					{ $$.type=$1->value.type; $$.tv.p=(void*)$1->value.ptv; }
	|	'&' FUNC %prec ADDR
					{ $$.type=$2->value.type; $$.tv.p=(void*)$2->value.ptv; }
	|	postfix
					{ CexpTypedValRec tmp;
					  EVAL( \
						CHECK(cexpTA2TV(&$$,&$1.lval)); \
						tmp.type=TUChar; \
						tmp.tv.c=1; \
						if (ONoop != $1.op) { \
							CHECK(cexpTVBinOp(&tmp,&$$,&tmp,$1.op)); \
							CHECK(cexpTVAssign(&$1.lval,&tmp)); \
						} \
					  );
					}
	|	prefix
					{ CexpTypedValRec tmp;
					  EVAL( \
						CHECK(cexpTA2TV(&$$,&$1.lval)); \
						tmp.type=TUChar; \
						tmp.tv.c=1; \
						if (ONoop != $1.op) { \
							CHECK(cexpTVBinOp(&$$,&$$,&tmp,$1.op)); \
							CHECK(cexpTVAssign(&$1.lval,&$$)); \
						} \
					  );
					}
;

castexp: unexp
	|	cast	castexp	%prec CAST
					{ $$=$2; CHECK(cexpTypeCast(&$$,$1,CNV_FORCE)); }
	|	pcast	castexp	%prec CAST
					{ $$=$2; CHECK(cexpTypeCast(&$$,$1,CNV_FORCE)); }
	|	fpcast	castexp	%prec CAST
					{ $$=$2; CHECK(cexpTypeCast(&$$,$1,CNV_FORCE)); }
;	


call:	'(' commaexp ')'
					{ $$=$2; }
	|	funcp
	|	call '(' ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,0))); }
	|	call '(' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,0))); }
	| 	call '(' exp ',' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,0))); }
	|	call '(' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,&$23,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,&$23,&$25,0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp ',' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,&$23,&$25,&$27,0))); }
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

	if (isdigit(ch) || '\''==ch) {
		/* a number */
		num=0;

		if ('\''==ch) {
			/* char constant */
			getch();
			num=ch;
			if ('\\'==ch) {
				getch();
				/* escape sequence */
				switch (ch) {
					case 't': num='\t'; break;
					case 'n': num='\n'; break;
					case 'r': num='\r'; break;
					case '0': num=0;	break;
					case 'f': num='\f';	break;
					case '\\': num='\\';break;
					case '\'': num='\'';break;
					default:
						yyerror("Warning: unknown escape sequence, using unescaped char");
						num=ch;
					break;
				}
			}
			getch();
			if ('\''!=ch)
				yyerror("Warning: missing closing '");
			else
				getch();
			rval->val.tv.c=(unsigned char)num;
			rval->val.type=TUChar;
			return NUMBER;
		}
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
			return CEXP_TYPE_FUNQ(rval->sym->value.type) ? FUNC : VAR;
		else if ((rval->uvar.plval=cexpVarLookup(sbuf,0))) {
#ifdef CONFIG_STRINGS_LIVE_FOREVER
			/* if uvars use the string table, it might be there anyway */
			if (!(rval->uvar.name=cexpStrLookup(sbuf,0)))
#endif
				rval->uvar.name=lstAddString(pa,sbuf);
			return rval->uvar.name ? UVAR : LEXERR;
		}

		/* it's a currently undefined symbol */
		rval->uvar.plval=0;
		return (rval->uvar.name=lstAddString(pa,sbuf)) ? IDENT : LEXERR;
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
#ifdef CONFIG_STRINGS_LIVE_FOREVER
				rval->val.tv.p=cexpStrLookup(sbuf,1);
#else
				rval->val.tv.p=lstAddString(pa,sbuf);
#endif
				return rval->val.tv.p ? STR_CONST : LEXERR;
			}
		} while (ch && limit>2);
		return LEXERR;
	} else {
		long rv=ch;
		if (rv) getch();

		/* comments? skip the rest of the line */
		if ('#'==rv || ('/'==ch && '/'==rv)) {
			while (ch && '\n'!=rv) {
				rv=ch;
				getch();
			}
			return '\n';
		}

		/* it's any kind of 'special' character such as
		 * an operator etc.
		 */

		/* check for 'double' character operators '&&' '||' '<<' '>>' '==' '!=' '<=' '>=' */
		switch (ch) { /* the second character */
			default: break;

			case '+': if ('+'==rv) rv=PP;  break;
			case '-': if ('-'==rv) rv=MM;  break;

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
					case '+': rv=MODOP; rval->typ=OAdd;	break;
					case '-': rv=MODOP; rval->typ=OSub;	break;
					case '*': rv=MODOP; rval->typ=OMul;	break;
					case '/': rv=MODOP; rval->typ=ODiv;	break;
					case '%': rv=MODOP; rval->typ=OMod;	break;
					case '&': rv=MODOP; rval->typ=OAnd;	break;
					case '^': rv=MODOP; rval->typ=OXor;	break;
					case '|': rv=MODOP; rval->typ=OOr;	break;
				}
			break;
		}
		if (rv>255) getch(); /* skip second char */
		/* yyparse cannot deal with '\0' chars, so we translate it back to '\n'...*/
		if ((SHL==rv || SHR==rv) && '=' == ch) {
			getch();
			rval->typ = (SHL==rv ? OShL : OShR);
			rv=MODOP;
		}
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
/* unfortunately, even %pure_parser doesn't pass us the parser argument along.
 * hence, it is not possible for a reentrant parser to tell us where we
 * should print :-( however, our caller may try to redirect stderr...
 */
fprintf(stderr,"Cexp syntax error: %s\n",msg);
}
