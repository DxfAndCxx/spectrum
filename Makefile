LUAFLAGS= -lm -ldl
LDFLAGS= -lpcre -lpthread $(LUAFLAGS)
CFLAGS= -g -I sws -I luajit/src/

objects += src/pattern.o
objects += src/record.o
objects += src/spectrum.o
objects += src/util.o
objects += src/server.o
objects += src/client.o

target=spectrum
lua=luajit/src/libluajit.a
sws=sws/libsws.a

all:$(objects) $(lua) $(sws)
	make -C sws
	gcc -o $(target) $^ $(LDFLAGS)


$(lua):
	make -C luajit


clean:
	find src -name '*.o' | xargs rm

run:
	./$(target)
