#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([che], [0.1])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
# Check for which host we are on and setup a few things
# specifically based on the host
case $host_os in
  darwin*)
        AC_MSG_NOTICE([Darwin platform detected])
        AC_DEFINE([CHE_DARWIN],[],[Compiling under MacOsX])
        ;;
  linux*)
        AC_MSG_NOTICE([Linux platform detected])
        AC_DEFINE([CHE_LINUX],[],[Compiling under Linux])
        ;;
  *mingw32*)
        AC_MSG_NOTICE([Windows platform detected])
        AC_DEFINE([CHE_WINDOWS],[],[Compiling for Windows])
        ;;
  *)
        #Default Case
        AC_MSG_ERROR([Your platform is not currently supported])
        ;;
esac

AC_PATH_PROG(SDL2_CONFIG, [sdl2-config])
if test -z "$SDL2_CONFIG"; then
	AC_MSG_ERROR([I can't found the sdl2-config script])
fi
AC_SUBST(SDL_CFLAGS,[`$SDL2_CONFIG --cflags`])
AC_SUBST(SDL_LDFLAGS,[`$SDL2_CONFIG --libs`])

# Checks for programs.
AC_PROG_CC

#Check if SDL is disabled or not
AC_ARG_ENABLE([sdl],
			  [AS_HELP_STRING([--disable-sdl],[Disable SDL support])],
			  [disable_sdl=yes],
			  [disable_sdl=no])

# SDL functionality
AC_MSG_CHECKING([Configuration: SDL enable/disable])
case x$disable_sdl in
xno) AC_MSG_RESULT([no]) 
	AC_CHECK_LIB(SDL2,SDL_Init,[], [AC_MSG_ERROR([SDL2 has not been detected])], [`$SDL2_CONFIG --libs`])
	AC_DEFINE([CHE_USE_SDL],[],[Enable SDL])

	;;
xyes) AC_MSG_RESULT([yes]) ;;
*)  AC_MSG_ERROR([Invalid: $enable_sdl]) ;;
esac

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h mach/mach.h stdint.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([clock_gettime memset strrchr])

AC_OUTPUT(Makefile src/Makefile)

