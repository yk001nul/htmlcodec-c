#ifndef HTML_CODEC_H
#define HTML_CODEC_H

#include "html-tokenizer.h"
#include <stdlib.h>
#include <stdint.h>

/* Codebook segment boundaries */
extern const int HTML_CODEBOOK_TAG_COUNT;
extern const int HTML_CODEBOOK_ATTR_START;
extern const int HTML_CODEBOOK_ATTR_COUNT;
extern const int HTML_CODEBOOK_MIME_START;
extern const int HTML_CODEBOOK_MIME_COUNT;
extern const int HTML_CODEBOOK_SIZE;

/* Flat codebook array (filled after html_codebook_init()) */
extern const char* HTML_CODEBOOK[];

/* Initialize the codebook (idempotent; called automatically by encode/decode) */
void html_codebook_init(void);

/**
 * Encodes an HTMLTokenArray into a compact binary stream.
 *
 * Bitstream layout (Steps 1–3 optimisation):
 *   10 bits : HTMLToken count
 *   [Step 3 — attr-value dictionary]
 *     5 bits : dict_size (0–31)
 *     Per dict entry: 7 bits str_len + str_len×8 bits raw chars
 *   [Step 1 — global NL stream]
 *     10 bits : text_tok_count (total text tokens)
 *     text_tok_count × 10 bits : nl_token_count per text token (0 = whitespace/empty)
 *     13 bits : global_nl_bytes
 *     global_nl_bytes bytes : nl_en_encode_opt() payload for all text NL tokens
 *   Per token (2 bits type first):
 *     type == 0 (text):
 *       2 bits  : subdataType
 *       If subdataType > 0:
 *         12 bits : sub_bytes; sub_bytes bytes sub-codec payload
 *     type == 1 or 2 (tag):
 *       9 bits : tagFlag (0..TAG_COUNT-1 = codebook, CODEBOOK_SIZE = raw)
 *       If raw: 6 bits length + length×8 bits ASCII
 *       1 bit  : selfClosing
 *       5 bits : attrCount
 *       Per attribute:
 *         9 bits : attrNameFlag (ATTR_START..MIME_START-1 or CODEBOOK_SIZE)
 *         If raw: 6 bits length + length×8 bits ASCII
 *         2 bits : attrValueSubdataType (0=raw/dict, 1=CSS, 2=JS, 3=NL)
 *         If subdataType == 0 (NONE):
 *           1 bit dict_hit; if hit: 5 bits dict_index
 *                           if miss: 7 bits value_len + value_len×8 bits ASCII
 *         Else (CSS/JS/NL):
 *           7 bits sub_len + sub_len×8 bits sub-codec payload
 *
 * @param arr     HTMLTokenArray to encode
 * @param count   Number of tokens (must be <= arr->count)
 * @param outSize Output: byte size of returned buffer
 * @return Heap-allocated buffer; caller must free it
 */
unsigned char* html_encode_ae_opt(const HTMLTokenArray* arr, size_t count,
                                  size_t* outSize);

/**
 * Decodes a buffer produced by html_encode_ae_opt back into an HTMLTokenArray.
 *
 * @param buffer     Encoded byte buffer
 * @param bufferSize Size in bytes
 * @return Heap-allocated HTMLTokenArray; caller must free with freeHTMLTokenArray
 */
HTMLTokenArray* html_decode_ae_opt(const unsigned char* buffer, size_t bufferSize);

#endif /* HTML_CODEC_H */
