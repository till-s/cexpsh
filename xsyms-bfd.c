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

static void
usage(char *nm)
{
char *chpt=strrchr(nm,'/');
if (chpt)
	nm=chpt+1;
fprintf(stderr,"usage: %s [-p] [-z] [-h] <infile> <outfile>\n", nm);
fprintf(stderr,"       %s implementation using BFD\n",nm);
fprintf(stderr,"	   $Id$\n\n");
fprintf(stderr,"       strip an object file leaving only the symbol table\n");
fprintf(stderr,"       -h this info\n");
fprintf(stderr,"       -p ignored (compatibility)\n");
fprintf(stderr,"       -z ignored (compatibility)\n");
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

int
main(int argc, char **argv)
{
bfd							*obfd=0,*ibfd=0;
const bfd_arch_info_type	*arch;
int							i,nsyms;
asymbol						**isyms=0, **osyms=0;
int							rval=1;
char						*ifilen,*ofilen = 0;

	/* scan options */
	while ( (i=getopt(argc, argv, "hpz")) > 0 ) {
		switch (i) {
			case 'h': usage(argv[0]); exit(0);

			default:
					  fprintf(stderr,"Unknown option %c\n",i);
			case 'p':
			case 'z': break;

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

	i++;
	if (i>=argc) {
		ofilen = my_strdup_suff(ifilen,"sym");
		if (!strcmp(ofilen,ifilen)) {
			fprintf(stderr,"default suffix substitution yields identical in/out filenames\n");
			goto cleanup;
		}
	} else {
		ofilen = my_strdup_suff(argv[i],0);
	}
	if (!ofilen || !*ofilen ||
		!(obfd=bfd_openw(ofilen,0)) ||
		! bfd_set_format(obfd,bfd_object) ||
		! bfd_set_arch_mach(obfd, arch->arch, arch->mach)
	    ) {
		fprintf(stderr,"Unable to create output BFD\n");
		goto cleanup;
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

	/* Now copy the symbol table */
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

	rval = 0;
cleanup:

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
