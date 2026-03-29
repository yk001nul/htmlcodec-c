// htmlcodec-c.cpp : Defines the entry point for the application.
//

#include "htmlcodec-c.h"
#include "tokenizer-test.h"



int main()
{
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

    printf("\n--- CSS TOKENIZER TESTS ---\n");
    test_css_simple_rule();
    test_css_multiple_selectors();
    test_css_multiple_properties();
    test_css_comment_handling();
    test_css_at_rules();
    test_css_complex_selectors();
    test_css_empty_rules();
    test_css_malformed_input();

    printf("\n====================================\n");
    printf("Results: %d passed, %d failed\n", testsPassed, testsFailed);

    return testsFailed == 0 ? 0 : 1;
}
