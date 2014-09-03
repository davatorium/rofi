SUBDIRS=test

##
# Rofi the program
##
bin_PROGRAMS=rofi

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

rofi_SOURCES=\
    source/rofi.c\
    source/textbox.c\
    source/xrmoptions.c\
    source/dmenu-dialog.c\
    source/run-dialog.c\
    source/ssh-dialog.c\
    source/script-dialog.c\
    source/history.c\
    config/config.c\
	source/helper.c\
    include/rofi.h\
    include/run-dialog.h\
    include/ssh-dialog.h\
    include/dmenu-dialog.h\
    include/script-dialog.h\
    include/xrmoptions.h\
    include/history.h\
    include/textbox.h\
	include/helper.h

##
# Manpage
##
man1_MANS=\
    doc/rofi.1

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
    $(man1_MANS)\
    $(markdown_FILES)\
    $(markdown_SC_FILES)\
    Examples/i3_switch_workspaces.sh\
    Examples/brightness_rofi.sh\
    Examples/domo_rofi_script.sh\
    INSTALL.md

##
# Indent
##

indent: ${rofi_SOURCES}
	uncrustify -c ${top_srcdir}/data/uncrustify.cfg --replace $^

update-manpage: ${top_srcdir}/doc/rofi-manpage.markdown
	md2man-roff $^ > ${top_srcdir}/doc/rofi.1 

.PHONY: test
test: rofi
	make -C test/ test
