/* $Id$ */

/* 'xsyms' utility implementation using BFD
 *
 * Author: Till Straumann, <strauman@slac.stanford.edu>, 2003
 *
 * 'xsyms' can be used to extract the symbol table from a
 * object file.
 */

#include <bfd.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ELF_BFD_H
#include "elf-bfd.h"
#endif

#define LINKER_VERSION_SEPARATOR '@'
#define DUMMY_ALIAS_PREFIX       "__cexp__dummy_alias_"

static void
usage(char *nm)
{
char *chpt=strrchr(nm,'/');
if (chpt)
	nm=chpt+1;
fprintf(stderr,"usage: %s [-p] [-z] [-h] [-a] <infile> <outfile>\n", nm);
fprintf(stderr,"       %s implementation using BFD\n",nm);
fprintf(stderr,"	   $Id$\n\n");
fprintf(stderr,"       strip an object file leaving only the symbol table\n");
fprintf(stderr,"       -h this info\n");
fprintf(stderr,"       -p ignored (compatibility)\n");
fprintf(stderr,"       -z ignored (compatibility)\n");
fprintf(stderr,"       -C generate C-source file for building symtab into executable\n");
fprintf(stderr,"       -a just print target machine description\n");
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
symfilter(bfd *abfd, asymbol *s)
{
asection *sect = bfd_get_section(s);
	if ( BSF_LOCAL & s->flags ) {
		/* keep local symbols only if they are section symbols of allocated sections */
		if ((BSF_SECTION_SYM & s->flags) && (SEC_ALLOC & bfd_get_section_flags(abfd,sect))) {
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

	/* scan options */
	while ( (i=getopt(argc, argv, "ahpzC")) > 0 ) {
		switch (i) {
			case 'h': usage(argv[0]); exit(0);

			default:
					  fprintf(stderr,"Unknown option %c\n",i);
			case 'p':
			case 'z':               break;

			case 'C': gensrc   = 1; break;
			case 'a': dumparch = 1; break;
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
	arch=bfd_get_arch_info(ibfd);

	if (!arch) {
		fprintf(stderr,"Unable to determine architecture\n");
		goto cleanup;
	}

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
		    !(obfd=bfd_openw(ofilen,0)) ||
	    	! bfd_set_format(obfd,bfd_object) ||
		    ! bfd_set_arch_mach(obfd, arch->arch, arch->mach)
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

	/* Now copy/generate the symbol table */
	if (gensrc) {
		char *stripped;

		fprintf(ofeil,"#include <cexpsyms.h>\n");
		for ( i=0; i<nsyms; i++ ) {
			if ( !symfilter(ibfd, isyms[i]) )
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

			if ( !symfilter(ibfd, isyms[i]) )
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
	} else {
	for (i=0; i<nsyms; i++) {
		osyms[i]          = bfd_make_empty_symbol(obfd);
		/* leave undefined symbols in the undefined section;
		 */
		if (bfd_is_und_section(isyms[i]->section)) {
			osyms[i]->section = bfd_und_section_ptr;
		} else {
			osyms[i]->section = bfd_abs_section_ptr;
		}
		osyms[i]->value   = bfd_asymbol_value(isyms[i]) -
							bfd_get_section_vma(obfd,osyms[i]->section);
		osyms[i]->flags   = isyms[i]->flags;
		osyms[i]->name    = isyms[i]->name;
		bfd_copy_private_symbol_data(ibfd,isyms[i],obfd,osyms[i]);
	}

	bfd_set_symtab(obfd,osyms,nsyms);
	}

	rval = 0;
cleanup:
	if (ofeil) {
		if (rval)
			unlink(ofilen);
		fclose(ofeil);
		ofeil = 0;
	}

	if (obfd) {
		if (rval) {
			unlink(ofilen);
			bfd_close_all_done(obfd);
		} else {
			bfd_close(obfd);
		}
	}

	free(isyms);
	free(osyms);
	free(ofilen);

	if (ibfd)
		bfd_close_all_done(ibfd);

	return rval;
}
