#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([cdtextinfo], [0.1], [andreas.frisch@multimedia-labs,de])
AC_CONFIG_SRCDIR([src/cdtextinfo.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([dist-bzip2])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([libcddb], [libcddb])
PKG_CHECK_MODULES([libcdio], [libcdio])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([strdup])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
