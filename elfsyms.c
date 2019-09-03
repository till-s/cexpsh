
/* routines to extract information from an ELF symbol table */

/* The ELF symbol table is reduced and copied to a local format.
 * (a lot of ELF info / symbols are not used by us and we want
 * to have two _sorted_ tables with name and address keys.
 * We don't bother to set up a hash table because we want
 * to implement regexp search and hence we want a sorted table
 * anyway...).
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
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <cexp_regex.h>

#include <pmelf.h>

#include "cexpsymsP.h"
#include "cexpmodP.h"
#define _INSIDE_CEXP_
#include "cexpHelp.h"

#include "elfdlmap.h"

#ifdef __rtems__
#include <sys/socket.h>
#endif

/*#define  USE_ELF_MEMORY*/

typedef struct FilterArgsRec_ {
	const char *strtab;
	uintptr_t   offset;
} FilterArgsRec, *FilterArgs;

/* filter the symbol table entries we're interested in */

/* NOTE: this routine defines the CexpType which is assigned
 *       to an object read from the symbol table. Currently,
 *       this only based on object size with integers
 *       taking preference over floats.
 *       All of this knowledge / heuristics is concentrated
 *       in one place, namely HERE.
 */
static const char *
filter32(void *ext_sym, void *closure)
{
Elf32_Sym   *sp     = ext_sym;
FilterArgs  args    = closure;
const char	*strtab = args->strtab;

	if ( STB_LOCAL == ELF32_ST_BIND(sp->st_info) || SHN_UNDEF == sp->st_shndx )
		return 0;

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
assign32(void *symp, CexpSym cesp, void *closure)
{
Elf32_Sym	*sp     = symp;
int 		s       = sp->st_size;
FilterArgs  args    = closure;
CexpType	t;

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

	cesp->value.ptv  = (CexpVal)((uintptr_t)sp->st_value + args->offset);
}

static const char *
filter64(void *ext_sym, void *closure)
{
Elf64_Sym	*sp     = ext_sym;
FilterArgs  args    = closure;
const char	*strtab = args->strtab;

	if ( STB_LOCAL == ELF64_ST_BIND(sp->st_info) || SHN_UNDEF == sp->st_shndx )
		return 0;

	switch (ELF64_ST_TYPE(sp->st_info)) {
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
assign64(void *symp, CexpSym cesp, void *closure)
{
Elf64_Sym	*sp     = symp;
int 		s       = sp->st_size;
FilterArgs  args    = closure;
CexpType	t;

	cesp->size = s;

	t=TVoid;

	switch (ELF64_ST_TYPE(sp->st_info)) {
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

	switch(ELF64_ST_BIND(sp->st_info)) {
		case STB_GLOBAL: cesp->flags|=CEXP_SYMFLG_GLBL; break;
		case STB_WEAK  : cesp->flags|=CEXP_SYMFLG_WEAK; break;
		default:
			break;
	}

	cesp->value.ptv  = (CexpVal)((uintptr_t)sp->st_value + args->offset);
}

static unsigned long symcnt(void *symtab, size_t symsz, long nsyms, CexpSymFilterProc filt, void *closure)
{
unsigned long rval    = 0;

	while ( nsyms-- ) {
		if ( filt(symtab, closure) )
			rval++;
		symtab += symsz;
	}
	return rval;
}

/* read an ELF file, extract the relevant information and
 * build our internal version of the symbol table.
 * All libelf resources are released upon return from this
 * routine.
 */
static CexpSymTbl
cexpSlurpElf(const char *filename, CexpModule mod)
{
Elf_Stream	  elf=0;
Elf_Ehdr      ehdr;
Pmelf_Shtab   shtab  = 0;
Pmelf_Symtab  symtab = 0;
CexpSymTbl	  rval=0,csymt=0;
CexpSym		  sane;
#if defined(USE_ELF_MEMORY) || defined(HAVE_RCMD)
char		  *buf=0,*ptr=0;
long		  got;
#endif
#if defined(USE_ELF_MEMORY)
long		  size=0,avail=0;
#endif
unsigned long nsyms;
CexpLinkMap   lmaps = 0, map;
FilterArgsRec args;

#ifdef HAVE_RCMD
#ifdef		__rtems__
extern		struct in_addr rtems_bsdnet_bootp_server_address;
char		HOST[30];
#else
#define		HOST "localhost"
#endif
#endif

FILE        *f = 0;

	pmelf_set_errstrm(stderr);

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
		if (!(buf=rshLoad(HOST,cmd+1,ptr,&got)))
			goto cleanup;
	}
	else
#endif
	{
		f = cexpSearchFile(getenv("PATH"), filename, 0, 0);

		if ( ! f ) {
			goto cleanup;
		}

#ifdef USE_ELF_MEMORY
		if ( setvbuf(f, 0, _IONBF, 0) ) {
			fprintf(stderr,"cexpSlurpElf: unable to disable buffering: %s\n", strerror(errno));
			goto cleanup;
		}

		do {
			if (avail<LOAD_CHUNK) {
				size+=LOAD_CHUNK; avail+=LOAD_CHUNK;
				if (!(buf=realloc(buf,size)))
					goto cleanup;
				ptr=buf+(size-avail);
			}
			got=fread(ptr,1,avail,f);
			if ( ferror( f ) )
				goto cleanup;
			avail-=got;
			ptr+=got;
		} while (!feof(f) && got > 0);
			got = ptr-buf;
#endif
	}

#if defined(USE_ELF_MEMORY) || defined(HAVE_RCMD)
	if ( buf ) {
		if (!(elf = pmelf_memstrm(buf,got)))
			goto cleanup;
	} else 
	/* this deals with the case if HAVE_RCMD but not USE_ELF_MEMORY when we use a non-rsh file */
#endif
	{
		elf = pmelf_mapstrm(filename,f);
		if ( !elf ) {
			if ( ENOTSUP != errno ) {
				fprintf(stderr,"Error: unable to mmap symbol file: %s\n", strerror(errno));
				goto cleanup;
			}
			if ( ! (elf =  pmelf_newstrm(filename,f)) ) {
				fprintf(stderr,"Error: unable to open symbol file: %s\n", strerror(errno));
				goto cleanup;
			}
		}
		/* FILE is now 'owned' by pmelf */
		f = 0;
	}

	/* we need the section header string table */
	if (      pmelf_getehdr(elf, &ehdr)
	     || ! (shtab  = pmelf_getshtab(elf, &ehdr))
	     || ! (symtab = pmelf_getsymtab(elf, shtab)) )
		goto cleanup;
	
	/* convert the symbol table */
	lmaps = cexpLinkMapBuild( 0, 0 );

	args.strtab = symtab->strtab;
	args.offset = 0;

	if ( ELFCLASS64 == ehdr.e_ident[EI_CLASS] ) {
		nsyms = symcnt(
				(void*)symtab->syms.p_t64,
				sizeof(Elf64_Sym), symtab->nsyms,
				filter64,
				&args);
	} else {
		nsyms = symcnt(
				(void*)symtab->syms.p_t32,
				sizeof(Elf32_Sym), symtab->nsyms,
				filter32,
				&args);
	}

	for ( map = lmaps; map; map = map->next ) {
		nsyms += map->nsyms - map->firstsym;
	}

	csymt = cexpNewSymTbl( nsyms );

	if ( ! csymt )
		goto cleanup;

	if ( ELFCLASS64 == ehdr.e_ident[EI_CLASS] ) {

		args.strtab = symtab->strtab;
		args.offset = 0;

		csymt = cexpAddSymTbl(
				csymt,
				(void*)symtab->syms.p_t64,
				sizeof(Elf64_Sym), symtab->nsyms,
				filter64,assign64,
				&args,
				0);
		for ( map = lmaps; map; map = map->next ) {
			args.strtab = map->strtab;
			args.offset = map->offset;
			cexpAddSymTbl(
				csymt,
				(void*)map->elfsyms + sizeof(Elf64_Sym) * map->firstsym,
				sizeof(Elf64_Sym), map->nsyms - map->firstsym,
				filter64, assign64,
				&args,
				(map->flags & CEXP_LINK_MAP_STATIC_STRINGS) ? CEXP_SYMTBL_FLAG_NO_STRCPY : 0
			);
		}
	} else {

		args.strtab = symtab->strtab;
		args.offset = 0;

		csymt = cexpAddSymTbl(
				csymt,
				(void*)symtab->syms.p_t32,
				sizeof(Elf32_Sym), symtab->nsyms,
				filter32,assign32,
				&args,
				0);
		for ( map = lmaps; map; map = map->next ) {
			args.strtab = map->strtab;
			args.offset = map->offset;
			cexpAddSymTbl(
				csymt,
				(void*)map->elfsyms + sizeof(Elf32_Sym) * map->firstsym,
				sizeof(Elf32_Sym), map->nsyms - map->firstsym,
				filter32, assign32,
				&args,
				(map->flags & CEXP_LINK_MAP_STATIC_STRINGS) ? CEXP_SYMTBL_FLAG_NO_STRCPY : 0
			);
		}
	}

	cexpSortSymTbl( csymt );

	if ( cexpIndexSymTbl( csymt ) )
		goto cleanup;

#ifndef ELFSYMS_TEST_MAIN
	/* do a couple of sanity checks */
	if ((sane=cexpSymTblLookup("cexpSlurpElf",csymt))) {
		extern void *_edata, *_etext;
		/* it must be the main symbol table */
		if (    sane->value.ptv!=(CexpVal)cexpSlurpElf
		     || !(sane=cexpSymTblLookup("_etext",csymt)) || sane->value.ptv!=(CexpVal)&_etext
		     || !(sane=cexpSymTblLookup("_edata",csymt)) || sane->value.ptv!=(CexpVal)&_edata ) {
			fprintf(stderr,"ELFSYMS SANITY CHECK FAILED: you possibly loaded the wrong symbol table\n");
			goto cleanup;
		}
		/* OK, sanity test passed */
	}
#endif

	rval  = csymt;
	csymt = 0;

cleanup:
	cexpLinkMapFree(lmaps);
	pmelf_delsymtab(symtab);
	pmelf_delshtab(shtab);
	pmelf_delstrm(elf,0);
#if defined(USE_ELF_MEMORY) || defined(HAVE_RCMD)
	if (buf)
		free(buf);
#endif
	if (csymt)
		cexpFreeSymTbl(&csymt);
	if (f)
		fclose(f);
	return rval;
}

int
cexpLoadFile(const char *filename, CexpModule new_module)
{
int			rval=-1;

	if (cexpSystemModule) {
		fprintf(stderr,
				"The ELF symbol file loader doesn't support loading object files, sorry\n");
		fprintf(stderr,
				"(only initial symbol table can be loaded) - recompile with --enable-loader\n");
		return rval;
	}

	if ((new_module->symtbl=cexpSlurpElf(filename, cexpSystemModule))) {
		rval=0;
	}
#ifdef HAVE_BFD_DISASSEMBLER
#ifndef USE_PMBFD
#error "This configuration should never happen"
#endif
	{
		extern void cexpDisassemblerInstall(void*);
		cexpDisassemblerInstall(0);
	}
#endif
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

	t=cexpSlurpElf(argv[1], 0);

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
			symp->value.ptv,
			symp->size,
			symp->name);	
		symp++;
	}
	cexpFreeSymTbl(&t);
	return 0;
}

#endif
