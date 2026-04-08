#include <stdio.h>
#ifndef TOKENIZER_TEST_H
#define TOKENIZER_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "html-tokenizer.h"
#include "css-tokenizer.h"

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

// Knuth-Liang hyphenator tests
void test_kl_hyphenator_best_case();
void test_kl_hyphenator_worst_case();
void test_kl_hyphenator_professional_text();

// Knuth-Liang frequency map tests
void test_kl_freqmap_good_hyphenation();
void test_kl_freqmap_bad_hyphenation();

#endif // TOKENIZER_TEST_H
