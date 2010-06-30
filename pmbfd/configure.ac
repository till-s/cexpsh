AC_INIT(pmbfd, 1.0, strauman@slac.stanford.edu)
AM_INIT_AUTOMAKE([foreign 1.10])
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

AC_PROG_INSTALL
AC_PROG_CC
RTEMS_CHECK_TOOL(RANLIB,ranlib)
RTEMS_CHECK_TOOL(AR,ar)

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
	[case $host_os in
	*[[rR]][[tT]][[eE]][[mM]][[sS]]*)
		enable_byteswap=no
	esac]
)

AC_ARG_ENABLE(tests,
	AC_HELP_STRING([--enable-tests],
		[enable test programs])
)

AC_ARG_ENABLE(attributes-vendors-all,
	AC_HELP_STRING([--enable-attributes-vendors-all],
		[compile-in all supported file attribute parsers; by default
		 only parsers for the host OS/CPU/ABI are compiled (if available)]),
	[],
	[case "${host_os}" in
		*[[rR]][[tT]][[eE]][[mM]][[sS]]*)
		:
		;;
		*)
		enable_attributes_vendors_all=yes
		;;
	esac]
)

AC_CHECK_HEADER([strings.h],
	[AC_DEFINE(HAVE_STRINGS_H, 1, [Define to 1 if you have <strings.h>.])]
)


if test "${enable_pmelf64}" = "yes"; then
	AC_DEFINE([PMELF_CONFIG_ELF64SUPPORT],1,[enable support for 64-bit ELF files])
fi
AM_CONDITIONAL(NO_64BIT, test "$enable_pmelf64" != "yes")

if test "${enable_byteswap}" = "no"; then
	AC_DEFINE([PMELF_CONFIG_NO_SWAPSUPPORT],1,[disable support for byte-swapping (non-native ELF)])
fi

if test "${enable_attributes_vendors_all}" = "yes" ; then
	AC_DEFINE([PMELF_CONFIG_ATTRIBUTE_VENDOR_ALL],1,[enable support for all known file attribute parsers])
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

TILLAM_MULTISUB_INSTALLDIR

AM_CONDITIONAL(BUILD_UTILS, [case "$host_os" in *[[rR]][[tT]][[eE]][[mM]][[sS]]*) false;; *) true;; esac])
AM_CONDITIONAL(BUILD_TESTS, [test "$enable_tests" = "yes"])

AC_CONFIG_FILES(Makefile)
AM_CONFIG_HEADER(config.h)
AC_OUTPUT
