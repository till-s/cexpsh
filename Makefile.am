HOSTCC=@HOSTCC@
SUBDIRS=@subdirs@
lib_LIBRARIES=libcexp.a
##BUILT_SOURCES = builddate.h
SRCS = cexp.c ctyps.c bfdstuff.c cexpsyms.c vars.c ctyps.h  cexpsyms.h  cexpsymsP.h  cexpmod.h cexpmodP.h cexpmod.c vars.h cexp.tab.c cexp.tab.h @srcdir@/getopt/mygetopt_r.c @srcdir@/getopt/mygetopt_r.h bfd-disas.c context.h

#MDBG=/home/till/slac/xfm/src/mdbg/mdbg.o
#MDBGCFLAGS=-DUSE_MDBG -I$(dir $(MDBG))
#MDBGLDFLAGS= $(MDBG) -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free
BINUT=/home/till/gnu/binutils-2.11.1
BFDINCS=-I$(BINUT)/include -I$(BINUT)/bfd -I$(BINUT)/$(ARCH)/bfd
TECLA=/home/till/rtems/apps/libtecla
TECLAINCS=-I$(TECLA)
ARCH=build-ppc-rtems
BFDLIBP=-L $(BINUT)/$(ARCH)/bfd/.libs/ -L $(BINUT)/$(ARCH)/libiberty/ -L $(BINUT)/$(ARCH)/opcodes/.libs/
TECLALIBS=-L$(TECLA)/$(ARCH) -ltecla_r




include_HEADERS = cexp.h

CONFIGHACKS=-DHAVE_ELF_BFD_H -DHAVE_BFD_DISASSEMBLER -DUSE_TECLA

xsyms_LDADD=-Llibelf-0.8.0/lib -lelf

if !ISRTEMS
CEXPPROG=cexp
CONFIG_THREADS=-DNO_THREAD_PROTECTION
cexp_SOURCES=$(SRCS)
cexp_CFLAGS= $(CONFIGHACKS) -Dcexp_main=main $(MDBGCFLAGS) $(BFDINCS) $(CONFIG_THREADS) $(TECLAINCS)
#cexp_LDADD= -Llibelf-0.8.0/lib -lelf -Lregexp -lregexp
cexp_LDADD=-lbfd -lopcodes -liberty -Lregexp -lregexp $(TECLALIBS)
cexp_LDFLAGS= $(MDBGLDFLAGS) $(BFDLIBP)
#-Wl,-Bstatic
bin_PROGRAMS=xsyms $(CEXPPROG)
else
bin_PROGRAMS=
endif

libcexp_a_CFLAGS=$(CONFIGHACKS) $(MDBGCFLAGS) $(BFDINCS) $(CONFIG_THREADS) $(TECLAINCS)


libcexp_a_SOURCES=$(SRCS)
INCLUDES = -I@srcdir@/regexp -Ilibelf-0.8.0/include/libelf -Ilibelf-0.8.0/include -Ilibelf-0.8.0/lib

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
