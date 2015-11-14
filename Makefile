.PHONY: all

all: che

che: che.c che_log.c che_log.h
	gcc -g -o che che.c che_log.c -Wall -Werror

clean:
	rm -f che
	rm -f *.o
