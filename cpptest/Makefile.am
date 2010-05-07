AUTOMAKE_OPTIONS=foreign

ACLOCAL_AMFLAGS=-I../m4

MOSTLYCLEANFILES =
CLEANFILES       =

EXEEXT           = .obj

bin_PROGRAMS     = com1 com2 lot1 lot2 lot3 ctdt cppe
dist_bin_SCRIPTS = $(srcdir)/st.test
noinst_SCRIPTS   = mak.defs

com1_SOURCES     = com_test.c
com1_CPPFLAGS    = -DCOM1

com2_SOURCES     = com_test.c
com2_CPPFLAGS    = -DCOM2

lot1_SOURCES     = linkonce_test.cc
lot1_CPPFLAGS    = -DLINKONCE_INST=1

lot2_SOURCES     = linkonce_test.cc
lot2_CPPFLAGS    = -DLINKONCE_INST=2

lot3_SOURCES     = linkonce_test.cc

ctdt_SOURCES     = cpptest0.cc

cppe_SOURCES     = cpptest1.cc

LINK             = $(LD) -r -o $@
CXXLINK          = $(LINK)

clump.rel:	$(bin_PROGRAMS)
	$(RM) $@
	$(CXXLINK) $^

mak.defs: clump.rel
	$(RM) $@
	$(NM) -u $^ | sed -n -e '/^[ \t]*U[ \t]/s/^[ \t]*U[ \t]/CEXP_TEST_UNDEFS+=-Wl,-u,/gp' > $@

CLEANFILES      += mak.defs clump.rel
