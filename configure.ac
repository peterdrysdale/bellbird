AC_PREREQ([2.62])
AC_INIT([bellbird], dnl Package name
        [0.1.8-unstable], dnl Package version
        [https://github.com/peterdrysdale/bellbird/issues] dnl Report bug address
       )
AC_CANONICAL_TARGET
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([include/config.h])
LT_PREREQ([2.2])

dnl Uncomment line corresponding to whether monolithic or shared libraries build is required
LT_INIT([disable-static, disable-shared])
dnl LT_INIT(disable-static, shared)]

AC_PROG_CC

dnl This flag accepts an argument of the form `current[:revision[:age]]'. So, passing `-version-info 3:12:1' sets current to 3, revision to 12, and age to 1.

dnl If either revision or age are omitted, they default to 0. Also note that age must be less than or equal to the current interface number.

dnl Here are a set of rules to help you update your library version information:

dnl   Start with version information of `0:0:0' for each libtool library.
dnl   Update the version information only immediately before a public 
dnl      release of your software. More frequent updates are unnecessary, 
dnl      and only guarantee that the current interface number gets 
dnl      larger faster.
dnl   If the library source code has changed at all since the last update, 
dnl      then increment revision (`c:r:a' becomes `c:r+1:a').
dnl   If any interfaces have been added, removed, or changed since the 
dnl      last update, increment current, and set revision to 0.
dnl   If any interfaces have been added since the last public release,
dnl      then increment age.
dnl   If any interfaces have been removed since the last public release,
dnl      then set age to 0. 

LIBBELLBIRD_VERSION="0:0:0"
AC_SUBST(LIBBELLBIRD_VERSION)
LIBBELLBIRD_CMULEX_VERSION="0:0:0"
AC_SUBST(LIBBELLBIRD_CMULEX_VERSION)
LIBBELLBIRD_USENGLISH_VERSION="0:0:0"
AC_SUBST(LIBBELLBIRD_USENGLISH_VERSION)

dnl 32bit linux systems need _FILE_OFFSET_BITS=64 to access
dnl files >2Gb. AC_SYS_LARGEFILE only sets _FILE_OFFSET_BITS
dnl as a #define in config.h. Thus config.h must be the absolutely first
dnl include in every *.c file which includes stdio.h. If
dnl it is accidentally left out at some stage in the future it
dnl can lead to complex to debug problems. We select a less brittle
dnl solution by forcing it as a command line option for the C compiler
dnl for affected systems.
AC_SYS_LARGEFILE
if test "x$ac_cv_sys_file_offset_bits" = x64; then
  CFLAGS=$CFLAGS" -D_FILE_OFFSET_BITS=64"
fi

dnl determine audio type or use none if none supported on this platform
AUDIODRIVER=none

# Seems the alsa code I have breaks on versions earlier than 1.0.11
# I'm not sure what the threshold version is, but made this depend on
# the one I know -- you can still specific --with-audio=alsa
AC_MSG_CHECKING([whether ALSA version is new enough])
AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[#include <alsa/version.h>]],
         [[
            #if !( ((SND_LIB_MAJOR == 1) && (SND_LIB_MINOR == 0) && (SND_LIB_SUBMINOR >= 11) ) || \
                   (SND_LIB_MAJOR > 1) || \
                   ((SND_LIB_MAJOR == 1) && (SND_LIB_MINOR > 0)) )
            #error too old
            #endif
         ]])],
         [AUDIODRIVER="alsa"
          AUDIOLIBS=-lasound],
         )

if test "$AUDIODRIVER" = "alsa"; then
   AC_MSG_RESULT([yes])
else
   AC_MSG_RESULT([no])
fi

dnl allow the user to override the one detected above
AC_ARG_WITH( audio,
	[  --with-audio          with specific audio support (none alsa) ],
        AUDIODRIVER=$with_audio
	)

case "$AUDIODRIVER" in
    alsa)
        AUDIODRIVER="alsa"
        AC_DEFINE([CST_AUDIO_ALSA], [], [Use ALSA sound driver])
        AUDIOLIBS=-lasound
	;;
    none)
        AUDIODRIVER="none"
        AC_DEFINE([CST_AUDIO_NONE], [], [Don't use a sound driver])
        AUDIOLIBS=
	;;
esac

AC_SUBST(AUDIODRIVER)
AC_SUBST(AUDIOLIBS)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
