#ifndef NL_EN_CODEC_H
#define NL_EN_CODEC_H

#include "nl-en-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/* ── Arithmetic-coding constants ────────────────────────────────────────── */
#define NL_AE_SCALE  65536u   /* 2^16 — fixed-point probability denominator */

/**
 * One entry in the arithmetic-coding symbol table.
 *
 * Holds the NLToken it represents, its raw frequency, and the cumulative
 * probability interval [cum_low, cum_high) scaled to NL_AE_SCALE (65536).
 */
typedef struct {
    NLToken  token;     /* the NLToken this symbol represents */
    uint32_t frequency; /* raw frequency count */
    uint32_t cum_low;   /* cumulative lower bound (scaled to NL_AE_SCALE) */
    uint32_t cum_high;  /* cumulative upper bound (scaled to NL_AE_SCALE) */
} AESymbol;

/**
 * Encodes an NLTokenArray into a byte buffer.
 * 
 * Bit layout:
 * - Bits 0-12: Array count (13 bits for NL_EN_MAX_TOKENS=4096)
 * - For each NLToken:
 *   - If isPattern=true:  variable width 8-16 bits:
 *       1 bit  isPattern=1
 *       4 bits bit-length N of the index value (1..9)
 *       N bits index value without leading zeros (1 bit min, 9 bits max for index 511)
 *       2 bits caseStyle
 *   - If isPattern=false: 9 bits (0 + 8 ASCII char bits)
 *
 * @param arr The NLTokenArray to encode
 * @param count The number of tokens to encode (must be <= arr->count)
 * @param outSize Output parameter: size of returned buffer in bytes
 * @return A dynamically allocated byte buffer, caller must free it
 */
unsigned char* nl_en_encode(const NLTokenArray* arr, size_t count, size_t* outSize);

/**
 * Decodes a byte buffer back into an NLTokenArray.
 * 
 * @param buffer The byte buffer to decode
 * @param bufferSize The size of the buffer in bytes
 * @return A dynamically allocated NLTokenArray, caller must free it with freeNLTokenArray
 */
NLTokenArray* nl_en_decode(const unsigned char* buffer, size_t bufferSize);

/**
 * Encodes an NLTokenArray using arithmetic encoding.
 *
 * Bit layout:
 *   13 bits : NLTokenArray count
 *   10 bits : number of unique NLTokens in the frequency table
 *   Per unique token (isPattern=true):  1+9+2+10 = 22 bits
 *   Per unique token (isPattern=false): 1+7+10   = 18 bits
 *   Variable : renormalized AE bitstream (E1/E2/E3 bit-emission)
 *              length is proportional to entropy of the token sequence
 *
 * @param arr     The NLTokenArray to encode
 * @param count   Number of tokens to encode (must be <= arr->count)
 * @param outSize Output: size of returned buffer in bytes
 * @return Dynamically allocated byte buffer; caller must free it
 */
unsigned char* nl_en_encode_ae(const NLTokenArray* arr, size_t count, size_t* outSize);

/**
 * Decodes a byte buffer produced by nl_en_encode_ae back into an NLTokenArray.
 *
 * @param buffer     The byte buffer to decode
 * @param bufferSize Size of the buffer in bytes
 * @return Dynamically allocated NLTokenArray; caller must free with freeNLTokenArray
 */
NLTokenArray* nl_en_decode_ae(const unsigned char* buffer, size_t bufferSize);

#endif // NL_EN_CODEC_H
