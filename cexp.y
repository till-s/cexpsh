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

#define CONFIG_STRINGS_LIVE_FOREVER

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
	unsigned long	evalInhibit;
} CexpParserCtxRec;

%}
%pure_parser

%union {
	CexpTypedValRec		val;
	CexpSym			sym;	/* a symbol table entry */
	CexpType		typ;
	struct			{
		CexpTypedVal	val;
		char	        *name;
	}			uvar;	/* copy of a UVAR */
	struct			{
		CexpTypedValRec	val;
		CexpBinOp	op;
	}			fixexp;
	unsigned long		ul;
}

%token <val>	NUMBER
%token <val>    STR_CONST
%token <sym>	FUNC VAR
%token <uvar>	IDENT		/* an undefined identifier */
%token <uvar>  	UVAR		/* user variable */
%token		KW_CHAR		/* keyword 'char' */
%token		KW_SHORT	/* keyword 'short' */
%token		KW_LONG		/* keyword 'long' */
%token		KW_DOUBLE	/* keyword 'long' */

%type  <val>	exp
%type  <val>	binexp
%type  <ul>	or
%type  <ul>	and
%type  <val>	unexp
%type  <fixexp> postfix prefix
%type  <val>	lval
%type  <val>	call
%type  <val>	funcp
%type  <val>	castexp

%type  <typ>	fpcast pcast cast typeid

%type  <uvar>   lvar

%nonassoc	NONE
%right		'?' ':'
%right		'='
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
%right		'!' '~' NEG CAST ADDR DEREF
%left		PP MM
%right		PREFIX
%left		CALL

%%

input:	line		{ YYACCEPT; }

line:	'\n'
	|	IDENT '\n'
					{
						yyerror("unknown symbol/variable; '=' expected");
						YYERROR;
					}
	|	exp '\n'
					{
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

lvar:	IDENT | UVAR
;

exp:	binexp 
	|   lval '=' exp
					{ $$=$3; EVAL(CHECK(cexpTVAssign(&$1, &$3))); }
	|   lvar '=' exp
					{ $$=$3; EVAL(if (!($1.val=cexpVarLookup($1.name,1)/*allow creation*/)) {	\
									yyerror("unable to add new user variable");	\
									YYERROR; 								\
								}\
								*$1.val=$3;);
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

prefix: 	MM lval	%prec PREFIX
					{ $$.val=$2; $$.op=OSub; }
	|	PP lval %prec PREFIX
					{ $$.val=$2; $$.op=OAdd; }
;

	
postfix: 	lval MM
					{ $$.val=$1; $$.op=OSub; }
	|	lval PP
					{ $$.val=$1; $$.op=OAdd; }
	|	lval %prec NONE
					{ $$.val=$1; $$.op=ONoop; }
;

unexp:
/*	VAR
					{ CHECK(cexpTVPtrDeref(&$$,&$1->value)); }
*/
		NUMBER
	|	STR_CONST
	|	call
	|	postfix
					{ CexpTypedValRec one;
					  EVAL(
						one.type=TUChar;
						one.tv.c=1;
						CHECK(cexpTVPtrDeref(&$$,&$1.val));
						if (ONoop != $1.op) {
							CHECK(cexpTVBinOp(&one,&$$,&one,$1.op));
							CHECK(cexpTVAssign(&$1.val,&one));
						}
					  );
					}
	|	prefix
					{ CexpTypedValRec one;
					  EVAL(
						one.type=TUChar;
						one.tv.c=1;
						CHECK(cexpTVPtrDeref(&$$,&$1.val));
						if (ONoop != $1.op) {
							CHECK(cexpTVBinOp(&$$,&$$,&one,$1.op));
							CHECK(cexpTVAssign(&$1.val,&$$));
						}
					  );
					}
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
					{ $$=$2->value; }
	|	'&' UVAR %prec ADDR
					{ CHECK(cexpTVPtr(&$$, $2.val)); }
;

lval:	VAR				{ $$=$1->value; }
	|   '*' castexp %prec DEREF
					{ $$=$2; }
	|   '(' castexp ')'
					{ $$=$2; }
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

fpcast:
		'(' typeid '(' '*' ')' '(' ')' ')'
					{ switch ($2) {
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

/* NOTE: for now, we consider it not legal to
 *       deal with pointers to UVARs
 */


funcp:	FUNC	
					{ $$=$1->value; }
	|	UVAR 		
					{$$=*$1.val;}
	|	'&' FUNC %prec ADDR
					{ $$=$2->value; }
;

castexp: unexp
	|	cast	castexp	%prec CAST
					{ $$=$2; CHECK(cexpTypeCast(&$$,$1,CNV_FORCE)); }
	|	pcast	castexp	%prec CAST
					{ $$=$2; CHECK(cexpTypeCast(&$$,$1,CNV_FORCE)); }
	|	fpcast	castexp	%prec CAST
					{ $$=$2; CHECK(cexpTypeCast(&$$,$1,CNV_FORCE)); }
;	


call:	'(' exp ')'
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
		else if ((rval->uvar.val=cexpVarLookup(sbuf,0))) {
#ifdef CONFIG_STRINGS_LIVE_FOREVER
			/* if uvars use the string table, it might be there anyway */
			if (!(rval->uvar.name=cexpStrLookup(sbuf,0)))
#endif
				rval->uvar.name=lstAddString(pa,sbuf);
			return rval->uvar.name ? UVAR : LEXERR;
		}

		/* it's a currently undefined symbol */
		rval->uvar.val=0;
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
/* unfortunately, even %pure_parser doesn't pass us the parser argument along.
 * hence, it is not possible for a reentrant parser to tell us where we
 * should print :-( however, our caller may try to redirect stderr...
 */
fprintf(stderr,"Cexp syntax error: %s\n",msg);
}
