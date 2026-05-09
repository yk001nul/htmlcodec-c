#include "cl-javascript-en-tokenizer.h"
#include "hc-once.h"
#include <string.h>
#include <ctype.h>

const char* CL_JS_EN_RAW_PATTERNS[CL_JS_EN_PATTERN_COUNT] = {
    /* 1) 64 ES2025 reserved JavaScript keywords (indices 0–63) */
    "await","break","case","catch","class","const","continue","debugger","default",
    "delete","do","else","enum","export","extends","false","finally","for","function",
    "if","implements","import","in","instanceof","interface","let","new","null","package",
    "private","protected","public","return","static","super","switch","this","throw","true",
    "try","typeof","var","void","while","with","yield","abstract","boolean","byte","char",
    "double","final","float","goto","int","long","native","short","synchronized","throws",
    "transient","volatile","as","of",

    /* 2) 96 common JavaScript library/framework function/method/API tokens (64–159) */
    "console.log","console.error","console.warn","console.info","console.debug",
    "console.table","console.assert","console.clear","console.group","console.groupEnd",
    "Object.keys","Object.values","Object.entries","Object.assign","Object.freeze",
    "Object.create","Object.defineProperty","Object.defineProperties","Object.hasOwnProperty",
    "Number.isNaN","Number.parseInt","Number.parseFloat",
    "String.prototype.split","String.prototype.trim","String.prototype.toLowerCase","String.prototype.toUpperCase",
    "Array.prototype.push","Array.prototype.pop","Array.prototype.shift","Array.prototype.unshift",
    "Array.prototype.slice","Array.prototype.splice","Array.prototype.map","Array.prototype.filter",
    "Array.prototype.reduce","Array.prototype.forEach","Array.prototype.some","Array.prototype.every",
    "Array.prototype.find","Array.prototype.findIndex","Array.prototype.includes","Array.prototype.sort",
    "Array.prototype.concat","Array.prototype.join",
    "window.addEventListener","window.removeEventListener","document.getElementById",
    "document.querySelector","document.querySelectorAll","document.createElement",
    "document.addEventListener","document.removeEventListener",
    "localStorage.getItem","localStorage.setItem","localStorage.removeItem",
    "sessionStorage.getItem","sessionStorage.setItem","sessionStorage.removeItem",
    "fetch","Promise.resolve","Promise.reject","Promise.all","Promise.race","Promise.allSettled",
    "Math.max","Math.min","Math.floor","Math.ceil","Math.round","Math.random",
    "decodeURIComponent","encodeURIComponent",
    "setTimeout","clearTimeout","setInterval","clearInterval",
    "history.pushState","history.replaceState","location.href","document.location",
    "window.location","document.body","document.head","document.title",
    "JSON.parse","JSON.stringify",
    "addEventListener","removeEventListener","getElementById","querySelector","querySelectorAll",
    "getComputedStyle","requestAnimationFrame","cancelAnimationFrame","setImmediate","clearImmediate",

    /* 3) 64 most commonly used English digraphs in source code (160–223) */
    "th","he","er","an","re","ed","on","es","st","en","at","te","or","ti","hi","ng",
    "it","is","al","le","se","ve","me","de","ro","no","us","ar","li","el","la","ul",
    "ur","ea","ui","io","ee","ow","ai","qu","ck","ll","ss","rr","ff","tt","pp","oo",
    "ch","sh","ph","wh","gh","wr","kn","mb","nt","ld","sp","pr","tr","cl","rh","yt",

    /* 4) 32 most commonly used non-alphanumeric digraphs in JavaScript (224–255) */
    "()","{}","[]","=>","==","!=","<=",">=","&&","||","??","?:","+=","-=","*=","/=",
    "%=","++","--","<<",">>","?.","::","/*","*/","//",";;","@@","##","~~","^=","&=",

    /* 5) 64 common JS identifiers and built-in words (256–319) */
    "result","error","value","index","item","data","type","name","node","target",
    "source","config","options","params","callback","handler","response","request",
    "context","instance","state","props","event","action","model","view","store",
    "router","service","component","element","input","output","message","promise",
    "resolve","reject","constructor","prototype","undefined","arguments","Infinity",
    "toString","valueOf","isArray","parseInt","parseFloat","payload","selector",
    "attribute","property","method","parameter","variable","constant","length",
    "integer","object","module","string","number","array","size","stream",

    /* 6) 64 common JS short method calls and API patterns (320–383) */
    ".then(",".catch(",".finally(",".map(",".filter(",".reduce(",".find(",".findIndex(",
    ".includes(",".some(",".every(",".flat(",".flatMap(",".keys(",".values(",".entries(",
    ".split(",".trim(",".replace(",".indexOf(",".slice(",".startsWith(",".endsWith(",
    ".bind(",".call(",".apply(",".toString(",".valueOf(",
    "require(","module.exports","exports.","process.env","async function ",
    "new Promise(","new Error(","new Map(","new Set(","new Array(","new Date(",
    ".json()","await ","} catch","} finally","(error)","(err, ","(err)",
    ".getAttribute(",".setAttribute(",".className",".innerHTML",".textContent",
    "React.","useState(","useEffect(","useRef(","useCallback(","useMemo(",
    ".style.","appendChild(","removeChild(","setTimeout(","clearTimeout(",

    /* 7) 64 common short JS operator patterns and punctuation (384–447) */
    " === "," !== "," == "," != "," <= "," >= "," && "," || "," ?? ",
    "() => {","() => ","(e) => ","(err) => ",") => {",
    "if (","} else {","} else if (","for (let ","for (const ","for (var ",
    "while (","switch (","case ","default:","break;","continue;","return;",
    "const {","let {","const [","let [","...rest","...args","...props",
    " ? "," : ","typeof ","void 0",
    "(function(","})()","});",
    "import {","import * as ","export default ","export const ","export function ",
    "class ","extends ","super(","this.","self.","window.","global.",
    " = "," + "," - "," * "," / ",

    /* 8) 64 common short verb/noun fragments for JS identifiers (448–511) */
    "get","set","has","add","run","log","use","emit","put","end",
    "init","bind","find","sort","push","pull","send","read","load","save","copy","move",
    "create","filter","update","insert","select","remove","handle","manage","process","render",
    "listen","watch","check","parse","build","map","queue","flush","reset","clear",
    "open","close","start","stop","pause","connect","encode","decode","sign","verify",
    "async","next","clone","merge","test","wrap","track","mount","invoke","toggle"
};

const char* CL_JS_EN_PATTERNS[CL_JS_EN_PATTERN_COUNT];

static hc_once_t s_patterns_once = HC_ONCE_INIT;
static size_t minPatternLen = 0;
bool CL_JS_EN_PATTERN_IS_DIGRAPH[CL_JS_EN_PATTERN_COUNT] = { false };

static int compare_pattern_length_desc(const void* a, const void* b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    size_t la = strlen(CL_JS_EN_RAW_PATTERNS[ia]);
    size_t lb = strlen(CL_JS_EN_RAW_PATTERNS[ib]);
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
    if (len == 0) return 3;
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
    return 3;
}

static void initialize_patterns(void) {

    int indices[CL_JS_EN_PATTERN_COUNT];
    for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
        indices[i] = i;
    }

    qsort(indices, CL_JS_EN_PATTERN_COUNT, sizeof(int), compare_pattern_length_desc);

    minPatternLen = (size_t)-1;
    for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
        const char* p = CL_JS_EN_RAW_PATTERNS[indices[i]];
        CL_JS_EN_PATTERNS[i] = p;
        /* English digraphs are raw indices 160–223 */
        CL_JS_EN_PATTERN_IS_DIGRAPH[i] = (indices[i] >= 160 && indices[i] < 224);
        size_t l = strlen(p);
        if (l < minPatternLen) minPatternLen = l;
    }
    if (minPatternLen == (size_t)-1 || minPatternLen == 0) minPatternLen = 1;
}

CLJSTokenArray* tokenizeJavaScript(const char* input) {
    if (!input) return NULL;
    hc_call_once(&s_patterns_once, initialize_patterns);

    size_t inputLen = strlen(input);
    CLJSTokenArray* result = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
    if (!result) return NULL;
    result->count = 0;

    size_t pos = 0;
    while (pos < inputLen && result->count < CL_JS_EN_MAX_TOKENS) {
        bool matched = false;
        size_t matchedLen = 0;
        unsigned short matchedIndex = 0;

        size_t remaining = inputLen - pos;

        for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
            const char* pattern = CL_JS_EN_PATTERNS[i];
            size_t pLen = strlen(pattern);
            if (pLen == 0 || pLen > remaining) continue;
            if (equal_case_insensitive(input + pos, pattern, pLen)) {
                matched = true;
                matchedLen = pLen;
                matchedIndex = (unsigned short)i;
                break;
            }
        }

        if (matched) {
            int style = 3;
            if (CL_JS_EN_PATTERN_IS_DIGRAPH[matchedIndex]) {
                style = detect_case_style(input + pos, matchedLen);
            }
            CLJSToken token = {true, matchedIndex, style};
            result->tokens[result->count++] = token;
            pos += matchedLen;
        } else {
            unsigned char ch = (unsigned char)input[pos];
            CLJSToken token = {false, (unsigned short)ch, 3};
            result->tokens[result->count++] = token;
            pos++;
        }
    }

    return result;
}

void freeCLJSTokenArray(CLJSTokenArray* arr) {
    if (arr) free(arr);
}

char* detokenizeCLJSTokenArray(const CLJSTokenArray* arr, int* cumLen) {
    if (!arr || !cumLen) return NULL;
    /* Max CLJS pattern: 28 chars ("document.removeEventListener"). */
    size_t bufSize = arr->count * 30 + 1;
    char* result = (char*)calloc(bufSize, 1);
    if (!result) return NULL;
    *cumLen = 0;
    for (size_t i = 0; i < arr->count; i++) {
        if (*cumLen >= (int)bufSize - 1) break;
        const CLJSToken* tok = &arr->tokens[i];
        if (tok->isPattern) {
            const char* pattern = CL_JS_EN_PATTERNS[tok->flag];
            size_t pLen = strlen(pattern);
            char tmp[64];
            if (pLen > sizeof(tmp) - 1) pLen = sizeof(tmp) - 1;
            memcpy(tmp, pattern, pLen);
            tmp[pLen] = '\0';
            /* Apply case style only for English digraph patterns. */
            if (CL_JS_EN_PATTERN_IS_DIGRAPH[tok->flag]) {
                switch (tok->caseStyle) {
                    case 1:
                        for (size_t j = 0; j < pLen; j++)
                            if (isalpha((unsigned char)tmp[j]))
                                tmp[j] = toupper((unsigned char)tmp[j]);
                        break;
                    case 2:
                        for (size_t j = 0; j < pLen; j++)
                            if (isalpha((unsigned char)tmp[j]))
                                tmp[j] = (j == 0) ? toupper((unsigned char)tmp[j])
                                                  : tolower((unsigned char)tmp[j]);
                        break;
                    case 3:
                        for (size_t j = 0; j < pLen; j++)
                            if (isalpha((unsigned char)tmp[j]))
                                tmp[j] = (j == pLen - 1) ? toupper((unsigned char)tmp[j])
                                                          : tolower((unsigned char)tmp[j]);
                        break;
                    default: break; /* case 0: already lowercase */
                }
            }
            /* Non-digraph patterns always use caseStyle=3 (no change). */
            size_t copy = pLen;
            if (*cumLen + (int)copy > (int)bufSize - 1)
                copy = (size_t)((int)bufSize - 1 - *cumLen);
            memcpy(result + *cumLen, tmp, copy);
            *cumLen += (int)copy;
        } else {
            result[*cumLen] = (char)tok->flag;
            (*cumLen)++;
        }
    }
    return result;
}
