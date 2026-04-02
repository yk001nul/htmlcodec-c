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
 * Calculate the total buffer size needed for encoding
 */
static size_t calculate_buffer_size_bits(size_t token_count) {
    // 13 bits for count (max 4096 values)
    size_t total_bits = 13;
    
    // Add bits for each token - assume worst-case pattern token count for allocation
    total_bits += token_count * 11; // 11 bits per token (worst case)
    
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
            // Write: 1 bit isPattern + 8 bits flag + 2 bits caseStyle
            set_bits(buffer, current_bit, 1, 1U);  // isPattern = 1
            current_bit += 1;
            
            set_bits(buffer, current_bit, 8, (unsigned int)token->flag);
            current_bit += 8;
            
            set_bits(buffer, current_bit, 2, (unsigned int)token->caseStyle);
            current_bit += 2;
        } else {
            // Write: 1 bit isPattern + 8 bits flag
            set_bits(buffer, current_bit, 1, 0U);  // isPattern = 0
            current_bit += 1;
            
            set_bits(buffer, current_bit, 8, (unsigned int)token->flag);
            current_bit += 8;
        }
    }
    
    *outSize = total_bytes;
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
            // Not enough data, truncate the array
            arr->count = read_count;
            break;
        }
        
        NLToken* token = &arr->tokens[read_count];
        
        // Read isPattern bit
        unsigned char isPattern = get_bit(buffer, current_bit);
        current_bit += 1;
        
        // Guard: check if we have 8 bits for flag
        if (current_bit + 8 > total_bits) {
            arr->count = read_count;
            break;
        }
        
        unsigned char flag = get_bits(buffer, current_bit, 8);
        current_bit += 8;
        
        token->isPattern = isPattern ? true : false;
        token->flag = flag;
        
        if (isPattern) {
            // Read caseStyle (2 bits)
            // Guard: check if we have 2 bits for caseStyle
            if (current_bit + 2 > total_bits) {
                arr->count = read_count;
                break;
            }
            
            unsigned char caseStyle = get_bits(buffer, current_bit, 2);
            current_bit += 2;
            token->caseStyle = caseStyle;
        } else {
            // For non-pattern tokens, caseStyle is not used
            token->caseStyle = 0;
        }
        
        read_count++;
    }
    
    return arr;
}
