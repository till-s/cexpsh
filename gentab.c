/* $Id$ */
#include <stdio.h>

/* 'gentab' is a utility to generate the CEXP function call
 * jumptable. It is very crude because the jumptable is
 * actually not meant to ever change...
 */

/* this is the 'configurable' parameter
 * read the respective comments in ctyps.c
 * before changing it. The numbers used 
 * by ctyps MUST AGREE with the number here.
 * As I said, this is _not_designed_ to 
 * ever change!
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
