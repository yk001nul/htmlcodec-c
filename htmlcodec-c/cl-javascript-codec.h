#ifndef CL_JAVASCRIPT_CODEC_H
#define CL_JAVASCRIPT_CODEC_H

#include "cl-javascript-en-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/* Fixed-point probability denominator (matches NL-EN and CSS codecs) */
#define CL_JS_AE_SCALE  65536u

/**
 * Encodes a CLJSTokenArray using an adaptive order-1 arithmetic codec.
 *
 * Bit stream layout:
 *   13 bits : token count (max CL_JS_EN_MAX_TOKENS = 8192)
 *   10 bits : vocab size (number of distinct token identities, first-appearance order)
 *   Per vocab entry (isPattern=true):  1 (isPattern) + 9 (flag, covers 0–511) = 10 bits
 *   Per vocab entry (isPattern=false): 1 (isPattern) + 8 (flag, full byte 0–255)  =  9 bits
 *   13 bits : sc_count (number of pattern tokens = number of caseStyle values)
 *   sc_count × 2 bits: caseStyle side-channel, one per pattern token in sequence order
 *   Variable : renormalized adaptive order-1 AE bitstream (E1/E2/E3 bit-emission)
 *
 * The caseStyle side-channel is written before the AE stream so the decoder
 * reads it at a deterministic bit position.
 *
 * The adaptive model uses a count[ctx][sym] table (Laplace-initialised to 1)
 * updated online after each encoded symbol.  ctx = vocab_size is the
 * start-of-sequence sentinel; after each symbol ctx advances to that symbol's
 * vocab index (order-1 conditioning).
 *
 * @param arr     CLJSTokenArray to encode
 * @param count   Number of tokens to encode (must be <= arr->count)
 * @param outSize Output: size of returned buffer in bytes
 * @return Heap-allocated byte buffer; caller must free it
 */
unsigned char* cljs_encode_ae_opt(const CLJSTokenArray* arr, size_t count,
                                  size_t* outSize);

/**
 * Decodes a byte buffer produced by cljs_encode_ae_opt back into a CLJSTokenArray.
 *
 * @param buffer     Byte buffer to decode
 * @param bufferSize Size of buffer in bytes
 * @return Heap-allocated CLJSTokenArray; caller must free with freeCLJSTokenArray
 */
CLJSTokenArray* cljs_decode_ae_opt(const unsigned char* buffer, size_t bufferSize);

#endif /* CL_JAVASCRIPT_CODEC_H */
