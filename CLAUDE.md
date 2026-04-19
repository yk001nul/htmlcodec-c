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
| `css-tokenizer` | CSS parser for selectors, properties, at-rules, and comments. Contains a 1208-entry pattern codebook (`CSS_PATTERNS[]`) across 11 segments: HTML type selectors (0–137), HTML attribute names (138–264), pseudo-classes (265–324), pseudo-elements (325–346), CSS properties (347–678), at-rules (679–697), combinators/symbols (698–707), reserved keyword values (708–923), value functions (924–1029), named colors (1030–1177), named at-rule blocks (1178–1207). `CSSTokenizable` (`isPattern` bool + `unsigned short flag`) represents one matched codebook entry or one raw ASCII character. `CSSProperty` carries `nameTokens[1024]`/`valueTokens[1024]` + sizes for whole-string tokenization of each property name and value. Both the `rule` union member (`selectorTokens[1024]`/`selectorTokenSize`) and the `atRule` union member (`atRuleTokens[1024]`/`atRuleTokenSize`) are populated by the same greedy longest-match left-to-right scan: at each position all patterns are tried, the longest matching one becomes one `CSSTokenizable` (isPattern=true); if none match, the single character is stored as ASCII (isPattern=false). This allows compound selectors such as `"a:hover"` to split into `"a"` (SEG1 tag) + `":hover"` (SEG3 pseudo-class), and multi-selectors such as `"html, body"` to yield `"html"` (pattern) + `","` (ASCII) + `" "` (ASCII) + `"body"` (pattern). `CSS_MAX_TOKENIZABLE`=1024, `CSS_MAX_PROPERTIES`=16, `CSS_MAX_TOKENS`=256. |
| `nl-en-tokenizer` | English/Dutch text tokenizer using a 512-entry pattern dictionary structured as: CV×48, CVC×48, CCV×48, CVCC×48, VC×48, VCC×48, CCC×16, prefixes×48, suffixes×128 (48 base + 80 fill), non-syllable trigraphs×16, digraphs×16. `NLToken.flag` is `unsigned short` (holds indices 0–511). Tokenizes right-to-left (suffix-first) then reverses the token list. `initialize_patterns()` builds `NL_EN_PATTERNS` using a round-robin merge across the 12 sections: each round picks one element per section (by rank within section), sorts the batch by ascending character length, then appends — placing the most-frequent patterns at lower indices for better variable-width compression. `collectNLFrequencies()` takes a completed `NLTokenArray` and returns a heap-allocated `NLFreqMap` containing one `NLFreqEntry` (NLToken copy + frequency count) per unique token (identified by isPattern+flag), sorted descending by frequency; `uniqueCount` ≤ `totalTokens` ≤ `NL_EN_MAX_TOKENS`. |
| `nl-en-codec` | Bit-level encoder/decoder for `NLTokenArray`; pattern tokens use variable width 8–16 bits (1 isPattern + 4 bitLength + N index bits + 2 caseStyle), ASCII tokens use 9 bits (1+8). `outSize` reflects actual bits written (rounded up to bytes), not worst-case allocation. Also provides arithmetic-encoding codec (`nl_en_encode_ae` / `nl_en_decode_ae`): builds a fixed-point probability table (`AESymbol`, cum bounds scaled to `NL_AE_SCALE`=65536) from the token frequency map, encodes the sequence by iteratively narrowing a `uint32_t` interval [low,high], then serialises the frequency table and final interval as a bit stream (13-bit count + 10-bit unique count + 22/18 bits per unique token + 64-bit tag [low,high]). Decoder rebuilds the probability table from the stored data, then performs arithmetic decoding using the stored lower bound as the initial code value. Precision is sufficient for sequences up to ~20 tokens with small alphabets. |
| `cl-javascript-en-tokenizer` | JavaScript tokenizer using 256-entry pattern dictionary (ES2025 keywords, API tokens, operators, digraphs) |
| `nl-en-us-hyphenator` | Knuth-Liang syllable extractor for US English. Reads `ushyphmax.tex` (4938 patterns) at first call to build a trie; falls back to the embedded `KL_US_HYPHEN_PATTERNS` array if the file is not found. `tokenizeKnuthLiang()` splits input on non-alphanumeric boundaries (using `KL_ASCII_PATTERNS[96]`), lowercases each word, then applies affix stripping before KL hyphenation: `kl_strip_affixes()` attempts to find the longest matching suffix (min length 3, from `KL_EN_SUFFIXES[128]`) that leaves a stem ≥ 3 chars; if found, the longest matching prefix (from `KL_EN_PREFIXES[128]`) is stripped from the stem if at least 3 chars remain. The prefix token (if any), KL-hyphenated stem syllables, and suffix token are emitted in order, all with `isHyphenated=true` and affixes always with `caseStyle=0`. If no suffix matches, normal KL hyphenation runs. Stem syllable case style is derived from the original-cased text. Both `KLTokenArray` (max `KL_MAX_TOKENS`=4096 tokens) and `KLToken` (inline `text[KL_MAX_TOKEN_TEXT=64]`) use fixed-size arrays with no per-token heap allocation. The trie is cached as a module-level static after the first call. `collectKLFrequencies()` takes a completed `KLTokenArray` and returns a heap-allocated `KLFreqMap` containing one `KLStringFreq` entry per unique string (text + frequency count), sorted descending by frequency; `uniqueCount` ≤ `totalTokens` ≤ `KL_MAX_TOKENS`. |

### Subdata Enrichment

`HTMLToken` and `HTMLAttribute` carry a `subdataType` field (`CSS`, `JS`, or `NL`) and a union pointer. After `parseHTML()`, call `enrichHTMLTokenSubdata()` to parse inline CSS/JS/NL content within HTML tokens and attributes.

### Encoding Format (NL-EN Codec — variable-width)

Binary stream structure:
- Header: 13 bits for token count
- Per token:
  - Pattern match: `1` (1 bit) + bit-length N (4 bits) + index value (N bits, 1–9) + case style (2 bits) = 8–16 bits variable
  - Raw ASCII: `0` (1 bit) + char (8 bits) = 9 bits fixed

The bit-length N is the number of significant bits in the pattern index (no leading zeros; minimum 1). Index 0 encodes as N=1, bit="0". Index 511 encodes as N=9, bits="111111111".

Case styles: `0`=all-lower, `1`=all-upper, `2`=first-upper, `3`=last-upper.

### Encoding Format (NL-EN Arithmetic Codec)

Bit stream structure produced by `nl_en_encode_ae`:
- 13 bits: NLTokenArray count
- 10 bits: unique token count (frequency table size)
- Per unique token (isPattern=true): 1 + 9 (flag/index) + 2 (caseStyle) + 10 (frequency) = 22 bits
- Per unique token (isPattern=false): 1 + 7 (flag−32, printable ASCII offset) + 10 (frequency) = 18 bits
- 32 bits: arithmetic coding lower bound (sequence tag low)
- 32 bits: arithmetic coding upper bound (sequence tag high)

The arithmetic encoder iteratively narrows a `uint32_t` interval using cumulative probabilities scaled to 65536. The decoder reconstructs token order using the stored lower bound as the initial code value. No renormalization/bit-streaming is used, so practical precision supports sequences up to ~20 tokens for typical vocabularies.

### Test Structure

Tests live in `tokenizer-test.c` with declarations in `tokenizer-test.h`. Tests use simple assertion helpers (`assert_equal_int`, `assert_equal_str`, `assert_true`) and global `testsPassed`/`testsFailed` counters. Adding a new test requires declaring it in `tokenizer-test.h`, implementing it in `tokenizer-test.c`, and calling it from `test-runner.c`.
