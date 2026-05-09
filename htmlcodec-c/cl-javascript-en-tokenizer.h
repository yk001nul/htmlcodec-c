#ifndef CL_JAVASCRIPT_EN_TOKENIZER_H
#define CL_JAVASCRIPT_EN_TOKENIZER_H

#include <stdlib.h>
#include <stdbool.h>
#include "htmlcodec-c.h"

#define CL_JS_EN_PATTERN_COUNT 502
#define CL_JS_EN_MAX_TOKENS 8192

HTMLCODEC_API extern const char* CL_JS_EN_RAW_PATTERNS[CL_JS_EN_PATTERN_COUNT];
HTMLCODEC_API extern const char* CL_JS_EN_PATTERNS[CL_JS_EN_PATTERN_COUNT];
/* True at sorted position i if that pattern is an English digraph (raw index 160–223) */
HTMLCODEC_API extern bool CL_JS_EN_PATTERN_IS_DIGRAPH[CL_JS_EN_PATTERN_COUNT];

typedef struct {
    bool isPattern;           /* true if matches one of the patterns */
    unsigned short flag;      /* pattern index (0–511) if isPattern, else ASCII char */
    int caseStyle;            /* 0: all lower, 1: all upper, 2: first uppercase, 3: no casing change needed */
} CLJSToken;

typedef struct {
    CLJSToken tokens[CL_JS_EN_MAX_TOKENS];
    size_t count;
} CLJSTokenArray;

#ifdef __cplusplus
extern "C" {
#endif

HTMLCODEC_API CLJSTokenArray* tokenizeJavaScript(const char* input);
HTMLCODEC_API void freeCLJSTokenArray(CLJSTokenArray* arr);

/**
 * Reconstruct the original string from a CLJSTokenArray.
 * Caller must free() the returned buffer.
 * *cumLen receives the number of bytes written (excluding null terminator).
 * Requires patterns to be initialized (call tokenizeJavaScript first).
 */
HTMLCODEC_API char* detokenizeCLJSTokenArray(const CLJSTokenArray* arr, int* cumLen);

#ifdef __cplusplus
}
#endif

#endif // CL_JAVASCRIPT_EN_TOKENIZER_H
