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

static Pmelf_attribute_vendor *known_vendors = 0;

/*
 * Read attribute set from memory buffer (see header file for more information).
 */
Pmelf_attribute_set *
pmelf_read_attribute_set(const uint8_t *b, unsigned bsize, int needswap, const char *obj_name)
{
Pmelf_attribute_set    *rval = 0;
Pmelf_attribute_vendor *pv;
Pmelf_Off              n,nbeg;
int                    l;
uint8_t                v;
const char             *vendor_name;
Pmelf_attribute_tbl    *patbl;
Elf32_Word             len,tag,sublen;

	if ( ! known_vendors ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): no vendors registered\n");
		/* No vendors registered; save us the trouble */
		return 0;
	}

	if ( ! (rval = calloc(sizeof(*rval),1)) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): no memory\n");
		return 0;
	}

	if ( ! (rval->obj_name = strdup(obj_name)) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): no memory (for obj_name)\n");
		goto cleanup;
	}

	n = 0;

	while ( n < bsize ) {
		nbeg = n;

		if ( FORMAT_VERSION_A != (v = b[n++]) ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): unknown attribute format version %i\n", v);
			goto cleanup;
		}

		if ( pmelf_getword(needswap, b+n, bsize-n, &len, "subsection") )
			goto cleanup;

		n += sizeof(len);
			
		if ( nbeg + len + 1 > bsize ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): vendor subsection length > section length\n");
			goto cleanup;
		}

		for ( l = n; b[l]; ) {
			if ( ++l >= bsize ) {
				PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): reading beyond section (getting vendor name)\n");
				goto cleanup;
			}
		}

		vendor_name = (const char*)(b + n);

		n = l+1;

		if ( (l = pmelf_guleb128(b+n, &tag, bsize - n)) < 0 ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_size() reading beyond subsubsection (getting tag)\n");
			goto cleanup;
		}

		n += l;

		if ( pmelf_getword(needswap, b + n, bsize - n, &sublen, "subsubsection") )
			goto cleanup;

		n += sizeof(sublen);

		/* subsubsection length includes byte count for tag and length itself */
		sublen -= sizeof(sublen) + l;

		switch ( tag ) {
			case Tag_File:
				for ( pv = known_vendors; pv && strcmp(pv->name, vendor_name); )
					pv = pv->next;

				if ( !pv ) {
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): no handlers for vendor '%s' found\n", vendor_name);
					goto cleanup;
				}

				for ( patbl=0, l=0; l<ATTR_MAX_VENDORS; l++ ) {
					if ( ! rval->attributes[l].file_attributes
						|| rval->attributes[l].file_attributes->pv == pv ) {
						/* setting a covers the case where we use a new empty slot */
						patbl = rval->attributes[l].file_attributes;
						break;
					}
				}

				if ( ! patbl ) {
					if ( l >= ATTR_MAX_VENDORS ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): not enough vendor slots\n");
						goto cleanup;
					}

					/* create attribute table */
					if ( !(patbl = calloc(sizeof(*patbl) + sizeof(patbl->map[0])*(pv->max_tag+1), 1)) ) {
						PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): no memory\n");
						goto cleanup;
					}

					patbl->vals.p_pub   = 0;
					patbl->avail        = 0;
					patbl->idx          = 1;	/* first entry is empty/unused */
					patbl->pv           = pv;
					patbl->aset         = rval;

					rval->attributes[l].file_attributes = patbl;
				}

				/* read attributes into table */
				if ( pv->file_attributes_read(patbl, b+n, sublen) )
					goto cleanup;
			break;

			case Tag_Section:
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): Tag_Section no yet supported\n");
					goto cleanup;
			case Tag_Symbol:
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): Tag_Symbol no yet supported\n");
					goto cleanup;
			default:
					PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_read_attribute_set(): unsupported tag %"PRIi32"\n", tag);
					goto cleanup;
		}

		n = nbeg + len + 1;
	}

	return rval;

cleanup:
	pmelf_destroy_attribute_set(rval);
	return 0;
}

/*
 * Read attribute set from ELF stream (see header file for more information).
 */
Pmelf_attribute_set *
pmelf_create_attribute_set(Elf_Stream s, Elf_Shdr *psect)
{
Pmelf_attribute_set    *rval = 0;
uint8_t                *b    = 0;
Pmelf_Off              sh_size;

	/* Hack; accessing sh_type as ELF32 should work for both, ELF32 and ELF64 */
	if ( psect->s32.sh_type != SHT_GNU_ATTRIBUTES ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_create_attribute_set(): not a .gnu_attributes section\n");
		return 0;
	}

	if ( ! (b = pmelf_getscn(s, psect, 0, 0, 0)) )
		goto cleanup;

	switch ( s->clss ) {
#ifdef PMELF_CONFIG_ELF64SUPPORT
		case ELFCLASS64:
			sh_size   = psect->s64.sh_size;
		break;
#endif
		case ELFCLASS32:
			sh_size   = psect->s32.sh_size;
		break;

		default:
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_create_attribute_set(): unsupported ELF class (64?)\n");
			goto cleanup;
	}

	rval = pmelf_read_attribute_set(b, sh_size, s->needswap, s->name);

	if ( !rval ) 
		goto cleanup;

	rval->scn_data = b;
	b              = 0;

cleanup:
	free( b );
	return rval;
}


/*
 * Destroy attribute set created by
 * pmelf_create_attribute_set() or pmelf_read_attribute_set().
 */
void
pmelf_destroy_attribute_set(Pmelf_attribute_set *pa)
{
int i;
	if ( pa ) {
		for ( i=0; i<ATTR_MAX_VENDORS; i++ ) {
			if ( pa->attributes[i].file_attributes ) {
				pa->attributes[i].file_attributes->pv->file_attributes_destroy(pa->attributes[i].file_attributes);
				free( pa->attributes[i].file_attributes );
				pa->attributes[i].file_attributes = 0;
			}
		}
		free( pa->obj_name );
		free( pa->scn_data );
		free( pa );
	}
}

/*
 * find position index of a particular vendor in the
 * list of all known vendors.
 */
static int
vidx(Pmelf_attribute_vendor *pv)
{
int                     i;
Pmelf_attribute_vendor *p;

	for ( i=0, p=known_vendors; p; p=p->next, i++ ) {
		if ( p == pv )
			return i;
	}
	return -1;
}

/* 
 * Compute bitmask of all vendors present in an attribute set 'pa'
 *
 * E.g., if 'pa' contains attributes associated with vendors "gnu"
 *       and "foo" and the vendors "gnu", "boo" and "foo" are
 *       registered/known then 'vset(pa)' returns 0x5 since it
 *       contains vendors #0 ("gnu") and #2 ("foo").
 */
static int
vset(Pmelf_attribute_set *pa)
{
int i,j;

unsigned rval = 0;

	for ( i=0; i<ATTR_MAX_VENDORS; i++ ) {
		if ( pa->attributes[i].file_attributes ) {
			j = vidx(pa->attributes[i].file_attributes->pv);
			if ( j < 0 ) {
				PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_match_attribute_set(): invalid vendor index ?? (object %s)\n", pa->obj_name);
				return -1;
			}
			rval |= (1<<j);
		}
	}
	return rval;
}

/*
 * Scan attribute sets 'pa' and 'pb' for common vendors and
 * call their file_attributes_match() methods to find incompatibilities.
 *
 * If the vendor sets in 'pa' and 'pb' do not match or if only one set
 * exists (the other being NULL) this is treated as a mismatch.
 *
 * RETURNS: zero if no incompatibility is found, a value < 0 otherwise.
 *
 * NOTE:    it is legal to pass 'pa==NULL', 'pb==NULL' in which case
 *          the routine returns 0 (SUCCESS).
 */
int
pmelf_match_attribute_set(Pmelf_attribute_set *pa, Pmelf_attribute_set *pb)
{
int                    i,j;
Pmelf_attribute_vendor *pv;
int                    vendor_set_a, vendor_set_b;
int                    rval = 0;

	if ( !pa && !pb )
		return 0;

	if ( ! (pa && pb) )
		return -1;

	if ( (vendor_set_a = vset(pa)) < 0 ) {
		return -1;
	}
	if ( (vendor_set_b = vset(pb)) < 0 ) {
		return -1;
	}

	if ( vendor_set_a != vendor_set_b ) {
		PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_match_attribute_set(): vendor set mismatch (objects %s, %s)\n", pa->obj_name, pb->obj_name);
		return -2;
	}

	for ( i=0; i<ATTR_MAX_VENDORS; i++ ) {
		if ( pa->attributes[i].file_attributes ) {
			pv = pa->attributes[i].file_attributes->pv;
			for ( j=0; j<ATTR_MAX_VENDORS; j++ ) {
				if ( pb->attributes[j].file_attributes ) {
					if ( pv == pb->attributes[j].file_attributes->pv ) {
						if ( ! pv->file_attributes_match ) {
							PMELF_PRINTF(pmelf_err, PMELF_PRE"pmelf_match_attribute_set(): vendor %s has no 'match' handler\n", pv->name);
							return -1;
						}
						rval |= pv->file_attributes_match( pa->attributes[i].file_attributes, pb->attributes[j].file_attributes );
					}
				}
			}
		}
	}
	return rval;
}

/* Dump attribute set to FILE 'f' */
void
pmelf_print_attribute_set(Pmelf_attribute_set *pa, FILE *f)
{
int i;

	if ( !f )
		f = stdout;
	for ( i=0; i<sizeof(pa->attributes)/sizeof(pa->attributes[0]); i++ ) {
		Pmelf_attribute_tbl *t = pa->attributes[i].file_attributes;
		if ( t ) {
			if ( t->pv->file_attributes_print )
				t->pv->file_attributes_print(t, f);
			else
				fprintf(f,"Attribute table [%i] (vendor %s) had no print member\n", i, t->pv->name);
		}
	}
}

int
pmelf_attributes_vendor_register(Pmelf_attribute_vendor *pv)
{
Pmelf_attribute_vendor *x;

	for ( x = known_vendors; x; x=x->next ) {
		if ( !strcmp(x->name, pv->name) ) {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_attributes_vendor_register(): vendor \"%s\" already registered\n", pv->name);
			return -1;
		}
	}
	pv->next = known_vendors;
	known_vendors = pv;
	return 0;
}

const char *
pmelf_attributes_vendor_name(Pmelf_attribute_vendor *pv)
{
	return pv->name;
}
