AC_INIT(pmbfd, unknown, unknown)
AM_INIT_AUTOMAKE(pmbfd, unknown)
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_RANLIB
AC_CHECK_TOOL(AR,ar)


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

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
