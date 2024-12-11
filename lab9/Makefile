.PHONY: test clean all

all: sender reciever 

sender: sender.c
	gcc -o sender -lrt sender.c

reciever: reciever.c
	gcc -o reciever -lrt reciever.c

test:
	echo "execute manually"

clean:
	rm -r sender reciever
