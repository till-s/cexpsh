TOP=../

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

GENINC = geninc


USR_CFLAGS +=  -DUSE_EPICS_OSI -DHAVE_BFD_DISASSEMBLER -DUSE_TECLA

BINUT = /home/till/gnu/binutils-2.11.1/

USR_INCLUDES += -I$(BINUT)/include -I$(BINUT)/bfd -I$(BINUT)/O.$(T_A)/bfd -I/home/till/slac/cexp/regexp -I/home/till/rtems/apps/libtecla -I../getopt

#USR_LDFLAGS += -Wl,-M

INC +=

#=============================

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC +=
#DBD +=


# <name>.dbd will be created from <name>Include.dbd
#DBD += ecdr814.dbd

#=============================

#PROD_RTEMS = ecdr814App
LIBRARY += cexp

cexp_SRCS += bfd-disas.c bfdstuff.c cexp.c cexpmod.c cexpsyms.c ctyps.c vars.c cexp.tab.c mygetopt_r.c
PROD_IOC += cexpTst

cextTst_SRCS_DEFAULT = cexpTstMain.c

cexpTst_LIBS += cexp
cexpTst_LIBS += iocsh
cexpTst_LIBS += miscIoc
cexpTst_LIBS += rsrvIoc
cexpTst_LIBS += recIoc
cexpTst_LIBS += softDevIoc
cexpTst_LIBS += testDevIoc
cexpTst_LIBS += dbtoolsIoc
cexpTst_LIBS += asIoc
cexpTst_LIBS += dbIoc
cexpTst_LIBS += registryIoc
cexpTst_LIBS += dbStaticIoc
cexpTst_LIBS += ca
cexpTst_LIBS += Com

cexpTst_LIBS += tecla_r bfd opcodes iberty regexp

BINUTBIN = /home/till/gnu/binutils-2.11.1/O.$(T_A)
REGEXPLIBS = /home/till/slac/cexp/O.$(T_A)/regexp

cexpTst_LDFLAGS += -Wl,-Map,blahm -L$(TOP)/lib/$(T_A) -L. -L/home/till/rtems/apps/libtecla/O.$(T_A)/ -L$(BINUTBIN)/bfd/.libs -L$(BINUTBIN)/opcodes/.libs -L$(BINUTBIN)/libiberty/.libs  -L$(REGEXPLIBS)/  -Wl,-u,iocshRegister

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
