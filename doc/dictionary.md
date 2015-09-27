Pronouncing dictionary
======================

Bellbird's pronounciation dictionary is descended from the Flite dictionary.
Like Flite, Bellbird first looks up a pronounciations in a dictionary and otherwise
uses letter to sound rules to pronounce a word or symbol. Words or symbols which are correctly
represented by the letter to sound rules are not placed in the dictionary to conserve 
memory.

Bellbird's pronouncing dictionary uses a different data structure to Flite. The difference
in data structure is to allow Bellbird to pronounce very rare foreign loan words in English
which use accented symbols. Tools for extracting or encoding Flite dictionaries do not work
with Bellbird.

##Determining Bellbird's pronounciation

The option `--printphones` allows the user to determine how Bellbird will pronounce a word.

##Temporary Bellbird runtime pronounciation changes

The `--add_dict` option allows temporary runtime changes to Bellbird's
pronounciation.
The addenda file uses the same format as the decoded Bellbird dictionary.
Follow the decoding Bellbird dictionary instructions to see the format.
Since addenda parsing is less efficient it is intended that only a small
number of words are added by the user via an addenda file.

##Decoding the Bellbird dictionary

The bellbird_extras github repository (`https://github.com/peterdrysdale/bellbird_extras`) 
contains Python3 tools for decoding and encoding the Bellbird dictionary.
Python3 is a prerequisite for using these tools.

To decode a dictionary take the files `lang/cmulex/cmu_lex_data_raw.c` and
`lang/cmulex/cmu_lex_entries.c` and place them in your bellbird_extras directory.
Call the decode routine with 

    ./decodelex.py cmu_lex_data_raw.c cmu_lex_entries.c

The generated file `dict` is your human readable dictionary file.

##Encoding the Bellbird dictionary

To encode a dictionary take the file `lang/cmulex/cmu_lex_entries.c` and your new dictionary file
`dict` and place them in your bellbird_extras directory.
Call the encode routine with 

    ./encodelex.py cmu_lex_entries.c dict

The generated file `compressed-lex` is then copied to `lang/cmulex/cmu_lex_data_raw.c`. The new
dictionary will probably be a different size to the old so it is necessary to:

    tail cmu_lex_data_raw.c

and copy the `num_bytes` value in the last line to `lang/cmulex/cmu_lex_num_bytes.c`.

Bellbird may then be recompiled with the updated pronounciations.

##Hints on writing new pronounciations

The phonemes recognised by Bellbird for pronounciation are found in the
table `cmu_lex_phone_table` in the file `lang/cmulex/cmu_lex_entries.c`.

Observing the way words are already encoded by looking at examples in the existing decoded
dictionary are invaluable for generating a new pronounciation.

Using the `--printphones` option to find how the letter to sound rules work may also help.

Always recheck a pronounciation when it is compiled in as the pronounciation may occasionally surprise.

##Bellbird dictionary policy

Bellbird welcomes dictionary additions but they must be made consistent with Bellbird's license.
It is crucial Bellbird's dictionary remains untainted from a copyright perspective. For copyright
reasons we may not use dictionary additions you ask to upstream if you have consulted any copyrighted
dictionary, copyrighted wordlist or related work to either find words or their pronounciations.

In the Bellbird authors experience the ideal way to improve Bellbird's pronounciation is to use
Bellbird and when you find words which can be improved, improve them from your own speaking experience.
Speak to a diversity of friends or colleagues to obtain opinions on pronounciation. Occasionally one
mispronounced word will provide an idea for a number of others that you can check. The authors find words
through their own use of Bellbird or by periodic hammering `bellbird` with words off the top
of their heads. Books in the public domain are a valuable source of words. Looking at the existing Flite and
Bellbird dictionaries is also invaluable. Seeing some words in a group in these existing dictionaries
allows one to easily remember other new words in the group which may then be tested.

##Current dictionary statistics

Over the past 12 months the originator of Bellbird has personally verified approximately
40,000 words' pronounciation in Bellbird. This is completely separate to the most valuable existing work which created
the CMU pronouncing dictionary which in festival appears to cover in excess of 100,000 words. It should
be noted that these sets are likely to strongly overlap since common words are more likely to be found by each effort.
Currently between one and two thousand words have been added or changed in Bellbird relative to Flite.


