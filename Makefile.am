# Specify automake version.
AUTOMAKE_OPTIONS  = 1.11.3

##
# Rofi the program
##
bin_PROGRAMS=rofi

dist_bin_SCRIPTS=script/rofi-sensible-terminal

LIBS=\
    @xft_LIBS@\
    @x11_LIBS@\
    @xinerama_LIBS@\
	@libsn_LIBS@\
	@pango_LIBS@

AM_CFLAGS=\
	@EXTRA_CFLAGS@\
    @xft_CFLAGS@\
    @x11_CFLAGS@\
    @xinerama_CFLAGS@\
    @pango_CFLAGS@\
	@libsn_CFLAGS@\
    -DMANPAGE_PATH="\"$(mandir)/\""\
    -I$(top_srcdir)/include/\
    -I$(top_srcdir)/config/\
    -I$(top_builddir)/\
	-Werror=missing-prototypes

rofi_SOURCES=\
    source/rofi.c\
	source/keyb.c\
    config/config.c\
	source/helper.c\
    source/textbox.c\
    source/history.c\
    source/scrollbar.c\
	source/i3-support.c\
    source/xrmoptions.c\
	source/x11-helper.c\
    source/dialogs/run.c\
    source/dialogs/ssh.c\
    source/dialogs/dmenu.c\
    source/dialogs/combi.c\
    source/dialogs/window.c\
    source/dialogs/script.c\
    include/rofi.h\
	include/keyb.h\
	include/helper.h\
    include/history.h\
    include/textbox.h\
    include/scrollbar.h\
    include/xrmoptions.h\
    include/i3-support.h\
	include/x11-helper.h\
    include/dialogs/ssh.h\
    include/dialogs/run.h\
    include/dialogs/dmenu.h\
    include/dialogs/combi.h\
    include/dialogs/script.h\
    include/dialogs/window.h\
    include/dialogs/dialogs.h

##
# Manpage
##
man1_MANS=\
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
${top_builddir}/config/config.c: ${top_srcdir}/config/config.def.c
	cp ${top_srcdir}/config/config.def.c $@

## 
# Extra DIST
##
EXTRA_DIST=\
    $(man1_MANS)\
    $(markdown_FILES)\
    $(markdown_SC_FILES)\
    Examples/i3_switch_workspaces.sh\
    Examples/brightness_rofi.sh\
    Examples/domo_rofi_script.sh\
    ${top_srcdir}/config/config.def.c\
    INSTALL.md\
	AUTHORS\
	Changelog

##
# Indent
##


update-manpage: ${top_srcdir}/doc/rofi-manpage.markdown
	ronn --roff --pipe $^ > ${top_srcdir}/doc/rofi.1

##
# Rofi test program
##
noinst_PROGRAMS=rofi_test textbox_test helper_test


rofi_test_SOURCES=\
    source/history.c\
    config/config.c\
    include/rofi.h\
    include/history.h\
	test/history-test.c

textbox_test_SOURCES=\
    source/textbox.c\
    config/config.c\
	source/keyb.c\
	source/x11-helper.c\
	source/xrmoptions.c\
	source/helper.c\
	include/keyb.h\
    include/rofi.h\
    include/textbox.h\
	include/x11-helper.h\
	include/xrmoptions.h\
	include/helper.h\
	test/textbox-test.c

helper_test_SOURCES=\
    config/config.c\
    include/rofi.h\
    source/helper.c\
    include/helper.h\
	include/xrmoptions.h\
	source/xrmoptions.c\
    test/helper-test.c

.PHONY: test
test: ${bin_PROGRAMS} 
	./rofi_test
	./helper_test

.PHONY: test-x
test-x: ${bin_PROGRAMS}
	echo "Test 1"	
	$(top_srcdir)/test/run_test.sh 123 $(top_builddir)/textbox_test $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "Test 2"
	$(top_srcdir)/test/run_test.sh 200 $(top_srcdir)/test/run_errormsg_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "Test 3"
	$(top_srcdir)/test/run_test.sh 201 $(top_srcdir)/test/run_switchdialog_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "Test 4"
	$(top_srcdir)/test/run_test.sh 202 $(top_srcdir)/test/run_dmenu_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "Test 5"
	$(top_srcdir)/test/run_test.sh 203 $(top_srcdir)/test/run_dmenu_custom_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "Test 6"
	$(top_srcdir)/test/run_test.sh 204 $(top_srcdir)/test/run_run_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "Test 7"
	$(top_srcdir)/test/run_test.sh 205 $(top_srcdir)/test/run_script_test.sh $(top_builddir) $(top_srcdir)/doc/example.xresources
	echo "End tests"


.PHONY: indent
indent: ${rofi_SOURCES} ${top_srcdir}/config/config.def.c ${helper_test_SOURCES} ${textbox_test_SOURCES} ${rofi_test_SOURCES}
	uncrustify -c ${top_srcdir}/data/uncrustify.cfg --replace $^

.PHONY: cppcheck

cppcheck: ${rofi_SOURCES}
	cppcheck --std=c99 --platform=unix64 --enable=all -Uerror_dialog -I ${top_srcdir}/include/  $^

.PHONY: ohcount

ohcount: ${rofi_SOURCES}
	ohcount -i ${top_srcdir}/source/ 
