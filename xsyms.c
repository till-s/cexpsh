/* $Id$ */

/* 'xsyms' utility implementation using BFD or pmbfd/pmelf
 *
 * 'xsyms' can be used to extract the symbol table from a
 * object file.
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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_PMBFD
#include <pmelf.h>
#include <pmbfd.h>
#undef HAVE_ELF_BFD_H
#else
#include <bfd.h>
#endif

#ifdef HAVE_ELF_BFD_H
#include "elf-bfd.h"
#endif

#define LINKER_VERSION_SEPARATOR '@'
#define DUMMY_ALIAS_PREFIX       "__cexp__dummy_alias_"

#ifdef _PMBFD_

static int cpyscn(Elf_Stream elfi, Elf_Stream elfo, Pmelf_Shtab shtab, Elf_Shdr *shdr)
{
void *dat = 0;
int  rval = -1;
Pmelf_Size sz;
	if ( !(dat=pmelf_getscn(elfi,shdr,0,0,0)) ) {
		fprintf(stderr,"Unable to read %s section\n", pmelf_sec_name(shtab, shdr));
		return -1;
	}
	sz = ELFCLASS64 == shtab->clss ? shdr->s64.sh_size : shdr->s32.sh_size;
	if ( pmelf_write(elfo, dat, sz ) ) {
		fprintf(stderr,"Unable to write %s section\n", pmelf_sec_name(shtab, shdr));
		goto cleanup;
	}
	rval = 0;

cleanup:
	free(dat);
	return rval;
}

static int
pmelf_copy_symtab(char *ifilen, char *ofilen)
{
FILE       *of;
Elf_Stream elfi=0, elfo=0;
Elf_Ehdr   ehdr;
union {
	Elf32_Shdr e32[4];
	Elf64_Shdr e64[4];
} shdrs;   /* NULL .shstrtab .symtab .strtab */
Elf_Shdr   *symsh, *strsh;
Pmelf_Shtab shtab = 0;
Pmelf_Symtab symtab = 0;
int        rval = -1, i;
char       buf[BUFSIZ];
int        elf64 = 0;

	/* Do it the ELF way... */
	memset(&shdrs,0,sizeof(shdrs));

	pmelf_set_errstrm(stderr);

	elfi = pmelf_newstrm(ifilen, 0);
	if ( pmelf_getehdr(elfi, &ehdr) ) {
		goto cleanup;
	}

	if ( !(shtab = pmelf_getshtab(elfi, &ehdr)) )
		goto cleanup;

	if ( !(symtab = pmelf_getsymtab(elfi, shtab)) )
		goto cleanup;

	if ( pmelf_find_symhdrs(elfi, shtab, &symsh, &strsh) < 0 ) {
		goto cleanup;
	}

	if ( ! (of = fopen(ofilen,"w")) ) {
		fprintf(stderr,"pmelf -- Opening %s for writing: %s\n",ofilen, strerror(errno));
		goto cleanup;
	}
	if ( ! (elfo = pmelf_newstrm(0, of)) ) {
		fclose(of);
		goto cleanup;
	}

	if ( (elf64 = (ELFCLASS64 == ehdr.e_ident[EI_CLASS])) ) {
		shdrs.e64[0]           = shtab->shdrs.p_s64[0];

		shdrs.e64[1].sh_name   = 1; /* .shstrtab */
		shdrs.e64[1].sh_type   = SHT_STRTAB;
		shdrs.e64[1].sh_offset = sizeof(ehdr.e64) + sizeof(shdrs.e64);
		shdrs.e64[1].sh_size   = 27;

		shdrs.e64[2]           = symsh->s64;
		shdrs.e64[2].sh_name   = 11;
		shdrs.e64[2].sh_offset = shdrs.e64[1].sh_offset + shdrs.e64[1].sh_size;
		shdrs.e64[2].sh_link   = 3;

		shdrs.e64[3]           = strsh->s64;
		shdrs.e64[3].sh_name   = 19;
		shdrs.e64[3].sh_offset = shdrs.e64[2].sh_offset + shdrs.e64[2].sh_size;

		ehdr.e64.e_entry = 0;
		ehdr.e64.e_phoff = 0;
		ehdr.e64.e_shoff = sizeof(ehdr.e64);
		ehdr.e64.e_phnum = 0;
		ehdr.e64.e_shnum = sizeof(shdrs.e64)/sizeof(shdrs.e64[0]);
		ehdr.e64.e_shstrndx = 1;
		if ( pmelf_putehdr64(elfo, &ehdr.e64) ) {
			goto cleanup;
		}
		for ( i = 0; i<ehdr.e64.e_shnum; i++ ) {
			if ( pmelf_putshdr64(elfo, shdrs.e64+i) ) {
				goto cleanup;
			}
		}
	} else {
		shdrs.e32[0]           = shtab->shdrs.p_s32[0];

		shdrs.e32[1].sh_name   = 1; /* .shstrtab */
		shdrs.e32[1].sh_type   = SHT_STRTAB;
		shdrs.e32[1].sh_offset = sizeof(ehdr.e32) + sizeof(shdrs.e32);
		shdrs.e32[1].sh_size   = 27;

		shdrs.e32[2]           = symsh->s32;
		shdrs.e32[2].sh_name   = 11;
		shdrs.e32[2].sh_offset = shdrs.e32[1].sh_offset + shdrs.e32[1].sh_size;
		shdrs.e32[2].sh_link   = 3;

		shdrs.e32[3]           = strsh->s32;
		shdrs.e32[3].sh_name   = 19;
		shdrs.e32[3].sh_offset = shdrs.e32[2].sh_offset + shdrs.e32[2].sh_size;

		ehdr.e32.e_entry = 0;
		ehdr.e32.e_phoff = 0;
		ehdr.e32.e_shoff = sizeof(ehdr.e32);
		ehdr.e32.e_phnum = 0;
		ehdr.e32.e_shnum = sizeof(shdrs.e32)/sizeof(shdrs.e32[0]);
		ehdr.e32.e_shstrndx = 1;
		if ( pmelf_putehdr32(elfo, &ehdr.e32) ) {
			goto cleanup;
		}
		for ( i = 0; i<ehdr.e32.e_shnum; i++ ) {
			if ( pmelf_putshdr32(elfo, shdrs.e32+i) ) {
				goto cleanup;
			}
		}
	}

	if (   pmelf_write(elfo,"",1)
			|| pmelf_write(elfo,".shstrtab",10)
			|| pmelf_write(elfo,".symtab",8)
			|| pmelf_write(elfo,".strtab",8) )
	{
		fprintf(stderr,"pmelf -- unable to write .shstrtab: %s\n", strerror(errno));
		goto cleanup;
	}

	if ( elf64 ) {
		Elf64_Sym *sym;
		for ( i=0, sym=symtab->syms.p_t64; i<symtab->nsyms; i++, sym++ ) {
			if ( sym->st_shndx != SHN_UNDEF && sym->st_shndx < shtab->nshdrs ) {
				/*
				   sym->st_value += shtab->shdrs[sym->st_shndx].sh_addr;
				   */
				sym->st_shndx = SHN_ABS;
			}
		}
	} else {
		Elf32_Sym *sym;
		for ( i=0, sym=symtab->syms.p_t32; i<symtab->nsyms; i++, sym++ ) {
			if ( sym->st_shndx != SHN_UNDEF && sym->st_shndx < shtab->nshdrs ) {
				/*
				   sym->st_value += shtab->shdrs[sym->st_shndx].sh_addr;
				   */
				sym->st_shndx = SHN_ABS;
			}
		}
	}

	if ( elf64 ) {
		for ( i=0; i<symtab->nsyms; i++ ) {
			if ( pmelf_putsym64(elfo, &symtab->syms.p_t64[i]) ) {
				fprintf(stderr,"pmelf -- unable to write .symtab\n");
			goto cleanup;
			}
		}
	} else {
		for ( i=0; i<symtab->nsyms; i++ ) {
			if ( pmelf_putsym32(elfo, &symtab->syms.p_t32[i]) ) {
				fprintf(stderr,"pmelf -- unable to write .symtab\n");
			goto cleanup;
			}
		}
	}

	if ( cpyscn(elfi, elfo, shtab, strsh) ) 
		goto cleanup;

	ofilen = 0;
	rval   = 0;

cleanup:
	pmelf_delstrm(elfi,0);
	if ( elfo && ofilen )
		unlink(ofilen);
	pmelf_delstrm(elfo,0);
	pmelf_delshtab(shtab);
	pmelf_delsymtab(symtab);
	return rval;
}
#endif


static void
usage(char *nm)
{
char *chpt=strrchr(nm,'/');
if (chpt)
	nm=chpt+1;
fprintf(stderr,"usage: %s [-p] [-z] [-h] [-a] <infile> <outfile>\n", nm);
#ifdef _PMBFD_
fprintf(stderr,"       %s implementation using PMBFD\n",nm);
#else
fprintf(stderr,"       %s implementation using BFD\n",nm);
#endif
fprintf(stderr,"	   $Id$\n\n");
fprintf(stderr,"       strip an object file leaving only the symbol table\n");
fprintf(stderr,"       -h this info\n");
fprintf(stderr,"       -a just print target machine description\n");
fprintf(stderr,"       -p ignored (compatibility)\n");
fprintf(stderr,"       -z ignored (compatibility)\n");
fprintf(stderr,"       -C generate C-source file for building symtab into executable\n");
fprintf(stderr,"       -s [together with -C]: add section symbols to the symtab and\n");
fprintf(stderr,"          print linker commands on stdout. You want to save these in\n");
fprintf(stderr,"          a file and add them as a linker script to the final link of\n");
fprintf(stderr,"          your application.\n");
fprintf(stderr,"       WARNING: use other than with -C[s] is deprecated; use\n");
fprintf(stderr,"                objcopy --extract symbol instead! (Requires\n");
fprintf(stderr,"                binutils >= 2.18).\n");
}

/* duplicate a string and replace/append a suffix */
static char *
my_strdup_suff(char *str, char *suff)
{
int  l = strlen(str);
char *rval,*chpt;

	if (suff)
		l+=strlen(suff)+1; /* if we have to insert a '.' */
	if ( (rval=malloc(l+1)) ) {
		strcpy(rval,str);
		if ( suff ) {
			if ( (chpt=strrchr(rval,'.')) )
				chpt++;
			else {
				chpt = rval + strlen(rval);
				*chpt++='.';
			}
			strcpy(chpt,suff);
		}
	}
	return rval;
}

/* replicate cexp/bfdstuff.c behavior */
static int
symfilter(bfd *abfd, asymbol *s, int accept_sect_syms)
{
asection *sect = bfd_get_section(s);
	if ( BSF_LOCAL & s->flags ) {
		/* keep local symbols only if they are section symbols of allocated sections */
		if ( accept_sect_syms && (BSF_SECTION_SYM & s->flags) && (SEC_ALLOC & bfd_get_section_flags(abfd,sect)) ) {
			return 1;
		} else {
			return 0;
		}
	}
	/* global symbol */
	if ( bfd_is_und_section(sect) ) {
		/* do some magic on undefined symbols which do have a
		 * type. These are probably sitting in a shared
		 * library and _do_ have a valid value. (This is the
		 * case when loading the symbol table e.g. on a
		 * linux platform.)
		 */
		return bfd_asymbol_value(s) && (BSF_FUNCTION & s->flags);
	}
	return 1;
}

static const char *getsname(bfd *abfd, asymbol *ps, char **pstripped)
{
char *chpt;
const char *sname = ( BSF_SECTION_SYM & ps->flags ) ?
						bfd_get_section_name(abfd, bfd_get_section(ps)) :
						bfd_asymbol_name(ps);

	*pstripped = strdup(sname);
#ifdef LINKER_VERSION_SEPARATOR
	if ( chpt = strchr(*pstripped, LINKER_VERSION_SEPARATOR) ) {
		*chpt = 0;
	}
#endif
	return sname;
}

int
main(int argc, char **argv)
{
bfd							*obfd=0,*ibfd=0;
FILE						*ofeil=0;
const bfd_arch_info_type	*arch;
int							i,nsyms;
asymbol						**isyms=0, **osyms=0;
int							rval=1;
char						*ifilen,*ofilen = 0;
int							gensrc=0;
int							dumparch=0;
int							sectsyms=0;

	/* scan options */
	while ( (i=getopt(argc, argv, "ahpszC")) > 0 ) {
		switch (i) {
			case 'h': usage(argv[0]); exit(0);

			default:
					  fprintf(stderr,"Unknown option %c\n",i);
			case 'p':
			case 'z':               break;

			case 'C': gensrc   = 1; break;
			case 'a': dumparch = 1; break;
			case 's': sectsyms = 1; break;
		}
	}

	bfd_init();

	i = optind;

	if ( i>=argc ) {
		usage(argv[0]);
		goto cleanup;
	}
	if (!(ibfd=bfd_openr(ifilen=argv[i],0)) ||
		! bfd_check_format(ibfd,bfd_object)) {
		fprintf(stderr,"Unable to open input file\n");
		goto cleanup;
	}

#ifndef _PMBFD_
	arch=bfd_get_arch_info(ibfd);

	if (!arch) {
		fprintf(stderr,"Unable to determine architecture\n");
		goto cleanup;
	}
#endif

	if (dumparch) {
		printf("%s\n",bfd_printable_name(ibfd));
		rval = 0;
		goto cleanup;
	}

	i++;
	if (i>=argc) {
		ofilen = my_strdup_suff(ifilen,gensrc ? "c" : "sym");
		if (!strcmp(ofilen,ifilen)) {
			fprintf(stderr,"default suffix substitution yields identical in/out filenames\n");
			goto cleanup;
		}
	} else {
		ofilen = my_strdup_suff(argv[i],0);
	}
	if ( gensrc ) {
		if ( !ofilen || !*ofilen )
			ofeil = stdout;
		else
			if ( ! (ofeil=fopen(ofilen,"w")) ) {
				perror("Opening output file");
				goto cleanup;
			}
	} else {
		if (!ofilen || !*ofilen ||
#ifdef _PMBFD_
			0
#else
		    !(obfd=bfd_openw(ofilen,0)) ||
	    	! bfd_set_format(obfd,bfd_object) ||
		    ! bfd_set_arch_mach(obfd, arch->arch, arch->mach)
#endif
	        ) {
		fprintf(stderr,"Unable to create output BFD\n");
		goto cleanup;
		}
	}

	/* sanity check */
	if (!(HAS_SYMS & bfd_get_file_flags(ibfd))) {
		fprintf(stderr,"No symbols found\n");
		goto cleanup;
	}

	if ((i=bfd_get_symtab_upper_bound(ibfd))<0) {
		fprintf(stderr,"Fatal error: illegal symtab size\n");
		goto cleanup;
	}

#ifdef _PMBFD_
	if ( gensrc ) {
#endif
	/* Allocate space for the symbol table  */
	if (i) {
		isyms=(asymbol**)xmalloc((i));
		osyms=(asymbol**)xmalloc((i));
	}
	nsyms= i ? i/sizeof(asymbol*) - 1 : 0;

	if (bfd_canonicalize_symtab(ibfd,isyms) <= 0) {
		bfd_perror("Canonicalizing symtab");
		goto cleanup;
	}
#ifdef _PMBFD_
	}
#endif

	/* Now copy/generate the symbol table */
	if (gensrc) {
		char *stripped;

		fprintf(ofeil,"/* THIS FILE WAS AUTOMATICALLY GENERATED BY xsyms -- DO NOT EDIT */\n");
		fprintf(ofeil,"#include <cexpsyms.h>\n");
		if ( sectsyms )
			printf("/* THIS FILE WAS AUTOMATICALLY GENERATED BY xsyms -- DO NOT EDIT */\n");
		for ( i=0; i<nsyms; i++ ) {
			if ( !symfilter(ibfd, isyms[i], sectsyms) )
				continue;
			getsname(ibfd, isyms[i], &stripped);


			fprintf(ofeil,"extern int "DUMMY_ALIAS_PREFIX"%i;\n",i);
			if ( isyms[i]->flags & BSF_SECTION_SYM )
				printf("%s%i = ADDR( %s ) ;\n", DUMMY_ALIAS_PREFIX, i, stripped);
			else
				fprintf(ofeil,"asm(\".set "DUMMY_ALIAS_PREFIX"%i,%s\\n\");\n",i,stripped);
			free(stripped);
		}
		fprintf(ofeil,"\n\nstatic CexpSymRec systemSymbols[] = {\n");
		for ( i=0; i<nsyms; i++ ) {
			const char *sname, *t = "TVoid";
			char sbuf[100];
			int sz = 0;
			unsigned long f = isyms[i]->flags;

			if ( !symfilter(ibfd, isyms[i], sectsyms) )
				continue;

			sname = getsname(ibfd, isyms[i], &stripped);

			sz = 0;
#ifdef HAVE_ELF_BFD_H
			{
			elf_symbol_type *elfsp = elf_symbol_from(ibfd, isyms[i]);
			if ( elfsp )
				sz = (elfsp)->internal_elf_sym.st_size;
			}
#endif
#ifdef _PMBFD_
			sz = elf_get_size(ibfd, isyms[i]);
#endif
			if ( !(BSF_SECTION_SYM & f) && bfd_is_com_section(bfd_get_section(isyms[i])) )
				sz = bfd_asymbol_value(isyms[i]); /* value holds size */

			sprintf(sbuf,"%i",sz);

			if ( BSF_FUNCTION & f ) {
				t = "TFuncP";
				if ( 0 == sz )
					sprintf(sbuf,"sizeof(void(*)())");
			}

			fprintf(ofeil,"\t{\n");
			fprintf(ofeil,"\t\t.name       =\"%s\",\n",sname);
			fprintf(ofeil,"\t\t.value.ptv  =(void*)&"DUMMY_ALIAS_PREFIX"%i,\n",i);
			fprintf(ofeil,"\t\t.value.type =%s,\n",    t);
			fprintf(ofeil,"\t\t.size       =%s,\n",    sbuf);
			fprintf(ofeil,"\t\t.flags      =0");
				if ( BSF_GLOBAL & f )
					fprintf(ofeil,"|CEXP_SYMFLG_GLBL");
				if ( (BSF_WEAK  & f) &&
				     /* weak in CEXP gets overridden by this table */
				     strcmp("cexpSystemSymbols",sname) )
					fprintf(ofeil,"|CEXP_SYMFLG_WEAK");
				if ( BSF_SECTION_SYM & f ) fprintf(ofeil,"|CEXP_SYMFLG_SECT");
			fprintf(ofeil,",\n");
			fprintf(ofeil,"\t},\n");
			free(stripped);
		}
		fprintf(ofeil,"\t{\n");
		fprintf(ofeil,"\t0, /* terminating record */\n");
		fprintf(ofeil,"\t},\n");
		fprintf(ofeil,"};\n");
		fprintf(ofeil,"CexpSym cexpSystemSymbols = systemSymbols;\n");
	} else
	{
	fprintf(stderr,"WARNING: the use of %s is deprecated; use 'objcopy --extract-symbol' instead\n",
	       argv[0]);
	fprintf(stderr,"         (requires binutils >= 2.18); use this utility only to generate a\n");
	fprintf(stderr,"         built-in symbol table via the '-C[s]' options.\n");
#ifndef _PMBFD_
	for (i=0; i<nsyms; i++) {
		osyms[i]          = bfd_make_empty_symbol(obfd);
		/* leave undefined symbols in the undefined section;
		 */
		if (bfd_is_und_section(bfd_get_section(isyms[i]))) {
			bfd_set_section(osyms[i], bfd_und_section_ptr);
		} else {
			bfd_set_section(osyms[i], bfd_abs_section_ptr);
		}
		osyms[i]->value   = bfd_asymbol_value(isyms[i]) -
							bfd_get_section_vma(obfd,bfd_get_section(osyms[i]));
		osyms[i]->flags   = isyms[i]->flags;
		osyms[i]->name    = isyms[i]->name;
		bfd_copy_private_symbol_data(ibfd,isyms[i],obfd,osyms[i]);
	}

	bfd_set_symtab(obfd,osyms,nsyms);
#else
	{
		if ( ibfd )
			bfd_close_all_done(ibfd);
		ibfd = 0;
		if ( pmelf_copy_symtab(ifilen, ofilen) )
			goto cleanup;
	}
#endif
	}

	rval = 0;
cleanup:
	if (ofeil) {
		if (rval)
			unlink(ofilen);
		fclose(ofeil);
		ofeil = 0;
	}

#ifndef _PMBFD_
	if (obfd) {
		if (rval) {
			unlink(ofilen);
			bfd_close_all_done(obfd);
		} else {
			bfd_close(obfd);
		}
	}
#endif

	free(isyms);
	free(osyms);
	free(ofilen);

	if (ibfd)
		bfd_close_all_done(ibfd);

	return rval;
}
