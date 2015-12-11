# The projects name is bellbird.
PROJECT(BELLBIRD C)

if( NOT VERSION )
    SET( VERSION "0.1.7-unstable")
endif( NOT VERSION )

cmake_minimum_required( VERSION 2.6.0 )
if( COMMAND cmake_policy )
    cmake_policy( SET CMP0003 NEW )
    cmake_policy( SET CMP0005 NEW )
endif( COMMAND cmake_policy)
SET( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeScripts )

# Check for 32bit linux systems which require _FILE_OFFSET_BITS
# set to be able to read and write files >2Gb
INCLUDE(CheckFileOffsetBits)
CHECK_FILE_OFFSET_BITS()
if( _FILE_OFFSET_BITS )
    ADD_DEFINITIONS (-D_FILE_OFFSET_BITS=64)
endif( _FILE_OFFSET_BITS)

option( ENABLE_ALSA "Build with ALSA support" ON )
option( ENABLE_O2 "Build with -O2 optimization" ON )
option( ENABLE_WALL "Build with -Wall warnings" ON )
option( ENABLE_CLANG "Build using clang compiler" OFF)
option( ENABLE_ANDROID "Build for Android" OFF)

if( ENABLE_ALSA )
    ADD_DEFINITIONS (-DCST_AUDIO_ALSA)
else( ENABLE_ALSA )
    ADD_DEFINITIONS (-DCST_AUDIO_NONE)
endif( ENABLE_ALSA )

if( ENABLE_O2 )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
endif( ENABLE_O2 )

if( ENABLE_WALL )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
endif( ENABLE_WALL )

if( ENABLE_CLANG )
    SET(CMAKE_C_COMPILER             "/usr/bin/clang")
endif( ENABLE_CLANG )

if( ENABLE_ANDROID )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16")
endif( ENABLE_ANDROID )

# Look for dependencies
if( ENABLE_ALSA )
    FIND_PACKAGE ( ALSA REQUIRED )
    SET( ALSA_LIBRARY asound )
endif( ENABLE_ALSA )

INCLUDE_DIRECTORIES( include )

SET(SRC_LIBBELLBIRD
    src/audio/au_alsa.c
    src/audio/au_none.c
    src/audio/bell_audio.c
    src/audio/bell_au_buff.c
    src/cg/cst_cg.c
    src/cg/cst_cg_load_voice.c
    src/cg/cst_cg_map.c
    src/cg/cst_mlpg.c
    src/cg/cst_mlsa.c
    src/cg/cst_spamf0.c
    src/commonsynth/HTS_misc.c
    src/commonsynth/bell_driver.c
    src/hrg/cst_ffeature.c
    src/hrg/cst_item.c
    src/hrg/cst_relation.c
    src/hrg/cst_utterance.c
    src/hts/HTS_engine.c
    src/hts/HTS_gstream.c
    src/hts/HTS_label.c
    src/hts/HTS_model.c
    src/hts/HTS_pstream.c
    src/hts/HTS_sstream.c
    src/hts/HTS_vocoder.c
    src/lexicon/cst_lexicon.c
    src/lexicon/cst_lts.c
    src/regex/cst_regex.c
    src/regex/regexp.c
    src/speech/cst_track.c
    src/speech/cst_wave.c
    src/speech/cst_wave_io.c
    src/stats/cst_cart.c
    src/synth/cst_ffeatures.c
    src/synth/cst_phoneset.c
    src/synth/cst_ssml.c
    src/synth/cst_synth.c
    src/synth/cst_utt_utils.c
    src/synth/cst_voice
    src/synth/flite.c
    src/utils/bell_file.c
    src/utils/cst_alloc.c
    src/utils/cst_endian.c
    src/utils/cst_error.c
    src/utils/cst_features.c
    src/utils/cst_string.c
    src/utils/cst_tokenstream.c
    src/utils/cst_val.c
    src/utils/cst_val_const.c
    src/utils/cst_val_user.c
)

SET(SRC_LIBBELLBIRD_USENGLISH
    lang/usenglish/us_int_accent_cart.c
    lang/usenglish/us_int_tone_cart.c
    lang/usenglish/us_phoneset.c
    lang/usenglish/us_ffeatures.c
    lang/usenglish/us_ffeatures_hts.c
    lang/usenglish/us_phrasing_cart.c
    lang/usenglish/us_gpos.c
    lang/usenglish/us_text.c
    lang/usenglish/us_expand.c
    lang/usenglish/us_nums_cart.c
    lang/usenglish/us_aswd.c
    lang/usenglish/usenglish.c
    lang/usenglish/us_pos_cart.c
)

SET(SRC_LIBBELLBIRD_CMULEX
    lang/cmulex/cmu_lts_rules.c
    lang/cmulex/cmu_lts_model.c
    lang/cmulex/cmu_lex.c
    lang/cmulex/cmu_lex_entries.c
    lang/cmulex/cmu_lex_data.c
    lang/cmulex/cmu_postlex.c
)

ADD_LIBRARY(
    libbellbird
    ${SRC_LIBBELLBIRD}
)

if ( ENABLE_ALSA )
TARGET_LINK_LIBRARIES (
    libbellbird
    ${ALSA_LIBRARY}
)
else ( ENABLE_ALSA )
TARGET_LINK_LIBRARIES (
    libbellbird
)
endif ( ENABLE_ALSA )

ADD_LIBRARY(
    libbellbird_usenglish 
    ${SRC_LIBBELLBIRD_USENGLISH}
)

ADD_LIBRARY(
    libbellbird_cmulex 
    ${SRC_LIBBELLBIRD_CMULEX}
)

ADD_EXECUTABLE ( bellbird main/bellbird.c )

SET_PROPERTY(
    TARGET bellbird
    APPEND PROPERTY COMPILE_DEFINITIONS PACKAGE_STRING=\"${VERSION}\"
)

TARGET_LINK_LIBRARIES (
    bellbird
    libbellbird_cmulex
    libbellbird_usenglish
    libbellbird
    m
)

ADD_EXECUTABLE ( compile_regexes main/compile_regexes.c )

TARGET_LINK_LIBRARIES (
    compile_regexes
    libbellbird_cmulex
    libbellbird_usenglish
    libbellbird
    m
)

