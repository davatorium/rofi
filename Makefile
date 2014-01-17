CFLAGS?=-Wall -Wextra -O3
PREFIX?=$(DESTDIR)/usr
BINDIR?=$(PREFIX)/bin
MANDIR?=$(PREFIX)/share/man/man1

# Check deps.
ifeq (${DEBUG},1)
CFLAGS+=-DTIMING=1
LDADD+=-lrt
endif

PKG_CONFIG?=$(shell which pkg-config)
ifeq (${PKG_CONFIG},${EMPTY})
$(error Failed to find pkg-config. Please install pkg-config)
endif

LDADD+=$(shell ${PKG_CONFIG} --cflags --libs x11 xinerama xft)

ifeq (${LDADD},${EMPTY})
$(error Failed to find the required dependencies: x11, xinerama, xft)
endif

I3?=$(shell which i3)
ifneq (${I3},${EMPTY})
$(info I3 mode is enabled)
CFLAGS+=-DI3 -I${PREFIX}/include/
endif

all: normal

normal:
	$(CC)  -o simpleswitcher simpleswitcher.c -std=c99 $(CFLAGS) $(LDADD) $(LDFLAGS)

debug:
	$(CC) -o simpleswitcher-debug simpleswitcher.c -std=c99 $(CFLAGS) -Wunused-parameter -g -DDEBUG $(LDADD) 

install: normal install-man
	install -Dm 755 simpleswitcher $(BINDIR)/simpleswitcher

install-man:
	install -Dm 644 simpleswitcher.1 $(MANDIR)/simpleswitcher.1
	gzip -f $(MANDIR)/simpleswitcher.1

clean:
	rm -f simpleswitcher simpleswitcher-debug


indent:
	@astyle --style=linux -S -C -D -N -H -L -W3 -f simpleswitcher.c textbox.c
