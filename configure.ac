AC_INIT(cexp, 1.0, <strauman@slac.stanford.edu>)
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

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_PROG_CC
AC_CHECK_PROGS(HOSTCC, gcc cc)
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_INCLUDES_DEFAULT

AC_ARG_ENABLE(YYDEBUG,
	AC_HELP_STRING([--enable-YYDEBUG],
		[whether to enable YYDEBUG; this adds a CEXP cmdline switch for enabling runtime parser state messages]),
)

AC_ARG_ENABLE(bfd,
	AC_HELP_STRING([--disable-bfd],
		[whether to disable using the BFD library (default is 'enable')])
)

AC_ARG_ENABLE(tecla,
	AC_HELP_STRING([--disable-tecla],
		[disable using the tecla library])
)

AC_ARG_ENABLE(readline,
	AC_HELP_STRING([--disable-readline],
		[disable using the readline library (used only if tecla is disabled)])
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

dnl we need libelf for the 'xsyms' tool
if test "$enable_bfd" = "no" ; then
dnl     AC_CONFIG_SUBDIRS needs a literal arg :-(
	AC_CONFIG_SUBDIRS(libelf-0.8.2)
fi

if test "$enable_bfd" != "no" ; then
dnl	AC_CONFIG_SUBDIRS needs a literal arg :-(
	AC_CONFIG_SUBDIRS(binutils)
	AC_DEFINE(HAVE_ELF_BFD_H,1,[do we have the bfd internal elf-bfd.h header])
	AC_DEFINE(HAVE_BFD_DISASSEMBLER,1,[do we have the bfd internal dis-asm.h header])
fi


AC_HAVE_FUNCS(rcmd vsnprintf)

AM_CONDITIONAL(ISRTEMS, test "$host_os" = "rtems")
AM_CONDITIONAL(USEBFD,  test "$enable_bfd" != "no")
AM_CONDITIONAL(USETECLA,test "$enable_tecla" != "no")

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)

AC_OUTPUT
