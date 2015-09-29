#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(vmpmemory, 1.0, quinncolm@optonline.net)
AM_INIT_AUTOMAKE([1.0 no-define])
AC_CONFIG_SRCDIR([src/vmpmemory.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
OLD_CXXFLAGS=$CXXFLAGS
OLD_CFLAGS=$CFLAGS
AC_PROG_CXX
AC_PROG_CC
CXXFLAGS=$OLD_CXXFLAGS
CFLAGS=$OLD_CFLAGS
LT_INIT

AC_CONFIG_MACRO_DIR([m4])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset socket])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 testsrc/Makefile])
AC_OUTPUT

