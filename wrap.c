/* Id */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* wrappers to make libelf work with RTEMS' TFTP file system
 * (which has no mmap, not even lseek - therefore libelf doesn't
 * work...)
 * This wrapper substitutes lseek/ftruncate by routines which
 * malloc() buffer memory and fake mmap/munmap by returning
 * / writing this buffer memory.
 *
 * NOTE: this is kind of a hack because it makes some assumptions
 *       about libelf internals. It should work with libelf-0.8.0.
 *
 *       THIS WRAPPER SHOULD NOT BE USED WITH SOFTWARE OTHER THAN
 *       LIBELF-0.8.0
 *
 * Assumptions:
 *   elf_begin() makes one call to lseek(0,SEEK_END) to find out
 *   the file size and essentially reads the entire file
 *   with one call to mmap().
 *
 *   elf_update calls ftruncate(0,len) to create an empty file
 *   which is then mmapp()ed as one chunk.
 *
 * --> lseek(SEEK_END) reads the file 'realloc()ing' buffer space.
 * --> mmap() returns a buffer
 * --> munmap() writes a dirty buffer back and releases it.
 * --> ftruncate allocates a buffer and zeroes it.
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

#define MAXBUFS 10
#define BLOCKSZ 1000

#define FLG_READ	1
#define FLG_WRITE	2

#ifdef LET_LD_DO_THE_WRAPPING
/* use --wrap=lseek etc. when linking */
#define my_lseek		__wrap_lseek
#define my_mmap			__wrap_mmap
#define my_munmap		__wrap_munmap
#define my_ftruncate	__wrap_ftruncate
#else
/* hardcoded names, these must be changed in the libelf sources */
#define my_lseek		libelf_lseek_hack
#define my_mmap			libelf_mmap_hack
#define my_munmap		libelf_munmap_hack
#define my_ftruncate	libelf_ftruncate_hack
#endif

typedef struct hackbuf_ {
	char	 		*d;
	unsigned long	s;
	unsigned long	flags;
} HackBuf;

static HackBuf bufs[MAXBUFS]={{0},};

off_t
my_lseek(int fd, off_t off, int whence)
{
off_t	rval=0,got;

	fprintf(stderr,"LSEEK WRAP: seek to %li\n",(unsigned long)off);

	assert(0==off);
	assert(fd<MAXBUFS);

	switch (whence) {
		case SEEK_END:
			assert(0==bufs[fd].d);
			/* slurp in the file */
			fprintf(stderr,"LSEEK WRAP: slurping on fd %i into buffer until EOF\n",fd);
			do {
				assert(bufs[fd].d=realloc(bufs[fd].d,rval+BLOCKSZ));
				got=read(fd,bufs[fd].d+rval,BLOCKSZ);
				rval+=got;
			} while (got > 0);
			assert(0==got);
			bufs[fd].s=rval;
			bufs[fd].flags=FLG_READ;
		break;
				


		default:
			/* ignore */
		break;
	}

	return rval;
}

void *
my_mmap(void *start, size_t len, int prot, int flags, int fd, off_t offset)
{
	assert(0==start);
	fprintf(stderr,"MMAP WRAP: Mapping fd %li, offset 0x%08lx, len %li\n",
			(long)fd, (unsigned long)offset, (unsigned long)len);
	assert(bufs[fd].d);
	return (void*)(bufs[fd].d+offset);
}

int
my_munmap(void *start, size_t len)
{
int i;
	for (i=0; i<MAXBUFS; i++) {
		if (bufs[i].d && (char *)start>=bufs[i].d && (char *)start<bufs[i].d+bufs[i].s) {
			fprintf(stderr,"MUNMAP WRAP: releasing buffer #%i (starts at %p, release addr was %p)",
					i, bufs[i].d, start);
			if (bufs[i].flags&FLG_WRITE) {
				fprintf(stderr,"--writing back to file--");
				write(i,start,len);
			}
			fprintf(stderr,"\n");
			free(bufs[i].d);
			bufs[i].d=0;
			bufs[i].s=0;
			break;
		}
	}
return -1;
}

int
my_ftruncate(int fd, off_t len)
{
	assert(fd<MAXBUFS);
	if (len>0) {
		assert(0==bufs[fd].d);
		assert(bufs[fd].d=malloc(len));
		memset(bufs[fd].d,0,len);
		bufs[fd].s=len;
		bufs[fd].flags=FLG_WRITE;
	}
	return 0;
}

void
wrap_release_elf_buffers(void)
{
int i;
	for (i=0; i<MAXBUFS; i++) {
		if (bufs[i].d) {
			fprintf(stderr,"ELF: writing back buffers not implemented - all changes are lost!\n");
			free(bufs[i].d);
			bufs[i].d=0;
			bufs[i].s=0;
		}
	}
}
