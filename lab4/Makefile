.PHONY: test clean


mychmod: main.c
	gcc -o mychmod main.c

test: mychmod
	touch file.txt
	chmod 644 file.txt
	ls -lah file.txt
	./mychmod +x file.txt
	ls -lah file.txt
	./mychmod u-r file.txt
	ls -lah file.txt
	./mychmod g+rw file.txt
	ls -lah file.txt
	./mychmod 766 file.txt
	ls -lah file.txt
	rm file.txt

clean:
	rm -r mychmod
