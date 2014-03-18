##
# Rofi the program
##
bin_PROGRAMS=rofi

LIBS=\
    @xft_LIBS@\
    @x11_LIBS@\
    @xdgbasedir_LIBS@\
    @xinerama_LIBS@

AM_CFLAGS=\
    @xft_CFLAGS@\
    @x11_CFLAGS@\
    @xdgbasedir_CFLAGS@\
    @xinerama_CFLAGS@\
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
    config/config.c\
    include/rofi.h\
    include/run-dialog.h\
    include/ssh-dialog.h\
    include/dmenu-dialog.h\
    include/xrmoptions.h\
    include/textbox.h 

##
# Manpage
##
man1_MANS=\
    doc/rofi.1

##
# Readme.md
##
markdown_FILES=\
    README.md


## 
# Extra DIST
##
EXTRA_DIST=\
    $(man1_MANS)\
    $(markdown_FILES)
