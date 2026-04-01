#include "cl-javascript-en-tokenizer.h"
#include <string.h>
#include <ctype.h>

static const char* rawPatterns[CL_JS_EN_PATTERN_COUNT] = {
    // 1) first 64 ES2025 reserved JavaScript keywords
    "await","break","case","catch","class","const","continue","debugger","default","delete","do","else","enum","export","extends","false","finally","for","function","if","implements","import","in","instanceof","interface","let","new","null","package","private","protected","public","return","static","super","switch","this","throw","true","try","typeof","var","void","while","with","yield","abstract","boolean","byte","char","double","final","float","goto","int","long","native","short","synchronized","throws","transient","volatile","as","of",

    // 2) next 96 common JavaScript library/framework function/method/API tokens
    "console.log","console.error","console.warn","console.info","console.debug","console.table","console.assert","console.clear","console.group","console.groupEnd",
    "Object.keys","Object.values","Object.entries","Object.assign","Object.freeze","Object.create","Object.defineProperty","Object.defineProperties","Object.hasOwnProperty","Number.isNaN","Number.parseInt","Number.parseFloat",
    "String.prototype.split","String.prototype.trim","String.prototype.toLowerCase","String.prototype.toUpperCase",
    "Array.prototype.push","Array.prototype.pop","Array.prototype.shift","Array.prototype.unshift","Array.prototype.slice","Array.prototype.splice","Array.prototype.map","Array.prototype.filter","Array.prototype.reduce","Array.prototype.forEach","Array.prototype.some","Array.prototype.every","Array.prototype.find","Array.prototype.findIndex","Array.prototype.includes","Array.prototype.sort","Array.prototype.concat","Array.prototype.join",
    "window.addEventListener","window.removeEventListener","document.getElementById","document.querySelector","document.querySelectorAll","document.createElement","document.addEventListener","document.removeEventListener",
    "localStorage.getItem","localStorage.setItem","localStorage.removeItem","sessionStorage.getItem","sessionStorage.setItem","sessionStorage.removeItem",
    "fetch","Promise.resolve","Promise.reject","Promise.all","Promise.race","Promise.allSettled","Math.max","Math.min","Math.floor","Math.ceil","Math.round","Math.random","decodeURIComponent","encodeURIComponent",
    "setTimeout","clearTimeout","setInterval","clearInterval","history.pushState","history.replaceState","location.href","document.location","window.location","document.body","document.head","document.title","JSON.parse","JSON.stringify",
    "addEventListener","removeEventListener","getElementById","querySelector","querySelectorAll","getComputedStyle","requestAnimationFrame","cancelAnimationFrame","setImmediate","clearImmediate",

    // 3) next 64 most commonly used English digraphs in source code
    "th","he","er","an","re","ed","on","es","st","en","at","te","or","ti","hi","ng","it","is","al","le","se","ve","me","de","ro","no","us","ar","li","el","la","ul","ur","ea","ui","io","ee","ow","ai","qu","ck","ll","ss","rr","ff","tt","pp","oo","ch","sh","ph","wh","gh","wr","kn","mb","nt","ld","sp","pr","tr","cl","rh","yt",

    // 4) next 32 most commonly used non-alphanumeric digraphs in JavaScript source code
    "()","{}","[]","=>","==","!=","<=",">=","&&","||","??","?:","+=","-=","*=","/=","%=","++","--","<<",">>","?.","::","/*","*/","//",";;","@@","##","~~","^=","&="
};

const char* CL_JS_EN_PATTERNS[CL_JS_EN_PATTERN_COUNT];

static bool patternsInitialized = false;
static size_t minPatternLen = 0;
static bool patternIsDigraph[CL_JS_EN_PATTERN_COUNT] = { false };

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
    if (len == 0) return 3; // no change needed for empty
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
    if (len > 0 && isalpha((unsigned char)s[0]) && isupper((unsigned char)s[0])) firstUpper = true;
    if (firstUpper) return 2;
    return 3; // mixed or other: no casing change / no explicit casing flag
}

static void initialize_patterns(void) {
    if (patternsInitialized) return;

    int indices[CL_JS_EN_PATTERN_COUNT];
    for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
        indices[i] = i;
    }

    qsort(indices, CL_JS_EN_PATTERN_COUNT, sizeof(int), compare_pattern_length_desc);

    minPatternLen = SIZE_MAX;
    for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
        const char* p = rawPatterns[indices[i]];
        CL_JS_EN_PATTERNS[i] = p;
        patternIsDigraph[i] = (indices[i] >= 160 && indices[i] < 224);
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

CLJSTokenArray* tokenizeJavaScript(const char* input) {
    if (!input) return NULL;
    initialize_patterns();

    size_t inputLen = strlen(input);
    CLJSTokenArray* result = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
    if (!result) return NULL;
    result->count = 0;

    size_t pos = 0;
    while (pos < inputLen && result->count < CL_JS_EN_MAX_TOKENS) {
        bool matched = false;
        size_t matchedLen = 0;
        unsigned char matchedIndex = 0;

        size_t remaining = inputLen - pos;

        for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
            const char* pattern = CL_JS_EN_PATTERNS[i];
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
            int style = 3; // default: no casing change needed
            if (patternIsDigraph[matchedIndex]) {
                style = detect_case_style(input + pos, matchedLen);
            }
            CLJSToken token = {true, matchedIndex, style};
            result->tokens[result->count++] = token;
            pos += matchedLen;
        } else {
            size_t toConsume = 1;
            if (toConsume > remaining) toConsume = remaining;
            for (size_t j = 0; j < toConsume && result->count < CL_JS_EN_MAX_TOKENS; j++) {
                unsigned char ch = (unsigned char)input[pos + j];
                int style = 3; // non-pattern case, preserve as no change-needed marker
                CLJSToken token = {false, ch, style};
                result->tokens[result->count++] = token;
            }
            pos += toConsume;
        }
    }

    return result;
}

void freeCLJSTokenArray(CLJSTokenArray* arr) {
    if (arr) free(arr);
}
