AC_INIT(cexp, 1.0, <strauman@slac.stanford.edu>)
AM_INIT_AUTOMAKE(cexp, 1.0)


AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_PROG_CC
AC_CHECK_PROGS(HOSTCC, gcc cc)
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_INCLUDES_DEFAULT

AC_ARG_ENABLE(YYDEBUG,
	[  --enable-YYDEBUG        whether to enable YYDEBUG; this adds a CEXP cmdline switch for enabling runtime parser state messages],
	[AC_DEFINE(YYDEBUG,1,[  --enable-YYDEBUG	whether to enable YYDEBUG])]
)

AC_CHECK_HEADER(readline/readline.h)
if test "$ac_cv_header_readline_readline_h" = "yes" ; then
	AC_DEFINE(USE_GNU_READLINE,1,[whether to use the GNU readline library])
	AC_CHECK_LIB(termcap,tgetnum)
	AC_CHECK_LIB(readline,readline)
fi

AM_CONDITIONAL(ISRTEMS, test "$host_os" = "rtems")


AC_CONFIG_SUBDIRS(libelf-0.8.0 regexp)

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)

AC_OUTPUT
