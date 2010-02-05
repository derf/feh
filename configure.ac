dnl Process this file with autoconf to create configure.

AC_INIT
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE(feh, 1.3.4)
AM_CONFIG_HEADER(src/config.h)

AC_PROG_CC
AM_PROG_CC_STDC
AC_C_CONST

AC_PROG_INSTALL
AC_PROG_MAKE_SET

AM_MAINTAINER_MODE
AM_WITH_DMALLOC

AC_ARG_WITH(imlib2,
        [  --with-imlib2=DIR       use imlib2 in <DIR>],
        [CFLAGS="$CFLAGS -I$withval/include"
        LIBS="-L$withval/lib $LIBS"])

AC_PATH_GENERIC(imlib2, 1.0.0, [
  AC_SUBST(IMLIB_LIBS)
  AC_SUBST(IMLIB_CFLAGS) ],
  AC_MSG_ERROR(Cannot find imlib2: Is imlib2-config in the path?) )
IMLIB_LIBS=`imlib2-config --libs`
IMLIB_CFLAGS=`imlib2-config --cflags`
AC_SUBST(IMLIB_LIBS)
AC_SUBST(IMLIB_CFLAGS)

AC_PATH_GENERIC(giblib, 1.2.4, [
  AC_SUBST(GIBLIB_LIBS)
  AC_SUBST(GIBLIB_CFLAGS) ],
  AC_MSG_ERROR(Cannot find giblib: Is giblib-config in the path?) )

dnl the above doesn't work for some reason :/
GIBLIB_LIBS=`giblib-config --libs`
GIBLIB_CFLAGS=`giblib-config --cflags`
AC_SUBST(GIBLIB_LIBS)
AC_SUBST(GIBLIB_CFLAGS)

dnl Check for Xinerama
CFLAGS="-I/usr/X11R6/include $CFLAGS"
LIBS="-L/usr/X11R6/lib $LIBS -lm -ljpeg"
AC_CHECK_LIB(Xext,XMissingExtension,, [CFLAGS="$CFLAGS -I/usr/X11R6/include"])
AC_CHECK_LIB(Xinerama,XineramaQueryScreens,, [CFLAGS="$CFLAGS -I/usr/X11R6/include"])
AC_SUBST(HAVE_XINERAMA)
AC_SUBST(XINERAMA_LIBS)
AC_CHECK_LIB(jpeg, jpeg_destroy_decompress)

dnl AC_CHECK_FUNC(getopt_long,,[AC_LIBOBJ="$AC_LIBOBJ getopt.o getopt1.o"])
dnl AC_SUBST(AC_LIBOBJ)

echo "checking for pr0n.... found"

AC_CONFIG_FILES([Makefile \
feh.spec \
cam/Makefile \
src/Makefile \
src/ttfonts/Makefile \
])
AC_OUTPUT

echo ""
echo "If you have used an earlier version of feh,"
echo "please remove your ~/.fehrc config file."
echo "feh will create a new one when it is first run."
