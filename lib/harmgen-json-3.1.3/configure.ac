# $Id: configure.ac 6977 2019-02-02 21:00:10Z flaterco $

# FIXME: Update version info on each release
AC_INIT([harmgen], [3.1.3], [dave@flaterco.com])

AM_INIT_AUTOMAKE([-Wall -Werror])

AC_LANG([C++])
AC_PROG_CXX
AC_PROG_SED

AC_CHECK_HEADER([Congen], [],
                [AC_MSG_ERROR([cannot find Congen header file; try setting CPPFLAGS.])])
AC_MSG_CHECKING([for libcongen (interfaceRevision_0)])
TEMPnWNUv1=$LIBS
LIBS="$LIBS -lcongen"
AC_LINK_IFELSE(
 [AC_LANG_PROGRAM([[#include <Congen>]],
                  [[Congen::interfaceRevision_0();]])],
  AC_MSG_RESULT([yes]),
 [AC_MSG_RESULT([NO])
  AC_MSG_ERROR([either cannot find libcongen or found wrong version; try setting LDFLAGS.])
 ])
LIBS=$TEMPnWNUv1

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
