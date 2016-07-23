LUAFLAGS= -lm -ldl
LDFLAGS= -lpcre -lsws -lpthread
CFLAGS= -g -I sws -I luajit/src/ -L sws $(LUAFLAGS) $(LDFLAGS)
objects=src/complie.o src/record.o src/spectrum.o src/util.o
target=spectrum
lua=luajit/src/libluajit.a

all:$(objects) $(lua)
	gcc -o $(target) $(objects) $(lua) $(CFLAGS)


$(lua):
	make -C luajit

clean:
	find src -name '*.o' | xargs rm

run:
	./$(target)
