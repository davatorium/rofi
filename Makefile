CFLAGS?=-Wall -O2
LDADD?=$(shell pkg-config --cflags --libs x11 xinerama x11 xft)

all: normal

normal:
	$(CC) $(CFLAGS) $(LDADD) $(LDFLAGS) -o simpleswitcher simpleswitcher.c

debug:
	$(CC) $(CFLAGS) -Wunused-parameter -g -DDEBUG $(LDADD) -o simpleswitcher-debug simpleswitcher.c

clean:
	rm -f simpleswitcher simpleswitcher-debug
