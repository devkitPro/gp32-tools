#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([gp32tools],[1.0.4],[https://github.com/devkitPro/gp32-tools/issues])
AC_CONFIG_SRCDIR([src/gpd/main.c])
#AM_INIT_AUTOMAKE([1.10])

AM_INIT_AUTOMAKE([subdir-objects])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX

AM_PROG_CC_C_O

ZLIB_MINIMUM=1.2.8
PKG_CHECK_MODULES(ZLIB, [zlib >= $ZLIB_MINIMUM], [found_zlib=yes], [found_zlib=no])
if test "x$found_zlib" = "xno" ; then
   AC_CHECK_LIB(z, inflate,
       [AC_CHECK_HEADER([zlib.h],
       [AC_COMPILE_IFELSE(
           [AC_LANG_PROGRAM([[
               #include <zlib.h>
               #if !defined(ZLIB_VERNUM) || (ZLIB_VERNUM < 0x1230)
               // compile error
               #endif
          ]],[])],
          [found_zlib=yes])])])
   if test "x$found_zlib" = "xno" ; then
      AC_MSG_ERROR([Working zlib library and headers not found])
   fi
   ZLIBS_LIBS='-z'
   AC_SUBST(ZLIB_LIBS)
fi
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
