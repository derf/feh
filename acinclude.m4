dnl @synopsis AC_PATH_GENERIC(LIBRARY [, MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl
dnl Runs a LIBRARY-config script and defines LIBRARY_CFLAGS and LIBRARY_LIBS
dnl
dnl The script must support `--cflags' and `--libs' args.
dnl If MINIMUM-VERSION is specified, the script must also support the
dnl `--version' arg.
dnl If the `--with-library-[exec-]prefix' arguments to ./configure are given,
dnl it must also support `--prefix' and `--exec-prefix'.
dnl (In other words, it must be like gtk-config.)
dnl
dnl For example:
dnl
dnl    AC_PATH_GENERIC(Foo, 1.0.0)
dnl
dnl would run `foo-config --version' and check that it is at least 1.0.0
dnl
dnl If so, the following would then be defined:
dnl
dnl    FOO_CFLAGS to `foo-config --cflags`
dnl    FOO_LIBS   to `foo-config --libs`
dnl
dnl At present there is no support for additional "MODULES" (see AM_PATH_GTK)
dnl (shamelessly stolen from gtk.m4 and then hacked around a fair amount)
dnl
dnl @author Angus Lees <gusl@cse.unsw.edu.au>
dnl @version $Id: acinclude.m4,v 1.1.1.1 2004/08/24 19:12:09 gilbertt Exp $

AC_DEFUN(AC_PATH_GENERIC,
[dnl
dnl we're going to need uppercase, lowercase and user-friendly versions of the
dnl string `LIBRARY'
pushdef([UP], translit([$1], [a-z], [A-Z]))dnl
pushdef([DOWN], translit([$1], [A-Z], [a-z]))dnl

dnl
dnl Get the cflags and libraries from the LIBRARY-config script
dnl
AC_ARG_WITH(DOWN-prefix,[  --with-]DOWN[-prefix=PFX       Prefix where $1 is installed (optional)],
        DOWN[]_config_prefix="$withval", DOWN[]_config_prefix="")
AC_ARG_WITH(DOWN-exec-prefix,[  --with-]DOWN[-exec-prefix=PFX Exec prefix where $1 is installed (optional)],
        DOWN[]_config_exec_prefix="$withval", DOWN[]_config_exec_prefix="")

  if test x$DOWN[]_config_exec_prefix != x ; then
     DOWN[]_config_args="$DOWN[]_config_args --exec-prefix=$DOWN[]_config_exec_prefix"
     if test x${UP[]_CONFIG+set} != xset ; then
       UP[]_CONFIG=$DOWN[]_config_exec_prefix/bin/DOWN-config
     fi
  fi
  if test x$DOWN[]_config_prefix != x ; then
     DOWN[]_config_args="$DOWN[]_config_args --prefix=$DOWN[]_config_prefix"
     if test x${UP[]_CONFIG+set} != xset ; then
       UP[]_CONFIG=$DOWN[]_config_prefix/bin/DOWN-config
     fi
  fi

  AC_PATH_PROG(UP[]_CONFIG, DOWN-config, no)
  ifelse([$2], ,
     AC_MSG_CHECKING(for $1),
     AC_MSG_CHECKING(for $1 - version >= $2)
  )
  no_[]DOWN=""
  if test "$UP[]_CONFIG" = "no" ; then
     no_[]DOWN=yes
  else
     UP[]_CFLAGS="`$UP[]_CONFIG $DOWN[]_config_args --cflags`"
     UP[]_LIBS="`$UP[]_CONFIG $DOWN[]_config_args --libs`"
     ifelse([$2], , ,[
        DOWN[]_config_major_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
        DOWN[]_config_minor_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
        DOWN[]_config_micro_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
        DOWN[]_wanted_major_version="regexp($2, [\<\([0-9]*\)], [\1])"
        DOWN[]_wanted_minor_version="regexp($2, [\<\([0-9]*\)\.\([0-9]*\)], [\2])"
        DOWN[]_wanted_micro_version="regexp($2, [\<\([0-9]*\).\([0-9]*\).\([0-9]*\)], [\3])"

        # Compare wanted version to what config script returned.
        # If I knew what library was being run, i'd probably also compile
        # a test program at this point (which also extracted and tested
        # the version in some library-specific way)
        if test "$DOWN[]_config_major_version" -lt \
                        "$DOWN[]_wanted_major_version" \
          -o \( "$DOWN[]_config_major_version" -eq \
                        "$DOWN[]_wanted_major_version" \
            -a "$DOWN[]_config_minor_version" -lt \
                        "$DOWN[]_wanted_minor_version" \) \
          -o \( "$DOWN[]_config_major_version" -eq \
                        "$DOWN[]_wanted_major_version" \
            -a "$DOWN[]_config_minor_version" -eq \
                        "$DOWN[]_wanted_minor_version" \
            -a "$DOWN[]_config_micro_version" -lt \
                        "$DOWN[]_wanted_micro_version" \) ; then
          # older version found
          no_[]DOWN=yes
          echo -n "*** An old version of $1 "
          echo -n "($DOWN[]_config_major_version"
          echo -n ".$DOWN[]_config_minor_version"
          echo    ".$DOWN[]_config_micro_version) was found."
          echo -n "*** You need a version of $1 newer than "
          echo -n "$DOWN[]_wanted_major_version"
          echo -n ".$DOWN[]_wanted_minor_version"
          echo    ".$DOWN[]_wanted_micro_version."
          echo "***"
          echo "*** If you have already installed a sufficiently new version, this error"
          echo "*** probably means that the wrong copy of the DOWN-config shell script is"
          echo "*** being found. The easiest way to fix this is to remove the old version"
          echo "*** of $1, but you can also set the UP[]_CONFIG environment to point to the"
          echo "*** correct copy of DOWN-config. (In this case, you will have to"
          echo "*** modify your LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf"
          echo "*** so that the correct libraries are found at run-time)"
        fi
     ])
  fi
  if test "x$no_[]DOWN" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$3], , :, [$3])
  else
     AC_MSG_RESULT(no)
     if test "$UP[]_CONFIG" = "no" ; then
       echo "*** The DOWN-config script installed by $1 could not be found"
       echo "*** If $1 was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the UP[]_CONFIG environment variable to the"
       echo "*** full path to DOWN-config."
     fi
     UP[]_CFLAGS=""
     UP[]_LIBS=""
     ifelse([$4], , :, [$4])
  fi
  AC_SUBST(UP[]_CFLAGS)
  AC_SUBST(UP[]_LIBS)

  popdef([UP])
  popdef([DOWN])
])
