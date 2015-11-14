AC_INIT([rofi], [0.15.12-devel], [https://github.com/DaveDavenport/rofi/])

AC_CONFIG_SRCDIR([source/rofi.c])
AC_CONFIG_HEADER([config.h])

dnl ---------------------------------------------------------------------
dnl Setup automake to be silent and in foreign mode.
dnl ---------------------------------------------------------------------
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects dist-xz])
AM_SILENT_RULES([yes])

dnl ---------------------------------------------------------------------
dnl  Check for compiler
dnl ---------------------------------------------------------------------
AC_PROG_CC([clang gcc cc])
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_USE_SYSTEM_EXTENSIONS

dnl ---------------------------------------------------------------------
dnl Disable window mode
dnl ---------------------------------------------------------------------
AC_ARG_ENABLE([windowmode], AC_HELP_STRING([--disable-windowmode],[Disable window mode]))
AS_IF([ test "x$enable_windowmode" != "xno"], [AC_DEFINE([WINDOW_MODE],[1],[Enable the window mode])])

dnl ---------------------------------------------------------------------
dnl Output timing information
dnl ---------------------------------------------------------------------
AC_ARG_ENABLE([timings], AC_HELP_STRING([--enable-timings],[Enable timing output]))
AS_IF( [ test "x$enable_timings" = "xyes"], [AC_DEFINE([TIMINGS],[1], [Enable timings output])])

dnl ---------------------------------------------------------------------
dnl I3 check
dnl ---------------------------------------------------------------------
AC_ARG_ENABLE(i3support, [AS_HELP_STRING([--disable-i3support], [Disable check for i3 support])])
AS_IF([test "x$enable_i3support" != xno && test "x$enable_windowmode" != "xno"],[AC_CHECK_HEADERS([i3/ipc.h], [i3_header=yes; break;])])


dnl ---------------------------------------------------------------------
dnl Check for C functions.
dnl ---------------------------------------------------------------------
AC_CHECK_FUNC([getdelim],, AC_MSG_ERROR("Could not find getdelim in c library"))
AC_CHECK_FUNC([sysconf],,  AC_MSG_ERROR("Could not find sysconf"))

dnl ---------------------------------------------------------------------
dnl Check dependencies
dnl ---------------------------------------------------------------------
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([x11],      [x11])
PKG_CHECK_MODULES([xinerama], [xinerama])
PKG_CHECK_MODULES([pango],    [pango pangocairo])
PKG_CHECK_MODULES([cairo],	  [cairo cairo-xlib])
PKG_CHECK_MODULES([libsn],    [libstartup-notification-1.0])

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
if test x$enable_timings = xyes; then
echo "Timing output:               Enabled"
else
echo "Timing output:               Disabled"
fi
echo "-------------------------------------"
echo "Now type 'make' to build"
echo ""
