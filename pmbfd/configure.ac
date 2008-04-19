AC_INIT(pmbfd, 1.0, strauman@slac.stanford.edu)
AM_INIT_AUTOMAKE(pmbfd, 1.0)
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_RANLIB
AC_CHECK_TOOL(AR,ar)

# Optional features
AC_ARG_ENABLE(pmelf64,
	AC_HELP_STRING([--enable-pmelf64],
		[enable 64-bit ELF support]),
	[],
	if test "${host_cpu}" = "x86_64"; then
		enable_pmelf64=yes
	fi
)

AC_ARG_ENABLE(byteswap,
	AC_HELP_STRING([--disable-byteswap],
		[disable support for byte swapping (byte-swapping
		is only required to access non-native ELF files)]),
	[],
	if test "${host_os}" = "rtems"; then
		enable_byteswap=no
	fi
)

if test "${enable_pmelf64}" = "yes"; then
	AC_DEFINE([PMELF_CONFIG_ELF64SUPPORT],1,[enable support for 64-bit ELF files])
fi
AM_CONDITIONAL(NO_64BIT, test "$enable_pmelf64" != "yes")

if test "${enable_byteswap}" = "no"; then
	AC_DEFINE([PMELF_CONFIG_NO_SWAPSUPPORT],1,[disable support for byte-swapping (non-native ELF)])
fi

pmbfd_cpu=$host_cpu
case "$pmbfd_cpu" in
	i*86 ) pmbfd_cpu=i386 ;;
esac

AC_SUBST(RELOCS)

RELOCS=bfd-reloc-"$pmbfd_cpu".c
if test ! -f $srcdir/"$RELOCS" ; then
	AC_MSG_WARN([$srcdir/"$RELOCS" not found -- no relocations for this CPU])
	RELOCS=
fi
AM_CONDITIONAL(HAVE_RELOCS, test ! "$RELOCS"xx = xx )

AM_ENABLE_MULTILIB([Makefile],[..])
dnl install multilibs into MULTISUBDIR
AC_SUBST(libdir,[${libdir}'$(MULTISUBDIR)'])

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)
AC_OUTPUT
