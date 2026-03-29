# HTML/CSS Tokenizer - Quick Reference Guide

## Using the HTML Tokenizer

```c
#include "html-tokenizer.h"

// Parse HTML
HTMLTokenArray* result = parseHTML("<div id=\"test\">Hello World</div>");

// Access tokens
for (int i = 0; i < result->count; i++) {
    HTMLToken* token = &result->tokens[i];
    if (token->type == 0) {
        // Text token
        printf("Text: %s\n", token->data.text.content);
    } else if (token->type == 1) {
        // Opening tag
        printf("Tag: %s\n", token->data.tag.name);
    } else if (token->type == 2) {
        // Closing tag
        printf("End: %s\n", token->data.tag.name);
    }
}

// Cleanup
freeHTMLTokenArray(result);
```

## Using the CSS Tokenizer

```c
#include "css-tokenizer.h"

// Parse CSS
CSSTokenArray result;
const char* css = "body { color: red; background: white; }";
parseCSS(css, &result);

// Access tokens
for (int i = 0; i < result.count; i++) {
    CSSToken* token = &result.tokens[i];
    if (token->type == 0) {
        // Selector rule
        printf("Selector: %s\n", token->data.rule.selector);
        for (int j = 0; j < token->data.rule.propertyCount; j++) {
            printf("  %s: %s\n", 
                token->data.rule.properties[j].name,
                token->data.rule.properties[j].value);
        }
    } else if (token->type == 1) {
        // At-rule
        printf("At-rule: %s\n", token->data.atRule.rule);
    } else if (token->type == 2) {
        // Comment
        printf("Comment: %s\n", token->data.comment.content);
    }
}

// Cleanup
freeCSS(&result);
```

## Key API Differences

### HTML Tokenizer
- Returns dynamically allocated `HTMLTokenArray*` (use `free()`)
- Token types: 0=text, 1=open-tag, 2=close-tag
- Handles attributes via `HTMLAttribute` array

### CSS Tokenizer
- Takes pre-allocated `CSSTokenArray*` parameter
- Token types: 0=selector-rule, 1=at-rule, 2=comment
- Parses CSS properties into `CSSProperty` arrays

## Constants

### HTML Tokenizer
- `HTML_MAX_TOKENS`: 1000
- `HTML_MAX_TAG_NAME`: 64
- `HTML_MAX_ATTR_COUNT`: 32
- `HTML_MAX_ATTR_NAME`: 32
- `HTML_MAX_ATTR_VALUE`: 256
- `HTML_MAX_TEXT_CONTENT`: 512

### CSS Tokenizer
- `CSS_MAX_TOKENS`: 1000
- `CSS_MAX_SELECTOR_LEN`: 256
- `CSS_MAX_PROPERTY_NAME`: 64
- `CSS_MAX_PROPERTY_VALUE`: 512
- `CSS_MAX_PROPERTIES`: 128

## Running Tests

The test suite can be executed by running the compiled executable:
```bash
htmlcodec-c.exe
```

Output will show:
- Pass/Fail status for each test
- Total passed and failed counts
- Organized by HTML and CSS tokenizer sections
