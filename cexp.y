%{
/* $Id$ */
/* Grammar definition and lexical analyzer for Cexp */
/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */

/*
 * Copyright 2002, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#define _INSIDE_CEXP_Y
#include "cexpsyms.h"
#include "cexpmod.h"
#undef  _INSIDE_CEXP_Y
#include "vars.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#define ISIDENTCHAR(ch) ('_'==(ch) || '@'==(ch))

#define LEXERR	-1
/* ugly hack; helper for word completion */
#define LEXERR_INCOMPLETE_STRING	-100

void yyerror();
int  yylex();

typedef char *LString;

typedef struct CexpParserCtxRec_ {
	const char		*chpt;
	LString			lineStrTbl[10];	/* allow for 10 strings on one line of input */
	CexpTypedValRec	rval;
	unsigned long	evalInhibit;
	FILE			*f;				/* where to print evaluated value			*/
} CexpParserCtxRec;

static CexpSym
varCreate(char *name, CexpType type)
{
CexpSym rval;
	if (!(rval=cexpVarLookup(name,1)/*allow creation*/)) {
		yyerror("unable to add new user variable");
		return 0;
	}
	rval->value.type = type;
	if (CEXP_TYPE_PTRQ(type))
		rval->value.ptv->p=0;
	else switch(type) {
		case TUChar:	rval->value.ptv->c=0;		break;
		case TUShort:	rval->value.ptv->s=0;		break;
		case TULong:	rval->value.ptv->l=0;		break;
		case TFloat:	rval->value.ptv->f=0.0;	break;
		case TDouble:	rval->value.ptv->d=0.0;	break;
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
	CexpTypedAddr				varp;
	CexpSym						sym;	/* a symbol table entry */
	CexpType					typ;
	CexpBinOp					binop;
	char						*lstr;	/* string in the line string table */
	struct			{
		CexpTypedAddrRec	lval;
		CexpBinOp			op;
	}							fixexp;
	struct			{
		CexpSym				sym;
		char				*mname;		/* string kept in the line string table */
	}							method;
	unsigned long				ul;
}

%token <val>	NUMBER
%token <val>    STR_CONST
%token <sym>	FUNC VAR UVAR
%token <lstr>	IDENT		/* an undefined identifier */
%token			KW_CHAR		/* keyword 'char' */
%token			KW_SHORT	/* keyword 'short' */
%token			KW_LONG		/* keyword 'long' */
%token			KW_DOUBLE	/* keyword 'long' */
%token <binop>	MODOP		/* +=, -= & friends */

%type  <varp>	anyvar
%type  <val>	def redef newdef line
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
%type  <method> symmethod

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
%left		'.'

%%

input:	line		{ CexpParserCtx pa=parm; pa->rval=$1; YYACCEPT; }
;

redef:	typeid anyvar
					{ EVAL($2->type = $1;); CHECK(cexpTA2TV(&$$,$2)); }
	| 	typeid '*' anyvar
					{ EVAL($3->type = CEXP_TYPE_BASE2PTR($1);); CHECK(cexpTA2TV(&$$,$3)); }
	| 	fptype '(' '*' anyvar ')' '(' ')'
					{ EVAL($4->type = $1); CHECK(cexpTA2TV(&$$,$4)); }
;

newdef: typeid IDENT
					{ CexpSym found;
					  EVAL(if (!(found = varCreate($2, $1))) YYERROR;);
					  CHECK(cexpTA2TV(&$$,&found->value));
					}
	| 	typeid '*' IDENT
					{ CexpSym found;
					  EVAL(if (!(found = varCreate($3, CEXP_TYPE_BASE2PTR($1)))) YYERROR;);
					  CHECK(cexpTA2TV(&$$,&found->value));
					}
	| 	fptype '(' '*' IDENT ')' '(' ')'
					{ CexpSym found;
					  EVAL(if (!(found = varCreate($4, $1))) YYERROR;);
					  CHECK(cexpTA2TV(&$$,&found->value));
					}
;

def: newdef | redef '!'
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
	|	def '\n'
	|   redef '\n'	{
						fprintf(stderr,"Symbol already defined; append '!' to enforce recast\n");
						YYERROR;
					}
	|	commaexp '\n'
					{FILE *f=((CexpParserCtx)parm)->f;
						$$=$1;
						if (CEXP_TYPE_FPQ($1.type)) {
							CHECK(cexpTypeCast(&$1,TDouble,0));
							if (f)
								fprintf(f,"%f\n",$1.tv.d);
						}else {
							if (TUChar==$1.type) {
								unsigned char c=$1.tv.c,e=0;
								if (f) {
									fprintf(f,"0x%02x (%d)",c,c);
									switch (c) {
										case 0:	    e=1; c='0'; break;
										case '\t':	e=1; c='t'; break;
										case '\r':	e=1; c='r'; break;
										case '\n':	e=1; c='n'; break;
										case '\f':	e=1; c='f'; break;
										default: 	break;
									}
									if (isprint(c)) {
										fputc('\'',f);
										if (e) fputc('\\',f);
										fputc(c,f);
										fputc('\'',f);
									}
									fputc('\n',f);
								}
							} else {
								CHECK(cexpTypeCast(&$1,TULong,0));
								if (f)
									fprintf(f,"0x%08lx (%ld)\n",$1.tv.l,$1.tv.l);
							}
						}
					}
;

exp:	binexp 
	|   lval  '=' exp
					{ $$=$3; EVAL(CHECK(cexpTVAssign(&$1, &$3))); }
	|   lval  MODOP	exp
					{ EVAL( \
						CHECK(cexpTA2TV(&$$,&$1)); \
						CHECK(cexpTVBinOp(&$$, &$$, &$3, $2)); \
						CHECK(cexpTVAssign(&$1,&$$)); \
					  );
					}
	|   IDENT '=' exp
					{ CexpSym found;
					  $$=$3; EVAL(if (!(found=varCreate($1, $3.type))) {	\
									YYERROR; 								\
								}\
								CHECK(cexpTVAssign(&found->value, &$3)); );
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
/*
	|	'(' commaexp ')' { $$=$2; }
*/
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
					{ CHECK(cexpTVPtr(&$$, &$2->value)); }
;

lvar:	 VAR				
					{ $$=$1->value; }
	|	UVAR		
					{ $$=$1->value; }
	|	'(' lval ')'
					{ $$=$2; }
;

anyvar:	VAR
					{ $$=&$1->value; }
	|	UVAR
					{ $$=&$1->value; }
	|	FUNC
					{ $$=&$1->value; }
;

clvar:	lvar			%prec NONE
	|	cast clvar	%prec VARCAST
					{ CexpTypedValRec v;
						v.type=$2.type; v.tv=*$2.ptv;
						CHECK(cexpTypeCast(&v,$1,1));
						$$=$2;
						$$.type=$1;
					}
/* TSILL
	|	'(' typeid ')' clvar	%prec VARCAST
					{ CexpTypedValRec v;
						v.type=$4.type; v.tv=*$4.ptv;
						CHECK(cexpTypeCast(&v,$2,1));
						$$=$4;
						$$.type=$2;
					}
*/
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
		%prec CAST	{ $$=$2; }
;

pcast:
		'(' typeid	'*'	 ')'
		%prec CAST	{ $$=CEXP_TYPE_BASE2PTR($2); }
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

symmethod:
		VAR '.' IDENT { $$.sym = $1; $$.mname=$3; }
	|
		UVAR '.' IDENT { $$.sym = $1; $$.mname=$3; }
	|
		FUNC '.' IDENT { $$.sym = $1; $$.mname=$3; }
;



call:
		'(' commaexp ')' %prec CALL{ $$=$2; }
	|	funcp
	|	symmethod '(' ')'
		%prec CALL	{	EVAL(CHECK(cexpSymMember(&$$, $1.sym, $1.mname, 0))); }
	|	symmethod '(' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpSymMember(&$$, $1.sym, $1.mname, &$3, 0))); }
	|	symmethod '(' exp ',' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpSymMember(&$$, $1.sym, $1.mname, &$3, &$5, 0))); }
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
LString
lstAddString(CexpParserCtx env, char *string)
{
LString			rval=0;
LString			*chppt;
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
			return (LString) rval;
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
	if (!isdigit(ch))
		return '.'; /* lonely '.' without attached number is perhaps a field separator */
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
		/* perhaps also a fractional number */
		return
			scanfrac(sbuf,sbuf,limit,rval,pa);
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
		else if ((rval->sym=cexpSymLookup(sbuf, 0)))
			return CEXP_TYPE_FUNQ(rval->sym->value.type) ? FUNC : VAR;
		else if ((rval->sym=cexpVarLookup(sbuf,0))) {
			return UVAR;
		}

		/* it's a currently undefined symbol */
		return (rval->lstr=lstAddString(pa,sbuf)) ? IDENT : LEXERR;
	} else if ('"'==ch) {
		/* generate a string constant */
		char *dst;
		const char *strStart;
		dst=sbuf-1;
		strStart = pa->chpt+1;
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
		return LEXERR_INCOMPLETE_STRING - (pa->chpt - strStart);
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
					case '+': rv=MODOP; rval->binop=OAdd;	break;
					case '-': rv=MODOP; rval->binop=OSub;	break;
					case '*': rv=MODOP; rval->binop=OMul;	break;
					case '/': rv=MODOP; rval->binop=ODiv;	break;
					case '%': rv=MODOP; rval->binop=OMod;	break;
					case '&': rv=MODOP; rval->binop=OAnd;	break;
					case '^': rv=MODOP; rval->binop=OXor;	break;
					case '|': rv=MODOP; rval->binop=OOr;	break;
				}
			break;
		}
		if (rv>255) getch(); /* skip second char */
		/* yyparse cannot deal with '\0' chars, so we translate it back to '\n'...*/
		if ((SHL==rv || SHR==rv) && '=' == ch) {
			getch();
			rval->binop = (SHL==rv ? OShL : OShR);
			rv=MODOP;
		}
		return rv ? rv : '\n';
	}
	return 0; /* seems to mean ERROR/EOF */
}

/* re-initialize a parser context to parse 'buf';
 * If called with a NULL argument, a new
 * context is created and initialized.
 *
 * RETURNS: initialized context
 */

static void
releaseStrings(CexpParserCtx ctx)
{
int			i;
char		**chppt;

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
cexpCreateParserCtx(FILE *f)
{
CexpParserCtx	ctx=0;

	assert(ctx=(CexpParserCtx)malloc(sizeof(*ctx)));
	memset(ctx,0,sizeof(*ctx));
	ctx->f = f;
	return ctx;
}

void
cexpResetParserCtx(CexpParserCtx ctx, const char *buf)
{
	ctx->chpt=buf;
	ctx->evalInhibit=0;
	releaseStrings(ctx);
}

void
cexpFreeParserCtx(CexpParserCtx ctx)
{
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
