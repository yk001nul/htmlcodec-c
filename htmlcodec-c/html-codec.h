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
 * Top-level bitstream layout:
 *   10 bits : HTMLToken count
 *   Per token:
 *     2 bits : type (0=text, 1=openTag, 2=closeTag)
 *     If type == 0 (text):
 *       12 bits : byte count of nl_en_encode_opt(textTokenArray), 0 = absent
 *       variable : encoded textTokenArray bytes
 *       2 bits  : subdataType
 *       If subdataType > 0:
 *         12 bits : byte count of encoded subdata
 *         variable : encoded CSS/JS/NL bytes
 *     Else (type 1 or 2):
 *       9 bits : tagFlag (0..TAG_COUNT-1 = codebook, CODEBOOK_SIZE = raw)
 *       If tagFlag == CODEBOOK_SIZE:
 *         6 bits + ASCII bytes : raw tag name
 *       1 bit  : selfClosing
 *       5 bits : attrCount
 *       Per attribute:
 *         9 bits : attrNameFlag (ATTR_START..MIME_START-1 or CODEBOOK_SIZE)
 *         If raw: 6 bits + ASCII bytes
 *         2 bits : attrValueSubdataType (0=raw ASCII, 1=CSS, 2=JS, 3=NL)
 *         7 bits : byte count of encoded attr value (max 127)
 *         variable : encoded bytes
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
