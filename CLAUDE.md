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

- **`htmlcodec-c-lib`** — static library: `html-tokenizer.c`, `css-tokenizer.c`, `css-codec.c`, `nl-en-tokenizer.c`, `nl-en-opt.c`, `nl-en-codec.c`, `cl-javascript-en-tokenizer.c`, `cl-javascript-codec.c`, `nl-en-us-hyphenator.c`
- **`htmlcodec-c`** — main executable stub (currently a no-op)
- **`htmlcodec-c-test`** — test runner (`test-runner.c` + `tokenizer-test.c`)

### Tokenizer/Codec Modules

Each module is a self-contained `.h`/`.c` pair:

| Module | Purpose |
|--------|---------|
| `html-tokenizer` | State-machine HTML parser; produces `HTMLTokenArray` with text/openTag/closeTag tokens |
| `css-tokenizer` | CSS parser for selectors, properties, at-rules, and comments. Contains a 1208-entry pattern codebook (`CSS_PATTERNS[]`) across 11 segments: HTML type selectors (0–137), HTML attribute names (138–264), pseudo-classes (265–324), pseudo-elements (325–346), CSS properties (347–678), at-rules (679–697), combinators/symbols (698–707), reserved keyword values (708–923), value functions (924–1029), named colors (1030–1177), named at-rule blocks (1178–1207). `CSSTokenizable` (`isPattern` bool + `unsigned short flag`) represents one matched codebook entry or one raw ASCII character. `CSSProperty` carries `nameTokens[1024]`/`valueTokens[1024]` + sizes for whole-string tokenization of each property name and value. Both the `rule` union member (`selectorTokens[1024]`/`selectorTokenSize`) and the `atRule` union member (`atRuleTokens[1024]`/`atRuleTokenSize`) are populated by the same greedy longest-match left-to-right scan. The `comment` union member carries `commentTokens[1024]`/`commentTokenSize` where each character of the comment text is stored as a raw-ASCII `CSSTokenizable`. The `rule` union member also carries `ruleTokens[1024]`/`ruleTokenSize` — a flat concatenation produced by `css_flatten_rule_tokens()`: selectorTokens + `{` + (for each property: nameTokens + `:` + valueTokens + `;`) + `}`, using ASCII `CSSTokenizable` sentinels at each syntactic boundary. `collectCSSFrequencies()` takes a completed `CSSTokenArray` and returns a heap-allocated `CSSFreqMap` (up to `CSS_MAX_UNIQUE_TOKENIZABLE`=1464 entries) containing one `CSSFreqEntry` per unique `CSSTokenizable`, sorted descending by frequency. `CSS_MAX_TOKENIZABLE`=1024, `CSS_MAX_PROPERTIES`=16, `CSS_MAX_TOKENS`=256, `CSS_MAX_UNIQUE_TOKENIZABLE`=1464. |
| `css-codec` | Arithmetic-encoding codec for `CSSTokenArray`. `css_encode_ae()` builds a frequency map via `collectCSSFrequencies()`, constructs a fixed-point cumulative probability table (`CSSAESymbol`, scaled to `CSS_AE_SCALE`=65536), then encodes the flat `CSSTokenizable` sequence using renormalized arithmetic coding with E1/E2/E3 bit-emission (WNC-style). Output format: 10-bit total-tokenizable count + 11-bit unique count + per-symbol (22 bits if isPattern, 18 bits if ASCII) + 9-bit CSSToken count + per-token (2-bit type + 10-bit tokenizable size) + variable-length AE bitstream. `css_decode_ae()` reconstructs the frequency table, performs renormalized arithmetic decoding to recover the flat `CSSTokenizable` sequence, then partitions it back into `CSSToken` structs using the per-token sizes and the `{`/`:`/`;`/`}` ASCII sentinels embedded by the flattening step. Lossless for arbitrary-length sequences (no fixed precision limit). Also provides an optimised adaptive codec: `css_encode_opt()` / `css_decode_opt()` implement three-step adaptive AE — Step 1: vocab-only header (no per-symbol frequencies); Step 2: order-1 context model (`count[ctx][sym]`, Laplace-initialised, updated online); Step 3: CSS structural bigram seeding (pre-warms count table with CSS segment membership rules). See Encoding Format section below. |
| `nl-en-tokenizer` | English/Dutch text tokenizer using a 512-entry pattern dictionary structured as: CV×48, CVC×48, CCV×48, CVCC×48, VC×48, VCC×48, CCC×16, prefixes×48, suffixes×128 (48 base + 80 fill), non-syllable trigraphs×16, digraphs×16. `NLToken.flag` is `unsigned short` (holds indices 0–511). Tokenizes right-to-left (suffix-first) then reverses the token list. `initialize_patterns()` builds `NL_EN_PATTERNS` using a round-robin merge across the 12 sections: each round picks one element per section (by rank within section), sorts the batch by ascending character length, then appends — placing the most-frequent patterns at lower indices for better variable-width compression. `collectNLFrequencies()` takes a completed `NLTokenArray` and returns a heap-allocated `NLFreqMap` containing one `NLFreqEntry` (NLToken copy + frequency count) per unique token (identified by isPattern+flag), sorted descending by frequency; `uniqueCount` ≤ `totalTokens` ≤ `NL_EN_MAX_TOKENS`. |
| `nl-en-opt` | Extended word-level dictionary and optimised tokenizer, compiled as a separate translation unit (`nl-en-opt.c`). Contains `NL_EN_WORD_PATTERNS[232]` — 232 common English words (4–8 chars) ranked by expected frequency. Word tokens carry flags in `[NL_EN_PATTERN_COUNT, NL_EN_OPT_PATTERN_COUNT)` = `[512, 744)`. `tokenizeEnglishOpt()` performs true longest-match right-to-left over both the 512-entry syllable dictionary and the 232-entry word dictionary; a longer match always beats a shorter one at the same position. Calls `tokenizeEnglish("")` internally on first use to trigger `initialize_patterns()` and populate `NL_EN_PATTERNS`. `NL_EN_WORD_COUNT`=232, `NL_EN_OPT_PATTERN_COUNT`=744. |
| `nl-en-codec` | Bit-level encoder/decoder for `NLTokenArray`; pattern tokens use variable width 8–16 bits (1 isPattern + 4 bitLength + N index bits + 2 caseStyle), ASCII tokens use 9 bits (1+8). `outSize` reflects actual bits written (rounded up to bytes), not worst-case allocation. Also provides: (a) static-frequency AE codec (`nl_en_encode_ae` / `nl_en_decode_ae`): builds a fixed-point probability table (`AESymbol`, cum bounds scaled to `NL_AE_SCALE`=65536) from the token frequency map, serialised as 13-bit count + 10-bit unique count + 22/18 bits per unique token + variable-length AE bitstream; (b) optimised adaptive AE codec (`nl_en_encode_opt` / `nl_en_decode_opt`): see Encoding Format section below. All codecs are lossless for arbitrary-length sequences (no fixed precision limit). |
| `cl-javascript-en-tokenizer` | JavaScript tokenizer with a 502-entry pattern dictionary (`CL_JS_EN_PATTERN_COUNT`=502). Eight sections: ES2025 reserved keywords (0–63), common JS library/framework API tokens (64–159), English digraphs (160–223), non-alphanumeric JS digraphs (224–255), JS identifiers/built-ins (256–319), JS method call patterns (320–383), JS operator patterns (384–447), short verb/noun fragments (448–501). `CLJSToken.flag` is `unsigned short` (holds sorted indices 0–501). `tokenizeJavaScript()` performs greedy longest-match left-to-right; `initialize_patterns()` sorts all 502 raw patterns by descending length (longest first) so the first match is always the longest. caseStyle detection runs only for English digraphs (raw indices 160–223); all other patterns use `caseStyle=3` (no change needed). ASCII (non-pattern) tokens use `caseStyle=3`. |
| `cl-javascript-codec` | Adaptive order-1 AE codec for `CLJSTokenArray`. `cljs_encode_ae_opt()` / `cljs_decode_ae_opt()` — adaptive header (bitmap when vocab\_size ≥ 80, per-entry otherwise), Laplace-initialised `count[ctx][sym]` table updated online, order-1 context conditioning. caseStyle stored in a 2-bit-per-digraph-token side-channel before the AE stream. ASCII tokens store the full 8-bit byte value (not a printable offset) to correctly handle `\n`, `\t`, and other non-printable source characters. `CLJS_BITMAP_THRESHOLD`=80. See Encoding Format section below. |
| `nl-en-us-hyphenator` | Knuth-Liang syllable extractor for US English. Reads `ushyphmax.tex` (4938 patterns) at first call to build a trie; falls back to the embedded `KL_US_HYPHEN_PATTERNS` array if the file is not found. `tokenizeKnuthLiang()` splits input on non-alphanumeric boundaries (using `KL_ASCII_PATTERNS[96]`), lowercases each word, then applies affix stripping before KL hyphenation: `kl_strip_affixes()` attempts to find the longest matching suffix (min length 3, from `KL_EN_SUFFIXES[128]`) that leaves a stem ≥ 3 chars; if found, the longest matching prefix (from `KL_EN_PREFIXES[128]`) is stripped from the stem if at least 3 chars remain. The prefix token (if any), KL-hyphenated stem syllables, and suffix token are emitted in order, all with `isHyphenated=true` and affixes always with `caseStyle=0`. If no suffix matches, normal KL hyphenation runs. Stem syllable case style is derived from the original-cased text. Both `KLTokenArray` (max `KL_MAX_TOKENS`=4096 tokens) and `KLToken` (inline `text[KL_MAX_TOKEN_TEXT=64]`) use fixed-size arrays with no per-token heap allocation. The trie is cached as a module-level static after the first call. `collectKLFrequencies()` takes a completed `KLTokenArray` and returns a heap-allocated `KLFreqMap` containing one `KLStringFreq` entry per unique string (text + frequency count), sorted descending by frequency; `uniqueCount` ≤ `totalTokens` ≤ `KL_MAX_TOKENS`. |

### Subdata Enrichment

`HTMLToken` and `HTMLAttribute` carry a `subdataType` field (`CSS`, `JS`, or `NL`) and a union pointer. After `parseHTML()`, call `enrichHTMLTokenSubdata()` to parse inline CSS/JS/NL content within HTML tokens and attributes.

### Encoding Format (NL-EN Codec — variable-width)

Binary stream structure:
- Header: 13 bits for token count
- Per token:
  - Pattern match: `1` (1 bit) + bit-length N (4 bits) + index value (N bits, 1–9) + case style (2 bits) = 8–16 bits variable
  - Raw ASCII: `0` (1 bit) + printable offset (7 bits, `flag − 32`, range [32, 126]) = 8 bits fixed

The bit-length N is the number of significant bits in the pattern index (no leading zeros; minimum 1). Index 0 encodes as N=1, bit="0". Index 511 encodes as N=9, bits="111111111".

Case styles: `0`=all-lower, `1`=all-upper, `2`=first-upper, `3`=last-upper.

### Encoding Format (NL-EN Arithmetic Codec)

Bit stream structure produced by `nl_en_encode_ae`:
- 13 bits: NLTokenArray count
- 10 bits: unique token count (frequency table size)
- Per unique token (isPattern=true): 1 + 9 (flag/index) + 2 (caseStyle) + 10 (frequency) = 22 bits
- Per unique token (isPattern=false): 1 + 7 (flag−32, printable ASCII offset) + 10 (frequency) = 18 bits
- Variable: renormalized AE bitstream (E1/E2/E3 bit-emission, WNC-style)

The arithmetic encoder uses a `uint32_t` interval narrowed with cumulative probabilities scaled to 65536. After each narrowing, E1 (both bounds in lower half), E2 (both in upper half), and E3 (straddle [0.25, 0.75)) renormalization conditions are checked; matching bits are emitted and the interval is widened. The decoder mirrors this by reading new bits on each renormalization. Lossless for arbitrary-length sequences.

### Encoding Format (CSS Arithmetic Codec)

Bit stream structure produced by `css_encode_ae`:
- 10 bits: total `CSSTokenizable` count across all tokens in the array
- 11 bits: unique `CSSTokenizable` count (frequency table size; max 1464 = 1208 patterns + 256 ASCII)
- Per unique token (isPattern=true): 1 + 11 (flag/index, covers 0–1207) + 10 (frequency) = 22 bits
- Per unique token (isPattern=false): 1 + 7 (flag−32, offset into printable ASCII 32–127) + 10 (frequency) = 18 bits
- 9 bits: `CSSToken` count (≤ `CSS_MAX_TOKENS`=256)
- Per `CSSToken`: 2 (type) + 10 (tokenizable size for this token) = 12 bits
  - type 0 size = `ruleTokenSize`; type 1 = `atRuleTokenSize`; type 2 = `commentTokenSize`
- Variable: renormalized AE bitstream (E1/E2/E3 bit-emission, WNC-style)

The flat `CSSTokenizable` sequence encodes all tokens concatenated in order. Decoding partitions the recovered sequence back into `CSSToken` structs using the per-token sizes and the embedded ASCII sentinels (`{`, `:`, `;`, `}`). Lossless for arbitrary-length sequences.

### Encoding Format (NL-EN Optimised Codec — Steps 1–4)

Bit stream structure produced by `nl_en_encode_opt` / decoded by `nl_en_decode_opt`:

- 13 bits: token count
- 10 bits: vocab size (number of distinct token identities, first-appearance order)
- Per vocab entry (isPattern=true): 1 (isPattern flag) + 10 (flag, covers 0–743) = 11 bits
- Per vocab entry (isPattern=false): 1 (isPattern flag) + 7 (flag−32, printable ASCII offset) = 8 bits
- 13 bits: sc\_count (number of pattern tokens in the sequence = number of caseStyle values)
- sc\_count × 2 bits: caseStyle side-channel, one 2-bit value per pattern token in sequence order
- Variable: renormalized AE bitstream (E1/E2/E3 bit-emission, WNC-style)

The caseStyle side-channel is placed **before** the AE bitstream so the decoder reads it from a deterministic bit position; placing it after the AE stream is unreliable because the renormalized AE decoder primes a 32-bit code register that may over-read into that region.

**Four optimisation steps applied:**

- **Step 1 — Adaptive AE (no static frequency table):** The vocab header records only token identities (no per-symbol frequency). Probabilities are maintained as a `count[ctx][sym]` table initialised with Laplace counts (all 1) and updated online after each decoded symbol.
- **Step 2 — Decoupled caseStyle:** caseStyle is encoded in a 2-bit side-channel in the header rather than as part of the AE alphabet, reducing the alphabet size and eliminating case-induced probability fragmentation.
- **Step 3 — Extended word dictionary:** The 232-entry word dictionary (`nl-en-opt.c`) extends the symbol space to flags 0–743. Word tokens (flags 512–743) are produced by `tokenizeEnglishOpt()`, which prefers word-level matches over syllable matches when both cover the same span.
- **Step 4 — Order-1 context model:** The count table has `(vocab_size + 1)` rows × `vocab_size` columns. Row `vocab_size` is the start-of-sequence sentinel. After decoding each symbol, the context advances to that symbol's row, so each symbol is coded under the distribution of its immediate predecessor.

**Benchmark (1091-byte English prose passage, 552 tokens):**

| Codec | Output | Ratio |
|-------|--------|-------|
| NL-EN variable-width | ~965 B | ~88% of input |
| NL-EN static AE | ~936 B | ~86% of input |
| NL-EN optimised AE (Steps 1–4) | ~826 B | ~76% of input |
| zlib | ~658 B | ~60% of input |

The optimised codec closes roughly half the gap between static AE and zlib on typical English prose. The remaining gap to zlib is due to per-character syllable/word boundary overhead; further gains would require whole-word tokenisation at a higher vocabulary granularity.

### Encoding Format (CSS Optimised Codec — Steps 1–3)

Bit stream structure produced by `css_encode_opt` / decoded by `css_decode_opt`:

- 13 bits: total `CSSTokenizable` count (wider than static AE's 10-bit field to avoid overflow)
- 11 bits: vocab size (number of distinct token identities, first-appearance order)
- Per vocab entry (isPattern=true): 1 (isPattern flag) + 11 (flag, covers 0–1207) = 12 bits
- Per vocab entry (isPattern=false): 1 (isPattern flag) + 7 (flag−32, printable ASCII offset) = 8 bits
- 9 bits: `CSSToken` count (≤ `CSS_MAX_TOKENS`=256)
- Per `CSSToken`: 2 (type) + 10 (tokenizable size for this token) = 12 bits
- Variable: renormalized AE bitstream (E1/E2/E3 bit-emission, WNC-style)

The flat `CSSTokenizable` sequence is encoded and decoded with an order-1 adaptive model. No frequency data appears in the header; all probability mass is derived online. The token-metadata block (token count + per-token sizes) mirrors the static AE format and uses the same sentinel-parsing logic for reconstruction.

**Three optimisation steps applied:**

- **Step 1 — Adaptive AE (vocab-only header):** The header records only token identities (no per-symbol frequency). Probabilities are maintained as a `count[ctx][sym]` table initialised with Laplace counts (all 1) and updated online after each encoded/decoded symbol.
- **Step 2 — Order-1 context model:** The count table has `(vocab_size + 1)` rows × `vocab_size` columns. Row `vocab_size` is the start-of-sequence sentinel. After each symbol is processed, the context advances to that symbol's vocab index.
- **Step 3 — CSS structural bigram seeding:** Before encoding begins, the count table is pre-warmed using CSS domain knowledge: after `{` → property-name tokens (seg 5, indices 347–678) get +20; after `:` → value tokens (seg 8–11, indices 708–1207) get +20; after `;` → property names get +20, `}` gets +10; after `}` → selectors (seg 1–4, indices 0–346) get +10; after a selector → `{` gets +15, other selectors +5; after a property name → `:` gets +50; after a value → `;` gets +20, other values +5; start-of-sequence → selectors +10.

### zlib vs CSS AE Benchmark

Two benchmark tests compare `css_encode_ae` output size against zlib applied to the raw CSS string:

| Case | CSS input | ruleTokenSize | uniqueCount | CSS AE | zlib |
|------|-----------|:---:|:---:|--------|------|
| Best  | `body { color: red; color: red; }` (32 B) | 11 | 7 | ~27 B | ~29 B |
| Worst | `a:hover { font-size: 2em; }` (27 B)       | 10 | 10 | ~34 B | ~35 B |
| Long  | realistic small-webpage stylesheet (1382 B) | 528 across 25 tokens | ~70 | ~632 B | ~563 B |

Best case uses heavy token repetition (7 unique / 11 total); renormalized AE slightly beats zlib due to the probability model amortising the frequency table overhead. Worst case has no repetition (10 unique / 10 total); both methods expand the tiny input slightly. The long stylesheet is a lossless round-trip with renormalization: the 528-tokenizable sequence is fully encoded, with output dominated by ~70 unique symbols × 20 bits header + the full AE bitstream (~403 bytes of payload at ~6.1 bits/token entropy). CSS AE is larger than zlib on this input because the per-symbol frequency table is a fixed overhead that only amortises at higher repetition ratios.

### zlib vs CSS Optimised AE Benchmark

Three benchmark tests compare `css_encode_opt` output size against `css_encode_ae` and zlib:

| Case | Raw input | CSS static AE | CSS opt AE | zlib | Opt reduction |
|------|-----------|:---:|:---:|:---:|:---:|
| Best  | `body { color: red; color: red; }` (32 B) | ~27 B | ~16 B | ~29 B | ~50% |
| Worst | `a:hover { font-size: 2em; }` (27 B)       | ~34 B | ~20 B | ~35 B | ~26% |
| Long  | realistic small-webpage stylesheet (1382 B) | ~632 B | ~480 B | ~563 B | ~65% |

Best case (heavy repetition): the adaptive model converges quickly and delivers a 50% reduction vs 10% for static AE. Worst case (no repetition): both codecs expand the tiny input; the opt codec is smaller because its header omits per-symbol frequencies. Long stylesheet: the opt codec achieves 65.3% reduction, outperforming both static AE (54.3%) and zlib (59.3%) — structural seeding gives the context model accurate initial priors before any adaptation has occurred, yielding better compression than zlib for CSS-structured token sequences.

### Encoding Format (CLJS AE Opt Codec)

Bit stream structure produced by `cljs_encode_ae_opt` / decoded by `cljs_decode_ae_opt`:

- 13 bits: token count (max `CL_JS_EN_MAX_TOKENS`=8192)
- 1 bit: `use_bitmap` flag (= 1 when vocab\_size ≥ `CLJS_BITMAP_THRESHOLD`=80, 0 otherwise)
- **if `use_bitmap`=1 (large vocab — bitmap header):**
  - 502 bits: pattern-presence bitmap (bit `i` = sorted pattern index `i` appears in sequence)
  - 256 bits: ASCII-presence bitmap (bit `i` = byte value `i` appears in sequence)
  - vocab reconstructed in sorted order: patterns 0–501 ascending, then ASCII 0–255
- **if `use_bitmap`=0 (small vocab — per-entry header):**
  - 10 bits: vocab\_size
  - Per vocab entry (isPattern=true): 1 + 9 (sorted flag 0–501) = 10 bits
  - Per vocab entry (isPattern=false): 1 + 8 (full byte 0–255) = 9 bits
  - vocab in sorted-flag order (patterns ascending then ASCII ascending)
- 13 bits: sc\_count (number of DIGRAPH pattern tokens with non-trivial caseStyle)
- sc\_count × 2 bits: caseStyle side-channel, one per digraph pattern token in sequence order
- Variable: renormalized adaptive order-1 AE bitstream (E1/E2/E3 bit-emission)

The bitmap header costs a fixed 758 bits (≈ 95 bytes) regardless of vocab size; the per-entry header costs ~9.7 bits × vocab\_size. The threshold of 80 entries is the break-even point. For large JS files (vocab ≈ 129), bitmap format saves ~60 bytes over per-entry encoding.

ASCII tokens use a full 8-bit flag (not a 7-bit printable offset) so that non-printable characters (`\n`, `\t`, etc.) common in JS source code round-trip correctly.

**Adaptive codec design:**
- **Vocab-only header:** No per-symbol frequency stored; all probability mass is derived online.
- **Order-1 context model:** `count[ctx][sym]` table, `(vocab_size+1)` rows × `vocab_size` columns, Laplace-initialised to 1. Row `vocab_size` is the start-of-sequence sentinel. Context advances to the last decoded symbol's vocab index after each symbol.
- **caseStyle side-channel:** Placed before the AE bitstream (at a deterministic bit position) so the decoder reads it correctly. Applied to DIGRAPH pattern tokens only (raw indices 160–223); all other pattern tokens always have `caseStyle=3`.
- **JS structural bigram seeding:** Count table pre-warmed with targeted single-cell seeds for common JS transitions: `{`→`\n`, `}`→`\n`, `;`→`\n`, `\n`→space, `,`→space, keyword→space, `this`→`.`, etc. No loop-based seeding to avoid row-total inflation.

### zlib vs CLJS AE Opt Benchmark

Two benchmark tests compare `cljs_encode_ae_opt` output size against zlib on real JavaScript inputs:

| Case | Raw input | CLJS AE opt | zlib | Codec reduction |
|------|-----------|:---:|:---:|:---:|
| Short | `function add(a, b) { return a + b; } ...` (60 B) | ~40 B | ~65 B | ~33% |
| Long  | production-like React module (1921 B, 910 tokens) | ~658 B | ~733 B | ~66% |

The short case already beats zlib below 100 bytes. The long case (1921 B, ≥ 1024 B target) achieves **65.7% reduction**, beating zlib's 61.8%. Best case (highly repetitive function calls) achieves **73.2% reduction**. The bitmap vocab header is the key enabler: for vocab\_size=129 it replaces ~1250 per-entry bits with a fixed 758-bit bitmap, saving ~62 bytes on the header alone.

### Test Structure

Tests live in `tokenizer-test.c` with declarations in `tokenizer-test.h`. Tests use simple assertion helpers (`assert_equal_int`, `assert_equal_str`, `assert_true`) and global `testsPassed`/`testsFailed` counters. Adding a new test requires declaring it in `tokenizer-test.h`, implementing it in `tokenizer-test.c`, and calling it from `test-runner.c`.
