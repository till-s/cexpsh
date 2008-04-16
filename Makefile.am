AUTOMAKE_OPTIONS=foreign no-installinfo no-installman

HOSTCC=@HOSTCC@
SUBDIRS=@THESUBDIRS@

##BUILT_SOURCES = builddate.h

MOSTLYCLEANFILES =

XSYMS=xsyms

if USETECLA
TECLA_SRCS       = teclastuff.c
TECLALIBP        = -L@LIBTECLA@
TECLALIBS        = -ltecla_r
TECLAINCS        = -I@srcdir@/@LIBTECLA@
endif

if USEBFD
SYMF_SRCS        = bfdstuff.c
SYMLIBP          = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty
SYMLIBS          = -lbfd -liberty
if USEOPCODES
SYMF_SRCS       += bfd-disas.c
SYMLIBP         += -L@BINUTILS@/opcodes/.libs
SYMLIBS         += -lopcodes
endif
SYMINCS          = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
xsyms_SOURCES    = xsyms-bfd.c
xsyms_LDADD      = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty
xsyms_CPPFLAGS   = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
gencore_SOURCES  = gencore.c
gencore_LDADD    = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty
gencore_CPPFLAGS = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
else
if USEPMBFD
SYMF_SRCS        = bfdstuff.c
SYMLIBP          = -Lpmbfd
SYMLIBS          = -lpmbfd -lpmelf
SYMINCS          = -I$(srcdir)/pmbfd
xsyms_SOURCES    = xsyms-bfd.c
xsyms_LDADD      = -Lpmbfd -lpmbfd -lpmelf
xsyms_CPPFLAGS   = -I$(srcdir)/pmbfd
else
if USELIBELF
SYMF_SRCS        = elfsyms.c
SYMLIBP          = -Llibelf-local/lib
SYMLIBS          = -lelf
SYMINCS          = -Ilibelf-local/include
else
SYMF_SRCS        = noloader.c
endif
xsyms_SOURCES    = xsyms.c
if NATIVE_LIBELF
xsyms_LDADD      = -lelf
else
xsyms_LDADD      = -Llibelf-local/lib -lelf
xsyms_CPPFLAGS   = -Ilibelf-local/include
endif
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



#CONFIGHACKS=-DHAVE_ELF_BFD_H -DHAVE_BFD_DISASSEMBLER -DUSE_TECLA

if DO_CEXPLIB

lib_LIBRARIES=libcexp.a
include_HEADERS = cexp.h cexpHelp.h

libcexp_a_CFLAGS=$(CONFIGHACKS) $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)

libcexp_a_SOURCES=$(SRCS)
AM_CPPFLAGS = -I@srcdir@/regexp $(xsyms_CPPFLAGS)

libcexp_a_DEPENDENCIES=$(SYMDEPS)

if DO_DEMO
cexp_SOURCES=$(SRCS)
cexp_CFLAGS= $(CONFIGHACKS) -Dcexp_main=main $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)
#cexp_LDADD= -Llibelf-0.8.0/lib -lelf -Lregexp -lspencer_regexp
# '-static' might not be recognized by other compilers
cexp_LDFLAGS_static_gcc_yes=-static
cexp_LDADD=$(SYMLIBS) $(TECLALIBS) -lspencer_regexp $(wildcard cexp-builtin-symtab.$(OBJEXT))
cexp_LDFLAGS= $(cexp_LDFLAGS_static_gcc_@GCC@) $(SYMLIBP) $(TECLALIBP) -Lregexp $(MDBGLDFLAGS)
# $(BFDLIBP) -Wl,-Bstatic
bin_PROGRAMS=cexp
bin_PROGRAMS += xsyms
endif
else
AM_CPPFLAGS = $(xsyms_CPPFLAGS)
if USEBFD
bin_PROGRAMS=xsyms gencore
else
bin_PROGRAMS=xsyms
endif
endif


cexp-elfsyms.$(OBJEXT) libcexp_a-elfsyms.$(OBJEXT): install-libelf-local

if NATIVE_LIBELF
else
xsyms-xsyms.$(OBJEXT): install-libelf-local
endif

install-libelf-local:
	$(MAKE) -C @LIBELF@ prefix=`pwd`/libelf-local exec_prefix=`pwd`/libelf-local datadir=`pwd`/libelf-local/share install

remove-libelf-local:
	$(RM) -r libelf-local

clean-local: remove-libelf-local

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

MOSTLYCLEANFILES+=builddate.c

install: all-recursive install-recursive

builtin-symtab:	cexp-builtin-symtab.$(OBJEXT)

cexp-builtin-symtab.c:	cexp
	$(XSYMS) -C $^ $@

cexp-builtin-symtab.$(OBJEXT):	cexp-builtin-symtab.c
	$(CC) -c -I$(srcdir) $(CFLAGS) -o $@ $^

.PHONY: ALWAYS

## gentab.c
## init.c
## xsyms.c
