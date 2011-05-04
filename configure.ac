#                                               -*- Autoconf -*-
# Copyright 2011 University Corporation for Atmospheric Research
#
# This file is part of the NOAAPORT package.  See the file COPYRIGHT
# in the top-level source-directory of the package for terms and
# conditions.
#
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(NOAAPORT, 1.6.2.6, support-noaaport@unidata.ucar.edu)
AC_CONFIG_SRCDIR([shmfifo.c])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

AM_CONDITIONAL([MAINTAINER],
    [test "$USER" = steve && test `uname -n` = gilda.unidata.ucar.edu &&
    test $PWD = $HOME/ldm/package/noaaport])

AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],[Turn on debugging support])],
[case "${enableval}" in
  yes)
    CFLAGS="-g${CFLAGS:+ $CFLAGS}"
    debug=true ;;
  no)
    CFLAGS="-O${CFLAGS:+ $CFLAGS}"
    debug=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],)
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

AC_ARG_ENABLE(max-size,
    AC_HELP_STRING([--disable-max-size], 
	[minimize the (long, void*, off_t) programming environment
	[[default=enable]]]),
    max_size=$enableval,
    max_size=yes)

AC_ARG_ENABLE([root-actions],
    AC_HELP_STRING([--disable-root-actions],
	[don't include superuser (i.e., "root") actions in "make install"
	[[default=include]]]),
    [
	case "$enableval" in
	yes) enable_include=true;;
	no) enable_include=false;;
	*) AC_MSG_ERROR(
	    [bad value {$enableval} for --enable-root-actions]);;
	esac
    ],
    [
	enable_include=true
    ])
AM_CONDITIONAL([INCLUDE_ROOT_ACTIONS], [test "x$enable_include" = "xtrue"])
AM_COND_IF([INCLUDE_ROOT_ACTIONS], 
    [if touch conftest && chmod u=rwxs conftest &&
	    ls -l conftest | grep -i rws >/dev/null; then \
	AC_CACHE_CHECK([for su(1) that you can use], [ac_cv_path_SU],
	    [AC_PATH_PROGS_FEATURE_CHECK([SU], [su],
		[echo "Enter root's password (or don't)." >/dev/tty
		$ac_path_SU root -c true </dev/tty >/dev/tty 2>&1 &&
                    ac_cv_path_SU=$ac_path_SU
                    ac_path_SU_found=:],
		[AC_MSG_RESULT([not found])
		AC_CACHE_CHECK([for sudo(1) that you can use],
                    [ac_cv_path_SUDO],
		    [AC_PATH_PROGS_FEATURE_CHECK([SUDO], [sudo],
			[echo "Enter your password (or don't)." >/dev/tty
			$ac_path_SUDO true >/dev/tty 2>&1 </dev/tty &&
                            ac_cv_path_SUDO=$ac_path_SUDO
                            ac_path_SUDO_found=:],
			[AC_MSG_RESULT([not found])])])])])
        [echo Continuing... >/dev/tty]
	AC_SUBST([SU], [$ac_cv_path_SU])
	AC_SUBST([SUDO], [$ac_cv_path_SUDO])
	if test -n "$SUDO" -o -n "$SU"; then \
	    include_root_actions=true
	else
	    include_root_actions=false
	fi
    else
	include_root_actions=false
    fi
    rm -f conftest],
    [include_root_actions=false])
AM_CONDITIONAL([INCLUDE_ROOT_ACTIONS],
    [test "$include_root_actions" = true])
AM_CONDITIONAL([HAVE_SUDO], [test -n "$SUDO"])
AM_CONDITIONAL([HAVE_SU], [test -n "$SU"])

# Ensure that compilation is optimized and with assertions disabled by default.
CFLAGS=${CFLAGS:--O}
CPPFLAGS=${CPPFLAGS:--DNDEBUG}

# Checks for programs.
AC_PROG_CC([c89 cc gcc])
AC_USE_SYSTEM_EXTENSIONS
AM_PROG_CC_C_O
if test -z "$CC"; then
    AC_MSG_ERROR([no C compiler found])
else
    AC_MSG_CHECKING(the C compiler)
    AC_COMPILE_IFELSE([int foo;], 
	AC_MSG_RESULT(works),
	AC_MSG_FAILURE($CC failed to compile test code))
fi
AC_PROG_LIBTOOL

# Checks for programming environment.
case "$CC" in
*c[[89]]9*)
    ARFLAGS=-cru
    if test "$max_size" = yes
    then
        AC_MSG_NOTICE(Checking how to maximize the (long, void*, off_t) 
                programming environment)
        success=no
        AC_MSG_CHECKING([if the system supports _XBS5_LPBIG_OFFBIG])
        avail=`getconf _XBS5_LPBIG_OFFBIG 2>/dev/null`
        if test $? = 0 -a "$avail" \!= -1 -a "$avail" \!= "undefined"
        then
            if cflags=`getconf XBS5_LPBIG_OFFBIG_CFLAGS 2>/dev/null`
            then
                if ldflags=`getconf XBS5_LPBIG_OFFBIG_LDFLAGS 2>/dev/null`
                then
                    if libs=`getconf XBS5_LPBIG_OFFBIG_LIBS 2>/dev/null`
                    then
                        success=yes
                    fi
                fi
            fi
        fi
        AC_MSG_RESULT($success)
        if test "$success" = no
        then
            AC_MSG_CHECKING([if the system supports _XBS5_LP64_OFF64])
            avail=`getconf _XBS5_LP64_OFF64 2>/dev/null`
            if test $? = 0 -a "$avail" \!= -1 -a "$avail" \!= "undefined"
            then
                if cflags=`getconf XBS5_LP64_OFF64_CFLAGS 2>/dev/null`
                then
                    if ldflags=`getconf XBS5_LP64_OFF64_LDFLAGS 2>/dev/null`
                    then
                        if libs=`getconf XBS5_LP64_OFF64_LIBS 2>/dev/null`
                        then
                            success=yes
                        fi
                    fi
                fi
            fi
            AC_MSG_RESULT($success)
        fi
        if test "$success" = no
        then
            AC_MSG_CHECKING([if the system supports _XBS5_ILP32_OFFBIG])
            avail=`getconf _XBS5_ILP32_OFFBIG 2>/dev/null`
            if test $? = 0 -a "$avail" \!= -1 -a "$avail" \!= "undefined"
            then
                if cflags=`getconf XBS5_ILP32_OFFBIG_CFLAGS 2>/dev/null`
                then
                    if ldflags=`getconf XBS5_ILP32_OFFBIG_LDFLAGS 2>/dev/null`
                    then
                        if libs=`getconf XBS5_ILP32_OFFBIG_LIBS 2>/dev/null`
                        then
                            success=yes
                        fi
                    fi
                fi
            fi
            AC_MSG_RESULT($success)
        fi
        if test "$success" = yes
        then
            test -n "$cflags"  && CFLAGS="$CFLAGS${CFLAGS+ }$cflags"
            test -n "$ldflags" && LDFLAGS="$LDFLAGS${LDFLAGS+ }$ldflags"
            test -n "$libs"    && LIBS="$LIBS${LIBS+ }$libs"
            test `uname` = AIX && ARFLAGS='-cru -X 32_64'
            AC_MSG_NOTICE([cflags="\"$cflags\""])
            AC_MSG_NOTICE([ldflags="\"$ldflags\""])
            AC_MSG_NOTICE([libs="\"$libs\""])
            AC_MSG_NOTICE([ARFLAGS="\"$ARFLAGS\""])
            AC_MSG_CHECKING([the C compiler (again)])
            AC_COMPILE_IFELSE([int foo;], 
                AC_MSG_RESULT(works),
                AC_MSG_FAILURE($CC failed to compile test code))
        fi
    fi
    AC_SUBST(ARFLAGS);;
esac

AC_MSG_CHECKING([the installation prefix]);
test -z "$prefix" -o "$prefix" = NONE && prefix=`(cd .. && pwd)`
#AM_COND_IF([MAINTAINER], [UD_PREFIX(..)], [UD_PREFIX(.)])
AC_MSG_RESULT([$prefix])

AC_MSG_CHECKING([LDMHOME]);
AM_COND_IF([MAINTAINER], [LDMHOME=$prefix],
    [test -z "$LDMHOME" && LDMHOME=$HOME])
AC_SUBST([LDMHOME])
AC_MSG_RESULT([$LDMHOME])

# Checks LDM version.
AC_MSG_CHECKING([LDM version]);
version=`ldmadmin config | grep version | awk '{print $3}'`
major=`echo $version | awk -F. '{print $1}'`
minor=`echo $version | awk -F. '{print $2}'`
AC_MSG_RESULT([$major.$minor])
if test \( $major -gt 6 \) -o \( $major -eq 6 -a $minor -ge 9 \); then
    AC_DEFINE([HAVE_GET_QUEUE_PATH], [1], 
        [Define to 1 if the LDM function getQueuePath() exists])
fi

# Checks for libraries.
AC_SEARCH_LIBS([sched_setscheduler], [rt])
AC_SEARCH_LIBS([socket], [socket], , 
    AC_MSG_ERROR([cannot find function socket()]))
AC_SEARCH_LIBS([gethostbyname], [nsl], , 
    AC_MSG_ERROR([cannot find function gethostbyname()]))

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h stdio.h unistd.h stdlib.h string.h sys/types.h \
        sys/ipc.h sys/shm.h sys/sem.h sys/stat.h sys/wait.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

AC_CONFIG_FILES([Makefile
                 mainpage.h
		 g2/Makefile
                 gempak/Makefile
                 tables/Makefile
                 zlib/Makefile])

export CC
AC_CONFIG_SUBDIRS([libpng])
#(cd libpng && ./configure --cache-file=../autom4te.cache --srcdir=$srcdir/libpng --prefix=$prefix CC="$CC")

AC_OUTPUT
