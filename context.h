/* $Id$ */
#ifndef CEXP_CONTEXT_H
#define CEXP_CONTEXT_H
/* Cexp Context Interface. Declares data belonging to a running instance
 * of Cexp which must be globally accessible. (E.g. for 'longjumping' out
 * of nested routines etc.)
 *
 * Currently, this only works on RTEMS in a multithreaded way. RTEMS
 * features 'task_variables', i.e. memory locations who appear to belong
 * to a task's context. The contents are magically exchanged at context
 * switch time.
 *
 * Note that there are actually 'context stacks' since cexp may recursively
 * invoke itself. Every instance of cexp() owns a CexpContextRec these
 * records are linked together 'per thread' using the 'next' field.
 * In an environment which implements task variables, 'cexpCurrentContext'
 * always points to the context of the innermost cexp instance of the
 * current task's stack.
 *
 */
#include "cexp.h"

#ifdef USE_EPICS_OSI
/* BFD redefines INLINE if we don't include epicsThread.h first; sigh... */
#include <epicsThread.h>
#endif

#ifdef USE_TECLA
#include <libtecla.h>
#endif
#ifdef HAVE_BFD_DISASSEMBLER
#define boolean bbbboooolean /* se comment in bfdstuff.c why we do that */
#include "dis-asm.h"
#endif

#include <setjmp.h>

typedef struct CexpContextRec_ {
	CexpContext			next;
	jmp_buf				jbuf;		/* for setjmp/longjmp */
#ifdef USE_TECLA
	GetLine				*gl;		/* line editor context; this could actually be one context per thread */
#endif
#ifdef HAVE_BFD_DISASSEMBLER
	disassemble_info	dinfo;		/* context for the disassembler */
#endif
} CexpContextRec;

/* register/unregister the cexpCurrentContext with the OS */
void				cexpContextRegister(void);
void				cexpContextUnregister(void);
/* retrieve/set the current task's context */
CexpContext			cexpContextGetCurrent(void);
CexpContext			cexpContextSetCurrent(CexpContext);

/* Note that the Register/Unregister and the Set/Get
 * semantics are actually two different approaches to
 * the problem.
 * Register/Unregister is used in combination with
 * RTEMS task variables (Get/Set map to simple
 * variable assignments in this case).
 * Get/Set are used by the EPICS or pthread APIs
 * where a task specific variable must explicitely
 * retrieved and stored (Register/Unregister do
 * nothing in this case).
 */

#ifdef NO_THREAD_PROTECTION

typedef CexpContext CexpContextOSD;

#define cexpContextInitOnce()		do {} while (0)
#define cexpContextRegister()		do {} while (0)
#define cexpContextUnregister()		do {} while(0)
#define cexpContextGetCurrent()		cexpCurrentContext
#define cexpContextSetCurrent(c)	(cexpCurrentContext=(c))

#define cexpContextRunOnce(pdone, fn)	do { if (!(*(pdone))) { (*(pdone))++; fn(0); } } while (0)

#elif defined(USE_EPICS_OSI)

typedef epicsThreadPrivateId	CexpContextOSD;

#define cexpContextInitOnce()	do { if (!cexpCurrentContext) cexpCurrentContext = epicsThreadPrivateCreate(); } while (0)

#define cexpContextRegister()	do {  } while (0)
#define cexpContextUnregister()	do {  } while (0)

#define cexpContextGetCurrent()		((CexpContext)epicsThreadPrivateGet(cexpCurrentContext))
/* cexpContextSetCurrent() returns its argument for convenience */
INLINE CexpContext
cexpContextSetCurrent(CexpContext c)
{
extern CexpContextOSD	cexpCurrentContext;
	epicsThreadPrivateSet(cexpCurrentContext,c);
	return c;
}

#define cexpContextRunOnce(pdone, fn)	epicsThreadOnce(pdone,(void (*)(void*))fn,0)

#elif defined(__rtems)

#ifdef RTEMS_TODO_DONE /* see cexplock.h comment */
#include <rtems.h>
#else
#define RTEMS_SELF	0
long rtems_task_variable_add();
long rtems_task_variable_delete();
#endif

typedef CexpContext CexpContextOSD;

#define cexpContextInitOnce()		do {} while (0)
#define cexpContextRegister()		do { \
										rtems_task_variable_add(\
										RTEMS_SELF,\
										&cexpCurrentContext,\
										0 /* context is part of the stack, hence\
										   * released automatically\
										   */); \
			   						} while (0)
#define cexpContextUnregister()		do { \
										rtems_task_variable_delete(\
										RTEMS_SELF,\
										&cexpCurrentContext); \
									} while (0)
#define cexpContextGetCurrent()		cexpCurrentContext
#define cexpContextSetCurrent(c)	(cexpCurrentContext=(c))

/* We assume the first instance of 'cexp' will be executed by
 * an initialization task or a an initialization task will
 * explicitely call cexpInit(), hence we don't bother
 * about race conditions in cexpInit().
 */

#define cexpContextRunOnce(pdone, fn)	do { if (!(*(pdone))) { (*(pdone))++; fn(0); } } while (0)

#else
#error "You need to implement cexpContextRegister & friends for this OS"
#endif

/* OS dependent representation of the thread context */
extern CexpContextOSD cexpCurrentContext;

#endif
