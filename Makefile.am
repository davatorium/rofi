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

nodist_rofi_SOURCES=\
	$(top_builddir)/gitconfig.h

rofi_SOURCES=\
	source/rofi.c\
	source/view.c\
	source/mode.c\
	source/keyb.c\
	config/config.c\
	source/helper.c\
	source/timings.c\
	source/history.c\
	source/widgets/box.c\
	source/widgets/widget.c\
	source/widgets/textbox.c\
	source/widgets/listview.c\
	source/widgets/scrollbar.c\
	source/widgets/separator.c\
	source/xrmoptions.c\
	source/x11-helper.c\
	source/dialogs/run.c\
	source/dialogs/ssh.c\
	source/dialogs/drun.c\
	source/dialogs/dmenu.c\
	source/dialogs/combi.c\
	source/dialogs/window.c\
	source/dialogs/script.c\
	source/dialogs/help-keys.c\
	include/xcb.h\
	include/xcb-internal.h\
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
	include/widgets/box.h\
	include/widgets/widget.h\
	include/widgets/widget-internal.h\
	include/widgets/textbox.h\
	include/widgets/listview.h\
	include/widgets/scrollbar.h\
	include/widgets/separator.h\
	include/xrmoptions.h\
	include/x11-helper.h\
	include/dialogs/ssh.h\
	include/dialogs/run.h\
	include/dialogs/drun.h\
	include/dialogs/dmenu.h\
	include/dialogs/combi.h\
	include/dialogs/script.h\
	include/dialogs/window.h\
	include/dialogs/dialogs.h\
	include/dialogs/help-keys.h\
	include/xkb.h\
	include/xkb-internal.h

rofi_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(pango_CFLAGS)\
	$(libsn_CFLAGS)\
	$(cairo_CFLAGS)\
	-DMANPAGE_PATH="\"$(mandir)/\""\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/\
	-Werror=missing-prototypes\
	-DSYSCONFDIR=\"$(sysconfdir)\"

rofi_LDADD=\
	$(glib_LIBS)\
	$(GW_XCB_LIBS)\
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
# Extra DIST
##
EXTRA_DIST=\
	$(markdown_FILES)\
	$(markdown_SC_FILES)\
	Examples/i3_switch_workspaces.sh\
	INSTALL.md\
	AUTHORS\
	doc/rofi.doxy.in\
	script/get_git_rev.sh\
	Changelog

##
# Indent
##


update-manpage: $(top_srcdir)/doc/rofi-manpage.markdown
	ronn --roff --pipe $^ > $(top_srcdir)/doc/rofi.1

##
# Rofi test program
##
check_PROGRAMS=history_test textbox_test helper_test helper_expand helper_config_cmdline_parser widget_test

history_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(libsn_CFLAGS)\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

history_test_LDADD=\
	$(glib_LIBS)


history_test_SOURCES=\
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
	$(GW_XCB_CFLAGS)\
	$(cairo_CFLAGS)\
	$(libsn_CFLAGS)\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

textbox_test_LDADD=\
	$(glib_LIBS)\
	$(pango_LIBS)\
	$(GW_XCB_LIBS)\
	$(cairo_LIBS)\
	$(libsn_LIBS)

widget_test_LDADD=$(textbox_test_LDADD)
widget_test_CFLAGS=$(textbox_test_CFLAGS)
widget_test_SOURCES=\
			source/widgets/widget.c\
			test/widget-test.c
textbox_test_SOURCES=\
	source/widgets/widget.c\
	source/widgets/textbox.c\
	config/config.c\
	include/keyb.h\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	include/settings.h\
	include/widgets/widget.h\
	include/widgets/widget-internal.h\
	include/widgets/textbox.h\
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
	source/x11-helper.c\
	test/helper-test.c

helper_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(pango_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(cairo_CFLAGS)\
	$(libsn_CFLAGS)\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

helper_test_LDADD=\
	$(glib_LIBS)\
	$(pango_LIBS)\
	$(GW_XCB_LIBS)\
	$(libsn_LIBS)\
	$(cairo_LIBS)

helper_expand_SOURCES=\
	config/config.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	source/helper.c\
	include/helper.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	source/x11-helper.c\
	test/helper-expand.c

helper_expand_CFLAGS=${helper_test_CFLAGS}

helper_expand_LDADD=${helper_test_LDADD}

helper_config_cmdline_parser_CFLAGS=${helper_test_CFLAGS}

helper_config_cmdline_parser_LDADD=${helper_test_LDADD}
helper_config_cmdline_parser_SOURCES=\
	config/config.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	source/helper.c\
	include/helper.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	source/x11-helper.c\
	test/helper-config-cmdline-parser.c

TESTS=\
	history_test\
	helper_test\
	helper_expand\
	helper_config_cmdline_parser\
	widget_test

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
	echo "Test config dump"
	$(top_srcdir)/test/run_test.sh 218 $(top_srcdir)/test/xr_config_test.sh $(top_builddir) $(top_srcdir)
	echo "Test issue 333"
	$(top_srcdir)/test/run_test.sh 221 $(top_srcdir)/test/run_issue333_test.sh $(top_builddir)
	echo "Test help output"
	$(top_srcdir)/test/run_test.sh 212 $(top_srcdir)/test/help_output_test.sh $(top_builddir) $(top_srcdir)

test-x1: $(bin_PROGRAMS)
	echo "Test dmenu-normal-window"
	$(top_srcdir)/test/run_test.sh 219 $(top_srcdir)/test/run_dmenu_normal_window_test.sh $(top_builddir)
	echo "Test window"
	$(top_srcdir)/test/run_test.sh 220 $(top_srcdir)/test/run_window_test.sh $(top_builddir) $(top_srcdir)
	echo "End tests"


.PHONY: indent
indent: $(rofi_SOURCES) $(helper_test_SOURCES) $(textbox_test_SOURCES) $(history_test_SOURCES) $(helper_expand_SOURCES)
	uncrustify -c $(top_srcdir)/data/uncrustify.cfg --replace $^

.PHONY: cppcheck

cppcheck: $(rofi_SOURCES)
	cppcheck --std=c99 --platform=unix64 --enable=all -Uerror_dialog --inconclusive -I $(top_srcdir)/include/  $^

.PHONY: ohcount

ohcount: $(rofi_SOURCES)
	ohcount -i $(top_srcdir)/source/ $(top_srcdir)/include/

doxy: doc/rofi.doxy $(rofi_SOURCES)
	doxygen $(top_builddir)/doc/rofi.doxy

clean-local:
	-rm $(top_builddir)/gitconfig.h

$(top_builddir)/gitconfig.h: .FORCE
	$(top_srcdir)/script/get_git_rev.sh $(top_srcdir) $(top_builddir)/gitconfig.h

$(rofi_SOURCES): $(top_builddir)/gitconfig.h

.PHONY: coverage
coverage: coverage/index.html

.PHONY: coverage-cli
coverage-cli: coverage.info
	lcov -l $^
coverage.info: $(top_builddir)/test/*.gcda $(top_builddir)/source/*.gcda  $(top_builddir)/source/**/*.gcda
	lcov --capture  --directory $(top_builddir) --output-file coverage.info
	lcov -r $@ /usr/include/\* -o $@

coverage/index.html: coverage.info
	genhtml $^ --output-directory coverage/

.PHONY: coverage-clean
coverage-clean:
	-rm -r coverage.info coverage/
	-find $(top_builddir) -name '*.gcda' | xargs rm

.PHONY: .FORCE
.FORCE:
