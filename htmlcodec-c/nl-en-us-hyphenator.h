#ifndef NL_EN_US_HYPHENATOR_H
#define NL_EN_US_HYPHENATOR_H

#include <stdlib.h>
#include <stdbool.h>

#define KL_US_HYPHEN_PATTERN_COUNT 4938
#define KL_MAX_TOKENS     4096
#define KL_MAX_TOKEN_TEXT 64

// Lookup table for printable ASCII characters (index = char - 32, covers [32, 127]).
// 1 = alphanumeric (word character), 0 = delimiter/non-alphanumeric.
extern const int KL_ASCII_PATTERNS[96];

// Hyphenation patterns from ushyphmax.tex embedded for reference.
// Used to build the Knuth-Liang trie without file I/O when the tex file is unavailable.
extern const char* KL_US_HYPHEN_PATTERNS[KL_US_HYPHEN_PATTERN_COUNT];

// Token produced by the Knuth-Liang hyphenator.
// Uses a fixed inline text buffer — no heap allocation per token.
typedef struct {
    char text[KL_MAX_TOKEN_TEXT]; // null-terminated syllable or word (truncated to fit)
    int  length;                   // strlen of text after any truncation
    int  caseStyle;                // 0=all-lower (or non-alpha), 1=all-upper, 2=first-upper, 3=last-upper
    bool isHyphenated;             // true if this token came from a successfully hyphenated word
} KLToken;

typedef struct {
    KLToken tokens[KL_MAX_TOKENS]; // fixed-capacity inline array, no realloc
    size_t  count;
} KLTokenArray;

// Trie node for the Knuth-Liang hyphenation algorithm.
// Children indexed by: 'a'-'z' -> 0-25, '.' -> 26.
#define KL_TRIE_ALPHA 27

typedef struct KLTrieNode {
    struct KLTrieNode* children[KL_TRIE_ALPHA];
    int* weights;     // weight array of length weightsLen; NULL if no pattern ends here
    int weightsLen;   // = (pattern char length) + 1
} KLTrieNode;

// Tokenize input text using the Knuth-Liang hyphenation algorithm.
// Reads ushyphmax.tex from the executable directory to build the trie on first call.
// Returns a heap-allocated KLTokenArray; caller must call freeKLTokenArray().
KLTokenArray* tokenizeKnuthLiang(const char* input);
void freeKLTokenArray(KLTokenArray* arr);

#endif // NL_EN_US_HYPHENATOR_H
