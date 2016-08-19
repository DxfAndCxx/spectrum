LUAFLAGS= -lm -ldl
LDFLAGS= -lpcre -lpthread $(LUAFLAGS)
CFLAGS= -I sws -I luajit/src/ -I jansson/src
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
lua_cjson=modules/lib/lua/5.1/cjson.so

all:$(objects) $(lua) $(sws) $(jansson) $(lua_cjson)
	make -C sws
	gcc -o $(target) $(objects) $(lua) $(sws) $(jansson) $(LDFLAGS)


$(lua):
	make -C luajit

$(jansson):
	cd jansson; ./configure --prefix=$$(pwd)/jansson
	make -C jansson
	make -C jansson install

$(lua_cjson):
	make -C lua-cjson PREFIX=../luajit/src/
	cp lua-cjson/cjson.so modules


clean:
	find src -name '*.o' | xargs rm

run:
	./$(target)
