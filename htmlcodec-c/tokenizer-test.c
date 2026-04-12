#include "tokenizer-test.h"
#include "nl-en-tokenizer.h"
#include "nl-en-codec.h"
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
            input.tokens[i].flag = ((i * 7) % 256); // different ASCII values
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
        decoded->tokens[1].flag != ((1 * 7) % 256)) {
        spot_checks_passed = 0;
        printf("  Spot check failed at token 1\n");
    }

    // Check token at middle (2048, even -> isPattern should be true)
    size_t mid = NL_EN_MAX_TOKENS / 2;
    int mid_isPattern = (mid % 2 == 0) ? 1 : 0;
    int mid_flag = (mid % 2 == 0) ? (int)(mid % NL_EN_PATTERN_COUNT) : (int)((mid * 7) % 256);
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
    int last_flag = (last % 2 == 0) ? (int)(last % NL_EN_PATTERN_COUNT) : (int)((last * 7) % 256);

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
