/* $Id$ */
#ifndef CEXP_LOCK_H
#define CEXP_LOCK_H

/* Wrapper header for mutexes */

#ifdef USE_EPICS_LOCKS

#include <epicsMutex.h>

typedef epicsMutexId CexpLock;

static __inline__ int
cexpLockCreate(CexpLock *lp)
{
	return (*lp=epicsMutexCreate()) ? 0 : -1;
}

#define cexpLock(l)   		epicsMutexLock((l))
#define cexpUnlock(l) 		epicsMutexUnlock((l))
#define cexpLockDestroy(l)	epicsMutexDestroy((l))


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
#define RTEMS_NO_TIMEOUT 		0
#define RTEMS_WAIT		 		0
#define RTEMS_PRIORITY			0x04	/* must be set to get priority inheritance */
#define RTEMS_BINARY_SEMAPHORE	0x10
#define RTEMS_INHERIT_PRIORITY	0x40
#endif

typedef rtems_id CexpLock;

#define cexpLock(l) 	rtems_semaphore_obtain((l), RTEMS_WAIT, RTEMS_NO_TIMEOUT)
#define cexpUnlock(l)	rtems_semaphore_release((l))
/* IMPORTANT: use a standard (not simple) binary semaphore that may nest */
static inline void
cexpLockCreate(CexpLock *l)
{
	rtems_semaphore_create(
		rtems_build_name('c','e','x','p'),
		1,/*initial count*/
		RTEMS_PRIORITY|RTEMS_BINARY_SEMAPHORE|RTEMS_INHERIT_PRIORITY,
		0,
		l);
}

#define cexpLockDestroy(l) rtems_semaphore_delete((l))

#elif defined(NO_THREAD_PROTECTION)
typedef void *CexpLock;
#define cexpLock(l)		do {} while(0)
#define cexpUnlock(l)		do {} while(0)
#define cexpLockCreate(l)	do {} while(0)
#define cexpLockDestroy(l)	do {} while(0)
#else
#error "thread protection not implemented for this target system"
#endif

#endif
