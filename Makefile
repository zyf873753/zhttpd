

zhttpd: main.o threadpool.o util.o config.o
	gcc -g -o zhttpd main.o threadpool.o util.o config.o -lpthread
main.o: main.c debug.h
	gcc -W -Wall -g -c main.c
util.o: util.h util.c
	gcc -W -Wall -g -c util.c
threadpool.o: threadpool.c threadpool.h debug.h
	gcc -W -Wall -g -c threadpool.c
config.o: config.c config.h
	gcc -W -Wall -g -c config.c


.PHONY: clean cleanobj cleanexe
clean: cleanobj cleanexe
	
cleanobj:
	rm *.o
cleanexe:
	rm zhttpd


