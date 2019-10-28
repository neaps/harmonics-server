# $Id: configure.ac 6743 2018-01-14 18:51:31Z flaterco $

# FIXME: Update version info on each release
AC_INIT([congen], [1.7], [dave@flaterco.com])

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_LANG([C++])
AC_PROG_CXX
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_SED

# Ensure that STL is present.  (Reportedly not found without CPPFLAGS
# under Mac OS X 10.4.9)
AC_CHECK_HEADER([map], [], [
  AC_MSG_NOTICE([[Not finding the Standard Template Library.]])
  AC_MSG_NOTICE([[Find a directory like /usr/include/c++/[...numbers...],]])
  AC_MSG_NOTICE([[then do ./configure CPPFLAGS="-I/usr/include/...".]])
  AC_MSG_ERROR([cannot find Standard Template Library; try setting CPPFLAGS.])
])

AC_CHECK_HEADER([tcd.h], [
  AC_CHECK_LIB([tcd], [get_tide_db_header], [AC_DEFINE([HAVE_LIBTCD])])
])
AM_CONDITIONAL(TCD_SWITCH, test "$ac_cv_lib_tcd_get_tide_db_header" == "yes")

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
