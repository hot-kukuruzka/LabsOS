.PHONY: test clean

pthreads_rwlock: main.c
	gcc -o pthreads_rwlock -lpthread main.c

test: pthreads_rwlock
	./pthreads_rwlock

clean:
	rm -r pthreads_rwlock
