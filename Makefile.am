HOSTCC=@HOSTCC@
SUBDIRS=@subdirs@
lib_LIBRARIES=libcexp.a
##BUILT_SOURCES = builddate.h
SRCS = cexp.c ctyps.c bfdstuff.c cexpsyms.c vars.c ctyps.h  cexpsyms.h  cexpsymsP.h  cexpmod.h cexpmodP.h cexpmod.c vars.h cexp.tab.c cexp.tab.h @srcdir@/getopt/mygetopt_r.c @srcdir@/getopt/mygetopt_r.h

include_HEADERS = cexp.h


xsyms_LDADD=-Llibelf-0.8.0/lib -lelf

if !ISRTEMS
CEXPPROG=cexp
cexp_SOURCES=$(SRCS)
cexp_CFLAGS=-DNO_THREAD_PROTECTION -Dcexp_main=main
libcexp_a_CFLAGS=-DNO_THREAD_PROTECTION
##cexp_LDADD= -Llibelf-0.8.0/lib -lelf -Lregexp -lregexp
cexp_LDADD=-lbfd -liberty -Lregexp -lregexp
cexp_LDFLAGS=-Wl,-Bstatic
endif

bin_PROGRAMS=xsyms $(CEXPPROG)

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
