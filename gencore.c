#include <bfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>

static char *targ="elf32-i386";
static char *tarch="i386";

int
main(int argc, char **argv)
{
bfd							*obfd,*ibfd;
const bfd_arch_info_type	*arch;
int							i,fd;
asection					*sl;
char						*data;
struct stat					st;

		bfd_init();

		if ( argc < 2 ) {
			fprintf(stderr,"Usage: %s memory_image_file\n", argv[0]);
			exit(1);
		}

		if ( (fd=open( argv[1], O_RDONLY ) < 0 ) ) {
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

		if (!(obfd=bfd_openw("foo","default")) ||
			! bfd_set_format(obfd,bfd_core) ||
			! (arch=bfd_scan_arch("")) ||
			! bfd_set_arch_mach(obfd,arch->arch,arch->mach)
		   )  {
			bfd_perror("Unable to create output BFD");
			goto cleanup;
		}
		printf("arch: %s\n",bfd_printable_name(obfd));


		if ( ! (sl=bfd_make_section(obfd,"load0")) ||
			 ! bfd_set_section_flags(obfd, sl, SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_DATA ) ||
			 ! bfd_set_section_size(obfd, sl, st.st_size) ||
			 ! bfd_set_section_contents (obfd, sl, data, 0, st.st_size)
			)
		{
			bfd_perror("error writing load section");
		}

cleanup:
		if (obfd)
			bfd_close(obfd);

		return 0;
}
