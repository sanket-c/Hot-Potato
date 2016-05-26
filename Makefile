CC = gcc

all: player master

player: 
		$(CC) -o player player.c

master: 
		$(CC) -o master master.c

clean:
	    rm -rf *.o player master