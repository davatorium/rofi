CFLAGS?=-Wall -Os
PREFIX?=$(DESTDIR)/usr
BINDIR?=$(PREFIX)/bin

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
	$(CC)  -o simpleswitcher simpleswitcher.c $(CFLAGS) $(LDADD) $(LDFLAGS)

debug:
	$(CC) -o simpleswitcher-debug simpleswitcher.c $(CFLAGS) -Wunused-parameter -g -DDEBUG $(LDADD) 

install:
	install -Dm 755 simpleswitcher $(BINDIR)/simpleswitcher

clean:
	rm -f simpleswitcher simpleswitcher-debug
