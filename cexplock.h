/* $Id$ */
#ifndef CEXP_LOCK_H
#define CEXP_LOCK_H

/* Wrapper header for mutexes */

#ifdef USE_EPICS_LOCKS

#include <epicsMutex.h>

typedef epicsMutexId CexpLock;
typedef epicsEventId CexpEvent;

static __inline__ int
cexpLockCreate(CexpLock *lp)
{
	return ! (*lp=epicsMutexCreate());
}

static __inline__ int
cexpEventCreate(CexpEvent *id)
{
	return ! (*id=epicsEventCreate(EpicsEventEmpty));
}

#define cexpLock(l)   		epicsMutexLock((l))
#define cexpUnlock(l) 		epicsMutexUnlock((l))
#define cexpLockDestroy(l)	epicsMutexDestroy((l))

#define cexpEventSend(e)	epicsEventSignal(e)
#define cexpEventWait(e)	epicsEventWait(e)
#define cexpEventDestroy(e)	epicsEventDestroy(e)


#elif defined(__rtems)

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

typedef rtems_id CexpLock;
typedef rtems_id CexpEvent;

#define cexpLock(l) 		rtems_semaphore_obtain((l), RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define cexpEventWait(l) 	rtems_semaphore_obtain((l), RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define cexpUnlock(l)		rtems_semaphore_release((l))
#define cexpEventSend(l)	rtems_semaphore_release((l))

/* IMPORTANT: use a standard (not simple) binary semaphore that may nest */
static inline int
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

static inline int
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
#define cexpLock(l)		do {} while(0)
#define cexpUnlock(l)		do {} while(0)
#define cexpLockCreate(l)	do {} while(0)
#define cexpLockDestroy(l)	do {} while(0)
#else
#error "thread protection not implemented for this target system"
#endif

typedef struct CexpRWLockRec_ {
	CexpLock	mutex;
	unsigned	readers;
	CexpEvent	nap;
	unsigned	sleeping_writers;
} CexpRWLockRec, *CexpRWLock;

static __inline__ void
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
static __inline__  void
cexpReadLock(CexpRWLock l)
{
	cexpLock(l->mutex);
	l->readers++;
	cexpUnlock(l->mutex);
}

static __inline__ void
cexpReadUnlock(CexpRWLock l)
{
	cexpLock(l->mutex);
	if (0 == --l->readers && l->sleeping_writers) {
		l->sleeping_writers--;
		cexpEventSend(l->nap);
	}
	cexpUnlock(l->mutex);
}

static __inline__ void
cexpWriteLock(CexpRWLock l)
{
	cexpLock(l->mutex);
	while (l->readers) {
		l->sleeping_writers++;
		cexpUnlock(l->mutex);
		cexpEventSleep(l->nap);
		cexpLock(l->mutex);
	}
}

static __inline__ void
cexpWriteUnlock(CexpRWLock l)
{
	cexpUnlock(l->mutex);
}

#endif
