Bellbird: A remix of a speech synthesis engine
===============================================

This document provides a short manual for Bellbird.

Bellbird is written in C, and is intended to run on Linux or win32 systems.
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

    ./bellbird --voice voice.flitevox -f inputtext -o - | opusenc --bitrate 128 - outputfile.opus

This will read inputtext and send it to `STDOUT` where it can be grabbed by `opusenc` (if that is your
preferred and installed encoder).

    ./bellbird --htsvoice cmu_us_arctic_slt.htsvoice -f inputtext -o - | lame --b 128 - outputfile.mp3

This will read inputtext and send it to `STDOUT` where it can be grabbed by `lame` (if that is your
preferred and installed encoder).

## Copying

Bellbird is open source software.
It contains code from a number of projects including flite, hts_engine,
festival and flite+hts. Please see the COPYING file for information on
the licenses. Note all the licenses are broadly similar to the original
Flite license. Bellbird uses the same license to allow the exchange of
code. Bellbird has benefitted extraordinarily from the
ability to use other projects code for creating Bellbird.

## Acknowledgements

Bellbird was branched from Flite for Android's version of:
         Flite: a small run-time speech synthesis engine
                      version 1.5.6-current

It contains code from:
  - flite 2.0 (Dec 2014)
  - hts_engine 1.08 (Dec 2013)
  - flite+hts 1.04 (Dec 2012)

Bellbird contains material from significant numbers of contributors
to a number of different projects. The ACKNOWLEDGEMENTS file attempts
to collate these contributions. Very little of Bellbird was written
by Bellbird's authors. Bellbird's authors wish to thank the other
projects whose code made Bellbird possible. Bellbird's authors wish the
best for the these other projects and look forward to their continued vitality.
Any deficiencies in Bellbird are solely the responsibility of Bellbird's
authors.

## Building and Installation

Bellbird is quick and easy to build with minimal dependencies.
Build instructions may be found in doc/building.md

## Voice loading

Bellbird voices are all dynamically loaded at Bellbird startup.
Bellbird support two voices types at the current time.

 1. The argument "--voice" should be set to a full pathname of dumped Clustergen voice.
 2. The argument "--htsvoice" should be set to a full pathname of HTS (version 1.07) voice.

Voice files for testing purposes are collected at `https://github.com/peterdrysdale/bellbird_extras`.

Note voices are a few megabytes in size but there will
still be some time required to read them into newly created structures. On my
700Mhz netbook there is a 0.2 sec overhead to read in the voices relative to
mapped Flite voices. Bellbird is not targetted for embedded work. Bellbird recommends
the Flite library for such applications.


## Rationale for Bellbird's development

Bellbird is not intended as a fork of flite, flite+hts or festival, in the sense
that we do NOT wish to compete with these worthwhile projects. This is the
reason we have labelled it a remix. In music remixes are rarely intended be a
competitor to the orginal work.

Bellbird was originally a toy of its originator while he acted as an uploader
for maintaining the Debian package of festival. It was used originally to test
ideas and his understanding of festival to allow him to maintain the Debian
packaging for festival. Additional features have been added to Bellbird.

Bellbird offers the user the following preferred features.

 1. Dynamic (only) loading of a range of voice types including clustergen
    (versions v1.5.6 and v2.0 flitevox) and hts voices.
 2. STDIN and STDOUT, input and output redirection via commandline. This
    is designed to allow flexible preprocessing of input text and allow interfacing
    to a wide range of audio conversion software for generated sound data.
    It is easy to send Bellbird sound data to opus, mp3 and other encoders for further
    processing.
 3. UTF-8 directional apostrophe support for contraction type words. With the rise of
    the web, UTF-8 punctuation in English text is more common. Bellbird will accept
    UTF-8 apostrophes in addition to ASCII formatted text. Flite v2.0 has now added this
    functionality also.
 4. UTF-8 quotation mark support.
 5. Allow UTF-8 symbols in dictionary words. It is now common in English for loan words
    to be written with UTF-8 accent symbols. Bellbird allows such words to be retrofitted to
    the ASCII dictionary.(Note automatic case conversion doesn't occur for UTF-8 symbols).
 6. Bellbird is able to pronounce numerous U.K./Commonwealth spelling variants of English words
    for U.S. voices (these are generally pronounced with U.S. accent). The previous cmu lexicon
    was confined to U.S. English spelling. A user (even if in the U.S.) has no control on
    the spelling variant of text available either in classic novels or on the internet. It is
    preferrable that TTS will read this in the same way that a U.S. reader will adapt to
    variant spelling. (Often such readers will not even notice which variant they are reading.)
    Words are added as they are heard by the authors during the operation of Bellbird. Bellbird
    does not have a complete U.K. spelling variant pronouncing dictionary at this time but it
    already has many common and not so common U.K. spelling variants. The authors are
    now rarely hearing U.K. spelling variants whose pronounciation is not upto the same
    standard as U.S. spelling variants when pronounced by the U.S. voices.
 7. Bellbird is actively improving its pronouncing dictionary as use of Bellbird highlights
    words which could have improved pronounciation.
 8. Large File support. The WAV format allows a maximum file size of 4Gb. Where
    the O.S. supports this, Bellbird will write WAV files in excess of 2Gb upto the format
    maximum of 4Gb. Users who wish to write more than 4Gb of sound data are recommended
    to use Bellbird's STDOUT redirection and use audio software such as an opus or mp3 encoder to
    reformat the data.

Bellbird offers the developer, the following features:

 1. compiles clean using gcc and clang with -Wall and -Wextra,
 2. regular testing and compliance against the latest compilers (Developers test against
    the latest gcc or clang as they become available in Arch or Debian testing),
 3. Autotools or cmake build environment with incremental builds,
 4. minor performance improvements,
 5. rapid full tree rebuilds.

In the long tradition of open source, Bellbird "scratched the itch" of one person.
It is being developed publicly in case others might find it useful and/or other
programmers wish to borrow code from it.

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

