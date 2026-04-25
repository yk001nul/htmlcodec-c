#ifndef CSS_TOKENIZER_H
#define CSS_TOKENIZER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define CSS_MAX_TOKENS          256
#define CSS_MAX_SELECTOR_LEN    256
#define CSS_MAX_PROPERTY_NAME   64
#define CSS_MAX_PROPERTY_VALUE  512
#define CSS_MAX_PROPERTIES      16
#define CSS_MAX_TOKENIZABLE     1024
#define CSS_MAX_UNIQUE_TOKENIZABLE 1464  /* 1208 patterns + 256 ASCII */

/* Codebook: flat array of all known CSS patterns across 11 segments.
   Segment boundary start indices are exposed below.                   */
extern const char* CSS_PATTERNS[];
extern const int   CSS_PATTERN_COUNT;

/* Segment start indices within CSS_PATTERNS */
extern const int CSS_SEG1_START;   /* HTML type selectors (tag names)        */
extern const int CSS_SEG2_START;   /* HTML attribute names                   */
extern const int CSS_SEG3_START;   /* Pseudo-class selectors                 */
extern const int CSS_SEG4_START;   /* Pseudo-element selectors               */
extern const int CSS_SEG5_START;   /* CSS properties                         */
extern const int CSS_SEG6_START;   /* CSS at-rules                           */
extern const int CSS_SEG7_START;   /* Combinators and conditional symbols     */
extern const int CSS_SEG8_START;   /* Reserved keyword values                */
extern const int CSS_SEG9_START;   /* Value functions (up to first '(')      */
extern const int CSS_SEG10_START;  /* Named colors                           */
extern const int CSS_SEG11_START;  /* Named blocks used in CSS at-rules      */

/* Requirement 2 ---------------------------------------------------------- */

/* A single tokenized unit.
   isPattern = true  -> flag holds the index into CSS_PATTERNS.
   isPattern = false -> flag holds the raw ASCII value of the character.    */
typedef struct {
    bool           isPattern;
    unsigned short flag;
} CSSTokenizable;

/* Requirement 3 ---------------------------------------------------------- */

typedef struct {
    char name[CSS_MAX_PROPERTY_NAME];
    char value[CSS_MAX_PROPERTY_VALUE];
    CSSTokenizable nameTokens[CSS_MAX_TOKENIZABLE];
    CSSTokenizable valueTokens[CSS_MAX_TOKENIZABLE];
    int nameTokenSize;
    int valueTokenSize;
} CSSProperty;

/* Requirement 4 & 5 ------------------------------------------------------ */

typedef struct {
    int type; /* 0: selector rule, 1: at-rule, 2: comment */
    union {
        struct {
            char selector[CSS_MAX_SELECTOR_LEN];
            CSSProperty properties[CSS_MAX_PROPERTIES];
            int propertyCount;
            /* Req 4 */
            CSSTokenizable selectorTokens[CSS_MAX_TOKENIZABLE];
            int selectorTokenSize;
            /* Req 2 (csscodec): flattened token stream for the whole rule */
            CSSTokenizable ruleTokens[CSS_MAX_TOKENIZABLE];
            int ruleTokenSize;
        } rule;
        struct {
            char rule[CSS_MAX_SELECTOR_LEN];
            /* Req 5 */
            CSSTokenizable atRuleTokens[CSS_MAX_TOKENIZABLE];
            int atRuleTokenSize;
        } atRule;
        struct {
            char content[CSS_MAX_PROPERTY_VALUE];
            /* Req 1 (csscodec): character-level tokenization of comment text */
            CSSTokenizable commentTokens[CSS_MAX_TOKENIZABLE];
            int commentTokenSize;
        } comment;
    } data;
} CSSToken;

typedef struct {
    CSSToken tokens[CSS_MAX_TOKENS];
    int count;
} CSSTokenArray;

void parseCSS(const char* css, CSSTokenArray* result);
void freeCSS(CSSTokenArray* arr);

/* Req 2 (csscodec): flatten selectorTokens + property name/value tokens of a
   type-0 CSSToken into token->data.rule.ruleTokens, inserting ASCII boundary
   sentinels ('{', ':', ';', '}') between sections.                          */
void css_flatten_rule_tokens(CSSToken* token);

/* Req 3 (csscodec): frequency map ---------------------------------------- */

typedef struct {
    CSSTokenizable token;
    int            frequency;
} CSSFreqEntry;

typedef struct {
    CSSFreqEntry entries[CSS_MAX_UNIQUE_TOKENIZABLE];
    int          uniqueCount;
    int          totalTokens;
} CSSFreqMap;

/* Build a frequency map from all CSSTokenizable data in arr.
   For type-0 tokens uses ruleTokens; type-1 uses atRuleTokens;
   type-2 uses commentTokens.  Returns heap-allocated map sorted
   descending by frequency; caller must call freeCSSFreqMap().   */
CSSFreqMap* collectCSSFrequencies(const CSSTokenArray* arr);
void        freeCSSFreqMap(CSSFreqMap* map);

#endif /* CSS_TOKENIZER_H */
