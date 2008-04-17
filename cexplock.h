/* $Id$ */
#ifndef CEXP_LOCK_H
#define CEXP_LOCK_H

/* Wrapper header for mutexes */

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EPICS_OSI

#include <epicsMutex.h>
#include <epicsEvent.h>

typedef epicsMutexId CexpLock;
typedef epicsEventId CexpEvent;

INLINE int
cexpLockCreate(CexpLock *lp)
{
	return ! (*lp=epicsMutexCreate());
}

INLINE int
cexpEventCreate(CexpEvent *id)
{
	return ! (*id=epicsEventCreate(epicsEventEmpty));
}

#define cexpLock(l)   		epicsMutexLock((l))
#define cexpUnlock(l) 		epicsMutexUnlock((l))
#define cexpLockDestroy(l)	epicsMutexDestroy((l))

#define cexpEventSend(e)	epicsEventSignal(e)
#define cexpEventWait(e)	epicsEventWait(e)
#define cexpEventDestroy(e)	epicsEventDestroy(e)


#elif defined(__rtems__)

#if defined(RTEMS_TODO_DONE) /* avoid pulling in <rtems.h> until we can do this in a BSP independent way */
#include <rtems.h>
#else
#define rtems_id unsigned long
long rtems_semaphore_obtain();
long rtems_semaphore_release();
long rtems_semaphore_create();
long rtems_semaphore_destroy();

#define rtems_build_name( _C1, _C2, _C3, _C4 ) \
  ( (_C1) << 24 | (_C2) << 16 | (_C3) << 8 | (_C4) )
#define RTEMS_NO_TIMEOUT 				0
#define RTEMS_WAIT		 				0
#define RTEMS_FIFO						0
#define RTEMS_PRIORITY					0x04	/* must be set to get priority inheritance */
#define RTEMS_BINARY_SEMAPHORE			0x10
#define RTEMS_SIMPLE_BINARY_SEMAPHORE	0x20
#define RTEMS_INHERIT_PRIORITY			0x40
#endif

#ifndef INLINE
#define INLINE static __inline__
#endif

typedef rtems_id CexpLock;
typedef rtems_id CexpEvent;

#define cexpLock(l) 		rtems_semaphore_obtain((l), RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define cexpEventWait(l) 	rtems_semaphore_obtain((l), RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define cexpUnlock(l)		rtems_semaphore_release((l))
#define cexpEventSend(l)	rtems_semaphore_release((l))

/* IMPORTANT: use a standard (not simple) binary semaphore that may nest */
INLINE int
cexpLockCreate(CexpLock *l)
{
	return
	rtems_semaphore_create(
		rtems_build_name('c','e','x','p'),
		1,/*initial count*/
		RTEMS_PRIORITY|RTEMS_BINARY_SEMAPHORE|RTEMS_INHERIT_PRIORITY,
		0,
		l);
}

INLINE int
cexpEventCreate(CexpEvent *pe)
{
	return
	rtems_semaphore_create(
		rtems_build_name('c','e','x','p'),
		0,/*initial count*/
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		pe);
}

#define cexpLockDestroy(l) rtems_semaphore_delete((l))
#define cexpEventDestroy(l) rtems_semaphore_delete((l))

#elif defined(NO_THREAD_PROTECTION)

typedef void *CexpLock;
typedef void *CexpEvent;

#define cexpLock(l)		do {} while(0)
#define cexpUnlock(l)		do {} while(0)
#define cexpLockCreate(l)	do {} while(0)
#define cexpLockDestroy(l)	do {} while(0)

#define cexpEventSend(l)	do {} while(0)
#define cexpEventWait(l)	do {} while(0)
#define cexpEventCreate(l)	do {} while(0)
#define cexpEventDestroy(l)	do {} while(0)

#define cexpReadLock(l)		do {} while(0)
#define cexpReadUnlock(l)	do {} while(0)
#define cexpWriteLock(l)	do {} while(0)
#define cexpWriteUnlock(l)	do {} while(0)
#define cexpRWLockInit(l)	do {} while(0)

#else
#error "thread protection not implemented for this target system"
#endif

typedef struct CexpRWLockRec_ {
	CexpLock	mutex;
	unsigned	readers;
	CexpEvent	nap;
	unsigned	sleeping_writers;
} CexpRWLockRec, *CexpRWLock;

#ifndef NO_THREAD_PROTECTION
INLINE void
cexpRWLockInit(CexpRWLock pl)
{
	cexpLockCreate(&pl->mutex);
	cexpEventCreate(&pl->nap);
	pl->readers=0;
	pl->sleeping_writers=0;
}

/* Readers / Writer lock implementation. This
 * should be a little more efficient for the common
 * case of a single reader which only has to 
 * acquire one mutex.
 * NOTE: A thread holding the write-lock may still
 *       do nested readLock() readUnlock() calls.
 *       However, a reader _MUST_NOT_ try to acquire
 *       the write lock!
 */
INLINE  void
cexpReadLock(CexpRWLock l)
{
	cexpLock(l->mutex);
	l->readers++;
	cexpUnlock(l->mutex);
}

INLINE void
cexpReadUnlock(CexpRWLock l)
{
	cexpLock(l->mutex);
	if (0 == --l->readers && l->sleeping_writers) {
		l->sleeping_writers--;
		cexpEventSend(l->nap);
	}
	cexpUnlock(l->mutex);
}

INLINE void
cexpWriteLock(CexpRWLock l)
{
	cexpLock(l->mutex);
	while (l->readers) {
		l->sleeping_writers++;
		cexpUnlock(l->mutex);
		cexpEventWait(l->nap);
		cexpLock(l->mutex);
	}
}

INLINE void
cexpWriteUnlock(CexpRWLock l)
{
	cexpUnlock(l->mutex);
}
#endif

#endif
