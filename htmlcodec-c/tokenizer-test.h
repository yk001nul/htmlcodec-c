#include <stdio.h>
#ifndef TOKENIZER_TEST_H
#define TOKENIZER_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "html-tokenizer.h"
#include "css-tokenizer.h"
#include "cl-javascript-codec.h"
#include "cmdline.h"

extern int testsPassed;
extern int testsFailed;

void assert_equal_int(int actual, int expected, const char* message);

void assert_equal_str(const char* actual, const char* expected, const char* message);

void assert_true(int condition, const char* message);

// Plain text tests
void test_plain_text_simple();
void test_plain_text_empty();
void test_plain_text_special_chars();

// Opening tag tests
void test_opening_tag_simple();
void test_opening_tag_with_attributes();

// Self-closing tag tests
void test_self_closing_tag_simple();
void test_self_closing_tag_with_attributes();

// Closing tag tests
void test_closing_tag();

// Comment tests
void test_comment_simple();

// Mixed content tests
void test_mixed_text_and_tags();

// Case conversion test
void test_uppercase_tags();

// Whitespace test
void test_whitespace_preservation();

// Large content test
void test_large_content();

// Nested tags test
void test_nested_tags();

// Worst case - unclosed tag
void test_unclosed_tag();

// CSS Tokenizer Tests - Best case scenarios
void test_css_simple_rule();
void test_css_multiple_selectors();
void test_css_multiple_properties();

// English tokenizer tests
void test_nl_en_tokenizer_best_case();
void test_nl_en_tokenizer_worst_case();

// JavaScript tokenizer tests
void test_cl_js_tokenizer_best_case();
void test_cl_js_tokenizer_worst_case();

// HTML integrated tokenizer tests
void test_html_integrated_tokenizer_token_content();
void test_html_integrated_tokenizer_attr_content();
void test_html_integrated_tokenizer_both_content();
void test_html_integrated_tokenizer_real_file();

// CSS Tokenizer Tests - Worse case scenarios
void test_css_comment_handling();
void test_css_at_rules();
void test_css_complex_selectors();
void test_css_empty_rules();
void test_css_malformed_input();

// NL-EN Codec tests
void test_nl_en_codec_best_case();
void test_nl_en_codec_worst_case();

// NL-EN Integration tests (tokenizer + codec)
void test_nl_en_integration_mobile_text();
void test_nl_en_integration_professional_text();

// zlib vs NL-EN benchmark tests
void test_zlib_compare_medium_text();
void test_zlib_compare_long_text();

// zlib vs NL-EN variable-width benchmark tests (same texts as AE benchmarks)
void test_zlib_compare_vw_short_message();
void test_zlib_compare_vw_long_text();

// Knuth-Liang hyphenator tests
void test_kl_hyphenator_best_case();
void test_kl_hyphenator_worst_case();
void test_kl_hyphenator_professional_text();

// Knuth-Liang frequency map tests
void test_kl_freqmap_good_hyphenation();
void test_kl_freqmap_bad_hyphenation();

// Knuth-Liang affix stripping tests
void test_kl_affix_strip();

// NL-EN Arithmetic Encoding codec tests
void test_nl_en_ae_codec_best_case();
void test_nl_en_ae_codec_worst_case();
void test_nl_en_ae_codec_long_sequence();

// zlib vs NL-EN AE benchmark tests
void test_zlib_compare_ae_short_message();
void test_zlib_compare_ae_long_text();

// NL-EN Frequency map tests (tokenizer)
void test_nl_en_freqmap_best_case();
void test_nl_en_freqmap_worst_case();

// CSS Tokenizable tests (Requirements 2-5)
void test_css_tokenizable_pattern_match();
void test_css_tokenizable_ascii_fallback();
void test_css_tokenizable_atrule();
void test_css_pattern_codebook();
void test_css_tokenizable_comprehensive();

// CSS AE long rule round-trip (renormalization test)
void test_css_ae_codec_long_rule();

// CSS codec AE tests (csscodec.md)
void test_css_comment_tokenization();
void test_css_flatten_rule_tokens();
void test_css_freqmap_best_case();
void test_css_freqmap_worst_case();
void test_css_ae_codec_best_case();
void test_css_ae_codec_worst_case();

// zlib vs CSS AE benchmark tests
void test_zlib_compare_css_ae_best_case();
void test_zlib_compare_css_ae_worst_case();
void test_zlib_compare_css_ae_long_stylesheet();

// NL-EN optimised codec tests (Steps 1-4)
void test_nl_en_opt_tokenizer_word_match();
void test_nl_en_opt_tokenizer_longest_match();
void test_nl_en_opt_codec_roundtrip();
void test_nl_en_opt_codec_casestyle();
void test_nl_en_opt_codec_long_roundtrip();
void test_nl_en_opt_compression_ratio();

// CSS optimised codec tests (Steps 1-3)
void test_css_opt_codec_roundtrip();
void test_css_opt_codec_long_roundtrip();
void test_css_opt_compression_ratio();
void test_zlib_compare_css_opt_best_case();
void test_zlib_compare_css_opt_worst_case();
void test_zlib_compare_css_opt_long_stylesheet();

// CLJS AE opt codec tests
void test_cljs_ae_opt_codec_best_case();
void test_cljs_ae_opt_codec_worst_case();
void test_cljs_ae_opt_codec_roundtrip();
void test_cljs_ae_opt_codec_long_roundtrip();
void test_zlib_compare_cljs_ae_opt_short();
void test_zlib_compare_cljs_ae_opt_long();

// Detokenizer tests (detok.md)
void test_nl_detokenizer_best_case();
void test_nl_detokenizer_worst_case();
void test_cljs_detokenizer_best_case();
void test_cljs_detokenizer_worst_case();
void test_css_detokenizer_best_case();
void test_css_detokenizer_worst_case();

// HTML detokenizer tests (htmldetok.md)
void test_html_detokenizer_best_case();
void test_html_detokenizer_worst_case();

// HTML codec tests (Requirement 6)
void test_html_codec_codebook_no_duplicates();
void test_html_codec_roundtrip_short();
void test_html_codec_roundtrip_with_attrs();
void test_html_codec_roundtrip_with_subdata();
void test_html_codec_roundtrip_unknown_tag();
void test_html_codec_worst_case();
void test_zlib_compare_html_codec_short();
void test_zlib_compare_html_codec_long();

// Command-line argument parse tests (cmdlinearg.md)
void test_cmdline_parse_text_only();
void test_cmdline_parse_file_flag();
void test_cmdline_parse_decode_flag();
void test_cmdline_parse_format_html();
void test_cmdline_parse_format_css();
void test_cmdline_parse_format_js();
void test_cmdline_parse_format_en_explicit();
void test_cmdline_parse_ascii_flag();
void test_cmdline_parse_outputpath();
void test_cmdline_parse_combined_all_flags();
void test_cmdline_parse_no_args();
void test_cmdline_parse_missing_format_arg();
void test_cmdline_parse_invalid_format();

// Command-line process tests (encode/decode round-trips)
void test_cmdline_process_en_encode_decode();
void test_cmdline_process_html_encode_decode();
void test_cmdline_process_css_encode_decode();
void test_cmdline_process_js_encode_decode();
void test_cmdline_process_file_input_en();
void test_cmdline_process_file_input_css();
void test_cmdline_process_file_input_js();
void test_cmdline_process_file_input_html();

#endif // TOKENIZER_TEST_H
