AC_INIT([rofi], [1.2.0], [https://github.com/DaveDavenport/rofi/])

AC_CONFIG_SRCDIR([source/rofi.c])
AC_CONFIG_HEADER([config.h])

dnl ---------------------------------------------------------------------
dnl Setup automake to be silent and in foreign mode.
dnl We want xz distribution
dnl ---------------------------------------------------------------------
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects dist-xz])
AM_SILENT_RULES([yes])

dnl ---------------------------------------------------------------------
dnl  Check for compiler
dnl ---------------------------------------------------------------------
AC_PROG_CC([clang gcc cc])

dnl ---------------------------------------------------------------------
dnl  C99 standard
dnl ---------------------------------------------------------------------
AC_PROG_CC_C99

dnl ---------------------------------------------------------------------
dnl C to Object rules.
dnl ---------------------------------------------------------------------
AM_PROG_CC_C_O

dnl ---------------------------------------------------------------------
dnl System extensions
dnl ---------------------------------------------------------------------
AC_USE_SYSTEM_EXTENSIONS

dnl ---------------------------------------------------------------------
dnl Static libraries programs
dnl ---------------------------------------------------------------------
AC_PROG_RANLIB
AM_PROG_AR

dnl ---------------------------------------------------------------------
dnl Base CFLAGS
dnl ---------------------------------------------------------------------
AM_CFLAGS="-Wall -Wextra -Wparentheses -Winline -pedantic  -Wunreachable-code"

dnl ---------------------------------------------------------------------
dnl Enable source code coverage reporting for GCC
dnl ---------------------------------------------------------------------

AC_ARG_ENABLE(gcov,
              [  --enable-gcov           Enable source code coverage testing using gcov],
              [AM_CFLAGS="${AM_CFLAGS} -coverage"])
AS_IF([test "X${enable_gcov}" == "xyes" ], [AC_DEFINE([ENABLE_GCOV], [1], [Enable gcov profiling])])
AC_ARG_ENABLE(asan,
              [  --enable-asan           asan],
              [AM_CFLAGS="${AM_CFLAGS} -fsanitize=address -fno-omit-frame-pointer -g3"])
AS_IF([test "X${enable_asan}" == "xyes" ], [AC_DEFINE([ENABLE_ASAN], [1], [Enable libasan])])


dnl --------------------------------------------------------------------
dnl DRun dialog
dnl --------------------------------------------------------------------
AC_ARG_ENABLE([drun], AC_HELP_STRING([--disable-drun],[Disable desktop file run dialog]))
AS_IF([test "x${enable_drun}" != "xno"], [AC_DEFINE([ENABLE_DRUN], [1], [Enable desktop file run dialog])])
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
dnl Check for C functions.
dnl ---------------------------------------------------------------------
AC_CHECK_FUNC([getline],,  AC_MSG_ERROR("Could not find getline in c library"))
AC_CHECK_FUNC([open],,     AC_MSG_ERROR("Could not find open in c library"))
AC_CHECK_FUNC([sysconf],,  AC_MSG_ERROR("Could not find sysconf"))
AC_CHECK_FUNC([getenv],,   AC_MSG_ERROR("Could not find getenv"))
AC_CHECK_FUNC([strtok_r],, AC_MSG_ERROR("Could not find strtok_r"))
AC_CHECK_FUNC([flock],,    AC_MSG_ERROR("Could not find flock"))
AC_CHECK_FUNC([ftruncate],,AC_MSG_ERROR("Could not find ftruncate"))
AC_CHECK_FUNC([fcntl],,    AC_MSG_ERROR("Could not find fcntl"))
AC_CHECK_FUNC([setlocale],,AC_MSG_ERROR("Could not find setlocale"))
AC_CHECK_FUNC([atexit],,  AC_MSG_ERROR("Could not find atexit in c library"))

dnl ---------------------------------------------------------------------
dnl Check dependencies
dnl ---------------------------------------------------------------------
PKG_PROG_PKG_CONFIG


dnl ---------------------------------------------------------------------
dnl PKG_CONFIG based dependencies  
dnl ---------------------------------------------------------------------
PKG_CHECK_MODULES([glib],     [glib-2.0 >= 2.40 gio-unix-2.0])
GW_CHECK_XCB([xcb-aux xcb-xkb xkbcommon >= 0.5.0 xkbcommon-x11 xcb-ewmh xcb-icccm xcb-xrm xcb-randr xcb-xinerama])
PKG_CHECK_MODULES([pango],    [pango pangocairo])
PKG_CHECK_MODULES([cairo],	  [cairo cairo-xcb])
PKG_CHECK_MODULES([libsn],    [libstartup-notification-1.0])


dnl ---------------------------------------------------------------------
dnl Add extra compiler flags
dnl ---------------------------------------------------------------------
AC_SUBST([AM_CFLAGS])

AC_CONFIG_FILES([Makefile doc/rofi.doxy])
AC_OUTPUT

dnl -----------------------------------------------
dnl Some output to easily spot if I3 is enabled.
dnl -----------------------------------------------

echo ""
echo "-------------------------------------"
if test x$enable_timings = xyes; then
echo "Timing output:               Enabled"
else
echo "Timing output:               Disabled"
fi
if test x$enable_drun != xno; then
echo "Desktop File drun dialog     Enabled"
else
echo "Desktop File drun dialog     Disabled"
fi
if test x$enable_windowmode != xno; then
echo "Window Switcher dialog       Enabled"
else
echo "Window Switcher dialog       Disabled"
fi
echo "-------------------------------------"
echo "Now type 'make' to build"
echo ""
