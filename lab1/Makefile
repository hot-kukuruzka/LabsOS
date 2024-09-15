.PHONY: test clean


myls: main.c
	gcc -o myls main.c

test:
	./myls
	./myls -a
	./myls -l
	./myls -la
	./myls -l -a
	./myls ..
	./myls .. -l
	./myls .. -la
	./myls -a ../.. -l

clean:
	rm -r myls
