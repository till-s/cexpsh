/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmbfd' BFD emulation for cexpsh) was created by
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

#include "pmbfdP.h"

enum   sparc_rel_check {
	sparc_rel_check_none = 0, /* truncate silently           */
	sparc_rel_check_bits = 1, /* - 2^w     ... + 2^w     - 1 */
	sparc_rel_check_sign = 2, /* - 2^(w-1) ... + 2^(w-1) - 1 */
	sparc_rel_check_unsg = 3  /*         0 ... + 2^w     - 1 */
};

struct sparc_rel_desc {
	unsigned char nbytes;
	unsigned char width;
	unsigned char shift;
	unsigned char sparc_rel_check: 2;
	unsigned char pc_relative:     1;
	unsigned char unaligned:       1;
};

#define REL_DESC(t, n, w, s, c , r, u) \
	[t] { nbytes: n, width: w, shift: s, sparc_rel_check: c, pc_relative: r, unaligned: u }

static struct sparc_rel_desc sparc_rels[] = {
	REL_DESC(R_SPARC_NONE,    0,  0,  0, sparc_rel_check_none, 0, 0),
	REL_DESC(R_SPARC_LO10,    4, 10,  0, sparc_rel_check_none, 0, 0),
	REL_DESC(R_SPARC_8,       1,  8,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_16,      2, 16,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_32,      4, 32,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_DISP8,   1,  8,  0, sparc_rel_check_sign, 1, 0),
	REL_DESC(R_SPARC_DISP16,  2, 16,  0, sparc_rel_check_sign, 1, 0),
	REL_DESC(R_SPARC_DISP32,  4, 32,  0, sparc_rel_check_sign, 1, 0),
	REL_DESC(R_SPARC_WDISP30, 4, 30,  2, sparc_rel_check_sign, 1, 0),
	REL_DESC(R_SPARC_WDISP22, 4, 22,  2, sparc_rel_check_sign, 1, 0),
	REL_DESC(R_SPARC_HI22,    4, 22, 10, sparc_rel_check_none, 0, 0),
	REL_DESC(R_SPARC_22,      4, 22,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_13,      4, 13,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_LO10,    4, 10,  0, sparc_rel_check_none, 0, 0),
	REL_DESC(R_SPARC_PC10,    4, 10,  0, sparc_rel_check_none, 1, 0),
	REL_DESC(R_SPARC_PC22,    4, 22, 10, sparc_rel_check_bits, 1, 0),
	REL_DESC(R_SPARC_UA32,    4, 32,  0, sparc_rel_check_bits, 0, 1),
	REL_DESC(R_SPARC_10,      4, 10,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_11,      4, 11,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_7,       4,  7,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_5,       4,  5,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_6,       4,  6,  0, sparc_rel_check_bits, 0, 0),
	REL_DESC(R_SPARC_UA16,    2, 16,  0, sparc_rel_check_bits, 0, 1),
};

bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf32_Word     pc;
int64_t        val, msk;
int32_t        oval,nval;
int16_t        sval;
int8_t         bval;
int64_t        llim,ulim;
Elf32_Rela     *rela = &r->rela32;
uint8_t        type  = ELF32_R_TYPE(rela->r_info);
struct sparc_rel_desc *dsc;

	if ( R_SPARC_NONE == type ) {
		/* No-op; BFD uses a zero dst_mask... */
		return bfd_reloc_ok;
	}

	/* use R_SPARC_NONE as a dummy for 'unsupported' */
	dsc = type >= sizeof(sparc_rels) ? &sparc_rels[R_SPARC_NONE] : &sparc_rels[type];

	if ( 0 == dsc->nbytes ) {
		ERRPR("pmbfd_perform_relocation_sparc(): unsupported relocation type : %"PRIu8"\n", type);
		return bfd_reloc_notsupported;
	}

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;

	pc  = bfd_get_section_vma(abfd, input_section) + rela->r_offset;

	if ( ! dsc->unaligned && (pc & (dsc->nbytes - 1)) ) {
		ERRPR("pmbfd_perform_relocation_sparc(): location to relocate (0x%08"PRIx32") not properly aligned\n", pc);
		return bfd_reloc_other;
	}

	val = (int64_t)bfd_asymbol_value(psym) + (int64_t)rela->r_addend;

	if ( dsc->pc_relative )
		val -= (int64_t)pc;

	val >>= dsc->shift;

	/* works also if the left shift is 32 */
	msk = (1LL << dsc->width);
	msk--;

	switch ( dsc->sparc_rel_check ) {
		default:
		case sparc_rel_check_none: ulim = ~(1LL<<63);  llim = ~ulim; break;
		case sparc_rel_check_unsg: ulim = msk;         llim = 0;     break;
		case sparc_rel_check_bits: ulim = msk;         llim = ~ulim; break;
		case sparc_rel_check_sign: ulim = msk>>1;      llim = ~ulim; break;
	}

#if (DEBUG & DEBUG_RELOC)
	fprintf(stderr,"Relocating val: 0x%08"PRIx64", ulim: 0x%08"PRIx64", pc: 0x%08"PRIx32", sym: 0x%08lx\n",
		val, ulim, pc, bfd_asymbol_value(psym));
#endif

	if ( val < llim || val > ulim ) {
		return bfd_reloc_overflow;
	}

	if ( 1 == dsc->nbytes ) {
		memcpy(&bval, (void*)pc, sizeof(bval));
		oval = bval;
	} else if ( 2 == dsc->nbytes ) {
		memcpy(&sval, (void*)pc, sizeof(sval));
		oval = sval;
	} else {
		memcpy(&oval, (void*)pc, sizeof(oval));
	}

	nval = ( oval & ~msk ) | (val & msk);

	/* patch back */
	if ( 1 == dsc->nbytes ) {
		bval = nval;
		memcpy((void*)pc, &bval, sizeof(bval));
	} else if ( 2 == dsc->nbytes ) {
		sval = nval;
		memcpy((void*)pc, &sval, sizeof(sval));
	} else {
		memcpy((void*)pc, &nval, sizeof(nval));
	}

	return bfd_reloc_ok;
}

const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *r)
{
	return pmelf_sparc_rel_name(&r->rela32);
}
