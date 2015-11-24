# che

The CHIP-8 Emulator project.

To compile type:

1) autoconf
This produces two files: autom4te.cache and configure. The first one is a directory used for speeding up the job of autohell tools, and may be removed when releasing the package. The latter is the shell script called by final users.

2) aclocal
This generates a file aclocal.m4 that contains macros for automake things

3) automake --add-missing
Automake now reads configure.ac and the top-level Makefile.am, interprets them (e.g. see further work has to be done in some subdirectories) and, for each Makefile.am produces a Makefile.in. The argument --add-missing tells automake to provide default scripts for reporting errors, installing etc, so it can be omitted in the next runs.

4)autoconf
This produces the final, full-featured configure shell script.

5) ./configure
6) make
7) sudo make install


Chip 8 Comes with an example dummy chip 8 executable.

To execute example dummy file type:
	./che loop.ch8
