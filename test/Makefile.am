##
# Rofi the program
##
bin_PROGRAMS=rofi_test

LIBS=\
    @xft_LIBS@\
    @x11_LIBS@\
    @xinerama_LIBS@\
	@pango_LIBS@

AM_CFLAGS=\
    @xft_CFLAGS@\
    @x11_CFLAGS@\
    @xinerama_CFLAGS@\
    @pango_CFLAGS@\
    -DMANPAGE_PATH="\"$(mandir)/man1/rofi.1\""\
    -I$(top_srcdir)/include/\
    -I$(top_srcdir)/config/\
    -I$(top_builddir)/

rofi_test_SOURCES=\
    ../source/history.c\
    ../config/config.c\
    ../include/rofi.h\
    ../include/history.h\
	history-test.c

.PHONY: test
test: rofi_test
	./$^ 
