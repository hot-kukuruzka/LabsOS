.PHONY: test clean

pthreads_condvar: main.c
	gcc -o pthreads_condvar -lpthread main.c

test: pthreads_condvar
	./pthreads_condvar

clean:
	rm -r pthreads_condvar
