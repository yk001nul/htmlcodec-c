#ifndef CL_JAVASCRIPT_EN_TOKENIZER_H
#define CL_JAVASCRIPT_EN_TOKENIZER_H

#include <stdlib.h>
#include <stdbool.h>

#define CL_JS_EN_PATTERN_COUNT 256
#define CL_JS_EN_MAX_TOKENS 8192

extern const char* CL_JS_EN_PATTERNS[CL_JS_EN_PATTERN_COUNT];

typedef struct {
    bool isPattern;        // true if matches one of the patterns
    unsigned char flag;    // pattern index if isPattern, else ASCII char
    int caseStyle;         // 0: all lower, 1: all upper, 2: first uppercase, 3: no casing change needed
} CLJSToken;

typedef struct {
    CLJSToken tokens[CL_JS_EN_MAX_TOKENS];
    size_t count;
} CLJSTokenArray;

CLJSTokenArray* tokenizeJavaScript(const char* input);
void freeCLJSTokenArray(CLJSTokenArray* arr);

#endif // CL_JAVASCRIPT_EN_TOKENIZER_H
