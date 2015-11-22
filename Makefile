.PHONY: all

CHE_INCLUDES=-I . -I platform/apple -I platform/linux
CHE_CFLAGS=-DCHE_TIME_APPLE ${CHE_INCLUDES}
CFLAGS+=-g -Wall -Werror ${CHE_CFLAGS}
PLATFORM=$(shell uname -s)

ifeq (${PLATFORM},Darwin)
PLATFORM_SOURCE=./platform/apple
else
PLATFORM_SOURCE=./platform/linux
endif

all: che ch8asm


che: che.o che_time.o che_log.o che_machine.o che_cycle.o ${PLATFORM_SOURCE}/che_time_platform.o che_scr.o che_rand.o


ch8asm: ch8asm.o che_log.o

clean:
	rm -f che
	rm -f *.o
