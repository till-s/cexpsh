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
 * Return the first 'Tag_Compat' element in the list attached
 * to the table 'patbl' or NULL if no such tag is present.
 */
static Pmelf_attribute_list *
find_compat(Pmelf_attribute_tbl *patbl)
{
Pmelf_attribute_list *el;
	for ( el=patbl->lst; el; el=el->next )
		if ( Tag_Compat == el->att.pub.tag )
			break;
	return el;
}

/*
 * Scan all 'Tag_Compat' attributes starting with 'el' and following
 * 'el' (assuming the list is sorted so that identical tags are
 * listed adjacent to each other) and return nonzero if any
 * attribute (nonzero flag) asks for a vendor other than "gnu"
 */
static int filter_nongnu(const char *nm, Pmelf_attribute_list *el)
{
int flgs_gt_1 = 0;
	while ( el && Tag_Compat == el->att.pub.tag ) {
		if ( el->att.pub.val.i > 0 ) {
			if ( !el->att.pub.val.s ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_match_compat - filter_nongnu: no tag string???\n");
				PMELF_PRINTF( pmelf_err, "(Tag_Compat, object %s\n", nm);
				return -1;
			}
			if ( strcmp("gnu", el->att.pub.val.s) ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_match_compat - object '%s' requests toolchain '%s'\n", nm, el->att.pub.val.s);
				return -1;
			}
			if ( el->att.pub.val.i > 1 )
				flgs_gt_1++;
		}
		el = el->next;
	}
	return flgs_gt_1;
}

/*
 * Scan 'Tag_Compat' attributes and verify that list 'elb' contains
 * elements matching all elements in 'ela' with flag > 1.
 * (But 'elb' could still contain flags > 1 not present in 'ela'!)
 *
 * Return 0 on success (nonzero flags in 'ela' are a subset of
 * nonzero flags in 'elb'), nonzero if mismatch is found (at least
 * one nonzero flag in 'ela' is not present in 'elb').
 */
static int
filter_gt1(const char *nma, const char *nmb, Pmelf_attribute_list *ela, Pmelf_attribute_list *elb)
{
Pmelf_attribute_list *pa, *pb, *pn;
Elf32_Word            flg;

	for ( pa = ela; pa && Tag_Compat == pa->att.pub.tag; pa=pa->next ) {
		if ( ( flg = pa->att.pub.val.i) > 1 ) {
			pn = elb;
			do {
				pb = pn;
				if ( !pb || Tag_Compat != pb->att.pub.tag ) {
					/* flag not found in set B */
					PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_match_attribute_set(): flag %u present in object %s not found in object %s\n", flg, nma, nmb);
					return -1;
				}
				pn = pn->next;
			} while ( flg != pb->att.pub.val.i );
			/* this flag found; proceed to next */
		}
	}
	return 0;
}

/*
 * Match all Tag_Compat attributes found in tables 'patbl' and 'pbtbl'
 *
 * RETURN: zero if no incompatibility was found, nonzero otherwise.
 */
int
pmelf_pub_file_attributes_match_compat(Pmelf_attribute_tbl *patbl, Pmelf_attribute_tbl *pbtbl)
{
Pmelf_attribute_list *ela, *elb, *pa, *pb;
int                   fla, flb, fl_cnt;
Elf32_Word            flg;
const char            *nma = patbl->aset->obj_name;
const char            *nmb = pbtbl->aset->obj_name;

	/* check that no non-gnu compatibility is requested */
	if ( patbl && (ela = find_compat(patbl)) && (fla = filter_nongnu(nma, ela)) < 0 ) {
		return -1;
	}
	if ( pbtbl && (elb = find_compat(pbtbl)) && (flb = filter_nongnu(nmb, elb)) < 0 ) {
		return -1;
	}

	/* each attribute with flag > 1 must be represented in both sets */
	if ( filter_gt1(nma, nmb, ela,elb) || filter_gt1(nmb, nma, elb,ela) )
		return -1;

	return 0;
}
