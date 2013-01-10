/* $Id$ */

/* This file contains the implementation of support for elementary
 * C types (unsigned long, double etc.).
 */

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

#include <assert.h>
#include <stdlib.h>

#include "ctyps.h"
#include "cexpmod.h"

static char *fundesc[]={
		"long (*)()  ",
		"double (*)()",
};

static char *desc[]={
	[CEXP_TYPE_MASK_SIZE(TVoid)]    = "void        ",		/* function pointer */
	[CEXP_TYPE_MASK_SIZE(TVoidP)]   = "void*       ",		/* void*            */
	[CEXP_TYPE_MASK_SIZE(TUChar)]   = "uchar       ",		/* unsigned char    */
	[CEXP_TYPE_MASK_SIZE(TUCharP)]  = "uchar*      ",		/* unsigned char*   */
	[CEXP_TYPE_MASK_SIZE(TUShort)]  = "ushort      ",		/* unsigned short   */
	[CEXP_TYPE_MASK_SIZE(TUShortP)] = "ushort*     ",		/* unsigned short*  */
	[CEXP_TYPE_MASK_SIZE(TUInt)]    = "uint        ",		/* unsigned int     */
	[CEXP_TYPE_MASK_SIZE(TUIntP)]   = "uint*       ",		/* unsigned int*    */
	[CEXP_TYPE_MASK_SIZE(TULong)]   = "ulong       ",		/* unsigned long    */
	[CEXP_TYPE_MASK_SIZE(TULongP)]  = "ulong*      ",		/* unsigned long*   */
	[CEXP_TYPE_MASK_SIZE(TFloat)]   = "float       ",		/* float            */
	[CEXP_TYPE_MASK_SIZE(TFloatP)]  = "float*      ",		/* float*           */
	[CEXP_TYPE_MASK_SIZE(TDouble)]  = "double      ",		/* double           */
	[CEXP_TYPE_MASK_SIZE(TDoubleP)] = "double*     ",		/* double*          */
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
	return desc[t] ? desc[t] : "<UNKNOWN>";
}

/* build a converter matrix */
typedef void (*Converter)(CexpTypedVal);

/* all possible type conversions, even dangerous ones */
static void c2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.c; }
static void c2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.c; }
static void c2i(CexpTypedVal v) { v->tv.i=(unsigned int)v->tv.c; }
static void c2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.c; }
static void c2p(CexpTypedVal v) { v->tv.p=(void*)(unsigned long)v->tv.c; }
static void c2f(CexpTypedVal v) { v->tv.f=(float)v->tv.c; }
static void c2d(CexpTypedVal v) { v->tv.d=(double)v->tv.c; }

static void s2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.s; }
static void s2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.s; }
static void s2i(CexpTypedVal v) { v->tv.i=(unsigned int)v->tv.s; }
static void s2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.s; }
static void s2p(CexpTypedVal v) { v->tv.p=(void*)(unsigned long)v->tv.s; }
static void s2f(CexpTypedVal v) { v->tv.f=(float)v->tv.s; }
static void s2d(CexpTypedVal v) { v->tv.d=(double)v->tv.s; }

static void i2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.i; }
static void i2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.i; }
static void i2i(CexpTypedVal v) { v->tv.i=(unsigned int)v->tv.i; }
static void i2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.i; }
static void i2p(CexpTypedVal v) { v->tv.p=(void*)(unsigned long)v->tv.i; }
static void i2f(CexpTypedVal v) { v->tv.f=(float)v->tv.i; }
static void i2d(CexpTypedVal v) { v->tv.d=(double)v->tv.i; }

static void l2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.l; }
static void l2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.l; }
static void l2i(CexpTypedVal v) { v->tv.i=(unsigned int)v->tv.l; }
static void l2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.l; }
static void l2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.l; }
static void l2f(CexpTypedVal v) { v->tv.f=(float)v->tv.l; }
static void l2d(CexpTypedVal v) { v->tv.d=(double)v->tv.l; }

static void p2c(CexpTypedVal v) { v->tv.c=(unsigned char)(unsigned long)v->tv.p; }
static void p2s(CexpTypedVal v) { v->tv.s=(unsigned short)(unsigned long)v->tv.p; }
static void p2i(CexpTypedVal v) { v->tv.i=(unsigned int)(unsigned long)v->tv.p; }
static void p2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.p; }
static void p2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.p; }
static void p2f(CexpTypedVal v) { v->tv.f=(float)(unsigned long)v->tv.p; }
static void p2d(CexpTypedVal v) { v->tv.d=(double)(unsigned long)v->tv.p; }

static void f2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.f; }
static void f2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.f; }
static void f2i(CexpTypedVal v) { v->tv.i=(unsigned int)v->tv.f; }
static void f2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.f; }
/* static void f2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.f; } */
static void f2f(CexpTypedVal v) { v->tv.f=(float)v->tv.f; }
static void f2d(CexpTypedVal v) { v->tv.d=(double)v->tv.f; }

static void d2c(CexpTypedVal v) { v->tv.c=(unsigned char)v->tv.d; }
static void d2s(CexpTypedVal v) { v->tv.s=(unsigned short)v->tv.d; }
static void d2i(CexpTypedVal v) { v->tv.i=(unsigned int)v->tv.d; }
static void d2l(CexpTypedVal v) { v->tv.l=(unsigned long)v->tv.d; }
/* static void d2p(CexpTypedVal v) { v->tv.p=(void*)v->tv.d; } */
static void d2f(CexpTypedVal v) { v->tv.f=(float)v->tv.d; }
static void d2d(CexpTypedVal v) { v->tv.d=(double)v->tv.d; }

/* forbidden under even if CVT_FORCE */
#define f2p (Converter)0
#define d2p (Converter)0

static Converter ctab[7][7] = {
	[CEXP_TYPE_INDX(TVoid)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = p2p,
		[CEXP_TYPE_INDX(TUChar)]  = p2c,
		[CEXP_TYPE_INDX(TUShort)] = p2s,
		[CEXP_TYPE_INDX(TUInt)]   = p2i,
		[CEXP_TYPE_INDX(TULong)]  = p2l,
		[CEXP_TYPE_INDX(TFloat)]  = p2f,
		[CEXP_TYPE_INDX(TDouble)] = p2d
	},
	[CEXP_TYPE_INDX(TUChar)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = c2p,
		[CEXP_TYPE_INDX(TUChar)]  = c2c,
		[CEXP_TYPE_INDX(TUShort)] = c2s,
		[CEXP_TYPE_INDX(TUInt)]   = c2i,
		[CEXP_TYPE_INDX(TULong)]  = c2l,
		[CEXP_TYPE_INDX(TFloat)]  = c2f,
		[CEXP_TYPE_INDX(TDouble)] = c2d
	},
	[CEXP_TYPE_INDX(TUShort)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = s2p,
		[CEXP_TYPE_INDX(TUChar)]  = s2c,
		[CEXP_TYPE_INDX(TUShort)] = s2s,
		[CEXP_TYPE_INDX(TUInt)]   = s2i,
		[CEXP_TYPE_INDX(TULong)]  = s2l,
		[CEXP_TYPE_INDX(TFloat)]  = s2f,
		[CEXP_TYPE_INDX(TDouble)] = s2d
	},
	[CEXP_TYPE_INDX(TUInt)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = i2p,
		[CEXP_TYPE_INDX(TUChar)]  = i2c,
		[CEXP_TYPE_INDX(TUShort)] = i2s,
		[CEXP_TYPE_INDX(TUInt)]   = i2i,
		[CEXP_TYPE_INDX(TULong)]  = i2l,
		[CEXP_TYPE_INDX(TFloat)]  = i2f,
		[CEXP_TYPE_INDX(TDouble)] = i2d
	},
	[CEXP_TYPE_INDX(TULong)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = l2p,
		[CEXP_TYPE_INDX(TUChar)]  = l2c,
		[CEXP_TYPE_INDX(TUShort)] = l2s,
		[CEXP_TYPE_INDX(TUInt)]   = l2i,
		[CEXP_TYPE_INDX(TULong)]  = l2l,
		[CEXP_TYPE_INDX(TFloat)]  = l2f,
		[CEXP_TYPE_INDX(TDouble)] = l2d
	},
	[CEXP_TYPE_INDX(TFloat)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = f2p,
		[CEXP_TYPE_INDX(TUChar)]  = f2c,
		[CEXP_TYPE_INDX(TUShort)] = f2s,
		[CEXP_TYPE_INDX(TUInt)]   = f2i,
		[CEXP_TYPE_INDX(TULong)]  = f2l,
		[CEXP_TYPE_INDX(TFloat)]  = f2f,
		[CEXP_TYPE_INDX(TDouble)] = f2d
	},
	[CEXP_TYPE_INDX(TDouble)] =
	{	[CEXP_TYPE_INDX(TVoid)]   = d2p,
		[CEXP_TYPE_INDX(TUChar)]  = d2c,
		[CEXP_TYPE_INDX(TUShort)] = d2s,
		[CEXP_TYPE_INDX(TUInt)]   = d2i,
		[CEXP_TYPE_INDX(TULong)]  = d2l,
		[CEXP_TYPE_INDX(TFloat)]  = d2f,
		[CEXP_TYPE_INDX(TDouble)] = d2d
	},
};

/* check whether v would fit into a variable of type t */

static int
fits(CexpTypedVal v, CexpType t)
{
unsigned long n;

	if (CEXP_TYPE_FPQ(t) && ! CEXP_TYPE_FPQ(v->type)) {
		/* IEEE FP number has 24bit mantissa */
		if (TFloat == t) {
			/* Only int and long could be too big to fit */
			switch ( v->type ) {
				case TULong: n = v->tv.l; break;
				case TUInt:  n = v->tv.i; break;
				default:     n = 0;       break;
			}
			return n < (1<<24);
		}
		/* anything fits into a double */
	} else if (!CEXP_TYPE_FPQ(t) && CEXP_TYPE_FPQ(v->type)) {
		return 0;
	}
	if (CEXP_TYPE_SIZE(t) < CEXP_TYPE_SIZE(v->type)) {
		if ( CEXP_TYPE_PTRQ(v->type) ) { 
			n = (unsigned long)v->tv.p;
		} else {
			switch(v->type) {
				case TUChar: 	n=v->tv.c; break;
				case TUShort:	n=v->tv.s; break;
				case TUInt:		n=v->tv.i; break;
				case TULong:	n=v->tv.l; break;

				case TFloat:
				case TDouble: 	return 0;

				default:
								assert(!"Invalid Type - you found a BUG");
								n=0;
				break;
			}
		}
		return n < (1<<(8*CEXP_TYPE_SIZE(t)));
	}
	return 1;
}

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
		if ( ! (flags&CNV_FORCE) && ! fits(v,t))
				return "cannot perform implicit cast; would truncate source value - use explicit cast to override";
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
		if (!from->tv.p)
				return "reject dereferencing NULL pointer";

		switch (from->type) {
				default:
				return "dereferencing invalid type";

				case TUCharP:	to->tv.c=*(unsigned char*)from->tv.p; break;
				case TUShortP:	to->tv.s=*(unsigned short*)from->tv.p; break;
				case TUIntP:	to->tv.i=*(unsigned int*)from->tv.p; break;
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
			case TUInt:		y->ptv->i = xx.tv.i; break;
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
							case TUChar:  y->tv.c=(unsigned char)  - (char) x->tv.c; break;
							case TUShort: y->tv.s=(unsigned short) - (short)x->tv.s; break;
							case TUInt:   y->tv.i=(unsigned int)   - (int)  x->tv.i; break;
							case TULong:  y->tv.l=(unsigned long)  - (long) x->tv.l; break;
							case TFloat:  y->tv.f=-x->tv.f; break;
							case TDouble: y->tv.d=-x->tv.d; break;
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
				case TUInt:		return v->tv.i;
				case TULong:	return v->tv.l;
				case TFloat:	return v->tv.f != (float)0.0;
				case TDouble:	return v->tv.d != (double)0.0;
		}
		assert(0=="unknown type???");
		return 0;
}

int
cexpTAPrintInfo(CexpTypedAddr a, FILE *f)
{
int i=0;

	if (CEXP_TYPE_PTRQ(a->type)) {
		i+=fprintf(f,"%p",a->ptv->p);
	} else {
		if (a->type != TVoid && 0==a->ptv) {
			i+=fprintf(f,"NULL");
		} else {
		switch (a->type) {
			default:
				assert(0=="type mismatch");
			case TUChar:
				i+=fprintf(f,"0x%02x ('%c'==%i)",a->ptv->c,a->ptv->c,a->ptv->c); break;
			case TUShort:
				i+=fprintf(f,"0x%04x (==%i)",a->ptv->s,a->ptv->s); break;
			case TUInt:
				i+=fprintf(f,"0x%08x (==%i)",a->ptv->i,a->ptv->i); break;
			case TULong:
				i+=fprintf(f,"0x%0*lx (==%li)",(int)(2*sizeof(a->ptv->l)),a->ptv->l,a->ptv->l); break;
			case TFloat:
				i+=fprintf(f,"%g",a->ptv->f); break;
			case TDouble:
				i+=fprintf(f,"%g",a->ptv->d); break;
			case TVoid:
				i+=fprintf(f,"VOID"); break;
		}
		}
	}

	for (;i<30;i++)
		fputc(' ',f);
	i+=fprintf(f,"%s",cexpTypeInfoString(a->type));
	return i;
}

const char *
cexpTA2TV(CexpTypedVal v, CexpTypedAddr a)
{
	if (!a->ptv)
			return "reject dereferencing NULL pointer";

	switch ((v->type=a->type)) {
			case TVoid:		return "cannot get 'void' value";
			case TUChar:	v->tv.c=a->ptv->c; break;
			case TUShort:	v->tv.s=a->ptv->s; break;
			case TUInt:		v->tv.i=a->ptv->i; break;
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

CexpType
cexpTypeGuessFromSize(int s)
{
CexpType t = TVoid;

	/* If any floating-point size equals an integer size then the latter has preference */

	if (CEXP_BASE_TYPE_SIZE(TUCharP) == s) {
		t=TUChar;
	} else if (CEXP_BASE_TYPE_SIZE(TUShortP) == s) {
		t=TUShort;
	/* Check for 'long' first so that it will have preference in
	 * case 'int' and 'long' are of the same size.
	 */
	} else if (CEXP_BASE_TYPE_SIZE(TULongP)  == s) {
		t=TULong;
	} else if (CEXP_BASE_TYPE_SIZE(TUIntP)   == s) {
		t=TUInt;
	} else if (CEXP_BASE_TYPE_SIZE(TDoubleP) == s) {
		t=TDouble;
	} else if (CEXP_BASE_TYPE_SIZE(TFloatP)  == s) {
		t=TFloat;
	} else {
		/* if it's bigger leave it (void*) */
	}
	return t;
}

#define UL unsigned long
#define DB double
#define AA CexpTypedVal

typedef UL (*UFUNC)();
typedef DB (*DFUNC)();

#if defined(__PPC__) && defined(_CALL_SYSV) && !defined(_SOFT_FLOAT)

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
 * The compiler does this automatically for us if we declare
 * the function pointers with an empty argument list:
 *   typedef long (*LFUNC)();
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
 *       Also note that the function call below must be
 *       adjusted when changing any of these numbers.
 */
#define MAXINTARGS 10
#define MAXDBLARGS 8

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

		if (!fn->tv.p)
				return "reject dereferencing NULL function pointer";

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

		/* call it */
		rval->type=CEXP_TYPE_PTR2BASE(fn->type);
		if (TDFuncP==fn->type)
			rval->tv.d=((DFUNC)fn->tv.p)(
							iargs[0],iargs[1],iargs[2],iargs[3],iargs[4],iargs[5],iargs[6],iargs[7],iargs[8],iargs[9],
							dargs[0],dargs[1],dargs[2],dargs[3],dargs[4],dargs[5],dargs[6],dargs[7]);
		else
			rval->tv.l=((UFUNC)fn->tv.p)(
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
 * NOTE: if this is changed, the function call
 *       in cexpTVFnCall must be changed accordingly.
 */
#define MAXARGS 10

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

		if (!fn->tv.p)
				return "reject dereferencing NULL function pointer";

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
					rval->tv.d=((DFUNC)(jumptab[fpargs]))(fn JUMPTAB_ARGLIST(args));
				else
					rval->tv.l=jumptab[fpargs](fn,args[0],args[1],args[2],args[3],args[4]);
		} else {
				if (TDFuncP==fn->type)
					rval->tv.d=((DFUNC)(fn->tv.p))(
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
					rval->tv.l=((UFUNC)(fn->tv.p))(
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
