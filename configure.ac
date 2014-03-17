AC_INIT([rofi], [0.14.3], [qball@gmpclient.org])

AC_CONFIG_SRCDIR([source])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([xft], [xft])
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([xinerama], [xinerama])
PKG_CHECK_MODULES([xdgbasedir], [libxdg-basedir])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
