PREFIX=$(shell pwd)/sp

LDFLAGS= -lpcre -lpthread  -L $(PREFIX)/lib -ljansson -lluajit-5.1
CFLAGS= -I sws -I $(PREFIX)/include -I $(PREFIX)/include/luajit-2.1
LDFLAGS+= -g  -O0 -Wl,-rpath $(PREFIX)/lib


export PREFIX

objects += src/pattern.o
objects += src/record.o
objects += src/spectrum.o
objects += src/util.o
objects += src/server.o
objects += src/client.o
objects += src/splua.o

target=spectrum

sws=sws/libsws.a


all:$(objects) $(lua) $(sws) $(jansson) $(lua_cjson) $(PREFIX)
	gcc  $(objects) -o $(target) $(sws)  $(LDFLAGS)

$(sws):
	make -C sws

$(PREFIX):
	make -C deps



clean:
	find src -name '*.o' | xargs rm

run:
	./$(target)
