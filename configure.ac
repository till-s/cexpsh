m4_include([./m4/cvstag.m4])

AC_INIT(cexp, TILLAC_CVSTAG([$Name$]), <strauman@slac.stanford.edu>)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign 1.10])
AM_MAINTAINER_MODE

# Check for critical programs we need for building
TILLAC_RTEMS_CHECK_TOOLS

BINUTILS=binutils
AC_SUBST(BINUTILS)
LIBTECLA=libtecla
AC_SUBST(LIBTECLA)

# Must add this BEFORE TILLAC_RTEMS_SETUP
# so that the dummy-top 'config.status' also
# knows how to make a config.h
AM_CONFIG_HEADER(config.h)

TILLAC_RTEMS_SETUP(domultilib)

# sys/termios.h and others can only be found outside of
# the BSP installation area if cpukit was multilibbed :-(
if test "${enable_multilib}" = "yes" ; then
	AC_MSG_CHECKING([Checking for multilibbed cpukit])
	if TILLAC_RTEMS_CPUKIT_MULTILIB ; then
		AC_MSG_RESULT([Yes; good.]);
	else
		AC_MSG_RESULT([No]);
		AC_MSG_ERROR([Can only build multilibbed CEXP if cpukit is multilibbed, sorry.])
	fi
fi

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

AC_ARG_ENABLE(text-segment,
	AC_HELP_STRING([--enable-text-segment=<size>],
		[Reserve <size> space in the CEXP executable for the .text
        sections of loadable modules. This is required on powerpc
        platforms with more than 32M of memory so that no far jumps
        are needed.
        <size> may be 'no' (disable feature), 'yes' (reserve default
        [[8MB]]) or a number. If <size> is zero then the application
        must provide the following two global variables:

            unsigned long cexpTextRegionSize=<desired_size>;

            unsigned char cexpTextRegion[<desired_size>];

        defining the memory region used.
		Alternatively, the application (or the linker script) may
		provide symbols _cexpTextRegionStart/_cexpTextRegionEnd.
        This option is ignored on platforms other than powerpc.
		The default is 0.])
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

AC_MSG_CHECKING([Check if text-segment was given])
if test ! "${enable_text_segment+set}" = "set"; then
	enable_text_segment=0
fi
case "${enable_text_segment}" in
	yes|YES)
# 8MB default; only relevant on PPC
		enable_text_segment=0x800000;
	;;
	no|NO)
		enable_text_segment=no;
	;;
	*)
	if  echo $enable_text_segment | grep -qv '\<\(0[[0-7]]*\|[[1-9]][[0-9]]*\|0x[[0-9a-fA-F]]\+\)\>' ; then
		AC_MSG_ERROR([Invalid setting for --enable-text-segment: $enable_text_segment])
	fi
	;;
esac
AC_MSG_RESULT([$enable_text_segment])

if test ! "$enable_text_segment" = no ; then
	AC_DEFINE_UNQUOTED(CEXP_TEXT_REGION_SIZE,$enable_text_segment,[Size of text region for loaded modules])
fi


#canonicalize os name
canon_os=$host_os
case "$canon_os" in
	*[[rR]][[tT]][[eE]][[mM]][[sS]]*)	
		canon_os=rtems;
	;;
esac

#canonicalize x86 cpus
canon_cpu=$host_cpu
case "$canon_cpu" in
	i?86 ) canon_cpu=i386
	;;
esac


AC_MSG_CHECKING([Check if we could use pmbfd])
# do we have relocations for our CPU arch ?
# Only relevant if we use the loader; the ELF symbol file reader should
# work anyways.
if test -f $srcdir/pmbfd/bfd-reloc-"$canon_cpu".c ; then
	have_pmbfd=yes
	AC_MSG_RESULT([yes])
else
	have_pmbfd=no
	if test "$have_bfd" = "yes" ; then
		AC_MSG_RESULT([no. Relocations for CPU $canon_cpu not implemented, sorry. Will use BFD.])
	else
		AC_MSG_RESULT([no. Relocations for CPU $canon_cpu not implemented, sorry. You must install (and not disable) BFD sources to use the loader.])
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
		if test "${binutils_installed}" = "yes"; then
			enable_opcodes=yes
		else
			enable_opcodes=no
		fi
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

AH_TEMPLATE([HAVE_RTEMS_H])
AC_CHECK_HEADER([rtems.h],[AC_DEFINE([HAVE_RTEMS_H],1,[If we found RTEMS headers; otherwise ugly hacks will be used!])])

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

if test "$enable_loader" = "yes" ; then
	AC_DEFINE(USE_LOADER,1,[whether we configured the run-time loader])
fi

# Reject maintainer mode if bfd/opcodes are used -- they require
# older autotools
#if test "$enable_maintainer_mode" = "yes" ; then
#	if test "$enable_bfd" = "yes" -o "$enable_opcodes" = "yes" ; then
#		AC_MSG_ERROR([Cannot use --enable-maintainer-mode when BFD or OPCODES are used -- these packages require older autotools than CEXP])
#	fi
#fi

# don't recall what these were for...
AC_HAVE_FUNCS(rcmd vsnprintf)

AH_VERBATIM(NO_THREAD_PROTECTION,[
/* Disable thread protection on OS other than RTEMS */
#ifndef __rtems__
#define NO_THREAD_PROTECTION
#endif
])

AC_CONFIG_FILES(Makefile)

# cannot use multilibs with BFD
if ( test ! "${enable_bfd}" = "no" || test ! "${enable_opcodes}" = "no" ) && test "${enable_multilib}" = "yes" ; then
	AC_MSG_ERROR([cannot build multilibbed BFD (or opcodes); please configure with --disable-multilib (or --disable-bfd --disable-opcodes)])
fi

# when building the cross tools (xsyms, gencore) we don't want to install
# the whole crap from the subdirectories
if test ! "$target_os" = "$host_os" ; then
	AC_SUBST(THESUBDIRS,['$(patsubst %,'"$subdirs"',$(filter-out install%,$@))'])
else
	AC_SUBST(THESUBDIRS,["$subdirs"])
fi

# Our particular snippet
AC_SUBST(SEGS_CPU_SRC)
SEGS_CPU_SRC=cexpsegs-"$canon_cpu"-"$canon_os".c
if test ! -f $srcdir/"$SEGS_CPU_SRC" ; then
	SEGS_CPU_SRC=cexpsegs-dflt.c
fi

AM_CONDITIONAL(DO_CEXPLIB, [test ! "$enable_cexp"    = "no"])
# if host=target AND host=build, then to the demo
AM_CONDITIONAL(DO_DEMO,    [test "$host_os" = "$build_os" && test "$host_os" = "$target_os"])
AM_CONDITIONAL(USEBFD,     [test ! "$enable_bfd"     = "no"])
AM_CONDITIONAL(USETECLA,   [test ! "$enable_tecla"   = "no"])
AM_CONDITIONAL(USEOPCODES, [test ! "$enable_opcodes" = "no"])
AM_CONDITIONAL(USELOADER,  [test ! "$enable_loader"  = "no"])
AM_CONDITIONAL(USEPMBFD,   [test ! "$enable_pmbfd"   = "no"])
AM_CONDITIONAL(USEELFSYMS, [test ! "$enable_elfsyms" = "no"])
AM_CONDITIONAL(OS_RTEMS,   [case "$host_os" in *rtems* ) : ;; *) false ;; esac])


AC_OUTPUT
