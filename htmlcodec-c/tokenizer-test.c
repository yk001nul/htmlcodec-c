#include "tokenizer-test.h"
#include "tokenizer-test.h"

int testsPassed = 0;
int testsFailed = 0;

void assert_equal_int(int actual, int expected, const char* message) {
    if (actual != expected) {
        printf("? FAIL: %s (expected %d, got %d)\n", message, expected, actual);
        testsFailed++;
    }
    else {
        testsPassed++;
    }
}

void assert_equal_str(const char* actual, const char* expected, const char* message) {
    if (strcmp(actual, expected) != 0) {
        printf("? FAIL: %s (expected '%s', got '%s')\n", message, expected, actual);
        testsFailed++;
    }
    else {
        testsPassed++;
    }
}

void assert_true(int condition, const char* message) {
    if (!condition) {
        printf("? FAIL: %s\n", message);
        testsFailed++;
    }
    else {
        testsPassed++;
    }
}

// Plain text tests
void test_plain_text_simple() {
    TokenArray* result = parseHTML("Hello World");
    assert_equal_int(result->count, 1, "Plain text: should have 1 token");
    assert_equal_int(result->tokens[0].type, 0, "Plain text: should be text type");
    assert_equal_str(result->tokens[0].data.text.content, "Hello World", "Plain text: content mismatch");
    freeTokenArray(result);
    printf("? Plain text - simple string\n");
}

void test_plain_text_empty() {
    TokenArray* result = parseHTML("");
    assert_equal_int(result->count, 0, "Empty HTML: should return 0 tokens");
    freeTokenArray(result);
    printf("? Plain text - empty string\n");
}

void test_plain_text_special_chars() {
    TokenArray* result = parseHTML("Text with <>&");
    assert_equal_int(result->count, 1, "Special chars: should have 1 token");
    assert_true(strchr(result->tokens[0].data.text.content, '<') != NULL, "Special chars: < should be preserved");
    freeTokenArray(result);
    printf("? Plain text - special characters\n");
}

// Opening tag tests
void test_opening_tag_simple() {
    TokenArray* result = parseHTML("<div>");
    assert_equal_int(result->count, 1, "Opening tag: should have 1 token");
    assert_equal_int(result->tokens[0].type, 1, "Opening tag: should be openTag type");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Opening tag: name mismatch");
    freeTokenArray(result);
    printf("? Opening tag - simple tag\n");
}

void test_opening_tag_with_attributes() {
    TokenArray* result = parseHTML("<div id=\"main\" class=\"container\">");
    assert_equal_int(result->count, 1, "Opening tag with attrs: should have 1 token");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Opening tag with attrs: name");
    assert_equal_int(result->tokens[0].data.tag.attrCount, 2, "Opening tag with attrs: should have 2 attributes");
    assert_equal_str(result->tokens[0].data.tag.attributes[0].name, "id", "Opening tag attrs: first attr name");
    assert_equal_str(result->tokens[0].data.tag.attributes[0].value, "main", "Opening tag attrs: first attr value");
    freeTokenArray(result);
    printf("? Opening tag - with attributes\n");
}

// Self-closing tag tests
void test_self_closing_tag_simple() {
    TokenArray* result = parseHTML("<br/>");
    assert_equal_int(result->count, 1, "Self-closing: should have 1 token");
    assert_equal_int(result->tokens[0].type, 1, "Self-closing: should be openTag");
    assert_equal_str(result->tokens[0].data.tag.name, "br", "Self-closing: name");
    assert_equal_int(result->tokens[0].data.tag.selfClosing, 1, "Self-closing: should be marked");
    freeTokenArray(result);
    printf("? Self-closing tag - simple\n");
}

void test_self_closing_tag_with_attributes() {
    TokenArray* result = parseHTML("<img src=\"test.png\" alt=\"image\"/>");
    assert_equal_int(result->count, 1, "Self-closing with attrs: should have 1 token");
    assert_equal_str(result->tokens[0].data.tag.name, "img", "Self-closing with attrs: name");
    assert_equal_int(result->tokens[0].data.tag.selfClosing, 1, "Self-closing with attrs: marked");
    assert_equal_int(result->tokens[0].data.tag.attrCount, 2, "Self-closing with attrs: should have 2 attributes");
    freeTokenArray(result);
    printf("? Self-closing tag - with attributes\n");
}

// Closing tag tests
void test_closing_tag() {
    TokenArray* result = parseHTML("</div>");
    assert_equal_int(result->count, 1, "Closing tag: should have 1 token");
    assert_equal_int(result->tokens[0].type, 2, "Closing tag: should be closeTag type");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Closing tag: name mismatch");
    freeTokenArray(result);
    printf("? Closing tag - simple\n");
}

// Comment tests
void test_comment_simple() {
    TokenArray* result = parseHTML("<!-- This is a comment -->");
    assert_equal_int(result->count, 0, "Comment: should be ignored");
    freeTokenArray(result);
    printf("? Comment - simple comment\n");
}

// Mixed content tests
void test_mixed_text_and_tags() {
    TokenArray* result = parseHTML("Hello <b>bold</b> world");
    assert_equal_int(result->count, 5, "Mixed: should have 5 tokens");
    assert_equal_int(result->tokens[0].type, 0, "Mixed: first should be text");
    assert_equal_int(result->tokens[1].type, 1, "Mixed: second should be openTag");
    assert_equal_int(result->tokens[3].type, 2, "Mixed: fourth should be closeTag");
    freeTokenArray(result);
    printf("? Mixed - text and tags\n");
}

// Case conversion test
void test_uppercase_tags() {
    TokenArray* result = parseHTML("<DIV><SPAN>");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Case: DIV should be div");
    assert_equal_str(result->tokens[1].data.tag.name, "span", "Case: SPAN should be span");
    freeTokenArray(result);
    printf("? Case conversion - uppercase tags\n");
}

// Whitespace test
void test_whitespace_preservation() {
    TokenArray* result = parseHTML("Text   with   spaces");
    assert_equal_int(result->count, 1, "Whitespace: should have 1 token");
    assert_true(strstr(result->tokens[0].data.text.content, "   ") != NULL, "Whitespace: spaces should be preserved");
    freeTokenArray(result);
    printf("? Whitespace - preserve spaces\n");
}

// Large content test
void test_large_content() {
    char largeText[1024] = { 0 };
    memset(largeText, 'A', 500);
    TokenArray* result = parseHTML(largeText);
    assert_equal_int(result->count, 1, "Large content: should have 1 token");
    assert_equal_int(strlen(result->tokens[0].data.text.content), 500, "Large content: length mismatch");
    freeTokenArray(result);
    printf("? Large content - long text\n");
}

// Nested tags test
void test_nested_tags() {
    TokenArray* result = parseHTML("<div><p><span>text</span></p></div>");
    assert_equal_int(result->count, 7, "Nested: should have 7 tokens");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Nested: first tag");
    assert_equal_str(result->tokens[1].data.tag.name, "p", "Nested: second tag");
    freeTokenArray(result);
    printf("? Nested - multiple levels\n");
}

// Worst case - unclosed tag
void test_unclosed_tag() {
    TokenArray* result = parseHTML("<div>content</div");
    assert_true(result->count > 0, "Unclosed: should parse something");
    freeTokenArray(result);
    printf("? Worst case - unclosed tag\n");
}
