# HTML/CSS/JavaScript/Natural Language Codec Suite - Quick Reference Guide

This guide covers the tokenizers and codecs for HTML, CSS, JavaScript, and English natural language text processing, including compression algorithms and hyphenation.

## Using the HTML Tokenizer

```c
#include "html-tokenizer.h"

// Parse HTML
HTMLTokenArray* result = parseHTML("<div id=\"test\">Hello World</div>");

// Access tokens
for (int i = 0; i < result->count; i++) {
    HTMLToken* token = &result->tokens[i];
    if (token->type == 0) {
        // Text token
        printf("Text: %s\n", token->data.text.content);
    } else if (token->type == 1) {
        // Opening tag
        printf("Tag: %s\n", token->data.tag.name);
    } else if (token->type == 2) {
        // Closing tag
        printf("End: %s\n", token->data.tag.name);
    }
}

// Cleanup
freeHTMLTokenArray(result);
```

## Using the CSS Tokenizer

```c
#include "css-tokenizer.h"

// Parse CSS
CSSTokenArray result;
const char* css = "body { color: red; background: white; }";
parseCSS(css, &result);

// Access tokens
for (int i = 0; i < result.count; i++) {
    CSSToken* token = &result.tokens[i];
    if (token->type == 0) {
        // Selector rule
        printf("Selector: %s\n", token->data.rule.selector);
        for (int j = 0; j < token->data.rule.propertyCount; j++) {
            printf("  %s: %s\n", 
                token->data.rule.properties[j].name,
                token->data.rule.properties[j].value);
        }
    } else if (token->type == 1) {
        // At-rule
        printf("At-rule: %s\n", token->data.atRule.rule);
    } else if (token->type == 2) {
        // Comment
        printf("Comment: %s\n", token->data.comment.content);
    }
}

// Cleanup
freeCSS(&result);
```

## Using the JavaScript Tokenizer

```c
#include "cl-javascript-en-tokenizer.h"

// Tokenize JavaScript
CLJSTokenArray* result = tokenizeJavaScript("function hello() { console.log('Hello World'); }");

// Access tokens
for (size_t i = 0; i < result->count; i++) {
    CLJSToken* token = &result->tokens[i];
    if (token->isPattern) {
        // Pattern token
        printf("Pattern %d: %s (style: %d)\n", 
               token->flag, 
               CL_JS_EN_PATTERNS[token->flag], 
               token->caseStyle);
    } else {
        // ASCII token
        printf("ASCII: %c\n", (char)token->flag);
    }
}

// Detokenize back to string
int length;
char* original = detokenizeCLJSTokenArray(result, &length);
printf("Reconstructed: %s\n", original);
free(original);

// Cleanup
freeCLJSTokenArray(result);
```

## Using the JavaScript Codec

```c
#include "cl-javascript-codec.h"

// Encode tokens
size_t encodedSize;
unsigned char* encoded = cljs_encode_ae_opt(result, result->count, &encodedSize);

// Decode back
CLJSTokenArray* decoded = cljs_decode_ae_opt(encoded, encodedSize);

// Cleanup
free(encoded);
freeCLJSTokenArray(decoded);
```

## Using the Natural Language (English) Tokenizer

```c
#include "nl-en-tokenizer.h"

// Tokenize English text
NLTokenArray* result = tokenizeEnglish("Hello world! This is a test.");

// Access tokens
for (size_t i = 0; i < result->count; i++) {
    NLToken* token = &result->tokens[i];
    if (token->isPattern) {
        // Pattern token
        printf("Pattern %d: %s (style: %d)\n", 
               token->flag, 
               NL_EN_PATTERNS[token->flag], 
               token->caseStyle);
    } else {
        // ASCII token
        printf("ASCII: %c\n", (char)(token->flag + 32));
    }
}

// Build frequency map
NLFreqMap* freqMap = collectNLFrequencies(result);
for (size_t i = 0; i < freqMap->uniqueCount; i++) {
    printf("Token freq: %d\n", freqMap->entries[i].frequency);
}

// Cleanup
freeNLFreqMap(freqMap);
freeNLTokenArray(result);
```

## Using the Natural Language Codec

```c
#include "nl-en-codec.h"

// Encode tokens
size_t encodedSize;
unsigned char* encoded = nl_en_encode(result, result->count, &encodedSize);

// Decode back
NLTokenArray* decoded = nl_en_decode(encoded, encodedSize);

// Cleanup
free(encoded);
freeNLTokenArray(decoded);
```

## Using the Knuth-Liang Hyphenator

```c
#include "nl-en-us-hyphenator.h"

// Hyphenate a word
const char* word = "hyphenation";
char* hyphenated = hyphenateWord(word);
printf("Hyphenated: %s\n", hyphenated);

// Cleanup
free(hyphenated);
```

## Key API Differences

### HTML Tokenizer
- Returns dynamically allocated `HTMLTokenArray*` (use `free()`)
- Token types: 0=text, 1=open-tag, 2=close-tag
- Handles attributes via `HTMLAttribute` array

### CSS Tokenizer
- Takes pre-allocated `CSSTokenArray*` parameter
- Token types: 0=selector-rule, 1=at-rule, 2=comment
- Parses CSS properties into `CSSProperty` arrays

### JavaScript Tokenizer
- Returns dynamically allocated `CLJSTokenArray*` (use `freeCLJSTokenArray()`)
- Tokens have `isPattern` (bool), `flag` (pattern index or ASCII), `caseStyle` (0-3)
- Supports detokenization back to original string

### Natural Language Tokenizer
- Returns dynamically allocated `NLTokenArray*` (use `freeNLTokenArray()`)
- Similar structure to JavaScript tokens
- Includes frequency map building for compression analysis

### Codecs
- All codecs use arithmetic encoding for compression
- Encode functions return heap-allocated byte buffers (caller must free)
- Decode functions return heap-allocated token arrays (caller must free)

## Constants

### HTML Tokenizer
- `HTML_MAX_TOKENS`: 1000
- `HTML_MAX_TAG_NAME`: 64
- `HTML_MAX_ATTR_COUNT`: 32
- `HTML_MAX_ATTR_NAME`: 32
- `HTML_MAX_ATTR_VALUE`: 256
- `HTML_MAX_TEXT_CONTENT`: 512

### CSS Tokenizer
- `CSS_MAX_TOKENS`: 1000
- `CSS_MAX_SELECTOR_LEN`: 256
- `CSS_MAX_PROPERTY_NAME`: 64
- `CSS_MAX_PROPERTY_VALUE`: 512
- `CSS_MAX_PROPERTIES`: 128

### JavaScript Tokenizer
- `CL_JS_EN_PATTERN_COUNT`: 502
- `CL_JS_EN_MAX_TOKENS`: 8192

### Natural Language Tokenizer
- `NL_EN_PATTERN_COUNT`: 512
- `NL_EN_MAX_TOKENS`: 4096

### Arithmetic Encoding
- `CL_JS_AE_SCALE`: 65536
- `NL_AE_SCALE`: 65536
- `CSS_AE_SCALE`: 65536

## Building the Project

This project uses CMake for building. To build the executables and libraries:

```bash
# Configure the project
cmake -S . -B build

# Build all targets
cmake --build build

# Or build specific targets
cmake --build build --target htmlcodec-c
cmake --build build --target htmlcodec-c-test
```

## Running Tests

The test suite can be executed by running the compiled executable:
```bash
htmlcodec-c.exe
```

Output will show:
- Pass/Fail status for each test
- Total passed and failed counts
- Organized by sections: HTML Tokenizer, English Tokenizer, JavaScript Tokenizer, CSS Tokenizer, HTML Integrated Tokenizer, NL-EN Codec, NL-EN Integration, Benchmarks, Hyphenator, Frequency Map, Arithmetic Encoding Codec, and CSS AE Codec
