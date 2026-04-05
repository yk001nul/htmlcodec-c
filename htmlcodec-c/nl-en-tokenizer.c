#include "nl-en-tokenizer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

const char* NL_EN_RAW_PATTERNS[NL_EN_PATTERN_COUNT] = {
    // Section 1: 24 most common CV syllables (consonant + vowel, 2 chars)
    "be","me","he","we","te","to","go","do","no","so","lo","by",
    "my","la","ma","pa","na","ha","li","hi","ti","di","bu","ye",

    // Section 2: 24 most common CVC syllables #1 (consonant + vowel + consonant, 3 chars)
    "can","man","had","has","him","his","not","but","let","get","set","met",
    "cut","run","sun","win","bit","sit","hit","hot","top","fix","mix","six",

    // Section 3: 24 most common CCV syllables #1 (consonant cluster + vowel, 3 chars)
    "pro","pri","pra","tra","tre","tri","tro","bra","bre","bri","gra","gre",
    "gri","cra","cre","dra","dre","dri","fla","fle","pla","ple","sla","the",

    // Section 4: 24 most common CCV syllables #2 (consonant cluster + vowel, 3 chars)
    "bla","ble","blo","bru","cla","cle","cli","clo","cri","cro","fro","glo",
    "glu","plo","plu","sca","ski","sna","sni","sta","ste","sti","sto","swi",

    // Section 5: 24 most common CVC syllables #2 (consonant + vowel + consonant, 3 chars)
    "pan","tan","ran","ban","fan","van","den","hen","men","ten","fin","gin",
    "kin","pin","tin","bon","con","son","ton","fun","gun","nun","pun","tun",

    // Section 6: 24 most common CVCC syllables (consonant + vowel + 2 consonants, 4 chars)
    "band","land","hand","sand","hard","park","mark","dark","bark","best","rest","test",
    "west","past","last","fast","cast","list","fist","mist","lost","cost","dust","rust",

    // Section 7: 24 most common VC syllables (vowel + consonant, 2 chars)
    "at","an","in","it","on","up","am","as","is","of","or","us",
    "ab","ad","ar","av","ax","et","ev","ub","uc","ud","uf","ug",

    // Section 8: 24 most common VCC syllables (vowel + 2 consonants, 3 chars)
    "and","end","old","art","ask","elf","ind","ost","ust","ast","ect","ang",
    "ong","ung","ank","ink","unk","ald","elt","ort","ond","aft","oft","ilt",

    // Section 9: 8 most common CCC consonant clusters
    "str","scr","spr","spl","squ","nth","shr","thr",

    // Section 10: Top 24 prefixes
    "pre","inter","un","dis","en","em","non","over","mis","sub","trans","super",
    "semi","anti","mid","under","fore","post","auto","bi","re","multi","de","ex",

    // Section 11: Top 24 suffixes
    "ing","ed","er","ion","tion","sion","ity","ness","ment","ful","less","ly",
    "est","able","ible","ant","ent","al","ive","ism","ize","ate","ist","ous",

    // Section 12: Top 8 non-alphanumeric digraphs
    ". ",", ","? ","! ","; ",": ","- ","' "
};

const char* NL_EN_PATTERNS[NL_EN_PATTERN_COUNT];

static bool patternsInitialized = false;
static size_t minPatternLen = 0;

static int compare_pattern_length_desc(const void* a, const void* b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    size_t la = strlen(NL_EN_RAW_PATTERNS[ia]);
    size_t lb = strlen(NL_EN_RAW_PATTERNS[ib]);
    if (la < lb) return 1;
    if (la > lb) return -1;
    return 0;
}

static bool equal_case_insensitive(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char ca = a[i];
        char cb = b[i];
        if (tolower((unsigned char)ca) != tolower((unsigned char)cb)) {
            return false;
        }
    }
    return true;
}

static int detect_case_style(const char* s, size_t len) {
    if (len == 0) return 0;
    bool allLower = true;
    bool allUpper = true;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isalpha(c)) {
            if (islower(c)) allUpper = false;
            if (isupper(c)) allLower = false;
        }
    }
    if (allLower) return 0;
    if (allUpper) return 1;
    bool firstUpper = false;
    bool lastUpper = false;
    if (len > 0 && isalpha((unsigned char)s[0]) && isupper((unsigned char)s[0])) firstUpper = true;
    if (len > 0 && isalpha((unsigned char)s[len - 1]) && isupper((unsigned char)s[len - 1])) lastUpper = true;
    if (firstUpper && !lastUpper) return 2;
    if (!firstUpper && lastUpper) return 3;
    return 0;
}

static void initialize_patterns(void) {
    if (patternsInitialized) return;

    // Build sorted pattern table by descending length (longest first, shortest last)
    int indices[NL_EN_PATTERN_COUNT];
    for (int i = 0; i < NL_EN_PATTERN_COUNT; i++) {
        indices[i] = i;
    }

    qsort(indices, NL_EN_PATTERN_COUNT, sizeof(int), compare_pattern_length_desc);

    minPatternLen = SIZE_MAX;
    for (int i = 0; i < NL_EN_PATTERN_COUNT; i++) {
        const char* p = NL_EN_RAW_PATTERNS[indices[i]];
        NL_EN_PATTERNS[i] = p;
        size_t l = strlen(p);
        if (l < minPatternLen) {
            minPatternLen = l;
        }
    }
    if (minPatternLen == SIZE_MAX || minPatternLen == 0) {
        minPatternLen = 1;
    }
    patternsInitialized = true;
}

NLTokenArray* tokenizeEnglish(const char* input) {
    if (!input) return NULL;
    initialize_patterns();

    size_t inputLen = strlen(input);
    NLTokenArray* result = (NLTokenArray*)malloc(sizeof(NLTokenArray));
    if (!result) return NULL;
    result->count = 0;
    int patternHit = 0;
    int patternMiss = 0;

    // histogram[len] = number of matches (or non-matches) of that character length.
    // Index 1 counts single-char non-matches; indices 2+ count pattern matches by length.
    #define NL_EN_HISTOGRAM_MAX_LEN 8
    int histogram[NL_EN_HISTOGRAM_MAX_LEN + 1];
    memset(histogram, 0, sizeof(histogram));

    // Read from the back: pos points one past the last unconsumed character.
    // Tokens are collected in reverse order, then the array is reversed at the end.
    size_t pos = inputLen;
    while (pos > 0 && result->count < NL_EN_MAX_TOKENS) {
        bool matched = false;

        for (int i = 0; i < NL_EN_PATTERN_COUNT; i++) {
            const char* pattern = NL_EN_PATTERNS[i];
            size_t pLen = strlen(pattern);
            if (pLen == 0 || pLen > pos) continue;
            if (equal_case_insensitive(input + (pos - pLen), pattern, pLen)) {
                int style = detect_case_style(input + (pos - pLen), pLen);
                NLToken token = {true, (unsigned char)i, style};
                result->tokens[result->count++] = token;
                //printf("[NL-EN] Pattern matched: \"%s\" (index %d, caseStyle %d)\n",
                //       NL_EN_PATTERNS[i], i, style);
                pos -= pLen;
                patternHit++;
                if (pLen <= NL_EN_HISTOGRAM_MAX_LEN) histogram[pLen]++;
                matched = true;
                break;
            }
        }

        if (!matched) {
            unsigned char ch = (unsigned char)input[pos - 1];
            int style = detect_case_style((const char*)&input[pos - 1], 1);
            NLToken token = {false, ch, style};
            result->tokens[result->count++] = token;
            //printf("[NL-EN] ASCII char: '%c' (0x%02X, caseStyle %d)\n",
            //         (ch >= 32 && ch < 127) ? ch : '?', ch, style);
            pos -= 1;
            patternMiss++;
            histogram[1]++;
        }
    }

    // Tokens were collected in reverse order; reverse so they read left-to-right.
    size_t lo = 0;
    size_t hi = result->count;
    while (hi > lo) {
        hi--;
        NLToken tmp = result->tokens[lo];
        result->tokens[lo] = result->tokens[hi];
        result->tokens[hi] = tmp;
        lo++;
    }

    printf("[NL-EN] Pattern hits: %d, misses: %d\n", patternHit, patternMiss);
    printf("[NL-EN] Match histogram by pattern length:\n");
    printf("[NL-EN]   length 1 (non-match): %d\n", histogram[1]);
    for (int len = 2; len <= NL_EN_HISTOGRAM_MAX_LEN; len++) {
        if (histogram[len] > 0) {
            printf("[NL-EN]   length %d: %d\n", len, histogram[len]);
        }
    }

    return result;
}

void freeNLTokenArray(NLTokenArray* arr) {
    if (arr) {
        free(arr);
    }
}
