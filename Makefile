.PHONY: all

all: che

che: che.c
	gcc -g -o che che.c -Wall -Werror

clean:
	rm -f che
	rm -f *.o
