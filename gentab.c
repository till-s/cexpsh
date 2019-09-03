#include <stdio.h>

/* 'gentab' is a utility to generate the CEXP function call
 * jumptable. It is very crude because the jumptable is
 * actually not meant to ever change...
 */

/* Author: Till Straumann <strauman@slac.stanford.edu>, 2/2002 */


/* this is the 'configurable' parameter
 * read the respective comments in ctyps.c
 * before changing it. The numbers used 
 * by ctyps MUST AGREE with the number here.
 * As I said, this is _not_designed_ to 
 * ever change!
 */

/*
 * Copyright 2002, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */

#define MAXBITS 5

#define UL 0
#define DB 1
#define RES UL

char *typ[]={"UL","DB"};
char  typch[]={'L','D'};
char  ch[]={'l','d'};

static void
tnam(int mask)
{
int i;
	for (i=1<<(MAXBITS-1); i; i>>=1)
		printf("%c",mask&i ? typch[DB] : typch[UL]);
}

static void
targs(int mask)
{
#if 0 /* DONT emit prototype args; PPC/SYSV calling conventions may
       * produce incorrect calls if calling a vararg function with
       * double arguments :-(
       */
int i;
	for (i=1<<(MAXBITS-1); i; i>>=1)
		printf("%s%s",mask&i ? typ[DB] : typ[UL],i==1  ? "" : ",");
#endif
}

static void
fnam(int mask)
{
int i;
	for (i=1<<(MAXBITS-1); i; i>>=1)
		printf("%c",mask&i ? ch[DB] : ch[UL]);
}

static void
protoargs(void)
{
int i;
	printf("(AA f");
	for (i=1; i<=MAXBITS; i++)
		printf(",AA a%i",i);
	printf(")");
}

static void
callargs(int mask)
{
int i,j;
	for ((i=1<<(MAXBITS-1)),(j=1); i; i>>=1,j++) {
		printf("%sa%i->tv.%c",
			j>1?",":"",
			j,
			mask&i ? ch[DB] : ch[UL]);
	}
}


int
main()
{
int mask;
	printf("/* WARNING: DO NOT EDIT THIS AUTOMATICALLY-GENERATED FILE */\n");
	printf("#define JUMPTAB_ARGLIST(args) ");
for (mask=0; mask < MAXBITS; mask++)
	printf(",args[%i]",mask);
	printf("\n");
	printf("#define MAXBITS %i\n",MAXBITS);
for (mask=0; mask< (1<<MAXBITS); mask++) {
	printf("typedef %s (*",typ[RES]); tnam(mask); printf(")("); targs(mask); printf(");\n");
	printf("static  %s ",typ[RES]);    fnam(mask); protoargs(); printf("\n");
	printf("{return (("); tnam(mask); printf(")f->tv.p)("); callargs(mask); printf(");}\n\n");
}
	printf("static UFUNC jumptab[%i]={\n",1<<MAXBITS);
for (mask=0; mask < (1<<MAXBITS); mask++) {
	printf("\t"); fnam(mask); printf(",\n");
}
	printf("};\n");
return 0;
}
