AUTOMAKE_OPTIONS=foreign no-installinfo no-installman

ACLOCAL_AMFLAGS=-I./m4

# make sure any third-party makefile
# inherits these settings which
# may be embedded in installation paths...
# (This is some hack)
export rtems_bsp
export RTEMS_BSP
export enable_rtemsbsp
export package_subdir
export host_alias
export host_cpu
export host_os

HOSTCC  = @HOSTCC@
SUBDIRS = @THESUBDIRS@
HOSTCFLAGS=-O2 -g

MOSTLYCLEANFILES =
CLEANFILES       =

if USETECLA
TECLA_SRCS       = teclastuff.c
TECLALIBP        = -L@LIBTECLA@
TECLALIBS        = -ltecla_r
TECLAINCS        = -I@srcdir@/@LIBTECLA@
endif

SYMF_SRCS        =
SYMLIBP          =
SYMLIBS          =
SYMINCS          =

if USELOADER
SYMF_SRCS       += bfdstuff.c
else
if USEELFSYMS
# ELF symbol file reader only
SYMF_SRCS       += elfsyms.c
else
# no symbol file reader at all,
# builtin symtab only
SYMF_SRCS       += noloader.c
endif
endif #USELOADER

if USEOPCODES
SYMF_SRCS       += bfd-disas.c
endif


if USEBFD
SYMINCS         += -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
SYMLIBP         += -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty

if USEOPCODES
SYMLIBS         += -lopcodes
SYMLIBP         += -L@BINUTILS@/opcodes/.libs
endif
SYMLIBS         += -lbfd
SYMLIBS         += -liberty

xsyms_CPPFLAGS   = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
xsyms_LDADD      = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty
gencore_CPPFLAGS = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
gencore_LDADD    = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty

else # USEBFD
SYMINCS         += -I$(srcdir)/pmbfd
SYMLIBP         += -Lpmbfd

if USEPMBFD

if USEOPCODES
SYMINCS         += -I$(srcdir)/@BINUTILS@/include -I@BINUTILS@/bfd
SYMLIBP         += -L@BINUTILS@/opcodes/.libs -L@BINUTILS@/libiberty
SYMLIBS         += -lopcodes -liberty -lpmbfd
else
if USELOADER
SYMLIBS         += -lpmbfd
endif
endif
SYMLIBS         += -lpmelf

endif #USEPMBFD

xsyms_CPPFLAGS   = -I$(srcdir)/pmbfd
xsyms_LDADD      = -Lpmbfd -lpmbfd -lpmelf
endif #USEBFD

if USELOADER
SEGS_SRCS        = $(SEGS_CPU_SRC) cexpsegs.c
endif

SRCS = cexp.c ctyps.c cexpsyms.c vars.c cexplock.h ctyps.h  cexpsyms.h  cexpsymsP.h
SRCS+= cexpmod.h cexpmodP.h cexpmod.c vars.h cexp.tab.c cexp.tab.h
SRCS+= cexpsegsP.h cexp_regex.h teclastuff.h rtems-hackdefs.h
SRCS+= @srcdir@/getopt/mygetopt_r.c @srcdir@/getopt/mygetopt_r.h context.h
SRCS+= help.c

EXTRA_SRCS=
EXTRA_SRCS+= cexpsegs.c cexpsegs-powerpc-rtems.c cexpsegs-dflt.c
EXTRA_SRCS+= $(SYMF_SRCS)
EXTRA_SRCS+= $(TECLA_SRCS)

EXTRA_DIST = cexp.y binutils-2.18.diff jumptab.c gentab.c config-ml.in
EXTRA_DIST+= README.ld.snippet Makefile LICENSE Makefile.syms
EXTRA_DIST+= wrap.c
EXTRA_DIST+= cpptest
EXTRA_DIST+= makefile.top.am makefile.top.in

xsyms_SOURCES    = xsyms.c
gencore_SOURCES  = gencore.c

#MDBG=/home/till/slac/xfm/src/mdbg/mdbg.o
#MDBGCFLAGS=-DUSE_MDBG -I$(dir $(MDBG))
#MDBGLDFLAGS= $(MDBG) -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free
#BINUT=/home/till/gnu/binutils-2.11.1
#BFDINCS=-I$(BINUT)/include -I$(BINUT)/bfd -I$(BINUT)/$(ARCH)/bfd
#TECLA=/home/till/rtems/apps/libtecla
#ARCH=build-ppc-rtems
#BFDLIBP=-L $(BINUT)/$(ARCH)/bfd/.libs/ -L $(BINUT)/$(ARCH)/libiberty/ -L $(BINUT)/$(ARCH)/opcodes/.libs/

XTRAOBJS = $(SEGS_SRCS:%.c=%.$(OBJEXT))
XTRAOBJS+= $(SYMF_SRCS:%.c=%.$(OBJEXT))
XTRAOBJS+= $(TECLA_SRCS:%.c=%.$(OBJEXT))
XTRAOBJS+= builddate.$(OBJEXT)

if DO_CEXPLIB

lib_LIBRARIES=libcexp.a
include_HEADERS = cexp.h cexpHelp.h ctyps.h cexpsyms.h


libcexp_a_SOURCES=$(SRCS)
EXTRA_libcexp_a_SOURCES=$(EXTRA_SRCS)

AM_CPPFLAGS   = -I@srcdir@/regexp
AM_CPPFLAGS  += $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)

libcexp_a_LIBADD      = $(XTRAOBJS)
libcexp_a_DEPENDENCIES=$(SYMDEPS) $(XTRAOBJS) builddate.o

if DO_DEMO
cexp_SOURCES  = cexp_main.c
# '-static' might not be recognized by other compilers
cexp_LDFLAGS_static_gcc_yes=-static
cexp_LDADD    = -lcexp $(SYMLIBS) $(TECLALIBS) -lspencer_regexp $(wildcard cexp-builtin-symtab.$(OBJEXT))
cexp_DEPENDENCIES=libcexp.a $(wildcard cpptest/mak.defs)  $(wildcard cexp-builtin-symtab.$(OBJEXT))
cexp_LDFLAGS  = -L. -Wl,-u,printf -Wl,-u,fprintf -Wl,-u,scanf -Wl,-u,fscanf $(CEXP_TEST_UNDEFS) $(cexp_LDFLAGS_static_gcc_@GCC@) $(SYMLIBP) $(TECLALIBP) -Lregexp $(MDBGLDFLAGS)
CEXP_TEST_UNDEFS=-Wl,-u,__cxa_finalize
-include cpptest/mak.defs
bin_PROGRAMS  = cexp
bin_PROGRAMS += xsyms
cexp_LINK     = $(CXX) $(AM_CFLAGS) $(CFLAGS) $(cexp_LDFLAGS) $(LDFLAGS) -o $@
endif #DO_DEMO
else #DO_CEXPLIB
# cross-tools only
gencore_CPPFLAGS = $(xsyms_CPPFLAGS)
if USEBFD
hostbin_PROGRAMS=xsyms gencore
else
hostbin_PROGRAMS=xsyms
endif
endif #DO_CEXPLIB

XSYMS=$(builddir)/xsyms

#we can't use automake for this because
# - we _need_ bison
# - our naming convention

cexpsyms.o: cexp.tab.h cexpsyms.c

cexp.tab.c cexp.tab.h: cexp.y
	bison -v -d -p cexp $^

%ctyps.o: $(srcdir)/jumptab.c

$(srcdir)/jumptab.c: $(srcdir)/gentab.c
	$(HOSTCC) $(HOSTCFLAGS) -o gentab $^
	./gentab > $@

CLEANFILES+=gentab


builddate.c: $(libcexp_a_SOURCES) $(EXTRA_SRCS) Makefile
	echo 'char *cexp_build_date="'`date +%Y%m%d%Z%T`'";' > $@

MOSTLYCLEANFILES+=builddate.c

install: all-recursive install-recursive

builtin-symtab:	cexp-builtin-symtab.$(OBJEXT)

cexp-builtin-symtab.c: cexp
	$(XSYMS) -C $^ $@

cexp-builtin-symtab.$(OBJEXT):	cexp-builtin-symtab.c
	$(CC) -c -I$(srcdir) $(CFLAGS) -o $@ $^

cexp_main.c:
	@echo '#include "cexp.h"' > $@
	@echo 'int main(int argc, char **argv) { return cexp_main(argc,argv); }' >>$@

dist-hook:
	if test "$(PACKAGE_VERSION)" = "untagged" ; then \
		echo "Need tagged version to cut a dist!";   \
		exit 1;                                      \
	fi
