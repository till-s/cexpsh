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

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <bfd.h>
#include <elf-bfd.h>

typedef uint32_t Reg_t;

/* m68k netbsd layout: */
typedef struct m68k_reg_ {
	Reg_t regs[16]; // D0-7, A0-7
	Reg_t sr;
	Reg_t pc;
} M68k_reg;

typedef struct m68k_fpreg_ {
	Reg_t fpregs[8*3]; // FP0..FP7
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
		"Usage: %s [-h] [-r registers] [-p pc] [-s sp] [-f fp] [-e register_block_offset] [-a vma] memory_image_file [core_file_name]\n\n",
		progn);
	fprintf(stderr,
		"       Generate a core file from a raw memory image.\n");
	fprintf(stderr,
		"       Built for a '%s' target; $Revision$\n",
		bfd_printable_arch_mach(arch->arch, arch->mach));
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

int
main(int argc, char **argv)
{
bfd							*obfd,*ibfd;
const bfd_arch_info_type	*arch;
int							i,fd,opt;
asection					*sl, *note_s = 0;
char						*data, *chpt, *nchpt, *name=0;
struct stat					st;
Reg_t						pcspfp[3] = {0};
int							hasRegs = 0;
int							note_size = 0;
char     					*note;
unsigned					vma = 0;
unsigned					regblock_addr;
unsigned					regsz = 0;
regs						gregs;
Reg_t						*preg;
int							nreg = 0;
int							machinc = 1;

		progn = argv[0];
		if ( chpt = strrchr(progn, '/') )
			progn = chpt + 1;
		memset(&gregs, 0, sizeof(gregs));

		bfd_init();

		while ( (opt = getopt(argc, argv, "hr:a:p:s:f:e:")) > 0 ) {
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

			case 'a':	vma = getn("Invalid -a argument\n", optarg);
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

		if ( !(data=mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0) ) ) {
			perror("mmap failed");
			exit(1);
		}

		if ( argc > 1 ) {
			chpt = argv[1];
		} else {
			if ( !(name = malloc(strlen(argv[0])+6)) ) {
				perror("malloc");
				exit(1);
			}

			strcpy(name, argv[0]);

			if ( !(chpt = strrchr(name,'.')) ) {
				chpt = name + strlen(name);
			}
			strcpy(chpt,".core");
			chpt = name;
		}

		if (!(obfd=bfd_openw(chpt,"default")) ||
			! bfd_set_format(obfd,bfd_core) ||
			! (arch=bfd_scan_arch("")) ||
			! bfd_set_arch_mach(obfd,arch->arch,arch->mach)
		   )  {
			bfd_perror("Unable to create output BFD");
			goto cleanup;
		}

		printf("Generating core file for target: %s ...",
				bfd_printable_arch_mach(arch->arch, arch->mach));

		switch ( bfd_get_arch(obfd) ) {
/*
 			case bfd_arch_alpha:
				machinc = 0;
			break;
 			case bfd_arch_sparc:
				machinc = 0;
			break;
			case bfd_arch_powerpc:
				regsz = sizeof(gregs.r_ppc32); break;
				if ( REGS_OPT == hasRegs ) {
					gregs.r_ppc32.pc     = pcspfp[0];
					gregs.r_ppc32.gpr[1] = pcspfp[1];
					gregs.r_ppc32.lr     = pcspfp[2];
				}
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
				if ( regblock_addr < vma
				     || (regblock_addr - vma) >= st.st_size
           	         || (regblock_addr - vma) + regsz >= st.st_size ) {
					fprintf(stderr,"-e argument out of range\n");
					exit(1);
				}
				preg = (Reg_t*)(data + (regblock_addr - vma));
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
			 || ! bfd_set_section_vma(obfd, sl, vma)
			)
		{
			bfd_perror("error creating load section");
			goto cleanup;
		}

		bfd_map_over_sections(obfd, mk_phdr, 0);

		if (
			 ! bfd_set_section_contents (obfd, sl, data, 0, st.st_size)
			 || (note_s && ! bfd_set_section_contents (obfd, note_s, note, 0, note_size) )
		   ) {
			bfd_perror("error setting section contents");
			goto cleanup;
		}

		printf("memory image is %i bytes.\n", st.st_size);

cleanup:
		if (obfd)
			bfd_close(obfd);
		free(name);

		return 0;
}
