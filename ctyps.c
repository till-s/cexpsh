/* $Id$ */

/* This file contains the implementation of support for elementary
 * C types (unsigned long, double etc.).
 */

/* Author: Till Straumann <strauman@slac.stanford.edu> */

/* License: GPL, http://www.gnu.org */

#include <assert.h>
#include <stdlib.h>

#include "ctyps.h"

static char *fundesc[]={
		"long (*)()  ",
		"double (*)()",
};

static char *desc[]={
		"void        ",		/* function pointer */
		"void*       ",		/* void*            */
		"uchar       ",		/* unsigned char    */
		"uchar*      ",		/* unsigned char*   */
		"ushort      ",		/* unsigned short   */
		"ushort*     ",		/* unsigned short*  */
		"ulong       ",		/* unsigned long    */
		"ulong*      ",		/* unsigned long*   */
		"float       ",		/* float            */
		"float*      ",		/* float*           */
		"double      ",		/* double           */
		"double*     ",		/* double*          */
};


const char *
cexpTypeInfoString(CexpType to)
{
int t=0; /* keep compiler happy */
	if (CEXP_TYPE_FUNQ(to)) {
		switch (to) {
			case TFuncP:  t=0; break;
			case TDFuncP: t=1; break;
			default:
				assert(0=="Unknown funcion Pointer");
		}
		return fundesc[t];
	}

	t=CEXP_TYPE_MASK_SIZE(to);
	assert(t>=0 && t<sizeof(desc)/sizeof(desc[0]));
	return desc[t];
}

/* build a converter matrix */
typedef void (*Converter)(CexpTypedVal);

/* all possible type conversions, even dangerous ones */
static void c2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.c; }
static void c2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.c; }
static void c2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.c; }
static void c2p(CexpTypedVal v) { v->tv.p=(void*)(unsigned long)v->tv.c; }
static void c2f(CexpTypedVal v) { v->tv.f=(float)v->tv.c; }
static void c2d(CexpTypedVal v) { v->tv.d=(double)v->tv.c; }

static void s2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.s; }
static void s2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.s; }
static void s2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.s; }
static void s2p(CexpTypedVal v) { v->tv.p=(void*)(unsigned long)v->tv.s; }
static void s2f(CexpTypedVal v) { v->tv.f=(float)v->tv.s; }
static void s2d(CexpTypedVal v) { v->tv.d=(double)v->tv.s; }

static void l2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.l; }
static void l2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.l; }
static void l2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.l; }
static void l2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.l; }
static void l2f(CexpTypedVal v) { v->tv.f=(float)v->tv.l; }
static void l2d(CexpTypedVal v) { v->tv.d=(double)v->tv.l; }

static void p2c(CexpTypedVal v) { v->tv.c=(unsigned char)(unsigned long)v->tv.p; }
static void p2s(CexpTypedVal v) { v->tv.s=(unsigned short)(unsigned long)v->tv.p; }
static void p2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.p; }
static void p2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.p; }
static void p2f(CexpTypedVal v) { v->tv.f=(float)(unsigned long)v->tv.p; }
static void p2d(CexpTypedVal v) { v->tv.d=(double)(unsigned long)v->tv.p; }

static void f2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.f; }
static void f2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.f; }
static void f2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.f; }
/* static void f2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.f; } */
static void f2f(CexpTypedVal v) { v->tv.f=(float)v->tv.f; }
static void f2d(CexpTypedVal v) { v->tv.d=(double)v->tv.f; }

static void d2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.d; }
static void d2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.d; }
static void d2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.d; }
/* static void d2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.d; } */
static void d2f(CexpTypedVal v) { v->tv.f=(float)v->tv.d; }
static void d2d(CexpTypedVal v) { v->tv.d=(double)v->tv.d; }

/* forbidden under even if CVT_FORCE */
#define f2p (Converter)0
#define d2p (Converter)0

static Converter ctab[6][6] = {
	{	p2p,	p2c,	p2s,	p2l,	p2f,	p2d	},
	{	c2p,	c2c,	c2s,	c2l,	c2f,	c2d	},
	{	s2p,	s2c,	s2s,	s2l,	s2f,	s2d	},
	{	l2p,	l2c,	l2s,	l2l,	l2f,	l2d	},
	{	f2p,	f2c,	f2s,	f2l,	f2f,	f2d	},
	{	d2p,	d2c,	d2s,	d2l,	d2f,	d2d	},
};

const char *
cexpTypeCast(CexpTypedVal v, CexpType t, int flags)
{
Converter	c;
int			from,to;

	from=CEXP_TYPE_PTRQ(v->type);
	to  =CEXP_TYPE_PTRQ(t);

	/* cast from one pointer into another ? */
	if (from && to) {
		if (!(flags&CNV_FORCE) && CEXP_BASE_TYPE_SIZE(t) > CEXP_BASE_TYPE_SIZE(v->type))
			return "cannot cast, target pointer element type too small";

			v->type=t;
			return 0;
	}

	if (from)	{
			/* use the 'from pointer' converter for any pointer */
			from=CEXP_TYPE_INDX(TVoidP);
			to  =CEXP_TYPE_INDX(t);
	} else if (to) {
			from=CEXP_TYPE_INDX(v->type);
			to  =CEXP_TYPE_INDX(TVoidP);
			/* use the 'to pointer' converter for any pointer */
	} else {
			from=CEXP_TYPE_INDX(v->type);
			to  =CEXP_TYPE_INDX(t);
	}

	/* we must convert the data */
	c=ctab[from][to];
	if (c) {
		if (!(flags&CNV_FORCE) && CEXP_TYPE_SIZE(t) < CEXP_TYPE_SIZE(v->type))
				return "cannot cast; would truncate source value";
		c(v);
		v->type=t;
	} else
		return "forbidden cast";
	return 0;
}

/* it is legal for from and to to point to the same object */
const char *
cexpTVPtrDeref(CexpTypedVal to, CexpTypedVal from)
{
		switch (from->type) {
				default:
				return "dereferencing invalid type";

				case TUCharP:	to->tv.c=*(unsigned char*)from->tv.p; break;
				case TUShortP:	to->tv.s=*(unsigned short*)from->tv.p; break;
				case TULongP:	to->tv.l=*(unsigned long*)from->tv.p; break;
				case TFloatP:	to->tv.f=*(float*)from->tv.p; break;
				case TDoubleP:	to->tv.d=*(double*)from->tv.p; break;
		}
		to->type = CEXP_TYPE_PTR2BASE(from->type);
		return 0;
}

const char *
cexpTVPtr(CexpTypedVal ptr, CexpTypedAddr a)
{
	if (CEXP_TYPE_PTRQ(a->type))
		return "refuse to take address of pointer";
	ptr->type=CEXP_TYPE_BASE2PTR(a->type);
	ptr->tv.p=(void*)a->ptv;
	return 0;
}

static const char *
compare(CexpTypedVal y, CexpTypedVal x1, CexpTypedVal x2, CexpBinOp op)
{
CexpTypedValRec xx1, xx2;
const char	 *errmsg;
int			 f=(CEXP_TYPE_FPQ(x1->type) || CEXP_TYPE_FPQ(x2->type));

		if ( f && (CEXP_TYPE_PTRQ(x1->type) || CEXP_TYPE_PTRQ(x2->type)))
				return "cannot compare pointers with floats";

		y->type=TULong;
		xx1=*x1; xx2=*x2;

		if ((errmsg=cexpTypeCast(&xx1, f ? TDouble : TULong, 0)) ||
			(errmsg=cexpTypeCast(&xx2, f ? TDouble : TULong, 0)))
				return errmsg;
		if (f) {
				switch (op) {
						default: return "unknown comparison operator";
						case OLt:	y->tv.l = xx1.tv.d <  xx2.tv.d; break;
						case OLe:	y->tv.l = xx1.tv.d <= xx2.tv.d; break;
						case OEq:	y->tv.l = xx1.tv.d == xx2.tv.d; break;
						case ONe:	y->tv.l = xx1.tv.d != xx2.tv.d; break;
						case OGe:	y->tv.l = xx1.tv.d >= xx2.tv.d; break;
						case OGt:	y->tv.l = xx1.tv.d >  xx2.tv.d; break;
				}
		} else {
				switch (op) {
						default: return "unknown comparison operator";
						case OLt:	y->tv.l = xx1.tv.l <  xx2.tv.l; break;
						case OLe:	y->tv.l = xx1.tv.l <= xx2.tv.l; break;
						case OEq:	y->tv.l = xx1.tv.l == xx2.tv.l; break;
						case ONe:	y->tv.l = xx1.tv.l != xx2.tv.l; break;
						case OGe:	y->tv.l = xx1.tv.l >= xx2.tv.l; break;
						case OGt:	y->tv.l = xx1.tv.l >  xx2.tv.l; break;
				}
		}
		return 0;
}

/* this (static!) routine has different semantics:
 * it assumes the caller has made copies of the source
 * operands and it is safe to modify them
 */
static const char *
binop(CexpTypedVal y, CexpTypedVal x1, CexpTypedVal x2, CexpBinOp op)
{
const char *errmsg;
		assert(x1->type == x2->type && !CEXP_TYPE_PTRQ(x1->type));

		if (CEXP_TYPE_SCALARQ(x1->type)) {
				CexpType res=x1->type;
				/* promote everything to ULONG */
				if ((errmsg=cexpTypeCast(x1,TULong,0)) ||
					(errmsg=cexpTypeCast(x2,TULong,0)))
					return errmsg;
				y->type=TULong;
				switch (op) {
					case OAdd:	y->tv.l=x1->tv.l + x2->tv.l; break;
					case OSub:	y->tv.l=x1->tv.l - x2->tv.l; break;
					case OMul:	y->tv.l=x1->tv.l * x2->tv.l; break;
					case ODiv:	y->tv.l=x1->tv.l / x2->tv.l; break;
					case OMod:	y->tv.l=x1->tv.l % x2->tv.l; break;
					case OShL:	y->tv.l=x1->tv.l << x2->tv.l; break;
					case OShR:	y->tv.l=x1->tv.l >> x2->tv.l; break;
					case OAnd:	y->tv.l=x1->tv.l & x2->tv.l; break;
					case OXor:	y->tv.l=x1->tv.l ^ x2->tv.l; break;
					case OOr:	y->tv.l=x1->tv.l | x2->tv.l; break;
					default: return "invalid operator on scalars";
				}
				/* cast result back to original type */ 
				return cexpTypeCast(y,res,CNV_FORCE);
		}

		y->type=x1->type;
		if (TFloat==y->type) {
				switch (op) {
					case OAdd:	y->tv.f=x1->tv.f + x2->tv.f; return 0;
					case OSub:	y->tv.f=x1->tv.f - x2->tv.f; return 0;
					case OMul:	y->tv.f=x1->tv.f * x2->tv.f; return 0;
					case ODiv:	y->tv.f=x1->tv.f / x2->tv.f; return 0;
					default: return "invalid operator on floats";
				}
		} else if (TDouble==y->type) {
				switch (op) {
					case OAdd:	y->tv.d=x1->tv.d + x2->tv.d; return 0;
					case OSub:	y->tv.d=x1->tv.d - x2->tv.d; return 0;
					case OMul:	y->tv.d=x1->tv.d * x2->tv.d; return 0;
					case ODiv:	y->tv.d=x1->tv.d / x2->tv.d; return 0;
					default: return "invalid operator on doubles";
				}
		}
		return 0;
}

const char *
cexpTVBinOp(CexpTypedVal y, CexpTypedVal x1, CexpTypedVal x2, CexpBinOp op)
{
CexpTypedVal	ptr=0;
CexpTypedVal	inc=0;
CexpTypedValRec	v,w;
const char		*errmsg;

		if (OAdd>op)
				return compare(y,x1,x2,op);

		if (CEXP_TYPE_PTRQ(x1->type)) {
				if (CEXP_TYPE_PTRQ(x2->type)) {
						unsigned long diff,s;
						/* add subtract two pointers */
						if (x1->type != x2->type)
								return "pointer type mismatch";
						if (op!=OSub)
								return "invalid operator on two pointers";

						y->type=TULong;
						diff=(char*)x1->tv.p - (char*)x2->tv.p;
						s=CEXP_BASE_TYPE_SIZE(x1->type);
						if (diff % s)
								return "subtracting misaligned pointers";
						diff/=s;
						y->tv.l=diff;
						return 0;
				}
				ptr=x1;
				inc=x2;
		}
		if (ptr || CEXP_TYPE_PTRQ(x2->type)) {
						/* pointer arithmetic */
						if (!ptr) {
							   if (OSub==op)
								return "cannot subtract a pointer";
							   ptr=x2; inc=x1;
						}
						v = *inc;
						
						if (OSub<op || cexpTypeCast(&v,TULong,0))
								return "invalid operator or pointer increment";

						if (OSub==op)
								y->tv.p=(void*)((char*)ptr->tv.p -
												v.tv.l*CEXP_BASE_TYPE_SIZE(ptr->type));
						else
								y->tv.p=(void*)((char*)ptr->tv.p +
												v.tv.l*CEXP_BASE_TYPE_SIZE(ptr->type));

						y->type=ptr->type;
						return 0;
		}
		/* 'normal' arithmetic */
		v=*x1; w=*x2;
		if ((errmsg=cexpTypePromote(&v,&w))) return errmsg;
		return binop(y,&v,&w,op);
}

const char *
cexpTVAssign(CexpTypedAddr y, CexpTypedVal x)
{
const char		*errmsg;
CexpTypedValRec	xx;
	xx=*x;
	/* cast the value to the proper type */
	if ((errmsg=cexpTypeCast(&xx,y->type,0)))
			return errmsg;
	/* perform the actual assignment */
	switch (y->type) {
			case TUChar:	y->ptv->c = xx.tv.c; break;
			case TUShort:	y->ptv->s = xx.tv.s; break;
			case TULong:	y->ptv->l = xx.tv.l; break;
			case TFloat:	y->ptv->f = xx.tv.f; break;
			case TDouble:	y->ptv->d = xx.tv.d; break;
			default:
				if (CEXP_TYPE_PTRQ(y->type)) {
					y->ptv->p = xx.tv.p;
				} else {
					return "invalid left hand type for assignment";
				}
			break;
	}
	return 0;
}

const char *
cexpTVUnOp(CexpTypedVal y, CexpTypedVal x, CexpUnOp op)
{
const char *errmsg;
		y->type=x->type;
		switch (op) {
				case ONeg:
					switch (x->type) {
							case TUChar:  y->tv.c=(unsigned char)-(char)x->tv.c; break;
							case TUShort: y->tv.s=(unsigned short)-(short)x->tv.s; break;
							case TULong:  y->tv.l=(unsigned long)-(long)x->tv.l; break;
							case TFloat:  y->tv.f=-x->tv.f; break;
							case TDouble: y->tv.c=-x->tv.d; break;
							default: return "invalid type for NEG operator";
					}
					return 0;

				case OCpl: /* bitwise complement (~) */
					if (!CEXP_TYPE_SCALARQ(x->type))
							return "invalid type for bitwise complement";
					*y=*x;
					if ((errmsg=cexpTypeCast(y,TULong,0)))
							return errmsg;
					y->tv.l=~y->tv.l;
					if ((errmsg=cexpTypeCast(y,x->type,CNV_FORCE)))
							return errmsg;
					return 0;
				default:
					break;
		}
		return "unknown unary operator";
}

/* boost both values to the size of the larger one */
const char *
cexpTypePromote(CexpTypedVal a, CexpTypedVal b)
{
CexpTypedVal small,big;
int			 f;

		if (CEXP_TYPE_PTRQ(a->type) || CEXP_TYPE_PTRQ(b->type))
				return "refuse to promote pointer type";

		if (CEXP_TYPE_FPQ(a->type) != (f=CEXP_TYPE_FPQ(b->type))) {
				/* exactly one of them is a floating point type */
				if (f) {
					big=b; small=a;
				} else {
					small=b; big=a;
				}
		} else {
				if (CEXP_BASE_TYPE_SIZE(a->type) > CEXP_BASE_TYPE_SIZE(b->type)) {
					small=b; big=a;
				} else {
					big=b; small=a;
				}
		}

		return cexpTypeCast(small,big->type,0);
}

unsigned long
cexpTVTrueQ(CexpTypedVal v)
{
		if (CEXP_TYPE_PTRQ(v->type))
				return v->tv.p ? 1 : 0;

		switch (v->type) {
				default: break;
				case TUChar:	return v->tv.c;
				case TUShort:	return v->tv.s;
				case TULong:	return v->tv.l;
				case TFloat:	return v->tv.f != (float)0.0;
				case TDouble:	return v->tv.d != (double)0.0;
		}
		assert(0=="unknown type???");
		return 0;
}

void
cexpTAPrintInfo(CexpTypedAddr a, FILE *f)
{
int i;

	if (CEXP_TYPE_PTRQ(a->type)) {
		i=fprintf(f,"%20p",a->ptv->p);
	} else {
		switch (a->type) {
			default:
				assert(0=="type mismatch");
			case TUChar:
				i=fprintf(f,"0x%02x ('%c'==%i)",a->ptv->c,a->ptv->c,a->ptv->c); break;
			case TUShort:
				i=fprintf(f,"0x%04x (==%i)",a->ptv->s,a->ptv->s); break;
			case TULong:
				i=fprintf(f,"0x%08lx (==%li)",a->ptv->l,a->ptv->l); break;
			case TFloat:
				i=fprintf(f,"%g",a->ptv->f); break;
			case TDouble:
				i=fprintf(f,"%g",a->ptv->d); break;
		}
	}
	for (;i<30;i++)
		fputc(' ',f);
	fprintf(f,"%s",cexpTypeInfoString(a->type));
}

const char *
cexpTA2TV(CexpTypedVal v, CexpTypedAddr a)
{
	switch ((v->type=a->type)) {
			case TVoid:		return "cannot get 'void' value";
			case TUChar:	v->tv.c=a->ptv->c; break;
			case TUShort:	v->tv.s=a->ptv->s; break;
			case TULong:	v->tv.l=a->ptv->l; break;
			case TFloat:	v->tv.f=a->ptv->f; break;
			case TDouble:	v->tv.d=a->ptv->d; break;
			default:
				if (CEXP_TYPE_PTRQ(a->type)) {
					v->tv.p=a->ptv->p;
				} else {
					return "unknown type in cexpTA2TV";
				}
			break;
	}
	return 0;
}

#define UL unsigned long
#define DB double
#define AA CexpTypedVal

#if defined(__PPC__) && defined(_CALL_SYSV)

/* an PPC / SVR4 ABI specific implementation of the function call
 * interface.
 *
 * The PPC-SVR4-ABI would always put the first 8 integer arguments
 * into gpr3..10 and the first 8 double arguments into f1..f8.
 * This makes calling any function (with integer/double args only)
 * a piece of cake...
 *
 * Bit 6 of the CR register must be set/cleared to indicate
 * whether floating point arguments were passed in FP registers
 * in case the called routine takes variable arguments.
 * 
 * Note that _ANY_ combination of integer/double (of up to
 * 8 of each) arguments will end up in the same registers.
 *
 * I.e 
 *
 * fprintf(stdout,"hello, %i and %lf\n",i,e);
 * 
 * and 
 *
 * fprintf(e,stdout,"hello, %i and %lf\n",i);
 * fprintf(stdout,e,"hello, %i and %lf\n",i);
 * fprintf(stdout,"hello, %i and %lf\n",e,i);
 *
 * will all work :-)
 *
 * If we have more than 8 arguments, we must not mix the ones
 * that exceed 8. Hence we allow for up to 10 integer and 8 doubles.
 *
 */

/* NOTE: the minimum of these MUST NOT exceed 8
 *       Also note that the FN_PPCSVRABI_xx type declarations
 *       as well as the function call itself must be adjusted
 *       when changing any of these numbers.
 */
#define MAXINTARGS 10
#define MAXDBLARGS 8

typedef UL (*FN_PPCSVRABI_UL)(UL,UL,UL,UL,UL,UL,UL,UL,UL,UL,DB,DB,DB,DB,DB,DB,DB,DB);
typedef DB (*FN_PPCSVRABI_DB)(UL,UL,UL,UL,UL,UL,UL,UL,UL,UL,DB,DB,DB,DB,DB,DB,DB,DB);


const char *
cexpTVFnCall(CexpTypedVal rval, CexpTypedVal fn, ...)
{
va_list 		ap;
CexpTypedVal 	v;
int				nargs,fpargs,i;
const char		*err=0;
UL				iargs[MAXINTARGS];
DB				dargs[MAXDBLARGS];

		/* sanity check */
		if (!CEXP_TYPE_FUNQ(fn->type))
				return "need a function pointer";

		nargs=0; fpargs=0;
		va_start(ap,fn);

		while ((v=va_arg(ap,CexpTypedVal))) {
			if (CEXP_TYPE_FPQ(v->type)) {
				if (fpargs>=MAXDBLARGS) {
					err="Too many double arguments";
					goto cleanup;
				}
				err=cexpTypeCast(v,TDouble,0);
				dargs[fpargs++]=v->tv.d;
			} else {
				if (nargs>=MAXINTARGS) {
					err="Too many integer arguments";
					goto cleanup;
				}
				err=cexpTypeCast(v,TULong,0);
				iargs[nargs++]=v->tv.l;
			}
		}
		va_end(ap);
		for (i=nargs; i<MAXINTARGS; i++)
				iargs[i]=0;
		for (i=fpargs; i<MAXDBLARGS; i++)
				dargs[i]=0;

		/* set CR[6] */
		if (fpargs)
			__asm__ __volatile__("crset 4*cr1+eq");
		else
			__asm__ __volatile__("crclr 4*cr1+eq");

		/* call it */
		rval->type=CEXP_TYPE_PTR2BASE(fn->type);
		if (TDFuncP==fn->type)
			rval->tv.d=((FN_PPCSVRABI_DB)fn->tv.p)(
							iargs[0],iargs[1],iargs[2],iargs[3],iargs[4],iargs[5],iargs[6],iargs[7],iargs[8],iargs[9],
							dargs[0],dargs[1],dargs[2],dargs[3],dargs[4],dargs[5],dargs[6],dargs[7]);
		else
			rval->tv.l=((FN_PPCSVRABI_UL)fn->tv.p)(
							iargs[0],iargs[1],iargs[2],iargs[3],iargs[4],iargs[5],iargs[6],iargs[7],iargs[8],iargs[9],
							dargs[0],dargs[1],dargs[2],dargs[3],dargs[4],dargs[5],dargs[6],dargs[7]);

		return 0; /* va_end already called */
	
cleanup:
		va_end(ap);
		return err;
}

#else  /* ABI dependent implementation of cexpTVFnCall */

/* This is the GENERIC / PORTABLE implementation of the function
 * call interface
 */

/* maximal number of mixed arguments
 * NOTE: if this is changed, the jumptable
 *       ('jumptab.c') included below MUST
 *       BE UPDATED accordingly. Also, the
 *       actual call through the jumptable
 *       in cexpTVFnCall must be modified.
 */
/* NOTE: this is now defined in gentab.c
 * only - it is then copied to jumptab.c...
 * #define MAXBITS	5
 */

/* maximal number of unsigned long only arguments
 * NOTE: if this is changed, the typedefs for L10
 *       and D10 as well as the actual function call
 *       in cexpTVFnCall must be changed accordingly.
 */
#define MAXARGS 10

typedef UL (*L10)(UL,UL,UL,UL,UL,UL,UL,UL,UL,UL);
typedef DB (*D10)(UL,UL,UL,UL,UL,UL,UL,UL,UL,UL);

/* include an automatically (gentab utility) generated
 * file containing definitions of wrappers for all 32
 * possible function call combinations as well as a
 * jumptable. For up to two arguments this would
 * look as follows:
 *
 * static long ll(CexpTypedVal fn, CexpTypedVal a1, CexpTypedVal a2)
 * { return ((unsigned long(*)()fn->tv.p)(a1->tv.l, a1->tv.l); }
 *
 * static long ld(CexpTypedVal fn, CexpTypedVal a1, CexpTypedVal a2)
 * { return ((unsigned long(*)()fn->tv.p)(a1->tv.l, a1->tv.d); }
 *
 * static long dl(CexpTypedVal fn, CexpTypedVal a1, CexpTypedVal a2)
 * { return ((unsigned long(*)()fn->tv.p)(a1->tv.d, a1->tv.l); }
 *
 * static long dd(CexpTypedVal fn, CexpTypedVal a1, CexpTypedVal a2)
 * { return ((unsigned long(*)()fn->tv.p)(a1->tv.d, a1->tv.d); }
 *
 * static long (*jumptab[2])()={ ll, ld, dl, dd};
 *
 * based on the number/position of double arguments, cexpTVFnCall then
 * computes the right index into the jumptable and calls the respective
 * wrapper. The compiler then takes care of setting up the correct
 * calling frame. If we have more information about the particular
 * ABI, this can be significantly improved (see PPC/SVR4 implementation
 * above).
 */

/* ABI NOTES:
 *    PowerPC: Casting the function pointer to a prototyped function
 *             fails if jumping to a function which takes variable
 *             arguments and if there are floating point args passed
 *             in registers :-(.
 *             The compiler produces correct code if the target function
 *             pointer is _not_ prototyped and if compiling with
 *             -mno-prototype (which seems to be the default, at least
 *             on linuxppc).
 */
#include "jumptab.c"

#if MAXBITS > MAXARGS
#error "MAXBITS (as defined in gentab.c) must be < MAXARGS"
#endif

const char *
cexpTVFnCall(CexpTypedVal rval, CexpTypedVal fn, ...)
{
va_list 		ap;
CexpTypedVal 	args[MAXARGS],v;
int				nargs,fpargs,i;
CexpTypedValRec zero;
const char		*err=0;

		/* sanity check */
		if (!CEXP_TYPE_FUNQ(fn->type))
				return "need a function pointer";

		zero.type=TULong;
		zero.tv.l=0;

		nargs=0; fpargs=0;
		va_start(ap,fn);

		while ((v=va_arg(ap,CexpTypedVal)) && nargs<MAXARGS) {
			fpargs<<=1;
			args[nargs++]=v;
			if (CEXP_TYPE_FPQ(v->type)) {
				fpargs|=1;
				err=cexpTypeCast(v,TDouble,0);
			} else {
				err=cexpTypeCast(v,TULong,0);
			}
			if (err)
				goto cleanup;
		}
		if (v || (fpargs && nargs>MAXBITS)) {
			err = "too many function arguments";
			goto cleanup;
		}
		/* pad with zeroes */
		for (i=nargs; i< (fpargs? MAXBITS : MAXARGS); i++, fpargs<<=1)
				args[i]=&zero;

		/* call it */
		rval->type=CEXP_TYPE_PTR2BASE(fn->type);
		if (fpargs) {
				if (TDFuncP==fn->type)
					rval->tv.d=((double(*)())(jumptab[fpargs]))(fn JUMPTAB_ARGLIST(args));
				else
					rval->tv.l=jumptab[fpargs](fn,args[0],args[1],args[2],args[3],args[4]);
		} else {
				if (TDFuncP==fn->type)
					rval->tv.d=((D10)(fn->tv.p))(
										args[0]->tv.l,
										args[1]->tv.l,
										args[2]->tv.l,
										args[3]->tv.l,
										args[4]->tv.l,
										args[5]->tv.l,
										args[6]->tv.l,
										args[7]->tv.l,
										args[8]->tv.l,
										args[9]->tv.l);

				else
					rval->tv.l=((L10)(fn->tv.p))(
										args[0]->tv.l,
										args[1]->tv.l,
										args[2]->tv.l,
										args[3]->tv.l,
										args[4]->tv.l,
										args[5]->tv.l,
										args[6]->tv.l,
										args[7]->tv.l,
										args[8]->tv.l,
										args[9]->tv.l);
		}

cleanup:
		va_end(ap);
		return err;
}
#endif /* ABI dependent implementation of cexpTVFnCall */
