#include "tokenizer-test.h"

int main(void) {
    printf("Running C HTML and CSS Parser Tests\n");
    printf("====================================\n\n");

    printf("--- HTML TOKENIZER TESTS ---\n");
    test_plain_text_simple();
    test_plain_text_empty();
    test_plain_text_special_chars();
    test_opening_tag_simple();
    test_opening_tag_with_attributes();
    test_self_closing_tag_simple();
    test_self_closing_tag_with_attributes();
    test_closing_tag();
    test_comment_simple();
    test_mixed_text_and_tags();
    test_uppercase_tags();
    test_whitespace_preservation();
    test_large_content();
    test_nested_tags();
    test_unclosed_tag();

    printf("\n--- English TOKENIZER TESTS ---\n");
    test_nl_en_tokenizer_best_case();
    test_nl_en_tokenizer_worst_case();

    printf("\n--- JavaScript TOKENIZER TESTS ---\n");
    test_cl_js_tokenizer_best_case();
    test_cl_js_tokenizer_worst_case();

    printf("\n--- CSS TOKENIZER TESTS ---\n");
    test_css_simple_rule();
    test_css_multiple_selectors();
    test_css_multiple_properties();
    test_css_comment_handling();
    test_css_at_rules();
    test_css_complex_selectors();
    test_css_empty_rules();
    test_css_malformed_input();

    printf("\n--- HTML INTEGRATED TOKENIZER TESTS ---\n");
    test_html_integrated_tokenizer_token_content();
    test_html_integrated_tokenizer_attr_content();
    test_html_integrated_tokenizer_both_content();
    test_html_integrated_tokenizer_real_file();

    printf("\n--- NL-EN CODEC TESTS ---\n");
    test_nl_en_codec_best_case();
    test_nl_en_codec_worst_case();

    printf("\n--- NL-EN INTEGRATION TESTS ---\n");
    test_nl_en_integration_mobile_text();
    test_nl_en_integration_professional_text();

    printf("\n--- ZLIB VS NL-EN BENCHMARK TESTS ---\n");
    test_zlib_compare_medium_text();
    test_zlib_compare_long_text();

    printf("\n--- KNUTH-LIANG HYPHENATOR TESTS ---\n");
    test_kl_hyphenator_best_case();
    test_kl_hyphenator_worst_case();
    test_kl_hyphenator_professional_text();

    printf("\n--- KNUTH-LIANG FREQUENCY MAP TESTS ---\n");
    test_kl_freqmap_good_hyphenation();
    test_kl_freqmap_bad_hyphenation();

    printf("\n====================================\n");
    printf("Results: %d passed, %d failed\n", testsPassed, testsFailed);

    return testsFailed == 0 ? 0 : 1;
}
