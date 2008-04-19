BFDDIR=/home/till/binutils-2.17/
BFDHDIR=-I$(BFDDIR)/build/bfd -I$(BFDDIR)/include -I$(BFDDIR)/bfd
BFDLIBDIR=-L$(BFDDIR)/build/bfd -L$(BFDDIR)/build/libiberty
all: bfdod rdelf libpmbfd.a

CFLAGS=-Wall -O2 -g -m64

PMELF_SRCS =symname.c secname.c dmpgrps.c
PMELF_SRCS+=strm.c mstrm.c fstrm.c
PMELF_SRCS+=dmpsym.c dmpsymtab.c
PMELF_SRCS+=dmpshdr.c dmpshtab.c
PMELF_SRCS+=dmpehdr.c
PMELF_SRCS+=symtab.c
PMELF_SRCS+=findsymhdrs.c
PMELF_SRCS+=shtab.c
PMELF_SRCS+=getgrp.c
PMELF_SRCS+=getscn.c
PMELF_SRCS+=getsym.c
PMELF_SRCS+=putsym.c
PMELF_SRCS+=putdat.c
PMELF_SRCS+=getshdr.c
PMELF_SRCS+=putshdr.c
PMELF_SRCS+=getehdr.c
PMELF_SRCS+=putehdr.c

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

bfdod: bfdod.o libpmelf.a libpmbfd.a
	$(CC) $(LDFLAGS) -o $@ $< -L. -lpmbfd -lpmelf

rdelf: rdelf.o libpmelf.a 
	$(CC) $(LDFLAGS) -o $@ $< -L. -lpmelf

odbfd: bfdod.c
	$(CC) $^ -o $@ $(CFLAGS) -DUSE_REAL_BFD $(BFDHDIR) $(LDFLAGS) $(BFDLIBDIR) -lbfd -liberty

libpmelf.a: $(PMELF_SRCS:%.c=%.o)
	ar -r $@ $^
	ranlib $@

libpmbfd.a: bfd.o
	ar -r $@ $^
	ranlib $@

clean:
	$(RM) $(patsubst %.c,%.o,$(wildcard *.c)) bfdod rdelf odbfd libpmbfd.a libpmelf.a
