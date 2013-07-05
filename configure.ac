dnl Process this file with autoconf to produce a configure script.
dnl This is a temporary configure.ac script for bellbird.
dnl See COPYING file for information on licenses.
dnl This is a significantly modified and cut down version of configure.in
dnl which appears in the flite package.
dnl The copyright notice on the original script was:
dnl########################################################################
dnl                                                                      ##
dnl                 Language Technologies Institute                      ##
dnl                    Carnegie Mellon University                        ##
dnl                     Copyright (c) 1999-2008                          ##
dnl                       All Rights Reserved.                           ##
dnl                                                                      ##
dnl Permission is hereby granted, free of charge, to use and distribute  ##
dnl this software and its documentation without restriction, including   ##
dnl without limitation the rights to use, copy, modify, merge, publish,  ##
dnl distribute, sublicense, and/or sell copies of this work, and to      ##
dnl permit persons to whom this work is furnished to do so, subject to   ##
dnl the following conditions:                                            ##
dnl  1. The code must retain the above copyright notice, this list of    ##
dnl     conditions and the following disclaimer.                         ##
dnl  2. Any modifications must be clearly marked as such.                ##
dnl  3. Original authors' names are not deleted.                         ##
dnl  4. The authors' names are not used to endorse or promote products   ##
dnl     derived from this software without specific prior written        ##
dnl     permission.                                                      ##
dnl                                                                      ##
dnl CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         ##
dnl DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      ##
dnl ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   ##
dnl SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      ##
dnl FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    ##
dnl WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   ##
dnl AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          ##
dnl ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       ##
dnl THIS SOFTWARE.                                                       ##
dnl                                                                      ##
dnl########################################################################

AC_INIT()

AC_CANONICAL_SYSTEM
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_CHECK_TOOL(AR, ar)
AC_C_BIGENDIAN

if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -D_FORTIFY_SOURCE=2 -fstack-protector --param ssp-buffer-size=4"
fi
AC_ARG_ENABLE( shared, 
               [  --disable-shared     without shared library support],
	       [case "${enableval}" in
                 yes) shared=true ;;
                 no) shared=false ;;
                 *) AC_MSG_ERROR(bad value ${enableval} for shared options) ;;
                 esac],
               [shared=false])

case "$target_cpu" in
     i386|i486|i586|i686)
	TARGET_CPU=i386
     ;;
     *)
        TARGET_CPU=$target_cpu
     ;;
esac     
TARGET_OS=$target_os

AC_SUBST(TARGET_OS)
AC_SUBST(TARGET_CPU)

AC_SUBST(LEXDEFS)

case "$host_cpu" in
     i386|i486|i586|i686)
	HOST_CPU=i386
     ;;
     *)
        HOST_CPU=$host_cpu
     ;;
esac     
HOST_OS=$host_os

AC_SUBST(HOST_OS)
AC_SUBST(HOST_CPU)

SHAREDLIBFLAGS=
withpic=false
AC_ARG_WITH( pic, [  --with-pic           with pic ],
        withpic=$with_pic )
if test "x$with_pic" = "xyes"; then
   case "$target_os" in
	linux*)
		CFLAGS="$CFLAGS -fPIC"
        ;;
	*)
		CFLAGS="$CFLAGS -fPIC"
	;;
   esac
fi

if test "$shared" = true; then
   case "$target_os" in
	linux*|k*bsd*-gnu|gnu*)
		SHAREDLIBFLAGS="-fPIC"
        ;;
	*)
	;;
   esac
fi
AC_SUBST(SHAREDLIBFLAGS)

dnl Does this compiler support initialization of unions
AC_TRY_COMPILE([#include <stdio.h>],
               [ struct a { union { float b; int c; } d; };
                 const struct a e = { .d={ .b=3.14 } }; ],
               [unioninit=yes],
               [unioninit=no])
if test "$unioninit" = no; then
   AC_DEFINE(NO_UNION_INITIALIZATION)
fi

dnl determine audio type or use none if none supported on this platform
AUDIODRIVER=none

# Seems the alsa code I have breaks on earlier versions
# I'm not sure what the threshold version is, but made this depend on
# the one I know -- you can still specific --with-audio=alsa
AC_TRY_COMPILE([#include <alsa/version.h>],
               [int j=
                #if SND_LIB_SUBMINOR >= 11
                3;
                #endif],
              [AUDIODRIVER="alsa"
	       AUDIODEFS=-DCST_AUDIO_ALSA
               AUDIOLIBS=-lasound])

dnl allow the user to override the one detected above
AC_ARG_WITH( audio,
	[  --with-audio          with specific audio support (none alsa) ],
        AUDIODRIVER=$with_audio
	AUDIODEFS="" )

if test "x$AUDIODEFS" = x; then
    case "$AUDIODRIVER" in
	alsa)
            AUDIODRIVER="alsa"
	    AUDIODEFS=-DCST_AUDIO_ALSA
            AUDIOLIBS=-lasound
	    ;;
	none)
            AUDIODRIVER="none"
	    AUDIODEFS=-DCST_AUDIO_NONE
            AUDIOLIBS=
	    ;;
    esac
fi
AC_SUBST(AUDIODRIVER)
AC_SUBST(AUDIODEFS)
AC_SUBST(AUDIOLIBS)

AC_CONFIG_FILES(config/config)
AC_OUTPUT
