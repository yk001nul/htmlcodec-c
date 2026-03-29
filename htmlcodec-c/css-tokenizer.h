#ifndef CSS_TOKENIZER_H
#define CSS_TOKENIZER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CSS_MAX_TOKENS 1000
#define CSS_MAX_SELECTOR_LEN 256
#define CSS_MAX_PROPERTY_NAME 64
#define CSS_MAX_PROPERTY_VALUE 512
#define CSS_MAX_PROPERTIES 128

typedef struct {
    char name[CSS_MAX_PROPERTY_NAME];
    char value[CSS_MAX_PROPERTY_VALUE];
} CSSProperty;

typedef struct {
    int type; // 0: selector, 1: at-rule, 2: comment
    union {
        struct {
            char selector[CSS_MAX_SELECTOR_LEN];
            CSSProperty properties[CSS_MAX_PROPERTIES];
            int propertyCount;
        } rule;
        struct {
            char rule[CSS_MAX_SELECTOR_LEN];
        } atRule;
        struct {
            char content[CSS_MAX_PROPERTY_VALUE];
        } comment;
    } data;
} CSSToken;

typedef struct {
    CSSToken tokens[CSS_MAX_TOKENS];
    int count;
} CSSTokenArray;

void parseCSS(const char* css, CSSTokenArray* result);

void freeCSS(CSSTokenArray* arr);

#endif // CSS_TOKENIZER_H
