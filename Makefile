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

XSYMS_ONLY_TEST_FILE=cexp.y

all:
	@if [ -f Makefile.am ] ; then \
		echo This Makefile must be invoked with an explicit target; \
		echo Possible Targets are; \
		echo '    "xsyms": make the "xsyms" host tool (using installed libelf or libbfd)';\
		echo '    "clean": remove "xsyms"'; \
		echo '     "prep": regenerate non-CVS/autoxxx files (requires bison-1.28 and autotools)'; \
		echo '"distclean": remove non-CVS files (REQUIRES bison-1.28 AND autotools TO RECREATE)'; \
		exit 1; \
	fi


prep: $(if $(wildcard $(XSYMS_ONLY_TEST_FILE)),src bootstrap, bootstrap-xsyms)
	@echo you may now create a build subdirectory and ../configure the package


YFLAGS=-v -d -p cexp

ifndef BISON
BISON=bison
endif

cexp.tab.c cexp.tab.h: cexp.y
	$(BISON) $(YFLAGS) $^

# remove the default rule which tries to make cexp.c from cexp.y
cexp.c: ;

gentab: gentab.c
	$(CC) -O -o $@ $^

jumptab.c: gentab
	if ./$^ > $@; then true ; else $(RM) $@ $^; exit 1; fi
	$(RM) $^

links:	$(LINKDIR)/binutils-2.13 $(LINKDIR)/regexp $(LINKDIR)/libelf-0.8.0 $(LINKDIR)/libtecla-1.4.1
	ln -s $^ .

src: cexp.tab.c cexp.tab.h jumptab.c

ifdef tools_prefix
# AUTOPATH = $(tools_prefix)/
# must use the real search path; providing absolute path
# when calling autotools doesn't work because automake seems
# to call 'autoconf'.
PATH:= $(tools_prefix):$(PATH)
endif

ACLOCAL    = $(AUTOPATH)aclocal
AUTOCONF   = $(AUTOPATH)autoconf 
AUTOHEADER = $(AUTOPATH)autoheader
AUTOMAKE   = $(AUTOPATH)automake

bootstrap-xsyms bootstrap-cexp:
	$(ACLOCAL) && $(AUTOCONF) && $(AUTOHEADER) && $(AUTOMAKE) -ac
#	ln -s binutils binutils-x
bootstrap: bootstrap-cexp
	(cd libtecla; $(AUTOCONF))

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
##################################################################
#       THIS WORKS ONLY ON AN ELF SYSTEM
#       On other host systems, you must use a 'cross-bfd' library,
#       i.e. you must configure and build binutils with the
#       "--target=<target>" system and link 'xsyms' against
#       the cross-bfd library (i.e. a BFD library that runs
#       on the host but knows how to interpret the target format).
##################################################################
#     
xsyms: xsyms.c xsyms-bfd.c
	@($(CC) -o $@ -I/usr/include/libelf xsyms.c -lelf && echo $@ built with libelf ) || ( echo 'build using libelf failed; trying libbfd' &&  $(CC) -o $@ xsyms-bfd.c -lbfd -liberty && echo $@ built with libbfd )

install:
	@echo You must install 'xsyms' manually
	exit 1

REVISION=$(filter-out $$%,$$Name$$)
tar: regexp prep
	@if [ -z $(REVISION) ] ; then \
		echo "I need a version checked out with a revision tag to make a tarball";\
		exit 1;\
    else \
		echo tarring revision $(REVISION);\
		tar cfz $(REVISION).tgz --exclude $(REVISION).tgz $(if $(wildcard tarexcl),-X tarexcl) -C .. $(shell basename `pwd`) ;\
    fi
