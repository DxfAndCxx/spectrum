all:
	gcc lua-monip.c -o monip.so -shared -I $(PREFIX)/include/luajit-2.1 -fPIC -lluajit-5.1

install:
	cp monip.so $(PREFIX)/../modules
