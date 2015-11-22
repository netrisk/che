.PHONY: all

PLATFORM=$(shell uname -s)
ifeq (${PLATFORM},Darwin)
PLATFORM_DIR=./platform/apple
else
PLATFORM_DIR=./platform/linux
endif

CHE_INCLUDES=-I . -I ${PLATFORM_DIR}
CFLAGS+=-g -Wall -Werror ${CHE_INCLUDES}

all: che ch8asm che_time_test

che: che.o che_time.o che_log.o che_machine.o che_cycle.o ${PLATFORM_DIR}/che_time_platform.o che_scr.o che_rand.o

ch8asm: ch8asm.o che_log.o

che_time_test: che_time_test.o che_time.o ${PLATFORM_DIR}/che_time_platform.o

clean:
	rm -f che
	rm -f *.o
