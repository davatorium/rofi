AC_INIT([rofi], [0.14.3], [qball@gmpclient.org])

AC_CONFIG_SRCDIR([source])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O

##
# I3 check
##
AC_CHECK_HEADERS([i3/ipc.h],
	[i3_header=yes; break;])

AS_IF([test "x$i3_header" != xno],
    [
    AM_CONDITIONAL(I3, true)
    AC_DEFINE(I3, [], [Support for I3])
    ], [
    AM_CONDITIONAL(I3, false)
    ])
##
# Check dependencies
##
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([xft], [xft])
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([xinerama], [xinerama])
PKG_CHECK_MODULES([xdgbasedir], [libxdg-basedir])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
