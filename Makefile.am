AUTOMAKE_OPTIONS=foreign no-installinfo no-installman

HOSTCC  = @HOSTCC@
SUBDIRS = @THESUBDIRS@

MOSTLYCLEANFILES =

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
if USEPMBFD
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

SYMLIBS         += -lbfd
if USEOPCODES
SYMLIBS         += -lopcodes
endif
SYMLIBS         += -liberty

xsyms_CPPFLAGS   = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
xsyms_LDADD      = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty
gencore_CPPFLAGS = -I@srcdir@/@BINUTILS@/include -I@srcdir@/@BINUTILS@/bfd -I@BINUTILS@/bfd
gencore_LDADD    = -L@BINUTILS@/bfd/.libs -L@BINUTILS@/libiberty -lbfd -liberty

else # USEBFD
SYMINCS         += -I$(srcdir)/pmbfd
SYMLIBP         += -Lpmbfd

if USEOPCODES
SYMINCS         += -I$(srcdir)/@BINUTILS@/include -I@BINUTILS@/bfd
SYMLIBP         += -L@BINUTILS@/opcodes/.libs -L@BINUTILS@/libiberty
SYMLIBS         += -lopcodes -liberty -lpmbfd
else
if USELOADER
SYMLIBS         += -lpmbfd
endif
endif

if USEPMBFD
SYMLIBS         += -lpmelf
endif

xsyms_CPPFLAGS   = -I$(srcdir)/pmbfd
xsyms_LDADD      = -Lpmbfd -lpmbfd -lpmelf
endif #USEBFD

SRCS = cexp.c ctyps.c cexpsyms.c vars.c ctyps.h  cexpsyms.h  cexpsymsP.h
SRCS+= cexpmod.h cexpmodP.h cexpmod.c vars.h cexp.tab.c cexp.tab.h
SRCS+= @srcdir@/getopt/mygetopt_r.c @srcdir@/getopt/mygetopt_r.h context.h
SRCS+= help.c $(SYMF_SRCS) $(TECLA_SRCS)

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


if DO_CEXPLIB

lib_LIBRARIES=libcexp.a
include_HEADERS = cexp.h cexpHelp.h

libcexp_a_CFLAGS=$(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)

libcexp_a_SOURCES=$(SRCS)
AM_CPPFLAGS   = -I@srcdir@/regexp $(xsyms_CPPFLAGS)

libcexp_a_DEPENDENCIES=$(SYMDEPS)

if DO_DEMO
cexp_SOURCES  =$(SRCS)
cexp_CFLAGS   = -Dcexp_main=main $(MDBGCFLAGS) $(SYMINCS) $(CONFIG_THREADS) $(TECLAINCS)
# '-static' might not be recognized by other compilers
cexp_LDFLAGS_static_gcc_yes=-static
cexp_LDADD    =$(SYMLIBS) $(TECLALIBS) -lspencer_regexp $(wildcard cexp-builtin-symtab.$(OBJEXT))
cexp_LDFLAGS  = $(cexp_LDFLAGS_static_gcc_@GCC@) $(SYMLIBP) $(TECLALIBP) -Lregexp $(MDBGLDFLAGS)
bin_PROGRAMS  = cexp
bin_PROGRAMS += xsyms
endif #DO_DEMO
else #DO_CEXPLIB
# cross-tools only
AM_CPPFLAGS = $(xsyms_CPPFLAGS)
if USEBFD
bin_PROGRAMS=xsyms gencore
else
bin_PROGRAMS=xsyms
endif
endif

XSYMS=xsyms

#we can't use automake for this because
# - we _need_ bison
# - our naming convention

cexpsyms.o: cexp.tab.h cexpsyms.c

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
