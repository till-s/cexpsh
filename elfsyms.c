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

#include <stdio.h>
#include <fcntl.h>
#include <elf.h>
#include <libelf/libelf.h>

#include "cexp.h"


/* filter the symbol table entries we're interested in */
static int
filter(Elf32_Sym *sp)
{

	if ( STT_OBJECT  == ELF32_ST_TYPE(sp->st_info))
		return VAR;
	if ( STT_FUNC    == ELF32_ST_TYPE(sp->st_info))
		return FUNC;
	return 0;
}

/* compare the name of two symbols */
static int
namecomp(const void *a, const void *b)
{
	CexpSym sa=(CexpSym)a;
	CexpSym sb=(CexpSym)b;
	return strcmp(sa->name, sb->name);
}

static int
addrcomp(const void *a, const void *b)
{
	CexpSym *sa=(CexpSym*)a;
	CexpSym *sb=(CexpSym*)b;
	return (*sa)->val.addr-(*sb)->val.addr;
}

typedef struct PrivSymTblRec_ {
	CexpSymTblRec	stab;		/* symbol table, sorted in ascending order (key=name) */
	char		*strtbl;	/* string table */
	CexpSym		*aindex;	/* an index sorted to ascending addresses */
} PrivSymTblRec, *PrivSymTbl;

CexpSym
cexpSymTblLookup(CexpSymTbl t, char *name)
{
CexpSymRec key;
	key.name = name;
	return (CexpSym)bsearch((void*)&key,
				t->syms,
				t->nentries,
				sizeof(*t->syms),
				namecomp);
}

CexpSymTbl
cexpSlurpElf(int fd)
{
Elf		*elf=0;
Elf32_Ehdr	*ehdr;
Elf_Scn		*scn;
Elf32_Shdr	*shdr;
Elf_Data	*strs;
PrivSymTbl	rval=0;

	elf_version(EV_CURRENT);
	if (!(elf=elf_begin(fd,ELF_C_READ,0)))
		goto cleanup;

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
	if ((rval=(PrivSymTbl)malloc(sizeof(*rval)))) {
		long		n,nsyms,nDstSyms,nDstChars;
		char		*strtab,*src,*dst;
		Elf32_Sym	*syms, *sp;
		CexpSym		cesp;

		memset((void*)rval, 0, sizeof(*rval));

		strtab=(char*)elf_getdata(elf_getscn(elf,shdr->sh_link),0)->d_buf;
		syms=(Elf32_Sym*)elf_getdata(scn,0)->d_buf;

		nsyms=shdr->sh_size/shdr->sh_entsize;

		/* count the number of valid symbols */
		for (sp=syms,n=0,nDstSyms=0,nDstChars=0; n<nsyms; sp++,n++)
			if (filter(sp)) {
				nDstChars+=(strlen(strtab+sp->st_name) + 1);
				nDstSyms++;
			}

		rval->stab.nentries=nDstSyms;

		/* create our copy of the symbol table - ELF contains
		 * many things we're not interested in and also, it's not
		 * sorted...
		 */
		
		/* allocate all the table space */
		if (!(rval->stab.syms=(CexpSym)malloc(sizeof(CexpSymRec)*nDstSyms)))
			goto cleanup;


		if (!(rval->strtbl=(char*)malloc(nDstChars)) ||
                    !(rval->aindex=(CexpSym*)malloc(nDstSyms*sizeof(*rval->aindex))))
			goto cleanup;

		/* now copy the relevant stuff */
		for (sp=syms,n=0,cesp=rval->stab.syms,dst=rval->strtbl; n<nsyms; sp++,n++) {
			int t;
			if ((t=filter(sp))) {
				/* copy the name to the string table and put a pointer
				 * into the symbol table.
				 */
				cesp->name=dst;
				src=strtab+sp->st_name;
				while (*(dst++)=*(src++));

				cesp->size = sp->st_size;

				/* determine the type of variable */
				if (VAR==(cesp->type=t)) {
					cesp->val.addr=(unsigned long *)sp->st_value;
					if (cesp->size<=2) {
						cesp->type=SHORT_VAR;
						if (cesp->size<=1) {
							cesp->type=CHAR_VAR;
							if (0==cesp->size)
								cesp->type=LABEL;
						}
					}
				} else {
					/* FUNC */
					cesp->val.func=(CexpFuncPtr)sp->st_value;
				}
				rval->aindex[cesp-rval->stab.syms]=cesp;
				
				cesp++;
			}
		}
		/* sort the tables */
		qsort((void*)rval->stab.syms,
			rval->stab.nentries,
			sizeof(*rval->stab.syms),
			namecomp);
		qsort((void*)rval->aindex,
			rval->stab.nentries,
			sizeof(*rval->aindex),
			addrcomp);
	} else {
		goto cleanup;
	}
	

	elf_cntl(elf,ELF_C_FDDONE);
	elf_end(elf);

	return &rval->stab;

cleanup:
	fprintf(stderr,"ELF error: %s\n",elf_errmsg(elf_errno()));
	if (elf) elf_end(elf);
	if (rval) cexpFreeSymTbl(&rval->stab);
	return 0;
}

void
cexpFreeSymTbl(CexpSymTbl arg)
{
PrivSymTbl st=(PrivSymTbl)arg;
	if (st) {
		free(st->stab.syms);
		free(st->strtbl);
		free(st->aindex);
		free(st);
	}
}

#ifdef CEXP_TEST_MAIN

int
main(int argc, char **argv)
{
int		fd,nsyms;
CexpSymTbl	t;
CexpSym		symp;

	if (argc<2) {
		fprintf(stderr,"Need a file name\n");
		return 1;
	}

	if ((fd=open(argv[1],O_RDONLY))<0) {
		return 1;
	}

	t=cexpSlurpElf(fd);
	close(fd);

	if (!t) {
		return 1;
	}

	fprintf(stderr,"%i symbols found\n",t->nentries);
	symp=t->syms;
	for (nsyms=0; nsyms<t->nentries;  nsyms++) {
		symp=((PrivSymTbl)t)->aindex[nsyms];
		fprintf(stderr,
			"%02i 0x%08xx (%2i) %s\n",
			symp->type,
			symp->val.addr,
			symp->size,
			symp->name);	
		symp++;
	}
	cexpFreeSymTbl(t);
	return 0;
}

#endif
