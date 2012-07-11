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

FILE *pmelf_err = 0;

int
pmelf_seek(Elf_Stream s, Pmelf_Off where)
{
	return s->seek ? s->seek(s->f, where, SEEK_SET) : -1;
}

int
pmelf_tell(Elf_Stream s, Pmelf_Off *ppos)
{
off_t pos;

	if ( ! s->tell )
		return -1;

	if ( ((off_t)-1) == (pos = s->tell(s->f)) )
		return -1;

	*ppos = (Pmelf_Off)pos;
	return 0;
}

void
pmelf_delstrm(Elf_Stream s, int noclose)
{
	if ( s ) {
		if ( !noclose && s->close )
			s->close(s->f);
		free(s->name);
		free(s);
	}
}

void
pmelf_set_errstrm(FILE *f)
{
	pmelf_err = f;
}
