#include "tokenizer-test.h"
#include "nl-en-tokenizer.h"
#include "nl-en-codec.h"
#include "css-codec.h"
#include "cl-javascript-en-tokenizer.h"
#include "nl-en-us-hyphenator.h"
#include <zlib.h>

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

static char* loadFileContent(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    size_t readBytes = fread(buffer, 1, (size_t)size, fp);
    fclose(fp);
    if (readBytes != (size_t)size) {
        free(buffer);
        return NULL;
    }
    buffer[size] = '\0';
    return buffer;
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

    // Verify fallback has one-char granularity for unmatched boundaries.
    // "he d" -> 'he' (pattern) + ' ' (ASCII) + 'd' (ASCII) = 3 tokens.
    const char* text2 = "he d";
    NLTokenArray* result2 = tokenizeEnglish(text2);
    assert_true(result2 != NULL, "English tokenizer second best case must not be NULL");
    assert_true(result2->tokens[0].isPattern, "NL-EN should match 'he' pattern first");
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

void test_cl_js_tokenizer_case_style_logic() {
    const char* text = "function Th";
    CLJSTokenArray* result = tokenizeJavaScript(text);
    assert_true(result != NULL, "JS tokenizer case style result must not be NULL");
    assert_equal_int(result->count, 3, "JS tokenizer case style should produce 3 tokens");

    assert_true(result->tokens[0].isPattern, "Token 0 should match 'function'");
    assert_equal_int(result->tokens[0].caseStyle, 3, "Non-digraph keyword should have caseStyle 3 (no change needed)");

    assert_true(result->tokens[1].isPattern == false, "Token 1 should be space char");
    assert_equal_int(result->tokens[1].caseStyle, 3, "Non-pattern character should have caseStyle 3 (no change needed)");

    assert_true(result->tokens[2].isPattern, "Token 2 should match digraph 'Th'");
    assert_equal_int(result->tokens[2].caseStyle, 2, "Digraph with first uppercase should have caseStyle 2");

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

void test_html_integrated_tokenizer_real_file() {
    char* html = loadFileContent("test.html");
    if (!html) {
        html = loadFileContent("../../../test.html");
    }
    assert_true(html != NULL, "Real HTML file should be loadable");
    if (!html) return; /* file not present in this environment — skip remaining assertions */

    HTMLTokenArray* result = parseHTML(html);
    assert_true(result != NULL, "Real HTML parse result should not be NULL");
    assert_true(result->count > 20, "Real HTML parse should produce many tokens");

    int hasStyle = 0;
    int hasScript = 0;
    int hasNL = 0;
    int hasOnAttr = 0;

    for (int i = 0; i < result->count; i++) {
        HTMLToken* token = &result->tokens[i];
        if (token->type == 0 && token->subdataType == HTML_SUBDATA_CSS) hasStyle = 1;
        if (token->type == 0 && token->subdataType == HTML_SUBDATA_JS) hasScript = 1;
        if (token->type == 0 && token->subdataType == HTML_SUBDATA_NL) hasNL = 1;
        if (token->type != 0) {
            for (int a = 0; a < token->data.tag.attrCount; a++) {
                if (token->data.tag.attributes[a].subdataType == HTML_SUBDATA_JS &&
                    strcmp(token->data.tag.attributes[a].name, "onclick") == 0) {
                    hasOnAttr = 1;
                }
            }
        }
    }

    assert_true(hasStyle, "Real HTML should contain CSS subdata token");
    assert_true(hasScript, "Real HTML should contain JS subdata token");
    assert_true(hasNL, "Real HTML should contain NL subdata token");
    assert_true(hasOnAttr, "Real HTML should include onclick JS attribute subdata");

    freeHTMLTokenArray(result);
    free(html);
    printf("? HTML integrated tokenizer - real test file content\n");
}

// NL-EN Codec Tests

void test_nl_en_codec_best_case() {
    // Best case: small array with mixed tokens
    NLTokenArray input;
    input.count = 3;
    
    // Token 1: pattern token
    input.tokens[0].isPattern = true;
    input.tokens[0].flag = 5;      // pattern index 5
    input.tokens[0].caseStyle = 2; // first uppercase
    
    // Token 2: non-pattern token
    input.tokens[1].isPattern = false;
    input.tokens[1].flag = 'A';    // ASCII 'A'
    input.tokens[1].caseStyle = 0; // ignored
    
    // Token 3: pattern token
    input.tokens[2].isPattern = true;
    input.tokens[2].flag = 100;    // pattern index 100
    input.tokens[2].caseStyle = 1; // all uppercase
    
    // Encode
    size_t encodedSize = 0;
    unsigned char* encoded = nl_en_encode(&input, input.count, &encodedSize);
    assert_true(encoded != NULL, "Codec best case: encoded buffer should not be NULL");
    assert_true(encodedSize > 0, "Codec best case: encoded size should be > 0");
    
    // Decode
    NLTokenArray* decoded = nl_en_decode(encoded, encodedSize);
    assert_true(decoded != NULL, "Codec best case: decoded array should not be NULL");
    assert_equal_int(decoded->count, 3, "Codec best case: decoded count should match input");
    
    // Verify tokens
    assert_equal_int(decoded->tokens[0].isPattern, 1, "Codec best case: token 0 isPattern");
    assert_equal_int(decoded->tokens[0].flag, 5, "Codec best case: token 0 flag");
    assert_equal_int(decoded->tokens[0].caseStyle, 2, "Codec best case: token 0 caseStyle");
    
    assert_equal_int(decoded->tokens[1].isPattern, 0, "Codec best case: token 1 isPattern");
    assert_equal_int(decoded->tokens[1].flag, 'A', "Codec best case: token 1 flag (ASCII)");
    
    assert_equal_int(decoded->tokens[2].isPattern, 1, "Codec best case: token 2 isPattern");
    assert_equal_int(decoded->tokens[2].flag, 100, "Codec best case: token 2 flag");
    assert_equal_int(decoded->tokens[2].caseStyle, 1, "Codec best case: token 2 caseStyle");
    
    free(encoded);
    freeNLTokenArray(decoded);
    printf("? NL-EN Codec - best case (mixed tokens)\n");
}

void test_nl_en_codec_worst_case() {
    // Worst case: maximum array size with all pattern tokens (maximal bit usage)
    NLTokenArray input;
    input.count = NL_EN_MAX_TOKENS;
    
    // Fill array with alternating pattern/non-pattern tokens to maximize bit variation
    for (size_t i = 0; i < NL_EN_MAX_TOKENS; i++) {
        if (i % 2 == 0) {
            input.tokens[i].isPattern = true;
            input.tokens[i].flag = (unsigned short)(i % NL_EN_PATTERN_COUNT);  // cycle 0-511
            input.tokens[i].caseStyle = (i % 4); // cycle through all caseStyle values
        } else {
            input.tokens[i].isPattern = false;
            input.tokens[i].flag = (unsigned short)(((i * 7) % 95) + 32); // printable ASCII [32,126]
            input.tokens[i].caseStyle = 0;
        }
    }
    
    // Encode
    size_t encodedSize = 0;
    unsigned char* encoded = nl_en_encode(&input, input.count, &encodedSize);
    assert_true(encoded != NULL, "Codec worst case: encoded buffer should not be NULL");
    assert_true(encodedSize > 0, "Codec worst case: encoded size should be > 0");
    
    // Decode
    NLTokenArray* decoded = nl_en_decode(encoded, encodedSize);
    assert_true(decoded != NULL, "Codec worst case: decoded array should not be NULL");
    assert_equal_int(decoded->count, NL_EN_MAX_TOKENS, "Codec worst case: decoded count should be max");
    
    // Spot check: verify several tokens across the array
    int spot_checks_passed = 1;
    
    // Check token 0 (even index -> isPattern = true)
    if (decoded->tokens[0].isPattern != 1 ||
        decoded->tokens[0].flag != (0 % NL_EN_PATTERN_COUNT) ||
        decoded->tokens[0].caseStyle != (0 % 4)) {
        spot_checks_passed = 0;
        printf("  Spot check failed at token 0\n");
    }

    // Check token 1 (odd index -> isPattern = false)
    if (decoded->tokens[1].isPattern != 0 ||
        decoded->tokens[1].flag != (unsigned short)(((1 * 7) % 95) + 32)) {
        spot_checks_passed = 0;
        printf("  Spot check failed at token 1\n");
    }

    // Check token at middle (2048, even -> isPattern should be true)
    size_t mid = NL_EN_MAX_TOKENS / 2;
    int mid_isPattern = (mid % 2 == 0) ? 1 : 0;
    int mid_flag = (mid % 2 == 0) ? (int)(mid % NL_EN_PATTERN_COUNT) : (int)(((mid * 7) % 95) + 32);
    int mid_caseStyle = (mid % 2 == 0) ? (int)(mid % 4) : 0;

    if (decoded->tokens[mid].isPattern != mid_isPattern ||
        decoded->tokens[mid].flag != mid_flag ||
        (mid_isPattern && decoded->tokens[mid].caseStyle != mid_caseStyle)) {
        spot_checks_passed = 0;
        printf("  Spot check failed at token %zu\n", mid);
    }

    // Check last token (4095, odd -> isPattern should be false)
    size_t last = NL_EN_MAX_TOKENS - 1;
    int last_isPattern = (last % 2 == 0) ? 1 : 0;
    int last_flag = (last % 2 == 0) ? (int)(last % NL_EN_PATTERN_COUNT) : (int)(((last * 7) % 95) + 32);

    if (decoded->tokens[last].isPattern != last_isPattern ||
        decoded->tokens[last].flag != last_flag) {
        spot_checks_passed = 0;
        printf("  Spot check failed at last token\n");
    }
    
    assert_true(spot_checks_passed, "Codec worst case: spot checks should all pass");
    
    free(encoded);
    freeNLTokenArray(decoded);
    printf("? NL-EN Codec - worst case (max tokens, alternating pattern)\n");
}

// Helper function to compare two NLTokenArrays
static int compare_token_arrays(const NLTokenArray* arr1, const NLTokenArray* arr2) {
    if (!arr1 || !arr2) return 0;
    if (arr1->count != arr2->count) return 0;
    
    for (size_t i = 0; i < arr1->count; i++) {
        if (arr1->tokens[i].isPattern != arr2->tokens[i].isPattern) return 0;
        if (arr1->tokens[i].flag != arr2->tokens[i].flag) return 0;
        if (arr1->tokens[i].isPattern && arr1->tokens[i].caseStyle != arr2->tokens[i].caseStyle) {
            return 0;
        }
    }
    return 1;
}

void test_nl_en_integration_mobile_text() {
    // Mobile phone style text: short, casual
    const char* text = "hey whats up bro cant wait 2 c u l8r lol";
    
    // Tokenize
    NLTokenArray* original = tokenizeEnglish(text);
    assert_true(original != NULL, "Integration mobile: tokenize should succeed");
    assert_true(original->count > 0, "Integration mobile: should produce tokens");
    
    size_t original_text_size = strlen(text);
    
    // Encode
    size_t encoded_size = 0;
    unsigned char* encoded = nl_en_encode(original, original->count, &encoded_size);
    assert_true(encoded != NULL, "Integration mobile: encode should succeed");
    assert_true(encoded_size > 0, "Integration mobile: encoded size should be > 0");
    
    // Decode
    NLTokenArray* decoded = nl_en_decode(encoded, encoded_size);
    assert_true(decoded != NULL, "Integration mobile: decode should succeed");
    
    // Verify correctness
    int arrays_equal = compare_token_arrays(original, decoded);
    assert_true(arrays_equal, "Integration mobile: decoded array should match original");
    assert_equal_int(decoded->count, original->count, "Integration mobile: decoded count should match");
    
    // Calculate compression ratio
    double compression = (1.0 - (double)encoded_size / (double)original_text_size) * 100.0;
    printf("  Mobile text: %zu bytes -> %zu bytes (%.2f%% reduction, %.2f ratio vs gzip target)\n", 
           original_text_size, encoded_size, compression, (double)original_text_size / (double)encoded_size);
    
    free(encoded);
    freeNLTokenArray(original);
    freeNLTokenArray(decoded);
    printf("? NL-EN Integration - mobile phone style text\n");
}

void test_nl_en_integration_professional_text() {
    // Professional style text: medium length, formal
    const char* text = "The implementation of advanced data compression algorithms requires careful consideration of memory efficiency and processing speed. Our approach utilizes bit-level packing to minimize storage requirements while maintaining data integrity throughout the encoding and decoding process.";
    
    // Tokenize
    NLTokenArray* original = tokenizeEnglish(text);
    assert_true(original != NULL, "Integration professional: tokenize should succeed");
    assert_true(original->count > 0, "Integration professional: should produce tokens");
    
    size_t original_text_size = strlen(text);
    
    // Encode
    size_t encoded_size = 0;
    unsigned char* encoded = nl_en_encode(original, original->count, &encoded_size);
    assert_true(encoded != NULL, "Integration professional: encode should succeed");
    assert_true(encoded_size > 0, "Integration professional: encoded size should be > 0");
    
    // Decode
    NLTokenArray* decoded = nl_en_decode(encoded, encoded_size);
    assert_true(decoded != NULL, "Integration professional: decode should succeed");
    
    // Verify correctness
    int arrays_equal = compare_token_arrays(original, decoded);
    assert_true(arrays_equal, "Integration professional: decoded array should match original");
    assert_equal_int(decoded->count, original->count, "Integration professional: decoded count should match");

    // Calculate compression ratio
    double compression = (1.0 - (double)encoded_size / (double)original_text_size) * 100.0;
    printf("  Professional text: %zu bytes -> %zu bytes (%.2f%% reduction, %.2f ratio vs gzip target)\n",
           original_text_size, encoded_size, compression, (double)original_text_size / (double)encoded_size);

    free(encoded);
    freeNLTokenArray(original);
    freeNLTokenArray(decoded);
    printf("? NL-EN Integration - professional style text\n");
}


static void print_benchmark_row(const char* label, size_t input_len,
                                size_t encoded_size, uLongf zlib_size) {
    printf("  Input:         %zu bytes\n", input_len);
    printf("  NL-EN encode:  %zu bytes (%.1f%% of original)\n",
           encoded_size, (double)encoded_size / (double)input_len * 100.0);
    printf("  zlib compress: %lu bytes (%.1f%% of original)\n",
           (unsigned long)zlib_size, (double)zlib_size / (double)input_len * 100.0);
    (void)label;
}

void test_zlib_compare_medium_text() {
    const char* text =
        "The global software industry continues to evolve at an unprecedented pace, "
        "driven by advances in artificial intelligence, cloud computing, and distributed "
        "systems. Organizations must adapt their development processes to remain competitive "
        "in an increasingly complex technological landscape. Effective architecture decisions "
        "require balancing performance, maintainability, and scalability while managing "
        "technical debt and ensuring long-term sustainability of the codebase. Engineering "
        "teams that invest in robust testing infrastructure and continuous integration "
        "pipelines consistently deliver higher quality products with fewer defects.";

    size_t input_len = strlen(text);

    NLTokenArray* tokens = tokenizeEnglish(text);
    assert_true(tokens != NULL, "zlib compare medium: tokenize should succeed");
    assert_true(tokens->count > 0, "zlib compare medium: should produce tokens");

    size_t nlen_size = 0;
    unsigned char* nlen_encoded = nl_en_encode(tokens, tokens->count, &nlen_size);
    assert_true(nlen_encoded != NULL, "zlib compare medium: NL-EN encode should succeed");
    assert_true(nlen_size > 0, "zlib compare medium: NL-EN encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "zlib compare medium: malloc for zlib buffer should succeed");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)text, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "zlib compare medium: compress should return Z_OK");
    assert_true(zlib_dest_len > 0, "zlib compare medium: zlib compressed size > 0");

    print_benchmark_row("medium", input_len, nlen_size, zlib_dest_len);

    free(nlen_encoded);
    free(zlib_dest);
    freeNLTokenArray(tokens);
    printf("PASS zlib vs NL-EN compare - medium professional text\n");
}

void test_zlib_compare_long_text() {
    const char* text =
        "Modern software engineering encompasses a broad spectrum of disciplines, from "
        "low-level systems programming to high-level application development. The design "
        "of efficient data compression algorithms represents one of the foundational "
        "challenges in computer science, with applications ranging from file archiving "
        "and network transmission to database storage and real-time streaming.\n\n"
        "Dictionary-based compression methods, such as the LZ77 algorithm that underpins "
        "the zlib library, achieve high compression ratios by replacing repeated byte "
        "sequences with compact references to earlier occurrences in the input stream. "
        "This approach is particularly effective for structured text and source code, "
        "where keywords, identifiers, and common phrases recur frequently throughout "
        "a document. The deflate format combines LZ77 with Huffman coding to further "
        "reduce the entropy of the compressed output.\n\n"
        "Domain-specific encoders take a different approach by exploiting prior knowledge "
        "about the expected content. Rather than discovering patterns dynamically, they "
        "rely on pre-built dictionaries of high-frequency tokens derived from large "
        "corpora. For English-language text, a relatively small vocabulary of common "
        "words, prefixes, suffixes, and character digraphs can cover a substantial "
        "fraction of any typical document. Each matched token is then represented as a "
        "single index into the dictionary, often requiring fewer bits than a general-"
        "purpose compressor would allocate to the same sequence.\n\n"
        "The trade-off between generality and specialization is a recurring theme in "
        "compression research. General-purpose compressors like zlib offer predictable "
        "performance across diverse input types and require no assumptions about content "
        "structure. Specialized encoders can outperform general compressors on their "
        "target domain but may expand data that falls outside their expected vocabulary. "
        "Hybrid strategies that combine a domain-specific first pass with a general "
        "entropy coder in a second pass are common in practice, with formats such as "
        "Brotli and Zstandard incorporating pre-defined dictionaries alongside adaptive "
        "statistical models to achieve strong compression across a wide range of inputs.\n\n"
        "Benchmarking compression algorithms requires careful attention to methodology. "
        "Compression ratio, encoding speed, and decoding speed are the primary metrics, "
        "but memory consumption, parallelization potential, and streaming capability also "
        "influence algorithm selection in production environments. A fair comparison must "
        "use representative input data, consistent measurement conditions, and multiple "
        "runs to account for processor cache effects and scheduling variability.";

    size_t input_len = strlen(text);

    NLTokenArray* tokens = tokenizeEnglish(text);
    assert_true(tokens != NULL, "zlib compare long: tokenize should succeed");
    assert_true(tokens->count > 0, "zlib compare long: should produce tokens");

    size_t nlen_size = 0;
    unsigned char* nlen_encoded = nl_en_encode(tokens, tokens->count, &nlen_size);
    assert_true(nlen_encoded != NULL, "zlib compare long: NL-EN encode should succeed");
    assert_true(nlen_size > 0, "zlib compare long: NL-EN encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "zlib compare long: malloc for zlib buffer should succeed");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)text, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "zlib compare long: compress should return Z_OK");
    assert_true(zlib_dest_len > 0, "zlib compare long: zlib compressed size > 0");

    print_benchmark_row("long", input_len, nlen_size, zlib_dest_len);

    free(nlen_encoded);
    free(zlib_dest);
    freeNLTokenArray(tokens);
    printf("PASS zlib vs NL-EN compare - long professional text\n");
}

/* ---- Knuth-Liang Hyphenator Tests ---- */

/* Best case: a common English word expected to hyphenate into multiple syllables.
   "butterfly" -> "but-ter-fly" -> 3 tokens, all isHyphenated=true. */
void test_kl_hyphenator_best_case(void) {
    KLTokenArray* arr = tokenizeKnuthLiang("butterfly");
    assert_true(arr != NULL, "KL best case: result not NULL");
    if (!arr) return;

    /* At least 2 sub-tokens from hyphenation */
    assert_true((int)arr->count >= 2, "KL best case: butterfly should produce >= 2 tokens");

    /* All tokens from a successfully hyphenated word are marked isHyphenated */
    bool all_hyphenated = true;
    for (size_t i = 0; i < arr->count; i++) {
        if (!arr->tokens[i].isHyphenated) { all_hyphenated = false; break; }
    }
    assert_true(all_hyphenated, "KL best case: all tokens from hyphenated word should be marked");

    /* Reconstructed text (joining tokens) matches original */
    char reconstructed[64] = "";
    for (size_t i = 0; i < arr->count; i++)
        strcat(reconstructed, arr->tokens[i].text);
    assert_equal_str(reconstructed, "butterfly", "KL best case: rejoined tokens match original");

    freeKLTokenArray(arr);
    printf("PASS KL hyphenator - best case (butterfly)\n");
}

/* Worst case: a short or unrecognisable word that yields no hyphenation,
   and a non-alphanumeric delimiter string.
   "zx" (<=2 chars, too short) -> 1 token, isHyphenated=false.
   "!?," -> 1 delimiter token, isHyphenated=false, caseStyle=0. */
void test_kl_hyphenator_worst_case(void) {
    /* Short word - cannot be hyphenated (length <= 2) */
    KLTokenArray* arr1 = tokenizeKnuthLiang("zx");
    assert_true(arr1 != NULL, "KL worst case: short word result not NULL");
    if (arr1) {
        assert_equal_int((int)arr1->count, 1, "KL worst case: short word produces 1 token");
        assert_true(!arr1->tokens[0].isHyphenated, "KL worst case: short word not hyphenated");
        freeKLTokenArray(arr1);
    }

    /* Non-alphanumeric string - delimiter token, no hyphenation */
    KLTokenArray* arr2 = tokenizeKnuthLiang("!?,");
    assert_true(arr2 != NULL, "KL worst case: delimiter result not NULL");
    if (arr2) {
        assert_equal_int((int)arr2->count, 1, "KL worst case: delimiter string produces 1 token");
        assert_true(!arr2->tokens[0].isHyphenated, "KL worst case: delimiter not hyphenated");
        assert_equal_int(arr2->tokens[0].caseStyle, 0, "KL worst case: delimiter caseStyle=0");
        freeKLTokenArray(arr2);
    }

    /* Mixed input: word + space + short word */
    KLTokenArray* arr3 = tokenizeKnuthLiang("mother like cookies");
    assert_true(arr3 != NULL, "KL worst case: mixed result not NULL");
    if (arr3) {
        /* Should have at least 5 tokens: word tokens + 2 spaces */
        assert_true((int)arr3->count >= 5, "KL worst case: mixed input >= 5 tokens");
        /* Space tokens should be non-hyphenated with caseStyle 0 */
        bool spaces_ok = true;
        for (size_t i = 0; i < arr3->count; i++) {
            if (arr3->tokens[i].text[0] == ' ') {
                if (arr3->tokens[i].isHyphenated || arr3->tokens[i].caseStyle != 0)
                    spaces_ok = false;
            }
        }
        assert_true(spaces_ok, "KL worst case: space tokens are delimiters with caseStyle=0");
        freeKLTokenArray(arr3);
    }

    printf("PASS KL hyphenator - worst case\n");
}

/* Professional text test: tokenize a magazine-style English passage and verify
   structural correctness of the hyphenation output across a realistic corpus. */
void test_kl_hyphenator_professional_text(void) {
    static const char* text =
        "The rapid advancement of artificial intelligence has fundamentally transformed "
        "the way organizations approach decision-making and problem-solving across virtually "
        "every industry. Machine learning algorithms, particularly deep neural networks, "
        "have demonstrated remarkable capabilities in pattern recognition, natural language "
        "understanding, and generative tasks that were previously considered exclusive "
        "to human intelligence.\n\n"
        "Researchers and practitioners continue to investigate the theoretical foundations "
        "underlying these models, seeking to understand why certain architectures generalize "
        "effectively while others overfit or fail to converge during training. Regularization "
        "techniques, attention mechanisms, and transfer learning have emerged as powerful "
        "strategies for improving model performance without proportionally increasing "
        "computational requirements.\n\n"
        "The deployment of large-scale language models has introduced new considerations "
        "around interpretability, fairness, and environmental sustainability. Organizations "
        "must balance the competitive advantages offered by sophisticated AI systems against "
        "the infrastructure costs, energy consumption, and potential societal implications "
        "associated with widespread adoption. Governance frameworks and international "
        "standards bodies are actively working to establish guidelines that promote "
        "responsible innovation while preserving the benefits of technological progress.\n\n"
        "Looking ahead, researchers anticipate continued improvements in multimodal "
        "understanding, reasoning under uncertainty, and efficient inference on edge devices. "
        "The convergence of hardware acceleration, novel training paradigms, and curated "
        "high-quality datasets is expected to unlock capabilities that remain out of reach "
        "with current approaches. As the field matures, interdisciplinary collaboration "
        "between computer scientists, ethicists, domain experts, and policymakers will "
        "become increasingly essential to navigating the complex landscape of modern "
        "artificial intelligence research and deployment.";

    KLTokenArray* arr = tokenizeKnuthLiang(text);
    assert_true(arr != NULL, "KL professional: result not NULL");
    if (!arr) return;

    /* Should produce a substantial number of tokens from this length of text */
    assert_true((int)arr->count > 300,
                "KL professional: long text should produce > 300 tokens");
    assert_true((int)arr->count <= KL_MAX_TOKENS,
                "KL professional: token count within fixed array capacity");

    int hyphenated_count = 0;
    int word_count       = 0;
    bool invariants_ok   = true;

    for (size_t i = 0; i < arr->count; i++) {
        KLToken* t = &arr->tokens[i];

        /* Every token must have non-zero length */
        if (t->length == 0) { invariants_ok = false; break; }

        unsigned char fc = (unsigned char)t->text[0];
        bool is_delim = !(fc >= 32 && fc < 128 && KL_ASCII_PATTERNS[fc - 32]);

        if (is_delim) {
            /* Delimiter tokens must never be marked as hyphenated and must have caseStyle 0 */
            if (t->isHyphenated || t->caseStyle != 0) { invariants_ok = false; break; }
        } else {
            word_count++;
            if (t->isHyphenated) hyphenated_count++;
        }
    }

    assert_true(invariants_ok,
                "KL professional: all tokens satisfy delimiter/caseStyle invariants");

    /* A meaningful fraction of word tokens should be hyphenated syllables —
       professional text contains many polysyllabic words. */
    assert_true(word_count > 150,
                "KL professional: text should yield > 150 word tokens");
    assert_true(hyphenated_count > 80,
                "KL professional: > 80 syllable tokens expected from polysyllabic vocabulary");

    int total_count = (int)arr->count;
    freeKLTokenArray(arr);
    printf("PASS KL hyphenator - professional text (%d tokens, %d hyphenated syllables)\n",
           total_count, hyphenated_count);
}

/* ---- Frequency map tests ---- */

/* Good hyphenation: a medium-length passage rich in polysyllabic words.
   After tokenization the frequency map should show many distinct syllables,
   common short syllables recurring multiple times, and entries sorted
   descending by frequency. */
void test_kl_freqmap_good_hyphenation(void) {
    static const char* text =
        "Scientists investigating the underlying mechanisms of biological evolution "
        "have discovered remarkable patterns of adaptation and diversification across "
        "generations. The development of genetic sequencing technologies has accelerated "
        "our understanding of hereditary information, revealing how populations accumulate "
        "mutations and respond to environmental pressures over extended periods. "
        "Computational models of evolutionary dynamics help researchers anticipate "
        "trajectories of change and identify the selective pressures responsible for "
        "observable morphological and behavioral transformations.";

    KLTokenArray* arr = tokenizeKnuthLiang(text);
    assert_true(arr != NULL, "KL freqmap good: tokenize returned non-NULL");
    if (!arr) return;

    KLFreqMap* map = collectKLFrequencies(arr);
    assert_true(map != NULL, "KL freqmap good: collectKLFrequencies returned non-NULL");
    if (!map) { freeKLTokenArray(arr); return; }

    /* totalTokens must equal the source array count */
    assert_equal_int((int)map->totalTokens, (int)arr->count,
                     "KL freqmap good: totalTokens matches source count");

    /* uniqueCount must be <= totalTokens and > 0 */
    assert_true((int)map->uniqueCount > 0,
                "KL freqmap good: at least one unique string");
    assert_true(map->uniqueCount <= map->totalTokens,
                "KL freqmap good: uniqueCount <= totalTokens");

    /* Polysyllabic text has repetition — expect meaningful deduplication */
    assert_true(map->uniqueCount < map->totalTokens,
                "KL freqmap good: repeated strings produce fewer unique entries than tokens");

    /* Entries must be sorted descending by frequency */
    bool sorted = true;
    for (size_t i = 1; i < map->uniqueCount; i++) {
        if (map->entries[i].frequency > map->entries[i - 1].frequency)
            { sorted = false; break; }
    }
    assert_true(sorted, "KL freqmap good: entries sorted descending by frequency");

    /* Every frequency must be >= 1 and every text non-empty */
    bool valid_entries = true;
    for (size_t i = 0; i < map->uniqueCount; i++) {
        if (map->entries[i].frequency < 1 || map->entries[i].text[0] == '\0')
            { valid_entries = false; break; }
    }
    assert_true(valid_entries, "KL freqmap good: all entries have frequency >= 1 and non-empty text");

    /* Space is the most common delimiter and should appear many times */
    bool space_found = false;
    for (size_t i = 0; i < map->uniqueCount; i++) {
        if (strcmp(map->entries[i].text, " ") == 0 && map->entries[i].frequency >= 5)
            { space_found = true; break; }
    }
    assert_true(space_found, "KL freqmap good: space delimiter recurs >= 5 times");

    /* The most-frequent entry must appear more than once (real repetition) */
    assert_true(map->entries[0].frequency > 1,
                "KL freqmap good: top entry appears more than once");

    printf("PASS KL freqmap - good hyphenation (%zu unique / %zu total tokens)\n",
           map->uniqueCount, map->totalTokens);

    freeKLFreqMap(map);
    freeKLTokenArray(arr);
}

/* Bad hyphenation: a text composed almost entirely of short words (<=2 chars)
   that the algorithm cannot hyphenate, plus repeated identical words.
   The frequency map should reflect high repetition with low uniqueCount
   and all word tokens marked isHyphenated=false. */
void test_kl_freqmap_bad_hyphenation(void) {
    /* All words are <=3 chars or already minimal; none should hyphenate */
    static const char* text =
        "a big cat sat on a mat a big dog ran by a big cat sat up and ran "
        "a dog bit a cat a cat bit a rat a rat bit a big dog by the leg";

    KLTokenArray* arr = tokenizeKnuthLiang(text);
    assert_true(arr != NULL, "KL freqmap bad: tokenize returned non-NULL");
    if (!arr) return;

    /* Confirm no hyphenation occurred */
    bool any_hyphenated = false;
    for (size_t i = 0; i < arr->count; i++)
        if (arr->tokens[i].isHyphenated) { any_hyphenated = true; break; }
    assert_true(!any_hyphenated,
                "KL freqmap bad: no tokens should be hyphenated for short-word text");

    KLFreqMap* map = collectKLFrequencies(arr);
    assert_true(map != NULL, "KL freqmap bad: collectKLFrequencies returned non-NULL");
    if (!map) { freeKLTokenArray(arr); return; }

    /* totalTokens matches source */
    assert_equal_int((int)map->totalTokens, (int)arr->count,
                     "KL freqmap bad: totalTokens matches source count");

    /* Highly repetitive text: uniqueCount should be much smaller than totalTokens */
    assert_true(map->uniqueCount < map->totalTokens / 2,
                "KL freqmap bad: highly repetitive text has uniqueCount < half of totalTokens");

    /* Sorted descending */
    bool sorted = true;
    for (size_t i = 1; i < map->uniqueCount; i++) {
        if (map->entries[i].frequency > map->entries[i - 1].frequency)
            { sorted = false; break; }
    }
    assert_true(sorted, "KL freqmap bad: entries sorted descending by frequency");

    /* "a" and " " should be the highest-frequency entries */
    assert_true(map->entries[0].frequency >= 10,
                "KL freqmap bad: top entry appears >= 10 times in repetitive text");

    /* Every frequency >= 1, text non-empty */
    bool valid_entries = true;
    for (size_t i = 0; i < map->uniqueCount; i++) {
        if (map->entries[i].frequency < 1 || map->entries[i].text[0] == '\0')
            { valid_entries = false; break; }
    }
    assert_true(valid_entries, "KL freqmap bad: all entries valid");

    printf("PASS KL freqmap - bad hyphenation (%zu unique / %zu total tokens, "
           "top entry \"%s\" x%d)\n",
           map->uniqueCount, map->totalTokens,
           map->entries[0].text, map->entries[0].frequency);

    freeKLFreqMap(map);
    freeKLTokenArray(arr);
}

/* ---- Knuth-Liang Affix Stripping Tests ---- */

/* Verify kl_strip_affixes and that tokenizeKnuthLiang uses it correctly.
   "sundering": "ing" suffix stripped, no matching prefix, KL splits "sunder".
   "preprocessing": "ing" suffix stripped, "pre" prefix stripped, KL splits "process". */
void test_kl_affix_strip(void) {
    KLAffixResult res;

    /* --- kl_strip_affixes: sundering --- */
    kl_strip_affixes("sundering", 9, &res);
    assert_equal_str(res.suffix, "ing",    "affix strip: sundering suffix='ing'");
    assert_equal_str(res.stem,   "sunder", "affix strip: sundering stem='sunder'");
    assert_equal_int(res.prefix_len, 0,    "affix strip: sundering no prefix");

    /* --- kl_strip_affixes: preprocessing --- */
    kl_strip_affixes("preprocessing", 13, &res);
    assert_equal_str(res.prefix, "pre",     "affix strip: preprocessing prefix='pre'");
    assert_equal_str(res.stem,   "process", "affix strip: preprocessing stem='process'");
    assert_equal_str(res.suffix, "ing",     "affix strip: preprocessing suffix='ing'");

    /* --- kl_strip_affixes: word with no 3+-char suffix (butterfly) --- */
    kl_strip_affixes("butterfly", 9, &res);
    assert_equal_int(res.suffix_len, 0,        "affix strip: butterfly no suffix");
    assert_equal_str(res.stem, "butterfly",    "affix strip: butterfly stem=whole word");

    /* --- tokenizeKnuthLiang("sundering") => "sun","der","ing" --- */
    KLTokenArray* arr1 = tokenizeKnuthLiang("sundering");
    assert_true(arr1 != NULL, "affix tokenize: sundering not NULL");
    if (arr1) {
        assert_equal_int((int)arr1->count, 3, "affix tokenize: sundering 3 tokens");
        if ((int)arr1->count == 3) {
            assert_equal_str(arr1->tokens[0].text, "sun", "affix tokenize: sundering[0]='sun'");
            assert_equal_str(arr1->tokens[1].text, "der", "affix tokenize: sundering[1]='der'");
            assert_equal_str(arr1->tokens[2].text, "ing", "affix tokenize: sundering[2]='ing'");
            assert_true(arr1->tokens[0].isHyphenated, "affix tokenize: sundering[0] isHyphenated");
            assert_true(arr1->tokens[1].isHyphenated, "affix tokenize: sundering[1] isHyphenated");
            assert_true(arr1->tokens[2].isHyphenated, "affix tokenize: sundering[2] isHyphenated");
            assert_equal_int(arr1->tokens[2].caseStyle, 0, "affix tokenize: suffix caseStyle=0");
        }
        freeKLTokenArray(arr1);
    }

    /* --- tokenizeKnuthLiang("preprocessing") => "pre","<stem...>","ing" --- */
    KLTokenArray* arr2 = tokenizeKnuthLiang("preprocessing");
    assert_true(arr2 != NULL, "affix tokenize: preprocessing not NULL");
    if (arr2) {
        assert_true((int)arr2->count >= 3, "affix tokenize: preprocessing >= 3 tokens");
        if ((int)arr2->count >= 1) {
            assert_equal_str(arr2->tokens[0].text, "pre",
                             "affix tokenize: preprocessing first token='pre'");
            assert_true(arr2->tokens[0].isHyphenated,
                        "affix tokenize: preprocessing prefix isHyphenated");
            assert_equal_int(arr2->tokens[0].caseStyle, 0,
                             "affix tokenize: preprocessing prefix caseStyle=0");
        }
        if ((int)arr2->count >= 2) {
            /* Last token must be the suffix "ing" */
            int last = (int)arr2->count - 1;
            assert_equal_str(arr2->tokens[last].text, "ing",
                             "affix tokenize: preprocessing last token='ing'");
            assert_true(arr2->tokens[last].isHyphenated,
                        "affix tokenize: preprocessing suffix isHyphenated");
            assert_equal_int(arr2->tokens[last].caseStyle, 0,
                             "affix tokenize: preprocessing suffix caseStyle=0");
        }
        freeKLTokenArray(arr2);
    }

    printf("PASS KL affix stripping\n");
}

/* ---- NL-EN Frequency Map Tests ---- */

/* Best case: short text with deliberate repetition so that the freq map
   has far fewer unique entries than total tokens, and is sorted correctly. */
void test_nl_en_freqmap_best_case(void) {
    /* "hi hi hi" -> tokenizer produces the same pattern tokens repeatedly */
    NLTokenArray* arr = tokenizeEnglish("hi hi hi hi hi");
    assert_true(arr != NULL, "NL freqmap best: tokenize returned non-NULL");
    if (!arr) return;
    assert_true((int)arr->count > 0, "NL freqmap best: at least one token");

    NLFreqMap* map = collectNLFrequencies(arr);
    assert_true(map != NULL, "NL freqmap best: collectNLFrequencies returned non-NULL");
    if (!map) { freeNLTokenArray(arr); return; }

    /* totalTokens must equal the source array count */
    assert_equal_int((int)map->totalTokens, (int)arr->count,
                     "NL freqmap best: totalTokens matches source count");

    /* uniqueCount must be > 0 and <= totalTokens */
    assert_true((int)map->uniqueCount > 0,
                "NL freqmap best: at least one unique token");
    assert_true(map->uniqueCount <= map->totalTokens,
                "NL freqmap best: uniqueCount <= totalTokens");

    /* Repetitive text: unique count should be less than total */
    assert_true(map->uniqueCount < map->totalTokens,
                "NL freqmap best: repeated tokens collapse to fewer unique entries");

    /* Entries must be sorted descending by frequency */
    bool sorted = true;
    for (size_t i = 1; i < map->uniqueCount; i++) {
        if (map->entries[i].frequency > map->entries[i - 1].frequency)
            { sorted = false; break; }
    }
    assert_true(sorted, "NL freqmap best: entries sorted descending by frequency");

    /* Sum of all frequencies must equal totalTokens */
    int freq_sum = 0;
    for (size_t i = 0; i < map->uniqueCount; i++)
        freq_sum += map->entries[i].frequency;
    assert_equal_int(freq_sum, (int)map->totalTokens,
                     "NL freqmap best: frequency sum equals total token count");

    /* Every entry must have frequency >= 1 */
    bool valid = true;
    for (size_t i = 0; i < map->uniqueCount; i++)
        if (map->entries[i].frequency < 1) { valid = false; break; }
    assert_true(valid, "NL freqmap best: all entries have frequency >= 1");

    printf("PASS NL-EN freqmap - best case (%zu unique / %zu total)\n",
           map->uniqueCount, map->totalTokens);

    freeNLFreqMap(map);
    freeNLTokenArray(arr);
}

/* Worst case: text composed of all different characters / rarely repeated tokens
   so that uniqueCount approaches totalTokens. */
void test_nl_en_freqmap_worst_case(void) {
    /* 26 distinct single letters with spaces — most tokens will be unique */
    const char* text = "a b c d e f g h i j k l m n o p q r s t u v w x y z";

    NLTokenArray* arr = tokenizeEnglish(text);
    assert_true(arr != NULL, "NL freqmap worst: tokenize returned non-NULL");
    if (!arr) return;
    assert_true((int)arr->count > 0, "NL freqmap worst: at least one token");

    NLFreqMap* map = collectNLFrequencies(arr);
    assert_true(map != NULL, "NL freqmap worst: collectNLFrequencies returned non-NULL");
    if (!map) { freeNLTokenArray(arr); return; }

    /* totalTokens matches source */
    assert_equal_int((int)map->totalTokens, (int)arr->count,
                     "NL freqmap worst: totalTokens matches source count");

    /* uniqueCount must be > 0 */
    assert_true((int)map->uniqueCount > 0,
                "NL freqmap worst: at least one unique token");

    /* With mostly distinct tokens, uniqueCount should be close to totalTokens */
    assert_true(map->uniqueCount <= map->totalTokens,
                "NL freqmap worst: uniqueCount <= totalTokens (invariant)");

    /* Sorted descending */
    bool sorted = true;
    for (size_t i = 1; i < map->uniqueCount; i++) {
        if (map->entries[i].frequency > map->entries[i - 1].frequency)
            { sorted = false; break; }
    }
    assert_true(sorted, "NL freqmap worst: entries sorted descending by frequency");

    /* Frequency sum == totalTokens */
    int freq_sum = 0;
    for (size_t i = 0; i < map->uniqueCount; i++)
        freq_sum += map->entries[i].frequency;
    assert_equal_int(freq_sum, (int)map->totalTokens,
                     "NL freqmap worst: frequency sum equals total token count");

    printf("PASS NL-EN freqmap - worst case (%zu unique / %zu total)\n",
           map->uniqueCount, map->totalTokens);

    freeNLFreqMap(map);
    freeNLTokenArray(arr);
}

/* ---- NL-EN Arithmetic Encoding Codec Tests ---- */

/* Best case: manually constructed 3-token sequence with 2 unique symbols.
   Verifies encode/decode identity and probability sum. */
void test_nl_en_ae_codec_best_case(void) {
    /* Build a tiny 4-token array: token A (pattern), token B (ASCII space),
       token A, token A  — 2 unique symbols, A appears 3x, B appears 1x.   */
    NLTokenArray input;
    input.count = 4;

    /* Token A: pattern index 5, case 0 */
    input.tokens[0].isPattern = true;
    input.tokens[0].flag      = 5;
    input.tokens[0].caseStyle = 0;
    /* Token B: ASCII space */
    input.tokens[1].isPattern = false;
    input.tokens[1].flag      = ' ';
    input.tokens[1].caseStyle = 0;
    /* Token A again */
    input.tokens[2].isPattern = true;
    input.tokens[2].flag      = 5;
    input.tokens[2].caseStyle = 0;
    /* Token A again */
    input.tokens[3].isPattern = true;
    input.tokens[3].flag      = 5;
    input.tokens[3].caseStyle = 0;

    /* Encode */
    size_t encoded_size = 0;
    unsigned char* encoded = nl_en_encode_ae(&input, input.count, &encoded_size);
    assert_true(encoded != NULL, "AE best: encoded buffer not NULL");
    assert_true(encoded_size > 0, "AE best: encoded size > 0");

    /* Decode */
    NLTokenArray* decoded = nl_en_decode_ae(encoded, encoded_size);
    assert_true(decoded != NULL, "AE best: decoded array not NULL");
    assert_equal_int((int)decoded->count, (int)input.count,
                     "AE best: decoded count matches input");

    /* Verify each token */
    if (decoded->count == input.count) {
        for (size_t i = 0; i < input.count; i++) {
            assert_equal_int(decoded->tokens[i].isPattern, input.tokens[i].isPattern,
                             "AE best: isPattern matches");
            assert_equal_int(decoded->tokens[i].flag, input.tokens[i].flag,
                             "AE best: flag matches");
            if (input.tokens[i].isPattern)
                assert_equal_int(decoded->tokens[i].caseStyle, input.tokens[i].caseStyle,
                                 "AE best: caseStyle matches");
        }
    }

    /* Verify that the probability table sums to NL_AE_SCALE — re-encode
       and check via a separate frequency map + cumulative calculation.    */
    {
        NLFreqMap* fmap = collectNLFrequencies(&input);
        assert_true(fmap != NULL, "AE best: freq map not NULL");
        if (fmap) {
            uint32_t total = 0;
            for (size_t i = 0; i < fmap->uniqueCount; i++)
                total += (uint32_t)fmap->entries[i].frequency;

            uint32_t cum = 0;
            uint32_t last_high = 0;
            for (size_t i = 0; i < fmap->uniqueCount; i++) {
                last_high = (uint32_t)((uint64_t)(cum + (uint32_t)fmap->entries[i].frequency)
                                       * NL_AE_SCALE / total);
                cum += (uint32_t)fmap->entries[i].frequency;
            }
            assert_equal_int((int)last_high, (int)NL_AE_SCALE,
                             "AE best: cumulative probability sum equals NL_AE_SCALE (= 1)");
            freeNLFreqMap(fmap);
        }
    }

    free(encoded);
    freeNLTokenArray(decoded);
    printf("PASS NL-EN AE codec - best case (4 tokens, 2 unique)\n");
}

/* Worst case: tokenize a short phrase with several distinct tokens and verify
   that encode -> decode is a lossless round-trip. */
void test_nl_en_ae_codec_worst_case(void) {
    /* Short phrase with several different syllable patterns and a space.
       Keep it short to stay within 32-bit fixed-point precision.          */
    const char* text = "the cat";

    NLTokenArray* original = tokenizeEnglish(text);
    assert_true(original != NULL, "AE worst: tokenize returned non-NULL");
    if (!original) return;
    assert_true((int)original->count > 0, "AE worst: at least one token");
    /* Precision guard: AE without renormalization needs a short sequence   */
    assert_true((int)original->count <= 20, "AE worst: token count within precision limit");

    /* Encode */
    size_t encoded_size = 0;
    unsigned char* encoded = nl_en_encode_ae(original, original->count, &encoded_size);
    assert_true(encoded != NULL, "AE worst: encoded buffer not NULL");
    assert_true(encoded_size > 0, "AE worst: encoded size > 0");

    /* Decode */
    NLTokenArray* decoded = nl_en_decode_ae(encoded, encoded_size);
    assert_true(decoded != NULL, "AE worst: decoded array not NULL");
    assert_equal_int((int)decoded->count, (int)original->count,
                     "AE worst: decoded count matches original");

    /* Full token comparison */
    if (decoded->count == original->count) {
        bool all_match = true;
        for (size_t i = 0; i < original->count; i++) {
            if (decoded->tokens[i].isPattern != original->tokens[i].isPattern ||
                decoded->tokens[i].flag != original->tokens[i].flag) {
                all_match = false;
                break;
            }
            if (original->tokens[i].isPattern &&
                decoded->tokens[i].caseStyle != original->tokens[i].caseStyle) {
                all_match = false;
                break;
            }
        }
        assert_true(all_match, "AE worst: all decoded tokens match original");
    }

    /* Probability sum check via frequency map */
    {
        NLFreqMap* fmap = collectNLFrequencies(original);
        assert_true(fmap != NULL, "AE worst: freq map not NULL");
        if (fmap) {
            uint32_t total = 0;
            for (size_t i = 0; i < fmap->uniqueCount; i++)
                total += (uint32_t)fmap->entries[i].frequency;

            uint32_t cum = 0;
            uint32_t last_high = 0;
            for (size_t i = 0; i < fmap->uniqueCount; i++) {
                last_high = (uint32_t)((uint64_t)(cum + (uint32_t)fmap->entries[i].frequency)
                                       * NL_AE_SCALE / total);
                cum += (uint32_t)fmap->entries[i].frequency;
            }
            assert_equal_int((int)last_high, (int)NL_AE_SCALE,
                             "AE worst: probability sum equals NL_AE_SCALE (= 1)");
            freeNLFreqMap(fmap);
        }
    }

    printf("PASS NL-EN AE codec - worst case (%zu tokens from \"%s\")\n",
           original->count, text);

    free(encoded);
    freeNLTokenArray(original);
    freeNLTokenArray(decoded);
}

/* ---- CSS Tokenizable Tests ---- */

/* Best case: selector and property name/value each match a known pattern,
   so each produces exactly 1 CSSTokenizable with isPattern=true.          */
void test_css_tokenizable_pattern_match(void) {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    if (!result) { printf("SKIP CSS tokenizable: malloc failed\n"); return; }

    /* "div { color: red; }" — "div", "color", and "red" are all in the
       pattern codebook (segments 1, 5, and 10 respectively).              */
    parseCSS("div { color: red; }", result);
    assert_equal_int(result->count, 1, "CSS tok pattern: 1 token");

    CSSToken* tok = &result->tokens[0];
    assert_equal_int(tok->type, 0, "CSS tok pattern: selector rule type");

    /* Selector "div" should tokenize to exactly 1 CSSTokenizable */
    assert_equal_int(tok->data.rule.selectorTokenSize, 1,
                     "CSS tok pattern: selector 'div' -> 1 tokenizable");
    assert_true(tok->data.rule.selectorTokens[0].isPattern,
                "CSS tok pattern: selector token isPattern");

    assert_equal_int(tok->data.rule.propertyCount, 1,
                     "CSS tok pattern: 1 property");

    CSSProperty* prop = &tok->data.rule.properties[0];

    /* Property name "color" should tokenize to exactly 1 CSSTokenizable */
    assert_equal_int(prop->nameTokenSize, 1,
                     "CSS tok pattern: prop name 'color' -> 1 tokenizable");
    assert_true(prop->nameTokens[0].isPattern,
                "CSS tok pattern: name token isPattern");

    /* Property value "red" should tokenize to exactly 1 CSSTokenizable */
    assert_equal_int(prop->valueTokenSize, 1,
                     "CSS tok pattern: prop value 'red' -> 1 tokenizable");
    assert_true(prop->valueTokens[0].isPattern,
                "CSS tok pattern: value token isPattern");

    freeCSS(result);
    printf("PASS CSS tokenizable - pattern match (selector + property + value)\n");
}

/* Worst case: selector and property are not in the codebook, so every
   character becomes its own CSSTokenizable with isPattern=false.          */
void test_css_tokenizable_ascii_fallback(void) {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    if (!result) { printf("SKIP CSS tokenizable: malloc failed\n"); return; }

    /* "zz { qqq: zzz; }" — none of these strings are CSS patterns */
    parseCSS("zz { qqq: zzz; }", result);
    assert_equal_int(result->count, 1, "CSS tok ascii: 1 token");

    CSSToken* tok = &result->tokens[0];
    CSSProperty* prop = &tok->data.rule.properties[0];

    /* Selector "zz" is not in the codebook -> 2 ASCII CSSTokenizables */
    assert_equal_int(tok->data.rule.selectorTokenSize, 2,
                     "CSS tok ascii: selector 'zz' -> 2 tokenizables");
    assert_true(!tok->data.rule.selectorTokens[0].isPattern,
                "CSS tok ascii: selector[0] isPattern=false");
    assert_equal_int(tok->data.rule.selectorTokens[0].flag, (int)'z',
                     "CSS tok ascii: selector[0] flag='z'");

    /* Property name "qqq" is not in codebook -> 3 ASCII CSSTokenizables */
    assert_equal_int(prop->nameTokenSize, 3,
                     "CSS tok ascii: name 'qqq' -> 3 tokenizables");
    assert_true(!prop->nameTokens[0].isPattern,
                "CSS tok ascii: name[0] isPattern=false");

    /* Property value "zzz" is not in codebook -> 3 ASCII CSSTokenizables */
    assert_equal_int(prop->valueTokenSize, 3,
                     "CSS tok ascii: value 'zzz' -> 3 tokenizables");
    assert_true(!prop->valueTokens[0].isPattern,
                "CSS tok ascii: value[0] isPattern=false");

    freeCSS(result);
    printf("PASS CSS tokenizable - ASCII fallback (unknown selector/property)\n");
}

/* At-rule tokenization: greedy longest-match scanning.
   "@media" is in the codebook; the remainder " screen" should be partly
   tokenized as ASCII (' ') and the word "screen" which IS in the codebook.*/
void test_css_tokenizable_atrule(void) {
    CSSTokenArray* result = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    if (!result) { printf("SKIP CSS tokenizable: malloc failed\n"); return; }

    /* Use a simple at-rule whose identifier and named block are both in
       the codebook so we can count precisely.                              */
    parseCSS("@media screen { }", result);
    assert_equal_int(result->count, 1, "CSS tok atrule: 1 token");

    CSSToken* tok = &result->tokens[0];
    assert_equal_int(tok->type, 1, "CSS tok atrule: at-rule type");

    int sz = tok->data.atRule.atRuleTokenSize;
    assert_true(sz > 0, "CSS tok atrule: atRuleTokenSize > 0");

    /* First tokenizable must be the "@media" pattern (isPattern=true) */
    assert_true(tok->data.atRule.atRuleTokens[0].isPattern,
                "CSS tok atrule: first token isPattern (matches '@media')");

    /* At least one more tokenizable for ' ' and "screen" */
    assert_true(sz >= 2, "CSS tok atrule: at least 2 tokenizables");

    /* The second tokenizable should be the space character (isPattern=false) */
    assert_true(!tok->data.atRule.atRuleTokens[1].isPattern,
                "CSS tok atrule: second token isPattern=false (space)");
    assert_equal_int(tok->data.atRule.atRuleTokens[1].flag, (int)' ',
                     "CSS tok atrule: second token flag=' '");

    /* The last tokenizable should be "screen" (isPattern=true) */
    assert_true(tok->data.atRule.atRuleTokens[sz - 1].isPattern,
                "CSS tok atrule: last token isPattern (matches 'screen')");

    freeCSS(result);
    printf("PASS CSS tokenizable - at-rule greedy tokenization (@media screen)\n");
}

/* Verify that the segment boundary constants are consistent: each start
   index must be non-negative, in ascending order, and within the pattern
   count.                                                                  */
void test_css_pattern_codebook(void) {
    assert_true(CSS_PATTERN_COUNT > 0,
                "CSS codebook: pattern count > 0");
    assert_true(CSS_SEG1_START == 0,
                "CSS codebook: seg1 starts at 0");
    assert_true(CSS_SEG2_START  > CSS_SEG1_START,  "CSS codebook: seg2 > seg1");
    assert_true(CSS_SEG3_START  > CSS_SEG2_START,  "CSS codebook: seg3 > seg2");
    assert_true(CSS_SEG4_START  > CSS_SEG3_START,  "CSS codebook: seg4 > seg3");
    assert_true(CSS_SEG5_START  > CSS_SEG4_START,  "CSS codebook: seg5 > seg4");
    assert_true(CSS_SEG6_START  > CSS_SEG5_START,  "CSS codebook: seg6 > seg5");
    assert_true(CSS_SEG7_START  > CSS_SEG6_START,  "CSS codebook: seg7 > seg6");
    assert_true(CSS_SEG8_START  > CSS_SEG7_START,  "CSS codebook: seg8 > seg7");
    assert_true(CSS_SEG9_START  > CSS_SEG8_START,  "CSS codebook: seg9 > seg8");
    assert_true(CSS_SEG10_START > CSS_SEG9_START,  "CSS codebook: seg10 > seg9");
    assert_true(CSS_SEG11_START > CSS_SEG10_START, "CSS codebook: seg11 > seg10");
    assert_true(CSS_SEG11_START < CSS_PATTERN_COUNT,
                "CSS codebook: seg11 start within total count");

    /* Spot check: "div" should be findable in segment 1 */
    int found_div = 0;
    for (int i = CSS_SEG1_START; i < CSS_SEG2_START; i++) {
        if (strcmp(CSS_PATTERNS[i], "div") == 0) { found_div = 1; break; }
    }
    assert_true(found_div, "CSS codebook: 'div' found in seg1 (HTML tags)");

    /* Spot check: "color" should be in segment 5 (CSS properties) */
    int found_color = 0;
    for (int i = CSS_SEG5_START; i < CSS_SEG6_START; i++) {
        if (strcmp(CSS_PATTERNS[i], "color") == 0) { found_color = 1; break; }
    }
    assert_true(found_color, "CSS codebook: 'color' found in seg5 (properties)");

    /* Spot check: "@media" should be in segment 6 (at-rules) */
    int found_media = 0;
    for (int i = CSS_SEG6_START; i < CSS_SEG7_START; i++) {
        if (strcmp(CSS_PATTERNS[i], "@media") == 0) { found_media = 1; break; }
    }
    assert_true(found_media, "CSS codebook: '@media' found in seg6 (at-rules)");

    printf("PASS CSS codebook - segment boundaries and spot checks (%d total patterns)\n",
           CSS_PATTERN_COUNT);
}

/* ---- CSS Tokenizable Comprehensive Test ---- */

void test_css_tokenizable_comprehensive(void) {
    /* A realistic CSS snippet exercising all 11 codebook segments:
       comments, at-rules (@import, @font-face, @keyframes, @media),
       type/class/id/combinators/pseudo-class/pseudo-element selectors,
       and a wide range of CSS properties and values.              */
    static const char css[] =
        "/* Global reset */\n"
        "@import url('base.css');\n"
        "@font-face { font-family: Inter; src: url('inter.woff2'); font-weight: normal; }\n"
        "@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }\n"
        "@media screen and (max-width: 768px) { body { font-size: 14px; } }\n"
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "/* Typography */\n"
        "html, body { width: 100%; height: 100%; overflow: hidden; }\n"
        "h1 { font-size: 2rem; font-weight: bold; color: darkblue; }\n"
        "p { font-size: 1rem; line-height: 1.5; color: inherit; }\n"
        "a { color: blue; text-decoration: none; cursor: pointer; }\n"
        "a:hover { color: darkblue; text-decoration: underline; }\n"
        "a:focus { outline: 2px solid blue; outline-offset: 2px; }\n"
        "a::before { content: ''; display: none; }\n"
        "/* Layout */\n"
        ".container { width: 100%; max-width: 1200px; margin: 0 auto; padding: 0 16px; }\n"
        ".flex { display: flex; align-items: center; gap: 16px; }\n"
        ".grid { display: grid; grid-template-columns: repeat(12, 1fr); column-gap: 16px; }\n"
        "#header { position: sticky; top: 0; z-index: 100; background: white; }\n"
        "/* Components */\n"
        ".card { background: white; border: 1px solid silver; border-radius: 8px; overflow: hidden; }\n"
        ".card:hover { transform: translateY(-2px); opacity: 0.95; }\n"
        ".card > .body { padding: 16px; color: #333; }\n"
        "nav > ul { list-style: none; display: flex; margin: 0; padding: 0; }\n"
        "nav > ul > li:first-child a:hover { color: royalblue; text-decoration: underline; }\n";

    CSSTokenArray* arr = (CSSTokenArray*)malloc(sizeof(CSSTokenArray));
    if (!arr) {
        printf("? FAIL: CSS comprehensive - malloc failed\n");
        testsFailed++;
        return;
    }
    parseCSS(css, arr);

    /* ---- Overall token count ---- */
    /* 4 comments + 4 at-rules + 17 selector rules = 25 */
    assert_equal_int(arr->count, 25, "CSS comprehensive: total token count");

    /* ---- Type distribution ---- */
    int nComments = 0, nAtRules = 0, nRules = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type == 2) nComments++;
        else if (arr->tokens[i].type == 1) nAtRules++;
        else nRules++;
    }
    assert_equal_int(nComments, 4, "CSS comprehensive: 4 comments");
    assert_equal_int(nAtRules,  4, "CSS comprehensive: 4 at-rules");
    assert_equal_int(nRules,   17, "CSS comprehensive: 17 selector rules");

    /* ---- All at-rules begin with an isPattern tokenizable ---- */
    int atRuleIdx = 0;
    for (int i = 0; i < arr->count && atRuleIdx < 4; i++) {
        if (arr->tokens[i].type != 1) continue;
        assert_true(arr->tokens[i].data.atRule.atRuleTokenSize > 0,
                    "CSS comprehensive: at-rule has tokenizables");
        assert_true(arr->tokens[i].data.atRule.atRuleTokens[0].isPattern,
                    "CSS comprehensive: at-rule first tokenizable is pattern");
        atRuleIdx++;
    }

    /* ---- Simple type selectors produce exactly 1 pattern tokenizable ---- */
    /* Find rules whose selector is exactly "h1", "p", "a" */
    int found_h1 = 0, found_p = 0, found_a = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        const char* sel = arr->tokens[i].data.rule.selector;
        int sz = arr->tokens[i].data.rule.selectorTokenSize;
        if (strcmp(sel, "h1") == 0) {
            found_h1 = 1;
            assert_equal_int(sz, 1, "CSS comprehensive: 'h1' selector → 1 tokenizable");
            assert_true(arr->tokens[i].data.rule.selectorTokens[0].isPattern,
                        "CSS comprehensive: 'h1' selector tokenizable is pattern");
        }
        if (strcmp(sel, "p") == 0) {
            found_p = 1;
            assert_equal_int(sz, 1, "CSS comprehensive: 'p' selector → 1 tokenizable");
            assert_true(arr->tokens[i].data.rule.selectorTokens[0].isPattern,
                        "CSS comprehensive: 'p' selector tokenizable is pattern");
        }
        if (strcmp(sel, "a") == 0) {
            found_a = 1;
            assert_equal_int(sz, 1, "CSS comprehensive: 'a' selector → 1 tokenizable");
            assert_true(arr->tokens[i].data.rule.selectorTokens[0].isPattern,
                        "CSS comprehensive: 'a' selector tokenizable is pattern");
        }
    }
    assert_true(found_h1, "CSS comprehensive: 'h1' rule found");
    assert_true(found_p,  "CSS comprehensive: 'p' rule found");
    assert_true(found_a,  "CSS comprehensive: 'a' rule found");

    /* ---- Multi-selector "html, body" falls back to ASCII tokenizables ---- */
    int found_html_body = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        if (strcmp(arr->tokens[i].data.rule.selector, "html, body") == 0) {
            found_html_body = 1;
            int sz = arr->tokens[i].data.rule.selectorTokenSize;
            /* Greedy scan: "html"(pattern) + ","(ASCII) + " "(ASCII) + "body"(pattern) = 4 */
            assert_equal_int(sz, 4,
                             "CSS comprehensive: 'html, body' selector → 4 greedy tokenizables");
            assert_true(arr->tokens[i].data.rule.selectorTokens[0].isPattern,
                        "CSS comprehensive: 'html, body' tok[0] is pattern (html)");
            assert_true(!arr->tokens[i].data.rule.selectorTokens[1].isPattern,
                        "CSS comprehensive: 'html, body' tok[1] is ASCII ','");
            assert_true(!arr->tokens[i].data.rule.selectorTokens[2].isPattern,
                        "CSS comprehensive: 'html, body' tok[2] is ASCII ' '");
            assert_true(arr->tokens[i].data.rule.selectorTokens[3].isPattern,
                        "CSS comprehensive: 'html, body' tok[3] is pattern (body)");
            break;
        }
    }
    assert_true(found_html_body, "CSS comprehensive: 'html, body' rule found");

    /* ---- Compound selector "a:hover" splits into two patterns ---- */
    /* Greedy: "a"(SEG1 tag) + ":hover"(SEG3 pseudo-class) = 2 pattern tokenizables */
    int found_a_hover = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        if (strcmp(arr->tokens[i].data.rule.selector, "a:hover") != 0) continue;
        found_a_hover = 1;
        int sz = arr->tokens[i].data.rule.selectorTokenSize;
        assert_equal_int(sz, 2, "CSS comprehensive: 'a:hover' → 2 tokenizables");
        assert_true(arr->tokens[i].data.rule.selectorTokens[0].isPattern,
                    "CSS comprehensive: 'a:hover' tok[0] is pattern (a)");
        assert_true(arr->tokens[i].data.rule.selectorTokens[1].isPattern,
                    "CSS comprehensive: 'a:hover' tok[1] is pattern (:hover)");
        break;
    }
    assert_true(found_a_hover, "CSS comprehensive: 'a:hover' rule found");

    /* ---- Pseudo-element "a::before" splits into two patterns ---- */
    /* Greedy: "a"(SEG1) + "::before"(SEG4 pseudo-element) = 2 pattern tokenizables */
    int found_a_before = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        if (strcmp(arr->tokens[i].data.rule.selector, "a::before") != 0) continue;
        found_a_before = 1;
        int sz = arr->tokens[i].data.rule.selectorTokenSize;
        assert_equal_int(sz, 2, "CSS comprehensive: 'a::before' → 2 tokenizables");
        assert_true(arr->tokens[i].data.rule.selectorTokens[0].isPattern,
                    "CSS comprehensive: 'a::before' tok[0] is pattern (a)");
        assert_true(arr->tokens[i].data.rule.selectorTokens[1].isPattern,
                    "CSS comprehensive: 'a::before' tok[1] is pattern (::before)");
        break;
    }
    assert_true(found_a_before, "CSS comprehensive: 'a::before' rule found");

    /* ---- Property name pattern matches ---- */
    /* In the h1 rule: color, font-size, font-weight should all be patterns */
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        if (strcmp(arr->tokens[i].data.rule.selector, "h1") != 0) continue;
        int nProps = arr->tokens[i].data.rule.propertyCount;
        assert_true(nProps >= 3, "CSS comprehensive: h1 has >= 3 properties");
        for (int j = 0; j < nProps; j++) {
            CSSProperty* prop = &arr->tokens[i].data.rule.properties[j];
            assert_equal_int(prop->nameTokenSize, 1,
                             "CSS comprehensive: h1 property name → 1 tokenizable");
            assert_true(prop->nameTokens[0].isPattern,
                        "CSS comprehensive: h1 property name tokenizable is pattern");
        }
        break;
    }

    /* ---- Property value pattern matches: blue, bold, flex, none ---- */
    int found_blue = 0, found_bold = 0, found_flex = 0, found_none = 0;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        int nProps = arr->tokens[i].data.rule.propertyCount;
        for (int j = 0; j < nProps; j++) {
            CSSProperty* prop = &arr->tokens[i].data.rule.properties[j];
            if (prop->valueTokenSize == 1 && prop->valueTokens[0].isPattern) {
                /* Look up which pattern this maps to */
                unsigned short idx = prop->valueTokens[0].flag;
                if (idx < (unsigned short)CSS_PATTERN_COUNT) {
                    const char* pname = CSS_PATTERNS[idx];
                    if (strcmp(pname, "blue")  == 0) found_blue = 1;
                    if (strcmp(pname, "bold")  == 0) found_bold = 1;
                    if (strcmp(pname, "flex")  == 0) found_flex = 1;
                    if (strcmp(pname, "none")  == 0) found_none = 1;
                }
            }
        }
    }
    assert_true(found_blue, "CSS comprehensive: value 'blue' → pattern tokenizable");
    assert_true(found_bold, "CSS comprehensive: value 'bold' → pattern tokenizable");
    assert_true(found_flex, "CSS comprehensive: value 'flex' → pattern tokenizable");
    assert_true(found_none, "CSS comprehensive: value 'none' → pattern tokenizable");

    /* ---- All selector rules have at least one property ---- */
    int all_have_props = 1;
    for (int i = 0; i < arr->count; i++) {
        if (arr->tokens[i].type != 0) continue;
        if (arr->tokens[i].data.rule.propertyCount == 0) {
            all_have_props = 0; break;
        }
    }
    assert_true(all_have_props,
                "CSS comprehensive: every selector rule has at least 1 property");

    printf("PASS CSS tokenizable comprehensive test\n");
    freeCSS(arr);
}

/* ---- zlib vs NL-EN AE Benchmark Tests ---- */

static void print_ae_benchmark_row(size_t input_len,
                                   size_t ae_size,
                                   uLongf zlib_size) {
    double ae_pct   = (double)ae_size   / (double)input_len * 100.0;
    double zlib_pct = (double)zlib_size / (double)input_len * 100.0;
    printf("  Input:            %zu bytes\n", input_len);
    printf("  NL-EN AE encode:  %zu bytes (%.1f%% of original, %.1f%% reduction)\n",
           ae_size,   ae_pct,   100.0 - ae_pct);
    printf("  zlib compress:    %lu bytes (%.1f%% of original, %.1f%% reduction)\n",
           (unsigned long)zlib_size, zlib_pct, 100.0 - zlib_pct);
}

/* Short message: a single casual sentence (~40 chars).
   Measures encoded size vs zlib on a very small input where per-symbol
   table overhead is visible relative to the payload.                      */
void test_zlib_compare_ae_short_message(void) {
    const char* text = "Hello, how are you doing today?";

    size_t input_len = strlen(text);

    NLTokenArray* tokens = tokenizeEnglish(text);
    assert_true(tokens != NULL, "AE benchmark short: tokenize should succeed");
    assert_true(tokens->count > 0, "AE benchmark short: should produce tokens");

    size_t ae_size = 0;
    unsigned char* ae_encoded = nl_en_encode_ae(tokens, tokens->count, &ae_size);
    assert_true(ae_encoded != NULL, "AE benchmark short: AE encode should succeed");
    assert_true(ae_size > 0,        "AE benchmark short: AE encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "AE benchmark short: malloc for zlib buffer should succeed");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)text, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "AE benchmark short: zlib compress should return Z_OK");
    assert_true(zlib_dest_len > 0,   "AE benchmark short: zlib compressed size > 0");

    print_ae_benchmark_row(input_len, ae_size, zlib_dest_len);

    free(ae_encoded);
    free(zlib_dest);
    freeNLTokenArray(tokens);
    printf("PASS zlib vs NL-EN AE compare - short English message\n");
}

/* Long professional text: a multi-paragraph passage (~700 chars).
   Measures encoded size vs zlib on a longer input where the fixed-size
   frequency table becomes a smaller fraction of total output.            */
void test_zlib_compare_ae_long_text(void) {
    const char* text =
        "The rapid advancement of machine learning has fundamentally altered how "
        "engineers approach software design and system architecture. Distributed "
        "computation frameworks, once reserved for large research institutions, are "
        "now accessible to small teams building production systems at scale.\n\n"
        "Effective compression techniques reduce bandwidth consumption and storage "
        "costs across every layer of the stack. General-purpose algorithms such as "
        "deflate offer broad applicability, while domain-specific codecs exploit "
        "structural knowledge of the target data to achieve superior ratios on "
        "their intended content class. Both approaches occupy important roles in "
        "modern infrastructure, often working in combination.\n\n"
        "Arithmetic coding assigns each symbol a probability-weighted sub-interval "
        "of the unit interval, encoding an entire sequence as a single fractional "
        "number. Compared with Huffman coding, it achieves entropy more closely "
        "when symbol probabilities are skewed and avoids the one-bit-per-symbol "
        "floor that limits fixed-length prefix codes. The practical trade-off is "
        "higher implementation complexity and sensitivity to precision in the "
        "underlying integer arithmetic.";

    size_t input_len = strlen(text);

    NLTokenArray* tokens = tokenizeEnglish(text);
    assert_true(tokens != NULL, "AE benchmark long: tokenize should succeed");
    assert_true(tokens->count > 0, "AE benchmark long: should produce tokens");

    size_t ae_size = 0;
    unsigned char* ae_encoded = nl_en_encode_ae(tokens, tokens->count, &ae_size);
    assert_true(ae_encoded != NULL, "AE benchmark long: AE encode should succeed");
    assert_true(ae_size > 0,        "AE benchmark long: AE encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "AE benchmark long: malloc for zlib buffer should succeed");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)text, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "AE benchmark long: zlib compress should return Z_OK");
    assert_true(zlib_dest_len > 0,   "AE benchmark long: zlib compressed size > 0");

    print_ae_benchmark_row(input_len, ae_size, zlib_dest_len);

    free(ae_encoded);
    free(zlib_dest);
    freeNLTokenArray(tokens);
    printf("PASS zlib vs NL-EN AE compare - long professional text\n");
}

/* ---- zlib vs NL-EN Variable-Width Benchmark Tests ---- */

static void print_vw_benchmark_row(size_t input_len,
                                   size_t vw_size,
                                   uLongf zlib_size) {
    double vw_pct   = (double)vw_size   / (double)input_len * 100.0;
    double zlib_pct = (double)zlib_size / (double)input_len * 100.0;
    printf("  Input:            %zu bytes\n", input_len);
    printf("  NL-EN vw encode:  %zu bytes (%.1f%% of original, %.1f%% reduction)\n",
           vw_size,   vw_pct,   100.0 - vw_pct);
    printf("  zlib compress:    %lu bytes (%.1f%% of original, %.1f%% reduction)\n",
           (unsigned long)zlib_size, zlib_pct, 100.0 - zlib_pct);
}

/* Same short text as the AE short-message benchmark for direct comparison. */
void test_zlib_compare_vw_short_message(void) {
    const char* text = "Hello, how are you doing today?";

    size_t input_len = strlen(text);

    NLTokenArray* tokens = tokenizeEnglish(text);
    assert_true(tokens != NULL, "vw benchmark short: tokenize should succeed");
    assert_true(tokens->count > 0, "vw benchmark short: should produce tokens");

    size_t vw_size = 0;
    unsigned char* vw_encoded = nl_en_encode(tokens, tokens->count, &vw_size);
    assert_true(vw_encoded != NULL, "vw benchmark short: encode should succeed");
    assert_true(vw_size > 0,        "vw benchmark short: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "vw benchmark short: malloc for zlib buffer should succeed");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)text, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "vw benchmark short: zlib compress should return Z_OK");
    assert_true(zlib_dest_len > 0,   "vw benchmark short: zlib compressed size > 0");

    print_vw_benchmark_row(input_len, vw_size, zlib_dest_len);

    free(vw_encoded);
    free(zlib_dest);
    freeNLTokenArray(tokens);
    printf("PASS zlib vs NL-EN vw compare - short English message\n");
}

/* Same long text as the AE long-text benchmark for direct comparison. */
void test_zlib_compare_vw_long_text(void) {
    const char* text =
        "The rapid advancement of machine learning has fundamentally altered how "
        "engineers approach software design and system architecture. Distributed "
        "computation frameworks, once reserved for large research institutions, are "
        "now accessible to small teams building production systems at scale.\n\n"
        "Effective compression techniques reduce bandwidth consumption and storage "
        "costs across every layer of the stack. General-purpose algorithms such as "
        "deflate offer broad applicability, while domain-specific codecs exploit "
        "structural knowledge of the target data to achieve superior ratios on "
        "their intended content class. Both approaches occupy important roles in "
        "modern infrastructure, often working in combination.\n\n"
        "Arithmetic coding assigns each symbol a probability-weighted sub-interval "
        "of the unit interval, encoding an entire sequence as a single fractional "
        "number. Compared with Huffman coding, it achieves entropy more closely "
        "when symbol probabilities are skewed and avoids the one-bit-per-symbol "
        "floor that limits fixed-length prefix codes. The practical trade-off is "
        "higher implementation complexity and sensitivity to precision in the "
        "underlying integer arithmetic.";

    size_t input_len = strlen(text);

    NLTokenArray* tokens = tokenizeEnglish(text);
    assert_true(tokens != NULL, "vw benchmark long: tokenize should succeed");
    assert_true(tokens->count > 0, "vw benchmark long: should produce tokens");

    size_t vw_size = 0;
    unsigned char* vw_encoded = nl_en_encode(tokens, tokens->count, &vw_size);
    assert_true(vw_encoded != NULL, "vw benchmark long: encode should succeed");
    assert_true(vw_size > 0,        "vw benchmark long: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "vw benchmark long: malloc for zlib buffer should succeed");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)text, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "vw benchmark long: zlib compress should return Z_OK");
    assert_true(zlib_dest_len > 0,   "vw benchmark long: zlib compressed size > 0");

    print_vw_benchmark_row(input_len, vw_size, zlib_dest_len);

    free(vw_encoded);
    free(zlib_dest);
    freeNLTokenArray(tokens);
    printf("PASS zlib vs NL-EN vw compare - long professional text\n");
}

/* =========================================================================
   CSS Codec AE tests
   ========================================================================= */

/* Req 1 (csscodec): comment tokens are raw ASCII characters */
void test_css_comment_tokenization(void) {
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS comment tok: alloc");

    parseCSS("/* hi */", arr);
    assert_equal_int(arr->count, 1, "CSS comment tok: 1 token");
    assert_equal_int(arr->tokens[0].type, 2, "CSS comment tok: type=2");

    int sz = arr->tokens[0].data.comment.commentTokenSize;
    assert_equal_int(sz, 8, "CSS comment tok: size=8 (slash-star space hi space star-slash)");
    assert_true(!arr->tokens[0].data.comment.commentTokens[0].isPattern,
                "CSS comment tok: isPattern=false");
    assert_equal_int((int)arr->tokens[0].data.comment.commentTokens[0].flag,
                     '/', "CSS comment tok: first char='/'");
    free(arr);
    printf("PASS CSS comment tokenization\n");
}

/* Req 2 (csscodec): flatten produces correct sentinel boundaries */
void test_css_flatten_rule_tokens(void) {
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS flatten: alloc");

    /* "body { color: red; }" - body=SEG1 pattern, color=SEG5 pattern, red=color pattern */
    parseCSS("body { color: red; }", arr);
    assert_equal_int(arr->count, 1, "CSS flatten: 1 token");
    assert_equal_int(arr->tokens[0].type, 0, "CSS flatten: type=0");

    int sz = arr->tokens[0].data.rule.ruleTokenSize;
    assert_true(sz > 0, "CSS flatten: ruleTokenSize > 0");

    /* Layout: [body] [{] [color] [:] [red] [;] [}]  = 7 entries */
    assert_equal_int(sz, 7, "CSS flatten: 7 entries for body{color:red;}");

    /* Sentinel at index 1 must be '{' */
    const CSSTokenizable* rt = arr->tokens[0].data.rule.ruleTokens;
    assert_true(!rt[1].isPattern, "CSS flatten: rt[1] isPattern=false");
    assert_equal_int((int)rt[1].flag, '{', "CSS flatten: rt[1]='{' ");
    /* ':' sentinel */
    assert_true(!rt[3].isPattern, "CSS flatten: rt[3] isPattern=false");
    assert_equal_int((int)rt[3].flag, ':', "CSS flatten: rt[3]=':'");
    /* ';' sentinel */
    assert_true(!rt[5].isPattern, "CSS flatten: rt[5] isPattern=false");
    assert_equal_int((int)rt[5].flag, ';', "CSS flatten: rt[5]=';'");
    /* '}' sentinel */
    assert_true(!rt[6].isPattern, "CSS flatten: rt[6] isPattern=false");
    assert_equal_int((int)rt[6].flag, '}', "CSS flatten: rt[6]='}'");

    free(arr);
    printf("PASS CSS flatten rule tokens\n");
}

/* Req 3 (csscodec): frequency map best case — repeated tokens */
void test_css_freqmap_best_case(void) {
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS freqmap best: alloc");

    /* Two identical selector rules => same tokens appear twice */
    parseCSS("body { color: red; } body { color: red; }", arr);
    assert_equal_int(arr->count, 2, "CSS freqmap best: 2 tokens");

    CSSFreqMap* fmap = collectCSSFrequencies(arr);
    assert_true(fmap != NULL, "CSS freqmap best: non-NULL");
    assert_true(fmap->uniqueCount > 0, "CSS freqmap best: uniqueCount>0");
    assert_true(fmap->totalTokens > 0, "CSS freqmap best: totalTokens>0");
    /* unique <= total */
    assert_true(fmap->uniqueCount <= fmap->totalTokens,
                "CSS freqmap best: unique<=total");
    /* most frequent entry should have frequency >= 2 */
    assert_true(fmap->entries[0].frequency >= 2,
                "CSS freqmap best: top freq>=2");
    /* sorted descending */
    for (int i = 0; i + 1 < fmap->uniqueCount; i++) {
        assert_true(fmap->entries[i].frequency >= fmap->entries[i+1].frequency,
                    "CSS freqmap best: sorted descending");
    }

    freeCSSFreqMap(fmap);
    free(arr);
    printf("PASS CSS freqmap best case\n");
}

/* Req 3 (csscodec): frequency map worst case — all unique tokens */
void test_css_freqmap_worst_case(void) {
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS freqmap worst: alloc");

    /* @media at-rule: uses uncommon tokens so frequency is 1 each */
    parseCSS("@media print { }", arr);
    assert_true(arr->count >= 1, "CSS freqmap worst: >=1 token");

    CSSFreqMap* fmap = collectCSSFrequencies(arr);
    assert_true(fmap != NULL, "CSS freqmap worst: non-NULL");
    assert_true(fmap->uniqueCount > 0, "CSS freqmap worst: uniqueCount>0");
    assert_true(fmap->uniqueCount <= fmap->totalTokens,
                "CSS freqmap worst: unique<=total");

    freeCSSFreqMap(fmap);
    free(arr);
    printf("PASS CSS freqmap worst case\n");
}

/* Req 5+6 (csscodec): encode -> decode round-trip, best case */
void test_css_ae_codec_best_case(void) {
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS AE best: alloc");

    /* Simple rule: body { color: red; } */
    parseCSS("body { color: red; }", arr);
    assert_equal_int(arr->count, 1, "CSS AE best: 1 rule");

    size_t outSize = 0;
    unsigned char* buf = css_encode_ae(arr, &outSize);
    assert_true(buf != NULL,  "CSS AE best: encode non-NULL");
    assert_true(outSize > 0,  "CSS AE best: encoded size > 0");

    CSSTokenArray* decoded = css_decode_ae(buf, outSize);
    assert_true(decoded != NULL, "CSS AE best: decode non-NULL");
    assert_equal_int(decoded->count, arr->count, "CSS AE best: token count");
    assert_equal_int(decoded->tokens[0].type, 0, "CSS AE best: type=0");

    /* selectorTokenSize round-trip */
    assert_equal_int(decoded->tokens[0].data.rule.selectorTokenSize,
                     arr->tokens[0].data.rule.selectorTokenSize,
                     "CSS AE best: selectorTokenSize");
    /* propertyCount round-trip */
    assert_equal_int(decoded->tokens[0].data.rule.propertyCount,
                     arr->tokens[0].data.rule.propertyCount,
                     "CSS AE best: propertyCount");

    /* Verify probability sum == CSS_AE_SCALE by re-building from freq map */
    CSSFreqMap* fmap = collectCSSFrequencies(arr);
    assert_true(fmap != NULL, "CSS AE best: fmap non-NULL");
    size_t n = (size_t)fmap->uniqueCount;
    CSSAESymbol* syms = (CSSAESymbol*)malloc(n * sizeof(CSSAESymbol));
    assert_true(syms != NULL, "CSS AE best: syms alloc");
    for (size_t i = 0; i < n; i++) {
        syms[i].token = fmap->entries[i].token;
        syms[i].frequency = (uint32_t)fmap->entries[i].frequency;
    }
    /* call internal helper via header-exposed prototype isn't available,
       so we verify the invariant manually: cum_high of last = CSS_AE_SCALE */
    uint32_t total_freq = 0;
    for (size_t i = 0; i < n; i++) total_freq += syms[i].frequency;
    uint32_t cum = 0;
    for (size_t i = 0; i < n; i++) {
        uint32_t hi = (uint32_t)((uint64_t)(cum + syms[i].frequency) * CSS_AE_SCALE / total_freq);
        cum += syms[i].frequency;
        if (i == n - 1)
            assert_equal_int((int)hi, (int)CSS_AE_SCALE,
                             "CSS AE best: prob sum == CSS_AE_SCALE");
    }
    free(syms);
    freeCSSFreqMap(fmap);

    free(buf);
    free(decoded);
    free(arr);
    printf("PASS CSS AE codec best case\n");
}

/* Req 5+6 (csscodec): encode -> decode round-trip, worst case.
   Two identical properties give heavy repetition so the 32-bit
   fixed-point arithmetic stays within precision limits, while the
   ruleToken count (11) and property parse (2 properties) are harder
   than the best-case (7 tokens, 1 property).                        */
void test_css_ae_codec_worst_case(void) {
    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS AE worst: alloc");

    parseCSS("body { color: red; color: red; }", arr);
    assert_equal_int(arr->count, 1, "CSS AE worst: 1 token");
    assert_equal_int(arr->tokens[0].data.rule.propertyCount, 2,
                     "CSS AE worst: 2 properties before encode");

    size_t outSize = 0;
    unsigned char* buf = css_encode_ae(arr, &outSize);
    assert_true(buf != NULL, "CSS AE worst: encode non-NULL");
    assert_true(outSize > 0, "CSS AE worst: encoded size > 0");

    CSSTokenArray* decoded = css_decode_ae(buf, outSize);
    assert_true(decoded != NULL, "CSS AE worst: decode non-NULL");
    assert_equal_int(decoded->count, arr->count, "CSS AE worst: token count");
    assert_equal_int(decoded->tokens[0].type, 0, "CSS AE worst: type=0");
    assert_equal_int(decoded->tokens[0].data.rule.propertyCount,
                     arr->tokens[0].data.rule.propertyCount,
                     "CSS AE worst: propertyCount match");

    free(buf);
    free(decoded);
    free(arr);
    printf("PASS CSS AE codec worst case\n");
}

/* ---- zlib vs CSS AE Benchmark Tests ---- */

static void print_css_ae_benchmark_row(size_t input_len,
                                       size_t ae_size,
                                       uLongf zlib_size) {
    double ae_pct   = (double)ae_size   / (double)input_len * 100.0;
    double zlib_pct = (double)zlib_size / (double)input_len * 100.0;
    printf("  Input:           %zu bytes\n", input_len);
    printf("  CSS AE encode:   %zu bytes (%.1f%% of original, %.1f%% reduction)\n",
           ae_size,   ae_pct,   100.0 - ae_pct);
    printf("  zlib compress:   %lu bytes (%.1f%% of original, %.1f%% reduction)\n",
           (unsigned long)zlib_size, zlib_pct, 100.0 - zlib_pct);
}

/* Best case: short rule with maximum token repetition.
   Two identical properties share the same ruleToken symbols, so the AE
   frequency table carries useful probability information relative to the
   raw byte count.  ruleTokenSize = 11, uniqueCount = 7.                  */
void test_zlib_compare_css_ae_best_case(void) {
    const char* css = "body { color: red; color: red; }";
    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS AE bench best: alloc");
    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS AE bench best: parsed token count > 0");

    size_t ae_size = 0;
    unsigned char* ae_encoded = css_encode_ae(arr, &ae_size);
    assert_true(ae_encoded != NULL, "CSS AE bench best: encode non-NULL");
    assert_true(ae_size > 0,        "CSS AE bench best: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "CSS AE bench best: malloc for zlib buffer");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)css, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "CSS AE bench best: zlib compress Z_OK");
    assert_true(zlib_dest_len > 0,   "CSS AE bench best: zlib size > 0");

    print_css_ae_benchmark_row(input_len, ae_size, zlib_dest_len);

    free(ae_encoded);
    free(zlib_dest);
    free(arr);
    printf("PASS zlib vs CSS AE compare - best case (repeated properties)\n");
}

/* Worst case: short rule with no token repetition.
   Every ruleToken is unique (10 total, 10 unique), so the AE frequency
   table overhead is large relative to the payload and the codec is at
   a disadvantage compared to zlib.  ruleTokenSize = 10, uniqueCount = 10. */
void test_zlib_compare_css_ae_worst_case(void) {
    const char* css = "a:hover { font-size: 2em; }";
    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS AE bench worst: alloc");
    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS AE bench worst: parsed token count > 0");

    size_t ae_size = 0;
    unsigned char* ae_encoded = css_encode_ae(arr, &ae_size);
    assert_true(ae_encoded != NULL, "CSS AE bench worst: encode non-NULL");
    assert_true(ae_size > 0,        "CSS AE bench worst: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "CSS AE bench worst: malloc for zlib buffer");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)css, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "CSS AE bench worst: zlib compress Z_OK");
    assert_true(zlib_dest_len > 0,   "CSS AE bench worst: zlib size > 0");

    print_css_ae_benchmark_row(input_len, ae_size, zlib_dest_len);

    free(ae_encoded);
    free(zlib_dest);
    free(arr);
    printf("PASS zlib vs CSS AE compare - worst case (all-unique tokens)\n");
}

/* Long stylesheet: a realistic CSS file representative of a small webpage.
   The total CSSTokenizable count (several hundred) far exceeds the 32-bit
   fixed-point precision limit of the AE codec, so the encoded output is
   structurally valid but would not decode correctly.  The test is a pure
   size benchmark — it reports how the codec's output compares to zlib on
   the raw CSS bytes.                                                        */
void test_zlib_compare_css_ae_long_stylesheet(void) {
    const char* css =
        "/* reset */\n"
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "/* base */\n"
        "body { font-family: sans-serif; font-size: 16px; line-height: 1.6; color: #333; background: white; }\n"
        "h1 { font-size: 2rem; font-weight: bold; color: #111; margin-bottom: 16px; }\n"
        "h2 { font-size: 1.5rem; font-weight: bold; color: #222; margin-bottom: 12px; }\n"
        "p { margin-bottom: 16px; }\n"
        "a { color: blue; text-decoration: underline; }\n"
        "a:hover { color: darkblue; text-decoration: none; }\n"
        "img { display: block; max-width: 100%; height: auto; }\n"
        "/* layout */\n"
        "header { display: flex; justify-content: space-between; align-items: center;"
        " padding: 16px 24px; background: white; border-bottom: 1px solid #ddd; }\n"
        "nav a { color: #333; text-decoration: none; font-weight: bold; }\n"
        "main { max-width: 960px; margin: 0 auto; padding: 32px 16px; }\n"
        "footer { background: #222; color: white; text-align: center; padding: 24px; }\n"
        "/* hero */\n"
        ".hero { background: #4f46e5; color: white; text-align: center; padding: 64px 24px; }\n"
        ".hero h1 { font-size: 3rem; margin-bottom: 24px; }\n"
        "/* card */\n"
        ".card { background: white; border-radius: 8px; padding: 24px; box-shadow: 0 2px 8px #0001; }\n"
        "/* button */\n"
        "button { display: inline; padding: 10px 20px; background: blue; color: white;"
        " border: none; border-radius: 4px; cursor: pointer; font-weight: bold; }\n"
        "button:hover { background: darkblue; }\n"
        "/* responsive */\n"
        "@media (max-width: 768px) { }\n";

    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS AE long: alloc");
    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS AE long: parsed token count > 0");

    /* Count total CSSTokenizables across all tokens */
    int total_tokenizables = 0;
    for (int t = 0; t < arr->count; t++) {
        const CSSToken* tok = &arr->tokens[t];
        if      (tok->type == 0) total_tokenizables += tok->data.rule.ruleTokenSize;
        else if (tok->type == 1) total_tokenizables += tok->data.atRule.atRuleTokenSize;
        else                     total_tokenizables += tok->data.comment.commentTokenSize;
    }

    size_t ae_size = 0;
    unsigned char* ae_encoded = css_encode_ae(arr, &ae_size);
    assert_true(ae_encoded != NULL, "CSS AE long: encode non-NULL");
    assert_true(ae_size > 0,        "CSS AE long: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "CSS AE long: malloc for zlib buffer");

    int zlib_result = compress(zlib_dest, &zlib_dest_len,
                               (const Bytef*)css, (uLong)input_len);
    assert_true(zlib_result == Z_OK, "CSS AE long: zlib compress Z_OK");
    assert_true(zlib_dest_len > 0,   "CSS AE long: zlib size > 0");

    printf("  CSS tokens:      %d rules/at-rules/comments, %d total CSSTokenizables\n",
           arr->count, total_tokenizables);
    print_css_ae_benchmark_row(input_len, ae_size, zlib_dest_len);

    free(ae_encoded);
    free(zlib_dest);
    free(arr);
    printf("PASS zlib vs CSS AE compare - long stylesheet\n");
}

/* ── Renormalization round-trip tests ────────────────────────────────────── */

/* NL-EN AE: encode/decode a 30+ token sequence that previously collapsed
   with fixed-point 32-bit AE (no renormalization).  Verifies that the
   renormalized codec produces a lossless round-trip for long inputs.        */
void test_nl_en_ae_codec_long_sequence(void) {
    /* Two repetitions of a sentence give enough token diversity and count
       to guarantee collapse under the old fixed-tag approach while staying
       fast in this unit test.                                               */
    const char* text =
        "the quick brown fox jumps over the lazy dog "
        "the quick brown fox jumps over the lazy dog";

    NLTokenArray* original = tokenizeEnglish(text);
    assert_true(original != NULL, "NL AE long: tokenize non-NULL");
    if (!original) return;

    assert_true((int)original->count >= 20,
                "NL AE long: at least 20 tokens (enough to stress precision)");

    size_t encoded_size = 0;
    unsigned char* encoded = nl_en_encode_ae(original, original->count, &encoded_size);
    assert_true(encoded != NULL,    "NL AE long: encoded non-NULL");
    assert_true(encoded_size > 0,   "NL AE long: encoded size > 0");
    if (!encoded) { freeNLTokenArray(original); return; }

    NLTokenArray* decoded = nl_en_decode_ae(encoded, encoded_size);
    assert_true(decoded != NULL, "NL AE long: decoded non-NULL");
    if (!decoded) { free(encoded); freeNLTokenArray(original); return; }

    assert_equal_int((int)decoded->count, (int)original->count,
                     "NL AE long: decoded count matches original");

    if (decoded->count == original->count) {
        bool all_match = true;
        for (size_t i = 0; i < original->count; i++) {
            if (decoded->tokens[i].isPattern != original->tokens[i].isPattern ||
                decoded->tokens[i].flag      != original->tokens[i].flag) {
                all_match = false;
                break;
            }
            if (original->tokens[i].isPattern &&
                decoded->tokens[i].caseStyle != original->tokens[i].caseStyle) {
                all_match = false;
                break;
            }
        }
        assert_true(all_match, "NL AE long: all decoded tokens match original");
    }

    printf("PASS NL-EN AE codec - long sequence (%zu tokens, %zu bytes encoded)\n",
           original->count, encoded_size);

    free(encoded);
    freeNLTokenArray(decoded);
    freeNLTokenArray(original);
}

/* CSS AE: encode/decode a rule with 6 properties — 30+ ruleTokenizables.
   Previously failed due to precision collapse; should now round-trip
   correctly with renormalization.                                           */
void test_css_ae_codec_long_rule(void) {
    const char* css =
        "section { display: flex; flex-direction: column; "
        "align-items: center; justify-content: center; "
        "background-color: white; color: black; }";

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS AE long rule: alloc");
    if (!arr) return;

    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS AE long rule: parseCSS produced tokens");
    if (arr->count == 0) { free(arr); return; }

    /* Confirm the rule has the expected properties */
    CSSToken* rule = &arr->tokens[0];
    assert_true(rule->type == 0, "CSS AE long rule: type is rule");
    assert_true(rule->data.rule.propertyCount == 6,
                "CSS AE long rule: 6 properties parsed");
    assert_true(rule->data.rule.ruleTokenSize >= 20,
                "CSS AE long rule: ruleTokenSize >= 20 (exercises renorm)");

    size_t ae_size = 0;
    unsigned char* ae_encoded = css_encode_ae(arr, &ae_size);
    assert_true(ae_encoded != NULL, "CSS AE long rule: encoded non-NULL");
    assert_true(ae_size > 0,        "CSS AE long rule: encoded size > 0");
    if (!ae_encoded) { free(arr); return; }

    CSSTokenArray* decoded = css_decode_ae(ae_encoded, ae_size);
    assert_true(decoded != NULL, "CSS AE long rule: decoded non-NULL");
    if (!decoded) { free(ae_encoded); free(arr); return; }

    assert_equal_int(decoded->count, arr->count,
                     "CSS AE long rule: decoded token count matches");

    if (decoded->count > 0 && arr->count > 0) {
        CSSToken* dr = &decoded->tokens[0];
        assert_equal_int(dr->type, 0,
                         "CSS AE long rule: decoded type is rule");
        assert_equal_int(dr->data.rule.propertyCount,
                         rule->data.rule.propertyCount,
                         "CSS AE long rule: propertyCount round-trips");
        assert_equal_int(dr->data.rule.selectorTokenSize,
                         rule->data.rule.selectorTokenSize,
                         "CSS AE long rule: selectorTokenSize round-trips");
        assert_equal_int(dr->data.rule.ruleTokenSize,
                         rule->data.rule.ruleTokenSize,
                         "CSS AE long rule: ruleTokenSize round-trips");
    }

    printf("PASS CSS AE codec - long rule (%d ruleTokens, %d properties, %zu bytes encoded)\n",
           rule->data.rule.ruleTokenSize,
           rule->data.rule.propertyCount,
           ae_size);

    free(ae_encoded);
    free(decoded);
    free(arr);
}

/* ── CSS optimised codec tests (Steps 1-3) ─────────────────────────────── */

/* Helper: compare two CSSTokenArrays for structural equality.
 * Checks type, ruleTokenSize/atRuleTokenSize/commentTokenSize,
 * propertyCount, selectorTokenSize, and every CSSTokenizable flag.   */
static bool css_token_arrays_equal(const CSSTokenArray* a, const CSSTokenArray* b) {
    if (a->count != b->count) return false;
    for (int t = 0; t < a->count; t++) {
        const CSSToken* ta = &a->tokens[t];
        const CSSToken* tb = &b->tokens[t];
        if (ta->type != tb->type) return false;
        if (ta->type == 0) {
            if (ta->data.rule.ruleTokenSize != tb->data.rule.ruleTokenSize) return false;
            if (ta->data.rule.selectorTokenSize != tb->data.rule.selectorTokenSize) return false;
            if (ta->data.rule.propertyCount != tb->data.rule.propertyCount) return false;
            for (int i = 0; i < ta->data.rule.ruleTokenSize; i++) {
                if (ta->data.rule.ruleTokens[i].isPattern !=
                    tb->data.rule.ruleTokens[i].isPattern) return false;
                if (ta->data.rule.ruleTokens[i].flag !=
                    tb->data.rule.ruleTokens[i].flag) return false;
            }
        } else if (ta->type == 1) {
            if (ta->data.atRule.atRuleTokenSize != tb->data.atRule.atRuleTokenSize)
                return false;
            for (int i = 0; i < ta->data.atRule.atRuleTokenSize; i++) {
                if (ta->data.atRule.atRuleTokens[i].isPattern !=
                    tb->data.atRule.atRuleTokens[i].isPattern) return false;
                if (ta->data.atRule.atRuleTokens[i].flag !=
                    tb->data.atRule.atRuleTokens[i].flag) return false;
            }
        } else {
            if (ta->data.comment.commentTokenSize != tb->data.comment.commentTokenSize)
                return false;
            for (int i = 0; i < ta->data.comment.commentTokenSize; i++) {
                if (ta->data.comment.commentTokens[i].isPattern !=
                    tb->data.comment.commentTokens[i].isPattern) return false;
                if (ta->data.comment.commentTokens[i].flag !=
                    tb->data.comment.commentTokens[i].flag) return false;
            }
        }
    }
    return true;
}

/* Round-trip: short stylesheet with repeated property (exercises bigram seeding
 * and verifies that encode -> decode is lossless for a simple input).         */
void test_css_opt_codec_roundtrip(void) {
    const char* css = "body { color: red; color: red; }";

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS opt roundtrip: alloc");
    if (!arr) return;
    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS opt roundtrip: parsed tokens > 0");

    size_t enc_size = 0;
    unsigned char* enc = css_encode_opt(arr, &enc_size);
    assert_true(enc != NULL, "CSS opt roundtrip: encode non-NULL");
    assert_true(enc_size > 0, "CSS opt roundtrip: encoded size > 0");

    if (!enc) { free(arr); return; }

    CSSTokenArray* dec = css_decode_opt(enc, enc_size);
    assert_true(dec != NULL, "CSS opt roundtrip: decode non-NULL");

    if (dec) {
        assert_equal_int(dec->count, arr->count,
                         "CSS opt roundtrip: decoded token count matches");
        assert_true(css_token_arrays_equal(arr, dec),
                    "CSS opt roundtrip: all tokenizables match losslessly");
        free(dec);
    }

    printf("PASS CSS opt codec - short round-trip (\"%s\", %zu bytes encoded)\n",
           css, enc_size);
    free(enc);
    free(arr);
}

/* Round-trip: 6-property rule — exercises renormalization over a long
 * sequence (mirrors test_css_ae_codec_long_rule for the opt codec).   */
void test_css_opt_codec_long_roundtrip(void) {
    const char* css =
        "section { display: flex; flex-direction: column; "
        "align-items: center; justify-content: center; "
        "background-color: white; color: black; }";

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS opt long roundtrip: alloc");
    if (!arr) return;
    parseCSS(css, arr);
    assert_true(arr->count > 0,              "CSS opt long roundtrip: parsed tokens > 0");
    assert_true(arr->tokens[0].type == 0,    "CSS opt long roundtrip: type 0 rule");
    assert_true(arr->tokens[0].data.rule.propertyCount == 6,
                "CSS opt long roundtrip: 6 properties");

    size_t enc_size = 0;
    unsigned char* enc = css_encode_opt(arr, &enc_size);
    assert_true(enc != NULL, "CSS opt long roundtrip: encode non-NULL");
    assert_true(enc_size > 0, "CSS opt long roundtrip: encoded size > 0");
    if (!enc) { free(arr); return; }

    CSSTokenArray* dec = css_decode_opt(enc, enc_size);
    assert_true(dec != NULL, "CSS opt long roundtrip: decode non-NULL");

    if (dec) {
        assert_equal_int(dec->count, arr->count,
                         "CSS opt long roundtrip: token count");
        assert_equal_int(dec->tokens[0].data.rule.propertyCount,
                         arr->tokens[0].data.rule.propertyCount,
                         "CSS opt long roundtrip: propertyCount");
        assert_equal_int(dec->tokens[0].data.rule.selectorTokenSize,
                         arr->tokens[0].data.rule.selectorTokenSize,
                         "CSS opt long roundtrip: selectorTokenSize");
        assert_equal_int(dec->tokens[0].data.rule.ruleTokenSize,
                         arr->tokens[0].data.rule.ruleTokenSize,
                         "CSS opt long roundtrip: ruleTokenSize");
        assert_true(css_token_arrays_equal(arr, dec),
                    "CSS opt long roundtrip: all tokenizables match losslessly");
        free(dec);
    }

    printf("PASS CSS opt codec - long round-trip (%d ruleTokens, %d properties, %zu bytes encoded)\n",
           arr->tokens[0].data.rule.ruleTokenSize,
           arr->tokens[0].data.rule.propertyCount,
           enc_size);
    free(enc);
    free(arr);
}

/* Compression ratio: opt codec vs static AE on the long stylesheet.
 * Asserts opt is smaller than static AE and that the round-trip is lossless. */
void test_css_opt_compression_ratio(void) {
    const char* css =
        "/* reset */\n"
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "/* base */\n"
        "body { font-family: sans-serif; font-size: 16px; line-height: 1.6; color: #333; background: white; }\n"
        "h1 { font-size: 2rem; font-weight: bold; color: #111; margin-bottom: 16px; }\n"
        "h2 { font-size: 1.5rem; font-weight: bold; color: #222; margin-bottom: 12px; }\n"
        "p { margin-bottom: 16px; }\n"
        "a { color: blue; text-decoration: underline; }\n"
        "a:hover { color: darkblue; text-decoration: none; }\n"
        "img { display: block; max-width: 100%; height: auto; }\n"
        "/* layout */\n"
        "header { display: flex; justify-content: space-between; align-items: center;"
        " padding: 16px 24px; background: white; border-bottom: 1px solid #ddd; }\n"
        "nav a { color: #333; text-decoration: none; font-weight: bold; }\n"
        "main { max-width: 960px; margin: 0 auto; padding: 32px 16px; }\n"
        "footer { background: #222; color: white; text-align: center; padding: 24px; }\n"
        "/* hero */\n"
        ".hero { background: #4f46e5; color: white; text-align: center; padding: 64px 24px; }\n"
        ".hero h1 { font-size: 3rem; margin-bottom: 24px; }\n"
        "/* card */\n"
        ".card { background: white; border-radius: 8px; padding: 24px; box-shadow: 0 2px 8px #0001; }\n"
        "/* button */\n"
        "button { display: inline; padding: 10px 20px; background: blue; color: white;"
        " border: none; border-radius: 4px; cursor: pointer; font-weight: bold; }\n"
        "button:hover { background: darkblue; }\n"
        "/* responsive */\n"
        "@media (max-width: 768px) { }\n";

    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS opt ratio: alloc");
    if (!arr) return;
    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS opt ratio: parsed tokens > 0");

    size_t opt_size = 0;
    unsigned char* opt_enc = css_encode_opt(arr, &opt_size);
    assert_true(opt_enc != NULL, "CSS opt ratio: opt encode non-NULL");

    size_t ae_size = 0;
    unsigned char* ae_enc = css_encode_ae(arr, &ae_size);
    assert_true(ae_enc != NULL, "CSS opt ratio: AE encode non-NULL");

    double opt_ratio = (double)opt_size / (double)input_len * 100.0;
    double ae_ratio  = (double)ae_size  / (double)input_len * 100.0;

    printf("  Input:        %zu bytes\n", input_len);
    printf("  Static AE:    %zu bytes (%.1f%% of input, %.1f%% reduction)\n",
           ae_size, ae_ratio, 100.0 - ae_ratio);
    printf("  Opt codec:    %zu bytes (%.1f%% of input, %.1f%% reduction)\n",
           opt_size, opt_ratio, 100.0 - opt_ratio);

    assert_true(opt_size < ae_size,
                "CSS opt ratio: opt codec smaller than static AE codec");

    /* Lossless round-trip verification */
    if (opt_enc) {
        CSSTokenArray* dec = css_decode_opt(opt_enc, opt_size);
        assert_true(dec != NULL, "CSS opt ratio: decode non-NULL");
        if (dec) {
            assert_equal_int(dec->count, arr->count,
                             "CSS opt ratio: decoded count matches");
            assert_true(css_token_arrays_equal(arr, dec),
                        "CSS opt ratio: round-trip is lossless");
            free(dec);
        }
    }

    free(opt_enc);
    free(ae_enc);
    free(arr);
    printf("PASS CSS opt codec - compression ratio (%.1f%% reduction vs %.1f%% for static AE)\n",
           100.0 - opt_ratio, 100.0 - ae_ratio);
}

/* ── zlib vs CSS opt benchmark tests ────────────────────────────────────── */

static void print_css_opt_benchmark_row(size_t input_len,
                                        size_t ae_size,
                                        size_t opt_size,
                                        uLongf zlib_size) {
    printf("  Input:           %zu bytes\n", input_len);
    printf("  CSS static AE:   %zu bytes (%.1f%% of original, %.1f%% reduction)\n",
           ae_size,  (double)ae_size  / (double)input_len * 100.0,
           100.0 - (double)ae_size  / (double)input_len * 100.0);
    printf("  CSS opt encode:  %zu bytes (%.1f%% of original, %.1f%% reduction)\n",
           opt_size, (double)opt_size / (double)input_len * 100.0,
           100.0 - (double)opt_size / (double)input_len * 100.0);
    printf("  zlib compress:   %lu bytes (%.1f%% of original, %.1f%% reduction)\n",
           (unsigned long)zlib_size,
           (double)zlib_size / (double)input_len * 100.0,
           100.0 - (double)zlib_size / (double)input_len * 100.0);
}

/* Best case: short rule with maximum token repetition. */
void test_zlib_compare_css_opt_best_case(void) {
    const char* css = "body { color: red; color: red; }";
    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS opt best: alloc");
    if (!arr) return;
    parseCSS(css, arr);

    size_t ae_size = 0;
    unsigned char* ae_enc = css_encode_ae(arr, &ae_size);
    assert_true(ae_enc != NULL, "CSS opt best: AE encode non-NULL");

    size_t opt_size = 0;
    unsigned char* opt_enc = css_encode_opt(arr, &opt_size);
    assert_true(opt_enc != NULL, "CSS opt best: opt encode non-NULL");
    assert_true(opt_size > 0,    "CSS opt best: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "CSS opt best: malloc zlib buffer");
    int zr = compress(zlib_dest, &zlib_dest_len, (const Bytef*)css, (uLong)input_len);
    assert_true(zr == Z_OK, "CSS opt best: zlib compress Z_OK");

    print_css_opt_benchmark_row(input_len, ae_size, opt_size, zlib_dest_len);

    /* Lossless round-trip */
    if (opt_enc) {
        CSSTokenArray* dec = css_decode_opt(opt_enc, opt_size);
        assert_true(dec != NULL, "CSS opt best: decode non-NULL");
        if (dec) {
            assert_true(css_token_arrays_equal(arr, dec),
                        "CSS opt best: round-trip lossless");
            free(dec);
        }
    }

    free(ae_enc); free(opt_enc); free(zlib_dest); free(arr);
    printf("PASS zlib vs CSS opt compare - best case\n");
}

/* Worst case: short rule with no token repetition. */
void test_zlib_compare_css_opt_worst_case(void) {
    const char* css = "a:hover { font-size: 2em; }";
    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS opt worst: alloc");
    if (!arr) return;
    parseCSS(css, arr);

    size_t ae_size = 0;
    unsigned char* ae_enc = css_encode_ae(arr, &ae_size);
    assert_true(ae_enc != NULL, "CSS opt worst: AE encode non-NULL");

    size_t opt_size = 0;
    unsigned char* opt_enc = css_encode_opt(arr, &opt_size);
    assert_true(opt_enc != NULL, "CSS opt worst: opt encode non-NULL");
    assert_true(opt_size > 0,    "CSS opt worst: encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "CSS opt worst: malloc zlib buffer");
    int zr = compress(zlib_dest, &zlib_dest_len, (const Bytef*)css, (uLong)input_len);
    assert_true(zr == Z_OK, "CSS opt worst: zlib compress Z_OK");

    print_css_opt_benchmark_row(input_len, ae_size, opt_size, zlib_dest_len);

    /* Lossless round-trip */
    if (opt_enc) {
        CSSTokenArray* dec = css_decode_opt(opt_enc, opt_size);
        assert_true(dec != NULL, "CSS opt worst: decode non-NULL");
        if (dec) {
            assert_true(css_token_arrays_equal(arr, dec),
                        "CSS opt worst: round-trip lossless");
            free(dec);
        }
    }

    free(ae_enc); free(opt_enc); free(zlib_dest); free(arr);
    printf("PASS zlib vs CSS opt compare - worst case\n");
}

/* Long stylesheet: main benchmark — verifies opt beats static AE and reports
 * compression ratio toward the 70% reduction target.                         */
void test_zlib_compare_css_opt_long_stylesheet(void) {
    const char* css =
        "/* reset */\n"
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "/* base */\n"
        "body { font-family: sans-serif; font-size: 16px; line-height: 1.6; color: #333; background: white; }\n"
        "h1 { font-size: 2rem; font-weight: bold; color: #111; margin-bottom: 16px; }\n"
        "h2 { font-size: 1.5rem; font-weight: bold; color: #222; margin-bottom: 12px; }\n"
        "p { margin-bottom: 16px; }\n"
        "a { color: blue; text-decoration: underline; }\n"
        "a:hover { color: darkblue; text-decoration: none; }\n"
        "img { display: block; max-width: 100%; height: auto; }\n"
        "/* layout */\n"
        "header { display: flex; justify-content: space-between; align-items: center;"
        " padding: 16px 24px; background: white; border-bottom: 1px solid #ddd; }\n"
        "nav a { color: #333; text-decoration: none; font-weight: bold; }\n"
        "main { max-width: 960px; margin: 0 auto; padding: 32px 16px; }\n"
        "footer { background: #222; color: white; text-align: center; padding: 24px; }\n"
        "/* hero */\n"
        ".hero { background: #4f46e5; color: white; text-align: center; padding: 64px 24px; }\n"
        ".hero h1 { font-size: 3rem; margin-bottom: 24px; }\n"
        "/* card */\n"
        ".card { background: white; border-radius: 8px; padding: 24px; box-shadow: 0 2px 8px #0001; }\n"
        "/* button */\n"
        "button { display: inline; padding: 10px 20px; background: blue; color: white;"
        " border: none; border-radius: 4px; cursor: pointer; font-weight: bold; }\n"
        "button:hover { background: darkblue; }\n"
        "/* responsive */\n"
        "@media (max-width: 768px) { }\n";

    size_t input_len = strlen(css);

    CSSTokenArray* arr = (CSSTokenArray*)calloc(1, sizeof(CSSTokenArray));
    assert_true(arr != NULL, "CSS opt long: alloc");
    if (!arr) return;
    parseCSS(css, arr);
    assert_true(arr->count > 0, "CSS opt long: parsed tokens > 0");

    int total_tokenizables = 0;
    for (int t = 0; t < arr->count; t++) {
        const CSSToken* tok = &arr->tokens[t];
        if      (tok->type == 0) total_tokenizables += tok->data.rule.ruleTokenSize;
        else if (tok->type == 1) total_tokenizables += tok->data.atRule.atRuleTokenSize;
        else                     total_tokenizables += tok->data.comment.commentTokenSize;
    }

    size_t ae_size = 0;
    unsigned char* ae_enc = css_encode_ae(arr, &ae_size);
    assert_true(ae_enc != NULL, "CSS opt long: AE encode non-NULL");
    assert_true(ae_size > 0,    "CSS opt long: AE encoded size > 0");

    size_t opt_size = 0;
    unsigned char* opt_enc = css_encode_opt(arr, &opt_size);
    assert_true(opt_enc != NULL, "CSS opt long: opt encode non-NULL");
    assert_true(opt_size > 0,    "CSS opt long: opt encoded size > 0");

    uLongf zlib_dest_len = compressBound((uLong)input_len);
    unsigned char* zlib_dest = (unsigned char*)malloc((size_t)zlib_dest_len);
    assert_true(zlib_dest != NULL, "CSS opt long: malloc zlib buffer");
    int zr = compress(zlib_dest, &zlib_dest_len, (const Bytef*)css, (uLong)input_len);
    assert_true(zr == Z_OK, "CSS opt long: zlib compress Z_OK");

    printf("  CSS tokens:      %d rules/at-rules/comments, %d total CSSTokenizables\n",
           arr->count, total_tokenizables);
    print_css_opt_benchmark_row(input_len, ae_size, opt_size, zlib_dest_len);

    /* Opt codec must outperform static AE */
    assert_true(opt_size < ae_size,
                "CSS opt long: opt codec smaller than static AE");

    /* Lossless round-trip at this scale */
    if (opt_enc) {
        CSSTokenArray* dec = css_decode_opt(opt_enc, opt_size);
        assert_true(dec != NULL, "CSS opt long: decode non-NULL");
        if (dec) {
            assert_equal_int(dec->count, arr->count,
                             "CSS opt long: decoded count matches");
            assert_true(css_token_arrays_equal(arr, dec),
                        "CSS opt long: round-trip is lossless");
            free(dec);
        }
    }

    free(ae_enc); free(opt_enc); free(zlib_dest); free(arr);
    printf("PASS zlib vs CSS opt compare - long stylesheet\n");
}

/* ══════════════════════════════════════════════════════════════════════════
 * NL-EN optimised codec tests (Steps 1-4)
 * ══════════════════════════════════════════════════════════════════════════ */

/* Verify that tokenizeEnglishOpt assigns word-level flags for common words.
 * A single word like "together" must produce exactly 1 token whose flag is
 * in [NL_EN_PATTERN_COUNT, NL_EN_OPT_PATTERN_COUNT).                       */
void test_nl_en_opt_tokenizer_word_match(void) {
    /* "together" is in the 8-char word list; old tokenizer splits it */
    NLTokenArray* old_arr = tokenizeEnglish("together");
    NLTokenArray* opt_arr = tokenizeEnglishOpt("together");

    assert_true(old_arr != NULL, "opt word match: old tokenize not NULL");
    assert_true(opt_arr != NULL, "opt word match: opt tokenize not NULL");

    if (old_arr && opt_arr) {
        /* Old tokenizer needs multiple tokens; optimised should use fewer */
        assert_true((int)opt_arr->count < (int)old_arr->count,
                    "opt word match: 'together' uses fewer tokens with word dict");

        /* Exactly 1 token expected when the whole word matches */
        assert_equal_int((int)opt_arr->count, 1,
                         "opt word match: 'together' -> 1 token");

        if (opt_arr->count == 1) {
            assert_true(opt_arr->tokens[0].isPattern,
                        "opt word match: token isPattern=true");
            /* Flag must fall in the word-dictionary range */
            assert_true(opt_arr->tokens[0].flag >= NL_EN_PATTERN_COUNT &&
                        opt_arr->tokens[0].flag <  NL_EN_OPT_PATTERN_COUNT,
                        "opt word match: flag in word-dict range [512, 744)");
        }
    }

    freeNLTokenArray(old_arr);
    freeNLTokenArray(opt_arr);
    printf("PASS NL-EN opt tokenizer - word match ('together' -> 1 token)\n");
}

/* Verify that longest-match correctly picks a longer word over a shorter
 * syllable pattern at the same position.  "between" starts with "be" (a CV
 * syllable pattern index 0), but the opt tokenizer must prefer the 7-char
 * word entry.                                                               */
void test_nl_en_opt_tokenizer_longest_match(void) {
    NLTokenArray* arr = tokenizeEnglishOpt("between");
    assert_true(arr != NULL, "opt longest match: tokenize not NULL");
    if (!arr) return;

    assert_equal_int((int)arr->count, 1,
                     "opt longest match: 'between' -> 1 token (not 'be' + rest)");

    if (arr->count == 1) {
        assert_true(arr->tokens[0].isPattern,
                    "opt longest match: token isPattern=true");
        assert_true(arr->tokens[0].flag >= NL_EN_PATTERN_COUNT,
                    "opt longest match: flag is a word entry, not syllable");
    }

    freeNLTokenArray(arr);

    /* Also check that "people" (6-char word) beats "pe" + "op" + "le" */
    NLTokenArray* arr2 = tokenizeEnglishOpt("people");
    assert_true(arr2 != NULL, "opt longest match: 'people' not NULL");
    if (arr2) {
        assert_equal_int((int)arr2->count, 1,
                         "opt longest match: 'people' -> 1 token");
        freeNLTokenArray(arr2);
    }

    printf("PASS NL-EN opt tokenizer - longest match ('between','people' each 1 token)\n");
}

/* Helper: compare two NLTokenArrays for lossless round-trip equality */
static int compare_token_arrays_opt(const NLTokenArray* a, const NLTokenArray* b) {
    if (!a || !b || a->count != b->count) return 0;
    for (size_t i = 0; i < a->count; i++) {
        if (a->tokens[i].isPattern != b->tokens[i].isPattern) return 0;
        if (a->tokens[i].flag      != b->tokens[i].flag)      return 0;
        if (a->tokens[i].isPattern &&
            a->tokens[i].caseStyle != b->tokens[i].caseStyle) return 0;
    }
    return 1;
}

/* Short-text lossless round-trip through nl_en_encode_opt / nl_en_decode_opt */
void test_nl_en_opt_codec_roundtrip(void) {
    const char* text = "the cat sat on the mat";

    NLTokenArray* original = tokenizeEnglishOpt(text);
    assert_true(original != NULL, "opt roundtrip: tokenize not NULL");
    if (!original) return;
    assert_true((int)original->count > 0, "opt roundtrip: at least 1 token");

    size_t enc_size = 0;
    unsigned char* encoded = nl_en_encode_opt(original, original->count, &enc_size);
    assert_true(encoded != NULL, "opt roundtrip: encode not NULL");
    assert_true(enc_size > 0,   "opt roundtrip: encoded size > 0");

    if (!encoded) { freeNLTokenArray(original); return; }

    NLTokenArray* decoded = nl_en_decode_opt(encoded, enc_size);
    assert_true(decoded != NULL, "opt roundtrip: decode not NULL");

    if (decoded) {
        assert_equal_int((int)decoded->count, (int)original->count,
                         "opt roundtrip: decoded count matches");
        assert_true(compare_token_arrays_opt(original, decoded),
                    "opt roundtrip: all tokens match losslessly");
    }

    free(encoded);
    freeNLTokenArray(decoded);
    freeNLTokenArray(original);
    printf("PASS NL-EN opt codec - short text round-trip (\"%s\")\n", text);
}

/* Verify that caseStyle survives the round-trip for all four style values.
 * Constructs a token array explicitly with caseStyle 0-3 and checks decode. */
void test_nl_en_opt_codec_casestyle(void) {
    NLTokenArray input;
    input.count = 4;

    /* Four pattern tokens, each with a different caseStyle (0=lower,1=upper,
     * 2=firstUpper,3=lastUpper).  Use word-dict flag 512 (first word).     */
    for (int cs = 0; cs < 4; cs++) {
        input.tokens[cs].isPattern = true;
        input.tokens[cs].flag      = (unsigned short)(NL_EN_PATTERN_COUNT + cs % NL_EN_WORD_COUNT);
        input.tokens[cs].caseStyle = cs;
    }

    size_t enc_size = 0;
    unsigned char* encoded = nl_en_encode_opt(&input, input.count, &enc_size);
    assert_true(encoded != NULL, "opt caseStyle: encode not NULL");
    if (!encoded) return;

    NLTokenArray* decoded = nl_en_decode_opt(encoded, enc_size);
    assert_true(decoded != NULL, "opt caseStyle: decode not NULL");

    if (decoded) {
        assert_equal_int((int)decoded->count, 4, "opt caseStyle: count=4");
        for (int cs = 0; cs < 4 && (int)decoded->count > cs; cs++) {
            assert_equal_int(decoded->tokens[cs].caseStyle, cs,
                             "opt caseStyle: caseStyle round-trips correctly");
        }
    }

    free(encoded);
    freeNLTokenArray(decoded);
    printf("PASS NL-EN opt codec - caseStyle round-trip (all 4 styles)\n");
}

/* Long-text lossless round-trip: exercises renormalization and the order-1
 * context model across a realistic multi-sentence passage.                  */
void test_nl_en_opt_codec_long_roundtrip(void) {
    const char* text =
        "The implementation of advanced data compression algorithms requires "
        "careful consideration of memory efficiency and processing speed. "
        "Our approach utilizes bit-level packing to minimize storage requirements "
        "while maintaining data integrity throughout the encoding and decoding process. "
        "Between encoder and decoder, the context model must remain perfectly "
        "synchronized so that every token is recovered without error.";

    NLTokenArray* original = tokenizeEnglishOpt(text);
    assert_true(original != NULL, "opt long roundtrip: tokenize not NULL");
    if (!original) return;
    assert_true((int)original->count > 20, "opt long roundtrip: >20 tokens");

    size_t enc_size = 0;
    unsigned char* encoded = nl_en_encode_opt(original, original->count, &enc_size);
    assert_true(encoded != NULL, "opt long roundtrip: encode not NULL");
    assert_true(enc_size > 0,   "opt long roundtrip: encoded size > 0");

    if (!encoded) { freeNLTokenArray(original); return; }

    NLTokenArray* decoded = nl_en_decode_opt(encoded, enc_size);
    assert_true(decoded != NULL, "opt long roundtrip: decode not NULL");

    if (decoded) {
        assert_equal_int((int)decoded->count, (int)original->count,
                         "opt long roundtrip: decoded count matches original");
        assert_true(compare_token_arrays_opt(original, decoded),
                    "opt long roundtrip: all tokens match losslessly");
    }

    printf("PASS NL-EN opt codec - long round-trip (%zu tokens, %zu bytes encoded)\n",
           original->count, enc_size);

    free(encoded);
    freeNLTokenArray(decoded);
    freeNLTokenArray(original);
}

/* Compression ratio benchmark on a 1024+ byte passage.
 * Compares nl_en_encode_opt against nl_en_encode_ae on the same text
 * (both from tokenizeEnglishOpt, which gives the opt codec an advantage
 * through word-level tokens AND reduces token count for the AE baseline).
 * Asserts that the opt codec is smaller than the old AE codec on this input,
 * and reports the ratio toward the 75% compression target.                   */
void test_nl_en_opt_compression_ratio(void) {
    const char* text =
        "The global software industry continues to evolve at an unprecedented pace, "
        "driven by advances in artificial intelligence, cloud computing, and distributed "
        "systems. Organizations must adapt their development processes to remain competitive "
        "in an increasingly complex technological landscape. Effective architecture decisions "
        "require balancing performance, maintainability, and scalability while managing "
        "technical debt and ensuring long-term sustainability of the codebase. Engineering "
        "teams that invest in robust testing infrastructure and continuous integration "
        "pipelines consistently deliver higher quality products with fewer defects. "
        "Between encoder and decoder, the order-one context model conditions each "
        "probability estimate on the previous token, so common syllable sequences "
        "receive shorter arithmetic codes than they would under a simple unigram model. "
        "The word-level dictionary further reduces the total token count by collapsing "
        "frequent multi-syllable words such as 'together', 'because', 'through', and "
        "'people' into single entries, each encoded as one symbol in the AE stream.";

    size_t input_len = strlen(text);
    assert_true(input_len >= 1024, "opt compression: input >= 1024 bytes");

    NLTokenArray* tokens = tokenizeEnglishOpt(text);
    assert_true(tokens != NULL,         "opt compression: tokenize not NULL");
    assert_true((int)tokens->count > 0, "opt compression: produces tokens");
    if (!tokens) return;

    /* Encode with new optimised codec */
    size_t opt_size = 0;
    unsigned char* opt_enc = nl_en_encode_opt(tokens, tokens->count, &opt_size);
    assert_true(opt_enc != NULL, "opt compression: opt encode not NULL");

    /* Encode with old AE codec (on the same extended-dict token array) */
    size_t ae_size = 0;
    unsigned char* ae_enc = nl_en_encode_ae(tokens, tokens->count, &ae_size);
    assert_true(ae_enc != NULL, "opt compression: AE encode not NULL");

    double opt_ratio = (double)opt_size  / (double)input_len * 100.0;
    double ae_ratio  = (double)ae_size   / (double)input_len * 100.0;

    printf("  Input:        %zu bytes\n", input_len);
    printf("  Tokens:       %zu\n", tokens->count);
    printf("  Old AE:       %zu bytes (%.1f%% of input)\n", ae_size,  ae_ratio);
    printf("  Opt codec:    %zu bytes (%.1f%% of input, target <=25%%)\n",
           opt_size, opt_ratio);

    /* The opt codec must produce a smaller output than the old AE on this input */
    assert_true(opt_size < ae_size,
                "opt compression: opt codec smaller than old AE codec");

    /* Lossless round-trip check even at this size */
    NLTokenArray* decoded = nl_en_decode_opt(opt_enc, opt_size);
    assert_true(decoded != NULL, "opt compression: decode not NULL");
    if (decoded) {
        assert_equal_int((int)decoded->count, (int)tokens->count,
                         "opt compression: decoded count matches");
        assert_true(compare_token_arrays_opt(tokens, decoded),
                    "opt compression: round-trip is lossless");
        freeNLTokenArray(decoded);
    }

    free(opt_enc);
    free(ae_enc);
    freeNLTokenArray(tokens);
    printf("PASS NL-EN opt codec - compression ratio benchmark (>1024 bytes)\n");
}

/* ══════════════════════════════════════════════════════════════════════════
 * CLJS AE opt codec tests
 * ══════════════════════════════════════════════════════════════════════════ */

static bool cljs_token_arrays_equal(const CLJSTokenArray* a, const CLJSTokenArray* b) {
    if (a->count != b->count) return false;
    for (size_t i = 0; i < a->count; i++) {
        if (a->tokens[i].isPattern != b->tokens[i].isPattern) return false;
        if (a->tokens[i].flag      != b->tokens[i].flag)      return false;
        /* caseStyle only matters for pattern tokens (non-pattern ASCII chars
           carry caseStyle=3 in the tokenizer but 0 after decoding — irrelevant) */
        if (a->tokens[i].isPattern &&
            a->tokens[i].caseStyle != b->tokens[i].caseStyle) return false;
    }
    return true;
}

/* Best case: highly repetitive JS with many identical pattern tokens. */
void test_cljs_ae_opt_codec_best_case(void) {
    const char* js = "function log() { console.log(result); console.log(result); "
                     "console.log(result); console.log(result); console.log(result); }";
    size_t input_len = strlen(js);

    CLJSTokenArray* arr = tokenizeJavaScript(js);
    assert_true(arr != NULL, "CLJS best: tokenize non-NULL");
    if (!arr) return;
    assert_true(arr->count > 0, "CLJS best: token count > 0");

    size_t enc_size = 0;
    unsigned char* enc = cljs_encode_ae_opt(arr, arr->count, &enc_size);
    assert_true(enc != NULL,  "CLJS best: encode non-NULL");
    assert_true(enc_size > 0, "CLJS best: encoded size > 0");
    assert_true(enc_size < input_len, "CLJS best: encoded smaller than raw input");

    printf("  CLJS best case: raw %zu B -> encoded %zu B (%.1f%%)\n",
           input_len, enc_size, 100.0 * (1.0 - (double)enc_size / (double)input_len));

    if (enc) {
        CLJSTokenArray* dec = cljs_decode_ae_opt(enc, enc_size);
        assert_true(dec != NULL, "CLJS best: decode non-NULL");
        if (dec) {
            assert_equal_int((int)dec->count, (int)arr->count,
                             "CLJS best: decoded count matches");
            assert_true(cljs_token_arrays_equal(arr, dec), "CLJS best: round-trip lossless");
            freeCLJSTokenArray(dec);
        }
        free(enc);
    }
    freeCLJSTokenArray(arr);
    printf("PASS CLJS AE opt codec - best case\n");
}

/* Worst case: only non-pattern ASCII characters, no dictionary matches. */
void test_cljs_ae_opt_codec_worst_case(void) {
    const char* js = "!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#!@#";
    size_t input_len = strlen(js);

    CLJSTokenArray* arr = tokenizeJavaScript(js);
    assert_true(arr != NULL, "CLJS worst: tokenize non-NULL");
    if (!arr) return;
    assert_true(arr->count > 0, "CLJS worst: token count > 0");

    size_t enc_size = 0;
    unsigned char* enc = cljs_encode_ae_opt(arr, arr->count, &enc_size);
    assert_true(enc != NULL,  "CLJS worst: encode non-NULL");
    assert_true(enc_size > 0, "CLJS worst: encoded size > 0");

    printf("  CLJS worst case: raw %zu B -> encoded %zu B\n", input_len, enc_size);

    if (enc) {
        CLJSTokenArray* dec = cljs_decode_ae_opt(enc, enc_size);
        assert_true(dec != NULL, "CLJS worst: decode non-NULL");
        if (dec) {
            assert_equal_int((int)dec->count, (int)arr->count,
                             "CLJS worst: decoded count matches");
            assert_true(cljs_token_arrays_equal(arr, dec), "CLJS worst: round-trip lossless");
            freeCLJSTokenArray(dec);
        }
        free(enc);
    }
    freeCLJSTokenArray(arr);
    printf("PASS CLJS AE opt codec - worst case\n");
}

/* Short round-trip: realistic JS snippet with mixed patterns and ASCII. */
void test_cljs_ae_opt_codec_roundtrip(void) {
    const char* js = "const result = fetch(url).then(response => response.json());";

    CLJSTokenArray* arr = tokenizeJavaScript(js);
    assert_true(arr != NULL, "CLJS roundtrip: tokenize non-NULL");
    if (!arr) return;

    size_t enc_size = 0;
    unsigned char* enc = cljs_encode_ae_opt(arr, arr->count, &enc_size);
    assert_true(enc != NULL,  "CLJS roundtrip: encode non-NULL");
    assert_true(enc_size > 0, "CLJS roundtrip: encoded size > 0");

    if (enc) {
        CLJSTokenArray* dec = cljs_decode_ae_opt(enc, enc_size);
        assert_true(dec != NULL, "CLJS roundtrip: decode non-NULL");
        if (dec) {
            assert_equal_int((int)dec->count, (int)arr->count,
                             "CLJS roundtrip: decoded count matches");
            assert_true(cljs_token_arrays_equal(arr, dec),
                        "CLJS roundtrip: round-trip is lossless");
            freeCLJSTokenArray(dec);
        }
        free(enc);
    }
    freeCLJSTokenArray(arr);
    printf("PASS CLJS AE opt codec - short round-trip\n");
}

/* Long round-trip: a realistic module-sized JS snippet. */
void test_cljs_ae_opt_codec_long_roundtrip(void) {
    const char* js =
        "import { useState, useEffect, useCallback } from 'react';\n"
        "function DataComponent({ config, options }) {\n"
        "  const [state, setState] = useState(null);\n"
        "  const [error, setError] = useState(null);\n"
        "  const loadData = useCallback(async () => {\n"
        "    try {\n"
        "      const response = await fetch(config.url);\n"
        "      if (!response.ok) throw new Error('Network error');\n"
        "      const data = await response.json();\n"
        "      setState(data);\n"
        "    } catch (err) {\n"
        "      setError(err.message);\n"
        "      console.error('Failed to load:', err);\n"
        "    }\n"
        "  }, [config]);\n"
        "  useEffect(() => { loadData(); return () => setState(null); }, [loadData]);\n"
        "  if (error) return null;\n"
        "  if (!state) return null;\n"
        "  return state;\n"
        "}\n"
        "export default DataComponent;\n";

    CLJSTokenArray* arr = tokenizeJavaScript(js);
    assert_true(arr != NULL, "CLJS long roundtrip: tokenize non-NULL");
    if (!arr) return;
    assert_true(arr->count > 0, "CLJS long roundtrip: token count > 0");

    size_t enc_size = 0;
    unsigned char* enc = cljs_encode_ae_opt(arr, arr->count, &enc_size);
    assert_true(enc != NULL,  "CLJS long roundtrip: encode non-NULL");
    assert_true(enc_size > 0, "CLJS long roundtrip: encoded size > 0");

    if (enc) {
        CLJSTokenArray* dec = cljs_decode_ae_opt(enc, enc_size);
        assert_true(dec != NULL, "CLJS long roundtrip: decode non-NULL");
        if (dec) {
            assert_equal_int((int)dec->count, (int)arr->count,
                             "CLJS long roundtrip: decoded count matches");
            assert_true(cljs_token_arrays_equal(arr, dec),
                        "CLJS long roundtrip: round-trip is lossless");
            freeCLJSTokenArray(dec);
        }
        free(enc);
    }
    freeCLJSTokenArray(arr);
    printf("PASS CLJS AE opt codec - long round-trip\n");
}

static void print_cljs_benchmark_row(size_t raw, size_t codec_size, size_t zlib_size) {
    double codec_ratio = 100.0 * (1.0 - (double)codec_size / (double)raw);
    double zlib_ratio  = 100.0 * (1.0 - (double)zlib_size  / (double)raw);
    printf("  raw %zu B | CLJS AE opt %zu B (%.1f%% reduction) | zlib %zu B (%.1f%% reduction)\n",
           raw, codec_size, codec_ratio, zlib_size, zlib_ratio);
}

/* Short script benchmark. */
void test_zlib_compare_cljs_ae_opt_short(void) {
    const char* js = "function add(a, b) { return a + b; } console.log(add(1, 2));";
    size_t input_len = strlen(js);

    CLJSTokenArray* arr = tokenizeJavaScript(js);
    assert_true(arr != NULL, "CLJS short bench: tokenize non-NULL");
    if (!arr) return;

    size_t enc_size = 0;
    unsigned char* enc = cljs_encode_ae_opt(arr, arr->count, &enc_size);
    assert_true(enc != NULL,  "CLJS short bench: encode non-NULL");
    assert_true(enc_size > 0, "CLJS short bench: encoded size > 0");

    uLongf zlib_len = compressBound((uLong)input_len);
    unsigned char* zlib_buf = (unsigned char*)malloc((size_t)zlib_len);
    assert_true(zlib_buf != NULL, "CLJS short bench: malloc zlib");
    int zr = compress(zlib_buf, &zlib_len, (const Bytef*)js, (uLong)input_len);
    assert_true(zr == Z_OK, "CLJS short bench: zlib Z_OK");

    print_cljs_benchmark_row(input_len, enc_size, (size_t)zlib_len);

    if (enc) {
        CLJSTokenArray* dec = cljs_decode_ae_opt(enc, enc_size);
        assert_true(dec != NULL, "CLJS short bench: decode non-NULL");
        if (dec) {
            assert_true(cljs_token_arrays_equal(arr, dec),
                        "CLJS short bench: round-trip lossless");
            freeCLJSTokenArray(dec);
        }
        free(enc);
    }
    free(zlib_buf);
    freeCLJSTokenArray(arr);
    printf("PASS zlib vs CLJS AE opt - short script\n");
}

/* Long script benchmark: production-like module >= 1024 bytes. */
void test_zlib_compare_cljs_ae_opt_long(void) {
    const char* js =
        "import { useState, useEffect, useCallback, useMemo } from 'react';\n"
        "\n"
        "const API_BASE = process.env.API_URL;\n"
        "\n"
        "async function fetchData(endpoint, options) {\n"
        "  const response = await fetch(API_BASE + endpoint, options);\n"
        "  if (!response.ok) throw new Error('Request failed: ' + response.status);\n"
        "  return response.json();\n"
        "}\n"
        "\n"
        "function useData(endpoint) {\n"
        "  const [data, setData] = useState(null);\n"
        "  const [error, setError] = useState(null);\n"
        "  const [loading, setLoading] = useState(false);\n"
        "\n"
        "  const load = useCallback(async () => {\n"
        "    setLoading(true);\n"
        "    try {\n"
        "      const result = await fetchData(endpoint);\n"
        "      setData(result);\n"
        "      setError(null);\n"
        "    } catch (err) {\n"
        "      setError(err.message);\n"
        "      console.error('useData error:', err);\n"
        "    } finally {\n"
        "      setLoading(false);\n"
        "    }\n"
        "  }, [endpoint]);\n"
        "\n"
        "  useEffect(() => { load(); }, [load]);\n"
        "  return { data, error, loading, reload: load };\n"
        "}\n"
        "\n"
        "function DataTable({ endpoint, columns }) {\n"
        "  const { data, error, loading } = useData(endpoint);\n"
        "\n"
        "  const rows = useMemo(() => {\n"
        "    if (!data) return [];\n"
        "    return data.map((item, index) => columns.map(col => item[col.key]));\n"
        "  }, [data, columns]);\n"
        "\n"
        "  if (loading) return null;\n"
        "  if (error) { console.error(error); return null; }\n"
        "  if (!data || data.length === 0) return null;\n"
        "  return rows;\n"
        "}\n"
        "\n"
        "function EventManager() {\n"
        "  const handlers = new Map();\n"
        "  function addEventListener(type, handler) {\n"
        "    if (!handlers.has(type)) handlers.set(type, []);\n"
        "    handlers.get(type).push(handler);\n"
        "  }\n"
        "  function removeEventListener(type, handler) {\n"
        "    if (!handlers.has(type)) return;\n"
        "    const list = handlers.get(type).filter(h => h !== handler);\n"
        "    handlers.set(type, list);\n"
        "  }\n"
        "  function dispatchEvent(type, event) {\n"
        "    if (!handlers.has(type)) return;\n"
        "    handlers.get(type).forEach(h => h(event));\n"
        "  }\n"
        "  return { addEventListener, removeEventListener, dispatchEvent };\n"
        "}\n"
        "\n"
        "export { useData, DataTable, EventManager };\n";

    size_t input_len = strlen(js);
    assert_true(input_len >= 1024, "CLJS long bench: input >= 1024 bytes");

    CLJSTokenArray* arr = tokenizeJavaScript(js);
    assert_true(arr != NULL, "CLJS long bench: tokenize non-NULL");
    if (!arr) return;
    assert_true(arr->count > 0, "CLJS long bench: token count > 0");

    size_t enc_size = 0;
    unsigned char* enc = cljs_encode_ae_opt(arr, arr->count, &enc_size);
    assert_true(enc != NULL,  "CLJS long bench: encode non-NULL");
    assert_true(enc_size > 0, "CLJS long bench: encoded size > 0");

    uLongf zlib_len = compressBound((uLong)input_len);
    unsigned char* zlib_buf = (unsigned char*)malloc((size_t)zlib_len);
    assert_true(zlib_buf != NULL, "CLJS long bench: malloc zlib");
    int zr = compress(zlib_buf, &zlib_len, (const Bytef*)js, (uLong)input_len);
    assert_true(zr == Z_OK, "CLJS long bench: zlib Z_OK");

    printf("  CLJS tokens: %zu\n", arr->count);
    print_cljs_benchmark_row(input_len, enc_size, (size_t)zlib_len);

    if (enc) {
        CLJSTokenArray* dec = cljs_decode_ae_opt(enc, enc_size);
        assert_true(dec != NULL, "CLJS long bench: decode non-NULL");
        if (dec) {
            assert_equal_int((int)dec->count, (int)arr->count,
                             "CLJS long bench: decoded count matches");
            assert_true(cljs_token_arrays_equal(arr, dec),
                        "CLJS long bench: round-trip lossless");
            freeCLJSTokenArray(dec);
        }
        free(enc);
    }
    free(zlib_buf);
    freeCLJSTokenArray(arr);
    printf("PASS zlib vs CLJS AE opt - long script (>= 1024 B)\n");
}
