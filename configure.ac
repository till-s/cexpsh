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
AC_PROG_CXX
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
		 instead even if (patched) BFD/binutils sources are installed])
)

AC_ARG_ENABLE(opcodes,
	AC_HELP_STRING([--disable-opcodes],
		[disable using the opcodes library; removes disassembler support.
		(only if (patched) 'binutils' sources are installed)])
)

AC_ARG_ENABLE(loader,
	AC_HELP_STRING([--disable-loader],
		[remove the dynamic loader; using pmelf you can still read
		the symbol table from an ELF file (unless you also say
		--disable-elfsyms; in this case you must create and link a
		'built-in' symbol table)])
)

AC_ARG_ENABLE(elfsyms,
	AC_HELP_STRING([--disable-elfsyms],
		[remove support for loading an ELF symbol file;
		 you must create and link a 'built-in'
		 symbol table])
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

AC_ARG_WITH(rtems-top,
	AC_HELP_STRING([--with-rtems-top=<rtems installation topdir>],
		[point to RTEMS installation (only relevant if --host=XXX-rtems)]),
		[case "${host_os}" in *rtems* )
			AC_DEFINE_UNQUOTED(HAVE_RTEMS_HEADERS,1,[use RTEMS headers])
		;; esac],
		[case "${host_os}" in *rtems* )
			AC_MSG_ERROR([use --with-rtems-top to point to your RTEMS installation])
		;; esac]
)

AC_ARG_ENABLE(rtemsbsps,
	AC_HELP_STRING([--enable-rtemsbsp="bsp1 bsp2 ..."],
		[BSPs to include in build (ignore bsps not found in RTEMS installation)])
)

case "${host_os}" in *rtems* )
AC_MSG_CHECKING([Checking RTEMS installation])
if test ! -d $with_rtems_top/${host_cpu}-rtems/ ; then
	AC_MSG_ERROR([Looking for headers: directory $with_rtems_top/${host_cpu}-rtems/ not found])
fi
AC_MSG_RESULT([ok])
AC_MSG_CHECKING([Checking for RTEMS BSPs])
if test ! "${enable_rtemsbsp+set}" = "set" ; then
	enable_rtemsbsp=`ls $with_rtems_top/${host_cpu}-rtems/`
fi
rtemsbsps=""
for bspcand in $enable_rtemsbsp
do
	echo $bspcand
	if test -d $with_rtems_top/${host_cpu}-rtems/$bspcand/lib/include ; then
		rtemsbsps="$rtemsbsps $bspcand"
	fi
done
if test "$rtemsbsps"xx = xx; then
	AC_MSG_ERROR([No RTEMS BSPs found])
fi
AC_MSG_RESULT([found $rtemsbsps])

rtems_bsp=mvme3100

eval `make -f - rtems_makevars <<'EOF_'
include /tmp/box/powerpc-rtems/mvme3100/Makefile.inc
include $(RTEMS_CUSTOM)
include $(CONFIG.CC)

rtems_makevars:
	@echo rtems_cpu_cflags=\'$(CPU_CFLAGS)\'
	@echo rtems_gcc_specs=\'$(GCCSPECS)\'
EOF_
`
AC_SUBST(rtems_libdir,[$with_rtems_top/${host_cpu}-rtems/$rtems_bsp/lib])
AC_SUBST(rtems_gcc_specs,[$rtems_gcc_specs])
AC_SUBST(rtems_cpu_cflags,[$rtems_cpu_cflags])
AC_SUBST(rtems_includedir,[-I$with_rtems_top/${host_cpu}-rtems/$rtems_bsp/lib/include])
AC_SUBST(rtems_extra_includedir,[])
;; esac

# VALIDATE OPTIONS

# Validate the --enable-cexp setting:
AC_MSG_NOTICE([Validating configuration options])

if test "${enable_cexp}" = "no" && test "${enable_loader+set}" = "set"; then
	AC_MSG_ERROR([you must not use --enable-loader or --disable-loader together with --disable-cexp])
fi

if test "${enable_cexp}" = "no" && test "${enable_opcodes+set}" = "set"; then
	AC_MSG_ERROR([you must not use --enable-opcodes or --disable-opcodes together with --disable-cexp])
fi

if test ! "$target_os" = "$host_os" ; then
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

AC_MSG_CHECKING([Check if you have binutils sources installed in 'binutils' (don't forget to patch!)])
#check if they installed bfd/opcodes
if test -d $srcdir/binutils/ ; then
	binutils_installed=yes
else
	binutils_installed=no
fi
AC_MSG_RESULT([$binutils_installed])

AC_MSG_CHECKING([Whether we could use BFD if needed])
if test "$binutils_installed" = "yes" && test ! "$enable_bfd" = "no" ; then
	have_bfd=yes
else
	have_bfd=no
fi
AC_MSG_RESULT([$have_bfd])

AC_MSG_CHECKING([Check if we could use pmbfd])
#canonicalize x86 cpus
pmbfd_cpu=$host_cpu
case "$pmbfd_cpu" in
	i?86 ) pmbfd_cpu=i386
	;;
esac
# do we have relocations for our CPU arch ?
# Only relevant if we use the loader; the ELF symbol file reader should
# work anyways.
if test -f $srcdir/pmbfd/bfd-reloc-"$pmbfd_cpu".c ; then
	have_pmbfd=yes
	AC_MSG_RESULT([yes])
else
	have_pmbfd=no
	if test "$have_bfd" = "yes" ; then
		AC_MSG_RESULT([no. Relocations for CPU $pmbfd_cpu not implemented, sorry. Will use BFD.])
	else
		AC_MSG_RESULT([no. Relocations for CPU $pmbfd_cpu not implemented, sorry. You must install (and not disable) BFD sources to use the loader.])
	fi
fi


# matrix of legal configurations
#
# --enable-loader --enable-elfsyms  bfd installed or pmbfd
#                                   support available
#
#   unset           unset           yes         --> enable_loader=yes; enable_elfsyms=no
#   unset           unset           no          --> enable_loader=no;  enable_elfsyms=yes
#   no              unset           yes         -->                    enable_elfsyms=yes
#   no              unset           no          -->       (*)          enable_elfsyms=yes
#   yes             unset           yes         -->                    enable_elfsyms=no;
#   yes             unset           no          --> ERROR(no BFD support available)
#   unset           no              yes         --> enable_loader=yes;
#   unset           no              no          --> enable_loader=no;
#   no              no              yes         --> OK
#   no              no              no          --> OK
#   yes             no              yes         --> OK
#   yes             no              no          --> ERROR(no BFD support available)
#   unset           yes             yes         --> enable_loader=no;
#   unset           yes             no          --> enable_loader=no;
#   no              yes             yes         --> OK
#   no              yes             no          --> OK
#   yes             yes             yes         --> ERROR(cannot enable both)
#   yes             yes             no          --> ERROR(cannot enable both)


AC_MSG_CHECKING([Checking consistency of configuration options --enable-loader and --enable-elfsyms])

if test "$enable_loader" = "yes" && test "$enable_elfsyms" = "yes"; then
	AC_MSG_ERROR([Using both, --enable-loader and --enable-elfsyms, does not make sense (loader supersedes elfsyms)])
fi

if test ! "$enable_loader" = "no" ; then
	if test "$have_bfd" = "yes" || test "$have_pmbfd" = "yes" ; then
		if test "$enable_elfsyms" = "yes" ; then
			enable_loader=no
		else
			enable_loader=yes
			enable_elfsyms=no
		fi
	else
		if test "$enable_loader" = "yes"; then
			AC_MSG_ERROR([BFD sources not installed and pmbfd doesn't support your CPU; unable to enable the loader]) 
		else
			enable_loader=no
			if test ! "$enable_elfsyms" = "no" ; then
				enable_elfsyms=yes
			fi
		fi
	fi
else
# enable_loader = no
	if test ! "${enable_elfsyms}" = "no"; then
		enable_elfsyms=yes;
	fi
fi
AC_MSG_RESULT([passed])

AC_MSG_NOTICE([enable_loader: $enable_loader, enable_elfsyms: $enable_elfsyms])

AC_MSG_NOTICE([Check whether we should use the disassembler (opcodes)])
if test ! "${target_os}" = "${host_os}" ; then
	if test "${enable_opcodes}" = "yes"; then
		AC_MSG_ERROR([cannot --enable-opcodes when building cross tools])
	else
		enable_opcodes=no
	fi
fi

if test "${enable_opcodes}" = "yes"; then
	if test "$binutils_installed" = "no"; then
		AC_MSG_ERROR([You need to install (patched) binutils sources (containing 'opcodes' library into 'binutils' subdirectory])
	fi
else
	if test ! "${enable_opcodes}" = "no"; then
		enable_opcodes=yes
	fi
fi
AC_MSG_RESULT([$enable_opcodes])

# cannot use both line editors
if test "${enable_readline}" = "yes" && test "${enable_tecla}" = "yes"; then
	AC_MSG_ERROR([can only use one of --enable-tecla, --enable-readline])
fi

# if target==host, we build CEXP; either for the build host
# or for a different one otherwise, we build 'xsyms' only...
if test ! "$target_os" = "$host_os" ; then
	# gencore needs stdint -- but that requires real BFD
	AC_CHECK_HEADER([stdint.h])
	AH_TEMPLATE([HAVE_TYPE_UINT32_T])
	AC_CHECK_TYPE(uint32_t,[AC_DEFINE([HAVE_TYPE_UINT32_T])],[AC_CHECK_SIZEOF([unsigned int])])
fi

if test ! "$enable_cexp" = "no" ; then

	# for gencore:
	AC_CHECK_HEADERS([sys/mman.h])

	if test ! "$enable_YYDEBUG" = "no" ; then
		AC_DEFINE(YYDEBUG,1,[  --enable-YYDEBUG whether to enable YYDEBUG])
	fi

	AC_CONFIG_SUBDIRS(regexp)

	# check for line editor
	if test ! "$enable_tecla" = "no" ; then
		AC_CONFIG_SUBDIRS(libtecla)
		AC_DEFINE(HAVE_TECLA,1,[the tecla (reentrant line editor) library])
		AC_CHECK_LIB(curses,tgetnum,[LIBS="$LIBS -lcurses"],[AC_CHECK_LIB(ncurses,tgetnum,[LIBS="$LIBS -lncurses"])])
	else if test ! "${enable_readline}" = "no" ; then
		AC_CHECK_HEADER(readline/readline.h)
		if test "${ac_cv_header_readline_readline_h}" = "yes" ; then
			AC_DEFINE(USE_GNU_READLINE,1,[whether to use the GNU readline library])
			AC_CHECK_LIB(termcap,tgetnum,[LIBS="$LIBS -ltermcap"],[AC_CHECK_LIB(ncurses,tgetnum,[LIBS="$LIBS -lncurses"])])
			AC_CHECK_LIB(readline,readline,[LIBS="-lreadline $LIBS"])
		fi
	fi
	fi

fi

# based on the features requested, check which bfd library to use

if test "$enable_elfsyms" = "yes" ; then
	enable_bfd=no
	enable_pmbfd=yes
else
	if test "$enable_loader" = "yes" || test "$enable_opcodes" = "yes" ; then
		if test "$have_bfd" = "yes"; then
			enable_bfd=yes
			enable_pmbfd=no
		else
			enable_bfd=no
			enable_pmbfd=yes
		fi
	else
		enable_bfd=no
		enable_pmbfd=no
	fi
fi

AC_MSG_NOTICE([enable_bfd: $enable_bfd, enable_pmbfd: $enable_pmbfd])

if test "$enable_bfd" = "yes"; then
	AC_CONFIG_SUBDIRS(binutils)
	AC_DEFINE(HAVE_ELF_BFD_H,1,[do we have the bfd internal elf-bfd.h header])
else
	# must configure pmbfd if host_os = build_os so that
	# xsyms can use it
	# all files in cexp testing for USE_PMBFD are configured away if
	# neither enable_bfd nor enable_pmbfd is 'yes'
	if test "$enable_pmbfd" = "yes" || test "$host_os" = "$build_os" ; then
		AC_CONFIG_SUBDIRS(pmbfd)
		AC_DEFINE(USE_PMBFD,1,[should we use our "poor man's BFD implentation?"])	fi
fi

if test ! "$enable_opcodes" = "no"; then
	if test ! "$enable_bfd" = "yes"; then
		AC_CONFIG_SUBDIRS(binutils/libiberty)
		AC_CONFIG_SUBDIRS(binutils/opcodes)
		# mock up a few things opcodes needs...
		mkdir -p binutils/bfd
        case "${srcdir}" in
        /* | [A-Za-z]:[\\/]* ) # absolute path
          ln -s $srcdir/pmbfd/pmbfd.h binutils/bfd/bfd.h
          ;;
        *) # otherwise relative
          ln -s ../../$srcdir/pmbfd/pmbfd.h binutils/bfd/bfd.h
          ;;
        esac
		echo 999 > binutils/bfd/libtool-soversion
	fi
	AC_DEFINE(HAVE_BFD_DISASSEMBLER,1,[do we have the bfd internal dis-asm.h header])
fi

# don't recall what these were for...
AC_HAVE_FUNCS(rcmd vsnprintf)

AM_CONDITIONAL(DO_CEXPLIB, [test ! "$enable_cexp"    = "no"])
# if host=target AND host=build, then to the demo
AM_CONDITIONAL(DO_DEMO,    [test ! "$host_os" = "$build_os" && test "$host_os" = "$target_os"])
AM_CONDITIONAL(USEBFD,     [test ! "$enable_bfd"     = "no"])
AM_CONDITIONAL(USETECLA,   [test ! "$enable_tecla"   = "no"])
AM_CONDITIONAL(USEOPCODES, [test ! "$enable_opcodes" = "no"])
AM_CONDITIONAL(USELOADER,  [test ! "$enable_loader"  = "no"])
AM_CONDITIONAL(USEPMBFD,   [test ! "$enable_pmbfd"   = "no"])
AM_CONDITIONAL(USEELFSYMS, [test ! "$enable_elfsyms" = "no"])
AM_CONDITIONAL(OS_RTEMS,   [case "$host_os" in *rtems* ) : ;; *) false ;; esac])

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
if test ! "${enable_multilib+set}" = "set" ; then
	multilib=no
fi
# cannot use multilibs with BFD
if ( test ! "${enable_bfd}" = "no" || test ! "${enable_opcodes}" = "no" ) && test ! "${multilib}" = "no" ; then
	AC_MSG_ERROR([cannot build multilibbed BFD (or opcodes); please configure with --disable-multilib (or --disable-bfd --disable-opcodes)])
fi

# when building the cross tools (xsyms, gencore) we don't want to install
# the whole crap from the subdirectories
if test ! "$target_os" = "$host_os" ; then
	AC_SUBST(THESUBDIRS,['$(patsubst %,'"$subdirs"',$(filter-out install%,$@))'])
else
	AC_SUBST(THESUBDIRS,["$subdirs"])
fi

AC_OUTPUT
