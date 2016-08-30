all:
	gcc ipip.c -o ipip.so -shared -I $(PREFIX)/include/luajit-2.1 -fPIC -lluajit-5.1 -L$(PREFIX)/lib

install:
	cp ipip.so $(PREFIX)/../
