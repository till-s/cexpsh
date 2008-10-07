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

/* FIXME: this is not complete but includes just what 
 *        gcc/linux on x86_64 seem to be using...
 *        (However, the spec says that the 8/16-bit relocs
 *        are not svr4 abi compliant anyways.)
 */
bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf64_Addr     pc = bfd_get_section_vma(abfd, input_section);
int64_t        val;
Elf64_Rela     *rel = &r->rela64;
int64_t        lim;
unsigned       sz;
Elf64_Word     v32;
	

	pc += rel->r_offset;

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;


	val = 0;
	lim = 0;
	sz  = sizeof(Elf64_Addr);

	switch ( ELF64_R_TYPE(rel->r_info) ) {
		default:
		return bfd_reloc_notsupported;

		case R_X86_64_PC32: sz = 4;  lim = -(1LL<<32);
		case R_X86_64_PC64:
                         val = -pc;
		break;

		case R_X86_64_32:   sz = 4;  lim = -(1LL<<32); break;
		case R_X86_64_32S:  sz = 4;  lim = -(1LL<<31); break;
		case R_X86_64_64:	 
		break;
	}

	if ( rel->r_offset + sz > bfd_get_section_size(input_section) )
		return bfd_reloc_outofrange;

	val += rel->r_addend + bfd_asymbol_value(psym);

	if ( lim && (val < lim || val > -lim - 1) )
		return bfd_reloc_overflow;

	switch ( sz ) {
		case sizeof(v32):
			v32 = val;
			memcpy((char*)pc, &v32, sizeof(v32));
		break;

		case sizeof(val):
			memcpy((char*)pc, &val, sizeof(val));
		break;

		default:
			return bfd_reloc_other;
	}


	return bfd_reloc_ok;
}

const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *r)
{
	return pmelf_x86_64_rel_name(&r->rela64);
}
