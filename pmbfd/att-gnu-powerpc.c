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

#define Tag_GNU_Power_ABI_FP	4
#define Tag_GNU_Power_ABI_VEC	8

#define GNU_Power_ABI_SPE	    3
#define GNU_Power_ABI_ALTIVEC	2
#define GNU_Power_ABI_NOVEC     1

/* PPC-specific (SYSV FP and ALTIVEC ABIs) tags */
static Pmelf_pub_attribute_t
pmelf_ppc_attribute_tag_t(Pmelf_attribute_tbl *patbl, Elf32_Word tag)
{
	switch ( tag ) {
		case Tag_Compat:            return Pmelf_Attribute_Type_both;
		case Tag_GNU_Power_ABI_FP:  return Pmelf_Attribute_Type_num;
		case Tag_GNU_Power_ABI_VEC: return Pmelf_Attribute_Type_num;
		default:
		break;
	}
	return Pmelf_Attribute_Type_unknown;
}

/* PPC-specific (SYSV FP and ALTIVEC ABIs) tag descriptions */
static const char * const
pmelf_ppc_attribute_tag_n(Pmelf_attribute_tbl *patbl, Elf32_Word tag)
{
	switch ( tag ) {
		case Tag_Compat:            return "Tag_Compat";
		case Tag_GNU_Power_ABI_FP:  return "Tag_GNU_Power_ABI_FP";
		case Tag_GNU_Power_ABI_VEC: return "Tag_GNU_Power_ABI_VEC";
		default:
		break;
	}
	return 0;
}

/* PPC-specific (SYSV FP ABI) attribute value descriptions */
static const char *
ppc_abi_fp_val(int val)
{
	switch ( val ) {
		case 0: return "hard-or-soft-float";
		case 1:	return "hard-float";
		case 2:	return "soft-float";
		default:
		break;
	}
	return 0;
}

/* PPC-specific (SYSV ALTIVEC ABI) attribute value descriptions */
static const char *
ppc_abi_vec_val(int val)
{
	switch ( val ) {
		case 1:	return "abi-no-vec";
		case 2:	return "abi-altivec";
		case 3:	return "abi-spe";
		default:
				break;
	}
	return 0;
}

/*
 * Dump PPC-specific (SYSV FP and ALTIVEC ABIs) attribute in 'public'
 * representation (assume 'att' is a pointer to a Pmelf_pub_attribute)
 *
 * RETURNS: number of chars printed.
 */
static int
pmelf_ppc_file_attribute_print(Pmelf_attribute_tbl *patbl, FILE *f, Elf32_Word tag, void *att)
{
Pmelf_pub_attribute   *a   = att;
const char            *tn  = pmelf_ppc_attribute_tag_n(patbl, tag);
const char            *tv  = 0;
int                   rval = 0;

	switch ( tag ) {
		case Tag_GNU_Power_ABI_FP:
			tv = ppc_abi_fp_val(a->i);
		break;

		case Tag_GNU_Power_ABI_VEC:
			tv = ppc_abi_vec_val(a->i);
		break;
	}

	rval += fprintf(f, "Tag: ");
	if ( tn )
		rval += fprintf(f, "%25s",tn);
	else
		rval += fprintf(f, "%25u",tag);

	rval += fprintf(f, " == ");

	if ( tv )
		rval += fprintf(f, "%s", tv);
	else {
		if ( Tag_Compat == tag )
			rval += fprintf(f, "%u, %s", a->i, a->s ? a->s : "<NONE>");
		else if ( TAG_NUMERICAL(tag) ) 
			rval += fprintf(f, "%u", a->i);
		else
			rval += fprintf(f, "%s", a->s ? a->s : "<NULL>");
	}
	fprintf(f,"\n");

	return rval;
}

/* filter unknown tags */
static int
gnu_ppc_unknown(Elf32_Word tag)
{
	switch ( tag ) {
		case Tag_Compat:
		case Tag_GNU_Power_ABI_FP:
		case Tag_GNU_Power_ABI_VEC:
		return -1;
	}
	return tag;
}

/*
 * Scan attribute table 'patbl' for unknown tags
 *
 * Note: -1 return value is success; unkown tag
 *       (>=0) is returned on failure.
 */
static int
filter_gnu_ppc_unknown(const char *nm, Pmelf_attribute_tbl *patbl)
{
int                  i;
int                  rval;
Pmelf_attribute_list *el;
const char           *str;

	/* Look for unknown tags */
	for ( i=0; i<=patbl->pv->max_tag; i++ ) {
		if ( patbl->map[i] && (rval = gnu_ppc_unknown(i)) >= 0 )
			goto bail;
	}

	for ( el = patbl->lst; el; el=el->next ) {
		if ( (rval = gnu_ppc_unknown(el->att.pub.tag)) >= 0 )
			goto bail;
	}

	return -1;

bail:
	PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_ppc_file_attributes_match(): Unknown Tag ");
	if ( patbl->pv->file_attributes_tag_name && (str = patbl->pv->file_attributes_tag_name(patbl, rval)) )
		PMELF_PRINTF( pmelf_err, "%s", str);
	else
		PMELF_PRINTF( pmelf_err, "#%u", rval);
	PMELF_PRINTF( pmelf_err, "int object %s\n", nm);
	return rval;
}

static void
ppc_complain(const char *nm, const char *(*valfun)(int), int val)
{
const char *str;
	PMELF_PRINTF( pmelf_err, "Object %s was built with ", nm);

	if ( (str = valfun(val)) )
		PMELF_PRINTF( pmelf_err, "%s", str);
	else
		PMELF_PRINTF( pmelf_err, "%u", val);
	PMELF_PRINTF( pmelf_err, "\n");
}

/*
 * PPC-specific (SYSV FP and ALTIVEC ABIs) attribute comparison
 * (main reason for this whole facility to exist...)
 */
static int
pmelf_ppc_file_attributes_match(Pmelf_attribute_tbl *patbl, Pmelf_attribute_tbl *pbtbl)
{
int         tag;
const char *str;
int         va,vb;

	if ( !patbl && !pbtbl )
		return 0;
	if ( ! (patbl && pbtbl) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_ppc_file_attributes_match(): only one object has attributes\n");
		return -1;
	}

	if ( (tag = filter_gnu_ppc_unknown(patbl->aset->obj_name, patbl)) >= 0 ) {
		return -1;
	}

	if ( (tag = filter_gnu_ppc_unknown(pbtbl->aset->obj_name, pbtbl)) >= 0 ) {
		return -1;
	}

	va = patbl->vals.p_pub[patbl->map[Tag_GNU_Power_ABI_FP]].i;
	vb = pbtbl->vals.p_pub[pbtbl->map[Tag_GNU_Power_ABI_FP]].i;

	if ( va > 2 ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_ppc_file_attributes_match(): unkown value %u of GNU_Power_ABI_FP attribute in %s\n", va, patbl->aset->obj_name);
	}
	if ( vb > 2 ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_ppc_file_attributes_match(): unkown value %u of GNU_Power_ABI_FP attribute in %s\n", vb, pbtbl->aset->obj_name);
	}

	if ( va != vb && ( va != 0 && vb != 0 ) ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_ppc_file_attributes_match(): mismatch of GNU_Power_ABI_FP attribute\n");

		ppc_complain(patbl->aset->obj_name, ppc_abi_fp_val, va);
		ppc_complain(pbtbl->aset->obj_name, ppc_abi_fp_val, vb);

		return -1;
	}

	va = patbl->vals.p_pub[patbl->map[Tag_GNU_Power_ABI_VEC]].i;
	vb = pbtbl->vals.p_pub[pbtbl->map[Tag_GNU_Power_ABI_VEC]].i;

	if ( va != vb ) {
		if (    (va == GNU_Power_ABI_NOVEC && vb == GNU_Power_ABI_ALTIVEC)
	         || (vb == GNU_Power_ABI_NOVEC && va == GNU_Power_ABI_ALTIVEC) ) {
			/* let altivec live with no-altivec (but not spe) -- I can't see any
			 * incompatibilities ATM
			 */
		} else {
			PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_ppc_file_attributes_match(): mismatch of GNU_Power_ABI_VEC attribute\n");

			ppc_complain(patbl->aset->obj_name, ppc_abi_vec_val, va);
			ppc_complain(pbtbl->aset->obj_name, ppc_abi_vec_val, vb);

			return -1;
		}
	}
	return pmelf_pub_file_attributes_match_compat(patbl,pbtbl);
}

/* PPC-SYSV-gnu 'vendor' table */

Pmelf_attribute_vendor pmelf_attributes_vendor_gnu_ppc = {
	next:	                     0,
	name:	                     "gnu",
	file_attributes_read:        pmelf_pub_file_attributes_read,
	file_attributes_match:       pmelf_ppc_file_attributes_match,
	file_attributes_destroy:     pmelf_pub_file_attributes_destroy,
	file_attributes_tag_type:    pmelf_ppc_attribute_tag_t,
	file_attributes_tag_name:    pmelf_ppc_attribute_tag_n,
	file_attributes_print:       pmelf_pub_file_attributes_print,
	file_attribute_print:        pmelf_ppc_file_attribute_print,
	max_tag:                     10
};
