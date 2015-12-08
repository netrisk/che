# che

The CHIP-8 Emulator project.

According to [Wikipedia's article](https://en.wikipedia.org/wiki/CHIP-8) of
CHIP-8:
*"CHIP-8 is an interpreted programming language, developed by Joseph Weisbecker.
It was initially used on the COSMAC VIP and Telmac 1800 8-bit microcomputers in
the mid-1970s."*

This project is an interpreter/emulator of CHIP-8 coded just for fun which aims
to be executed in many platforms. Right now only GNU/Linux and MacOS X are
supported but support for Windows will come. We have in our minds to port it to
an Arduino board too.

## Compilation

Go to the main directory and type the following:

1) `autoconf`

This produces two files: autom4te.cache and configure. The first one is a
directory used for speeding up the job of autotools, and may be removed when
releasing the package. The latter is the shell script called by final users.

2) `aclocal`

This generates a file aclocal.m4 that contains macros for automake things.

3) `automake --add-missing`

Automake now reads configure.ac and the top-level Makefile.am, interprets them
(e.g. see further work has to be done in some subdirectories) and, for each
Makefile.am produces a Makefile.in. The argument --add-missing tells automake to
provide default scripts for reporting errors, installing etc, so it can be
omitted in the next runs.

4) `autoconf`

This produces the final, full-featured configure shell script.

5) `./configure`

6) `make`

## Execution

Chip 8 Comes with an example dummy chip 8 executable.

To execute example dummy file type:
	`./src/che tests/loop.ch8`

## CHIP-8 ROMs

* [CHIP-8 Games Pack from Zophar's Domain](http://www.zophar.net/pdroms/chip8/chip-8-games-pack.html):
A pack of simple yet amusing 15 CHIP-8 games.

* [Compilation of CHIP-8, SuperChip and MegaChip8 games and demos ](http://www.chip8.com/?page=109):
Compilation of many programs, games and demos. We still don't support neither
SuperChip nor MegaChip.
