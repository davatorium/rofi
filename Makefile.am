# Specify automake version.
AUTOMAKE_OPTIONS  = 1.11.3

ACLOCAL_AMFLAGS = -I libgwater ${ACLOCAL_FLAGS}

noinst_LIBRARIES =
include $(top_srcdir)/libgwater-xcb-nolibtool.mk

##
# Rofi the program
##
bin_PROGRAMS=rofi

dist_bin_SCRIPTS=script/rofi-sensible-terminal

rofi_SOURCES=\
	source/rofi.c\
	source/view.c\
	source/mode.c\
	source/keyb.c\
	config/config.c\
	source/helper.c\
	source/widget.c\
	source/textbox.c\
	source/timings.c\
	source/history.c\
	source/scrollbar.c\
	source/i3-support.c\
	source/xrmoptions.c\
	source/x11-helper.c\
	source/x11-event-source.c\
	source/dialogs/run.c\
	source/dialogs/ssh.c\
	source/dialogs/drun.c\
	source/dialogs/dmenu.c\
	source/dialogs/combi.c\
	source/dialogs/window.c\
	source/dialogs/script.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	include/settings.h\
	include/keyb.h\
	include/view.h\
	include/view-internal.h\
	include/helper.h\
	include/timings.h\
	include/history.h\
	include/widget.h\
	include/textbox.h\
	include/scrollbar.h\
	include/xrmoptions.h\
	include/i3-support.h\
	include/x11-helper.h\
	include/x11-event-source.h\
	include/dialogs/ssh.h\
	include/dialogs/run.h\
	include/dialogs/drun.h\
	include/dialogs/dmenu.h\
	include/dialogs/combi.h\
	include/dialogs/script.h\
	include/dialogs/window.h\
	include/dialogs/dialogs.h

rofi_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(x11_CFLAGS)\
	$(xinerama_CFLAGS)\
	$(pango_CFLAGS)\
	$(libsn_CFLAGS)\
	$(cairo_CFLAGS)\
	-DMANPAGE_PATH="\"$(mandir)/\""\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/\
	-Werror=missing-prototypes

rofi_LDADD=\
	$(glib_LIBS)\
	$(GW_XCB_LIBS)\
	$(x11_LIBS)\
	$(xinerama_LIBS)\
	$(libsn_LIBS)\
	$(pango_LIBS)\
	$(cairo_LIBS)\
	$(LIBS)

##
# Manpage
##
dist_man1_MANS=\
	doc/rofi.1\
	doc/rofi-sensible-terminal.1

##
# Readme.md
##
markdown_SC_FILES=\
	README.md

# want the html to show up in release.
md_verbose = $(md_verbose_@AM_V@)
md_verbose_ = $(md_verbose_@AM_DEFAULT_V@)
md_verbose_0 = @echo "  MD" $@;

markdown_FILES=\
	README.html

README.html: README.md
	$(md_verbose) markdown $< > $@


##
# config directory
##
$(top_builddir)/config/config.c: $(top_srcdir)/config/config.def.c
	cp $(top_srcdir)/config/config.def.c $@

##
# Extra DIST
##
EXTRA_DIST=\
	$(markdown_FILES)\
	$(markdown_SC_FILES)\
	Examples/i3_switch_workspaces.sh\
	$(top_srcdir)/config/config.def.c\
	INSTALL.md\
	AUTHORS\
	doc/rofi.doxy.in\
	Changelog

##
# Indent
##


update-manpage: $(top_srcdir)/doc/rofi-manpage.markdown
	ronn --roff --pipe $^ > $(top_srcdir)/doc/rofi.1

##
# Rofi test program
##
check_PROGRAMS=rofi_test textbox_test helper_test

rofi_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

rofi_test_LDADD=\
	$(glib_LIBS)


rofi_test_SOURCES=\
	source/history.c\
	config/config.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	include/settings.h\
	include/history.h\
	test/history-test.c

textbox_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(pango_CFLAGS)\
	$(x11_CFLAGS)\
	$(xinerama_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(cairo_CFLAGS)\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

textbox_test_LDADD=\
	$(glib_LIBS)\
	$(pango_LIBS)\
	$(x11_LIBS)\
	$(xinerama_LIBS)\
	$(GW_XCB_LIBS)\
	$(cairo_LIBS)

textbox_test_SOURCES=\
	source/widget.c\
	source/textbox.c\
	config/config.c\
	source/keyb.c\
	source/x11-helper.c\
	source/xrmoptions.c\
	source/helper.c\
	include/keyb.h\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	include/settings.h\
	include/widget.h\
	include/textbox.h\
	include/widget.h\
	include/x11-helper.h\
	include/xrmoptions.h\
	include/helper.h\
	test/textbox-test.c

helper_test_SOURCES=\
	config/config.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	source/helper.c\
	include/helper.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	test/helper-test.c

helper_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(pango_CFLAGS)\
	$(x11_CFLAGS)\
	$(xinerama_CFLAGS)\
	$(cairo_CFLAGS)\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

helper_test_LDADD=\
	$(glib_LIBS)\
	$(pango_LIBS)\
	$(x11_LIBS)\
	$(xinerama_LIBS)\
	$(cairo_LIBS)

TESTS=\
	rofi_test\
	helper_test

.PHONY: test-x
test-x: $(bin_PROGRAMS) textbox_test
	echo "Test 1"
	$(top_srcdir)/test/run_test.sh 123 $(top_builddir)/textbox_test $(top_builddir)
	echo "Test 2"
	$(top_srcdir)/test/run_test.sh 200 $(top_srcdir)/test/run_errormsg_test.sh $(top_builddir)
	echo "Test 3"
	$(top_srcdir)/test/run_test.sh 201 $(top_srcdir)/test/run_switchdialog_test.sh $(top_builddir)
	echo "Test 4"
	$(top_srcdir)/test/run_test.sh 202 $(top_srcdir)/test/run_dmenu_test.sh $(top_builddir)
	echo "Test 5"
	$(top_srcdir)/test/run_test.sh 203 $(top_srcdir)/test/run_dmenu_custom_test.sh $(top_builddir)
	echo "Test 6"
	$(top_srcdir)/test/run_test.sh 204 $(top_srcdir)/test/run_run_test.sh $(top_builddir)
	echo "Test 7"
	$(top_srcdir)/test/run_test.sh 205 $(top_srcdir)/test/run_script_test.sh $(top_builddir)
	echo "Issue 256"
	$(top_srcdir)/test/run_test.sh 206 $(top_srcdir)/test/run_issue_256.sh $(top_builddir)
	echo "Issue 275"
	$(top_srcdir)/test/run_test.sh 207 $(top_srcdir)/test/run_issue_275.sh $(top_builddir)
	echo "Crash empty list"
	$(top_srcdir)/test/run_test.sh 208 $(top_srcdir)/test/run_dmenu_empty.sh $(top_builddir)
	echo "Test multiple select"
	$(top_srcdir)/test/run_test.sh 209 $(top_srcdir)/test/run_dmenu_issue_292.sh $(top_builddir)
	echo "Test screenshot"
	$(top_srcdir)/test/run_test.sh 210 $(top_srcdir)/test/run_screenshot_test.sh $(top_builddir)
	echo "Test daemon"
	$(top_srcdir)/test/run_test.sh 211 $(top_srcdir)/test/run_daemon_test.sh $(top_builddir)
	echo "Test xr dump"
	$(top_srcdir)/test/run_test.sh 212 $(top_srcdir)/test/xr_dump_test.sh $(top_builddir) $(top_srcdir)
	echo "Test drun"
	$(top_srcdir)/test/run_test.sh 213 $(top_srcdir)/test/run_drun_test.sh $(top_builddir)
	echo "Test combi"
	$(top_srcdir)/test/run_test.sh 214 $(top_srcdir)/test/run_combi_test.sh $(top_builddir)
	echo "Test dmenu regex"
	$(top_srcdir)/test/run_test.sh 215 $(top_srcdir)/test/run_regex_test.sh $(top_builddir)
	echo "Test dmenu glob"
	$(top_srcdir)/test/run_test.sh 216 $(top_srcdir)/test/run_glob_test.sh $(top_builddir)
	echo "Test dmenu fuzzy"
	$(top_srcdir)/test/run_test.sh 217 $(top_srcdir)/test/run_fuzzy_test.sh $(top_builddir)
	echo "Test config dump"
	$(top_srcdir)/test/run_test.sh 218 $(top_srcdir)/test/xr_config_test.sh $(top_builddir) $(top_srcdir)
	echo "Test issue 333"
	$(top_srcdir)/test/run_test.sh 221 $(top_srcdir)/test/run_issue333_test.sh $(top_builddir)

test-x1: $(bin_PROGRAMS)
	echo "Test dmenu-normal-window"
	$(top_srcdir)/test/run_test.sh 219 $(top_srcdir)/test/run_dmenu_normal_window_test.sh $(top_builddir)
	echo "Test window"
	$(top_srcdir)/test/run_test.sh 220 $(top_srcdir)/test/run_window_test.sh $(top_builddir) $(top_srcdir)
	echo "End tests"


.PHONY: indent
indent: $(rofi_SOURCES) $(top_srcdir)/config/config.def.c $(helper_test_SOURCES) $(textbox_test_SOURCES) $(rofi_test_SOURCES)
	uncrustify -c $(top_srcdir)/data/uncrustify.cfg --replace $^

.PHONY: cppcheck

cppcheck: $(rofi_SOURCES)
	cppcheck --std=c99 --platform=unix64 --enable=all -Uerror_dialog -I $(top_srcdir)/include/  $^

.PHONY: ohcount

ohcount: $(rofi_SOURCES)
	ohcount -i $(top_srcdir)/source/ $(top_srcdir)/include/

doxy: doc/rofi.doxy $(rofi_SOURCES)
	doxygen $(top_builddir)/doc/rofi.doxy

