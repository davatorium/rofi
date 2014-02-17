QUIET?=@
CFLAGS?=-Wall -Wextra -O3 -g 
VERSION?=0.14.2

PROGRAM=simpleswitcher


PREFIX?=$(DESTDIR)/usr
BIN_DIR?=$(PREFIX)/bin
MAN_DIR?=$(PREFIX)/share/man/man1

MAN_PAGE=$(PROGRAM).1

SOURCE_DIR=source
CONFIG_DIR=config
DOC_DIR=doc
BUILD_DIR=build

SOURCES=$(wildcard $(SOURCE_DIR)/*.c $(CONFIG_DIR)/*.c )
OBJECTS=$(SOURCES:%.c=$(BUILD_DIR)/%.o)
HEADERS=$(wildcard include/*.h)
OTHERS=Makefile LICENSE README.md

INSTALL_MANPAGE_PATH=$(MAN_DIR)/$(MAN_PAGE).gz
INSTALL_PROGRAM=$(BIN_DIR)/$(PROGRAM)


DIST_TARGET=$(BUILD_DIR)/$(PROGRAM)-$(VERSION).tar.xz


CFLAGS+=-DMANPAGE_PATH="\"$(INSTALL_MANPAGE_PATH)\""
CFLAGS+=-std=c99
CFLAGS+=-Iinclude/
CFLAGS+=-DVERSION="\"$(VERSION)\""

# Check deps.
ifeq (${DEBUG},1)
CFLAGS+=-DTIMING=1 -g3
LDADD+=-lrt
endif

ifeq (${QC_MODE},1)
CFLAGS+=-D__QC_MODE__
endif


CLANG=$(shell which clang 2>/dev/null)

ifneq (${CLANG},${EMPTY})
    $(info Using clang compiler: ${CLANG})
    CC=${CLANG}
endif


##
# Check dependencies
##
PKG_CONFIG?=$(shell which pkg-config)
ifeq (${PKG_CONFIG},${EMPTY})
$(error Failed to find pkg-config. Please install pkg-config)
endif

CFLAGS+=$(shell ${PKG_CONFIG} --cflags x11 xinerama xft libxdg-basedir )
LDADD+=$(shell ${PKG_CONFIG} --libs x11 xinerama xft libxdg-basedir )

ifeq (${LDADD},${EMPTY})
$(error Failed to find the required dependencies: x11, xinerama, xft)
endif


##
# Check for i3.
##
I3?=$(shell which i3)
ifneq (${I3},${EMPTY})
$(info I3 mode is enabled)
CFLAGS+=-DI3 -I${PREFIX}/include/
endif

all: $(BUILD_DIR)/$(PROGRAM)

$(BUILD_DIR):
	$(info Creating build dir)
	$(QUIET)mkdir -p $@
	$(QUIET)mkdir -p $@/$(SOURCE_DIR)
	$(QUIET)mkdir -p $@/$(CONFIG_DIR)

# Objects depend on header files and makefile too.
$(BUILD_DIR)/%.o: %.c Makefile $(HEADERS) | $(BUILD_DIR)
	$(info Compiling $< -> $@)
	$(QUIET) $(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(PROGRAM): $(OBJECTS)
	$(info Linking   $@)
	$(QUIET)$(CC) -o $@ $^  $(LDADD) $(LDFLAGS)

install: $(INSTALL_PROGRAM) $(INSTALL_MANPAGE_PATH) 

$(INSTALL_PROGRAM): $(BUILD_DIR)/$(PROGRAM)
	$(info Install   $^ -> $@ )
	$(QUIET)install -Dm 755 $^ $@ 


$(BUILD_DIR)/$(MAN_PAGE).gz: $(DOC_DIR)/$(MAN_PAGE)
	$(info Creating  man page)
	$(QUIET) gzip -c $^ > $@

$(INSTALL_MANPAGE_PATH): $(BUILD_DIR)/$(MAN_PAGE).gz
	$(info Install   $^ -> $@)
	$(QUIET) install -Dm 644 $^ $(INSTALL_MANPAGE_PATH) 

clean:
	$(info Clean build dir)
	$(QUIET)rm -rf $(BUILD_DIR)


indent:
	@astyle --style=linux -S -C -D -N -H -L -W3 -f $(SOURCES) $(HEADERS)

dist: $(DIST_TARGET)

$(BUILD_DIR)/$(PROGRAM)-$(VERSION): $(SOURCES) $(HEADERS) $(DOC_DIR)/$(MAN_PAGE) $(OTHERS) | $(BUILD_DIR)
	$(info Create release directory)
	$(QUIET)mkdir -p $@
	$(QUIET)cp -a --parents $^ $@


$(DIST_TARGET): $(BUILD_DIR)/$(PROGRAM)-$(VERSION) 
	$(info Creating release tarball: $@)
	$(QUIET) tar -C $(BUILD_DIR) -cavvJf $@ $(PROGRAM)-$(VERSION)
