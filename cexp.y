%{
/* Grammar definition and lexical analyzer for Cexp */

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

#include <stdio.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "cexpsyms.h"
#include "cexpmod.h"
#include "vars.h"
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* not letting them live makes not much sense */
#ifndef CONFIG_STRINGS_LIVE_FOREVER
#define CONFIG_STRINGS_LIVE_FOREVER
#endif

#define YYLEX_PARAM		ctx
#define YYERROR_VERBOSE

#define EVAL_INH	 ((ctx)->evalInhibit)
#define PSHEVAL(inh) do { EVAL_INH<<=1; if (inh) EVAL_INH++; } while(0)
#define POPEVAL      do { EVAL_INH>>=1; } while(0)
#define EVAL(stuff)  if (! EVAL_INH ) do { stuff; } while (0)

#define CHECK(cexpTfuncall) do { const char *e=(cexpTfuncall);\
					 if (e) { yyerror(ctx, e); YYERROR; } } while (0)

/* acceptable characters for identifiers - must not
 * overlap with operators
 */
#define ISIDENTCHAR(ch) ('_'==(ch) || '@'==(ch))

#define LEXERR	-1
/* ugly hack; helper for word completion */
#define LEXERR_INCOMPLETE_STRING	-100

       void yyerror(CexpParserCtx ctx, const char*msg);
static void errmsg(CexpParserCtx ctx, const char *msg, ...);
static void wrnmsg(CexpParserCtx ctx, const char *msg, ...);

int  yylex();

typedef char *LString;

struct CexpParserCtxRec_;

typedef void (*RedirCb)(struct CexpParserCtxRec_ *, void *);

typedef struct CexpParserCtxRec_ {
	const char		*chpt;
	LString			lineStrTbl[10];	/* allow for 10 strings on one line of input  */
	CexpSymRec		rval_sym;       /* return value and status of last evaluation */
	CexpValU		rval;
	int             status;         
	unsigned long	evalInhibit;
	FILE			*outf;			/* where to print evaluated value			  */
	FILE			*errf;			/* where to print error messages 			  */
	char            sbuf[1000];		/* scratch space for strings                  */
	FILE            *o_stdout;      /* redirection */
	FILE            *o_stderr;      /* redirection */
	FILE            *o_stdin;       /* redirection */
	FILE            *o_outf;
	FILE            *o_errf;
	RedirCb         redir_cb;
	void            *cb_arg;
} CexpParserCtxRec;

static CexpSym
varCreate(CexpParserCtx ctx, char *name, CexpType type)
{
CexpSym rval;
	if (!(rval=cexpVarLookup(name,1)/*allow creation*/)) {
		if ( ctx->errf )
			fprintf(ctx->errf, "unable to add new user variable");
		return 0;
	}
	rval->value.type = type;
	if (CEXP_TYPE_PTRQ(type))
		rval->value.ptv->p=0;
	else switch(type) {
		case TUChar:	rval->value.ptv->c=0;	break;
		case TUShort:	rval->value.ptv->s=0;	break;
		case TUInt:		rval->value.ptv->i=0;	break;
		case TULong:	rval->value.ptv->l=0;	break;
		case TFloat:	rval->value.ptv->f=0.0;	break;
		case TDouble:	rval->value.ptv->d=0.0;	break;
		default:
			assert(!"unknown type");
	}
	return rval;
}

static int
cexpRedir(CexpParserCtx ctx, unsigned long op, void *opath, void *ipath);

static void
cexpUnredir(CexpParserCtx ctx);

/* Redefine so that we can wrap */
#undef yyparse
#define yyparse __cexpparse

%}
%pure-parser

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
	char                        *chrp;
}

%token <val>	NUMBER
%token <val>    STR_CONST
%token <sym>	FUNC VAR UVAR
%token <lstr>	IDENT		/* an undefined identifier */
%token			KW_CHAR		/* keyword 'char' */
%token			KW_SHORT	/* keyword 'short' */
%token			KW_INT		/* keyword 'int' */
%token			KW_LONG		/* keyword 'long' */
%token			KW_FLOAT	/* keyword 'float' */
%token			KW_DOUBLE	/* keyword 'double' */
%token <binop>	MODOP		/* +=, -= & friends */
%token <ul>     REDIR
%token <ul>     REDIRBOTH
%token <ul>     REDIRAPPEND
%token <ul>     REDIRAPPENDBOTH

%type  <varp>	nonfuncvar
%type  <varp>	anyvar
%type  <chrp>   redirarg
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
%type  <ul>     oredirop

%type  <typ>	fpcast pcast cast typeid fptype

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

%parse-param {CexpParserCtx ctx}
%lex-param   {CexpParserCtx ctx}

%%

input:	redir line	{ if ( TVoid != $2.type ) { ctx->rval=$2.tv; ctx->rval_sym.value.type = $2.type; } ctx->status = 0; YYACCEPT; }
;

oredirop: '>'
		{ $$=REDIR; }
	|	SHR
		{ $$=REDIRAPPEND; }
	|	'>' '&'
		{ $$=REDIRBOTH; }
	|	SHR '&'
		{ $$=REDIRAPPENDBOTH; }
;

redirarg: nonfuncvar ':'
		{
			if ( TUCharP != $1->type ) {
				errmsg(ctx, "(bad type): redirector requires string argument\n");
				YYERROR;
			}
			$$ = $1->ptv->p;
		}
	|     STR_CONST  ':'
		{
			$$ = $1.tv.p;
		}
;

redir:  /* nothing */
	|  oredirop redirarg
		{ if ( cexpRedir( ctx, $1, $2,  0 ) ) YYERROR; }
	|  '<'      redirarg
		{ if ( cexpRedir( ctx,  0,  0, $2 ) ) YYERROR; }
	|  '<'      redirarg  oredirop redirarg
		{ if ( cexpRedir( ctx, $3, $4, $2 ) ) YYERROR; }
	|  oredirop redirarg  '<'      redirarg
		{ if ( cexpRedir( ctx, $1, $2, $4 ) ) YYERROR; }
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
					  EVAL(if (!(found = varCreate(ctx, $2, $1))) YYERROR; \
					  		CHECK(cexpTA2TV(&$$,&found->value)) );
					}
	| 	typeid '*' IDENT
					{ CexpSym found;
					  EVAL(if (!(found = varCreate(ctx, $3, CEXP_TYPE_BASE2PTR($1)))) YYERROR; \
					  		CHECK(cexpTA2TV(&$$,&found->value)));
					}
	| 	fptype '(' '*' IDENT ')' '(' ')'
					{ CexpSym found;
					  EVAL(if (!(found = varCreate(ctx, $4, $1))) YYERROR; \
					  		CHECK(cexpTA2TV(&$$,&found->value)));
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
						yyerror(ctx, "unknown symbol/variable; '=' expected");
						YYERROR;
					}
	|	def '\n'
	|   redef '\n'	{
						errmsg(ctx, ": symbol already defined; append '!' to enforce recast\n");
						YYERROR;
					}
	|	commaexp '\n'
					{FILE *f=ctx->outf;
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
									fprintf(f,"0x%0*lx (%ld)\n",(int)(2*sizeof($1.tv.l)), $1.tv.l, $1.tv.l);
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
					  $$=$3; EVAL(if (!(found=varCreate(ctx, $1, $3.type))) {	\
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
	|	'&' nonfuncvar %prec ADDR
					{ CHECK(cexpTVPtr(&$$, $2)); }
;

nonfuncvar: VAR
					{ $$=&$1->value; }
	|       UVAR		
					{ $$=&$1->value; }

anyvar:	nonfuncvar
					{ $$=$1; }
	|	FUNC
					{ $$=&$1->value; }
;

lval: 	nonfuncvar  %prec NONE
					{ $$ = *$1; }
	|   '*' castexp %prec DEREF
					{ if (!CEXP_TYPE_PTRQ($2.type) || CEXP_TYPE_FUNQ($2.type)) {
						yyerror(ctx, "not a valid lval address");
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
	|	KW_INT
					{ $$=TUInt; }
	|	KW_LONG
					{ $$=TULong; }
	|	KW_FLOAT
					{ $$=TFloat; }
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
							yyerror(ctx, "invalid type for function pointer cast");
						YYERROR;

						case TDouble:
							$$=TDFuncP;
						break;

						/* INTFIX */
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
		%prec CALL	{	EVAL(CHECK(cexpSymMember(&$$, $1.sym, $1.mname, (void*)0))); }
	|	symmethod '(' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpSymMember(&$$, $1.sym, $1.mname, &$3, (void*)0))); }
	|	symmethod '(' exp ',' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpSymMember(&$$, $1.sym, $1.mname, &$3, &$5, (void*)0))); }
	|	call '(' ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,(void*)0))); }
	|	call '(' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,(void*)0))); }
	| 	call '(' exp ',' exp ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,&$23,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,&$23,&$25,(void*)0))); }
	|	call '(' exp ',' exp ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp  ',' exp ',' exp ',' exp ',' exp  ')'
		%prec CALL	{	EVAL(CHECK(cexpTVFnCall(&$$,&$1,&$3,&$5,&$7,&$9,&$11,&$13,&$15,&$17,&$19,&$21,&$23,&$25,&$27,(void*)0))); }
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
	if ( env->errf )
		fprintf(env->errf,"Cexp: Line String Table exhausted\n");
	return 0;
}

#define ch ((int)(*pa->chpt))
#define getch() do { (pa->chpt)++;} while(0)

/* helper to save typing */
static int
prerr(CexpParserCtx ctx)
{
	errmsg(ctx, "(yylex): buffer overflow\n");
	return LEXERR;
}

static int
scanfrac(char *buf, char *chpt, int size, YYSTYPE *rval, CexpParserCtx pa, int rejectLonely)
{
int hasE=0;
	/* first, we put ch to the buffer */
	*(chpt++)=(char)ch; size--; /* assume it's still safe */
	getch();
	if ( isdigit(ch) || 'E' == toupper(ch) ) {
		do {
			while(isdigit(ch) && size) {
				*(chpt++)=(char)ch; if (!--size) return prerr(pa);
				getch();
			}
			if (toupper(ch)=='E' && !hasE) {
				*(chpt++)=(char)'E'; if (!--size) return prerr(pa);
				getch();
				if ('-'==ch || '+'==ch) {
					*(chpt++)=(char)ch; if (!--size) return prerr(pa);
					getch();
				}
				hasE=1;
			} else {
		break; /* the loop */
			}
		} while (1);
	} else {
		if ( rejectLonely )
			return '.';
	}
	*chpt=0;
	rval->val.type=TDouble;
	rval->val.tv.d=strtod(buf,&chpt);
	return *chpt ? LEXERR : NUMBER;
}

int
yylex(YYSTYPE *rval, CexpParserCtx pa)
{
unsigned long num;
int           limit=sizeof(pa->sbuf)-1;
char          *chpt;

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
						wrnmsg(pa, ": unknown escape sequence, using unescaped char\n");
						num=ch;
					break;
				}
			}
			getch();
			if ('\''!=ch)
				wrnmsg(pa, ": missing closing '\n");
			else
				getch();
			rval->val.tv.c=(unsigned char)num;
			rval->val.type=TUChar;
			return NUMBER;
		}
		chpt=pa->sbuf;
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
				return scanfrac(pa->sbuf,chpt,limit,rval,pa,0);
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
				return prerr(pa);
			}
			if ('.'==ch) {
				/* it's a fractional number */
				return scanfrac(pa->sbuf,chpt,limit,rval,pa,0);
			}
		}
		rval->val.tv.l=num;
		rval->val.type=TULong;
		return NUMBER;
	} else if ('.'==ch) {
		/* perhaps also a fractional number */
		return
			scanfrac(pa->sbuf,pa->sbuf,limit,rval,pa,1);
	} else if (isalpha(ch) || ISIDENTCHAR(ch)) {
		/* slurp in an identifier */
		chpt=pa->sbuf;
		do {
			*(chpt++)=ch;
			getch();
		} while ((isalnum(ch)||ISIDENTCHAR(ch)) && (--limit > 0));
		*chpt=0;
		if (!limit)
			return prerr(pa);
		/* is it one of the type cast keywords? */
		if (!strcmp(pa->sbuf,"char"))
			return KW_CHAR;
		else if (!strcmp(pa->sbuf,"short"))
			return KW_SHORT;
		else if (!strcmp(pa->sbuf,"int"))
			return KW_INT;
		else if (!strcmp(pa->sbuf,"long"))
			return KW_LONG;
		else if (!strcmp(pa->sbuf,"float"))
			return KW_FLOAT;
		else if (!strcmp(pa->sbuf,"double"))
			return KW_DOUBLE;
		else if ((rval->sym=cexpSymLookup(pa->sbuf, 0)))
			return CEXP_TYPE_FUNQ(rval->sym->value.type) ? FUNC : VAR;
		else if ((rval->sym=cexpVarLookup(pa->sbuf,0))) {
			return UVAR;
		}

		/* it's a currently undefined symbol */
		return (rval->lstr=lstAddString(pa,pa->sbuf)) ? IDENT : LEXERR;
	} else if ('"'==ch) {
		/* generate a string constant */
		char *dst;
		const char *strStart;
		dst=pa->sbuf-1;
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
				rval->val.tv.p=cexpStrLookup(pa->sbuf,1);
#else
				rval->val.tv.p=lstAddString(pa,pa->sbuf);
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
cexpCreateParserCtx(FILE *outf, FILE *errf, RedirCb redir_cb, void *uarg)
{
CexpParserCtx	ctx=0;

	assert(ctx=(CexpParserCtx)malloc(sizeof(*ctx)));
	memset(ctx,0,sizeof(*ctx));
	ctx->rval_sym.value.type = TULong;
	ctx->rval.l              = 0;
	ctx->rval_sym.value.ptv  = &ctx->rval;
	ctx->rval_sym.name       = CEXP_LAST_RESULT_VAR_NAME;
	ctx->rval_sym.size       = sizeof(ctx->rval);
	ctx->rval_sym.flags      = 0;
	ctx->rval_sym.xtra.help  = "value of last evaluated expression";
	ctx->outf                = outf;
	ctx->errf                = errf;
	ctx->status              = -1;
	ctx->o_stdout            = 0;
	ctx->o_stderr            = 0;
	ctx->o_stdin             = 0;
	ctx->o_outf              = 0;
	ctx->o_errf              = 0;
	ctx->redir_cb            = redir_cb;
	ctx->cb_arg              = uarg;

	return ctx;
}

void
cexpResetParserCtx(CexpParserCtx ctx, const char *buf)
{
	ctx->chpt=buf;
	ctx->evalInhibit=0;
	ctx->status = -1;
	cexpUnredir(ctx);
	releaseStrings(ctx);
}

void
cexpFreeParserCtx(CexpParserCtx ctx)
{
	cexpUnredir(ctx);
	releaseStrings(ctx);
	free(ctx);
}

CexpSym
cexpParserCtxGetResult(CexpParserCtx ctx)
{
	return &ctx->rval_sym;
}

int
cexpParserCtxGetStatus(CexpParserCtx ctx)
{
	return ctx->status;
}

void
yyerror(CexpParserCtx ctx, const char*msg)
{
	if ( ctx->errf ) {
		fprintf(ctx->errf,"Cexp syntax error: %s\n", msg);
	}
}

/* Other errors that are not syntax errors */
static void
errmsg(CexpParserCtx ctx, const char *fmt, ...)
{
va_list ap;
	if ( ctx->errf ) {
		fprintf(ctx->errf,"Cexp error ");
		va_start(ap, fmt);
		vfprintf(ctx->errf, fmt, ap); 
		va_end(ap);
	}
}

static void
wrnmsg(CexpParserCtx ctx, const char *fmt, ...)
{
va_list ap;
	if ( ctx->errf ) {
		fprintf(ctx->errf,"Cexp warning ");
		va_start(ap, fmt);
		vfprintf(ctx->errf, fmt, ap); 
		va_end(ap);
	}
}


static int
cexpRedir(CexpParserCtx ctx, unsigned long op, void *oarg, void *iarg)
{
const char *opath = oarg;
const char *ipath = iarg;
FILE       *nof = 0, *nif = 0;
const char *mode;

	if ( !oarg && !iarg ) {
		errmsg(ctx, "(cexpRedir): NO PATH ARGUMENT ??\n");
		return -1;
	}

	if ( opath && (ctx->o_stdout || ctx->o_stderr) ) {
		errmsg(ctx, "(cexpRedir): OUTPUT ALREADY REDIRECTED ??\n");
		return -1;
	}

	if ( ipath && ctx->o_stdin ) {
		errmsg(ctx, "(cexpRedir): INPUT ALREADY REDIRECTED ??\n");
		return -1;
	}

	if ( ipath ) {
		if ( ! (nif = fopen(ipath, "r")) ) {
			if ( ctx->errf )
				fprintf(ctx->errf, "cexpRedir (IN) ERROR - unable to open file: %s\n", strerror(errno));
			return -1;
		}
		ctx->o_stdin = stdin;
		stdin        = nif;
	}

	if ( opath ) {
		if ( REDIRAPPEND == op || REDIRAPPENDBOTH == op )
			mode = "a";
		else
			mode = "w";

		if ( ! (nof = fopen(opath, mode)) ) {
			if ( ctx->errf )
				fprintf(ctx->errf, "cexpRedir (OUT) ERROR - unable to open file: %s\n", strerror(errno));
			if ( nif ) {
				stdin = ctx->o_stdin;
				fclose(nif);
			}
			return -1;
		}
		fflush(stdout);
		ctx->o_stdout = stdout;
		stdout = nof;

		if ( ctx->outf ) {
			fflush(ctx->outf);
			ctx->o_outf = ctx->outf;
			ctx->outf   = nof;
		}

		if ( REDIRBOTH == op || REDIRAPPENDBOTH == op ) {
			fflush(stderr);
			ctx->o_stderr = stderr;
			stderr = nof;

			if ( ctx->errf ) {
				fflush(ctx->errf);
				ctx->o_errf = ctx->errf;
				ctx->errf   = nof;
			}
		}
	}

	if ( ctx->redir_cb )
		ctx->redir_cb(ctx, ctx->cb_arg);

	return 0;
}

static void
cexpUnredir(CexpParserCtx ctx)
{
	if ( ctx->o_stdout ) {
		fclose(stdout);
		stdout = ctx->o_stdout;
		ctx->o_stdout = 0;
	}
	if ( ctx->o_outf ) {
		ctx->outf   = ctx->o_outf;
		ctx->o_outf = 0;
	}

	if ( ctx->o_stderr ) {
		stderr = ctx->o_stderr;
		ctx->o_stderr = 0;
	}
	if ( ctx->o_errf ) {
		ctx->errf   = ctx->o_errf;
		ctx->o_errf = 0;
	}

	if ( ctx->o_stdin ) {
		fclose(stdin);
		stdin = ctx->o_stdin;
		ctx->o_stdin = 0;
	}

	if ( ctx->redir_cb )
		ctx->redir_cb(ctx, ctx->cb_arg);
}

/* Trivial wrapper so that we can make sure
 * redirections are undone always and before
 * cexpparse() returns to the caller.
 */
int
cexpparse(CexpParserCtx ctx)
{
int rval;

	rval = __cexpparse(ctx);	
	cexpUnredir( ctx );

	return rval;
}
