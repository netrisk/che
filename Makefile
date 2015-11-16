.PHONY: all

CFLAGS+=-g -Wall -Werror

all: che ch8asm

che: che.c che_log.c che_time.c che_time.h che_log.h che_scr.h \
     platform/linux/che_scr.c platform/linux/che_scr_platform.h
	gcc ${CFLAGS} -o che che.c che_log.c che_time.c platform/linux/che_scr.c  -I . -I platform/linux

ch8asm: ch8asm.o che_log.o

clean:
	rm -f che
	rm -f *.o
