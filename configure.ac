AC_PREREQ([2.69])
AC_INIT([wilqpaint], [0.1], [fatwildcat@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/wilqpaint.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
# GLIB_GSETTINGS - not yet

# Checks for libraries.
AC_CHECK_LIB([m], [fabs])
PKG_CHECK_MODULES(LIBGTK, [gtk+-3.0])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit floor modf sqrt strcasecmp strrchr])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
