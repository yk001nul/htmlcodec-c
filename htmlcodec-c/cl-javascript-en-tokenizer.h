#ifndef CL_JAVASCRIPT_EN_TOKENIZER_H
#define CL_JAVASCRIPT_EN_TOKENIZER_H

#include <stdlib.h>
#include <stdbool.h>

#define CL_JS_EN_PATTERN_COUNT 502
#define CL_JS_EN_MAX_TOKENS 8192

extern const char* CL_JS_EN_RAW_PATTERNS[CL_JS_EN_PATTERN_COUNT];
extern const char* CL_JS_EN_PATTERNS[CL_JS_EN_PATTERN_COUNT];
/* True at sorted position i if that pattern is an English digraph (raw index 160–223) */
extern bool CL_JS_EN_PATTERN_IS_DIGRAPH[CL_JS_EN_PATTERN_COUNT];

typedef struct {
    bool isPattern;           /* true if matches one of the patterns */
    unsigned short flag;      /* pattern index (0–511) if isPattern, else ASCII char */
    int caseStyle;            /* 0: all lower, 1: all upper, 2: first uppercase, 3: no casing change needed */
} CLJSToken;

typedef struct {
    CLJSToken tokens[CL_JS_EN_MAX_TOKENS];
    size_t count;
} CLJSTokenArray;

CLJSTokenArray* tokenizeJavaScript(const char* input);
void freeCLJSTokenArray(CLJSTokenArray* arr);

/**
 * Reconstruct the original string from a CLJSTokenArray.
 * Caller must free() the returned buffer.
 * *cumLen receives the number of bytes written (excluding null terminator).
 * Requires patterns to be initialized (call tokenizeJavaScript first).
 */
char* detokenizeCLJSTokenArray(const CLJSTokenArray* arr, int* cumLen);

#endif // CL_JAVASCRIPT_EN_TOKENIZER_H
