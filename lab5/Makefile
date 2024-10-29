.PHONY: test clean


archiver: main.c
	gcc -o archiver main.c

test: archiver
	./archiver -h
	echo "Text" > file.txt
	chmod 756 file.txt
	chown 1000:1001	file.txt
	./archiver archive -s
	ls -lah archive file.txt
	./archiver archive -i Makefile
	./archiver archive -i file.txt
	./archiver archive -i main.c
	ls -lah archive file.txt
	rm file.txt
	./archiver archive -s
	./archiver archive -e file.txt
	./archiver archive -s
	ls -lah archive file.txt
	rm file.txt
	rm archive

clean:
	rm -r archiver
