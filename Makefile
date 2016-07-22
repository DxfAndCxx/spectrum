CFLAGS= -g -I sws -lpcre -lsws -L sws -DTEST
objects=src/complie.o src/record.o
target=spectrum

all:$(objects)
	gcc $(objects) -o $(target) $(CFLAGS)


