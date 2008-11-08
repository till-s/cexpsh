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
#include <inttypes.h>


/* Allocation chunk size */
#define CHUNKSZ 30

/*
 * Decode attributes from byte stream in memory (public format; see header for
 * more detailed description.
 */
int
pmelf_pub_file_attributes_read(Pmelf_attribute_tbl *patbl, const uint8_t *buf, unsigned size)
{
Elf32_Word            tag;
int                   l,n;
Pmelf_pub_attribute_t tagt;
Elf32_Word            ival;
const char *          sval;
Pmelf_attribute_list  *el,**pn,**pp;
unsigned              csz;

	if ( !size )
		return 0;

	if ( ! patbl ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read(): no attribute table (internal error)\n");
		return -1;
	}

	while ( size ) {

		if ( (n = pmelf_guleb128(buf, &tag, size)) < 0 ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() reading beyond subsubsection (getting tag)\n");
			goto cleanup;
		}
		buf  += n;
		size -= n;

		if ( patbl->avail == 0 && (tag <= patbl->pv->max_tag && Tag_Compat != tag) ) {

			csz = CHUNKSZ < patbl->pv->max_tag ? CHUNKSZ : patbl->pv->max_tag;

			if ( !(patbl->vals.p_pub = realloc(patbl->vals.p_pub, sizeof(*patbl->vals.p_pub)*(patbl->idx + csz))) ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read(): no memory\n");
				goto cleanup;
			}

			patbl->avail  += csz;
		}

		ival = 0;
		sval = 0;

		/* FIXME: we cannot have multiple identical tags in the array (but could in the linked list part) */
		if ( (tag <= patbl->pv->max_tag && Tag_Compat != tag) && patbl->map[tag] ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() tag %"PRIi32" already defined\n", tag);
			goto cleanup;
		}

		if ( Tag_Compat == tag ) {
			tagt = Pmelf_Attribute_Type_both;
		} else if ( ! patbl->pv->file_attributes_tag_type ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() vendor %s has no type handler!\n", patbl->pv->name );
			tagt = Pmelf_Attribute_Type_unknown;
		} else {
			tagt = patbl->pv->file_attributes_tag_type(patbl, tag);
		}

		switch ( tagt ) {
			default:
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() unknown tag %"PRIi32"\n", tag );
				goto cleanup;
			case Pmelf_Attribute_Type_none:
				continue;

			case Pmelf_Attribute_Type_num:
			case Pmelf_Attribute_Type_both:
				if ( (n = pmelf_guleb128(buf, &ival, size)) < 0 ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() reading beyond subsubsection (getting (i)value)\n");
					goto cleanup;
				}
				buf  += n;
				size -= n;
				if ( Pmelf_Attribute_Type_num == tagt )
					break;
				/* else fall thru */

			case Pmelf_Attribute_Type_str:
				for ( l=0; buf[l++]; ) {
					if ( l >= size ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() reading beyond subsubsection (getting (s)value)\n");
						goto cleanup;
					}
				}
				sval  = (const char*)buf;
				buf  += l;
				size -= l;
			break;
		}

		/* Tag_Compat always go to the list */
		if ( tag > patbl->pv->max_tag || Tag_Compat == tag ) {
			if ( ! (el = calloc( sizeof(*el), 1 ) ) ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_pub_file_attributes_read() no memory for attribute list element\n");
				goto cleanup;
			}
			/* append to possibly existing instances of 'tag' */
			for ( pp = &patbl->lst; *pp; pp = &(*pp)->next ) {
				if ( (*pp)->att.pub.tag == tag ) {
					for ( pn = &(*pp)->next; *pn && (*pn)->att.pub.tag == tag; pp = pn)
						;
					break;
				}
			}
			el->next                = *pp;
			*pp                     = el;
			el->att.pub.tag         = tag;
			el->att.pub.val.i       = ival;
			el->att.pub.val.s       = sval;
		} else {
			patbl->map[tag]             = patbl->idx;
			patbl->vals.p_pub[patbl->idx].i = ival;
			patbl->vals.p_pub[patbl->idx].s = sval;
			patbl->idx++;
		}
	}

	return 0;

cleanup:
	return -1;
}
