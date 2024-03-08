dump: dump.c
	gcc dump.c -lversion -o dump.exe
	
clean:
	rm -rf dump.exe
