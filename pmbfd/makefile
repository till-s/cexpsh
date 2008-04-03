all: bfdod rdelf

CFLAGS=-Wall -O2

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

bfdod: bfdod.o bfd.o elf.o
	$(CC) $(LDFLAGS) -o $@ $^

rdelf: rdelf.o elf.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	$(RM) *.o bfdod rdelf

