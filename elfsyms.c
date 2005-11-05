/* $Id$ */

/* routines to extract information from an ELF symbol table */

/* The ELF symbol table is reduced and copied to a local format.
 * (a lot of ELF info / symbols are not used by us and we want
 * to have two _sorted_ tables with name and address keys.
 * We don't bother to set up a hash table because we want
 * to implement regexp search and hence we want a sorted table
 * anyway...).
 */

/* Author/Copyright: Till Straumann <Till.Straumann@TU-Berlin.de> */

/*
 * Copyright 2002, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cexp_regex.h>

#include <libelf/libelf.h>

#include "cexpsymsP.h"
#include "cexpmodP.h"
#define _INSIDE_CEXP_
#include "cexpHelp.h"

/* filter the symbol table entries we're interested in */

/* NOTE: this routine defines the CexpType which is assigned
 *       to an object read from the symbol table. Currently,
 *       this only based on object size with integers
 *       taking preference over floats.
 *       All of this knowledge / heuristics is concentrated
 *       in one place, namely HERE.
 */
static const char *
filter(void *ext_sym, void *closure)
{
Elf32_Sym	*sp=ext_sym;
char		*strtab=closure;

	switch (ELF32_ST_TYPE(sp->st_info)) {
	case STT_OBJECT:
	case STT_FUNC:
	case STT_NOTYPE:
	return strtab + sp->st_name;

	default:
	break;
	}

	return 0;

}

static void
assign(void *symp, CexpSym cesp, void *closure)
{
Elf32_Sym	*sp=symp;
CexpType	t;
int 		s=sp->st_size;

	cesp->size = s;

	t=TVoid;

	switch (ELF32_ST_TYPE(sp->st_info)) {
	case STT_OBJECT:
		/* determine the type of variable */
		t = cexpTypeGuessFromSize(s);
	break;

	case STT_FUNC:
		t=TFuncP;
	break;

	case STT_NOTYPE:
		t=TVoid;
	break;

	case STT_SECTION:
		t=TVoid;
		cesp->flags|=CEXP_SYMFLG_SECT;
	break;

	default:
	break;

	}

	cesp->value.type = t;

	switch(ELF32_ST_BIND(sp->st_info)) {
		case STB_GLOBAL: cesp->flags|=CEXP_SYMFLG_GLBL; break;
		case STB_WEAK  : cesp->flags|=CEXP_SYMFLG_WEAK; break;
		default:
			break;
	}

	cesp->value.ptv  = (CexpVal)sp->st_value;
}


#define USE_ELF_MEMORY


/* read an ELF file, extract the relevant information and
 * build our internal version of the symbol table.
 * All libelf resources are released upon return from this
 * routine.
 */
static CexpSymTbl
cexpSlurpElf(char *filename)
{
Elf			*elf=0;
Elf32_Ehdr	*ehdr;
Elf_Scn		*scn;
Elf32_Shdr	*shdr=0;
Elf_Data	*strs;
CexpSymTbl	rval=0;
CexpSym		sane;
#ifdef USE_ELF_MEMORY
char		*buf=0,*ptr=0;
long		size=0,avail=0,got;
#ifdef		__rtems__
extern		struct in_addr rtems_bsdnet_bootp_server_address;
char		HOST[30];
#else
#define		HOST "localhost"
#endif
#endif
int			fd=-1;

	elf_version(EV_CURRENT);

#ifdef USE_ELF_MEMORY
#ifdef HAVE_RCMD
	if ('~'==filename[0]) {
		char *cmd=malloc(strlen(filename)+40);
		strcpy(cmd,filename);
		ptr=strchr(cmd,'/');
		if (!ptr) {
			fprintf(stderr,"Illegal filename for rshLoad %s\n",filename);
			free(cmd);
			goto cleanup;
		}
		*(ptr++)=0;
		memmove(ptr+4,ptr,strlen(ptr));
		memcpy(ptr,"cat ",4);
#ifdef __rtems__
		inet_ntop(AF_INET, &rtems_bsdnet_bootp_server_address, HOST, sizeof(HOST));
#endif
		/* try to load via rsh */
		if (!(buf=rshLoad(HOST,cmd+1,ptr)))
			goto cleanup;
	}
	else
#endif
	{
		if ((fd=open(filename,O_RDONLY,0))<0)
			goto cleanup;

		do {
			if (avail<LOAD_CHUNK) {
				size+=LOAD_CHUNK; avail+=LOAD_CHUNK;
				if (!(buf=realloc(buf,size)))
					goto cleanup;
				ptr=buf+(size-avail);
			}
			got=read(fd,ptr,avail);
			if (got<0)
				goto cleanup;
			avail-=got;
			ptr+=got;
		} while (got);
			got = ptr-buf;
	}
	if (!(elf=elf_memory(buf,got)))
		goto cleanup;
#else
	if ((fd=open(filename,O_RDONLY,0))<0)
		goto cleanup;

	if (!(elf=elf_begin(fd,ELF_C_READ,0)))
		goto cleanup;
#endif

	/* we need the section header string table */
	if (!(ehdr=elf32_getehdr(elf)) ||
	    !(scn =elf_getscn(elf,ehdr->e_shstrndx)) ||
	    !(strs=elf_getdata(scn,0)))
		goto cleanup;
	
	for (scn=0; (scn=elf_nextscn(elf,scn)) && (shdr=elf32_getshdr(scn));) {
		if (!(strcmp((char*)strs->d_buf + shdr->sh_name,".symtab")))
			break;
	}
	if (!shdr)
		goto cleanup;

	/* get the string table */
	{
		long		nsyms;
		Elf32_Sym	*syms;
		CexpSym		cesp;
		char		*strtab;

		strtab=(char*)elf_getdata(elf_getscn(elf,shdr->sh_link),0)->d_buf;
		syms=(Elf32_Sym*)elf_getdata(scn,0)->d_buf;

		nsyms=shdr->sh_size/shdr->sh_entsize;

		if (!(rval=cexpCreateSymTbl((void*)syms,sizeof(*syms),nsyms,filter,assign,(void*)strtab)))
			goto cleanup;

	}


	elf_cntl(elf,ELF_C_FDDONE);
	elf_end(elf); elf=0;
#ifdef USE_ELF_MEMORY
	if (buf) {
		free(buf); buf=0;
	}
#endif
	if (fd>=0) close(fd);

#ifndef ELFSYMS_TEST_MAIN
	/* do a couple of sanity checks */
	if ((sane=cexpSymTblLookup("cexpSlurpElf",rval))) {
		extern void *_edata, *_etext;
		/* it must be the main symbol table */
		if (sane->value.ptv!=(CexpVal)cexpSlurpElf)
			goto bailout;
		if (!(sane=cexpSymTblLookup("_etext",rval)) || sane->value.ptv!=(CexpVal)&_etext)
			goto bailout;
		if (!(sane=cexpSymTblLookup("_edata",rval)) || sane->value.ptv!=(CexpVal)&_edata)
			goto bailout;
		/* OK, sanity test passed */
	}
#endif


	return rval;

bailout:
	fprintf(stderr,"ELFSYMS SANITY CHECK FAILED: you possibly loaded the wrong symbol table\n");

cleanup:
	if (elf_errno())
		fprintf(stderr,"ELF error: %s\n",elf_errmsg(elf_errno()));
	if (elf)
		elf_end(elf);
#ifdef USE_ELF_MEMORY
	if (buf)
		free(buf);
#endif
	if (rval)
		cexpFreeSymTbl((CexpSymTbl*)&rval);
	if (fd>=0)
		close(fd);
	if (elf_errno()) fprintf(stderr,"ELF error: %s\n",elf_errmsg(elf_errno()));
	return 0;
}

int
cexpLoadFile(char *filename, CexpModule new_module)
{
int			rval=-1;

	if (cexpSystemModule) {
		fprintf(stderr,
				"The ELF file loader doesn't support loading object files, sorry\n");
		fprintf(stderr,
				"(only initial symbol table can be loaded) - recompile with BFD support\n");
		return rval;
	}

	if ((new_module->symtbl=cexpSlurpElf(filename))) {
		rval=0;
	}
	return rval;
}

#ifdef ELFSYMS_TEST_MAIN
/* only build this 'main' if we are testing the ELF subsystem */

int
elfsyms_main(int argc, char **argv)
{
int			fd,nsyms;
CexpSymTbl	t;
CexpSym		symp;

	if (argc<2) {
		fprintf(stderr,"Need a file name\n");
		return 1;
	}

	t=cexpSlurpElf(argv[1]);

	if (!t) {
		return 1;
	}

	fprintf(stderr,"%i symbols found\n",t->nentries);
	symp=t->syms;
	for (nsyms=0; nsyms<t->nentries;  nsyms++) {
		symp=t->aindex[nsyms];
		fprintf(stderr,
			"%02i 0x%08xx (%2i) %s\n",
			symp->value.type,
			symp->value.tv.p,
			symp->size,
			symp->name);	
		symp++;
	}
	cexpFreeSymTbl(&t);
	return 0;
}

#endif
