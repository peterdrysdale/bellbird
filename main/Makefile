###########################################################################
##              This Makefile has been modified for bellbird.            ##
##              Please see COPYING file for copyright notice             ##
##              of the modified file. The original copyright             ##
##              notice for the unmodified file is included               ##
##              below.                                                   ##
###########################################################################
###########################################################################
##                                                                       ##
##                  Language Technologies Institute                      ##
##                     Carnegie Mellon University                        ##
##                       Copyright (c) 2001-2008                         ##
##                        All Rights Reserved.                           ##
##                                                                       ##
##  Permission is hereby granted, free of charge, to use and distribute  ##
##  this software and its documentation without restriction, including   ##
##  without limitation the rights to use, copy, modify, merge, publish,  ##
##  distribute, sublicense, and/or sell copies of this work, and to      ##
##  permit persons to whom this work is furnished to do so, subject to   ##
##  the following conditions:                                            ##
##   1. The code must retain the above copyright notice, this list of    ##
##      conditions and the following disclaimer.                         ##
##   2. Any modifications must be clearly marked as such.                ##
##   3. Original authors' names are not deleted.                         ##
##   4. The authors' names are not used to endorse or promote products   ##
##      derived from this software without specific prior written        ##
##      permission.                                                      ##
##                                                                       ##
##  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         ##
##  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      ##
##  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   ##
##  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      ##
##  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    ##
##  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   ##
##  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          ##
##  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       ##
##  THIS SOFTWARE.                                                       ##
##                                                                       ##
###########################################################################
##                                                                       ##
##  Top level user programs                                              ##
##                                                                       ##
###########################################################################
TOP=..
DIRNAME=main
BUILD_DIRS = 
ALL_DIRS= 
SRCS = bellbird_main.c \
       t2p_main.c compile_regexes.c
OBJS = $(SRCS:.c=.o)
FILES = Makefile $(SRCS)
LOCAL_INCLUDES = 

ALL = shared_libs \
      $(BINDIR)/bellbird \
      $(BINDIR)/text2phones $(BINDIR)/compile_regexes \
      $(EXTRABINS)

VOICES=$(VOXES)
VOICELIBS=$(VOICES:%=flite_%)

bellbird_LIBS = $(VOICELIBS) $(LANGS:%=bellbird_%) $(LEXES:%=bellbird_%) bellbird

bellbird_LIBS_flags = -L$(LIBDIR) $(bellbird_LIBS:%=-l%)
bellbird_LIBS_deps = $(bellbird_LIBS:%=$(LIBDIR)/lib%.a)

include $(TOP)/config/common_make_rules

# We need to set these vars, even if we're not going to build them
# so make clean can remove them
SHAREDARLIBS= $(bellbird_LIBS:%=$(LIBDIR)/lib%.shared.a)
SHAREDLIBS = $(SHAREDARLIBS:%.shared.a=%.so)
VERSIONSHAREDLIBS = $(SHAREDLIBS:%=%.${PROJECT_VERSION}) \
                    $(SHAREDLIBS:%=%.${PROJECT_SHLIB_VERSION})

LOCAL_CLEAN = $(SHAREDARLIBS) $(SHAREDLIBS) $(VERSIONSHAREDLIBS) \
              $(bellbird_LIBS_deps) $(VOICES:%=$(BINDIR)/flite_%)

ifdef SHAREDLIBFLAGS
bellbird_LIBS_flags += -Wl,-rpath $(LIBDIR) 
shared_libs: $(SHAREDLIBS)
else
shared_libs: nothing
endif

.NOTPARALLEL: $(ALL)

$(BINDIR)/bellbird: bellbird_main.o $(bellbird_LIBS_deps)
	$(CC) $(CFLAGS) -o $@ bellbird_main.o $(bellbird_LIBS_flags) $(LDFLAGS)

$(BINDIR)/text2phones: t2p_main.o $(bellbird_LIBS_deps)
	$(CC) $(CFLAGS) -o $@ t2p_main.o $(bellbird_LIBS_flags) $(LDFLAGS)

$(BINDIR)/compile_regexes: compile_regexes.o $(BELLLIBS)
	$(CC) $(CFLAGS) -o $@ compile_regexes.o $(BELLLIBFLAGS) $(LDFLAGS)

install:
#       The basic binaries
	@ $(INSTALL) -m 755 $(BINDIR)/bellbird $(DESTDIR)$(INSTALLBINDIR)
#       The libraries: static and shared (if built)
	cp -pd $(bellbird_LIBS_deps) $(DESTDIR)$(INSTALLLIBDIR)
ifdef SHAREDLIBFLAGS
	cp -pd $(SHAREDLIBS) $(VERSIONSHAREDLIBS) $(DESTDIR)$(INSTALLLIBDIR)
endif

