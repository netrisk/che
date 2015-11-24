# Aditional automake flags
AUTOMAKE_OPTIONS=foreign

# Aditional aclocal flags
# what flags you want to pass to the C compiler & linker
# TODO: Do this SDL stuff properly
#AM_CFLAGS=-DCHE_CFG_SDL -D_REENTRANT -I/usr/include/SDL2 -lSDL2
AM_CFLAGS=
AM_CPPFLAGS=

SRCFILES=che.c\
		che_time_linux.c\
		che_time.c\
		che_log.c\
		che_machine.c\
		che_cycle.c\
		che_io_console.c\
		che_io_sdl.c\
		che_rand.c

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = che 
che_SOURCES = $(SRCFILES) 
