.PHONY: all

all: che

che: che.c che_log.c che_time.c che_time.h che_log.h che_scr.h \
     platform/linux/che_scr.c platform/linux/che_scr_platform.h
	gcc -g -o che che.c che_log.c che_time.c platform/linux/che_scr.c  -I . -I platform/linux -Wall -Werror

ch8asm: ch8asm.o

clean:
	rm -f che
	rm -f *.o
