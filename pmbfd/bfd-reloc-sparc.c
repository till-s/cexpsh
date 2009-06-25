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

/* THIS IS A STUB ONLY --- NOT YET IMPLEMENTED */

bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf32_Word     pc;
unsigned       sz, algn;
int32_t        val,oval;
int16_t        sval;
int32_t        lim;
uint32_t       msk,lomsk;
Elf32_Rela     *rela = &r->rela32;
uint8_t        type  = ELF32_R_TYPE(rela->r_info);

	if ( R_SPARC_NONE == type ) {
		/* No-op; BFD uses a zero dst_mask... */
		return bfd_reloc_ok;
	}

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;

	pc  = bfd_get_section_vma(abfd, input_section) + rela->r_offset;

	val = bfd_asymbol_value(psym) + rela->r_addend;

#if 0
	/* gcc seems to only use these...
	 *
	 * R_PPC_ADDR16_HA
	 * R_PPC_ADDR16_HI
	 * R_PPC_ADDR16_LO
	 * R_PPC_ADDR24
	 * R_PPC_ADDR32
	 * R_PPC_REL24
	 * R_PPC_REL32
	 * R_PPC_NONE
	 */

	msk   = ~0;
	lim   =  0;
	lomsk =  0;

	algn  =  1;
	sz    =  4;

	switch ( type ) {

		default:
		return bfd_reloc_notsupported;

		case R_PPC_ADDR16_HA:
		case R_PPC_ADDR16_HI:
		case R_PPC_ADDR16_LO: sz = algn = 2;
		break;
	 
	 	case R_PPC_ADDR24:
	 	case R_PPC_REL24:
							  lim = - (1<<(27-1));
		                      if ( R_PPC_REL24 == type )
							  	lim >>= 1;
							  msk   =  0x03fffffc;
							  lomsk =  0x00000003;
	 	case R_PPC_REL32:     sz = algn = 4;
		break;

		/* This is used by gcc to put data at unaligned addresses, too */
	 	case R_PPC_ADDR32:    sz = 4; algn = 1;
		break;
	}

	if ( rela->r_offset + sz > bfd_get_section_size(input_section) )
		return bfd_reloc_outofrange;

	if ( pc & (algn-1)) {
		ERRPR("pmbfd_perform_relocation_ppc(): location to relocate (0x%08"PRIx32") not properly aligned\n", pc);
		return bfd_reloc_other;
	}


	switch ( ELF32_R_TYPE(rela->r_info) ) {
		case R_PPC_REL24:
		case R_PPC_REL32:
			val -= pc;
		break;
		default:
		break;
	}

	if ( lim && (val < lim || val > -lim - 1) )
		return bfd_reloc_overflow;

	if ( lomsk & val ) {
		ERRPR("pmbfd_perform_relocation_ppc(): value (0x%08"PRIx32") not properly aligned\n", val);
		return bfd_reloc_other;
	}

#if DEBUG & DEBUG_RELOC
	fprintf(stderr,"Relocating val: 0x%08"PRIx32", lim: 0x%08"PRIx32", pc: 0x%08"PRIx32"\n",
		val, lim, pc);
#endif

	if ( 2 == sz ) {
		memcpy(&sval, (void*)pc, sizeof(sval));
		oval = sval;
	} else {
		memcpy(&oval, (void*)pc, sizeof(oval));
	}
	val = ( oval & ~msk ) | (val & msk);

	switch ( ELF32_R_TYPE(rela->r_info) ) {
		case R_PPC_ADDR16_HA:
			if ( val & 0x8000 )
				val+=0x10000;
		case R_PPC_ADDR16_HI:
			val >>= 16;
		break;
		default:
		break;
	}

	/* patch back */
	if ( 2 == sz ) {
		sval = val;
		memcpy((void*)pc, &sval, sizeof(sval));
	} else {
		memcpy((void*)pc, &val,  sizeof(val) );
	}

	return bfd_reloc_ok;
#else
	return bfd_reloc_notsupported;
#endif
}

const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *r)
{
	return pmelf_sparc_rel_name(&r->rela32);
}
