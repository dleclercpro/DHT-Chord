CC=gcc

all: peer

peer: peer.c
	$(CC) -o peer peer.c -lm

clean:
	rm peer