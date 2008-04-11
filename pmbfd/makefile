BFDDIR=/home/till/binutils-2.17/
BFDHDIR=-I$(BFDDIR)/build/bfd -I$(BFDDIR)/include -I$(BFDDIR)/bfd
BFDLIBDIR=-L$(BFDDIR)/build/bfd -L$(BFDDIR)/build/libiberty
all: bfdod rdelf libpmbfd.a

CFLAGS=-Wall -O2 -g

PMELF_SRCS=elf.c symname.c secname.c strm.c dmpgrps.c
PMELF_SRCS+=dmpsym.c dmpsymtab.c
PMELF_SRCS+=dmpshdr.c dmpshtab.c
PMELF_SRCS+=dmpehdr.c
PMELF_SRCS+=symtab.c
PMELF_SRCS+=findsymhdrs.c
PMELF_SRCS+=shtab.c
PMELF_SRCS+=getgrp.c
PMELF_SRCS+=getscn.c
PMELF_SRCS+=getsym.c
PMELF_SRCS+=getshdr.c
PMELF_SRCS+=getehdr.c

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

libpmbfd.a: bfd.o elf.o
	ar -r $@ $^
	ranlib $@

clean:
	$(RM) $(patsubst %.c,%.o,$(wildcard *.c)) bfdod rdelf odbfd libpmbfd.a libpmelf.a
