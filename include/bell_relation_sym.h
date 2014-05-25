#ifndef __BELL_RELATION_SYM_H_
#define __BELL_RELATION_SYM_H_

// These are compact forms of utt_relation_create symbols
// Using these definitions improves performance of internal_ff
// The values themselves are irrelevant it is only required they are unique

#define TOKEN        "\x01"
#define PHRASE       "\x02"
#define SYLLABLE     "\x03"
#define SYLSTRUCTURE "\x04"
#define SEGMENT      "\x05"
#define WORD         "\x06"

#define HMMSTATE     "\x10"
#define SEGSTATE     "\x11"
#define MCEP_LINK    "\x12"
#define MCEP         "\x13"

#endif
