#ifndef HTML_TOKENIZER_H
#define HTML_TOKENIZER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "css-tokenizer.h"
#include "cl-javascript-en-tokenizer.h"
#include "nl-en-tokenizer.h"

#define HTML_MAX_TOKENS 1000
#define HTML_MAX_TAG_NAME 64
#define HTML_MAX_ATTR_COUNT 32
#define HTML_MAX_ATTR_NAME 32
#define HTML_MAX_ATTR_VALUE 256
#define HTML_MAX_TEXT_CONTENT 512

typedef enum {
    HTML_SUBDATA_NONE = 0,
    HTML_SUBDATA_CSS = 1,
    HTML_SUBDATA_JS = 2,
    HTML_SUBDATA_NL = 3
} HTMLSubdataType;

typedef struct {
    char name[HTML_MAX_ATTR_NAME];
    char value[HTML_MAX_ATTR_VALUE];
    HTMLSubdataType subdataType;
    union {
        CSSTokenArray* css;
        CLJSTokenArray* js;
        NLTokenArray* nl;
    } subdata;
} HTMLAttribute;

typedef struct {
    int type; // 0: text, 1: openTag, 2: closeTag
    HTMLSubdataType subdataType;
    union {
        struct {
            char content[HTML_MAX_TEXT_CONTENT];
            NLTokenArray* textTokenArray;
        } text;
        struct {
            char name[HTML_MAX_TAG_NAME];
            HTMLAttribute attributes[HTML_MAX_ATTR_COUNT];
            int attrCount;
            int selfClosing;
        } tag;
    } data;
    union {
        CSSTokenArray* css;
        CLJSTokenArray* js;
        NLTokenArray* nl;
    } subdata;
} HTMLToken;

typedef struct {
    HTMLToken tokens[HTML_MAX_TOKENS];
    int count;
} HTMLTokenArray;

void parseHTMLAttributes(const char* attrString, HTMLAttribute* attrs, int* attrCount);

HTMLTokenArray* parseHTML(const char* html);

void enrichHTMLTokenSubdata(HTMLTokenArray* tokens);

void freeHTMLTokenArray(HTMLTokenArray* arr);

/**
 * Reconstruct the original HTML string from an HTMLTokenArray.
 * Text tokens are detokenized via detokenizeNLTokenArray(); attribute values with
 * CSS/JS subdata are detokenized via their respective sub-tokenizers.
 * Caller must free() the returned buffer.
 * *cumLen receives the number of bytes written (excluding null terminator).
 * Requires NL patterns to be initialized (satisfied by any prior parseHTML call).
 */
char* detokenizeHTMLTokenArray(const HTMLTokenArray* arr, int* cumLen);

#endif // HTML_TOKENIZER_H
