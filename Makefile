CFLAGS?=-Wall -Wextra -O3

PROGRAM=simpleswitcher


PREFIX?=$(DESTDIR)/usr
BINDIR?=$(PREFIX)/bin
MANDIR?=$(PREFIX)/share/man/man1

BUILD_DIR=build
SOURCE_DIR=source
DOC_DIR=doc

SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
OBJECTS=$(SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)
HEADERS=$(wildcard include/*.h)

MANPAGE_PATH=$(MANDIR)/simpleswitcher.1.gz

CFLAGS+=-DMANPAGE_PATH="\"$(MANPAGE_PATH)\""
CFLAGS+=-std=c99
CFLAGS+=-Iinclude/

# Check deps.
ifeq (${DEBUG},1)
CFLAGS+=-DTIMING=1 -g3
LDADD+=-lrt
endif


##
# Check dependencies
##
PKG_CONFIG?=$(shell which pkg-config)
ifeq (${PKG_CONFIG},${EMPTY})
$(error Failed to find pkg-config. Please install pkg-config)
endif

CFLAGS+=$(shell ${PKG_CONFIG} --cflags x11 xinerama xft)
LDADD+=$(shell ${PKG_CONFIG} --libs x11 xinerama xft)

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
	mkdir -p $@
# Objects depend on header files and makefile too.

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c | Makefile $(HEADERS) $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/$(PROGRAM): $(OBJECTS)
	$(CC) -o $@ $^  $(LDADD) $(LDFLAGS)

install: $(BUILD_DIR)/$(PROGRAM) install-man
	install -Dm 755 $(BUILD_DIR)/$(PROGRAM) $(BINDIR)/$(PROGRAM)

install-man:
	install -Dm 644 $(DOC_DIR)/simpleswitcher.1 $(MANDIR)/simpleswitcher.1
	gzip -f $(MANDIR)/simpleswitcher.1

clean:
	rm -rf $(BUILD_DIR)


indent:
	@astyle --style=linux -S -C -D -N -H -L -W3 -f $(SOURCES) $(HEADERS)
