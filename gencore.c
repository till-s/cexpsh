/* $Id$ */

/* Utility to create a (elf) core file from a raw memory image.
 * For selected architectures (currently ppc32, i386 and m68k),
 * recording registers in the core file (netbsd layout) is supported.
 *
 * The netbsd layout was chosen because it is always supported by
 * libbfd.
 *
 * Author: Till Straumann <strauman@slac.stanford.edu>, 2004
 *
 * LICENSE: GPL -- this program makes extensive use of the BFD library.
 *          For details about the GPL, consult www.gnu.org.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define HAVE_STDINT_H 1
#define HAVE_SYS_TYPES_H 1
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <bfd.h>
#include <elf-bfd.h>

#if ! HAVE_TYPE_UINT32_T
#if SIZEOF_UNSIGNED_INT == 4
typedef unsigned int uint32_t;
#else
#error "Cannot determine an unsigned 32bit type for your host machine, please help"
#endif
#endif

typedef uint32_t Reg_t;

/* m68k netbsd layout: */
typedef struct m68k_reg_ {
	Reg_t regs[16]; /* D0-7, A0-7 */
	Reg_t sr;
	Reg_t pc;
} M68k_reg;

typedef struct m68k_fpreg_ {
	Reg_t fpregs[8*3]; /* FP0..FP7 */
	Reg_t fpcr;
	Reg_t fpsr;
	Reg_t fpiar;
} M68k_fpreg;

/* ppc32 netbsd layout: */
typedef struct ppc32_reg_ {
	Reg_t gpr[32];
	Reg_t lr;
	Reg_t cr;
	Reg_t xer;
	Reg_t ctr;
	Reg_t pc;
	Reg_t msr;	/* our extension */
	Reg_t dar;	/* our extension */
	Reg_t vec;  /* our extension */
} Ppc32_reg;

typedef struct ppc32_fpreg_ {
	double fpreg[32];
	double fpscr;
} Ppc32_fpreg;

typedef struct ppc32_vreg_ {
	Reg_t vreg[32][4];
	Reg_t vrsave;
	Reg_t spare[2];
	Reg_t vscr;
} Ppc32_vreg;

/* i386 */
typedef struct i386_reg_ {
	Reg_t	eax;
	Reg_t	ecx;
	Reg_t	edx;
	Reg_t	ebx;
	Reg_t	esp;
	Reg_t	epc;	/* ebp */
	Reg_t	esi;
	Reg_t	edi;
	Reg_t	eip;
	Reg_t	eflags;
	Reg_t	cs;
	Reg_t	ss;
	Reg_t	ds;
	Reg_t	es;
	Reg_t	fs;
	Reg_t	gs;
} I386_reg;

typedef struct i386_fpreg_ {
	char __data[108];
} I386_fpreg;

typedef struct i386_xmmreg_ {
	char __data[512];
} I386_xmmreg;

typedef union regs_ {
	M68k_reg  r_m68k;
	Ppc32_reg r_ppc32;
	I386_reg  r_i386;
} regs;

typedef union fpregs_ {
	M68k_fpreg  f_m68k;
	Ppc32_fpreg f_ppc32;
	I386_fpreg  f_i386;
} fpregs;

static char *progn = "gencore";

static void usage(int code)
{
const bfd_arch_info_type *arch = bfd_scan_arch("");

	fprintf(stderr,
		"Usage: %s [-hCc] [-r registers] [-p pc] [-s sp] [-f fp] [-e register_block_offset] [-x ELF_executable] [-a vma] memory_image_file [core_file_name] [-V symbol=magic_string]\n\n",
		progn);
	fprintf(stderr,
		"       Generate a core file from a raw memory image.\n");
	fprintf(stderr,
		"       Built for a '%s' target; $Revision$\n",
		bfd_target_list()[0]);
	fprintf(stderr,
		"       Author: Till Straumann, 2004; License: GPL\n");
	fputc('\n', stderr);
	fprintf(stderr,
		"       -a image_base     : base address of the memory image\n");
	fprintf(stderr,
		"       -h                : this message\n\n");
	fprintf(stderr,
		"       There are three methods to supply register values (optional):\n");
	fprintf(stderr,
		"       -r reg0[,reg1]... : supply registers in netbsd (ptregs) layout\n");
	fprintf(stderr,
		"                           (not all registers need to be specified)\n");
	fprintf(stderr,
		"       -p pc/-s sp/-f fp : just supply PC, SP and the frame pointer\n"); 
	fprintf(stderr,
		"       -e offset         : registers are read at 'offset' in the memory image\n");
	fprintf(stderr,
		"       -x ELF_executable : try to read register block offset using its symbol in the ELF executable\n");
	fprintf(stderr,
		"       -V strvar=strval  : do validity check on memory image. Look for and\n");
	fprintf(stderr,
		"                           compare a string variable. Yield an error if 'string'\n");
	fprintf(stderr,
		"                           is not found in the image at address of symbol.\n");
	fprintf(stderr,
		"                           E.g., if your application defines a string:\n");
	fprintf(stderr,
		"                               char *myString=\"MY_MAGIC\";\n");
	fprintf(stderr,
		"                           you would use '-V myString=MY_MAGIC'\n");
	fprintf(stderr,
		"       -c                : same as -V; use the CEXP magic string for validation\n");
	fprintf(stderr,
		"       -C                : same as -c; if the magic string is found, dump section -- NEEDS -x\n");
	fprintf(stderr,
		"                           address info for use by GDB (although you could and\n");
	fprintf(stderr,
		"                           should use GDB itself to create this info)\n");
	if (code)
		exit( code < 0 ? 1:0);
}

#define REGS_R		(1<<0)
#define REGS_OPT	(1<<1)
#define REGS_E		(1<<2)

/* inspired by gdb/gcore.c */
static void mk_phdr(bfd *obfd, asection *sec, void *closure)
{
int type = strncmp( bfd_section_name(obfd, sec), "load", 4 ) ? PT_NOTE : PT_LOAD;
	bfd_record_phdr(obfd, type, 1, PT_NOTE==type ?  PF_R : PF_R|PF_W|PF_X, 0, 0, 0, 0, 1, &sec);	
}

static int checkopt(int old, int opt)
{
	if ( old && ( REGS_OPT != opt || REGS_OPT != old ) ) {
			fprintf(stderr,"Only one of '-e', '-r' or any out of '-psf' can be used\n");
			exit(1);
	}
	return opt;
}

static unsigned long getn(char *msg, char *ptr)
{
char *chpt;
unsigned long rval;
	if ( !ptr || (rval=strtoul(ptr,&chpt,0), ptr==chpt) ) {
		fprintf(stderr,msg);
		usage(-1);
	}
	return rval;
}

static char *core_base = 0, *core_limit = 0;
static bfd_vma	core_vma = 0;
static bfd  *obfd = 0, *ibfd = 0;

static bfd_vma mapa(uint32_t addr)
{
	if ( addr < core_vma || (bfd_vma)(addr-core_vma) >  (bfd_vma)(core_limit-core_base) ) {
		fprintf(stderr,"Unable to map core address 0x%08x - cannot extract Cexp module info\n", addr);
		exit(1);
	}
	return (bfd_vma)(addr-core_vma) + (bfd_vma)core_base;
}

static bfd_vma geti(uint32_t addr)
{
	return bfd_get_bits((bfd_byte*)mapa(addr), 32, bfd_big_endian(ibfd));
}

#define OFFOF(RecPType, field) ((uint32_t)&((RecPType)0)->field - (uint32_t)(RecPType)0)
#define NOCEXP 0xffffffff					/* a presumably invalid address */
#define CEXPMOD_MAGIC_VAL "cexp0000"	    /* version of the cexpmod interface */
#define CEXPMOD_MAGIC_SYM "cexpMagicString"

/* These must match the layout in 'cexpmod.h' -- hopefully the x-compiler doesn't
 * change it...
 */
typedef struct mod_ {
	uint32_t p_name;
	uint32_t p_next;
	uint32_t p_sect_syms;
	uint32_t text_vma;
} *CexpMod;

typedef struct sym_ {
	uint32_t p_name;
	uint32_t p_val;
} *CexpSym;

static void
dump_cexp_info(bfd_vma addr)
{
uint32_t ptr,psyms,sym;
		ptr = geti(addr);
		while ( ptr ) {
			printf("Module Name: '%s'\n",   mapa(geti(ptr+OFFOF(CexpMod,p_name))));
			printf("       Text: 0x%08x\n", geti(ptr+OFFOF(CexpMod,text_vma)));
			if ( psyms = geti(ptr + OFFOF(CexpMod, p_sect_syms)) ) {
				printf("       Section Info:\n");
				while ( (sym = geti(psyms)) ) {
					printf("         @0x%08x: %s\n",
								geti(sym+OFFOF(CexpSym,p_val)),
								mapa(geti(sym+OFFOF(CexpSym,p_name)))
						  );
					psyms+=sizeof(uint32_t);
				}
			}
			ptr = geti(ptr + OFFOF(CexpMod,p_next));
		}
}

static bfd*
get_regs_vma_from_file(char *filename, char *symname, bfd_vma *pvma,int *phasRegs, bfd_vma *pcexp, char *magic_str)
{
bfd		*abfd, *rval = 0;
asymbol	**syms = 0;
int		i,found,tmp;
bfd_vma	theaddr;
char	*magic_val;

	if ( magic_str ) {
		assert( (magic_val = strchr(magic_str,'=')) );
		*magic_val++ = 0;

		if ( strcmp(CEXPMOD_MAGIC_SYM, magic_str) )
			pcexp = 0;
	}

	if ( ! (abfd=bfd_openr(filename,0)) ) {
		bfd_perror("Opening executable file");
		goto cleanup;
	}

	if (!bfd_check_format(abfd, bfd_object)) {
		bfd_perror("Checking executable file format");
		goto cleanup;
	}

	fprintf(stderr,"Executable file is for target %s, Arch/Mach: %s\n",
			bfd_get_target(abfd),
			bfd_printable_name(abfd));

	if (symname) {
		if (!(HAS_SYMS & bfd_get_file_flags(abfd))) {
			fprintf(stderr,"No symbols found\n");
			goto cleanup;
		}
		if ((i=bfd_get_symtab_upper_bound(abfd))<0) {
			fprintf(stderr,"Fatal error: illegal symtab size\n");
			goto cleanup;
		}
		if (i) {
			syms=(asymbol**)xmalloc(i);
			i = i/sizeof(asymbol*) - 1;
		}
		if (bfd_canonicalize_symtab(abfd,syms) <= 0) {
			bfd_perror("Canonicalizing symtab");
			goto cleanup;
		}

		found = 0;
		while (--i >= 0 && found < 7) {
			if ( (syms[i]->flags & BSF_GLOBAL) && ! (syms[i]->flags & BSF_FUNCTION) ) {
				if (!strcmp(symname,bfd_asymbol_name(syms[i]))) {
					theaddr = bfd_asymbol_value(syms[i]);
					found|=1;
				}
				else if (pcexp && !strcmp("cexpSystemModule",bfd_asymbol_name(syms[i]))) {
					*pcexp = bfd_asymbol_value(syms[i]);
					found|=2;
				}
				else if ( magic_str && !strcmp(magic_str,bfd_asymbol_name(syms[i]))) {
					uint32_t pstring = bfd_asymbol_value(syms[i]);
					uint32_t string  = bfd_get_bits((bfd_byte*)mapa(pstring), 32, bfd_big_endian(abfd));

					tmp = core_limit - (char*)mapa(string);	

					if ( !strncmp(magic_val, (char*)mapa(string), tmp) )
						found|=4;
					else {
						char buf[50];
						if ( tmp > sizeof(buf) ) 
							tmp = sizeof(buf);
						strncpy(buf,(char*)mapa(string), tmp);
						fprintf(stderr,"Magic string doesn't match - this is probably not a valid image file\n");
						fprintf(stderr,"Looking for '%s' @(0x%08x->0x%08x) -- found '", magic_val, pstring, string);
						for (tmp=0; tmp<sizeof(buf) && buf[tmp]; tmp++)
							fputc( isprint(buf[tmp]) ? buf[tmp] : '.' , stderr);
						fprintf(stderr,"'\nBAILING OUT\n");
						goto cleanup;
					}
				}
			}
		}
		if ( magic_str && !(found & 4) ) {
			fprintf(stderr,"Magic symbol '%s' not found in executable; BAILING OUT\n", magic_str);
			goto cleanup;
		}
		if ( !(found & 1) ) {
			fprintf(stderr,"Symbol '%s' not found in executable - won't write registers to core file\n",
					symname);
			fprintf(stderr,"(unless specified by explicit option)\n");
		} else {
			if ( *phasRegs ) {
				fprintf(stderr,"Warning: '%s' found in executable but overriden by command line option\n",
					symname);
			} else {
				printf("Registers (symbol '%s' found) at 0x%08x\n", symname, theaddr);
				*phasRegs = REGS_E;
				*pvma     = theaddr;
			}
		}
	} else {
		if ( !*phasRegs ) {
			fprintf(stderr,"Warning: no symbol name given - won't write registers to core file\n");
		}
	}

	rval = abfd;
	abfd = 0;

cleanup:
	if (abfd)
		bfd_close_all_done(abfd);
	if (syms)
		free(syms);
	return rval;
}


int
main(int argc, char **argv)
{
const bfd_arch_info_type	*arch;
int							i,fd,opt;
asection					*sl, *note_s = 0;
char						*chpt, *nchpt, *name=0;
struct stat					st;
Reg_t						pcspfp[3] = {0};
int							hasRegs = 0;
int							note_size = 0;
char     					*note;
bfd_vma						regblock_addr;
bfd_vma						cexp_addr = NOCEXP;
unsigned					regsz = 0;
regs						gregs;
Reg_t						*preg;
int							nreg = 0;
int							machinc = 1;
char						*symname="_BSP_Exception_NBSD_Registers";
char						*symfile = 0;
int							rval = 1;
char						*magic = 0;
int							dumpSectionInfo = 0;

		progn = argv[0];
		if ( chpt = strrchr(progn, '/') )
			progn = chpt + 1;
		memset(&gregs, 0, sizeof(gregs));

		bfd_init();

		while ( (opt = getopt(argc, argv, "hr:a:p:s:f:e:x:S:V:cC")) > 0 ) {
			i = 0;
			switch ( opt ) {
			default:	fprintf(stderr,"Invalid option\n");
			case 'h':	usage(1);

			case 'r':	nreg = 0;
						preg = (Reg_t*)&gregs;
						if ( !(chpt = optarg) ) {
							usage(-1);
						}
						do {
							if ( nreg >= sizeof(gregs)/sizeof(Reg_t) ) {
								fprintf(stderr,"Too many registers\n");
								exit(1);
							}
							*preg = strtoul(chpt, &nchpt, 0);
							if ( chpt == nchpt ) {
								fprintf(stderr,"Invalid number\n");
								usage(-1);
							}
							preg++; nreg++;
						} while ( (chpt = nchpt) && *chpt++==',' );
						hasRegs = checkopt(hasRegs, REGS_R);
						break;

			case 'f':	i++;
			case 's':	i++;
			case 'p':   pcspfp[i] = getn("Invalid PC/SP/FP argument\n", optarg);
						hasRegs = checkopt(hasRegs,REGS_OPT);
						break;

			case 'e':	regblock_addr = getn("Invalid -e argument\n", optarg);
						hasRegs = checkopt(hasRegs,REGS_E);
						break;

			case 'a':	core_vma = getn("Invalid -a argument\n", optarg);
						break;

			case 'x':	if ( !(symfile = optarg) || !*symfile )
							fprintf(stderr,"Invalid -x argument\n");
						break;

			case 'S':	symname = optarg;
						break;

			case 'C':	dumpSectionInfo=1;
			case 'V':	
			case 'c':	if ( magic ) {
							fprintf(stderr,"Only 1 of -V -c -C may be given\n");
							exit(1);
						}
						if ( 'V' == opt ) {
							if ( !(magic=strdup(optarg)) || !strchr(magic,'=') ) {
								fprintf(stderr,"invalid -V argument: %s is not of format 'sym=val'\n", optarg);
								exit(1);
							}
						} else {
							magic = strdup(CEXPMOD_MAGIC_SYM"="CEXPMOD_MAGIC_VAL);
						}
						break;
			}
		}

		argc -= optind;
		argv += optind;
		if ( argc < 1 ) {
			usage(-1);
		}

		if ( (fd=open( argv[0], O_RDONLY )) < 0 ) {
			perror("Opening memory image file");
			exit(1);
		}

		if ( fstat( fd, &st ) ) {
			perror("Stat on memory image");
			exit(1);
		}

		if ( !(core_base=mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0) ) ) {
			perror("mmap failed");
			exit(1);
		}
		core_limit = core_base + st.st_size;

		if ( symfile ) {
			if ( !(ibfd = get_regs_vma_from_file(symfile, symname, &regblock_addr, &hasRegs, &cexp_addr, magic)) ) {
				fprintf(stderr,"(Unable to read register block address from executable)\n");
				goto cleanup;
			} else {
				if ( NOCEXP!=cexp_addr && dumpSectionInfo )
					dump_cexp_info(cexp_addr);
			}
		} else {
			/* try to guess defaults (see below) */
		}

		if ( argc > 1 ) {
			chpt = argv[1];
		} else {
			chpt = symfile ? symfile : argv[0];
			if ( !(name = malloc(strlen(chpt)+6)) ) {
				perror("malloc");
				exit(1);
			}

			strcpy(name, chpt);

			if ( !(chpt = strrchr(name,'.')) ) {
				chpt = name + strlen(name);
			}
			strcpy(chpt,".core");
			chpt = name;
		}


		if (   !(obfd=bfd_openw(chpt, ibfd ? bfd_get_target(ibfd) : "default"))
			|| ! bfd_set_format(obfd,bfd_core)
		   )  {
			bfd_perror("Unable to create output BFD");
			goto cleanup;
		}

		printf("Generating core file for target: %s", bfd_get_target(obfd));

		if ( ibfd ) {
			if ( ! bfd_set_arch_mach(obfd, bfd_get_arch(ibfd), bfd_get_mach(ibfd)) ) {
				printf("\n");
				bfd_perror("Unable to set core file architecture)");
				fprintf(stderr,"(BFD Target %s, Arch/Mach: %s)\n",
						bfd_get_target(obfd),
						bfd_printable_name(ibfd));
				goto cleanup;
			}
			printf(" (Arch/Mach: %s) ...", bfd_printable_name(obfd));
		} else {
			for ( i = (int)bfd_arch_obscure + 1; i < (int)bfd_arch_last; i++ ) {
				if ( bfd_set_arch_mach(obfd, (enum bfd_architecture)i, 0) ) {
					printf("-> guessed Arch/Mach: %s\n", bfd_printable_name(obfd));
					break;
				}
			}
			if ( i >= (int)bfd_arch_last ) {
				fprintf(stderr,"No architecture found :-(\n");
				goto cleanup;
			}
		}

		switch ( bfd_get_arch(obfd) ) {
/*
 			case bfd_arch_alpha:
				machinc = 0;
			break;
 			case bfd_arch_sparc:
				machinc = 0;
			break;
*/
			case bfd_arch_powerpc:
				regsz = sizeof(gregs.r_ppc32); break;
				if ( REGS_OPT == hasRegs ) {
					gregs.r_ppc32.pc     = pcspfp[0];
					gregs.r_ppc32.gpr[1] = pcspfp[1];
					gregs.r_ppc32.lr     = pcspfp[2];
				}
/*
			case bfd_arch_i386:
				regsz = sizeof(gregs.r_i386);  break;
				if ( REGS_OPT == hasRegs ) {
					gregs.r_i386.eip = pcspfp[0];
					gregs.r_i386.esp = pcspfp[1];
					gregs.r_i386.epc = pcspfp[2];
				}
*/
			case bfd_arch_m68k:
				regsz = sizeof(gregs.r_m68k);
				if ( REGS_OPT == hasRegs ) {
					gregs.r_m68k.pc        = pcspfp[0];
					gregs.r_m68k.regs[8+7] = pcspfp[1];
					gregs.r_m68k.regs[8+6] = pcspfp[2];
				}
			break;

			default:
				fprintf(stderr,"Setting registers for this architecture not supported\n");
				hasRegs = 0;
				break;	
		}

		if ( hasRegs ) {
			if ( REGS_E != hasRegs ) {
				Reg_t r;
				for ( i = 0, preg=((Reg_t*)&gregs); i < regsz/sizeof(Reg_t); i++, preg++ ) {
					r = *preg;
					bfd_put_bits(r, (bfd_byte*)preg, 32, bfd_big_endian(obfd));
				}
				preg = (Reg_t*)&gregs;
			} else {
				if ( regblock_addr < core_vma
				     || (regblock_addr - core_vma) >= st.st_size
           	         || (regblock_addr - core_vma) + regsz >= st.st_size ) {
					fprintf(stderr,"-e argument out of range\n");
					exit(1);
				}
				preg = (Reg_t*)(core_base + (regblock_addr - core_vma));
			}
			note = elfcore_write_note(obfd, 0, &note_size, "NetBSD-CORE", NT_NETBSDCORE_FIRSTMACH + machinc, preg, regsz);
			if ( !(note_s = bfd_make_section(obfd,".note"))
				 || ! bfd_set_section_flags(obfd, note_s, SEC_HAS_CONTENTS | SEC_READONLY | SEC_ALLOC)
				 || ! bfd_set_section_size(obfd, note_s, note_size)
			   ) {
				bfd_perror("Creating ELF Note for registers failed");
				goto cleanup;
			}
			bfd_set_section_vma(obfd, note_s, 0);
			bfd_set_section_alignment(obfd, note_s, 0);
		} else {
			printf("No registers supplied; omitting.\n");
		}

		if ( ! (sl=bfd_make_section(obfd,"load0"))
			 || ! bfd_set_section_flags(obfd, sl, SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_DATA )
			 || ! bfd_set_section_size(obfd, sl, st.st_size)
			 || ! bfd_set_section_vma(obfd, sl, core_vma)
			)
		{
			bfd_perror("error creating load section");
			goto cleanup;
		}

		bfd_map_over_sections(obfd, mk_phdr, 0);

		if (
			 ! bfd_set_section_contents (obfd, sl, core_base, 0, st.st_size)
			 || (note_s && ! bfd_set_section_contents (obfd, note_s, note, 0, note_size) )
		   ) {
			bfd_perror("error setting section contents");
			goto cleanup;
		}

		printf("memory image is %i bytes.\n", st.st_size);
		rval = 0;

cleanup:
		if (obfd) {
			bfd_close(obfd);
			if ( rval )
				unlink(chpt);
		}
		if (ibfd)
			bfd_close_all_done(ibfd);
		free(name);
		free(magic);

		return rval;
}
