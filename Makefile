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
		echo '         "host": Build DEMO CEXP to run on the host (--> host-arch == target-arch )';\
		echo ;\
		echo '     "rtemsbsp": Cross-build CEXP for a specific RTEMS BSP. The environment variable';\
		echo '                 RTEMS_MAKEFILE_PATH must point to the directory where the';\
		echo '                 specific BSPs Makefile.inc is located.';\
		echo ;\
		echo '   "cross-ARCH": Cross-build CEXP to run on ARCH; build "xsyms"-tool to run on host,';\
		echo '                 generating symbol files for ARCH.';\
		echo '                 You may specify additional "configure" options on the command line (see example).';\
		echo '                 NOTES: "ARCH-gcc" must be in your PATH!';\
		echo '                        if "configure" fails, you might have to "make clean" and reconfigure';\
		echo ;\
		echo '                 EXAMPLE: make cross-powerpc-rtems TGT_CONFIG_OPTS="--prefix=/opt/rtems --with-multisubdir=m750"';\
		echo ;\
		echo ' "install-ARCH": Install cross build.';\
		echo '                 NOTES: You must install "build-X-ARCH/xsyms" manually to your cross-tool executable directory';\
		echo '                        (possibly renaming it "ARCH-xsyms").'; \
		echo '                        Installdir for CEXP is set by --prefix at configure time (TGT_CONFIG_OPTS above) or by';\
		echo '                        passing "prefix=DIR" now. Default installation is to /usr/local.';\
		echo ;\
		echo '                 EXAMPLE: make install-powerpc-rtems prefix=/opt/rtems';\
		echo ;\
		echo '        "clean": remove all "build-XXX" directories';\
		echo '   "clean-ARCH": remove "build-ARCH" and "build-X-ARCH" directories';\
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

tar: AUTOFORCE=f

bootstrap-xsyms bootstrap-cexp:
	$(ACLOCAL) && $(AUTOCONF) && $(AUTOHEADER) && $(AUTOMAKE) -ac$(AUTOFORCE)
#	ln -s binutils binutils-x
bootstrap: bootstrap-cexp
	(cd libtecla; $(AUTOCONF))
	(cd regexp;   $(ACLOCAL) && $(AUTOCONF) && $(AUTOMAKE) -ac$(AUTOFORCE))

clean-xsyms:
	$(RM) xsyms

clean: clean-xsyms
	$(RM) -r build-*

clean-%:
	$(RM) -r build-$* build-X-$*

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
	@($(HOSTCC) -o $@ -I/usr/include/libelf xsyms.c -lelf && echo $@ built with libelf ) || ( echo 'build using libelf failed; trying libbfd' &&  $(CC) -o $@ xsyms-bfd.c -lbfd -liberty && echo $@ built with libbfd )

install:
	@echo You must install 'xsyms' manually
	exit 1

host:
	if [ ! -d build-host ] ; then mkdir build-host; fi
	( cd build-host ; ../configure $(TGT_CONFIG_OPTS) --disable-nls )
	$(MAKE) -C build-host

HOSTCC:=$(CC)

#if RTEMS_MAKEFILE_PATH is set then 
#build for a particular BSP
ifdef RTEMS_MAKEFILE_PATH
include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(CONFIG.CC)
CFLAGARG="CFLAGS=$(CPU_CFLAGS)"
ifndef RTEMS_SITE_INSTALLDIR
#traditional RTEMS install
ifeq ($(filter,--prefix,$(TGT_CONFIG_OPTS))xx,xx)
PREFIXARG=--prefix=$(PROJECT_ROOT)
endif
ifeq ($(filter,--exec-prefix,$(TGT_CONFIG_OPTS))xx,xx)
EXCPREFIXARG=--exec-prefix='$(prefix)/$(RTEMS_CPU)-rtems/$(RTEMS_BSP)'
endif
ifeq ($(filter,--includedir,$(TGT_CONFIG_OPTS))xx,xx)
INCDIRARG=--includedir='$(prefix)/$(RTEMS_CPU)-rtems/$(RTEMS_BSP)/lib/include'
endif
endif
rtemsbsp: cross-$(RTEMS_CPU)-rtems
	@echo or 'make rtemsbsp-install'
rtemsbsp-install: rtemsbsp install-$(RTEMS_CPU)-rtems
else
rtemsbsp%:
	$(error you must set RTEMS_MAKEFILE_PATH=<dir where Makefile.inc of your BSP lives> either on the commandline or in the environment)
endif


cross-%:
	@if [ ! -d build-$* ] ; then \
		mkdir build-$*; \
		echo CONFIGURING FOR CROSS BUILD TO ARCHITECTURE $*; \
		( cd build-$*; ../configure CC=$*-gcc $(CFLAGARG) --host=$* $(TGT_CONFIG_OPTS) --disable-nls --disable-multilib --with-newlib $(PREFIXARG) $(EXCPREFIXARG) $(INCDIRARG) ); \
	fi
	@echo MAKING CROSS BUILD TO ARCHITECTURE $*
	$(MAKE) -C build-$*
	@if [ ! -d build-X-$* ] ; then \
		mkdir build-X-$*; \
		echo CONFIGURING CROSS xsyms UTILITY FOR TARGET $*; \
		( cd build-X-$*; ../configure CC=$(HOSTCC) --target=$* --disable-nls ); \
	fi
	@echo MAKING xsyms FOR TARGET $*
	$(MAKE) -C build-X-$*
	@echo 'DONE you should install build-X-$*/xsyms manually to <dir>/$*-xsyms'
	@echo to install CEXP, run 'make install-$*'

	
install-%:
	$(MAKE) -C build-$* install

REVISION=$(filter-out $$%,$$Name$$)
tar: regexp prep
	@if [ -z $(REVISION) ] ; then \
		echo "I need a version checked out with a revision tag to make a tarball";\
		exit 1;\
    else \
		echo tarring revision $(REVISION);\
		tar cfz $(REVISION).tgz --exclude $(REVISION).tgz $(if $(wildcard tarexcl),-X tarexcl) -C .. $(shell basename `pwd`) ;\
    fi
