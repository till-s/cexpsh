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

bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf32_Word     pc   = bfd_get_section_vma(abfd, input_section);
Elf32_Rel      *rel = &r->rel32;
uint8_t        type = ELF32_R_TYPE(rel->r_info);
Elf32_Word     val, add;

	if ( R_386_NONE == type ) {
		/* No-op; BFD uses a zero dst_mask... */
		return bfd_reloc_ok;
	}

	if ( rel->r_offset + sizeof(Elf32_Word) > bfd_get_section_size(input_section) )
		return bfd_reloc_outofrange;

	pc += rel->r_offset;

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;


	val = 0;
	switch ( type ) {
		default:
		return bfd_reloc_notsupported;

		case R_386_PC32: val = -(Elf32_Word)pc;

		case R_386_32:	 memcpy(&add, (char*)pc, sizeof(add));

						 val += add + bfd_asymbol_value(psym);
						 memcpy((char*)pc, &val, sizeof(val));
		break;
	}

	return bfd_reloc_ok;
}

const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *r)
{
	return pmelf_i386_rel_name(&r->rel32);
}

