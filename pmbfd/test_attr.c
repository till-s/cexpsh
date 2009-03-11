#include <pmelf.h>
#include <stdio.h>
#include <unistd.h>

static Pmelf_attribute_set *
read_atts(char *filenm, int force_vendor_missing)
{
Elf_Stream             s;
Pmelf_Shtab            shtab = 0;
Elf_Ehdr               eh;
Pmelf_attribute_set    *pa    = 0;
Pmelf_attribute_set    *rval  = 0;
Pmelf_attribute_vendor *pv;
int                    i;
unsigned               m,abi;

	if ( ! (s = pmelf_newstrm(filenm, 0)) ) {
		fprintf(stderr,"Unable to create ELF stream\n");
		return 0;
	}

	if ( pmelf_getehdr(s, &eh) ) {
		fprintf(stderr,"Unable to read ELF header\n");
		goto cleanup;
	}

	/* Hack: layout of elf32 and elf64 matches... */
	m   = eh.e32.e_machine;
	abi = eh.e_ident[EI_OSABI]; 

	if ( ! force_vendor_missing ) {
		if ( ! (pv = pmelf_attributes_vendor_find_gnu(m,abi)) ) {
			fprintf(stderr,"No 'vendor' for attribute parsing/matching found (machine %u, abi %u)\n",m,abi);
		} else {
			if ( pmelf_attributes_vendor_register(pv) ) {
				fprintf(stderr,"Registration of vendor %s failed (already registered ?)\n", pmelf_attributes_vendor_name(pv));
			}
		}
	}

	if ( ! (shtab = pmelf_getshtab(s, &eh)) ) {
		fprintf(stderr,"Unable to read SH table\n");
		goto cleanup;
	}

	for ( i = 0; i<shtab->nshdrs; i++ ) {
		Elf_Shdr *shdr;
		/* hack: accessing sh_type as ELF32 should always work
		 *       because layout of the first fields of ELF64 matches
		 *       ELF32...
		 */
		shdr =  (shtab->clss == ELFCLASS64 ? (Elf_Shdr*)&shtab->shdrs.p_s64[i] : (Elf_Shdr*)&shtab->shdrs.p_s32[i]);
		if ( SHT_GNU_ATTRIBUTES == shdr->s32.sh_type ) {
			if ( ! (pa = pmelf_create_attribute_set(s, shdr)) ) {
				fprintf(stderr,"Creating attribute set failed -- unable to parse '.gnu.attributes' section\n");
				goto cleanup;
			}
			break;
		}
	}
	if ( !pa ) {
		fprintf(stderr,"No attributes found\n");
		goto cleanup;
	}

	rval = pa;
	pa   = 0;

cleanup:

	if ( shtab ) 
		pmelf_delshtab(shtab);
	if ( pa )
		pmelf_destroy_attribute_set(pa);
	pmelf_delstrm(s, 0);
	return rval;
}

int
main(int argc, char **argv)
{
Pmelf_attribute_set *pa    = 0;
Pmelf_attribute_set *pb    = 0;
int                  rval  = 1;
int                  opt;
int                  novendor = 0;

	while ( -1 != (opt = getopt(argc, argv, "m")) ) {
		switch ( opt ) {
			case 'm':
				novendor = 1;
			break;
			default:
			break;
		}
	}

	pmelf_set_errstrm(stderr);

	if ( argc - optind < 1 ) {
		fprintf(stderr,"Need object file arg\n");
		return 1;
	}

	if ( ( pa = read_atts(argv[optind], novendor) ) ) {
		pmelf_print_attribute_set(pa, stdout);
	}

	if ( argc - optind > 1 && ( pb = read_atts(argv[optind+1], novendor) ) ) {
		pmelf_print_attribute_set(pb, stdout);
		if ( pmelf_match_attribute_set(pa,pb) ) {
			printf("Attribute mismatch\n");
		} else {
			printf("Attribute match\n");
		}
	}


	rval = 0;

 /*
cleanup:
 */

	if ( pa )
		pmelf_destroy_attribute_set(pa);
	if ( pb )
		pmelf_destroy_attribute_set(pb);
	return rval;
}
