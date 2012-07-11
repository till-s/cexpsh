/* $Id$ */

/* generic symbol table handling */

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
#include <config.h>
#endif

#ifdef HAVE_RCMD

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#ifndef LOAD_CHUNK
#define LOAD_CHUNK	2000
#endif

#if !defined(HAVE_RTEMS_H) && defined(__rtems__)
/* avoid pulling in networking headers under __rtems__
 * until BSP stuff is separated out from the core
 */
#define	AF_INET	2
extern char *inet_ntop();
extern int	socket();
extern int  select();
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#define RSH_PORT 514

static char *
handleInput(int fd, int errfd, unsigned long *psize)
{
long	n=0,size,avail;
fd_set	r,w,e;
char	errbuf[1000],*ptr,*buf;
struct  timeval timeout;

register long ntot=0,got,put,idx;

	if (n<fd)		n=fd;
	if (n<errfd)	n=errfd;

	n++;

	buf=ptr=0;
	size=avail=0;

	while (fd>=0 || errfd>=0) {
		FD_ZERO(&r);
		FD_ZERO(&w);
		FD_ZERO(&e);

		timeout.tv_sec=5;
		timeout.tv_usec=0;
		if (fd>=0) 		FD_SET(fd,&r);
		if (errfd>=0)	FD_SET(errfd,&r);
		if ((got=select(n,&r,&w,&e,&timeout))<=0) {
				if (got) {
					fprintf(stderr,"rsh select() error: %s.\n",
							strerror(errno));
				} else {
					fprintf(stderr,"rsh timeout\n");
				}
				goto cleanup;
		}
		if (errfd>=0 && FD_ISSET(errfd,&r)) {
				got=read(errfd,errbuf,sizeof(errbuf));
				if (got<0) {
					fprintf(stderr,"rsh error (reading stderr): %s.\n",
							strerror(errno));
					goto cleanup;
				}
				if (got) {
					idx = 0;
					do {
						put = write(2,errbuf+idx,got);
						if ( put <= 0 ) {
							fprintf(stderr,"rsh error (writing stderr): %s.\n",
									strerror(errno));
							goto cleanup;
						}
						idx += put;
						got -= put;
					} while ( got > 0 );
				} else {
					errfd=-1; 
				}
		}
		if (fd>=0 && FD_ISSET(fd,&r)) {
				if (avail < LOAD_CHUNK) {
					size+=LOAD_CHUNK; avail+=LOAD_CHUNK;
					if (!(buf=realloc(buf,size))) {
						fprintf(stderr,"out of memory\n");
						goto cleanup;
					}
					ptr = buf + (size-avail);
				}
				got=read(fd,ptr,avail);
				if (got<0) {
					fprintf(stderr,"rsh error (reading stdout): %s.\n",
							strerror(errno));
					goto cleanup;
				}
				if (got) {
					ptr+=got;
					ntot+=got;
					avail-=got;
				} else {
					fd=-1;
				}
		}
	}
	if (psize) *psize=ntot;
	return buf;
cleanup:
	free(buf);
	return 0;
}

char *
rshLoad(char *host, char *user, char *cmd)
{
char	*chpt=host,*buf=0;
int		fd,errfd;
long	ntot;
extern  int rcmd();

	fd=rcmd(&chpt,RSH_PORT,user,user,cmd,&errfd);
	if (fd<0) {
		fprintf(stderr,"rcmd: got no remote stdout descriptor\n");
		goto cleanup;
	}
	if (errfd<0) {
		fprintf(stderr,"rcmd: got no remote stderr descriptor\n");
		goto cleanup;
	}

	if (!(buf=handleInput(fd,errfd,&ntot))) {
		goto cleanup; /* error message has already been printed */
	}

	fprintf(stderr,"0x%lx (%li) bytes read\n",ntot,ntot);

cleanup:
	if (fd>=0)		close(fd);
	if (errfd>=0)	close(errfd);
	return buf;
}
#endif
