CC=gcc
CFLAGS=-I../include/ -I../ -std=c99  -fprofile-arcs -ftest-coverage -g3
SOURCES=\
		history-test.c\
		../config/config.c\
		../source/history.c

history-test: $(SOURCES)
	$(CC) -o $@ $^ `pkg-config --libs --cflags glib-2.0`  $(CFLAGS)


test: history-test
	./$^
