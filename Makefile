#
# Makefile to bootstrap after a CVS checkout
#
# NOTE: look in the ATTIC for 'mf' - it was my
#       original makefile and might contain
#       useful info
#

all: src bootstrap


YFLAGS=-v -d -p cexp

cexp.tab.c cexp.tab.h: cexp.y
	bison $(YFLAGS) $^

# remove the default rule which tries to make cexp.c from cexp.y
cexp.c: ;

gentab: gentab.c
	$(CC) -O -o $@ $^

jumptab.c: gentab
	./$^ > $@
	$(RM) $^

links:	./binutils-2.13 ./regexp ./libelf-0.8.0 ./libtecla-1.4.1
	ln -s $(LINKDIR)/$^ .

src: cexp.tab.c cexp.tab.h jumptab.c

bootstrap:
	aclocal
	autoconf
	autoheader
	automake -a
