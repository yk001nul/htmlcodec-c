#include "cl-javascript-codec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Use bitmap vocab header when vocab_size is large enough that the fixed
 * 502+256=758-bit overhead beats per-entry encoding (~9.7 bits/entry). */
#define CLJS_BITMAP_THRESHOLD 80

/* ── Bit I/O helpers ──────────────────────────────────────────────────────── */

static void set_bit(unsigned char* buf, size_t pos, unsigned char v) {
    size_t byte = pos / 8, bit = pos % 8;
    if (v) buf[byte] |=  (unsigned char)(1u << bit);
    else   buf[byte] &= (unsigned char)~(1u << bit);
}

static unsigned char get_bit(const unsigned char* buf, size_t pos) {
    return (buf[pos / 8] >> (pos % 8)) & 1u;
}

static void set_bits(unsigned char* buf, size_t start, size_t n, unsigned int v) {
    for (size_t i = 0; i < n; i++) set_bit(buf, start + i, (unsigned char)((v >> i) & 1u));
}

static unsigned int get_bits(const unsigned char* buf, size_t start, size_t n) {
    unsigned int r = 0;
    for (size_t i = 0; i < n; i++) r |= (unsigned int)get_bit(buf, start + i) << i;
    return r;
}

/* ── Renormalization boundaries ───────────────────────────────────────────── */

#define JS_AE_TOP   0x80000000u
#define JS_AE_QRTR  0x40000000u

static void ae_emit_bit(unsigned char* buf, size_t* bp, unsigned int bit) {
    set_bit(buf, *bp, (unsigned char)(bit & 1u));
    (*bp)++;
}

static void ae_renorm_enc(uint32_t* lo, uint32_t* hi, int* pend,
                          unsigned char* buf, size_t* bp) {
    while (1) {
        if (*hi < JS_AE_TOP) {
            ae_emit_bit(buf, bp, 0);
            for (int k = 0; k < *pend; k++) ae_emit_bit(buf, bp, 1);
            *pend = 0;
            *lo = *lo << 1;
            *hi = (*hi << 1) | 1u;
        } else if (*lo >= JS_AE_TOP) {
            ae_emit_bit(buf, bp, 1);
            for (int k = 0; k < *pend; k++) ae_emit_bit(buf, bp, 0);
            *pend = 0;
            *lo = (*lo - JS_AE_TOP) << 1;
            *hi = (*hi - JS_AE_TOP) << 1 | 1u;
        } else if (*lo >= JS_AE_QRTR && *hi < (JS_AE_TOP | JS_AE_QRTR)) {
            (*pend)++;
            *lo = (*lo - JS_AE_QRTR) << 1;
            *hi = (*hi - JS_AE_QRTR) << 1 | 1u;
        } else {
            break;
        }
    }
}

static void ae_flush_enc(uint32_t lo, int pend, unsigned char* buf, size_t* bp) {
    pend++;
    if (lo < JS_AE_QRTR) {
        ae_emit_bit(buf, bp, 0);
        for (int k = 0; k < pend; k++) ae_emit_bit(buf, bp, 1);
    } else {
        ae_emit_bit(buf, bp, 1);
        for (int k = 0; k < pend; k++) ae_emit_bit(buf, bp, 0);
    }
}

static unsigned int ae_read_bit_safe(const unsigned char* buf, size_t pos, size_t total) {
    return (pos < total) ? (unsigned int)get_bit(buf, pos) : 0u;
}

static void ae_renorm_dec(uint32_t* lo, uint32_t* hi, uint32_t* code,
                          const unsigned char* buf, size_t* bp, size_t total) {
    while (1) {
        if (*hi < JS_AE_TOP) {
            *lo   = *lo << 1;
            *hi   = (*hi << 1) | 1u;
            *code = (*code << 1) | ae_read_bit_safe(buf, (*bp)++, total);
        } else if (*lo >= JS_AE_TOP) {
            *lo   = (*lo - JS_AE_TOP) << 1;
            *hi   = (*hi - JS_AE_TOP) << 1 | 1u;
            *code = (*code - JS_AE_TOP) << 1 | ae_read_bit_safe(buf, (*bp)++, total);
        } else if (*lo >= JS_AE_QRTR && *hi < (JS_AE_TOP | JS_AE_QRTR)) {
            *lo   = (*lo - JS_AE_QRTR) << 1;
            *hi   = (*hi - JS_AE_QRTR) << 1 | 1u;
            *code = (*code - JS_AE_QRTR) << 1 | ae_read_bit_safe(buf, (*bp)++, total);
        } else {
            break;
        }
    }
}

/* ── Vocab helpers ────────────────────────────────────────────────────────── */

typedef struct {
    bool           isPattern;
    unsigned short flag;
} CLJSVocabEntry;

/* Compute [cum_low, cum_high) for symbol sym in given count row. */
static void cljs_cum_bounds(const uint32_t* row, size_t vocab_size, size_t sym,
                            uint32_t* out_low, uint32_t* out_high) {
    uint32_t total = 0;
    for (size_t j = 0; j < vocab_size; j++) total += row[j];

    uint32_t cum = 0;
    for (size_t j = 0; j < vocab_size; j++) {
        if (j == sym) {
            *out_low  = (uint32_t)((uint64_t)cum * CL_JS_AE_SCALE / total);
            *out_high = (j + 1 == vocab_size)
                      ? CL_JS_AE_SCALE
                      : (uint32_t)((uint64_t)(cum + row[j]) * CL_JS_AE_SCALE / total);
            return;
        }
        cum += row[j];
    }
    *out_low = *out_high = 0;
}

/* Decode: find the sym whose interval contains scaled. */
static int cljs_find_sym_for_scaled(const uint32_t* row, size_t vocab_size,
                                    uint32_t scaled) {
    uint32_t total = 0;
    for (size_t j = 0; j < vocab_size; j++) total += row[j];

    uint32_t cum = 0;
    for (size_t j = 0; j < vocab_size; j++) {
        uint32_t lo = (uint32_t)((uint64_t)cum * CL_JS_AE_SCALE / total);
        uint32_t hi = (j + 1 == vocab_size)
                    ? CL_JS_AE_SCALE
                    : (uint32_t)((uint64_t)(cum + row[j]) * CL_JS_AE_SCALE / total);
        if (scaled >= lo && scaled < hi) return (int)j;
        cum += row[j];
    }
    return -1;
}

/* ── JS structural bigram seeding ─────────────────────────────────────────── */

static void cljs_seed_count_table(uint32_t* count_table, size_t vocab_size,
                                   const CLJSVocabEntry* vocab) {
    /* Locate single-character ASCII context tokens */
    int v_semi = -1, v_obrace = -1, v_cbrace = -1;
    int v_space = -1, v_newline = -1, v_oparen = -1, v_cparen = -1;
    int v_dot = -1, v_comma = -1, v_eq = -1, v_obracket = -1;

    for (size_t i = 0; i < vocab_size; i++) {
        if (vocab[i].isPattern) continue;
        unsigned short fl = vocab[i].flag;
        if      (fl == ';')  v_semi     = (int)i;
        else if (fl == '{')  v_obrace   = (int)i;
        else if (fl == '}')  v_cbrace   = (int)i;
        else if (fl == ' ')  v_space    = (int)i;
        else if (fl == '\n') v_newline  = (int)i;
        else if (fl == '(')  v_oparen   = (int)i;
        else if (fl == ')')  v_cparen   = (int)i;
        else if (fl == '.')  v_dot      = (int)i;
        else if (fl == ',')  v_comma    = (int)i;
        else if (fl == '=')  v_eq       = (int)i;
        else if (fl == '[')  v_obracket = (int)i;
    }

/* Only seeds a single specific cell — no loop dilution. */
#define SEED(row_idx, col_idx, amt) \
    do { if ((row_idx) >= 0 && (col_idx) >= 0) \
         count_table[(size_t)(row_idx) * vocab_size + (size_t)(col_idx)] += (amt); } while(0)

    /* ── High-volume ASCII transitions ── */
    /* After '\n': indentation space is by far most common */
    SEED(v_newline, v_space,   200);
    SEED(v_newline, v_cbrace,   50); /* closing block at start of line */
    SEED(v_newline, v_newline,  20); /* blank lines */

    /* After ';': newline almost always follows in formatted code */
    SEED(v_semi, v_newline, 120);
    SEED(v_semi, v_space,    20);
    SEED(v_semi, v_cbrace,   15);

    /* After '}': newline most common; nested } or ; occasionally */
    SEED(v_cbrace, v_newline, 120);
    SEED(v_cbrace, v_cbrace,   30);
    SEED(v_cbrace, v_semi,     15);
    SEED(v_cbrace, v_space,    15);

    /* After '{': newline almost always follows in block bodies */
    SEED(v_obrace, v_newline, 100);
    SEED(v_obrace, v_space,    20); /* one-liner objects: { key: val } */

    /* After ',': space then next element */
    SEED(v_comma, v_space,   150);
    SEED(v_comma, v_newline,  20);

    /* After ' ': another space (indentation) or open paren are common */
    SEED(v_space, v_space,   80);
    SEED(v_space, v_oparen,  10);

    /* After '(': closing ')' (empty call) or expression */
    SEED(v_oparen, v_cparen, 30);

    /* After '=' (bare equals): space */
    SEED(v_eq, v_space,    80);
    SEED(v_eq, v_oparen,   20);
    SEED(v_eq, v_obracket, 15);
    SEED(v_eq, v_obrace,   10);

    /* After ')': space, '{', or ';' */
    SEED(v_cparen, v_space,   60);
    SEED(v_cparen, v_obrace,  40);
    SEED(v_cparen, v_semi,    30);
    SEED(v_cparen, v_newline, 20);
    SEED(v_cparen, v_dot,     15);

    /* ── Pattern context rows: only targeted single-cell seeds ── */
    for (size_t ctx = 0; ctx < vocab_size; ctx++) {
        if (!vocab[ctx].isPattern) continue;
        const char* cp   = CL_JS_EN_PATTERNS[vocab[ctx].flag];
        size_t      clen = strlen(cp);

        /* Declaration keywords → space (almost certain) */
        if (strcmp(cp, "const") == 0 || strcmp(cp, "let") == 0 ||
            strcmp(cp, "var")   == 0) {
            SEED((int)ctx, v_space, 200);
        }
        /* function → space (named) or '(' (anonymous) */
        else if (strcmp(cp, "function") == 0) {
            SEED((int)ctx, v_space,  150);
            SEED((int)ctx, v_oparen,  60);
        }
        else if (strcmp(cp, "async function ") == 0) {
            SEED((int)ctx, v_space, 150);
        }
        /* return → space or ';' */
        else if (strcmp(cp, "return") == 0) {
            SEED((int)ctx, v_space,  120);
            SEED((int)ctx, v_semi,    40);
            SEED((int)ctx, v_oparen,  25);
        }
        /* this → '.' (almost always in JS class bodies) */
        else if (strcmp(cp, "this") == 0) {
            SEED((int)ctx, v_dot, 250);
        }
        /* new → space then constructor */
        else if (strcmp(cp, "new") == 0) {
            SEED((int)ctx, v_space, 200);
        }
        /* class / class  → space then name */
        else if (strcmp(cp, "class") == 0 || strcmp(cp, "class ") == 0) {
            SEED((int)ctx, v_space, 180);
        }
        /* import → space or '{' */
        else if (strcmp(cp, "import") == 0) {
            SEED((int)ctx, v_space,  100);
            SEED((int)ctx, v_obrace,  50);
        }
        /* export → space */
        else if (strcmp(cp, "export") == 0) {
            SEED((int)ctx, v_space, 150);
        }
        /* await → space */
        else if (strcmp(cp, "await") == 0 || strcmp(cp, "await ") == 0) {
            SEED((int)ctx, v_space, 180);
        }
        /* if / for / while / switch → space or '(' */
        else if (strcmp(cp, "if") == 0 || strcmp(cp, "for") == 0 ||
                 strcmp(cp, "while") == 0 || strcmp(cp, "switch") == 0) {
            SEED((int)ctx, v_space,  100);
            SEED((int)ctx, v_oparen,  80);
        }
        /* extends / throw → space */
        else if (strcmp(cp, "extends") == 0 || strcmp(cp, "extends ") == 0 ||
                 strcmp(cp, "throw")   == 0) {
            SEED((int)ctx, v_space, 150);
        }
        /* Compound patterns ending in '{': newline then indent */
        else if (clen > 0 && cp[clen - 1] == '{') {
            SEED((int)ctx, v_newline, 100);
            SEED((int)ctx, v_space,    40);
        }
        /* Compound patterns ending in ';': newline follows */
        else if (clen > 0 && cp[clen - 1] == ';') {
            SEED((int)ctx, v_newline, 120);
            SEED((int)ctx, v_space,    20);
        }
        /* "} else"/"} catch"/"} finally": space then '{' */
        else if (strncmp(cp, "} else",    6) == 0 ||
                 strncmp(cp, "} catch",   7) == 0 ||
                 strncmp(cp, "} finally", 9) == 0) {
            SEED((int)ctx, v_space,  80);
            SEED((int)ctx, v_obrace, 60);
        }
        /* method-call chains ending in ')': ';', newline, or another '.' */
        else if (clen > 1 && cp[0] == '.' && cp[clen - 1] == ')') {
            SEED((int)ctx, v_semi,    50);
            SEED((int)ctx, v_newline, 30);
            SEED((int)ctx, v_space,   20);
        }
        /* Operator patterns with spaces (" = ", " === ", etc.) */
        else if (clen > 2 && cp[0] == ' ' && cp[clen - 1] == ' ') {
            SEED((int)ctx, v_oparen, 15);
            SEED((int)ctx, v_obrace, 10);
        }
        /* import { / export { → space then identifier */
        else if (strcmp(cp, "import {") == 0 || strcmp(cp, "export {") == 0) {
            SEED((int)ctx, v_space, 200);
        }
    }

    /* After '.' → method-call patterns starting with '.' */
    if (v_dot >= 0) {
        for (size_t j = 0; j < vocab_size; j++) {
            if (!vocab[j].isPattern) continue;
            const char* jp = CL_JS_EN_PATTERNS[vocab[j].flag];
            if (jp[0] == '.') count_table[(size_t)v_dot * vocab_size + j] += 50;
        }
    }

    /* After '}' compound patterns: seed "} else/catch/finally" targets */
    if (v_cbrace >= 0) {
        for (size_t j = 0; j < vocab_size; j++) {
            if (!vocab[j].isPattern) continue;
            const char* jp = CL_JS_EN_PATTERNS[vocab[j].flag];
            if (strncmp(jp, "} else",    6) == 0 ||
                strncmp(jp, "} catch",   7) == 0 ||
                strncmp(jp, "} finally", 9) == 0) {
                count_table[(size_t)v_cbrace * vocab_size + j] += 60;
            }
        }
    }

    /* Start-of-sequence → module-level statement starters */
    {
        uint32_t* row = count_table + vocab_size * vocab_size;
        for (size_t j = 0; j < vocab_size; j++) {
            if (!vocab[j].isPattern) continue;
            const char* jp = CL_JS_EN_PATTERNS[vocab[j].flag];
            if (strcmp(jp, "import") == 0 || strcmp(jp, "export") == 0 ||
                strcmp(jp, "function") == 0 || strcmp(jp, "class") == 0 ||
                strcmp(jp, "const") == 0   || strcmp(jp, "let") == 0   ||
                strncmp(jp, "import {", 8)         == 0 ||
                strncmp(jp, "export default ",  15) == 0 ||
                strncmp(jp, "export const ",    13) == 0 ||
                strncmp(jp, "export function ", 16) == 0 ||
                strncmp(jp, "async function ",  15) == 0) {
                row[j] += 30;
            }
        }
    }

#undef SEED
}

/* ── cljs_encode_ae_opt ───────────────────────────────────────────────────── */

unsigned char* cljs_encode_ae_opt(const CLJSTokenArray* arr, size_t count,
                                  size_t* outSize) {
    if (!arr || count == 0 || count > arr->count) {
        *outSize = 0;
        return NULL;
    }

    /* ── Build bitmaps and sorted vocab ── */
    unsigned char pat_bmp[64] = {0}; /* 502 bits: which sorted-pattern indices used */
    unsigned char asc_bmp[32] = {0}; /* 256 bits: which ASCII byte values used */

    for (size_t i = 0; i < count; i++) {
        if (arr->tokens[i].isPattern) {
            unsigned short f = arr->tokens[i].flag;
            pat_bmp[f >> 3] |= (unsigned char)(1u << (f & 7));
        } else {
            unsigned char f = (unsigned char)arr->tokens[i].flag;
            asc_bmp[f >> 3] |= (unsigned char)(1u << (f & 7));
        }
    }

    /* Vocab in sorted order: patterns (flag 0..501 ascending) then ASCII (0..255) */
    CLJSVocabEntry vocab[CL_JS_EN_PATTERN_COUNT + 256];
    int pat_rank[CL_JS_EN_PATTERN_COUNT]; /* flag → vocab index, -1 if absent */
    int asc_rank[256];
    size_t vocab_size = 0;

    for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
        if (pat_bmp[i >> 3] & (1u << (i & 7))) {
            pat_rank[i] = (int)vocab_size;
            vocab[vocab_size].isPattern = true;
            vocab[vocab_size].flag      = (unsigned short)i;
            vocab_size++;
        } else {
            pat_rank[i] = -1;
        }
    }
    for (int i = 0; i < 256; i++) {
        if (asc_bmp[i >> 3] & (1u << (i & 7))) {
            asc_rank[i] = (int)vocab_size;
            vocab[vocab_size].isPattern = false;
            vocab[vocab_size].flag      = (unsigned short)i;
            vocab_size++;
        } else {
            asc_rank[i] = -1;
        }
    }

    if (vocab_size == 0) { *outSize = 0; return NULL; }

    /* ── Count digraph tokens for caseStyle side-channel ── */
    size_t sc_count = 0;
    for (size_t i = 0; i < count; i++)
        if (arr->tokens[i].isPattern &&
            CL_JS_EN_PATTERN_IS_DIGRAPH[arr->tokens[i].flag])
            sc_count++;

    /* ── Choose header format ── */
    int use_bmp = (vocab_size >= CLJS_BITMAP_THRESHOLD) ? 1 : 0;

    /* ── Allocate output buffer (conservative worst-case) ── */
    size_t hdr_bits = 13 + 1
        + (use_bmp ? (CL_JS_EN_PATTERN_COUNT + 256)
                   : (10 + vocab_size * 10))
        + 13 + sc_count * 2;
    size_t ae_bits   = count * 32 + 64;
    size_t buf_bytes = (hdr_bits + ae_bits + 7) / 8;

    unsigned char* buffer = (unsigned char*)calloc(buf_bytes, 1);
    if (!buffer) { *outSize = 0; return NULL; }

    size_t bit_pos = 0;

    /* ── Write header ── */
    set_bits(buffer, bit_pos, 13, (unsigned int)count);   bit_pos += 13;
    set_bits(buffer, bit_pos,  1, (unsigned int)use_bmp); bit_pos +=  1;

    if (use_bmp) {
        /* Write 502-bit pattern bitmap */
        for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++)
            set_bit(buffer, bit_pos + i, (pat_bmp[i >> 3] >> (i & 7)) & 1u);
        bit_pos += CL_JS_EN_PATTERN_COUNT;
        /* Write 256-bit ASCII bitmap */
        for (int i = 0; i < 256; i++)
            set_bit(buffer, bit_pos + i, (asc_bmp[i >> 3] >> (i & 7)) & 1u);
        bit_pos += 256;
    } else {
        set_bits(buffer, bit_pos, 10, (unsigned int)vocab_size); bit_pos += 10;
        for (size_t i = 0; i < vocab_size; i++) {
            if (vocab[i].isPattern) {
                set_bits(buffer, bit_pos, 1, 1U);                            bit_pos += 1;
                set_bits(buffer, bit_pos, 9, (unsigned int)vocab[i].flag);   bit_pos += 9;
            } else {
                set_bits(buffer, bit_pos, 1, 0U);                                   bit_pos += 1;
                set_bits(buffer, bit_pos, 8, (unsigned int)vocab[i].flag & 0xFFu);  bit_pos += 8;
            }
        }
    }

    /* ── caseStyle side-channel — digraph pattern tokens only ── */
    set_bits(buffer, bit_pos, 13, (unsigned int)sc_count); bit_pos += 13;
    for (size_t i = 0; i < count; i++) {
        if (arr->tokens[i].isPattern &&
            CL_JS_EN_PATTERN_IS_DIGRAPH[arr->tokens[i].flag]) {
            set_bits(buffer, bit_pos, 2, (unsigned int)arr->tokens[i].caseStyle);
            bit_pos += 2;
        }
    }

    /* ── Initialise adaptive order-1 count table: (vocab_size+1) × vocab_size ── */
    size_t    ctx_rows    = vocab_size + 1;
    uint32_t* count_table = (uint32_t*)malloc(ctx_rows * vocab_size * sizeof(uint32_t));
    if (!count_table) { free(buffer); *outSize = 0; return NULL; }
    for (size_t k = 0; k < ctx_rows * vocab_size; k++) count_table[k] = 1u;

    cljs_seed_count_table(count_table, vocab_size, vocab);

    /* ── AE encode ── */
    uint32_t lo   = 0;
    uint32_t hi   = 0xFFFFFFFFu;
    int      pend = 0;
    size_t   ctx  = vocab_size; /* start-of-sequence sentinel */

    for (size_t i = 0; i < count; i++) {
        int sym = arr->tokens[i].isPattern
                  ? pat_rank[arr->tokens[i].flag]
                  : asc_rank[(unsigned char)arr->tokens[i].flag];
        if (sym < 0) { free(count_table); free(buffer); *outSize = 0; return NULL; }

        uint32_t* row = count_table + ctx * vocab_size;
        uint32_t  s_low, s_high;
        cljs_cum_bounds(row, vocab_size, (size_t)sym, &s_low, &s_high);

        uint64_t range = (uint64_t)(hi - lo) + 1;
        hi = lo + (uint32_t)(range * s_high / CL_JS_AE_SCALE) - 1;
        lo = lo + (uint32_t)(range * s_low  / CL_JS_AE_SCALE);

        ae_renorm_enc(&lo, &hi, &pend, buffer, &bit_pos);

        row[(size_t)sym]++;
        ctx = (size_t)sym;
    }
    ae_flush_enc(lo, pend, buffer, &bit_pos);
    free(count_table);

    *outSize = (bit_pos + 7) / 8;
    return buffer;
}

/* ── cljs_decode_ae_opt ───────────────────────────────────────────────────── */

CLJSTokenArray* cljs_decode_ae_opt(const unsigned char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        CLJSTokenArray* arr = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
        if (arr) arr->count = 0;
        return arr;
    }

    size_t total_bits = bufferSize * 8;
    size_t bit_pos    = 0;

    if (bit_pos + 13 > total_bits) return NULL;
    size_t count = get_bits(buffer, bit_pos, 13); bit_pos += 13;

    if (bit_pos + 1 > total_bits) return NULL;
    int use_bmp = (int)get_bits(buffer, bit_pos, 1); bit_pos += 1;

    CLJSVocabEntry vocab[CL_JS_EN_PATTERN_COUNT + 256];
    size_t vocab_size = 0;

    if (use_bmp) {
        /* Read 502-bit pattern bitmap */
        if (bit_pos + CL_JS_EN_PATTERN_COUNT > total_bits) return NULL;
        for (int i = 0; i < CL_JS_EN_PATTERN_COUNT; i++) {
            if (get_bit(buffer, bit_pos + i)) {
                vocab[vocab_size].isPattern = true;
                vocab[vocab_size].flag      = (unsigned short)i;
                vocab_size++;
            }
        }
        bit_pos += CL_JS_EN_PATTERN_COUNT;
        /* Read 256-bit ASCII bitmap */
        if (bit_pos + 256 > total_bits) return NULL;
        for (int i = 0; i < 256; i++) {
            if (get_bit(buffer, bit_pos + i)) {
                vocab[vocab_size].isPattern = false;
                vocab[vocab_size].flag      = (unsigned short)i;
                vocab_size++;
            }
        }
        bit_pos += 256;
    } else {
        if (bit_pos + 10 > total_bits) return NULL;
        vocab_size = get_bits(buffer, bit_pos, 10); bit_pos += 10;
        if (vocab_size == 0) {
            CLJSTokenArray* arr = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
            if (arr) arr->count = 0;
            return arr;
        }
        for (size_t i = 0; i < vocab_size; i++) {
            if (bit_pos >= total_bits) return NULL;
            unsigned int isp = get_bit(buffer, bit_pos); bit_pos += 1;
            if (isp) {
                if (bit_pos + 9 > total_bits) return NULL;
                vocab[i].isPattern = true;
                vocab[i].flag      = (unsigned short)get_bits(buffer, bit_pos, 9);
                bit_pos += 9;
            } else {
                if (bit_pos + 8 > total_bits) return NULL;
                vocab[i].isPattern = false;
                vocab[i].flag      = (unsigned short)get_bits(buffer, bit_pos, 8);
                bit_pos += 8;
            }
        }
    }

    if (vocab_size == 0 || count == 0) {
        CLJSTokenArray* arr = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
        if (arr) arr->count = 0;
        return arr;
    }

    /* ── Read caseStyle side-channel (digraph pattern tokens only) ── */
    if (bit_pos + 13 > total_bits) return NULL;
    size_t sc_count = get_bits(buffer, bit_pos, 13); bit_pos += 13;

    int* cs_store = NULL;
    if (sc_count > 0) {
        cs_store = (int*)malloc(sc_count * sizeof(int));
        if (!cs_store) return NULL;
        for (size_t k = 0; k < sc_count; k++) {
            if (bit_pos + 2 > total_bits) { cs_store[k] = 0; continue; }
            cs_store[k] = (int)get_bits(buffer, bit_pos, 2);
            bit_pos += 2;
        }
    }

    /* ── Initialise adaptive order-1 count table ── */
    size_t    ctx_rows    = vocab_size + 1;
    uint32_t* count_table = (uint32_t*)malloc(ctx_rows * vocab_size * sizeof(uint32_t));
    if (!count_table) { free(cs_store); return NULL; }
    for (size_t k = 0; k < ctx_rows * vocab_size; k++) count_table[k] = 1u;

    cljs_seed_count_table(count_table, vocab_size, vocab);

    /* ── Prime AE code register with 32 bits MSB-first ── */
    uint32_t code = 0;
    for (int b = 31; b >= 0; b--)
        code |= (uint32_t)ae_read_bit_safe(buffer, bit_pos++, total_bits) << b;

    CLJSTokenArray* arr = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
    if (!arr) { free(count_table); free(cs_store); return NULL; }
    arr->count = 0;

    uint32_t lo  = 0;
    uint32_t hi  = 0xFFFFFFFFu;
    size_t   ctx = vocab_size;

    for (size_t i = 0; i < count && arr->count < CL_JS_EN_MAX_TOKENS; i++) {
        uint32_t* row    = count_table + ctx * vocab_size;
        uint64_t  range  = (uint64_t)(hi - lo) + 1;
        uint32_t  scaled = (uint32_t)(((uint64_t)(code - lo + 1) * CL_JS_AE_SCALE - 1) / range);

        int sym = cljs_find_sym_for_scaled(row, vocab_size, scaled);
        if (sym < 0) break;

        uint32_t s_low, s_high;
        cljs_cum_bounds(row, vocab_size, (size_t)sym, &s_low, &s_high);

        hi = lo + (uint32_t)(range * s_high / CL_JS_AE_SCALE) - 1;
        lo = lo + (uint32_t)(range * s_low  / CL_JS_AE_SCALE);

        ae_renorm_dec(&lo, &hi, &code, buffer, &bit_pos, total_bits);

        arr->tokens[arr->count].isPattern = vocab[sym].isPattern;
        arr->tokens[arr->count].flag      = vocab[sym].flag;
        /* Non-digraph patterns always have caseStyle=3; digraphs read from side-channel */
        arr->tokens[arr->count].caseStyle =
            (vocab[sym].isPattern && !CL_JS_EN_PATTERN_IS_DIGRAPH[vocab[sym].flag]) ? 3 : 0;
        arr->count++;

        row[(size_t)sym]++;
        ctx = (size_t)sym;
    }

    free(count_table);

    /* ── Apply digraph caseStyle side-channel ── */
    if (cs_store) {
        size_t cs_idx = 0;
        for (size_t i = 0; i < arr->count && cs_idx < sc_count; i++) {
            if (arr->tokens[i].isPattern &&
                CL_JS_EN_PATTERN_IS_DIGRAPH[arr->tokens[i].flag])
                arr->tokens[i].caseStyle = cs_store[cs_idx++];
        }
        free(cs_store);
    }

    return arr;
}
