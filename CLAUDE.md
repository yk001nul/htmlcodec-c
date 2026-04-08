# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test Commands

All source files are under `htmlcodec-c/`. Build using CMake presets (MSVC/Ninja on Windows):

```bash
# Configure
cmake --preset x64-debug -S htmlcodec-c -B htmlcodec-c/out/build/x64-debug

# Build
cmake --build htmlcodec-c/out/build/x64-debug

# Run tests via ctest
ctest -C Debug --test-dir htmlcodec-c/out/build/x64-debug

# Or run the test executable directly
./htmlcodec-c/out/build/x64-debug/htmlcodec-c-test.exe
```

Available presets: `x64-debug`, `x64-release`, `x86-debug`, `x86-release`.

## Architecture

This is a C17 compression library that tokenizes HTML, CSS, English/Dutch text, and JavaScript using pre-built pattern dictionaries, then encodes the result into compact binary formats.

### CMake Targets

- **`htmlcodec-c-lib`** — static library containing all tokenizers and codecs
- **`htmlcodec-c`** — main executable stub (currently a no-op)
- **`htmlcodec-c-test`** — test runner (`test-runner.c` + `tokenizer-test.c`)

### Tokenizer/Codec Modules

Each module is a self-contained `.h`/`.c` pair:

| Module | Purpose |
|--------|---------|
| `html-tokenizer` | State-machine HTML parser; produces `HTMLTokenArray` with text/openTag/closeTag tokens |
| `css-tokenizer` | CSS parser for selectors, properties, at-rules, and comments |
| `nl-en-tokenizer` | English/Dutch text tokenizer using a 512-entry pattern dictionary structured as: CV×48, CVC×48, CCV×48, CVCC×48, VC×48, VCC×48, CCC×16, prefixes×48, suffixes×128 (48 base + 80 fill), non-syllable trigraphs×16, digraphs×16. `NLToken.flag` is `unsigned short` (holds indices 0–511). Tokenizes right-to-left (suffix-first) then reverses the token list. `initialize_patterns()` builds `NL_EN_PATTERNS` using a round-robin merge across the 12 sections: each round picks one element per section (by rank within section), sorts the batch by ascending character length, then appends — placing the most-frequent patterns at lower indices for better variable-width compression. |
| `nl-en-codec` | Bit-level encoder/decoder for `NLTokenArray`; pattern tokens use variable width 8–16 bits (1 isPattern + 4 bitLength + N index bits + 2 caseStyle), ASCII tokens use 9 bits (1+8). `outSize` reflects actual bits written (rounded up to bytes), not worst-case allocation. |
| `cl-javascript-en-tokenizer` | JavaScript tokenizer using 256-entry pattern dictionary (ES2025 keywords, API tokens, operators, digraphs) |
| `nl-en-us-hyphenator` | Knuth-Liang syllable extractor for US English. Reads `ushyphmax.tex` (4938 patterns) at first call to build a trie; falls back to the embedded `KL_US_HYPHEN_PATTERNS` array if the file is not found. `tokenizeKnuthLiang()` splits input on non-alphanumeric boundaries (using `KL_ASCII_PATTERNS[96]`), lowercases each word, applies the algorithm (dotted string + weight accumulation), copies hyphen positions back to the original-cased buffer, then emits one `KLToken` per syllable (`isHyphenated=true`) or per unsplit word/delimiter (`isHyphenated=false`). `KLToken` stores `text`, `length`, `caseStyle` (0–3), and `isHyphenated`. The trie is cached as a module-level static after the first call. |

### Subdata Enrichment

`HTMLToken` and `HTMLAttribute` carry a `subdataType` field (`CSS`, `JS`, or `NL`) and a union pointer. After `parseHTML()`, call `enrichHTMLTokenSubdata()` to parse inline CSS/JS/NL content within HTML tokens and attributes.

### Encoding Format (NL-EN Codec)

Binary stream structure:
- Header: 13 bits for token count
- Per token:
  - Pattern match: `1` (1 bit) + bit-length N (4 bits) + index value (N bits, 1–9) + case style (2 bits) = 8–16 bits variable
  - Raw ASCII: `0` (1 bit) + char (8 bits) = 9 bits fixed

The bit-length N is the number of significant bits in the pattern index (no leading zeros; minimum 1). Index 0 encodes as N=1, bit="0". Index 511 encodes as N=9, bits="111111111".

Case styles: `0`=all-lower, `1`=all-upper, `2`=first-upper, `3`=last-upper.

### Test Structure

Tests live in `tokenizer-test.c` with declarations in `tokenizer-test.h`. Tests use simple assertion helpers (`assert_equal_int`, `assert_equal_str`, `assert_true`) and global `testsPassed`/`testsFailed` counters. Adding a new test requires declaring it in `tokenizer-test.h`, implementing it in `tokenizer-test.c`, and calling it from `test-runner.c`.
