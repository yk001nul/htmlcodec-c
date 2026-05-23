#ifndef JSON_TOKENIZER_H
#define JSON_TOKENIZER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "htmlcodec-c.h"

#define JSON_MAX_TOKENS     4096
#define JSON_MAX_TOKEN_DATA 64

typedef enum {
    JSON_ASCII   = 0,
    JSON_SPECIAL = 1,
    JSON_INTEGER = 2,
    JSON_ESCAPE  = 3,
    JSON_HEX     = 4,
    JSON_FIXED   = 5
} JSONTokenType;

typedef struct {
    short type;
    char  data[JSON_MAX_TOKEN_DATA];
    int   dataSize;
} JSONToken;

typedef struct {
    JSONToken tokens[JSON_MAX_TOKENS];
    int       count;
} JSONTokenArray;

#ifdef __cplusplus
extern "C" {
#endif

HTMLCODEC_API JSONTokenArray* tokenizeJSON(const char* input);
HTMLCODEC_API char* detokenizeJSON(const JSONTokenArray* arr, int* outSize);

#ifdef __cplusplus
}
#endif

#endif /* JSON_TOKENIZER_H */
