#ifndef CSS_CODEC_H
#define CSS_CODEC_H

#include "css-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/* Fixed-point scale for arithmetic coding (same as NL-EN codec) */
#define CSS_AE_SCALE    65536u
#define CSS_AE_MAX_CODE 0xFFFFFFFFu

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
 *   32 bits : sequence tag lower bound
 *   32 bits : sequence tag upper bound
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

#endif /* CSS_CODEC_H */
