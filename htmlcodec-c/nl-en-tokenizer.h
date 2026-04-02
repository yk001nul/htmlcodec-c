#ifndef NL_EN_TOKENIZER_H
#define NL_EN_TOKENIZER_H

#include <stdlib.h>
#include <stdbool.h>

#define NL_EN_PATTERN_COUNT 256
#define NL_EN_MAX_TOKENS 4096

extern const char* NL_EN_RAW_PATTERNS[NL_EN_PATTERN_COUNT];
extern const char* NL_EN_PATTERNS[NL_EN_PATTERN_COUNT];

typedef struct {
    bool isPattern;        // true if matches one of the patterns
    unsigned char flag;    // pattern index if isPattern, else ASCII char
    int caseStyle;         // 0: all lower, 1: all upper, 2: first uppercase, 3: last uppercase
} NLToken;

typedef struct {
    NLToken tokens[NL_EN_MAX_TOKENS];
    size_t count;
} NLTokenArray;

NLTokenArray* tokenizeEnglish(const char* input);
void freeNLTokenArray(NLTokenArray* arr);

#endif // NL_EN_TOKENIZER_H
