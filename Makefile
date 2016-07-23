LUAFLAGS= -lm -ldl
CFLAGS= -g -I sws -I luajit/src/ -lpcre -lsws -L sws $(LUAFLAGS)
objects=src/complie.o src/record.o src/spectrum.o src/util.o
target=spectrum
lua=luajit/src/libluajit.a

all:$(objects) $(lua)
	gcc -o $(target) $(objects) $(lua) $(CFLAGS)


$(lua):
	make -C luajit


run:
	./$(target)
