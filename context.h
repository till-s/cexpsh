/* $Id$ */
#ifndef CEXP_CONTEXT_H
#define CEXP_CONTEXT_H

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cexp.h"

#ifdef USE_EPICS_OSI
/* BFD redefines INLINE if we don't include epicsThread.h first; sigh... */
#include <epicsThread.h>
#endif

#ifdef HAVE_TECLA
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
#ifdef HAVE_TECLA
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
void				cexpContextGetCurrent(CexpContext *);
void				cexpContextSetCurrent(CexpContext);

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
#define cexpContextGetCurrent(pc)	do { *(pc) = cexpCurrentContext;	} while (0)
#define cexpContextSetCurrent(c)	do { cexpCurrentContext=(c);		} while (0)

#define cexpContextRunOnce(pdone, fn)	do { if (!(*(pdone))) {							\
												(*(pdone))++; fn(0);					\
											 }											\
									} while (0)

#elif defined(USE_EPICS_OSI)

typedef epicsThreadPrivateId	CexpContextOSD;

#define cexpContextInitOnce()		do { if (!cexpCurrentContext)								\
											cexpCurrentContext = epicsThreadPrivateCreate();	\
									} while (0)

#define cexpContextRegister()		do {  } while (0)
#define cexpContextUnregister()		do {  } while (0)

#define cexpContextGetCurrent(pc)	do { *(pc) = (CexpContext)epicsThreadPrivateGet(	\
																	cexpCurrentContext	\
																);						\
									} while (0))

#define cexpContextSetCurrent(c)	do { 												\
										extern CexpContextOSD	cexpCurrentContext;		\
										epicsThreadPrivateSet(cexpCurrentContext,(c));	\
									} while (0)

#define cexpContextRunOnce(pdone, fn)	epicsThreadOnce(pdone,(void (*)(void*))fn,0)

#elif defined(__rtems__)

#ifdef RTEMS_TODO_DONE /* see cexplock.h comment */
#include <rtems.h>
#else
#define RTEMS_SELF	0

#ifdef CEXP_RTEMS_NOTEPAD
#error Using a notepad is currently unsupported - it must be initialized to 0 at task creation and I dont know how to do that
#endif

#ifdef CEXP_RTEMS_NOTEPAD
long rtems_task_get_note();
long rtems_task_set_note();
#else
/* use task vars - discouraged */
long rtems_task_variable_add();
long rtems_task_variable_delete();
#endif
#endif

typedef CexpContext CexpContextOSD;


/* We assume the first instance of 'cexp' will be executed by
 * an initialization task or a an initialization task will
 * explicitely call cexpInit(), hence we don't bother
 * about race conditions in cexpInit().
 */
#define cexpContextInitOnce()		do {} while (0)
#define cexpContextRunOnce(pdone, fn)	do { if (!(*(pdone))) {							\
												(*(pdone))++; fn(0);					\
											 }											\
									} while (0)


#ifdef CEXP_RTEMS_NOTEPAD

#define cexpContextRegister()		do {} while (0)
#define cexpContextUnregister()		do {} while (0)

#define cexpContextGetCurrent(pc)	do {												\
										rtems_task_get_note(							\
												RTEMS_SELF,								\
												CEXP_RTEMS_NOTEPAD,						\
												(pc)									\
											);											\
									} while (0)

#define cexpContextSetCurrent(c)	do {												\
										rtems_task_set_note(							\
												RTEMS_SELF,								\
												CEXP_RTEMS_NOTEPAD,						\
												(c)										\
											);											\
									} while (0)
#else /* task var variant (discouraged - increases context switch latency) */
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
#define cexpContextGetCurrent(pc)	do { *(pc) = cexpCurrentContext;	} while (0)
#define cexpContextSetCurrent(c)	do { cexpCurrentContext=(c);		} while (0)
#endif

#else
#error "You need to implement cexpContextRegister & friends for this OS"
#endif

/* OS dependent representation of the thread context */
extern CexpContextOSD cexpCurrentContext;

#endif
