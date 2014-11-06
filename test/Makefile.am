##
# Rofi the program
##
noinst_PROGRAMS=rofi_test textbox_test helper_test

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

textbox_test_SOURCES=\
    ../source/textbox.c\
    ../config/config.c\
    ../include/rofi.h\
    ../include/textbox.h\
	textbox-test.c

helper_test_SOURCES=\
    ../config/config.c\
    ../include/rofi.h\
    ../source/helper.c\
    ../include/helper.h\
    helper-test.c

.PHONY: test
test: ${bin_PROGRAMS} 
	./rofi_test
	./helper_test
	$(top_srcdir)/test/run_test.sh 123 $(top_builddir)/test/textbox_test $(top_builddir) $(top_srcdir)/doc/example.xresources
	$(top_srcdir)/test/run_test.sh 200 $(top_srcdir)/test/run_errormsg_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	$(top_srcdir)/test/run_test.sh 201 $(top_srcdir)/test/run_switchdialog_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	$(top_srcdir)/test/run_test.sh 202 $(top_srcdir)/test/run_dmenu_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	$(top_srcdir)/test/run_test.sh 203 $(top_srcdir)/test/run_dmenu_custom_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	$(top_srcdir)/test/run_test.sh 204 $(top_srcdir)/test/run_run_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	$(top_srcdir)/test/run_test.sh 205 $(top_srcdir)/test/run_script_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
