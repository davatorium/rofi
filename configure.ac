AC_INIT([rofi], [0.15.1], [qball@gmpclient.org])

AC_CONFIG_SRCDIR([source/rofi.c])
AC_CONFIG_HEADER([config.h])

##
# Setup automake to be silent and in foreign mode.
##
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_SILENT_RULES([yes])

##
# Check for compiler
##
AC_PROG_CC([clang gcc cc])
AC_PROG_CC_C99

AC_USE_SYSTEM_EXTENSIONS

##
# I3 check
##
AC_ARG_ENABLE(i3support,
        [AS_HELP_STRING([--disable-i3support], [Disable check for i3 support])])
if test "x$enable_i3support" != xno;
then
    AC_CHECK_HEADERS([i3/ipc.h],
        [i3_header=yes; break;])
fi


dnl ---------------------------------------------------------------------
dnl Enable source code coverage reporting for GCC
dnl ---------------------------------------------------------------------

AC_ARG_ENABLE(gcov,
        [  --enable-gcov           Enable source code coverage testing using gcov],
        [CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])


dnl
dnl Enable timing debug output.
dnl

AC_ARG_ENABLE(timings,
        [ --enable-timings         Enable debug information about timings],
        [CFLAGS="$CFLAGS -DTIMING"])

##
# Check dependencies
##
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([xft], [xft])
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([xinerama], [xinerama])
PKG_CHECK_MODULES([pango], [pango pangoxft])

AC_SUBST([EXTRA_CFLAGS], ["-Wall -Wextra -Wparentheses -Winline"])

AC_CONFIG_FILES([Makefile ])
AC_OUTPUT
