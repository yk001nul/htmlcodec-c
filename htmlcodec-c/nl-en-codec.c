#include "nl-en-codec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Helper: Set a bit at a specific position in a byte buffer
 */
static void set_bit(unsigned char* buffer, size_t bit_pos, unsigned char value) {
    size_t byte_pos = bit_pos / 8;
    unsigned char bit_index = bit_pos % 8;
    if (value) {
        buffer[byte_pos] |= (1 << bit_index);
    } else {
        buffer[byte_pos] &= ~(1 << bit_index);
    }
}

/**
 * Helper: Get a bit at a specific position in a byte buffer
 */
static unsigned char get_bit(const unsigned char* buffer, size_t bit_pos) {
    size_t byte_pos = bit_pos / 8;
    unsigned char bit_index = bit_pos % 8;
    return (buffer[byte_pos] >> bit_index) & 1;
}

/**
 * Helper: Set multiple bits (up to 32 bits) at a specific position
 */
static void set_bits(unsigned char* buffer, size_t start_bit, size_t num_bits, unsigned int value) {
    for (size_t i = 0; i < num_bits; i++) {
        unsigned char bit_value = (value >> i) & 1;
        set_bit(buffer, start_bit + i, bit_value);
    }
}

/**
 * Helper: Get multiple bits (up to 32 bits) from a specific position
 */
static unsigned int get_bits(const unsigned char* buffer, size_t start_bit, size_t num_bits) {
    unsigned int result = 0;
    for (size_t i = 0; i < num_bits; i++) {
        unsigned char bit_value = get_bit(buffer, start_bit + i);
        result |= (bit_value << i);
    }
    return result;
}

/**
 * Calculate the bit width needed to represent a value without leading zeros.
 * Minimum is 1 (even for value 0).
 */
static unsigned int bit_width_of(unsigned short value) {
    if (value == 0) return 1;
    unsigned int width = 0;
    unsigned short v = value;
    while (v > 0) {
        width++;
        v >>= 1;
    }
    return width;
}

/**
 * Calculate the total buffer size needed for encoding (worst case).
 *
 * Per token worst case:
 *   Pattern token:  1 (isPattern) + 4 (bitLength) + 9 (max index bits for 512 patterns) + 2 (caseStyle) = 16 bits
 *   ASCII token:    1 (isPattern) + 8 (char) = 9 bits
 * We use 16 bits per token as the worst-case allocation.
 */
static size_t calculate_buffer_size_bits(size_t token_count) {
    // 13 bits for count (max 4096 values)
    size_t total_bits = 13;

    // Add bits for each token - worst case is 16 bits (pattern token with 9-bit index)
    total_bits += token_count * 16;

    return total_bits;
}

unsigned char* nl_en_encode(const NLTokenArray* arr, size_t count, size_t* outSize) {
    if (!arr || count > arr->count) {
        *outSize = 0;
        return NULL;
    }

    // Calculate required buffer size in bits (worst case allocation)
    size_t total_bits = calculate_buffer_size_bits(count);
    size_t total_bytes = (total_bits + 7) / 8;  // Round up to nearest byte

    // Allocate and zero-initialize buffer
    unsigned char* buffer = (unsigned char*)calloc(total_bytes, sizeof(unsigned char));
    if (!buffer) {
        *outSize = 0;
        return NULL;
    }

    // Write array count in first 13 bits (max 4096)
    set_bits(buffer, 0, 13, (unsigned int)count);

    size_t current_bit = 13;

    // Write each token
    for (size_t i = 0; i < count; i++) {
        const NLToken* token = &arr->tokens[i];

        if (token->isPattern) {
            // Variable-width pattern token:
            //   1 bit:        isPattern = 1
            //   4 bits:       bit length N of the index value
            //   N bits:       index value (no leading zeros, minimum 1 bit)
            //   2 bits:       caseStyle

            unsigned int bw = bit_width_of(token->flag);

            set_bits(buffer, current_bit, 1, 1U);        // isPattern = 1
            current_bit += 1;

            set_bits(buffer, current_bit, 4, bw);         // bit length
            current_bit += 4;

            set_bits(buffer, current_bit, bw, (unsigned int)token->flag);  // index
            current_bit += bw;

            set_bits(buffer, current_bit, 2, (unsigned int)token->caseStyle);  // caseStyle
            current_bit += 2;
        } else {
            // Fixed 9-bit ASCII token:
            //   1 bit:  isPattern = 0
            //   8 bits: ASCII char value

            set_bits(buffer, current_bit, 1, 0U);         // isPattern = 0
            current_bit += 1;

            set_bits(buffer, current_bit, 8, (unsigned int)token->flag);   // char
            current_bit += 8;
        }
    }

    // Return the actual number of bits written, rounded up to bytes
    *outSize = (current_bit + 7) / 8;
    return buffer;
}

NLTokenArray* nl_en_decode(const unsigned char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        NLTokenArray* arr = (NLTokenArray*)malloc(sizeof(NLTokenArray));
        if (arr) {
            arr->count = 0;
        }
        return arr;
    }

    size_t total_bits = bufferSize * 8;

    // Read token count from first 13 bits (max 4096)
    size_t token_count = get_bits(buffer, 0, 13);

    // Validate: ensure count is within limits
    if (token_count > NL_EN_MAX_TOKENS) {
        token_count = NL_EN_MAX_TOKENS;
    }

    // Allocate output array
    NLTokenArray* arr = (NLTokenArray*)malloc(sizeof(NLTokenArray));
    if (!arr) {
        return NULL;
    }

    arr->count = token_count;

    if (token_count == 0) {
        return arr;
    }

    // Start reading tokens from bit 13
    size_t current_bit = 13;
    size_t read_count = 0;

    while (read_count < token_count) {
        // Guard: check if we have at least 1 bit to read
        if (current_bit >= total_bits) {
            arr->count = read_count;
            break;
        }

        NLToken* token = &arr->tokens[read_count];

        // Read isPattern bit
        unsigned char isPattern = get_bit(buffer, current_bit);
        current_bit += 1;

        if (isPattern) {
            // Read 4 bits: bit length of index
            if (current_bit + 4 > total_bits) {
                arr->count = read_count;
                break;
            }
            unsigned int bit_length = get_bits(buffer, current_bit, 4);
            current_bit += 4;

            // Read bit_length bits: index value
            if (bit_length == 0 || current_bit + bit_length > total_bits) {
                arr->count = read_count;
                break;
            }
            unsigned int flag = get_bits(buffer, current_bit, bit_length);
            current_bit += bit_length;

            // Read 2 bits: caseStyle
            if (current_bit + 2 > total_bits) {
                arr->count = read_count;
                break;
            }
            unsigned int caseStyle = get_bits(buffer, current_bit, 2);
            current_bit += 2;

            token->isPattern = true;
            token->flag = (unsigned short)flag;
            token->caseStyle = (int)caseStyle;
        } else {
            // Read 8 bits: ASCII char
            if (current_bit + 8 > total_bits) {
                arr->count = read_count;
                break;
            }
            unsigned int flag = get_bits(buffer, current_bit, 8);
            current_bit += 8;

            token->isPattern = false;
            token->flag = (unsigned short)flag;
            token->caseStyle = 0;
        }

        read_count++;
    }

    return arr;
}

/* ── Arithmetic-coding helpers ─────────────────────────────────────────── */

/* Renormalization boundaries for the 32-bit interval */
#define AE_TOP   0x80000000u
#define AE_QRTR  0x40000000u

/* Build cumulative probability bounds (scaled to NL_AE_SCALE) for a symbol
 * table whose .frequency fields have already been populated.                 */
static void ae_build_cum_bounds(AESymbol* syms, size_t n) {
    uint32_t total = 0;
    for (size_t i = 0; i < n; i++) total += syms[i].frequency;

    uint32_t cum = 0;
    for (size_t i = 0; i < n; i++) {
        syms[i].cum_low  = (uint32_t)((uint64_t)cum                      * NL_AE_SCALE / total);
        syms[i].cum_high = (uint32_t)((uint64_t)(cum + syms[i].frequency) * NL_AE_SCALE / total);
        cum += syms[i].frequency;
    }
}

/* Find the index of the AESymbol matching tok (by isPattern + flag).
 * Returns -1 when not found.                                                 */
static int ae_find_symbol(const AESymbol* syms, size_t n, const NLToken* tok) {
    for (size_t i = 0; i < n; i++) {
        if (syms[i].token.isPattern == tok->isPattern &&
            syms[i].token.flag      == tok->flag) {
            return (int)i;
        }
    }
    return -1;
}

/* ── Renormalization helpers (bit-emission AE) ─────────────────────────── */

/* Write a single bit at *bp and advance the cursor */
static void ae_emit_bit(unsigned char* buf, size_t* bp, unsigned int bit) {
    set_bit(buf, *bp, (unsigned char)(bit & 1u));
    (*bp)++;
}

/* After narrowing [*lo, *hi], emit agreed MSBs and handle E3 underflow.
 * Maintains the invariant that hi - lo >= AE_QRTR - 1 on exit.             */
static void ae_renorm_enc(uint32_t* lo, uint32_t* hi, int* pend,
                           unsigned char* buf, size_t* bp) {
    while (1) {
        if (*hi < AE_TOP) {
            /* E1: both in lower half — emit 0, then pend 1s */
            ae_emit_bit(buf, bp, 0);
            for (int k = 0; k < *pend; k++) ae_emit_bit(buf, bp, 1);
            *pend = 0;
            *lo = *lo << 1;
            *hi = (*hi << 1) | 1u;
        } else if (*lo >= AE_TOP) {
            /* E2: both in upper half — emit 1, then pend 0s */
            ae_emit_bit(buf, bp, 1);
            for (int k = 0; k < *pend; k++) ae_emit_bit(buf, bp, 0);
            *pend = 0;
            *lo = (*lo - AE_TOP) << 1;
            *hi = (*hi - AE_TOP) << 1 | 1u;
        } else if (*lo >= AE_QRTR && *hi < (AE_TOP | AE_QRTR)) {
            /* E3: straddle [0.25, 0.75) — count pending, remove second bit */
            (*pend)++;
            *lo = (*lo - AE_QRTR) << 1;
            *hi = (*hi - AE_QRTR) << 1 | 1u;
        } else {
            break;
        }
    }
}

/* Flush remaining bits so the decoder can uniquely identify the interval */
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

/* Safe bit read: returns 0 (padding) when past end of stream */
static unsigned int ae_read_bit_safe(const unsigned char* buf, size_t pos, size_t total) {
    return (pos < total) ? (unsigned int)get_bit(buf, pos) : 0u;
}

/* Mirror ae_renorm_enc for the decoder: widen interval and read new bits */
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

/* ── nl_en_encode_ae ───────────────────────────────────────────────────── */

unsigned char* nl_en_encode_ae(const NLTokenArray* arr, size_t count, size_t* outSize) {
    if (!arr || count == 0 || count > arr->count) {
        *outSize = 0;
        return NULL;
    }

    /* Build a temporary NLTokenArray limited to 'count' tokens so that
     * collectNLFrequencies() only sees the tokens we are about to encode.   */
    NLTokenArray* view = (NLTokenArray*)malloc(sizeof(NLTokenArray));
    if (!view) { *outSize = 0; return NULL; }
    view->count = count;
    for (size_t i = 0; i < count; i++) view->tokens[i] = arr->tokens[i];

    NLFreqMap* fmap = collectNLFrequencies(view);
    free(view);
    if (!fmap) { *outSize = 0; return NULL; }

    size_t unique = fmap->uniqueCount;

    AESymbol* syms = (AESymbol*)malloc(unique * sizeof(AESymbol));
    if (!syms) { freeNLFreqMap(fmap); *outSize = 0; return NULL; }

    for (size_t i = 0; i < unique; i++) {
        syms[i].token     = fmap->entries[i].token;
        syms[i].frequency = (uint32_t)fmap->entries[i].frequency;
    }
    freeNLFreqMap(fmap);

    ae_build_cum_bounds(syms, unique);

    if (syms[unique - 1].cum_high != NL_AE_SCALE) {
        fprintf(stderr, "[AE] Probability verification failed: expected %u, got %u\n",
                NL_AE_SCALE, syms[unique - 1].cum_high);
        free(syms);
        *outSize = 0;
        return NULL;
    }

    /* Allocate output buffer.
     * Header:    13 + 10 + unique*22 bits (worst case per symbol)
     * AE stream: count*32 + 64 bits (conservative: ~log2(unique) bits/token
     *            in practice, but 32 is a safe upper bound per token)       */
    size_t header_bits = 13 + 10 + unique * 22;
    size_t ae_bits     = count * 32 + 64;
    size_t bytes_needed = (header_bits + ae_bits + 7) / 8;
    unsigned char* buffer = (unsigned char*)calloc(bytes_needed, 1);
    if (!buffer) { free(syms); *outSize = 0; return NULL; }

    size_t bit_pos = 0;

    /* 13-bit: NLTokenArray count */
    set_bits(buffer, bit_pos, 13, (unsigned int)count);
    bit_pos += 13;

    /* 10-bit: unique token count */
    set_bits(buffer, bit_pos, 10, (unsigned int)unique);
    bit_pos += 10;

    /* Per-symbol data */
    for (size_t i = 0; i < unique; i++) {
        if (syms[i].token.isPattern) {
            /* isPattern(1) + flag(9) + caseStyle(2) + freq(10) = 22 bits */
            set_bits(buffer, bit_pos,  1, 1U);
            bit_pos += 1;
            set_bits(buffer, bit_pos,  9, (unsigned int)syms[i].token.flag);
            bit_pos += 9;
            set_bits(buffer, bit_pos,  2, (unsigned int)syms[i].token.caseStyle);
            bit_pos += 2;
            set_bits(buffer, bit_pos, 10, (unsigned int)syms[i].frequency);
            bit_pos += 10;
        } else {
            /* isPattern(1) + ascii_offset(7) + freq(10) = 18 bits */
            unsigned int offset = (syms[i].token.flag >= 32u) ?
                                  (unsigned int)(syms[i].token.flag - 32u) : 0u;
            if (offset > 94u) offset = 94u;
            set_bits(buffer, bit_pos,  1, 0U);
            bit_pos += 1;
            set_bits(buffer, bit_pos,  7, offset);
            bit_pos += 7;
            set_bits(buffer, bit_pos, 10, (unsigned int)syms[i].frequency);
            bit_pos += 10;
        }
    }

    /* AE encode with renormalization — replaces the previous fixed 64-bit tag */
    uint32_t lo = 0, hi = 0xFFFFFFFFu;
    int pend = 0;

    for (size_t i = 0; i < count; i++) {
        int idx = ae_find_symbol(syms, unique, &arr->tokens[i]);
        if (idx < 0) { free(syms); free(buffer); *outSize = 0; return NULL; }

        uint64_t range = (uint64_t)(hi - lo) + 1;
        hi = lo + (uint32_t)(range * syms[idx].cum_high / NL_AE_SCALE) - 1;
        lo = lo + (uint32_t)(range * syms[idx].cum_low  / NL_AE_SCALE);

        ae_renorm_enc(&lo, &hi, &pend, buffer, &bit_pos);
    }

    ae_flush_enc(lo, pend, buffer, &bit_pos);

    *outSize = (bit_pos + 7) / 8;
    free(syms);
    return buffer;
}

/* ── nl_en_decode_ae ───────────────────────────────────────────────────── */

NLTokenArray* nl_en_decode_ae(const unsigned char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) {
        NLTokenArray* arr = (NLTokenArray*)malloc(sizeof(NLTokenArray));
        if (arr) arr->count = 0;
        return arr;
    }

    size_t total_bits = bufferSize * 8;
    size_t bit_pos    = 0;

    /* Read 13-bit NLTokenArray count */
    if (bit_pos + 13 > total_bits) return NULL;
    size_t count = get_bits(buffer, bit_pos, 13);
    bit_pos += 13;

    /* Read 10-bit unique token count */
    if (bit_pos + 10 > total_bits) return NULL;
    size_t unique = get_bits(buffer, bit_pos, 10);
    bit_pos += 10;

    if (unique == 0 || count == 0) {
        NLTokenArray* arr = (NLTokenArray*)malloc(sizeof(NLTokenArray));
        if (arr) arr->count = 0;
        return arr;
    }

    /* Rebuild AESymbol table from the stored per-symbol data */
    AESymbol* syms = (AESymbol*)malloc(unique * sizeof(AESymbol));
    if (!syms) return NULL;

    for (size_t i = 0; i < unique; i++) {
        if (bit_pos >= total_bits) { free(syms); return NULL; }

        unsigned int isPattern = get_bit(buffer, bit_pos);
        bit_pos += 1;

        if (isPattern) {
            if (bit_pos + 21 > total_bits) { free(syms); return NULL; }
            unsigned int flag      = get_bits(buffer, bit_pos, 9);  bit_pos += 9;
            unsigned int caseStyle = get_bits(buffer, bit_pos, 2);  bit_pos += 2;
            unsigned int freq      = get_bits(buffer, bit_pos, 10); bit_pos += 10;
            syms[i].token.isPattern = true;
            syms[i].token.flag      = (unsigned short)flag;
            syms[i].token.caseStyle = (int)caseStyle;
            syms[i].frequency       = freq;
        } else {
            if (bit_pos + 17 > total_bits) { free(syms); return NULL; }
            unsigned int offset = get_bits(buffer, bit_pos, 7);  bit_pos += 7;
            unsigned int freq   = get_bits(buffer, bit_pos, 10); bit_pos += 10;
            syms[i].token.isPattern = false;
            syms[i].token.flag      = (unsigned short)(offset + 32u);
            syms[i].token.caseStyle = 0;
            syms[i].frequency       = freq;
        }
    }

    ae_build_cum_bounds(syms, unique);

    /* Initialize code register: read first 32 bits MSB-first from AE stream */
    uint32_t code = 0;
    for (int i = 31; i >= 0; i--)
        code |= (uint32_t)ae_read_bit_safe(buffer, bit_pos++, total_bits) << i;

    NLTokenArray* arr = (NLTokenArray*)malloc(sizeof(NLTokenArray));
    if (!arr) { free(syms); return NULL; }
    arr->count = 0;

    uint32_t lo = 0, hi = 0xFFFFFFFFu;

    for (size_t i = 0; i < count && arr->count < NL_EN_MAX_TOKENS; i++) {
        uint64_t range  = (uint64_t)(hi - lo) + 1;
        uint32_t scaled = (uint32_t)(((uint64_t)(code - lo + 1) * NL_AE_SCALE - 1) / range);

        int sym_idx = -1;
        for (size_t j = 0; j < unique; j++) {
            if (scaled >= syms[j].cum_low && scaled < syms[j].cum_high) {
                sym_idx = (int)j;
                break;
            }
        }

        if (sym_idx < 0) {
            fprintf(stderr, "[AE] Decode error at step %zu: scaled %u not in any range\n",
                    i, scaled);
            break;
        }

        arr->tokens[arr->count++] = syms[sym_idx].token;

        hi = lo + (uint32_t)(range * syms[sym_idx].cum_high / NL_AE_SCALE) - 1;
        lo = lo + (uint32_t)(range * syms[sym_idx].cum_low  / NL_AE_SCALE);

        ae_renorm_dec(&lo, &hi, &code, buffer, &bit_pos, total_bits);
    }

    free(syms);
    return arr;
}
