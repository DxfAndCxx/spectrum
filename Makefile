LUAFLAGS= -lm -ldl
LDFLAGS= -lpcre -lpthread $(LUAFLAGS)
CFLAGS= -I sws -I luajit/src/ -I jansson/jansson/include/
CFLAGS+= -g  -O0

objects += src/pattern.o
objects += src/record.o
objects += src/spectrum.o
objects += src/util.o
objects += src/server.o
objects += src/client.o
objects += src/splua.o

target=spectrum

lua=luajit/src/libluajit.a
sws=sws/libsws.a
jansson=jansson/jansson/lib/libjansson.a

all:$(objects) $(lua) $(sws) $(jansson)
	make -C sws
	gcc -o $(target) $^ $(LDFLAGS)


$(lua):
	make -C luajit

$(jansson):
	cd jansson; ./configure --prefix=$(shell pwd)/jansson
	make -C jansson
	make -C jansson install


clean:
	find src -name '*.o' | xargs rm

run:
	./$(target)
