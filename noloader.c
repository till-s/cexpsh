#include <cexp.h>
#include <cexpmodP.h>

int
cexpLoadFile(char *filename, CexpModule new_module)
{
	fprintf(stderr, "Unable to load '%s' -- no object loader was configured\n", filename);
	return -1;
}
