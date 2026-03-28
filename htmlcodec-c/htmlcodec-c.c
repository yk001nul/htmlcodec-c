// htmlcodec-c.cpp : Defines the entry point for the application.
//

#include "htmlcodec-c.h"
#include "tokenizer-test.h"



int main()
{
    printf("Running C HTML Parser Tests\n");
    printf("=============================\n\n");

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

    printf("\n=============================\n");
    printf("Results: %d passed, %d failed\n", testsPassed, testsFailed);

    return testsFailed == 0 ? 0 : 1;
}
