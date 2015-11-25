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
  darwin* )
        AC_MSG_NOTICE([Darwin platform detected])
        #Im not sure about this way. Maybe there is a better way
        AM_CONDITIONAL(DARWIN_PLATFORM, true)
        AM_CONDITIONAL(LINUX_PLATFORM, false)
        ;;
  linux*)
        AC_MSG_NOTICE([Linux platform detected])
        AM_CONDITIONAL(LINUX_PLATFORM, true)
        AM_CONDITIONAL(DARWIN_PLATFORM, false)
        ;;
    *)
        #Default Case
        AC_MSG_ERROR([Your platform is not currently supported])
        ;;
esac

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
#AC_CHECK_LIB([SDL2], [main],[],[AC_MSG_ERROR([SDL2 library is not installed])])

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
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memset strrchr])

AC_OUTPUT(Makefile src/Makefile)
