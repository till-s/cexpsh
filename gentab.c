#include <stdio.h>

#define UL 0
#define DB 1
#define RES UL

#define MAXBITS 5

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
int i;
	for (i=1<<(MAXBITS-1); i; i>>=1)
		printf("%s%s",mask&i ? typ[DB] : typ[UL],i==1  ? "" : ",");
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
for (mask=0; mask< (1<<MAXBITS); mask++) {
	printf("typedef %s (*",typ[RES]); tnam(mask); printf(")("); targs(mask); printf(");\n");
	printf("static  %s ",typ[RES]);    fnam(mask); protoargs(); printf("\n");
	printf("{return (("); tnam(mask); printf(")f->tv.p)("); callargs(mask); printf(");}\n\n");
}
	printf("static %s (*jumptab[%i])()={\n",typ[RES],1<<MAXBITS);
for (mask=0; mask < (1<<MAXBITS); mask++) {
	printf("\t"); fnam(mask); printf(",\n");
}
	printf("};\n");
}
