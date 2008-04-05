BFDDIR=/home/till/binutils-2.17/
BFDHDIR=-I$(BFDDIR)/build/bfd -I$(BFDDIR)/include -I$(BFDDIR)/bfd
BFDLIBDIR=-L$(BFDDIR)/build/bfd -L$(BFDDIR)/build/libiberty
all: bfdod rdelf libpmbfd.a

CFLAGS=-Wall -O2 -g

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

bfdod: bfdod.o bfd.o elf.o
	$(CC) $(LDFLAGS) -o $@ $^

rdelf: rdelf.o elf.o
	$(CC) $(LDFLAGS) -o $@ $^

odbfd: bfdod.c
	$(CC) $^ -o $@ $(CFLAGS) -DUSE_REAL_BFD $(BFDHDIR) $(LDFLAGS) $(BFDLIBDIR) -lbfd -liberty

libpmbfd.a: bfd.o elf.o
	ar -r $@ $^
	ranlib $@

clean:
	$(RM) $(patsubst %.c,%.o,$(wildcard *.c)) bfdod rdelf odbfd libpmbfd.a
