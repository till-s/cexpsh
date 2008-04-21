/* $Id$ */

/* Interface to the BFD disassembler */

/* IMPORTANT LICENSING INFORMATION:
 *
 *  linking this code against 'libbfd'/ 'libopcodes'
 *  may be subject to the GPL conditions.
 *  (This file itself is distributed under the 'SLAC'
 *  license)
 *
 */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *    Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 * 	  Stanford Linear Accelerator Center, Stanford University.
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
 * SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>


#define BUFMAX	500

#include "cexp.h"
#include "context.h"
#include "cexpsymsP.h"
#include "cexpmodP.h"

#ifdef USE_PMBFD
#include "pmbfd.h"
#endif

#include "dis-asm.h"

static disassembler_ftype	bfdDisassembler  = 0;
enum bfd_endian				bfdEndian        = BFD_ENDIAN_UNKNOWN;
enum bfd_flavour            bfdFlavour       = bfd_target_unknown_flavour;
enum bfd_architecture       bfdArch          = bfd_arch_unknown;
unsigned long               bfdMach          = 0;
unsigned int                bfdOctetsPerByte = 1;

typedef struct DAStreamRec_ {
	char	buf[BUFMAX];	/* buffer to assemble the line 		*/
	int		p;				/* 'cursor'							*/
} DAStreamRec, *DAStream;

static int
daPrintf(DAStream s, char *fmt, ...)
{
va_list ap;
int		written;
	va_start(ap, fmt);

#ifdef HAVE_VSNPRINTF
	written=vsnprintf(s->buf+s->p, BUFMAX - s->p, fmt, ap);
#else
	written=vsprintf(s->buf+s->p, fmt, ap);
#endif

	assert(written >= 0  && (s->p+=written) < BUFMAX);

	va_end(ap);
	return written;
}

static void
printSymAddr(bfd_vma addr, CexpModule mod, CexpSym sym,  disassemble_info *di)
{
	if (!sym) {
		di->fprintf_func(di->stream,"?NULL?");
		return;
	} else {
		long diff=addr - (bfd_vma)sym->value.ptv;
		char diffbuf[30];
		if (diff)
			sprintf(diffbuf," + 0x%x",(unsigned)diff);
		else
			diffbuf[0]=0;

		di->fprintf_func(di->stream,
						"<%s:%s%s>",
						cexpModuleName(mod),
						sym->name,
						diffbuf);
	}
}

static void
printAddr(bfd_vma addr, disassemble_info *di)
{
CexpSym		sym;
CexpModule	mod;
	sym = cexpSymLkAddr((void*)addr, 0, 0, &mod);
	printSymAddr(addr,mod,sym,di);
}

static int
symbolAtAddr(bfd_vma addr, disassemble_info *di)
{
CexpSym	s;
	return (s=cexpSymLkAddr((void*)addr,0,0,0)) &&
			(void*)s->value.ptv == (void*)addr;
}

#if 0 /* unused - why did I create that ?? */
static int
readMem(bfd_vma vma, bfd_byte *buf, unsigned int length, disassemble_info *di)
{
	/* memory is already holding the data we want to disassemble */
	return 0;
}
#endif


void
cexpDisassemblerInit(disassemble_info *di, PTR stream)
{
DAStreamRec dummy;

	dummy.p = 0;

	/* newer versions don't export the BFD_VERSION macro anymore :0 */
#ifdef BFD_VERSION
	INIT_DISASSEMBLE_INFO((*di),(PTR)&dummy, (fprintf_ftype)daPrintf);
#else
	init_disassemble_info (di, &dummy, (fprintf_ftype) daPrintf);
#endif

	/* don't need the buffer_length; just set to a value high enough */
	di->buffer_length			  = 100;
	
	di->display_endian 			  = di->endian = bfdEndian;
	di->buffer 					  = (bfd_byte *)cexpDisassemblerInit;
	di->symbol_at_address_func	  = symbolAtAddr;
	di->print_address_func		  = printAddr;

	di->flavour                   = bfdFlavour;
	di->arch                      = bfdArch;
	di->mach                      = bfdMach;
	di->octets_per_byte           = bfdOctetsPerByte;

#ifndef BFD_VERSION
	/* Allow the target to customize the info structure.  */
	disassemble_init_for_target (di);
#endif

	/* disassemble one line to set the bytes_per_line field */
	if (bfdDisassembler) {
		bfdDisassembler((bfd_vma)di->buffer, di);
	}
	/* reset stream */
	di->stream 			= stream;
	di->fprintf_func	= (fprintf_ftype)fprintf;
}

void
cexpDisassemblerInstall(bfd *abfd)
{
	if (bfdDisassembler)
		return; /* has been installed already */

#ifdef USEPMBFD
	/* Special hack; the disassembler asks BFD for 
	 * target properties. If pmbfd receives a NULL
	 * BFD handle then it returns the host machine's
	 * data which is what we want here...
	 */
	abfd = 0;
#endif

	if (!(bfdDisassembler = disassembler(abfd))) {
		bfd_perror("Warning: no disassembler found");
		return;
	}
	if (bfd_big_endian(abfd))
		bfdEndian = BFD_ENDIAN_BIG;
	else if (bfd_little_endian(abfd))
		bfdEndian = BFD_ENDIAN_LITTLE;
	else {
		fprintf(stderr,
			"UNKNOWN BFD ENDIANNESS; unable to install disassembler\n");
		bfdDisassembler=0;
	}
	bfdFlavour       = bfd_get_flavour(abfd);
	bfdArch          = bfd_get_arch(abfd);
	bfdMach          = bfd_get_mach(abfd);
	bfdOctetsPerByte = bfd_octets_per_byte(abfd);
}

static CexpSym	
getNextSym(int *pindex, CexpModule *pmod, void *addr)
{
	if (*pindex < 0 || *pindex >= (*pmod)->symtbl->nentries-1) {
		/* reached the end of the module's symbol table;
		 * search module list again
		 */
		*pindex = cexpSymLkAddrIdx(addr, 0, 0, pmod);
		if (*pindex < 0)
			return 0;
	} else {
		(*pindex)++;
	}
	return (*pmod)->symtbl->aindex[*pindex];
}

int
cexpDisassemble(void *addr, int n, disassemble_info *di)
{
FILE			*f;
fprintf_ftype	orig_fprintf;
DAStreamRec		b;
CexpSym			currSym,nextSym;
CexpModule		currMod,nextMod;
int				found;

	if (!bfdDisassembler) {
		fprintf(stderr,"No disassembler support\n");
		  return -1;
	}

	if (!di) {
		CexpContext currentContext = 0;
	    cexpContextGetCurrent(&currentContext);
		assert(currentContext);
		di = &currentContext->dinfo;
	}
	if (addr)
		di->buffer=addr;
	/* redirect the stream */
	orig_fprintf = di->fprintf_func;
	f = di->stream;

	di->stream = (PTR) &b;
	b.p = 0;
	di->fprintf_func = (fprintf_ftype)daPrintf;

	if (n<1)
		n=10;

	found=-1;
	currSym=getNextSym(&found,&currMod,di->buffer);
	nextSym=0;

	while (n-- > 0) {
		int decoded,i,j,k,clip,spaces,bpc,bpl;

		if (currSym) {
			printSymAddr((bfd_vma)di->buffer,currMod, currSym, di);
			b.p=0;
			orig_fprintf(f,"\n%s:\n\n",b.buf);
			currSym=0;
		}

		di->buffer_vma = (bfd_vma)di->buffer;

		decoded = bfdDisassembler((bfd_vma)di->buffer, di);

		bpc = di->bytes_per_chunk;
		if (0==bpc) {
			/* many targets don't set/use this */
			bpc=1;
		}

		bpl = di->bytes_per_line;
		if (0==bpl) {
			/* some targets don't set this; take a wild guess
			 * (same as objdump)
			 */
			bpl = 4;
		}


		/* print the data in 'bytes_per_chunk' chunks
		 * which are endian-converted. Limit one line's
		 * output to  'bytes_per_line'.
		 */
		clip = decoded > bpl ? bpl : decoded;
		for (i=0; i < decoded; i+=clip) {
			/* print address */
			orig_fprintf(f,"0x%08x: ",di->buffer + i);
			for (k=0; k < clip && k+i < decoded; k+=bpc) {
				for (j=0; j<bpc; j++) {
					if (BFD_ENDIAN_BIG == di->display_endian)
						orig_fprintf(f,"%02x",di->buffer[i + k + j]);
					else
						orig_fprintf(f,"%02x",di->buffer[i + k + bpc - 1 - j]);
				}
				orig_fprintf(f," ");
			}
			if (i==0) {
				spaces = (bpl - clip);
				spaces = 2 * spaces + spaces/bpc;
				orig_fprintf(f,"  %*s%s\n",spaces,"",b.buf);
			} else {
				orig_fprintf(f,"\n");
			}
		}

		di->buffer+=decoded;

		if (!nextSym) {
			nextMod=currMod;
			nextSym=getNextSym(&found,&nextMod,di->buffer);
		}
		if (nextSym) {
			if (di->buffer >= (bfd_byte *)nextSym->value.ptv) {
				currSym=nextSym; currMod=nextMod;
				nextSym=0;
			}
		}
		b.p=0;
	}

	/* restore the stream */
	di->stream = f;
	di->fprintf_func = orig_fprintf;
	return 0;
}
