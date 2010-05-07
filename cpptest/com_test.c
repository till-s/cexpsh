#if defined(COM1) || defined(COM2)
long com1;
#if defined(COM1)
long cexp_test_num_errors = 0;
#endif
#if defined(COM2)
#define COM_ALIGNMENT  32
long com2;
long com2a __attribute__((aligned(COM_ALIGNMENT)));
#endif
#endif

#if defined(COM1)
int
_cexpModuleInitialize(void*unused)
{
	com1 = (long)&com1;
}
#endif

#if defined(COM2)

#include <stdio.h>

#define NAM "com_test "

static int check_algn(unsigned long addr, unsigned long algn, char *nam)
{
	if ( (addr & (algn - 1)) ) {
		fprintf(stderr,NAM"FAILED: %s misaligned: requested %lu, address is 0x%lx\n", nam, algn, addr);
		return 1;
	}
	return 0;
}

int
run_com_test(void)
{
int rval = 0;

	/* Check that we read back what was set from the first module */
	if ( com1 != (long)&com1 ) {
		fprintf(stderr,NAM"FAILED: 2nd module didn't get COM variable value of first module (com1: 0x%lx, &com1 0x%0lx)\n", com1, (long)&com1);
		rval++;
	}

	/* Check alignment of 'com2a' */
	rval += check_algn((unsigned long)&com2a, COM_ALIGNMENT, "com2a");

	/* Also double check alignment of com1, com2 */
	rval += check_algn((unsigned long)&com1, sizeof(com1), "com1");
	rval += check_algn((unsigned long)&com2, sizeof(com2), "com2");

	if ( !rval ) {
		fprintf(stderr,"COM test PASSED\n");
	}

	return rval;
}
#endif
