#define _XOPEN_SOURCE 500

#include "cexplock.h"

#ifdef USE_EPICS_OSI

#elif defined(HAVE_PTHREADS)

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

pthread_mutexattr_t cexpMutexAttributes;

int
cexpLockCreate(CexpLock *l)
{
int rval;
	if ( ! (*l = malloc(sizeof(**l))) )
		return -1;

	if ( (rval = pthread_mutex_init(*l, &cexpMutexAttributes)) )
		free(*l);

	return rval;
}

int
cexpLockDestroy(CexpLock l)
{
int rval;
	rval = pthread_mutex_destroy(l);
	free(l);
	return rval;
}

int
cexpLockingInitialize()
{
int rval;

	memset(&cexpMutexAttributes, 0, sizeof(cexpMutexAttributes));

	if ( (rval = pthread_mutexattr_init(&cexpMutexAttributes)) ) {
		fprintf(stderr,"pthread_mutexattr_init failed: %s\n", strerror(rval));
		return rval;
	}

	if ( (rval = pthread_mutexattr_setprotocol(&cexpMutexAttributes, PTHREAD_PRIO_INHERIT)) ) {
		fprintf(stderr,"pthread_mutexattr_setprotocol failed: %s\n", strerror(rval));
		return rval;
	}

	if ( (rval = pthread_mutexattr_settype(&cexpMutexAttributes, PTHREAD_MUTEX_RECURSIVE)) ) {
		fprintf(stderr,"pthread_mutexattr_settype failed: %s\n", strerror(rval));
		return rval;
	}

	return 0;
}
#endif
