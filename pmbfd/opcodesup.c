/*$Id$*/

/* support for libopcodes (BFD disassembler) */

#include "pmbfdP.h"

/* Read big endian into host order    */
bfd_vma
bfd_getb32(const void *p)
{
uint8_t c[sizeof(uint32_t)];
	memcpy(c,p,sizeof(c));
	return ((uint32_t)c[0]<<24)|((uint32_t)c[1]<<16)|((uint32_t)c[2]<<8)|c[3];
}

/* Read little endian into host order */
bfd_vma bfd_getl32(const void *p)
{
uint8_t c[sizeof(uint32_t)];
	memcpy(c,p,sizeof(c));
	return ((uint32_t)c[3]<<24)|((uint32_t)c[2]<<16)|((uint32_t)c[1]<<8)|c[0];
}

/* CPU-specific instruction sets      */
unsigned bfd_m68k_mach_to_features (int mach)
{
	/* Not implemented yet */
	return 0;
}


