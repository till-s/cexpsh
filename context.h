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

#ifdef USE_TECLA
#include <libtecla.h>
#endif
#ifdef HAVE_BFD_DISASSEMBLER
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

extern CexpContext	cexpCurrentContext;

/* register/unregister the cexpCurrentContext with the OS */
void cexpContextRegister(void);
void cexpContextUnregister(void);

#ifdef NO_THREAD_PROTECTION

#define cexpContextRegister()	do {} while(0)
#define cexpContextUnregister()	do {} while(0)

#elif defined(__rtems)

#ifdef RTEMS_TODO_DONE /* see cexplock.h comment */
#include <rtems.h>
#else
#define RTEMS_SELF	0
long rtems_task_variable_add();
long rtems_task_variable_delete();
#endif

#define cexpContextRegister()	rtems_task_variable_add(\
									RTEMS_SELF,\
									&cexpCurrentContext,\
									0 /* context is part of the stack, hence\
									   * released automatically\
									   */\
			   					)
#define cexpContextUnregister()	rtems_task_variable_delete(\
									RTEMS_SELF,\
									&cexpCurrentContext)
#else
#error "You need to implement cexpContextRegister & friends for this OS"
#endif

#endif
