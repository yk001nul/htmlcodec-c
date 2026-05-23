#include "json-tokenizer.h"
#include <stdbool.h>

/* Two-character SPECIAL boundary symbols (checked before single-char) */
static const char* SPECIAL2[8] = {
    "{{", "}}", "[[", "]]", "{[", "]}", "{\"", "\"}"
};

JSONTokenArray* tokenizeJSON(const char* input) {
    if (!input) return NULL;

    size_t inputLen = strlen(input);
    JSONTokenArray* result = (JSONTokenArray*)calloc(1, sizeof(JSONTokenArray));
    if (!result) return NULL;
    result->count = 0;

    size_t pos = 0;
    while (pos < inputLen && result->count < JSON_MAX_TOKENS) {
        char ch = input[pos];
        size_t remaining = inputLen - pos;
        JSONToken tok;
        memset(&tok, 0, sizeof(tok));

        /* HEX: \uXXXX (6 chars) */
        if (ch == '\\' && remaining >= 6 && input[pos + 1] == 'u' &&
            isxdigit((unsigned char)input[pos + 2]) &&
            isxdigit((unsigned char)input[pos + 3]) &&
            isxdigit((unsigned char)input[pos + 4]) &&
            isxdigit((unsigned char)input[pos + 5])) {
            tok.type = JSON_HEX;
            memcpy(tok.data, input + pos, 6);
            tok.dataSize = 6;
            result->tokens[result->count++] = tok;
            pos += 6;
            continue;
        }

        /* ESCAPE: \X where X is one of: " \ / b f n r t */
        if (ch == '\\' && remaining >= 2) {
            char esc = input[pos + 1];
            if (esc == '"' || esc == '\\' || esc == '/' ||
                esc == 'b' || esc == 'f'  || esc == 'n' ||
                esc == 'r' || esc == 't') {
                tok.type = JSON_ESCAPE;
                tok.data[0] = '\\';
                tok.data[1] = esc;
                tok.dataSize = 2;
                result->tokens[result->count++] = tok;
                pos += 2;
                continue;
            }
        }

        /* FIXED: true, false, null */
        if (remaining >= 4 && strncmp(input + pos, "true", 4) == 0) {
            tok.type = JSON_FIXED;
            memcpy(tok.data, "true", 4);
            tok.dataSize = 4;
            result->tokens[result->count++] = tok;
            pos += 4;
            continue;
        }
        if (remaining >= 5 && strncmp(input + pos, "false", 5) == 0) {
            tok.type = JSON_FIXED;
            memcpy(tok.data, "false", 5);
            tok.dataSize = 5;
            result->tokens[result->count++] = tok;
            pos += 5;
            continue;
        }
        if (remaining >= 4 && strncmp(input + pos, "null", 4) == 0) {
            tok.type = JSON_FIXED;
            memcpy(tok.data, "null", 4);
            tok.dataSize = 4;
            result->tokens[result->count++] = tok;
            pos += 4;
            continue;
        }

        /* SPECIAL two-char (longer match before single-char) */
        bool matched = false;
        if (remaining >= 2) {
            for (int i = 0; i < 8; i++) {
                if (input[pos] == SPECIAL2[i][0] && input[pos + 1] == SPECIAL2[i][1]) {
                    tok.type = JSON_SPECIAL;
                    tok.data[0] = SPECIAL2[i][0];
                    tok.data[1] = SPECIAL2[i][1];
                    tok.dataSize = 2;
                    result->tokens[result->count++] = tok;
                    pos += 2;
                    matched = true;
                    break;
                }
            }
        }
        if (matched) continue;

        /* SPECIAL single-char */
        if (ch == '{' || ch == '}' || ch == ':' || ch == '[' ||
            ch == ']' || ch == ',' || ch == '"') {
            tok.type = JSON_SPECIAL;
            tok.data[0] = ch;
            tok.dataSize = 1;
            result->tokens[result->count++] = tok;
            pos++;
            continue;
        }

        /* INTEGER: consecutive digit characters */
        if (isdigit((unsigned char)ch)) {
            size_t start = pos;
            while (pos < inputLen &&
                   isdigit((unsigned char)input[pos]) &&
                   (int)(pos - start) < JSON_MAX_TOKEN_DATA - 1) {
                pos++;
            }
            tok.type = JSON_INTEGER;
            int len = (int)(pos - start);
            memcpy(tok.data, input + start, (size_t)len);
            tok.dataSize = len;
            result->tokens[result->count++] = tok;
            continue;
        }

        /* ASCII: single character fallback */
        tok.type = JSON_ASCII;
        tok.data[0] = ch;
        tok.dataSize = 1;
        result->tokens[result->count++] = tok;
        pos++;
    }

    return result;
}

char* detokenizeJSON(const JSONTokenArray* arr, int* outSize) {
    if (!arr || !outSize) return NULL;

    size_t bufSize = 0;
    for (int i = 0; i < arr->count; i++) {
        bufSize += (size_t)arr->tokens[i].dataSize;
    }
    bufSize++;

    char* result = (char*)calloc(bufSize, 1);
    if (!result) return NULL;

    int pos = 0;
    for (int i = 0; i < arr->count; i++) {
        memcpy(result + pos, arr->tokens[i].data, (size_t)arr->tokens[i].dataSize);
        pos += arr->tokens[i].dataSize;
    }
    result[pos] = '\0';
    *outSize = pos;
    return result;
}
