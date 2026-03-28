#include <stdio.h>
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 1000
#define MAX_TAG_NAME 64
#define MAX_ATTR_COUNT 32
#define MAX_ATTR_NAME 32
#define MAX_ATTR_VALUE 256
#define MAX_TEXT_CONTENT 512

typedef struct {
    char name[MAX_ATTR_NAME];
    char value[MAX_ATTR_VALUE];
} Attribute;

typedef struct {
    int type; // 0: text, 1: openTag, 2: closeTag
    union {
        struct {
            char content[MAX_TEXT_CONTENT];
        } text;
        struct {
            char name[MAX_TAG_NAME];
            Attribute attributes[MAX_ATTR_COUNT];
            int attrCount;
            int selfClosing;
        } tag;
    } data;
} Token;

typedef struct {
    Token tokens[MAX_TOKENS];
    int count;
} TokenArray;

void parseAttributes(const char* attrString, Attribute* attrs, int* attrCount);

TokenArray* parseHTML(const char* html);

void freeTokenArray(TokenArray* arr);

#endif // TOKENIZER_H