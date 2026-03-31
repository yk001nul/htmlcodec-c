#include "nl-en-tokenizer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static const char* rawPatterns[NL_EN_PATTERN_COUNT] = {
    // first 64 common tokens (most frequently used English words, internet media)
    "the","be","to","of","and","even","in","that","have","also","it","for","not","on","with","he",
    "as","you","do","at","this","but","his","by","from","they","we","say","her","she","or","an",
    "will","my","one","all","would","there","their","what","so","up","out","if","about","who","get","which",
    "go","me","when","make","can","like","time","no","just","him","know","take","into","year","your","more",

    // next 64 words > 5 characters
    "should","because","system","before","number","during","company","program","information","international","computer","business",
    "service","community","project","through","between","government","important","different","development","example","security","internet",
    "learning","sentence","language","research","history","product","performance","available","including","possible","support","process",
    "culture","quality","education","significant","practice","function","analysis","technology","experience","software","network","material",
    "mission","complete","specific","records","message","digital","virtual","general","modern","related","control","context","details","content","patterns","response",

    // next 32 prefixes
    "pre","inter","un","dis","en","em","non","over","mis","sub","trans","super","semi","anti","mid","under",
    "fore","post","auto","bi","tri","ultra","hyper","micro","macro","retro","tele","multi","omni","pan","pseudo","mega",

    // next 32 suffixes
    "ing","ed","er","ide","ion","tion","sion","ity","ness","ment","ful","less","ly","est","able","ible",
    "ant","ent","al","ive","ism","ize","ate","ist","ous","ary","ward","wise","ship","cy","ance","hood",

    // next 32 trigraphs/clusters
    "sch","thr","igh","ear","urs","nth","shr","chr","ght","tio","nce","pro","ter","ort","sti","sil",
    "str","sio","liv","wor","lei","fum","cha","qua","par","com","con","tra","uni","fla","pla","ver",

    // next 32 digraphs/clusters
    ", ", ". ", "th","hi","ae","xy","zk","qp","lv","mp","sn","dg","br","cl","fr","gr",
    "hl","jh","kr","lp","mn","pt","sv","wv","xr","yz","bq","du","ez","fa","gs","qu"
};

const char* NL_EN_PATTERNS[NL_EN_PATTERN_COUNT];

static bool patternsInitialized = false;
static size_t minPatternLen = 0;

static int compare_pattern_length_desc(const void* a, const void* b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    size_t la = strlen(rawPatterns[ia]);
    size_t lb = strlen(rawPatterns[ib]);
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
        const char* p = rawPatterns[indices[i]];
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

    size_t pos = 0;
    while (pos < inputLen && result->count < NL_EN_MAX_TOKENS) {
        bool matched = false;
        size_t matchedLen = 0;
        unsigned char matchedIndex = 0;

        size_t remaining = inputLen - pos;

        for (int i = 0; i < NL_EN_PATTERN_COUNT; i++) {
            const char* pattern = NL_EN_PATTERNS[i];
            size_t pLen = strlen(pattern);
            if (pLen == 0 || pLen > remaining) continue;
            if (equal_case_insensitive(input + pos, pattern, pLen)) {
                matched = true;
                matchedLen = pLen;
                matchedIndex = (unsigned char)i;
                break;
            }
        }

        if (matched) {
            int style = detect_case_style(input + pos, matchedLen);
            NLToken token = {true, matchedIndex, style};
            result->tokens[result->count++] = token;
            pos += matchedLen;
        } else {
            size_t toConsume = 1;
            if (toConsume > remaining) toConsume = remaining;
            for (size_t j = 0; j < toConsume && result->count < NL_EN_MAX_TOKENS; j++) {
                unsigned char ch = (unsigned char)input[pos + j];
                int style = detect_case_style((const char*)&input[pos + j], 1);
                NLToken token = {false, ch, style};
                result->tokens[result->count++] = token;
            }
            pos += toConsume;
        }
    }

    return result;
}

void freeNLTokenArray(NLTokenArray* arr) {
    if (arr) {
        free(arr);
    }
}
