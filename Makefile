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
		echo ;\
		echo This Makefile must be invoked with an explicit target; \
		echo ;\
		echo Possible Targets are; \
		echo '         "prep": regenerate non-CVS/autoxxx files (requires bison-1.28 and autotools)'; \
		echo '    "distclean": remove non-CVS files (REQUIRES bison-1.28 AND autotools TO RECREATE)'; \
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
	$(HOSTCC) -O -o $@ $^

jumptab.c: gentab.c
	$(HOSTCC) -O -o gentab $^
	if ./gentab > $@; then true ; else $(RM) $@ gentab; exit 1; fi
	$(RM) gentab

src: cexp.tab.c cexp.tab.h jumptab.c

ifdef tools_prefix
# AUTOPATH = $(tools_prefix)/
# must use the real search path; providing absolute path
# when calling autotools doesn't work because automake seems
# to call 'autoconf'.
PATH:= $(tools_prefix):$(PATH)
endif

ACLOCAL    = $(AUTOPATH)aclocal -I$(shell pwd)/m4
AUTOCONF   = $(AUTOPATH)autoconf 
AUTOHEADER = $(AUTOPATH)autoheader
AUTOMAKE   = $(AUTOPATH)automake

bootstrap-xsyms bootstrap-cexp:
	$(ACLOCAL) && $(AUTOCONF) && $(AUTOHEADER) && $(AUTOMAKE) -ac$(AUTOFORCE)
#	ln -s binutils binutils-x
bootstrap: bootstrap-cexp
	(cd libtecla; $(ACLOCAL) && $(AUTOCONF)   && $(AUTOMAKE) -ac$(AUTOFORCE))
	(cd regexp;   $(ACLOCAL) && $(AUTOCONF)   && $(AUTOMAKE) -ac$(AUTOFORCE))
	(cd pmbfd;    $(ACLOCAL) && $(AUTOHEADER) && $(AUTOCONF) && $(AUTOMAKE) -ac$(AUTOFORCE))


AUTOFILES=Makefile.in aclocal.m4 config.guess config.sub
AUTOFILES+=depcomp install-sh missing compile configure config.h.in
AUTODIRS=autom4te.cache



distclean:
	$(RM) gentab cexp.output cexp.tab.c cexp.tab.h
	$(RM) gentab jumptab.c 
	$(RM) makefile.top.am makefile.top.in
	$(RM) $(AUTOFILES)
	$(RM) -r $(AUTODIRS)
	$(RM) $(addprefix libtecla/,$(AUTOFILES))
	$(RM) -r $(addprefix libtecla/,$(AUTODIRS))
	$(RM) $(addprefix pmbfd/,$(AUTOFILES))
	$(RM) -r $(addprefix pmbfd/,$(AUTODIRS))
	$(RM) $(addprefix regexp/,$(AUTOFILES))
	$(RM) -r $(addprefix regexp/,$(AUTODIRS))

HOSTCC:=$(CC)
