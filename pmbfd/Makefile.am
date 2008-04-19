AUTOMAKE_OPTIONS = foreign no-installinfo no-installman
lib_LIBRARIES    = libpmelf.a libpmbfd.a
libpmelf_a_SOURCES = symname.c secname.c putdat.c dmpgrps.c
libpmelf_a_SOURCES+=strm.c fstrm.c mstrm.c
libpmelf_a_SOURCES+=dmpsym.c dmpsymtab.c
libpmelf_a_SOURCES+=dmpshdr.c dmpshtab.c
libpmelf_a_SOURCES+=dmpehdr.c
libpmelf_a_SOURCES+=symtab.c
libpmelf_a_SOURCES+=findsymhdrs.c
libpmelf_a_SOURCES+=shtab.c
libpmelf_a_SOURCES+=getgrp.c
libpmelf_a_SOURCES+=getscn.c
libpmelf_a_SOURCES+=getsym.c
libpmelf_a_SOURCES+=putsym.c
libpmelf_a_SOURCES+=getshdr.c
libpmelf_a_SOURCES+=putshdr.c
libpmelf_a_SOURCES+=getehdr.c
libpmelf_a_SOURCES+=putehdr.c
if NO_64BIT
libpmelf_a_SOURCES+=noelf64.c
endif

libpmbfd_a_SOURCES=bfd.c
EXTRA_libpmbfd_a_SOURCES=$(wildcard $(srcdir)/bfd-reloc-*.c)
if HAVE_RELOCS
libpmbfd_a_LIBADD=$(RELOCS:%.c=%.$(OBJEXT))
libpmbfd_a_DEPENDENCIES=$(RELOCS:%.c=%.$(OBJEXT))
endif

include_HEADERS  = pmelf.h
