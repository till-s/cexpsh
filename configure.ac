dnl strip $Name$ from cvs tag and also a possible 'Cexp_' prefix
m4_define(
	cexpversion,
	m4_bregexp(
		[$Name$],
		[\([$]Name:[ ]*\)\([cC][eE][xX][pP]_?\)?\([^$]*\)[$]],
		\3))

m4_define(
	CEXPVERSION,
	m4_if(cexpversion,,[untagged],cexpversion))

m4_undefine([cexpversion])

AC_INIT(cexp, CEXPVERSION, <strauman@slac.stanford.edu>)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(cexp, CEXPVERSION)
AM_MAINTAINER_MODE

# Check for critical programs we need for building
AC_PROG_CC
AC_SUBST(GCC)
AC_PROG_CPP
AC_CHECK_PROGS(HOSTCC, gcc cc)
AC_CHECK_TOOL(AR,ar)
AC_PROG_RANLIB
AC_PROG_INSTALL

BINUTILS=binutils
AC_SUBST(BINUTILS)
LIBTECLA=libtecla
AC_SUBST(LIBTECLA)

# Optional features
AC_ARG_ENABLE(cexp,
	AC_HELP_STRING([--disable-cexp],
		[dont build CEXP but only XSYMS])
)

AC_ARG_ENABLE(bfd,
	AC_HELP_STRING([--disable-bfd],
		[whether to disable using the BFD library; use pmbfd/pmelf
		 instead even if BFD is installed])
)

AC_ARG_ENABLE(opcodes,
	AC_HELP_STRING([--disable-opcodes],
		[disable using the opcodes library; removes disassembler support.
		(only if BFD is installed and enabled)])
)

AC_ARG_ENABLE(pmbfd,
	AC_HELP_STRING([--disable-pmbfd],
		[dont use the pmbfd/pmelf library; if you disable both,
		 bfd and pmbfd/pmelf then you must create and link a 'built-in'
		 symbol table])
)

AC_ARG_ENABLE(loader,
	AC_HELP_STRING([--disable-loader],
		[disable the dynamic loader; using pmbfd/pmelf you can still read
		the symbol table from an ELF file (unless you also say
		--disable-pmbfd; in this case you must create and link a
		'built-in' symbol table)])
)

AC_ARG_ENABLE(YYDEBUG,
	AC_HELP_STRING([--enable-YYDEBUG],
		[whether to enable YYDEBUG; this adds a CEXP cmdline switch
		for enabling runtime parser state messages]),
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
		[use an RTEMS notepad (0..14) to store context info instead of a
		task variable; NOTE: EPICS uses 11]),
		AC_DEFINE_UNQUOTED(CEXP_RTEMS_NOTEPAD,
		                   $withval,
						   [use RTEMS notepad $withval for storing context info])
)

# VALIDATE OPTIONS

# Validate the --enable-cexp setting:

if test "${enable_cexp}" = "no" -a "${enable_loader+set}" = "set"; then
	AC_MSG_ERROR([you must not use --enable-loader or --disable-loader together with --disable-cexp])
fi

if test "$target-system" != "$host-system" ; then
	if test "${enable_cexp}" = "yes"; then
		AC_MSG_ERROR([you cannot --enable-cexp for a target; --target is for xsyms only])
	fi
	enable_cexp=no
else
	if test ! "${enable_cexp+set}" = "set"; then
		enable_cexp=yes
	fi
fi

#is this a xsyms-only checkout ?
if test ! -f $srcdir/cexp.y ; then
	if test "${enable_cexp}" = "yes" ; then
		AC_MSG_ERROR([cannot --enable-cexp on a xsyms-only checked out copy])
	else
		enable_cexp=no
	fi
fi

# Matrix of all configuration possibilities of --enable-bfd, --enable-pmbfd, --enable-loader
#
#                                       loader  unset             off              on
#
#bfd     pmbfd    BFD installed        
#unset   unset          no           -> pmbfd:= on,  l:=on *| pmbfd:=on bfd:=off*| pmbfd:=on bfd:=off *|
#                                       bfd:=off
#on      unset          no           -> error              *| error             *| error              *|
#off     unset          no           -> pmbfd:= on,  l:=on *| pmbfd:=on         *| pmbfd:=on          *|
#unset   on             no           -> bfd  := off, l:=on *| bfd := off        *| bfd:=off           *|
#on      on             no           -> error              *| error             *| error              *|
#off     on             no           ->              l:=on *| ok                *|  ok                *|
#unset   off            no           -> bfd  := off, l:=off*| bfd:=off          *| error              *|
#on      off            no           -> error              *| error             *| error              *|
#off     off            no           ->              l:=off*| ok                *| error              *|
#unset   unset          yes          -> pmbfd:= off,
#                                       bfd:=on,     l:=on *| bfd:=off pmbfd:=on*| bfd:=on, pmbfd:=off*|
#on      unset          yes          -> pmbfd:= off, l:=on *| error             *| pmbfd:=off         *|
#off     unset          yes          -> pmbfd:= on,  l:=on *| pmbfd:=on         *| pmbfd:=on          *|
#unset   on             yes          -> bfd  := off, l:=on *| bfd:=off          *| bfd:=off           *|
#on      on             yes          -> error              *| error             *| error              *|
#off     on             yes          ->              l:=on *| ok                *|  ok                *|
#unset   off            yes          -> bfd  := on   l:=on *| bfd:=off          *|  bfd:=on           *|
#on      off            yes          ->              l:=on *| error             *|  ok                *|
#off     off            yes          ->              l:=off*| ok                *| error              *|

# Check valid combinations of --enable-bfd, --enable-pmbfd, --enable-loader
# and pick suitable defaults for unset options.
if test ! -d $srcdir/binutils/bfd ; then
	# BFD not installed; 
	if test "${enable_bfd}" = "yes"; then
		AC_MSG_ERROR([cannot use BFD; 'binutils' sources not found in cexp top directory (install and patch first)])
	fi
	# make sure it is switched off
	enable_bfd=no
else
	# both: --enable-bfd --enable-pmbfd given
	if test "${enable_bfd}" = "yes" -a "${enable_pmbfd}" = "yes"; then
		AC_MSG_ERROR([cannot use both, BFD and pmbfd; please disable either of them])
	fi
	# using BFD without loader doesn't make sense
	if test "${enable_bfd}" = "yes" -a "${enable_loader}" = "no"; then
		AC_MSG_ERROR([enabling BFD but disabling the loader doesn't make sense])
	fi
fi

if test ! "${enable_bfd+set}" = "set"; then
	# neither --enable-bfd nor --disable-bfd was given; pick a default:
	# unless the loader is explicitly disabled we enable BFD -- except
	# if pmbfd is explicitly asked for.
	if test "${enable_pmbfd}" = "yes" -o "${enable_loader}" = "no"; then
		enable_bfd=no
	else
		enable_bfd=yes
	fi
fi

# cannot ask for the loader and disable both, pmbfd + BFD
if test "${enable_loader}" = "yes" -a "${enable_bfd}" = "no" -a "${enable_pmbfd}" = "no"; then
	AC_MSG_ERROR([you cannot enable the loader with both BFD and pmbfd disabled])
fi

if test ! "${enable_loader+set}" = "set"; then
	# neither --enable-loader nor --disable-loader given; pick a default
	if test "${enable_bfd}" = "no" -a "${enable_pmbfd}" = "no"; then
		enable_loader=no
	else
		enable_loader=yes
	fi
fi

# finally we need to find out if we should use pmbfd...
if test ! "${enable_pmbfd+set}" = "set"; then
	if test "${enable_loader}" = "yes"; then
		if "${enable_bfd}" = "yes"; then
			enable_pmbfd=no
		else
			enable_pmbfd=yes
		fi
	else
		enable_pmbfd=yes
	fi
fi


# At this point we have cooked down to the following valid combinations
#
# loader = yes, bfd = yes, pmbfd = no  -> loader using BFD
# loader = yes, bfd = no,  pmbfd = yes -> loader using pmbfd
# loader = no,  bfd = no,  pmbfd = yes -> ELF symbol file only, using pmbfd
# loader = no,  bfd = no,  pmbfd = no  -> no symbol file reader; must use builtin symbol table

# opcodes can only be used if we have BFD
if test "${enable_opcodes}" = "yes" -a "${enable_bfd}" = "no"; then
	AC_MSG_ERROR([you can not --enable-opcodes w/o BFD installed and enabled])
fi
# pick default
if test ! "${enable_opcodes+set}" = "set"; then
	if test "${enable_bfd}" = "yes"; then
		enable_opcodes=yes
	else
		enable_opcodes=no
	fi
fi

# cannot use both line editors
if test "${enable_readline}" = "yes" -a "${enable_tecla}" = "yes"; then
	AC_MSG_ERROR([can only use one of --enable-tecla, --enable-readline])
fi

# if target==host, we build CEXP; either for the build host
# or for a different one otherwise, we build 'xsyms' only...
if test "$target-system" != "$host-system" ; then
	# gencore needs stdint -- but that requires real BFD
	AC_CHECK_HEADER([stdint.h])
	AH_TEMPLATE([HAVE_TYPE_UINT32_T])
	AC_CHECK_TYPE(uint32_t,[AC_DEFINE([HAVE_TYPE_UINT32_T])],[AC_CHECK_SIZEOF([unsigned int])])
fi

if test "$enable_cexp" != "no" ; then

	# for gencore:
	AC_CHECK_HEADERS([sys/mman.h])

	if test "$enable_YYDEBUG" != "no" ; then
		AC_DEFINE(YYDEBUG,1,[  --enable-YYDEBUG whether to enable YYDEBUG])
	fi

	AC_CONFIG_SUBDIRS(regexp)

	# check for line editor
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

# check if relocations for our CPU are implemented by pmbfd
if test "$enable_pmbfd" = "yes"; then
	pmbfd_cpu=$host_cpu
	case "$pmbfd_cpu" in
		i*86 ) pmbfd_cpu=i386
		;;
	esac
	# try to use 'pmbfd' -- do we have relocations for our CPU arch ?
	# Only relevant if we use the loader; the ELF symbol file reader should
	# work anyways.
	if test "${enable_loader}" = "yes" -a ! -f $srcdir/pmbfd/bfd-reloc-"$pmbfd_cpu".c ; then
		AC_MSG_ERROR([pmbfd relocations for $pmbfd_cpu not implemented, sorry Provide an implementation or install BFD])
	fi
	AC_CONFIG_SUBDIRS(pmbfd)
	AC_DEFINE(USE_PMBFD,1,[should we use our "poor man's BFD implentation?"])
fi

if test "$enable_bfd" = "yes"; then
	AC_CONFIG_SUBDIRS(binutils)
	AC_DEFINE(HAVE_ELF_BFD_H,1,[do we have the bfd internal elf-bfd.h header])
	if test "$enable_opcodes" != "no"; then
		AC_DEFINE(HAVE_BFD_DISASSEMBLER,1,[do we have the bfd internal dis-asm.h header])
	fi
fi

# don't recall what these were for...
AC_HAVE_FUNCS(rcmd vsnprintf)

AM_CONDITIONAL(DO_CEXPLIB, test "$enable_cexp"    != "no")
# if host=target AND host=build, then to the demo
AM_CONDITIONAL(DO_DEMO,    test "$host-system" = "$build-system" -a "$host-system" = "$target-system")
AM_CONDITIONAL(USEBFD,     test "$enable_bfd"     != "no")
AM_CONDITIONAL(USETECLA,   test "$enable_tecla"   != "no")
AM_CONDITIONAL(USEOPCODES, test "$enable_opcodes" != "no")
AM_CONDITIONAL(USELOADER,  test "$enable_loader"  != "no")
AM_CONDITIONAL(USEPMBFD,   test "$enable_pmbfd"   != "no")

AH_VERBATIM(NO_THREAD_PROTECTION,[
/* Disable thread protection on OS other than RTEMS */
#ifndef __rtems__
#define NO_THREAD_PROTECTION
#endif
])

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)

AM_ENABLE_MULTILIB([Makefile],[.])
# install multilibs into MULTISUBDIR
AC_SUBST(libdir,[${libdir}'$(MULTISUBDIR)'])
# in order to properly build multilibs in sub-libraries it seems we
# must pass the --enable-multilibs arg to sub-configures or multilibs
# are not built there.
# To work around, we simply set the default to 'no' so the user must
# say --enable-multilib to get them.
if test "${enable_multilib+set}" != "set" ; then
	multilib=no
fi
# cannot use multilibs with BFD
if test "${enable_bfd}" != "no" -a "${multilib}" != "no" ; then
	AC_MSG_ERROR([cannot build multilibbed BFD; please configure with --disable-multilib])
fi

# when building the cross tools (xsyms, gencore) we don't want to install
# the whole crap from the subdirectories
if test "$target-system" != "$host-system" ; then
	AC_SUBST(THESUBDIRS,['$(patsubst %,'"$subdirs"',$(filter-out install%,$@))'])
else
	AC_SUBST(THESUBDIRS,["$subdirs"])
fi

AC_OUTPUT
