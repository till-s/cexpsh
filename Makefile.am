HOSTCC=@HOSTCC@
SUBDIRS=@subdirs@
lib_LIBRARIES=libcexp.a
SRCS = cexp.c ctyps.c elfsyms.c vars.c ctyps.h  elfsyms.h  vars.h cexp.tab.c cexp.tab.h @srcdir@/getopt/mygetopt_r.c @srcdir@/getopt/mygetopt_r.h

include_HEADERS = cexp.h

if !ISRTEMS
bin_PROGRAMS=cexp
cexp_SOURCES=$(SRCS)
cexp_CFLAGS=-DNO_THREAD_PROTECTION -Dcexp_main=main
libcexp_a_CFLAGS=-DNO_THREAD_PROTECTION
cexp_LDADD=-Llibelf-0.8.0/lib -lelf -Lregexp -lregexp
endif


libcexp_a_SOURCES=$(SRCS)
INCLUDES = -I@srcdir@/regexp -Ilibelf-0.8.0/include -Ilibelf-0.8.0/lib

#we can't use automake for this because
# - we _need_ bison
# - our naming convention

elfsyms.o: cexp.tab.h

cexp.tab.c cexp.tab.h: cexp.y
	bison -v -d -p cexp $^

jumptab.c: gentab
	./$^ > $@

gentab: gentab.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

xsyms:  xsyms.c
	$(HOSTCC) $(CFLAGS) $(INCLUDES) -o $@ $^


## gentab.c
## init.c
## xsyms.c
