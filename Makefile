CFLAGS?=-Wall -Os 
PREFIX?=$(DESTDIR)/usr
BINDIR?=$(PREFIX)/bin
MANDIR?=$(PREFIX)/share/man/man1

# Check deps.

PKG_CONFIG?=$(shell which pkg-config)
ifeq (${PKG_CONFIG},${EMPTY})
$(error Failed to find pkg-config. Please install pkg-config)
endif

LDADD?=$(shell ${PKG_CONFIG} --cflags --libs x11 xinerama xft)

ifeq (${LDADD},${EMPTY})
$(error Failed to find the required dependencies: x11, xinerama, xft)
endif

all: normal

normal:
	$(CC)  -o simpleswitcher simpleswitcher.c -std=c99 $(CFLAGS) $(LDADD) $(LDFLAGS)

debug:
	$(CC) -o simpleswitcher-debug simpleswitcher.c -std=c99 $(CFLAGS) -Wunused-parameter -g -DDEBUG $(LDADD) 

install: install-man
	install -Dm 755 simpleswitcher $(BINDIR)/simpleswitcher

install-man:
	install -Dm 644 simpleswitcher.1 $(MANDIR)
	gzip $(MANDIR)/simpleswitcher.1

clean:
	rm -f simpleswitcher simpleswitcher-debug
