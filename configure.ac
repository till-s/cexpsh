AC_INIT(cexp, 1.0, <strauman@slac.stanford.edu>)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(cexp, 1.0)
AM_MAINTAINER_MODE

dnl if the versions are changed, the AC_CONFIG_SUBDIRS arguments
dnl below need to be adapted :-(
LIBELF=libelf-0.8.2
AC_SUBST(LIBELF)
BINUTILS=binutils
AC_SUBST(BINUTILS)
LIBTECLA=libtecla
AC_SUBST(LIBTECLA)

AC_PROG_CC
AC_SUBST(GCC)
AC_PROG_CPP
AC_CHECK_PROGS(HOSTCC, gcc cc)
AC_CHECK_TOOL(AR,ar)
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_ARG_ENABLE(cexp,
	AC_HELP_STRING([--disable-cexp],
		[dont build CEXP but only XSYMS])
)

AC_ARG_ENABLE(bfd,
	AC_HELP_STRING([--disable-bfd],
		[whether to disable using the BFD library in favor of libelf (default is 'enable')])
)

dnl if target==host, we build CEXP; either for the build host
dnl or for a different one otherwise, we build 'xsyms' only...
if test "$target-system" != "$host-system" ; then
enable_cexp=no
dnl gencore needs stdint.h
AC_CHECK_HEADER([stdint.h])
AH_TEMPLATE([HAVE_TYPE_UINT32_T])
AC_CHECK_TYPE(uint32_t,[AC_DEFINE([HAVE_TYPE_UINT32_T])],[AC_CHECK_SIZEOF([unsigned int])])
fi

dnl is this a 'xsyms' only checkout?
if test ! -f $srcdir/cexp.y ; then
	enable_cexp=no
fi

if test "$enable_cexp" != "no" ; then

AC_CHECK_HEADERS([sys/mman.h])

AC_ARG_ENABLE(YYDEBUG,
	AC_HELP_STRING([--enable-YYDEBUG],
		[whether to enable YYDEBUG; this adds a CEXP cmdline switch for enabling runtime parser state messages]),
)

AC_ARG_ENABLE(tecla,
	AC_HELP_STRING([--disable-tecla],
		[disable using the tecla library])
)

AC_ARG_ENABLE(readline,
	AC_HELP_STRING([--disable-readline],
		[disable using the readline library (used only if tecla is disabled)])
)

AC_ARG_ENABLE(opcodes,
	AC_HELP_STRING([--disable-opcodes],
		[disable using the opcodes library; removes disassembler support. (only if bfd is enabled)])
)

AC_ARG_ENABLE(libelf,
	AC_HELP_STRING([--disable-libelf],
		[disable using the libelf library for symbol table reading; you must then create and link a 'built-in' symbol table])
)

AC_ARG_WITH(rtems-notepad,
	AC_HELP_STRING([--with-rtems-notepad=<notepad_number>],
		[use an RTEMS notepad (0..14) to store context info instead of a task variable; NOTE: EPICS uses 11]),
		AC_DEFINE_UNQUOTED(CEXP_RTEMS_NOTEPAD, $withval,  [use RTEMS notepad $withval for storing context info])
)

if test "$enable_YYDEBUG" != "no" ; then
	AC_DEFINE(YYDEBUG,1,[  --enable-YYDEBUG whether to enable YYDEBUG])
fi

AC_CONFIG_SUBDIRS(regexp)

if test "$enable_tecla" != "no" ; then
	AC_CONFIG_SUBDIRS(libtecla)
	AC_DEFINE(HAVE_TECLA,1,[the tecla (reentrant line editor) library])
	AC_CHECK_LIB(curses,tgetnum,[LIBS="$LIBS -lcurses"],[AC_CHECK_LIB(ncurses,tgetnum,[LIBS="$LIBS -lncurses"])])
else if test "$enable_readline" != "no" ; then
	AC_CHECK_HEADER(readline/readline.h)
	if test "$ac_cv_header_readline_readline_h" = "yes" ; then
		AC_DEFINE(USE_GNU_READLINE,1,[whether to use the GNU readline library])
		AC_CHECK_LIB(termcap,tgetnum,[LIBS="$LIBS -ltermcap"],[AC_CHECK_LIB(ncurses,tgetnum,[LIBS="$LIBS -lncurses"])])
		AC_CHECK_LIB(readline,readline,[LIBS="-lreadline $LIBS"])
	fi
fi
fi

fi

dnl we need libelf for the 'xsyms' tool
if test "$enable_bfd" = "no" ; then
	enable_opcodes=no;

	if test "$enable_libelf" = "no"; then
		if test "$enable_cexp" = "no"; then
			AC_MSG_ERROR([must not disable both, bfd and libelf when building xsyms])
		fi
	else
		enable_libelf=yes	
		dnl can we use a locally installed lib?
		if test "$host-system" = "$build-system" ; then
			AC_CHECK_HEADER(libelf/libelf.h, AC_HAVE_LIBRARY(elf,we_have_native_libelf=yes))
dnl			AC_HAVE_LIBRARY(elf,we_have_native_libelf=yes)
		fi
		if test "$we_have_native_libelf" != "yes" ; then
dnl	     AC_CONFIG_SUBDIRS needs a literal arg :-(
			if test ! -d $srcdir/$LIBELF ; then
				AC_MSG_ERROR("YOU NEED TO INSTALL a $LIBELF source tree TO THE SOURCE DIRECTORY")
			fi
			AC_CONFIG_SUBDIRS(libelf-0.8.2)
		fi
		AC_DEFINE(HAVE_LIBELF,1,[do we have (and use) libelf])
	fi
else
	if test ! -d $srcdir/binutils ; then
		AC_MSG_ERROR("YOU NEED TO INSTALL binutils source tree TO THE SOURCE DIRECTORY)
	fi
dnl	AC_CONFIG_SUBDIRS needs a literal arg :-(
	AC_CONFIG_SUBDIRS(binutils)
	AC_DEFINE(HAVE_ELF_BFD_H,1,[do we have the bfd internal elf-bfd.h header])
	if test "$enable_opcodes" != "no"; then
		AC_DEFINE(HAVE_BFD_DISASSEMBLER,1,[do we have the bfd internal dis-asm.h header])
	fi
	enable_libelf=no
fi

AC_HAVE_FUNCS(rcmd vsnprintf)

dnl dont build the cexp library if host != target; they want to build xsyms
AM_CONDITIONAL(DO_CEXPLIB, test "$enable_cexp" != "no")
dnl if host=target AND host=build, then to the demo
AM_CONDITIONAL(DO_DEMO, 	test "$host-system" = "$build-system")
AM_CONDITIONAL(NATIVE_LIBELF, test "$we_have_native_libelf" = "yes" )
AM_CONDITIONAL(USEBFD,  test "$enable_bfd" != "no")
AM_CONDITIONAL(USETECLA,test "$enable_tecla" != "no")
AM_CONDITIONAL(USEOPCODES,test "$enable_opcodes" != "no")
AM_CONDITIONAL(USELIBELF,test "$enable_libelf" != "no")

AH_VERBATIM(NO_THREAD_PROTECTION,[
/* Disable thread protection on OS other than RTEMS */
#ifndef __rtems__
#define NO_THREAD_PROTECTION
#endif
])

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)

dnl when building these tools we don't want to install the whole crap in the subdirectories
if test "$target-system" != "$host-system" ; then
AC_SUBST(THESUBDIRS,['$(patsubst %,'"$subdirs"',$(filter-out install%,$@))'])
else
AC_SUBST(THESUBDIRS,["$subdirs"])
fi

AC_OUTPUT
