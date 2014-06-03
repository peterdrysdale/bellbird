Bellbird: A remix of a speech synthesis engine
===============================================

Bellbird is currently BETA software. It is still in development

This document provides a short manual for bellbird.

Bellbird is written in C, and is intended to run on Linux systems.
It does not support other systems since no test hardware is available
to the current developers. If developers wish to join, other systems
could be supported.

## Quick Usage Examples

Here are some permutations of usage examples.

    ./bellbird --voice voice.flitevox -f inputtext -o play

This will read inputtext and play out to ALSA using the Clustergen voice (voice.flitevox)

    ./bellbird --htsvoice cmu_us_arctic_slt.htsvoice -f - -o outputfile.wav

This will read `STDIN` and output to a `wav` file using a hts voice (`cmu_us_arctic_slt.htsvoice` in
the current working directory)

    ./bellbird --nitechvoice /home/user/voxdata/nitech_us_awb_arctic_hts -f quickbrown.txt -o -

This will read `quickbrown.txt` and send the resulting wav file to `STDOUT` using the classic awb nitech
voice if the classic awb is located at `/home/user/voxdata/nitech_us_awb_arctic_hts`.

    ./bellbird --htsvoice cmu_us_arctic_slt.htsvoice -f inputtext -o - | opusenc --bitrate 128 - outputfile.opus

This will read inputtext and send it to `STDOUT` where it can be grabbed by `opusenc` (if that is your
preferred and installed encoder).

    ./bellbird --htsvoice cmu_us_arctic_slt.htsvoice -f inputtext -o - | lame --b 128 - outputfile.mp3

This will read inputtext and send it to `STDOUT` where it can be grabbed by `lame` (if that is your
preferred and installed encoder).

## Copying

Bellbird is open source software.
Is contains code from a number of projects including flite, hts_engine,
festival and flite+hts. Please see the COPYING file for information on
the licenses. Note all the licenses are broadly similar to the original
flite license. Bellbird uses the same license to allow the exchange of
code. Bellbird has benefitted extraordinarily from the
ability to use other projects code for creating the remix known as bellbird.

## Acknowledgements

This remix version was branched from Flite for Android's version of:
         Flite: a small run-time speech synthesis engine
                      version 1.5.6-current

It contains code from:
  - hts_engine 1.08 (Dec 2013)
  - flite+hts 1.04 (Dec 2012)
  - festival 1.96 hts_engine module (now superceded) for using classic nitech voices

Bellbird contains material from significant numbers of contributors
to a number of different projects. The ACKNOWLEDGEMENTS file attempts
to collate these contributions. Very little of Bellbird was written
by Bellbird's authors. Bellbird's authors wish to thank the other
projects whose code made Bellbird possible. Bellbird's authors wish the
best for the these other projects and look forward to their continued vitality.
Any deficiencies in Bellbird are solely the responsibility of Bellbird's
authors.

## Building and Installation

Bellbird contains the following build prerequisites:
Gnu Make
ALSA development libraries (known as `libasound2-dev` on Debian and derivative systems)
C compiler (gcc 4.7,4.8,4.9 and clang 3.1,3.2,3.3,3.4 have been tested)

If Bellbird is obtained from a Version Control System (such as a git repository),
automake and libtool are required to generate the configure script. On Debian
and derivative systems these tools are in `automake`, `libtool` and `autotools-dev`
packages. The `configure` script can be generated using:

    ./autogen.sh

Bellbird can be built using:

    ./configure
    make

Bellbird can be installed using:

    make install

The configure script follows GNU conventions, therefore `make install` 
will install all the files in `/usr/local/bin`, `/usr/local/lib` etc.
You can specify an installation prefix other than `/usr/local` using
`--prefix`, for instance `--prefix=$HOME`.

## Voice loading

Bellbird voices are all dynamically loaded at bellbird startup.
Bellbird support three voices types at the current time.

 1. The argument "--voice" should be set to a full pathname of dumped Clustergen voice.
 2. The argument "--htsvoice" should be set to a full pathname of HTS (version 1.07) voice.
 3. The argument "--nitechvoice" should be set to the pathname of the base directory of 
    one of the classic six nitech voices. The classic nitech voice set are an older 
    type of HTS voice.


Note voices are a few megabytes in size but there will
still be some time required to read them into newly created structures. On my
700Mhz netbook there is a 0.2 sec overhead to read in the voices relative to
mapped flite voices. Bellbird is not targetted for embedded work. Bellbird recommends
the Flite library for such applications.


## Rationale for Bellbird's development

Bellbird is not intended as a fork of flite, flite+hts or festival, in the sense
that we do NOT wish to compete with these worthwhile projects. This is the
reason we have labelled it a remix. In music remixes are rarely intended be a
competitor to the orginal work.

Bellbird was originally a toy of its originator while he acted as an uploader
for maintaining the Debian package of festival. It was used originally to test
ideas and his understanding of festival to allow him to maintain the Debian
packaging for festival. Originating as a toy program it had no clear design goals.

The preferred features for bellbird are:

 1. dynamic (only) loading of clustergen, hts and nitech voices,
 2. input and output allowed via STDIN and STDOUT,
 3. compile clean using gcc and clang with -Wall and -Wextra,
 4. only one load of nitech voices per execution,
 5. UTF-8 directional apostrophe support for contraction type words,
 6. Allow UTF-8 symbols in dictionary words since loan words in English may use accents
    (Note automatic case conversion doesn't occur for UTF-8 symbols),
 7. rapid full tree rebuilds.

The last feature was largely to enable quick tests of understanding of speech synthesis.
This has had the side effect that bellbird has many less features than festival and flite 
(less code = less features here). It does less but is a handy command line
tool. All the heavy lifting was done by code from the flite, festival and hts_engine projects.

In the long tradition of open source, bellbird "scratched the itch" of one person.
It is being released in case others might find it useful and/or other programmers wish to borrow
code from it.

Bellbird welcomes other developers to assist in developing Bellbird as a community
based open source package. (Although we don't yet support BSD, we would welcome our friends from
the BSD communities if they wish to join). Any code which would better benefit festival,
flite or hts_engine should be sent to those projects first and in preference.

How much Bellbird develops depends on if other people find it useful and provide feedback
to Bellbird.

## APIs

Bellbird at this time is not yet expected to be embedded into
other applications. Bellbird recommends programmers consider
the Flite library for such purposes.

