.PHONY: test clean

pthreads: main.c
	gcc -o pthreads -lpthread main.c

test: pthreads
	./pthreads

clean:
	rm -r pthreads
