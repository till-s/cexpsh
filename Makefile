#
# Makefile to bootstrap after a CVS checkout
#
# NOTE: look in the ATTIC for 'mf' - it was my
#       original makefile and might contain
#       useful info
#

prep \
all: src bootstrap


YFLAGS=-v -d -p cexp

cexp.tab.c cexp.tab.h: cexp.y
	bison $(YFLAGS) $^

# remove the default rule which tries to make cexp.c from cexp.y
cexp.c: ;

gentab: gentab.c
	$(CC) -O -o $@ $^

jumptab.c: gentab
	if ! ./$^ > $@; then $(RM) $@ $^; exit 1; fi
	$(RM) $^

links:	$(LINKDIR)/binutils-2.13 $(LINKDIR)/regexp $(LINKDIR)/libelf-0.8.0 $(LINKDIR)/libtecla-1.4.1
	ln -s $^ .

src: cexp.tab.c cexp.tab.h jumptab.c

bootstrap:
	aclocal && autoconf && autoheader && automake -ac
	(cd libtecla-1.4.1; autoconf)
	(cd libelf-0.8.2; aclocal && autoconf && autoheader)
	(cd regexp; aclocal && autoconf && automake)

clean distclean:
	$(RM) gentab Makefile.in aclocal.m4 cexp.output cexp.tab.c
	$(RM) -r autom4te.cache
	$(RM) cexp.tab.h compile config.guess config.h.in config.sub configure
	$(RM) depcomp gentab install-sh jumptab.c missing mkinstalldirs
	$(RM) libtecla-1.4.1/configure

