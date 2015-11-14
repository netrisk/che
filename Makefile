.PHONY: all

all: che

che: che.c che_log.c che_log.h che_scr.h \
     platform/linux/che_scr.c platform/linux/che_scr_platform.h
	gcc -g -o che che.c che_log.c platform/linux/che_scr.c  -I . -I platform/linux -Wall -Werror

clean:
	rm -f che
	rm -f *.o
