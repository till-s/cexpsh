#
#  $Id$
#
# Templates/Makefile.lib
#       Template library Makefile
#
# Set this to YES or NO if you want / dont want to use GNU readline
USE_READLINE=YES

USE_READLINE_YES=-DUSE_GNU_READLINE

LIBNAME=libcexp.a
LIB=${ARCH}/${LIBNAME}

VPATH=.:getopt

# C and C++ source names, if any, go here -- minus the .c or .cc
C_PIECES=elfsyms cexpsyms vars cexp ctyps cexp.tab mygetopt_r
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

LIB_HEADERS=cexp.h

H_FILES=${LIB_HEADERS} cexpsyms.h vars.h ctyps.h cexp.tab.h

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/lib.cfg

#
# Add local stuff here using +=
#

DEFINES  += -DYYDEBUG -DCONFIG_STRINGS_LIVE_FOREVER $(USE_READLINE_$(USE_READLINE))
CPPFLAGS += -I/usr/local/rtems/powerpc-rtems/include
CFLAGS   +=

# need bison to generate a reentrant parser
YACC=bison

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS += 
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(LIB)

%.tab.c %.tab.h: %.y
	bison -d -p $(^:%.y=%) $^
# remove the default rule which tries to make cexp.c from cexp.y
cexp.c: ;

$(LIB): ${OBJS}
	$(make-library)

# Install the library, appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
install:  all
	$(INSTALL_VARIANT) -m 644 ${LIB} ${PROJECT_RELEASE}/lib
	$(INSTALL_CHANGE) -m 644 ${LIB_HEADERS} ${PROJECT_RELEASE}/lib/include
