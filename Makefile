CFLAGS=-Wall -g
LIBS=-lcheck

all: linklayer.o

linklayer.o:
	gcc $(CFLAGS) src/alarm.c src/linklayer.c -o bin/linklayer.o

clean:
	rm ./bin/*
