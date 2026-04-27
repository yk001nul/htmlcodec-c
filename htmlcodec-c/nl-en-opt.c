/* nl-en-opt.c — word-level dictionary and tokenizeEnglishOpt().
   Lives in a separate translation unit so a fresh obj is always produced,
   independent of any stale nl-en-tokenizer.c.obj in the build cache. */
#include "nl-en-tokenizer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

/* ── Word dictionary ─────────────────────────────────────────────────────── */

const char* NL_EN_WORD_PATTERNS[NL_EN_WORD_COUNT] = {
    /* 4-char words (80) */
    "that","this","with","they","from","will","what","your",
    "when","have","more","make","like","into","just","good",
    "them","then","some","here","much","well","each","both",
    "back","down","over","also","such","even","come","know",
    "look","show","move","want","tell","feel","talk","walk",
    "open","stop","stay","play","read","call","fall","give",
    "left","face","door","note","wait","plan","road","star",
    "form","area","name","life","long","word","work","city",
    "home","body","blue","mind","kind","real","love","true",
    "head","keep","view","live","room","deep","fire","part",
    /* 5-char words (72) */
    "there","their","about","which","would","these","other",
    "first","after","think","place","could","where","every",
    "large","those","right","since","three","state","small",
    "again","still","found","often","world","never","early",
    "along","night","south","north","black","white","while",
    "order","stand","clear","table","class","field","water",
    "music","story","house","point","heart","light","plant",
    "bring","short","today","under","great","young","power",
    "level","money","until","human","local","major","might",
    "issue","given","going","doing","based","cases","times",
    "years","makes",
    /* 6-char words (38) */
    "people","little","always","before","should","public",
    "family","around","really","school","second","center",
    "common","during","across","simply","matter","moment",
    "number","become","within","system","reason","called",
    "making","having","taking","almost","though","enough",
    "former","future","indeed","inside","likely","period",
    "change","others",
    /* 7-char words (34) */
    "between","because","through","without","another","nothing",
    "problem","example","country","program","further","already",
    "however","million","whether","several","against","usually",
    "history","include","company","service","control","develop",
    "support","process","provide","product","current","general",
    "morning","project","thought","various",
    /* 8-char words (8) */
    "children","together","although","everyone","business",
    "language","national","research",
};

/* ── Local helpers (static copies of nl-en-tokenizer.c internals) ─────────── */

static bool opt_equal_case_insensitive(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return false;
    }
    return true;
}

static int opt_detect_case_style(const char* s, size_t len) {
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
    if (len > 0 && isalpha((unsigned char)s[0])      && isupper((unsigned char)s[0]))      firstUpper = true;
    if (len > 0 && isalpha((unsigned char)s[len - 1]) && isupper((unsigned char)s[len - 1])) lastUpper = true;
    if (firstUpper && !lastUpper) return 2;
    if (!firstUpper && lastUpper) return 3;
    return 0;
}

/* Ensure NL_EN_PATTERNS has been populated by the tokenizer's lazy init.
   We trigger it by making a no-op tokenizeEnglish call on an empty string. */
static void ensure_patterns_initialized(void) {
    if (NL_EN_PATTERNS[0] != NULL) return;
    NLTokenArray* tmp = tokenizeEnglish("");
    freeNLTokenArray(tmp);
}

/* ── tokenizeEnglishOpt ──────────────────────────────────────────────────── */

NLTokenArray* tokenizeEnglishOpt(const char* input) {
    if (!input) return NULL;
    ensure_patterns_initialized();

    size_t inputLen = strlen(input);
    NLTokenArray* result = (NLTokenArray*)malloc(sizeof(NLTokenArray));
    if (!result) return NULL;
    result->count = 0;

    size_t pos = inputLen;
    while (pos > 0 && result->count < NL_EN_MAX_TOKENS) {
        size_t best_len = 0;
        int    best_flag = -1;
        int    best_case = 0;

        for (int i = 0; i < NL_EN_WORD_COUNT; i++) {
            size_t pLen = strlen(NL_EN_WORD_PATTERNS[i]);
            if (pLen > pos) continue;
            if (opt_equal_case_insensitive(input + (pos - pLen),
                                           NL_EN_WORD_PATTERNS[i], pLen)) {
                if (pLen > best_len) {
                    best_len  = pLen;
                    best_flag = NL_EN_PATTERN_COUNT + i;
                    best_case = opt_detect_case_style(input + (pos - pLen), pLen);
                }
            }
        }

        for (int i = 0; i < NL_EN_PATTERN_COUNT; i++) {
            const char* pattern = NL_EN_PATTERNS[i];
            size_t pLen = strlen(pattern);
            if (pLen == 0 || pLen > pos) continue;
            if (opt_equal_case_insensitive(input + (pos - pLen), pattern, pLen)) {
                if (pLen > best_len) {
                    best_len  = pLen;
                    best_flag = i;
                    best_case = opt_detect_case_style(input + (pos - pLen), pLen);
                }
            }
        }

        if (best_flag >= 0) {
            NLToken token = {true, (unsigned short)best_flag, best_case};
            result->tokens[result->count++] = token;
            pos -= best_len;
        } else {
            unsigned char ch = (unsigned char)input[pos - 1];
            NLToken token = {false, ch, 0};
            result->tokens[result->count++] = token;
            pos -= 1;
        }
    }

    for (size_t lo = 0, hi = result->count; lo + 1 < hi; lo++, hi--) {
        NLToken tmp2 = result->tokens[lo];
        result->tokens[lo]      = result->tokens[hi - 1];
        result->tokens[hi - 1] = tmp2;
    }

    return result;
}
