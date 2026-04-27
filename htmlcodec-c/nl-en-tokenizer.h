#ifndef NL_EN_TOKENIZER_H
#define NL_EN_TOKENIZER_H

#include <stdlib.h>
#include <stdbool.h>

#define NL_EN_PATTERN_COUNT 512
#define NL_EN_MAX_TOKENS 4096

extern const char* NL_EN_RAW_PATTERNS[NL_EN_PATTERN_COUNT];
extern const char* NL_EN_PATTERNS[NL_EN_PATTERN_COUNT];

typedef struct {
    bool isPattern;         // true if matches one of the patterns
    unsigned short flag;    // pattern index if isPattern, else ASCII char
    int caseStyle;          // 0: all lower, 1: all upper, 2: first uppercase, 3: last uppercase
} NLToken;

typedef struct {
    NLToken tokens[NL_EN_MAX_TOKENS];
    size_t count;
} NLTokenArray;

NLTokenArray* tokenizeEnglish(const char* input);
void freeNLTokenArray(NLTokenArray* arr);

// Frequency entry: one unique NLToken from an NLTokenArray and its occurrence count.
typedef struct {
    NLToken token;     // copy of the NLToken from the source array
    int     frequency; // number of times this token appears in the source array
} NLFreqEntry;

// Frequency map built from a completed NLTokenArray.
// Entries are sorted by frequency descending so the most common tokens come first.
typedef struct {
    NLFreqEntry entries[NL_EN_MAX_TOKENS]; // fixed-capacity: at most one entry per unique token
    size_t      uniqueCount;               // number of distinct tokens populated
    size_t      totalTokens;               // arr->count of the source NLTokenArray
} NLFreqMap;

// Build a frequency map from a completed token array.
// Returns a heap-allocated NLFreqMap sorted by frequency descending;
// caller must call freeNLFreqMap().
NLFreqMap* collectNLFrequencies(const NLTokenArray* arr);
void freeNLFreqMap(NLFreqMap* map);

/* ── Extended word-level dictionary ─────────────────────────────────────── */

/* Common English words appended to the syllable dictionary.
 * Token flags NL_EN_PATTERN_COUNT .. NL_EN_OPT_PATTERN_COUNT-1 index these. */
#define NL_EN_WORD_COUNT       232
#define NL_EN_OPT_PATTERN_COUNT (NL_EN_PATTERN_COUNT + NL_EN_WORD_COUNT)

extern const char* NL_EN_WORD_PATTERNS[NL_EN_WORD_COUNT];

/**
 * Extended tokenizer using true longest-match over both the 512-entry syllable
 * dictionary and the NL_EN_WORD_COUNT word dictionary.  Word entries always win
 * ties because a longer match supersedes a shorter one at the same position.
 *
 * Syllable tokens: flag in [0, NL_EN_PATTERN_COUNT)
 * Word tokens:     flag in [NL_EN_PATTERN_COUNT, NL_EN_OPT_PATTERN_COUNT)
 */
NLTokenArray* tokenizeEnglishOpt(const char* input);

#endif // NL_EN_TOKENIZER_H
