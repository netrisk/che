.PHONY: all

CFLAGS+=-g -Wall -Werror

all: che ch8asm

che: che.c che_time.c che_time.h che_log.h che_scr.h che_rand.h \
     platform/linux/che_scr.c platform/linux/che_scr_platform.h \
     platform/linux/che_rand_linux.c \
     platform/linux/che_time_linux.c \
     platform/linux/che_log_linux.c che_machine.c che_cycle.c
	gcc ${CFLAGS} -o che che.c che_time.c che_machine.c che_cycle.c \
	    platform/linux/che_rand_linux.c \
	    platform/linux/che_scr.c  platform/linux/che_time_linux.c \
	    platform/linux/che_log_linux.c -I . -I platform/linux

ch8asm: ch8asm.o che_log.o

clean:
	rm -f che
	rm -f *.o
