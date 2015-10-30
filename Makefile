CFLAGS=-Wall -g
LIBS=-lcheck

all: linklayer.o

linklayer.o:
	mkdir bin -p
	gcc $(CFLAGS) src/alarm.c src/linklayer.c src/applicationlayer.c -o bin/linklayer.o

clean:
	rm ./bin/*
