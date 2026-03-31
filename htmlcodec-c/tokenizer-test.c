#include "tokenizer-test.h"
#include "nl-en-tokenizer.h"
#include "cl-javascript-en-tokenizer.h"

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

// HTML Tokenizer Tests - Plain text tests
void test_plain_text_simple() {
    HTMLTokenArray* result = parseHTML("Hello World");
    assert_equal_int(result->count, 1, "Plain text: should have 1 token");
    assert_equal_int(result->tokens[0].type, 0, "Plain text: should be text type");
    assert_equal_str(result->tokens[0].data.text.content, "Hello World", "Plain text: content mismatch");
    freeHTMLTokenArray(result);
    printf("? Plain text - simple string\n");
}

void test_plain_text_empty() {
    HTMLTokenArray* result = parseHTML("");
    assert_equal_int(result->count, 0, "Empty HTML: should return 0 tokens");
    freeHTMLTokenArray(result);
    printf("? Plain text - empty string\n");
}

void test_plain_text_special_chars() {
    HTMLTokenArray* result = parseHTML("Text with <>&");
    assert_equal_int(result->count, 1, "Special chars: should have 1 token");
    assert_true(strchr(result->tokens[0].data.text.content, '<') != NULL, "Special chars: < should be preserved");
    freeHTMLTokenArray(result);
    printf("? Plain text - special characters\n");
}

// HTML Tokenizer Tests - Opening tag tests
void test_opening_tag_simple() {
    HTMLTokenArray* result = parseHTML("<div>");
    assert_equal_int(result->count, 1, "Opening tag: should have 1 token");
    assert_equal_int(result->tokens[0].type, 1, "Opening tag: should be openTag type");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Opening tag: name mismatch");
    freeHTMLTokenArray(result);
    printf("? Opening tag - simple tag\n");
}

void test_opening_tag_with_attributes() {
    HTMLTokenArray* result = parseHTML("<div id=\"main\" class=\"container\">");
    assert_equal_int(result->count, 1, "Opening tag with attrs: should have 1 token");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Opening tag with attrs: name");
    assert_equal_int(result->tokens[0].data.tag.attrCount, 2, "Opening tag with attrs: should have 2 attributes");
    assert_equal_str(result->tokens[0].data.tag.attributes[0].name, "id", "Opening tag attrs: first attr name");
    assert_equal_str(result->tokens[0].data.tag.attributes[0].value, "main", "Opening tag attrs: first attr value");
    freeHTMLTokenArray(result);
    printf("? Opening tag - with attributes\n");
}

// HTML Tokenizer Tests - Self-closing tag tests
void test_self_closing_tag_simple() {
    HTMLTokenArray* result = parseHTML("<br/>");
    assert_equal_int(result->count, 1, "Self-closing: should have 1 token");
    assert_equal_int(result->tokens[0].type, 1, "Self-closing: should be openTag");
    assert_equal_str(result->tokens[0].data.tag.name, "br", "Self-closing: name");
    assert_equal_int(result->tokens[0].data.tag.selfClosing, 1, "Self-closing: should be marked");
    freeHTMLTokenArray(result);
    printf("? Self-closing tag - simple\n");
}

void test_self_closing_tag_with_attributes() {
    HTMLTokenArray* result = parseHTML("<img src=\"test.png\" alt=\"image\"/>");
    assert_equal_int(result->count, 1, "Self-closing with attrs: should have 1 token");
    assert_equal_str(result->tokens[0].data.tag.name, "img", "Self-closing with attrs: name");
    assert_equal_int(result->tokens[0].data.tag.selfClosing, 1, "Self-closing with attrs: marked");
    assert_equal_int(result->tokens[0].data.tag.attrCount, 2, "Self-closing with attrs: should have 2 attributes");
    freeHTMLTokenArray(result);
    printf("? Self-closing tag - with attributes\n");
}

// HTML Tokenizer Tests - Closing tag tests
void test_closing_tag() {
    HTMLTokenArray* result = parseHTML("</div>");
    assert_equal_int(result->count, 1, "Closing tag: should have 1 token");
    assert_equal_int(result->tokens[0].type, 2, "Closing tag: should be closeTag type");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Closing tag: name mismatch");
    freeHTMLTokenArray(result);
    printf("? Closing tag - simple\n");
}

// HTML Tokenizer Tests - Comment tests
void test_comment_simple() {
    HTMLTokenArray* result = parseHTML("<!-- This is a comment -->");
    assert_equal_int(result->count, 0, "Comment: should be ignored");
    freeHTMLTokenArray(result);
    printf("? Comment - simple comment\n");
}

// HTML Tokenizer Tests - Mixed content tests
void test_mixed_text_and_tags() {
    HTMLTokenArray* result = parseHTML("Hello <b>bold</b> world");
    assert_equal_int(result->count, 5, "Mixed: should have 5 tokens");
    assert_equal_int(result->tokens[0].type, 0, "Mixed: first should be text");
    assert_equal_int(result->tokens[1].type, 1, "Mixed: second should be openTag");
    assert_equal_int(result->tokens[3].type, 2, "Mixed: fourth should be closeTag");
    freeHTMLTokenArray(result);
    printf("? Mixed - text and tags\n");
}

// HTML Tokenizer Tests - Case conversion test
void test_uppercase_tags() {
    HTMLTokenArray* result = parseHTML("<DIV><SPAN>");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Case: DIV should be div");
    assert_equal_str(result->tokens[1].data.tag.name, "span", "Case: SPAN should be span");
    freeHTMLTokenArray(result);
    printf("? Case conversion - uppercase tags\n");
}

// HTML Tokenizer Tests - Whitespace test
void test_whitespace_preservation() {
    HTMLTokenArray* result = parseHTML("Text   with   spaces");
    assert_equal_int(result->count, 1, "Whitespace: should have 1 token");
    assert_true(strstr(result->tokens[0].data.text.content, "   ") != NULL, "Whitespace: spaces should be preserved");
    freeHTMLTokenArray(result);
    printf("? Whitespace - preserve spaces\n");
}

// HTML Tokenizer Tests - Large content test
void test_large_content() {
    char largeText[1024] = { 0 };
    memset(largeText, 'A', 500);
    HTMLTokenArray* result = parseHTML(largeText);
    assert_equal_int(result->count, 1, "Large content: should have 1 token");
    assert_equal_int(strlen(result->tokens[0].data.text.content), 500, "Large content: length mismatch");
    freeHTMLTokenArray(result);
    printf("? Large content - long text\n");
}

// HTML Tokenizer Tests - Nested tags test
void test_nested_tags() {
    HTMLTokenArray* result = parseHTML("<div><p><span>text</span></p></div>");
    assert_equal_int(result->count, 7, "Nested: should have 7 tokens");
    assert_equal_str(result->tokens[0].data.tag.name, "div", "Nested: first tag");
    assert_equal_str(result->tokens[1].data.tag.name, "p", "Nested: second tag");
    freeHTMLTokenArray(result);
    printf("? Nested - multiple levels\n");
}

// HTML Tokenizer Tests - Worst case - unclosed tag
void test_unclosed_tag() {
    HTMLTokenArray* result = parseHTML("<div>content</div");
    assert_true(result->count > 0, "Unclosed: should parse something");
    freeHTMLTokenArray(result);
    printf("? Worst case - unclosed tag\n");
}

// English tokenizer tests - best case
void test_nl_en_tokenizer_best_case() {
    const char* text = "Information and community in development";
    NLTokenArray* result = tokenizeEnglish(text);
    assert_true(result != NULL, "English tokenizer result must not be NULL");
    printf("? NL-EN best case: input length %zu -> token count %zu\n", strlen(text), result->count);
    assert_true(result->count > 0, "English tokenizer best case: at least one token");

    // Verify fallback has one-char granularity for unmatched boundaries
    const char* text2 = "do d";
    NLTokenArray* result2 = tokenizeEnglish(text2);
    assert_true(result2 != NULL, "English tokenizer second best case must not be NULL");
    assert_true(result2->tokens[0].isPattern, "NL-EN should match 'do' pattern first");
    assert_true(result2->tokens[1].isPattern == false, "NL-EN second token should be space");
    assert_true(result2->tokens[2].isPattern == false, "NL-EN third token should be unmatched 'd' (non-pattern)");
    assert_equal_int(result2->count, 3, "NL-EN second best case count");
    freeNLTokenArray(result2);

    freeNLTokenArray(result);
}

// English tokenizer tests - worst case
void test_nl_en_tokenizer_worst_case() {
    const char* text = "!!!!????~~~~";
    NLTokenArray* result = tokenizeEnglish(text);
    assert_true(result != NULL, "English tokenizer result must not be NULL");
    printf("? NL-EN worst case: input length %zu -> token count %zu\n", strlen(text), result->count);
    assert_equal_int(result->count, (int)strlen(text), "English tokenizer worst case should produce one token per char (single-size fallback)");
    freeNLTokenArray(result);
}

void test_cl_js_tokenizer_best_case() {
    const char* text = "function doFour()";
    CLJSTokenArray* result = tokenizeJavaScript(text);
    assert_true(result != NULL, "JS tokenizer result must not be NULL");
    printf("? CL-JS best case: input length %zu -> token count %zu\n", strlen(text), result->count);
    assert_true(result->count > 0, "JS tokenizer best case: should produce tokens");
    assert_true(result->tokens[0].isPattern, "Token 0 should match 'function'");
    assert_true(result->tokens[1].isPattern == false, "Token 1 should be space char");
    assert_true(result->tokens[2].isPattern, "Token 2 should match 'do'");
    freeCLJSTokenArray(result);
}

void test_cl_js_tokenizer_worst_case() {
    size_t iterations = 1000;
    char* buffer = (char*)malloc(iterations + 1);
    for (size_t i = 0; i < iterations; i++) buffer[i] = 'x';
    buffer[iterations] = '\0';

    CLJSTokenArray* result = tokenizeJavaScript(buffer);
    assert_true(result != NULL, "JS tokenizer worst case result must not be NULL");
    printf("? CL-JS worst case: input length %zu -> token count %zu\n", strlen(buffer), result->count);
    assert_true(result->count > 0, "JS tokenizer worst case: should produce tokens");
    freeCLJSTokenArray(result);
    free(buffer);
}

// CSS Tokenizer Tests - Best case scenarios
void test_css_simple_rule() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "body { color: red; }";
    parseCSS(css, result);
    assert_equal_int(result->count, 1, "CSS simple: should have 1 token");
    assert_equal_int(result->tokens[0].type, 0, "CSS simple: should be selector rule");
    assert_equal_str(result->tokens[0].data.rule.selector, "body", "CSS simple: selector mismatch");
    assert_equal_int(result->tokens[0].data.rule.propertyCount, 1, "CSS simple: should have 1 property");
    assert_equal_str(result->tokens[0].data.rule.properties[0].name, "color", "CSS simple: property name");
    assert_equal_str(result->tokens[0].data.rule.properties[0].value, "red", "CSS simple: property value");
    freeCSS(result);
    printf("? CSS - simple rule\n");
}

void test_css_multiple_selectors() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "h1, h2 { margin: 0; }";
    parseCSS(css, result);
    assert_equal_int(result->count, 1, "CSS multi-sel: should have 1 token");
    assert_true(strchr(result->tokens[0].data.rule.selector, ',') != NULL, "CSS multi-sel: should contain comma");
    freeCSS(result);
    printf("? CSS - multiple selectors\n");
}

void test_css_multiple_properties() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = ".container { width: 100%; height: 50px; padding: 10px; }";
    parseCSS(css, result);
    assert_equal_int(result->count, 1, "CSS multi-prop: should have 1 token");
    assert_equal_int(result->tokens[0].data.rule.propertyCount, 3, "CSS multi-prop: should have 3 properties");
    freeCSS(result);
    printf("? CSS - multiple properties\n");
}

// CSS Tokenizer Tests - Worse case scenarios
void test_css_comment_handling() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "/* This is a comment */ body { color: blue; }\n";
    parseCSS(css, result);
    assert_equal_int(result->count, 2, "CSS comment: should have comment + rule");
    assert_equal_int(result->tokens[0].type, 2, "CSS comment: first should be comment");
    assert_equal_int(result->tokens[1].type, 0, "CSS comment: second should be rule");
    freeCSS(result);
    printf("? CSS - comment handling\n");
}

void test_css_at_rules() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "@media (max-width: 600px) { body { font-size: 14px; } }";
    parseCSS(css, result);
    assert_equal_int(result->count, 1, "CSS at-rule: should have 1 at-rule token");
    assert_equal_int(result->tokens[0].type, 1, "CSS at-rule: should be at-rule type");
    freeCSS(result);
    printf("? CSS - at-rules\n");
}

void test_css_complex_selectors() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "div > p.active:hover { color: green; }";
    parseCSS(css, result);
    assert_equal_int(result->count, 1, "CSS complex: should have 1 token");
    assert_equal_int(result->tokens[0].type, 0, "CSS complex: should be selector rule");
    assert_true(strlen(result->tokens[0].data.rule.selector) > 0, "CSS complex: selector should not be empty");
    freeCSS(result);
    printf("? CSS - complex selectors\n");
}

void test_css_empty_rules() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "div { }";
    parseCSS(css, result);
    assert_equal_int(result->count, 1, "CSS empty: should have 1 token");
    assert_equal_int(result->tokens[0].data.rule.propertyCount, 0, "CSS empty: should have 0 properties");
    freeCSS(result);
    printf("? CSS - empty rules\n");
}

void test_css_malformed_input() {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    const char* css = "broken { color: red";
    parseCSS(css, result);
    assert_true(result->count >= 0, "CSS malformed: should handle gracefully");
    freeCSS(result);
    printf("? CSS - malformed input (worst case)\n");
}

void test_html_integrated_tokenizer_token_content() {
    HTMLTokenArray* result = parseHTML("<style>body { color: red; }</style><script>var a = 1;</script><div>Plain text</div>");
    assert_equal_int(result->count, 9, "Integrated token content: count");

    assert_equal_int(result->tokens[1].subdataType, HTML_SUBDATA_CSS, "Style text should be CSS subdata");
    assert_true(result->tokens[1].subdata.css != NULL, "Style subdata should be present");

    assert_equal_int(result->tokens[4].subdataType, HTML_SUBDATA_JS, "Script text should be JS subdata");
    assert_true(result->tokens[4].subdata.js != NULL, "Script subdata should be present");

    assert_equal_int(result->tokens[7].subdataType, HTML_SUBDATA_NL, "Plain text before closing div should be NL subdata");
    assert_true(result->tokens[7].subdata.nl != NULL, "NL subdata should be present");

    freeHTMLTokenArray(result);
    printf("? HTML integrated tokenizer - token based content\n");
}

void test_html_integrated_tokenizer_attr_content() {
    HTMLTokenArray* result = parseHTML("<div style=\"color:blue;\" onclick=\"alert('x')\">x</div>");
    assert_equal_int(result->count, 3, "Integrated attr content: count");

    HTMLAttribute* styleAttr = &result->tokens[0].data.tag.attributes[0];
    assert_equal_int(styleAttr->subdataType, HTML_SUBDATA_CSS, "Style attribute should be CSS subdata");
    assert_true(styleAttr->subdata.css != NULL, "Style attribute subdata should be present");

    HTMLAttribute* onAttr = &result->tokens[0].data.tag.attributes[1];
    assert_equal_int(onAttr->subdataType, HTML_SUBDATA_JS, "on* attribute should be JS subdata");
    assert_true(onAttr->subdata.js != NULL, "on* attribute subdata should be present");

    freeHTMLTokenArray(result);
    printf("? HTML integrated tokenizer - attribute based content\n");
}

void test_html_integrated_tokenizer_both_content() {
    HTMLTokenArray* result = parseHTML("<script>function test() { return 5; }</script><div style=\"background: white;\" onmouseover=\"console.log('h');\">Hello</div>");
    assert_equal_int(result->count, 6, "Integrated both content: count");

    assert_equal_int(result->tokens[1].subdataType, HTML_SUBDATA_JS, "Script text should be JS subdata in both test");
    assert_true(result->tokens[1].subdata.js != NULL, "Script text subdata should be present in both test");

    HTMLAttribute* styleAttr = &result->tokens[3].data.tag.attributes[0];
    assert_equal_int(styleAttr->subdataType, HTML_SUBDATA_CSS, "Style attribute should be CSS subdata in both test");
    assert_true(styleAttr->subdata.css != NULL, "Style attribute subdata should be present in both test");

    HTMLAttribute* onAttr = &result->tokens[3].data.tag.attributes[1];
    assert_equal_int(onAttr->subdataType, HTML_SUBDATA_JS, "on* attribute should be JS subdata in both test");
    assert_true(onAttr->subdata.js != NULL, "on* attribute subdata should be present in both test");

    assert_equal_int(result->tokens[4].subdataType, HTML_SUBDATA_NL, "Text token should be NL subdata in both test");
    assert_true(result->tokens[4].subdata.nl != NULL, "Text token NL subdata should be present in both test");

    freeHTMLTokenArray(result);
    printf("? HTML integrated tokenizer - both token and attribute content\n");
}
















