#
#  $Id$
#
# Templates/Makefile.leaf
# 	Template leaf node Makefile
#

APP=cexp
# APP_BSP = mvme2307
APP_BSP = svgm5

LIBELF_PATH=/home/till/rtems/libelf-0.8.0/
LIBREADLINE_PATH=/home/till/rtems/apps/add_ons/readline-4.2/
REGEXP_PATH=/home/till/slac/xfm/regexp
LIBTERMCAP=/home/till/rtems/apps/termcap-1.3/build-powerpc-rtems -ltermcap
#/home/till/rtems/apps/add_ons/ncurses-5.2/lib -lncurses
#/home/till/rtems/apps/termcap-1.3/build-powerpc-rtems -ltermcap


RTEMS_MAKEFILE_PATH=$(PROJECT_ROOT)/powerpc-rtems/$(APP_BSP)/
RTMSLIBDIR=$(RTEMS_MAKEFILE_PATH)/lib


# C source names, if any, go here -- minus the .c
C_PIECES=init wrap cexp.tab elfsyms ce vars
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# C++ source names, if any, go here -- minus the .cc
CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

PGMS=${ARCH}/${APP}
PGM=${ARCH}/${APP}

# List of RTEMS managers to be included in the application goes here.
# Use:
#     MANAGERS=all
# to include all RTEMS managers in the application.
#MANAGERS=io event message rate_monotonic semaphore timer, etc.
#MANAGERS=io event semaphore timer
MANAGERS=all

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

#
# (OPTIONAL) Add local stuff here using +=
#

DEFINES  +=
CPPFLAGS += -I$(LIBELF_PATH)/lib -I$(LIBELF_PATH)/build-powerpc-rtems/lib
CPPFLAGS += -I$(REGEXP_PATH) -I$(LIBREADLINE_PATH)
CFLAGS   += -O2 -Winline

#
# CFLAGS_DEBUG_V are used when the `make debug' target is built.
# To link your application with the non-optimized RTEMS routines,
# uncomment the following line:
# CFLAGS_DEBUG_V += -qrtems_debug
#

# TS: this didn't work :-( LD_PATHS  +=
LD_LIBS   += -lm -L$(LIBELF_PATH)/build-powerpc-rtems/lib -lelf
LD_LIBS	  += -L$(REGEXP_PATH)/${ARCH} -lregexp
LD_LIBS   += -L$(LIBREADLINE_PATH)/${ARCH} -lreadline
LD_LIBS   += -L$(LIBTERMCAP)
# ../drivers/${ARCH}/libdrv.a
LDFLAGS   +=
#-Wl,--wrap=lseek -Wl,--wrap=mmap -Wl,--wrap=munmap -Wl,--wrap=ftruncate

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS += cexp.tab.c cexp.tab.h
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(PGMS)

#${ARCH}/${APP}.bootimg

${ARCH}/${APP}: ${OBJS} ${LINK_FILES}
	$(make-exe)
#	$(CC) $(CFLAGS) -o $@ $(LINK_OBJS) $(LINK_LIBS)

${ARCH}/rtems.gz: ${ARCH}/${APP}
	$(RM) $@ ${ARCH}/rtems
	$(OBJCOPY) ${ARCH}/${APP} ${ARCH}/rtems -O binary -R .comment -S
	gzip -vf9 ${ARCH}/rtems

${ARCH}/${APP}.bootimg: ${ARCH}/${APP} ${ARCH}/rtems.gz
	(cd ${ARCH} ; $(LD) -o $(APP).bootimg $(RTMSLIBDIR)/bootloader.o --just-symbols=${APP} -bbinary rtems.gz -T$(RTMSLIBDIR)/ppcboot.lds -Map ${APP}.map)

%.tab.c %.tab.h: %.y
	bison -d $^

# Install the program(s), appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
install:  all
	$(INSTALL_VARIANT) -m 555 ${PGMS} ${PROJECT_RELEASE}/bin
# DO NOT DELETE
elfsyms.o: elfsyms.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stdarg.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/va-ppc.h \
 /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
 /usr/include/bits/sched.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/wchar.h \
 /usr/include/bits/wchar.h /usr/include/gconv.h \
 /usr/include/bits/stdio_lim.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/libelf/libelf.h /usr/include/libelf/sys_elf.h \
 /usr/include/elf.h /usr/include/stdint.h /usr/include/bits/wordsize.h \
 /usr/include/assert.h /usr/include/regexp.h /usr/include/alloca.h \
 /usr/include/regex.h /usr/include/stdlib.h /usr/include/string.h \
 cexp.h ctyps.h cexp.tab.h
cexp.tab.o: cexp.tab.c /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stdarg.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/va-ppc.h \
 /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
 /usr/include/bits/sched.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/wchar.h \
 /usr/include/bits/wchar.h /usr/include/gconv.h \
 /usr/include/bits/stdio_lim.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/assert.h /usr/include/stdlib.h /usr/include/alloca.h \
 /usr/include/ctype.h /usr/include/string.h cexp.h ctyps.h vars.h
vars.o: vars.c vars.h ctyps.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stdarg.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/va-ppc.h \
 /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stddef.h \
 /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
 /usr/include/bits/sched.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/wchar.h \
 /usr/include/bits/wchar.h /usr/include/gconv.h \
 /usr/include/bits/stdio_lim.h /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
 /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h
ctyps.o: ctyps.c /usr/include/assert.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/include/stdlib.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stddef.h \
 /usr/include/sys/types.h /usr/include/bits/types.h \
 /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/alloca.h ctyps.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stdarg.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/va-ppc.h \
 /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h \
 /usr/include/bits/stdio_lim.h
ce.o: ce.c cexp.h /usr/include/stdio.h /usr/include/features.h \
 /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stddef.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/stdarg.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/va-ppc.h \
 /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
 /usr/include/bits/sched.h /usr/include/libio.h \
 /usr/include/_G_config.h /usr/include/wchar.h \
 /usr/include/bits/wchar.h /usr/include/gconv.h \
 /usr/include/bits/stdio_lim.h ctyps.h cexp.tab.h /usr/include/fcntl.h \
 /usr/include/bits/fcntl.h /usr/include/sys/types.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/unistd.h /usr/include/bits/posix_opt.h \
 /usr/include/bits/confname.h /usr/include/getopt.h \
 /usr/include/stdlib.h /usr/include/alloca.h \
 /usr/include/readline/readline.h /usr/include/readline/rlstdc.h \
 /usr/include/readline/keymaps.h /usr/include/readline/chardefs.h \
 /usr/include/ctype.h /usr/include/string.h \
 /usr/include/readline/tilde.h /usr/include/readline/history.h \
 /usr/include/regexp.h /usr/include/regex.h /usr/include/math.h \
 /usr/include/bits/huge_val.h /usr/include/bits/mathdef.h \
 /usr/include/bits/mathcalls.h \
 /usr/lib/gcc-lib/powerpc-suse-linux/2.95.2/include/float.h
