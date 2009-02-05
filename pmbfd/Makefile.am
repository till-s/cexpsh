AUTOMAKE_OPTIONS = foreign no-installinfo no-installman

ACLOCAL_AMFLAGS=-I../m4

AM_CFLAGS=-Wall

lib_LIBRARIES    = libpmelf.a libpmbfd.a
libpmelf_a_SOURCES = pmelfP.h attrP.h
libpmelf_a_SOURCES+=symname.c secname.c putdat.c dmpgrps.c
libpmelf_a_SOURCES+=strm.c fstrm.c mstrm.c
libpmelf_a_SOURCES+=dmpsym.c dmpsymtab.c
libpmelf_a_SOURCES+=dmpshdr.c dmpshtab.c
libpmelf_a_SOURCES+=dmpehdr.c dmprels.c
libpmelf_a_SOURCES+=symtab.c
libpmelf_a_SOURCES+=findsymhdrs.c
libpmelf_a_SOURCES+=shtab.c
libpmelf_a_SOURCES+=getgrp.c
libpmelf_a_SOURCES+=getrel.c
libpmelf_a_SOURCES+=getscn.c
libpmelf_a_SOURCES+=getsym.c
libpmelf_a_SOURCES+=putsym.c
libpmelf_a_SOURCES+=getshdr.c
libpmelf_a_SOURCES+=putshdr.c
libpmelf_a_SOURCES+=getehdr.c
libpmelf_a_SOURCES+=putehdr.c
libpmelf_a_SOURCES+=attpbfasmatch.c
libpmelf_a_SOURCES+=attpbfasdestroy.c
libpmelf_a_SOURCES+=attpbfasread.c
libpmelf_a_SOURCES+=attpbfasprint.c
libpmelf_a_SOURCES+=attpbfaprint.c
libpmelf_a_SOURCES+=attpbprinttag.c
libpmelf_a_SOURCES+=attset.c
libpmelf_a_SOURCES+=attprint.c
libpmelf_a_SOURCES+=attvendfind.c
libpmelf_a_SOURCES+=guleb128.c getwrd.c
libpmelf_a_SOURCES+=att-gnu-powerpc.c
if NO_64BIT
libpmelf_a_SOURCES+=noelf64.c
endif

if BUILD_UTILS
bin_PROGRAMS=rdelf bfdod
rdelf_LDADD=-L. -lpmelf
rdelf_DEPENDENCIES=libpmelf.a
bfdod_LDADD=-L. -lpmbfd -lpmelf
bfdod_DEPENDENCIES=libpmelf.a libpmbfd.a
endif

if BUILD_TESTS
bin_PROGRAMS=test_attr
test_attr_LDADD=-L. -lpmelf
test_attr_DEPENDENCIES=libpmelf.a
endif

libpmbfd_a_SOURCES =bfd.c pmbfdP.h pmbfd.h
libpmbfd_a_SOURCES+=opcodesup.c
EXTRA_libpmbfd_a_SOURCES=$(wildcard $(srcdir)/bfd-reloc-*.c)
if HAVE_RELOCS
libpmbfd_a_LIBADD=$(RELOCS:%.c=%.$(OBJEXT))
libpmbfd_a_DEPENDENCIES=$(RELOCS:%.c=%.$(OBJEXT))
endif

include_HEADERS  = pmelf.h

EXTRA_DIST       = LICENSE makefile
