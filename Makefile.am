AUTOMAKE_OPTIONS=foreign no-installinfo no-installman

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
xsyms_SOURCES=xsyms-bfd.c
xsyms_LDADD=-L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty
xsyms_INCLUDES=-I@BINUTILS@/bfd -I@srcdir@/@BINUTILS@/include
else
SYMF_SRCS = elfsyms.c
SYMLIBP=-Llibelf-local/lib
SYMLIBS=-lelf
SYMINCS=-Ilibelf-local/include
xsyms_SOURCES=xsyms.c
if NATIVE_LIBELF
xsyms_LDADD=-lelf
else
xsyms_LDADD=-Llibelf-local/lib -lelf
xsyms_INCLUDES=-Ilibelf-local/include
endif
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


if !ISRTEMS
CONFIG_THREADS=-DNO_THREAD_PROTECTION
endif

if DO_CEXPLIB

libcexp_a_CFLAGS=$(CONFIGHACKS) $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)

libcexp_a_SOURCES=$(SRCS)
INCLUDES = -I@srcdir@/regexp

libcexp_a_DEPENDENCIES=$(SYMDEPS)

if DO_DEMO
cexp_SOURCES=$(SRCS)
cexp_CFLAGS= $(CONFIGHACKS) -Dcexp_main=main $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)
#cexp_LDADD= -Llibelf-0.8.0/lib -lelf -Lregexp -lspencer_regexp
cexp_LDADD=$(SYMLIBS) $(TECLALIBS) -lspencer_regexp
cexp_LDFLAGS= $(SYMLIBP) $(TECLALIBP) -Lregexp $(MDBGLDFLAGS)
# $(BFDLIBP) -Wl,-Bstatic
bin_PROGRAMS=cexp xsyms
endif
else
bin_PROGRAMS=xsyms
endif


cexp-elfsyms.o libcexp_a-elfsyms.o: install-libelf-local

if NATIVE_LIBELF
else
xsyms.o: install-libelf-local
endif

install-libelf-local:
	$(MAKE) -C @LIBELF@ prefix=`pwd`/libelf-local install

remove-libelf-local:
	$(RM) -r libelf-local

clean: remove-libelf-local	

#we can't use automake for this because
# - we _need_ bison
# - our naming convention

cexpsyms.o: cexp.tab.h

cexp.tab.c cexp.tab.h: cexp.y
	bison -v -d -p cexp $^

%ctyps.o: jumptab.c

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
