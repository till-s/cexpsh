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
#include <pmelf.h>
#include <pmelfP.h>
#include <attrP.h>
#include <stdio.h>

/* Decode 'uleb128'-encoded number from memory
 * buffer (of size 'sz').
 *
 * RETURNS: Number of bytes read or -1 on failure
 *          (attempt to read beyond buffer size).
 *
 *          Decoded number is deposited in *p.
 *
 * NOTE:    Decoded number is only 32-bits but
 *          the encoded number could be longer
 *          (if buffer is not exhausted).
 *          Only the least-significant 32-bits
 *          are deposited in '*p' in this case.
 */
int
pmelf_guleb128(const uint8_t *s, Elf32_Word *p, int sz)
{
int     	shft,n;
uint8_t     v;
Elf32_Word  rval;
	
	rval = 0;
	shft = 0;
	n    = 0;

	do {
		if ( sz <= n )
			return -1;
		rval |= ( (v = s[n++]) & 0x7f) << shft;
		shft += 7;
	} while ( v & 0x80 );

	*p = rval;

	return n;
}

