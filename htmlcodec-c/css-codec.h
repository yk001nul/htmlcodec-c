#ifndef CSS_CODEC_H
#define CSS_CODEC_H

#include "css-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/* Fixed-point probability denominator (same as NL-EN codec) */
#define CSS_AE_SCALE  65536u

/* One entry in the arithmetic-coding symbol table */
typedef struct {
    CSSTokenizable token;
    uint32_t       frequency;
    uint32_t       cum_low;
    uint32_t       cum_high;
} CSSAESymbol;

/**
 * Encodes a CSSTokenArray using arithmetic encoding.
 *
 * Bit layout:
 *   10 bits : total CSSTokenizable count across all tokens
 *   11 bits : unique CSSTokenizable count (frequency table size)
 *   Per unique token (isPattern=true):  1+11+10 = 22 bits
 *   Per unique token (isPattern=false): 1+7+10  = 18 bits
 *    9 bits : CSSToken count
 *   Per CSSToken: 2 (type) + 10 (tokenizable size) = 12 bits
 *   Variable : renormalized AE bitstream (E1/E2/E3 bit-emission)
 *
 * @param arr     CSSTokenArray to encode
 * @param outSize Output: size of returned buffer in bytes
 * @return Heap-allocated byte buffer; caller must free it
 */
unsigned char* css_encode_ae(const CSSTokenArray* arr, size_t* outSize);

/**
 * Decodes a byte buffer produced by css_encode_ae back into a CSSTokenArray.
 *
 * @param buffer     Byte buffer to decode
 * @param bufferSize Size of buffer in bytes
 * @return Heap-allocated CSSTokenArray; caller must free with freeCSS
 */
CSSTokenArray* css_decode_ae(const unsigned char* buffer, size_t bufferSize);

/* ── Optimised CSS codec (Steps 1-3) ─────────────────────────────────────
 *  Step 1 – Adaptive AE: vocab-only header (no per-symbol frequencies)
 *  Step 2 – Order-1 context model: count[ctx][sym], Laplace init
 *  Step 3 – CSS structural bigram seeding: pre-warms the count table using
 *            CSS segment membership rules so the model predicts well even
 *            before sufficient adaptation tokens have been observed.
 * ──────────────────────────────────────────────────────────────────────── */

/* Vocabulary entry for the optimised codec (identity only, no frequency) */
typedef struct {
    bool           isPattern;
    unsigned short flag;
} CSSOptVocabEntry;

/**
 * Encodes a CSSTokenArray using the optimised adaptive order-1 codec.
 *
 * Bit layout:
 *   13 bits : total CSSTokenizable count across all tokens
 *   11 bits : vocab size (first-appearance order, no frequencies)
 *   Per vocab entry (isPattern=true):  1+11 = 12 bits
 *   Per vocab entry (isPattern=false): 1+7  =  8 bits
 *    9 bits : CSSToken count
 *   Per CSSToken: 2 (type) + 10 (tokenizable size) = 12 bits
 *   Variable : adaptive order-1 AE bitstream (E1/E2/E3 renormalization)
 *
 * @param arr     CSSTokenArray to encode
 * @param outSize Output: size of returned buffer in bytes
 * @return Heap-allocated byte buffer; caller must free it
 */
unsigned char* css_encode_opt(const CSSTokenArray* arr, size_t* outSize);

/**
 * Decodes a byte buffer produced by css_encode_opt back into a CSSTokenArray.
 *
 * @param buffer     Byte buffer to decode
 * @param bufferSize Size of buffer in bytes
 * @return Heap-allocated CSSTokenArray; caller must free it
 */
CSSTokenArray* css_decode_opt(const unsigned char* buffer, size_t bufferSize);

#endif /* CSS_CODEC_H */
