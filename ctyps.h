#ifndef CEXP_TYPES_H
#define CEXP_TYPES_H

/* $Id$ */

/* Interface to the Cexp C-type 'engine' */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *     Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 *     Stanford Linear Accelerator Center, Stanford University.
 *
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 * the Stanford Linear Accelerator Center, Stanford University,
 * under Contract DE-AC03-76SFO0515 with the Department of Energy.
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

#include <stdarg.h>
#include <stdio.h>	/* only needed for the PrintInfo routine */
#include "cexp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* for now, we only support ulong result type */
typedef unsigned long (*CexpFuncPtr)();

/* types ordered according to their space
 * requirements. Note that the LSB is the
 * pointer flag.
 */

/* this choice leaves the lower bits in a natural order */
#define CEXP_PTR_BIT		(1<<0)

#define CEXP_FUN_BIT		(1<<6)
#define CEXP_ORDER_MASK		(0x1f)
typedef enum {
    TVoid		=0,
    TVoidP      =0  | (sizeof(unsigned char)<<8) | CEXP_PTR_BIT, /* treat void* like char* */
    TUChar      =2  | (sizeof(unsigned char)<<8),
    TUCharP     =2  | (sizeof(unsigned char)<<8) | CEXP_PTR_BIT,
    TUShort     =4  | (sizeof(unsigned short)<<8),
    TUShortP    =4  | (sizeof(unsigned short)<<8) | CEXP_PTR_BIT,
    TUInt       =6  | (sizeof(unsigned int)<<8),
    TUIntP      =6  | (sizeof(unsigned int)<<8) | CEXP_PTR_BIT,
    TULong      =8  | (sizeof(unsigned long)<<8),
    TULongP     =8  | (sizeof(unsigned long)<<8) | CEXP_PTR_BIT,
    TFloat      =10 | (sizeof(float)<<8),
    TFloatP     =10 | (sizeof(float)<<8) | CEXP_PTR_BIT,
    TDouble     =12 | (sizeof(double)<<8),
    TDoubleP    =12 | (sizeof(double)<<8) | CEXP_PTR_BIT,
/* INTFIX */
    TFuncP      =TULong  | CEXP_FUN_BIT | CEXP_PTR_BIT,
    TDFuncP     =TDouble | CEXP_FUN_BIT | CEXP_PTR_BIT
} CexpType;

/* Utility macros operating on CexpType */
#define CEXP_TYPE_PTRQ(type_enum) ((type_enum)&CEXP_PTR_BIT)
#define CEXP_TYPE_MASK_SIZE(type_enum) ((type_enum)&CEXP_ORDER_MASK)
#define CEXP_TYPE_INDX(type_enum) (CEXP_TYPE_MASK_SIZE((type_enum))>>1)

/* object, not a function pointer: */
#define CEXP_TYPE_FUNQ(type_enum) ((type_enum) & CEXP_FUN_BIT)

#define CEXP_TYPE_SCALARQ(type_enum) (\
				!CEXP_TYPE_PTRQ(type_enum) && \
				CEXP_TYPE_INDX(TUChar) <= CEXP_TYPE_INDX(type_enum) && \
				CEXP_TYPE_INDX(TULong) >= CEXP_TYPE_INDX(type_enum))

/* is it double or float ? */
#define CEXP_TYPE_FPQ(type_enum) ( \
				! CEXP_TYPE_PTRQ(type_enum) && \
				CEXP_TYPE_INDX(TFloat)<=CEXP_TYPE_INDX(type_enum))

#define CEXP_BASE_TYPE_SIZE(type_enum) (((type_enum)>>8)&0xff)
#define CEXP_TYPE_SIZE(type_enum) (\
				CEXP_TYPE_PTRQ(type_enum) ? \
			   		sizeof(void *) : \
				   	CEXP_BASE_TYPE_SIZE(type_enum))

#define CEXP_TYPE_PTR2BASE(type_enum) \
				((type_enum) & ~(CEXP_PTR_BIT|CEXP_FUN_BIT))
#define CEXP_TYPE_BASE2PTR(type_enum) \
				((type_enum) | CEXP_PTR_BIT)

typedef union  CexpValU_ {
		void           *p;
		unsigned char  c;
		unsigned short s;
		unsigned int   i;
		unsigned long  l;
		float          f;
		double         d;
} CexpValU, *CexpVal;

typedef struct CexpTypedValRec_{
	CexpValU	tv;
	CexpType	type;
} CexpTypedValRec, *CexpTypedVal;

/* If the layout of this changes, the module magic ID must be changed
 * see cexpmodP.h for details.
 */
typedef struct CexpTypedAddrRec_ {
	CexpVal		ptv;
	CexpType	type;
} CexpTypedAddrRec, *CexpTypedAddr;

/* NOTE: Order is important */
typedef enum {
		ONoop	=-1,
		OLt		=5,
		OLe		=6,
		OEq		=7,
		ONe		=8,
		OGe		=9,
		OGt		=10,
		OAdd	=11,		/* comparisons above here */
		OSub	=12,		/* pointers cannot do below this */
		OMul	=13,
		ODiv	=14,		/* floats cannot do below this */
		OMod	=15,
		OShL	=16,		/* bitwise left shift */
		OShR	=17,		/* bitwise right shift */
		OAnd	=18,
		OXor	=19,
		OOr	=20
} CexpBinOp;

/* unary operators */
typedef enum {
		ONeg	=0,		/* negate unsigned numbers ??? */
		OCpl	=1		/* bitwise inversion */
} CexpUnOp;


/* obtain a pointer to a static string describing the
 * type
 */
const char *
cexpTypeInfoString(CexpType t);

/* guess type based on size info (bytes) */
CexpType cexpTypeGuessFromSize(int size);

/* operations on typed values */

/* NOTE: all of these routines return 0 on success and
 *       a pointer to a static error message in case of
 *       failure.
 */

/* flags to give hints to the converter:
 */
#define CNV_FORCE	1
/* cast a typed value to a new type, set CNV_FORCE if
 * you want to allow for losing information
 * (such as casting a TUInt to a TUChar).
 */
const char *
cexpTypeCast(CexpTypedVal v, CexpType t, int flags);

/* it is legal for from and to to point to the same object */
const char *
cexpTVPtrDeref(CexpTypedVal to, CexpTypedVal from);

/* return a pointer to v (v must not be a pointer already
 * the return value gets an address to v's data area and
 * the proper pointer type.
 */
const char *
cexpTVPtr(CexpTypedVal ptr, CexpTypedAddr a);

const char *
cexpTVBinOp(CexpTypedVal y, CexpTypedVal x1, CexpTypedVal x2, CexpBinOp op);

/* assign x to *y, i.e. y must be a pointer */
const char *
cexpTVAssign(CexpTypedAddr y, CexpTypedVal x);

/* unary operators */
const char *
cexpTVUnOp(CexpTypedVal y, CexpTypedVal x, CexpUnOp op);

/* test for nonzero value */
unsigned long
cexpTVTrueQ(CexpTypedVal v);

/* boost both values to the size of the larger one */
const char *
cexpTypePromote(CexpTypedVal a, CexpTypedVal b);

/* call a function with a variable list of arguments
 * The arguments are all CexpTypedVals, terminated by
 * a NULL pointer
 * If the 'ctx' pointer is non-null, it will be prepended
 * to the arguments who are passed to 'fn'.
 *
 * NOTE: this routine _modifies_ the argument list, i.e.
 *       it casts everything to ulong or double.
 *       The caller is responsible for making copies if
 *       this is not permissible.
 */
const char *
cexpTVFnCall(CexpTypedVal rval, CexpTypedVal fn, ...);

/* this routine prints info about the typed address 'a'
 * to a file (without newline).
 * RETURNS: number or chars written
 *
 * NOTE: it is legal to pass a NULL argument (for 'a')
 * which will cause the routine to print spaces for
 * pretty printing.
 */
int
cexpTAPrintInfo(CexpTypedAddr a, FILE *f);

/* convert a typed address to a typed value */
const char *
cexpTA2TV(CexpTypedVal v, CexpTypedAddr a);

#ifdef __cplusplus
}
#endif

#endif
