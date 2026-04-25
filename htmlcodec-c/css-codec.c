#include "css-codec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Bit I/O helpers (identical pattern to nl-en-codec) ─────────────────── */

static void set_bit(unsigned char* buf, size_t pos, unsigned char val) {
    if (val) buf[pos / 8] |=  (unsigned char)(1u << (pos % 8));
    else     buf[pos / 8] &= (unsigned char)~(1u << (pos % 8));
}

static unsigned char get_bit(const unsigned char* buf, size_t pos) {
    return (buf[pos / 8] >> (pos % 8)) & 1u;
}

static void set_bits(unsigned char* buf, size_t start, size_t n, unsigned int val) {
    for (size_t i = 0; i < n; i++)
        set_bit(buf, start + i, (unsigned char)((val >> i) & 1u));
}

static unsigned int get_bits(const unsigned char* buf, size_t start, size_t n) {
    unsigned int r = 0;
    for (size_t i = 0; i < n; i++)
        r |= (unsigned int)get_bit(buf, start + i) << i;
    return r;
}

/* ── Arithmetic-coding helpers ──────────────────────────────────────────── */

/* Renormalization boundaries (identical to nl-en-codec) */
#define AE_TOP   0x80000000u
#define AE_QRTR  0x40000000u

static void ae_build_cum_bounds(CSSAESymbol* syms, size_t n) {
    uint32_t total = 0;
    for (size_t i = 0; i < n; i++) total += syms[i].frequency;

    uint32_t cum = 0;
    for (size_t i = 0; i < n; i++) {
        syms[i].cum_low  = (uint32_t)((uint64_t)cum                       * CSS_AE_SCALE / total);
        syms[i].cum_high = (uint32_t)((uint64_t)(cum + syms[i].frequency) * CSS_AE_SCALE / total);
        cum += syms[i].frequency;
    }
}

static int ae_find_symbol(const CSSAESymbol* syms, size_t n,
                          const CSSTokenizable* tok) {
    for (size_t i = 0; i < n; i++) {
        if (syms[i].token.isPattern == tok->isPattern &&
            syms[i].token.flag      == tok->flag)
            return (int)i;
    }
    return -1;
}

static void ae_emit_bit(unsigned char* buf, size_t* bp, unsigned int bit) {
    set_bit(buf, *bp, (unsigned char)(bit & 1u));
    (*bp)++;
}

static void ae_renorm_enc(uint32_t* lo, uint32_t* hi, int* pend,
                           unsigned char* buf, size_t* bp) {
    while (1) {
        if (*hi < AE_TOP) {
            ae_emit_bit(buf, bp, 0);
            for (int k = 0; k < *pend; k++) ae_emit_bit(buf, bp, 1);
            *pend = 0;
            *lo = *lo << 1;
            *hi = (*hi << 1) | 1u;
        } else if (*lo >= AE_TOP) {
            ae_emit_bit(buf, bp, 1);
            for (int k = 0; k < *pend; k++) ae_emit_bit(buf, bp, 0);
            *pend = 0;
            *lo = (*lo - AE_TOP) << 1;
            *hi = (*hi - AE_TOP) << 1 | 1u;
        } else if (*lo >= AE_QRTR && *hi < (AE_TOP | AE_QRTR)) {
            (*pend)++;
            *lo = (*lo - AE_QRTR) << 1;
            *hi = (*hi - AE_QRTR) << 1 | 1u;
        } else {
            break;
        }
    }
}

static void ae_flush_enc(uint32_t lo, int pend, unsigned char* buf, size_t* bp) {
    pend++;
    if (lo < AE_QRTR) {
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
        if (*hi < AE_TOP) {
            *lo   = *lo << 1;
            *hi   = (*hi << 1) | 1u;
            *code = (*code << 1) | ae_read_bit_safe(buf, (*bp)++, total);
        } else if (*lo >= AE_TOP) {
            *lo   = (*lo - AE_TOP) << 1;
            *hi   = (*hi - AE_TOP) << 1 | 1u;
            *code = (*code - AE_TOP) << 1 | ae_read_bit_safe(buf, (*bp)++, total);
        } else if (*lo >= AE_QRTR && *hi < (AE_TOP | AE_QRTR)) {
            *lo   = (*lo - AE_QRTR) << 1;
            *hi   = (*hi - AE_QRTR) << 1 | 1u;
            *code = (*code - AE_QRTR) << 1 | ae_read_bit_safe(buf, (*bp)++, total);
        } else {
            break;
        }
    }
}

/* Count total CSSTokenizables across the whole array */
static size_t css_total_tokenizables(const CSSTokenArray* arr) {
    size_t total = 0;
    for (int t = 0; t < arr->count; t++) {
        const CSSToken* tok = &arr->tokens[t];
        if      (tok->type == 0) total += (size_t)tok->data.rule.ruleTokenSize;
        else if (tok->type == 1) total += (size_t)tok->data.atRule.atRuleTokenSize;
        else                     total += (size_t)tok->data.comment.commentTokenSize;
    }
    return total;
}

/* ── css_encode_ae ──────────────────────────────────────────────────────── */

unsigned char* css_encode_ae(const CSSTokenArray* arr, size_t* outSize) {
    *outSize = 0;
    if (!arr || arr->count == 0) return NULL;

    /* Build frequency map */
    CSSFreqMap* fmap = collectCSSFrequencies(arr);
    if (!fmap) return NULL;

    size_t unique = (size_t)fmap->uniqueCount;
    size_t total  = (size_t)fmap->totalTokens;

    /* Build AESymbol table */
    CSSAESymbol* syms = (CSSAESymbol*)malloc(unique * sizeof(CSSAESymbol));
    if (!syms) { freeCSSFreqMap(fmap); return NULL; }

    for (size_t i = 0; i < unique; i++) {
        syms[i].token     = fmap->entries[i].token;
        syms[i].frequency = (uint32_t)fmap->entries[i].frequency;
    }
    freeCSSFreqMap(fmap);

    ae_build_cum_bounds(syms, unique);

    /* Verify probability sum */
    if (syms[unique - 1].cum_high != CSS_AE_SCALE) {
        fprintf(stderr, "[CSS AE] Probability sum error: got %u\n",
                syms[unique - 1].cum_high);
        free(syms);
        return NULL;
    }

    /* Allocate output buffer.
     * Header: 10 + 11 + unique*22 + 9 + count*12 bits
     * AE stream: total*32 + 64 bits (conservative upper bound)            */
    size_t header_bits = 10 + 11 + unique * 22 + 9 + (size_t)arr->count * 12;
    size_t ae_bits     = total * 32 + 64;
    size_t bytes_needed = (header_bits + ae_bits + 7) / 8;
    unsigned char* buffer = (unsigned char*)calloc(bytes_needed, 1);
    if (!buffer) { free(syms); return NULL; }

    size_t bp = 0;

    /* 10 bits: total CSSTokenizable count */
    set_bits(buffer, bp, 10, (unsigned int)total); bp += 10;

    /* 11 bits: unique CSSTokenizable count */
    set_bits(buffer, bp, 11, (unsigned int)unique); bp += 11;

    /* Per-unique token */
    for (size_t i = 0; i < unique; i++) {
        if (syms[i].token.isPattern) {
            /* 1 + 11 + 10 = 22 bits */
            set_bits(buffer, bp,  1, 1u);                               bp += 1;
            set_bits(buffer, bp, 11, (unsigned int)syms[i].token.flag); bp += 11;
            set_bits(buffer, bp, 10, (unsigned int)syms[i].frequency);  bp += 10;
        } else {
            /* 1 + 7 + 10 = 18 bits; flag stored as (flag - 32) to fit 7 bits */
            unsigned int offset = (syms[i].token.flag >= 32u)
                                  ? (unsigned int)(syms[i].token.flag - 32u) : 0u;
            if (offset > 95u) offset = 95u;
            set_bits(buffer, bp,  1, 0u);     bp += 1;
            set_bits(buffer, bp,  7, offset); bp += 7;
            set_bits(buffer, bp, 10, (unsigned int)syms[i].frequency); bp += 10;
        }
    }

    /* 9 bits: CSSToken count */
    set_bits(buffer, bp, 9, (unsigned int)arr->count); bp += 9;

    /* Per-token: 2-bit type + 10-bit tokenizable size */
    for (int t = 0; t < arr->count; t++) {
        const CSSToken* tok = &arr->tokens[t];
        int tokSize = 0;
        if      (tok->type == 0) tokSize = tok->data.rule.ruleTokenSize;
        else if (tok->type == 1) tokSize = tok->data.atRule.atRuleTokenSize;
        else                     tokSize = tok->data.comment.commentTokenSize;

        set_bits(buffer, bp, 2,  (unsigned int)tok->type); bp += 2;
        set_bits(buffer, bp, 10, (unsigned int)tokSize);   bp += 10;
    }

    /* AE encode the flat CSSTokenizable sequence with renormalization */
    uint32_t lo = 0, hi = 0xFFFFFFFFu;
    int pend = 0;

    for (int t = 0; t < arr->count; t++) {
        const CSSToken* tok = &arr->tokens[t];
        const CSSTokenizable* src = NULL;
        int srcSize = 0;

        if (tok->type == 0) { src = tok->data.rule.ruleTokens;        srcSize = tok->data.rule.ruleTokenSize; }
        else if (tok->type == 1) { src = tok->data.atRule.atRuleTokens;  srcSize = tok->data.atRule.atRuleTokenSize; }
        else { src = tok->data.comment.commentTokens; srcSize = tok->data.comment.commentTokenSize; }

        for (int i = 0; i < srcSize; i++) {
            int idx = ae_find_symbol(syms, unique, &src[i]);
            if (idx < 0) { free(syms); free(buffer); return NULL; }

            uint64_t range = (uint64_t)(hi - lo) + 1;
            hi = lo + (uint32_t)(range * syms[idx].cum_high / CSS_AE_SCALE) - 1;
            lo = lo + (uint32_t)(range * syms[idx].cum_low  / CSS_AE_SCALE);

            ae_renorm_enc(&lo, &hi, &pend, buffer, &bp);
        }
    }

    ae_flush_enc(lo, pend, buffer, &bp);

    *outSize = (bp + 7) / 8;
    free(syms);
    return buffer;
}

/* ── css_decode_ae ──────────────────────────────────────────────────────── */

CSSTokenArray* css_decode_ae(const unsigned char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
        return arr;
    }

    size_t total_bits = bufferSize * 8;
    size_t bp = 0;

    /* 10 bits: total CSSTokenizable count */
    if (bp + 10 > total_bits) return NULL;
    size_t total = get_bits(buffer, bp, 10); bp += 10;

    /* 11 bits: unique count */
    if (bp + 11 > total_bits) return NULL;
    size_t unique = get_bits(buffer, bp, 11); bp += 11;

    if (unique == 0 || total == 0) {
        CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
        return arr;
    }

    /* Rebuild AESymbol table */
    CSSAESymbol* syms = (CSSAESymbol*)malloc(unique * sizeof(CSSAESymbol));
    if (!syms) return NULL;

    for (size_t i = 0; i < unique; i++) {
        if (bp >= total_bits) { free(syms); return NULL; }

        unsigned int isPattern = get_bit(buffer, bp); bp += 1;
        if (isPattern) {
            if (bp + 21 > total_bits) { free(syms); return NULL; }
            unsigned int flag = get_bits(buffer, bp, 11); bp += 11;
            unsigned int freq = get_bits(buffer, bp, 10); bp += 10;
            syms[i].token.isPattern = true;
            syms[i].token.flag      = (unsigned short)flag;
            syms[i].frequency       = freq;
        } else {
            if (bp + 17 > total_bits) { free(syms); return NULL; }
            unsigned int offset = get_bits(buffer, bp, 7); bp += 7;
            unsigned int freq   = get_bits(buffer, bp, 10); bp += 10;
            syms[i].token.isPattern = false;
            syms[i].token.flag      = (unsigned short)(offset + 32u);
            syms[i].frequency       = freq;
        }
    }

    /* 9 bits: CSSToken count */
    if (bp + 9 > total_bits) { free(syms); return NULL; }
    size_t tokenCount = get_bits(buffer, bp, 9); bp += 9;

    if (tokenCount == 0 || tokenCount > CSS_MAX_TOKENS) {
        free(syms);
        CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
        return arr;
    }

    /* Per-token structural metadata */
    int types[CSS_MAX_TOKENS];
    int sizes[CSS_MAX_TOKENS];
    for (size_t t = 0; t < tokenCount; t++) {
        if (bp + 12 > total_bits) { free(syms); return NULL; }
        types[t] = (int)get_bits(buffer, bp, 2); bp += 2;
        sizes[t] = (int)get_bits(buffer, bp, 10); bp += 10;
    }

    /* Rebuild cumulative probability bounds */
    ae_build_cum_bounds(syms, unique);

    /* Phase 1: arithmetic-decode the flat CSSTokenizable sequence.
     * The AE bitstream begins immediately after the per-token metadata.     */
    CSSTokenizable* flat = (CSSTokenizable*)malloc(total * sizeof(CSSTokenizable));
    if (!flat) { free(syms); return NULL; }

    /* Initialize code register: first 32 bits MSB-first from AE stream */
    uint32_t code = 0;
    for (int i = 31; i >= 0; i--)
        code |= (uint32_t)ae_read_bit_safe(buffer, bp++, total_bits) << i;

    uint32_t lo = 0, hi = 0xFFFFFFFFu;
    size_t decoded = 0;

    for (size_t i = 0; i < total; i++) {
        uint64_t range  = (uint64_t)(hi - lo) + 1;
        uint32_t scaled = (uint32_t)(((uint64_t)(code - lo + 1) * CSS_AE_SCALE - 1) / range);

        int sym_idx = -1;
        for (size_t j = 0; j < unique; j++) {
            if (scaled >= syms[j].cum_low && scaled < syms[j].cum_high) {
                sym_idx = (int)j;
                break;
            }
        }
        if (sym_idx < 0) {
            fprintf(stderr, "[CSS AE] Decode error at step %zu\n", i);
            break;
        }
        flat[decoded++] = syms[sym_idx].token;

        hi = lo + (uint32_t)(range * syms[sym_idx].cum_high / CSS_AE_SCALE) - 1;
        lo = lo + (uint32_t)(range * syms[sym_idx].cum_low  / CSS_AE_SCALE);

        ae_renorm_dec(&lo, &hi, &code, buffer, &bp, total_bits);
    }
    free(syms);

    /* Phase 2: distribute flat tokens back into CSSTokenArray */
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    if (!arr) { free(flat); return NULL; }

    size_t pos = 0;  /* cursor into flat[] */

    for (size_t t = 0; t < tokenCount; t++) {
        if ((int)arr->count >= CSS_MAX_TOKENS) break;
        CSSToken* out = &arr->tokens[arr->count++];
        out->type = types[t];
        int sz = sizes[t];

        if (types[t] == 1) {
            /* at-rule: copy tokens directly */
            int copied = 0;
            while (copied < sz && pos < decoded && copied < CSS_MAX_TOKENIZABLE) {
                out->data.atRule.atRuleTokens[copied++] = flat[pos++];
            }
            out->data.atRule.atRuleTokenSize = copied;

        } else if (types[t] == 2) {
            /* comment: copy tokens directly */
            int copied = 0;
            while (copied < sz && pos < decoded && copied < CSS_MAX_TOKENIZABLE) {
                out->data.comment.commentTokens[copied++] = flat[pos++];
            }
            out->data.comment.commentTokenSize = copied;

        } else {
            /* type 0 (selector rule): parse boundary sentinels */
            out->data.rule.ruleTokenSize = 0;
            out->data.rule.selectorTokenSize = 0;
            out->data.rule.propertyCount = 0;

            int totalRead = 0;
            bool inSelector = true;
            int propNameSize = 0;
            CSSTokenizable propNameBuf[CSS_MAX_TOKENIZABLE];
            int propValSize = 0;
            CSSTokenizable propValBuf[CSS_MAX_TOKENIZABLE];
            bool inName = false;

            while (totalRead < sz && pos < decoded) {
                CSSTokenizable ct = flat[pos++];
                totalRead++;

                /* copy to ruleTokens */
                if (out->data.rule.ruleTokenSize < CSS_MAX_TOKENIZABLE)
                    out->data.rule.ruleTokens[out->data.rule.ruleTokenSize++] = ct;

                if (!ct.isPattern) {
                    unsigned char ch = (unsigned char)ct.flag;

                    if (ch == '{' && inSelector) {
                        /* end of selector — everything collected so far is selector */
                        inSelector = false;
                        inName = true;
                        propNameSize = 0;
                        propValSize  = 0;
                        continue;
                    }
                    if (ch == ':' && !inSelector && inName) {
                        inName = false;
                        propValSize = 0;
                        continue;
                    }
                    if (ch == ';' && !inSelector && !inName) {
                        /* end of property — commit */
                        if (out->data.rule.propertyCount < CSS_MAX_PROPERTIES) {
                            CSSProperty* pr =
                                &out->data.rule.properties[out->data.rule.propertyCount++];
                            pr->nameTokenSize = propNameSize;
                            for (int k = 0; k < propNameSize; k++)
                                pr->nameTokens[k] = propNameBuf[k];
                            pr->valueTokenSize = propValSize;
                            for (int k = 0; k < propValSize; k++)
                                pr->valueTokens[k] = propValBuf[k];
                        }
                        propNameSize = 0;
                        propValSize  = 0;
                        inName = true;
                        continue;
                    }
                    if (ch == '}' && !inSelector) {
                        /* end of rule block */
                        break;
                    }
                }

                /* accumulate into the right buffer */
                if (inSelector) {
                    if (out->data.rule.selectorTokenSize < CSS_MAX_TOKENIZABLE)
                        out->data.rule.selectorTokens[out->data.rule.selectorTokenSize++] = ct;
                } else if (inName) {
                    if (propNameSize < CSS_MAX_TOKENIZABLE)
                        propNameBuf[propNameSize++] = ct;
                } else {
                    if (propValSize < CSS_MAX_TOKENIZABLE)
                        propValBuf[propValSize++] = ct;
                }
            }
        }
    }

    free(flat);
    return arr;
}
