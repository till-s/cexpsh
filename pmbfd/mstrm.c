/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmelf' ELF file reader) was created by
 *     Till Straumann <strauman@slac.stanford.edu>, 2008,
 * 	   Stanford Linear Accelerator Center, Stanford University.
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
 * ------------------ SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 
#include "pmelfP.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

typedef struct _Elf_Memstream {
	struct _Elf_Stream s;
	char               *buf;
	size_t             len;
	off_t              pos;
#ifdef HAVE_SYS_MMAN_H
	FILE               *f;
#endif
} *Elf_Memstream;

static size_t mrd(void *buf, size_t size, size_t nelms, void *p)
{
Elf_Memstream s = p;
size_t        l = size*nelms;

	if ( s->pos + l > s->len ) {
		errno = EINVAL;
		return -1;
	}

	memcpy(buf, &s->buf[s->pos], l);
	s->pos += l;
	return nelms;
}

static int mseek(void *p, off_t offset, int whence)
{
Elf_Memstream s = p;
	if ( SEEK_SET != whence ) {
		errno = ENOTSUP;
		return -1;
	}
	if ( offset < 0 || offset >= s->len ) {
		errno = EINVAL;
		return -1;
	}
	s->pos = offset;
	return 0;
}

static off_t mtell(void *p)
{
Elf_Memstream s = p;
	return s->pos;
}

#ifdef HAVE_SYS_MMAN_H
static int mapclose(void *p, int noclose)
{
int           rval = 0;
Elf_Memstream s = p;
	if ( ! noclose && s->f ) {
		rval = rval || fclose(s->f);
	}
	return rval || munmap(s->buf, s->len);
}
#endif

Elf_Stream
pmelf_memstrm(void *buf, size_t len)
{
Elf_Memstream s;

	if ( len < 1 )
		return 0;

	if ( ! (s = calloc(1, sizeof(*s))) ) {
		return 0;
	}

	if ( ! (s->s.name = strdup("<memory>")) ) {
		free(s);
		return 0;
	}

	s->s.f = s;
	s->buf = buf;
	s->len = len;
	s->pos = 0;

	s->s.read = (void*)mrd;
	s->s.seek = (void*)mseek;
	s->s.tell = (void*)mtell;

	return &s->s;
}

Elf_Stream
pmelf_mapstrm(const char *name, FILE *f)
{
Elf_Stream  s    = 0;
#ifdef HAVE_SYS_MMAN_H
void        *buf = 0;
int         fd   = -1;
struct stat sbf;

	fd = f ? fileno(f) : open(name, O_RDONLY);

	if ( fd < 0 )
		goto cleanup;

	if ( fstat(fd, &sbf) )
		goto cleanup;

	buf = mmap(0, sbf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if ( 0 == buf )
		goto cleanup;

	s   = pmelf_memstrm(buf, sbf.st_size);

	if ( 0 == s )
		goto cleanup;

	/* mapping now owned by memstrm */
	s->close = mapclose;
	((Elf_Memstream)s)->f = f;
	buf      = 0;

	if ( name ) {
		free(s->name);
		s->name = strdup(name);
	}

cleanup:
	if ( buf ) {
		munmap(buf, sbf.st_size);
	}
	if ( fd >= 0 && !f ) {
		close(fd);	
	}
#else
	errno = ENOTSUP;
#endif
	return s;
}
