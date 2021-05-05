# Specify automake version.
AUTOMAKE_OPTIONS  = 1.11.3

ACLOCAL_AMFLAGS = -I subprojects/libnkutils -I subprojects/libgwater ${ACLOCAL_FLAGS}

AM_YFLAGS = -d

noinst_LIBRARIES =
EXTRA_DIST=
noinst_PROGRAMS=
check_PROGRAMS=
TESTS=
include $(top_srcdir)/subprojects/libnkutils/libnkutils-nolibtool.mk
include $(top_srcdir)/libgwater-xcb-nolibtool.mk

##
# PKG CONFIG file
##
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/rofi.pc


BUILT_SOURCES=\
			  lexer/theme-parser.h\
			  lexer/theme-parser.c\
			  lexer/theme-lexer.c\
			  resources/resources.c\
			  resources/resources.h

$(top_builddir)/resources/resources.c: $(top_srcdir)/resources/resources.xml
			mkdir -p $(top_builddir)/resources/
			$(GLIB_COMPILE_RESOURCES) $<  --generate-source --target=$@ --sourcedir=$(top_srcdir)

$(top_builddir)/resources/resources.h: $(top_srcdir)/resources/resources.xml
			mkdir -p $(top_builddir)/resources/
			$(GLIB_COMPILE_RESOURCES) $<  --generate-header --target=$@ --sourcedir=$(top_srcdir)

$(top_builddir)/lexer/theme-lexer.c: $(top_srcdir)/lexer/theme-lexer.l

$(top_builddir)/lexer/theme-parser.c $(top_builddir)/lexer/theme-parser.h: $(top_srcdir)/lexer/theme-parser.y

rofiincludedir=${includedir}/rofi
rofiinclude_HEADERS=\
			include/mode.h\
			include/mode-private.h\
			include/helper.h\
			include/rofi-types.h\
			include/rofi-icon-fetcher.h

##
# Rofi the program
##
bin_PROGRAMS=rofi

dist_bin_SCRIPTS=\
	script/rofi-sensible-terminal\
	script/rofi-theme-selector

nodist_rofi_SOURCES=\
	$(top_builddir)/gitconfig.h

SOURCES=\
	source/rofi.c\
	source/view.c\
	source/mode.c\
	source/keyb.c\
	config/config.c\
	source/helper.c\
	source/timings.c\
	source/history.c\
	source/theme.c\
	source/rofi-types.c\
	source/rofi-icon-fetcher.c\
	source/widgets/box.c\
	source/widgets/container.c\
	source/widgets/icon.c\
	source/widgets/widget.c\
	source/widgets/textbox.c\
	source/widgets/listview.c\
	source/widgets/scrollbar.c\
	source/xrmoptions.c\
	source/xcb.c\
	source/css-colors.c\
	source/dialogs/run.c\
	source/dialogs/ssh.c\
	source/dialogs/drun.c\
	source/dialogs/dmenu.c\
	source/dialogs/combi.c\
	source/dialogs/window.c\
	source/dialogs/script.c\
	source/dialogs/help-keys.c\
	source/dialogs/filebrowser.c\
	include/display.h\
	include/xcb.h\
	include/xcb-internal.h\
	include/rofi.h\
	include/rofi-types.h\
	include/rofi-icon-fetcher.h\
	include/mode.h\
	include/mode-private.h\
	include/settings.h\
	include/keyb.h\
	include/view.h\
	include/view-internal.h\
	include/helper.h\
	include/helper-theme.h\
	include/timings.h\
	include/history.h\
	include/theme.h\
	include/css-colors.h\
	include/widgets/box.h\
	include/widgets/container.h\
	include/widgets/icon.h\
	include/widgets/widget.h\
	include/widgets/widget-internal.h\
	include/widgets/textbox.h\
	include/widgets/listview.h\
	include/widgets/scrollbar.h\
	include/xrmoptions.h\
	include/dialogs/ssh.h\
	include/dialogs/run.h\
	include/dialogs/drun.h\
	include/dialogs/dmenu.h\
	include/dialogs/combi.h\
	include/dialogs/script.h\
	include/dialogs/window.h\
	include/dialogs/dialogs.h\
	include/dialogs/help-keys.h\
	include/dialogs/filebrowser.h\
	include/dialogs/dmenuscriptshared.h\
	resources/resources.c\
	resources/resources.h

rofi_SOURCES=\
			 lexer/theme-parser.y\
			 lexer/theme-lexer.l\
			 resources/resources.xml\
			 $(SOURCES)

rofi_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(NKUTILS_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(pango_CFLAGS)\
	$(libsn_CFLAGS)\
	$(cairo_CFLAGS)\
	$(gdkpixbuf_CFLAGS)\
	-DMANPAGE_PATH="\"$(mandir)/\""\
	-I$(top_srcdir)/include/\
	-I$(top_builddir)/lexer/\
	-I$(top_srcdir)/lexer/\
	-I$(top_builddir)/resources/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/\
	-Werror=missing-prototypes\
	-DSYSCONFDIR=\"$(sysconfdir)\"\
	-DTHEME_CONVERTER\
	-DPLUGIN_PATH=\"${libdir}/rofi\"\
	-DTHEME_DIR=\"$(themedir)\"

rofi_LDADD=\
	$(glib_LIBS)\
	$(NKUTILS_LIBS)\
	$(GW_XCB_LIBS)\
	$(libsn_LIBS)\
	$(pango_LIBS)\
	$(cairo_LIBS)\
	$(gdkpixbuf_LIBS)\
	$(LIBS)

##
# Manpage
##
dist_man1_MANS=\
	doc/rofi.1\
	doc/rofi-theme-selector.1\
	doc/rofi-sensible-terminal.1

dist_man5_MANS=\
	doc/rofi-theme.5\
	doc/rofi-script.5

EXTRA_DIST += \
	doc/rofi-theme.5.markdown \
	doc/rofi-script.5.markdown \
	doc/rofi-theme-selector.1.markdown \
	doc/rofi-sensible-terminal.1.markdown \
	doc/rofi.1.markdown


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

###
# Themes
###
themedir=$(pkgdatadir)/themes/
theme_DATA=\
		   themes/Adapta-Nokto.rasi\
		   themes/Arc.rasi\
		   themes/Arc-Dark.rasi\
		   themes/DarkBlue.rasi\
		   themes/Pop-Dark.rasi\
		   themes/Indego.rasi\
		   themes/Monokai.rasi\
		   themes/Paper.rasi\
		   themes/android_notification.rasi\
		   themes/arthur.rasi\
		   themes/blue.rasi\
		   themes/c64.rasi\
		   themes/dmenu.rasi\
		   themes/docu.rasi\
		   themes/glue_pro_blue.rasi\
		   themes/gruvbox-common.rasi\
		   themes/gruvbox-dark-hard.rasi\
		   themes/gruvbox-dark-soft.rasi\
		   themes/gruvbox-dark.rasi\
		   themes/gruvbox-light-hard.rasi\
		   themes/gruvbox-light-soft.rasi\
		   themes/gruvbox-light.rasi\
		   themes/lb.rasi\
		   themes/paper-float.rasi\
		   themes/purple.rasi\
		   themes/sidebar.rasi\
		   themes/solarized.rasi\
		   themes/solarized_alternate.rasi\
		   themes/fancy.rasi

##
# Extra DIST
##
EXTRA_DIST+=\
	$(markdown_FILES)\
	$(markdown_SC_FILES)\
	Examples/i3_switch_workspaces.sh\
	Examples/i3_empty_workspace.sh\
	Examples/test_script_mode.sh\
	Examples/rofi-file-browser.sh\
	INSTALL.md\
	AUTHORS\
	doc/rofi.doxy.in\
	script/get_git_rev.sh\
	$(theme_DATA)\
	doc/default_theme.rasi\
	Changelog
##
# Indent
##


update-manpage: $(top_srcdir)/doc/rofi-theme-selector.1.markdown $(top_srcdir)/doc/rofi.1.markdown $(top_srcdir)/doc/rofi-theme.5.markdown $(top_srcdir)/doc/rofi-script.5.markdown ${top_srcdir}/doc/rofi-sensible-terminal.1.markdown
	go-md2man -in $(top_srcdir)/doc/rofi.1.markdown -out $(top_srcdir)/doc/rofi.1
	go-md2man -in $(top_srcdir)/doc/rofi-theme-selector.1.markdown -out $(top_srcdir)/doc/rofi-theme-selector.1
	go-md2man -in $(top_srcdir)/doc/rofi-theme.5.markdown -out $(top_srcdir)/doc/rofi-theme.5
	go-md2man -in $(top_srcdir)/doc/rofi-script.5.markdown -out $(top_srcdir)/doc/rofi-script.5
	go-md2man -in $(top_srcdir)/doc/rofi-sensible-terminal.1.markdown -out $(top_srcdir)/doc/rofi-sensible-terminal.1

##
# Rofi test program
##
check_PROGRAMS+=\
			   history_test\
			   textbox_test\
			   helper_test\
			   helper_expand\
			   helper_pidfile\
			   helper_config_cmdline_parser\
			   widget_test\
			   box_test\
			   scrollbar_test

if USE_CHECK
check_PROGRAMS+=mode_test theme_parser_test helper_tokenize
endif



history_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(NKUTILS_CFLAGS)\
	$(libsn_CFLAGS)\
	$(cairo_CFLAGS)\
	-DTHEME_DIR=\"$(themedir)\"\
	-DPLUGIN_PATH=\"${libdir}/rofi\"\
	-I$(top_srcdir)/include/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

history_test_LDADD=\
	$(glib_LIBS)\
	$(NKUTILS_LIBS)


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
	$(NKUTILS_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(cairo_CFLAGS)\
	$(libsn_CFLAGS)\
	$(gdkpixbuf_CFLAGS)\
	-DPLUGIN_PATH=\"${libdir}/rofi\"\
	-DTHEME_DIR=\"$(themedir)\"\
	-I$(top_srcdir)/include/\
	-I$(top_builddir)/lexer/\
	-I$(top_srcdir)/lexer/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

textbox_test_LDADD=\
	$(glib_LIBS)\
	$(pango_LIBS)\
	$(NKUTILS_LIBS)\
	$(GW_XCB_LIBS)\
	$(cairo_LIBS)\
	$(libsn_LIBS)

helper_pidfile_CFLAGS=$(textbox_test_CFLAGS)
helper_pidfile_LDADD=$(textbox_test_LDADD)
helper_pidfile_SOURCES=\
					   config/config.c\
					   include/rofi.h\
					   include/mode.h\
					   include/mode-private.h\
					   source/helper.c\
					   source/rofi-types.c\
					   include/rofi-types.h\
					   include/helper.h\
					   include/helper-theme.h\
					   include/xrmoptions.h\
					   source/xrmoptions.c\
					   test/helper-pidfile.c

widget_test_LDADD=$(textbox_test_LDADD)
widget_test_CFLAGS=$(textbox_test_CFLAGS)
widget_test_SOURCES=\
			source/widgets/widget.c\
			source/widgets/textbox.c\
			source/theme.c\
			source/rofi-types.c\
			include/rofi-types.h\
			source/css-colors.c\
			source/helper.c\
			config/config.c\
			lexer/theme-parser.y\
			lexer/theme-lexer.l\
			test/widget-test.c

box_test_LDADD=$(textbox_test_LDADD)
box_test_CFLAGS=$(textbox_test_CFLAGS)
box_test_SOURCES=\
			source/widgets/widget.c\
			source/widgets/box.c\
			lexer/theme-parser.y\
			lexer/theme-lexer.l\
			source/theme.c\
			source/rofi-types.c\
			include/rofi-types.h\
			source/css-colors.c\
			include/theme.h\
			include/css-colors.h\
      config/config.c\
			test/box-test.c

scrollbar_test_LDADD=$(textbox_test_LDADD)
scrollbar_test_CFLAGS=$(textbox_test_CFLAGS)
scrollbar_test_SOURCES=\
			source/widgets/widget.c\
			source/widgets/scrollbar.c\
			lexer/theme-parser.y\
			lexer/theme-lexer.l\
			source/theme.c\
			source/rofi-types.c\
			include/rofi-types.h\
			source/css-colors.c\
			include/theme.h\
			include/css-colors.h\
      config/config.c\
			test/scrollbar-test.c

textbox_test_SOURCES=\
	source/widgets/widget.c\
	source/widgets/textbox.c\
	lexer/theme-parser.y\
	lexer/theme-lexer.l\
	source/theme.c\
	source/rofi-types.c\
	include/rofi-types.h\
	source/css-colors.c\
	source/helper.c\
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
	include/helper-theme.h\
	test/textbox-test.c

if USE_CHECK
theme_parser_test_CFLAGS=${helper_test_CFLAGS} $(check_CFLAGS)
theme_parser_test_LDADD=${helper_test_LDADD} $(check_LIBS)
theme_parser_test_SOURCES=\
	config/config.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	source/helper.c\
	include/helper.h\
	include/helper-theme.h\
	include/theme.h\
	include/css-colors.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	lexer/theme-lexer.c\
	lexer/theme-parser.c\
	lexer/theme-parser.h\
	source/theme.c\
	source/rofi-types.c\
	include/rofi-types.h\
	source/css-colors.c\
	test/theme-parser-test.c
endif

helper_test_SOURCES=\
	config/config.c\
	include/rofi.h\
	include/mode.h\
	include/mode-private.h\
	source/helper.c\
	include/helper.h\
	include/helper-theme.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	source/rofi-types.c\
	include/rofi-types.h\
	test/helper-test.c


helper_test_CFLAGS=\
	$(AM_CFLAGS)\
	$(glib_CFLAGS)\
	$(pango_CFLAGS)\
	$(NKUTILS_CFLAGS)\
	$(GW_XCB_CFLAGS)\
	$(cairo_CFLAGS)\
	$(gdkpixbuf_CFLAGS)\
	$(libsn_CFLAGS)\
	-DPLUGIN_PATH=\"${libdir}/rofi\"\
	-DTHEME_DIR=\"$(themedir)\"\
	-I$(top_srcdir)/include/\
	-I$(top_builddir)/lexer/\
	-I$(top_srcdir)/lexer/\
	-I$(top_srcdir)/config/\
	-I$(top_builddir)/

helper_test_LDADD=\
	$(glib_LIBS)\
	$(pango_LIBS)\
	$(NKUTILS_LIBS)\
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
	include/helper-theme.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	source/rofi-types.c\
	include/rofi-types.h\
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
	source/rofi-types.c\
	include/rofi-types.h\
	include/helper.h\
	include/helper-theme.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
	test/helper-config-cmdline-parser.c


if USE_CHECK
mode_test_CFLAGS=$(textbox_test_CFLAGS) $(check_CFLAGS)
mode_test_LDADD=$(textbox_test_LDADD) $(check_LIBS)
mode_test_SOURCES=\
				  config/config.c\
				  test/mode-test.c\
			      source/dialogs/help-keys.c\
				  source/helper.c\
				  source/mode.c\
				  source/rofi-types.c\
				  include/rofi-types.h\
				  source/xrmoptions.c\
				  source/keyb.c\
				  include/mode.h\
				  include/mode-private.h
helper_tokenize_CFLAGS=$(textbox_test_CFLAGS) $(check_CFLAGS)
helper_tokenize_LDADD=$(textbox_test_LDADD) $(check_LIBS)
helper_tokenize_SOURCES=\
					   config/config.c\
					   include/rofi.h\
					   include/mode.h\
					   include/mode-private.h\
					   source/helper.c\
					   source/rofi-types.c\
					   include/rofi-types.h\
					   include/helper.h\
					   include/helper-theme.h\
					   include/xrmoptions.h\
					   source/xrmoptions.c\
					   test/helper-tokenize.c

endif

TESTS+=\
	history_test\
	helper_test\
	helper_expand\
	helper_pidfile\
	helper_config_cmdline_parser\
	textbox_test\
	widget_test\
	box_test\
	scrollbar_test

if USE_CHECK
TESTS+=theme_parser_test\
	helper_tokenize\
	mode_test
endif

.PHONY: test-x
test-x: $(bin_PROGRAMS)
	MESON_SOURCE_ROOT="$(top_srcdir)" \
	MESON_BUILD_ROOT="$(top_builddir)" \
	$(top_srcdir)/test/run_all_tests.sh


.PHONY: indent
indent: $(SOURCES)
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
	-rm $(top_builddir)/resources/resources.h
	-rm $(top_builddir)/resources/resources.c

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

EXTRA_DIST += \
	doc/meson.build \
	subprojects/libnkutils/meson.build \
	subprojects/libnkutils/meson_options.txt \
	subprojects/libgwater/xcb/meson.build \
	meson_options.txt \
	meson.build
