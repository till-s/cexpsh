HOSTCC=@HOSTCC@
SUBDIRS=@subdirs@
lib_LIBRARIES=libcexp.a
##BUILT_SOURCES = builddate.h

if USETECLA
TECLA_SRCS = teclastuff.c
TECLALIBP=-L@LIBTECLA@
TECLALIBS=-ltecla_r
TECLAINCS=-I@srcdir@/@LIBTECLA@
endif

if USEBFD
SYMF_SRCS = bfdstuff.c bfd-disas.c
SYMLIBP=-L@BINUTILS@/bfd/.libs -L@BINUTILS@/opcodes/.libs -L@BINUTILS@/libiberty
SYMLIBS=-lbfd -lopcodes -liberty
SYMINCS=-I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
else
SYMF_SRCS = elfsyms.c
SYMLIBP=-L@LIBELF@/lib
SYMLIBS=-lelf
SYMINCS=-I@LIBELF@/include
endif

SRCS = cexp.c ctyps.c cexpsyms.c vars.c ctyps.h  cexpsyms.h  cexpsymsP.h  cexpmod.h cexpmodP.h cexpmod.c vars.h cexp.tab.c cexp.tab.h @srcdir@/getopt/mygetopt_r.c @srcdir@/getopt/mygetopt_r.h context.h help.c $(SYMF_SRCS) $(TECLA_SRCS)

#MDBG=/home/till/slac/xfm/src/mdbg/mdbg.o
#MDBGCFLAGS=-DUSE_MDBG -I$(dir $(MDBG))
#MDBGLDFLAGS= $(MDBG) -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free
#BINUT=/home/till/gnu/binutils-2.11.1
#BFDINCS=-I$(BINUT)/include -I$(BINUT)/bfd -I$(BINUT)/$(ARCH)/bfd
#TECLA=/home/till/rtems/apps/libtecla
#ARCH=build-ppc-rtems
#BFDLIBP=-L $(BINUT)/$(ARCH)/bfd/.libs/ -L $(BINUT)/$(ARCH)/libiberty/ -L $(BINUT)/$(ARCH)/opcodes/.libs/


include_HEADERS = cexp.h cexpHelp.h

#CONFIGHACKS=-DHAVE_ELF_BFD_H -DHAVE_BFD_DISASSEMBLER -DUSE_TECLA

xsyms_LDADD=-Llibelf-0.8.0/lib -lelf

if !ISRTEMS
CONFIG_THREADS=-DNO_THREAD_PROTECTION
cexp_SOURCES=$(SRCS)
cexp_CFLAGS= $(CONFIGHACKS) -Dcexp_main=main $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)
#cexp_LDADD= -Llibelf-0.8.0/lib -lelf -Lregexp -lspencer_regexp
cexp_LDADD=$(SYMLIBS) $(TECLALIBS) -lspencer_regexp
cexp_LDFLAGS= $(SYMLIBP) $(TECLALIBP) -Lregexp $(MDBGLDFLAGS)
# $(BFDLIBP) -Wl,-Bstatic
bin_PROGRAMS=xsyms cexp
else
bin_PROGRAMS=
endif

libcexp_a_CFLAGS=$(CONFIGHACKS) $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)

libcexp_a_SOURCES=$(SRCS)
INCLUDES = -I@srcdir@/regexp

#we can't use automake for this because
# - we _need_ bison
# - our naming convention

cexpsyms.o: cexp.tab.h

cexp.tab.c cexp.tab.h: cexp.y
	bison -v -d -p cexp $^

jumptab.c: gentab
	./$^ > $@

gentab: gentab.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

%cexp.o: builddate.c

cexp.c: builddate.c

builddate.c: ALWAYS
	echo 'static char *cexp_build_date="'`date +%Y%m%d%Z%T`'";' > $@

.PHONY: ALWAYS

## gentab.c
## init.c
## xsyms.c
