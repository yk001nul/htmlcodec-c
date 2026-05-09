#ifndef CL_JAVASCRIPT_CODEC_H
#define CL_JAVASCRIPT_CODEC_H

#include "cl-javascript-en-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/* Fixed-point probability denominator (matches NL-EN and CSS codecs) */
#define CL_JS_AE_SCALE  65536u

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encodes a CLJSTokenArray using an adaptive order-1 arithmetic codec.
 *
 * Bit stream layout:
 *   13 bits : token count (max CL_JS_EN_MAX_TOKENS = 8192)
 *    1 bit  : use_bitmap (= 1 when vocab_size >= CLJS_BITMAP_THRESHOLD=80)
 *   if use_bitmap=1 (large vocab — bitmap header):
 *     502 bits : pattern-presence bitmap (bit i = sorted pattern index i present)
 *     256 bits : ASCII-presence bitmap   (bit i = byte value i present)
 *   if use_bitmap=0 (small vocab — per-entry header):
 *     10 bits : vocab size
 *     Per pattern entry : 1 (isPattern) + 9 (flag 0–501) = 10 bits
 *     Per ASCII entry   : 1 (isPattern) + 8 (flag 0–255) =  9 bits
 *   13 bits : sc_count (DIGRAPH pattern tokens with stored caseStyle)
 *   sc_count × 2 bits : caseStyle side-channel (digraph tokens only, in sequence order)
 *   Variable : renormalized adaptive order-1 AE bitstream (E1/E2/E3 bit-emission)
 *
 * @param arr     CLJSTokenArray to encode
 * @param count   Number of tokens to encode (must be <= arr->count)
 * @param outSize Output: size of returned buffer in bytes
 * @return Heap-allocated byte buffer; caller must free it
 */
HTMLCODEC_API unsigned char* cljs_encode_ae_opt(const CLJSTokenArray* arr, size_t count,
                                                size_t* outSize);

/**
 * Decodes a byte buffer produced by cljs_encode_ae_opt back into a CLJSTokenArray.
 *
 * @param buffer     Byte buffer to decode
 * @param bufferSize Size of buffer in bytes
 * @return Heap-allocated CLJSTokenArray; caller must free with freeCLJSTokenArray
 */
HTMLCODEC_API CLJSTokenArray* cljs_decode_ae_opt(const unsigned char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif /* CL_JAVASCRIPT_CODEC_H */
