#include "nl-en-codec.h"
#include <string.h>
#include <stdio.h>

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
