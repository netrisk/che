# Aditional automake flags
AUTOMAKE_OPTIONS=foreign

# Aditional aclocal flags
# what flags you want to pass to the C compiler & linker
AM_CFLAGS=
AM_CPPFLAGS=

SRCFILES=che.c\
		che_time_linux.c\
		che_time.c\
		che_log.c\
		che_machine.c\
		che_cycle.c\
		che_scr.c\
		che_rand.c

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = che 
che_SOURCES = $(SRCFILES) 
