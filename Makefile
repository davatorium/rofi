CFLAGS?=-Wall -Wextra -O3
PREFIX?=$(DESTDIR)/usr
BINDIR?=$(PREFIX)/bin
MANDIR?=$(PREFIX)/share/man/man1

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:%.c=%.o)

MANPAGE_PATH=$(MANDIR)/simpleswitcher.1.gz

CFLAGS+=-DMANPAGE_PATH="\"$(MANPAGE_PATH)\""
CFLAGS+=-std=c99

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

all: normal


normal: $(OBJECTS) | Makefile
	$(CC)  -o simpleswitcher $^  $(LDADD) $(LDFLAGS)

install: normal install-man
	install -Dm 755 simpleswitcher $(BINDIR)/simpleswitcher

install-man:
	install -Dm 644 simpleswitcher.1 $(MANDIR)/simpleswitcher.1
	gzip -f $(MANDIR)/simpleswitcher.1

clean:
	rm -f simpleswitcher $(OBJECTS)


indent:
	@astyle --style=linux -S -C -D -N -H -L -W3 -f simpleswitcher.c textbox.c
