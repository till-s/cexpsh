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

/*
 * Generic routine to dump an attribute table to file 'f';
 * uses 'file_attribute_print' method to dump individual 
 * attributes.
 */
void
pmelf_pub_file_attributes_print(Pmelf_attribute_tbl *patbl, FILE *f)
{
int                  i,idx;
Pmelf_attribute_list *e;

	if ( !f )
		f = stdout;
	fprintf(f,"VENDOR: %s\n", patbl->pv->name);
	fprintf(f,"Fixed Table:\n");
#ifdef DEBUG
	fprintf(f,"  Slots available: %u, used: %u\n", patbl->avail, patbl->idx);
#endif
	idx = -1;
	for ( i=0; i <= patbl->pv->max_tag; i++ ) {
		if ( (idx = patbl->map[i]) ) {
			pmelf_print_attribute(patbl, f, i, &patbl->vals.p_pub[idx]);
		}
	}
	if ( idx < 0 ) {
		fprintf(f,"<EMPTY>\n");
	}
	fprintf(f,"\nVariable Table:\n");
	if ( !patbl->lst ) {
		fprintf(f,"<EMPTY>\n");
	} else {
		for ( e=patbl->lst; e; e=e->next ) {
			pmelf_print_attribute(patbl, f, e->att.pub.tag, &e->att.pub.val);
		}
	}
}
