/* $Id$ */

/* routines to extract information from an ELF symbol table */

/* The ELF symbol table is reduced and copied to a local format.
 * (a lot of ELF info / symbols are not used by us and we want
 * to have two _sorted_ tables with name and address keys.
 * We don't bother to set up a hash table because we want
 * to implement regexp search and hence we want a sorted table
 * anyway...).
 */

/* Author/Copyright: Till Straumann <Till.Straumann@TU-Berlin.de>
 * License: GPL, see http://www.gnu.org for the exact terms
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

#include <spencer_regexp.h>

#include <libelf/libelf.h>

#include "cexpsymsP.h"
#include "cexpmodP.h"
#define _INSIDE_CEXP_
#include "help.h"

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

		if (CEXP_BASE_TYPE_SIZE(TUCharP) == s) {
			t=TUChar;
		} else if (CEXP_BASE_TYPE_SIZE(TUShortP) == s) {
			t=TUShort;
		} else if (CEXP_BASE_TYPE_SIZE(TULongP) == s) {
			t=TULong;
		} else if (CEXP_BASE_TYPE_SIZE(TDoubleP) == s) {
			t=TDouble;
		} else if (CEXP_BASE_TYPE_SIZE(TFloatP) == s) {
			/* if sizeof(float) == sizeof(long), long has preference */
			t=TFloat;
		} else {
			/* if it's bigger than double, leave it (void*) */
		}
	break;

	case STT_FUNC:
		t=TFuncP;
	break;

	case STT_NOTYPE:
		t=TVoid;
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
#ifdef		__rtems
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
#ifdef __rtems
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
CexpSym			found;
spencer_regexp	*rc;
int				rval=-1,max;;

	assert(rc=spencer_regcomp(CEXP_HELP_TAB_NAME));
	if ((new_module->symtbl=cexpSlurpElf(filename))) {
		for (found=0,max=1; found=_cexpSymTblLookupRegex(rc,&max,found,0,new_module->symtbl); found++,max=1) {
			cexpAddHelpToSymTab((CexpHelpTab)found->value.ptv, new_module->symtbl);
		}
		rval=0;
	}
	free(rc);
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
