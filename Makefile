#
# Makefile to bootstrap after a CVS checkout
#
# NOTE: look in the ATTIC for 'mf' - it was my
#       original makefile and might contain
#       useful info
#

# if this is checked out for the 'xsyms' module,
# most of the files are missing and we just use
# the trivial rule for 'xsyms'

all: $(if $(wildcard Makefile.am),,xsyms)
	@if [ -f Makefile.am ] ; then \
		echo This Makefile must be invoked with an explicit target; \
		echo Possible Targets are; \
		echo '    "xsyms": make the "xsyms" host tool (using installed libelf or libbfd)';\
		echo '    "clean": remove "xsyms"'; \
		echo '     "prep": regenerate non-CVS/autoxxx files (requires bison-1.28 and autotools)'; \
		echo '"distclean": remove non-CVS files (REQUIRES bison-1.28 AND autotools TO RECREATE)'; \
		exit 1; \
	fi


prep: $(if $(wildcard Makefile.am),src bootstrap,xsyms)


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

bootstrap-xsyms bootstrap-cexp:
	aclocal && autoconf && autoheader && automake -ac

bootstrap: bootstrap-cexp
	(cd libtecla; autoconf)
	(cd regexp; aclocal && autoconf && automake)

clean:
	$(RM) xsyms

distclean: clean
	$(RM) gentab Makefile.in aclocal.m4 cexp.output cexp.tab.c
	$(RM) -r autom4te.cache
	$(RM) cexp.tab.h compile config.guess config.h.in config.sub configure
	$(RM) depcomp gentab install-sh jumptab.c missing mkinstalldirs
	$(RM) libtecla/configure

# primitive rule to just make either variant of xsyms
# NOTE: these rules are intended to use either libbfd or libelf
#       already installed on the HOST, i.e. not the ones probably
#       in a CEXP subdir.
xsyms: xsyms.c xsyms-bfd.c
	@($(CC) -o $@ -I/usr/include/libelf xsyms.c -lelf && echo $@ built with libelf ) || ( echo 'build using libelf failed; trying libbfd' &&  $(CC) -o $@ xsyms-bfd.c -lbfd -liberty && echo $@ built with libbfd )

install:
	@echo You must install 'xsyms' manually
	exit 1
