#ifndef NL_EN_CODEC_H
#define NL_EN_CODEC_H

#include "nl-en-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/**
 * Encodes an NLTokenArray into a byte buffer.
 * 
 * Bit layout:
 * - Bits 0-12: Array count (13 bits for NL_EN_MAX_TOKENS=4096)
 * - For each NLToken:
 *   - If isPattern=true: 11 bits (1 + 8 flag bits + 2 caseStyle bits)
 *   - If isPattern=false: 9 bits (0 + 8 flag bits)
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

#endif // NL_EN_CODEC_H
