CFLAGS= -g -I sws -lpcre -lsws -L sws
all:
	gcc src/parser.c -o spectrum $(CFLAGS)


