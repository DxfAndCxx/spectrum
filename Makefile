LUAFLAGS= -lm -ldl
LDFLAGS= -lpcre -lpthread $(LUAFLAGS)
CFLAGS= -g -I sws -I luajit/src/
objects=src/pattern.o src/record.o src/spectrum.o src/util.o  src/server.o
target=spectrum
lua=luajit/src/libluajit.a
sws=sws/libsws.a

all:$(objects) $(lua) $(sws)
	gcc -o $(target) $^ $(LDFLAGS)


$(lua):
	make -C luajit

$(sws):
	make -C sws

clean:
	find src -name '*.o' | xargs rm

run:
	./$(target)
