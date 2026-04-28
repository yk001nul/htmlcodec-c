#include "cl-javascript-codec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static int cljs_find_sym(const CLJSVocabEntry* vocab, size_t vocab_size,
                         bool isPattern, unsigned short flag) {
    for (size_t i = 0; i < vocab_size; i++) {
        if (vocab[i].isPattern == isPattern && vocab[i].flag == flag)
            return (int)i;
    }
    return -1;
}

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

/* ── cljs_encode_ae_opt ───────────────────────────────────────────────────── */

unsigned char* cljs_encode_ae_opt(const CLJSTokenArray* arr, size_t count,
                                  size_t* outSize) {
    if (!arr || count == 0 || count > arr->count) {
        *outSize = 0;
        return NULL;
    }

    /* Pass 1: build first-appearance vocabulary */
    CLJSVocabEntry vocab[CL_JS_EN_MAX_TOKENS];
    size_t vocab_size = 0;

    for (size_t i = 0; i < count; i++) {
        bool           isp  = arr->tokens[i].isPattern;
        unsigned short flag = arr->tokens[i].flag;
        if (cljs_find_sym(vocab, vocab_size, isp, flag) < 0) {
            vocab[vocab_size].isPattern = isp;
            vocab[vocab_size].flag      = flag;
            vocab_size++;
            if (vocab_size >= CL_JS_EN_MAX_TOKENS) break;
        }
    }

    if (vocab_size == 0) { *outSize = 0; return NULL; }

    /* Count pattern tokens for caseStyle side-channel */
    size_t sc_count = 0;
    for (size_t i = 0; i < count; i++)
        if (arr->tokens[i].isPattern) sc_count++;

    /* Allocate output buffer (conservative worst-case) */
    size_t header_bits = 13 + 10 + vocab_size * 10 + 13 + sc_count * 2;
    size_t ae_bits     = count * 32 + 64;
    size_t buf_bytes   = (header_bits + ae_bits + 7) / 8;

    unsigned char* buffer = (unsigned char*)calloc(buf_bytes, 1);
    if (!buffer) { *outSize = 0; return NULL; }

    size_t bit_pos = 0;

    /* Write header */
    set_bits(buffer, bit_pos, 13, (unsigned int)count);       bit_pos += 13;
    set_bits(buffer, bit_pos, 10, (unsigned int)vocab_size);   bit_pos += 10;

    for (size_t i = 0; i < vocab_size; i++) {
        if (vocab[i].isPattern) {
            /* 1 + 9 bits (flag 0–511) */
            set_bits(buffer, bit_pos, 1, 1U);                           bit_pos += 1;
            set_bits(buffer, bit_pos, 9, (unsigned int)vocab[i].flag);  bit_pos += 9;
        } else {
            /* 1 + 8 bits (full byte 0-255, handles non-printable like \n, \t) */
            set_bits(buffer, bit_pos, 1, 0U);                              bit_pos += 1;
            set_bits(buffer, bit_pos, 8, (unsigned int)vocab[i].flag & 0xFFu); bit_pos += 8;
        }
    }

    /* caseStyle side-channel before AE stream */
    set_bits(buffer, bit_pos, 13, (unsigned int)sc_count); bit_pos += 13;
    for (size_t i = 0; i < count; i++) {
        if (arr->tokens[i].isPattern) {
            set_bits(buffer, bit_pos, 2, (unsigned int)arr->tokens[i].caseStyle);
            bit_pos += 2;
        }
    }

    /* Initialise adaptive order-1 count table: (vocab_size+1) × vocab_size */
    size_t   ctx_rows   = vocab_size + 1;
    uint32_t* count_table = (uint32_t*)malloc(ctx_rows * vocab_size * sizeof(uint32_t));
    if (!count_table) { free(buffer); *outSize = 0; return NULL; }
    for (size_t k = 0; k < ctx_rows * vocab_size; k++) count_table[k] = 1u;

    /* AE encode */
    uint32_t lo   = 0;
    uint32_t hi   = 0xFFFFFFFFu;
    int      pend = 0;
    size_t   ctx  = vocab_size; /* start-of-sequence sentinel */

    for (size_t i = 0; i < count; i++) {
        int sym = cljs_find_sym(vocab, vocab_size,
                                arr->tokens[i].isPattern, arr->tokens[i].flag);
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

    if (bit_pos + 10 > total_bits) return NULL;
    size_t vocab_size = get_bits(buffer, bit_pos, 10); bit_pos += 10;

    if (vocab_size == 0 || count == 0) {
        CLJSTokenArray* arr = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
        if (arr) arr->count = 0;
        return arr;
    }

    CLJSVocabEntry* vocab = (CLJSVocabEntry*)malloc(vocab_size * sizeof(CLJSVocabEntry));
    if (!vocab) return NULL;

    for (size_t i = 0; i < vocab_size; i++) {
        if (bit_pos >= total_bits) { free(vocab); return NULL; }
        unsigned int isp = get_bit(buffer, bit_pos); bit_pos += 1;
        if (isp) {
            if (bit_pos + 9 > total_bits) { free(vocab); return NULL; }
            vocab[i].isPattern = true;
            vocab[i].flag      = (unsigned short)get_bits(buffer, bit_pos, 9);
            bit_pos += 9;
        } else {
            if (bit_pos + 8 > total_bits) { free(vocab); return NULL; }
            vocab[i].isPattern = false;
            vocab[i].flag      = (unsigned short)get_bits(buffer, bit_pos, 8);
            bit_pos += 8;
        }
    }

    /* Read caseStyle side-channel */
    if (bit_pos + 13 > total_bits) { free(vocab); return NULL; }
    size_t sc_count = get_bits(buffer, bit_pos, 13); bit_pos += 13;

    int* cs_store = NULL;
    if (sc_count > 0) {
        cs_store = (int*)malloc(sc_count * sizeof(int));
        if (!cs_store) { free(vocab); return NULL; }
        for (size_t k = 0; k < sc_count; k++) {
            if (bit_pos + 2 > total_bits) { cs_store[k] = 0; continue; }
            cs_store[k] = (int)get_bits(buffer, bit_pos, 2);
            bit_pos += 2;
        }
    }

    /* Initialise adaptive order-1 count table */
    size_t   ctx_rows   = vocab_size + 1;
    uint32_t* count_table = (uint32_t*)malloc(ctx_rows * vocab_size * sizeof(uint32_t));
    if (!count_table) { free(cs_store); free(vocab); return NULL; }
    for (size_t k = 0; k < ctx_rows * vocab_size; k++) count_table[k] = 1u;

    /* Prime the AE code register with 32 bits MSB-first */
    uint32_t code = 0;
    for (int b = 31; b >= 0; b--)
        code |= (uint32_t)ae_read_bit_safe(buffer, bit_pos++, total_bits) << b;

    CLJSTokenArray* arr = (CLJSTokenArray*)malloc(sizeof(CLJSTokenArray));
    if (!arr) { free(count_table); free(cs_store); free(vocab); return NULL; }
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
        arr->tokens[arr->count].caseStyle = 0;
        arr->count++;

        row[(size_t)sym]++;
        ctx = (size_t)sym;
    }

    free(count_table);

    /* Apply caseStyle side-channel to pattern tokens */
    if (cs_store) {
        size_t cs_idx = 0;
        for (size_t i = 0; i < arr->count && cs_idx < sc_count; i++) {
            if (arr->tokens[i].isPattern)
                arr->tokens[i].caseStyle = cs_store[cs_idx++];
        }
        free(cs_store);
    }

    free(vocab);
    return arr;
}
