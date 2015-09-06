AC_INIT([rofi], [0.15.8], [qball@gmpclient.org])

AC_CONFIG_SRCDIR([source/rofi.c])
AC_CONFIG_HEADER([config.h])

##
# Setup automake to be silent and in foreign mode.
##
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects dist-xz])
AM_SILENT_RULES([yes])

##
# Check for compiler
##
AC_PROG_CC([clang gcc cc])
AC_PROG_CC_C99
AM_PROG_CC_C_O

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

##
# Check dependencies
##
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([xft], [xft])
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([xinerama], [xinerama])
PKG_CHECK_MODULES([pango], [pango pangoxft])

AC_SUBST([EXTRA_CFLAGS], ["-Wall -Wextra -Wparentheses -Winline -pedantic"])

AC_CONFIG_FILES([Makefile ])
AC_OUTPUT

dnl -----------------------------------------------
dnl Some output to easily spot if I3 is enabled.
dnl -----------------------------------------------

echo ""
echo "-------------------------------------"
if test x$i3_header = xyes; then
echo "I3 support:                  Enabled"
else
echo "I3 support:                  Disabled"
fi
echo "-------------------------------------"
echo "Now type 'make' to build"
echo ""
