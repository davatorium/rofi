CFLAGS?=-Wall -Os
LDADD?=`pkg-config --cflags --libs x11 xinerama xft`
PREFIX?=$(DESTDIR)/usr
BINDIR?=$(PREFIX)/bin

all: normal

normal:
	$(CC) $(CFLAGS) $(LDADD) $(LDFLAGS) -o simpleswitcher simpleswitcher.c

debug:
	$(CC) $(CFLAGS) -Wunused-parameter -g -DDEBUG $(LDADD) -o simpleswitcher-debug simpleswitcher.c

install:
	install -Dm 755 simpleswitcher $(BINDIR)/simpleswitcher

clean:
	rm -f simpleswitcher simpleswitcher-debug
